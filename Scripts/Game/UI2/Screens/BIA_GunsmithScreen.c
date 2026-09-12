//! GUNSMITH tab: the draft's weapon on its own stage in a fixed hero pose. Selecting a hardpoint
//! glides the camera onto the mount and opens the compatible-candidates strip; picking one swaps
//! it into the draft, the stage re-clones with the new build, and the callouts relabel. The
//! receiver card bottom-right opens the weapon browser to switch the staged weapon; the stats
//! block bottom-left reads honest engine data off the staged build. Manual orbit stays on drag.
class BIA_GunsmithScreen : SCR_SubMenuBase
{
	protected static const int STRIP_HIDDEN = 0;
	protected static const int STRIP_ATTACHMENTS = 1;
	protected static const int STRIP_WEAPONS = 2;
	protected static const int STRIP_MAGAZINES = 3;

	protected RenderTargetWidget m_wStageWorld;
	protected Widget m_wCalloutRoot;
	protected TextWidget m_wCounter;

	protected ref BIA_WeaponStage m_Stage;
	protected SCR_InputButtonComponent m_WearChip;
	protected ref BIA_CalloutLayer m_CalloutLayer;
	protected ref BIA_StatsBlock m_Stats;
	protected ref BIA_TileStrip m_Strip;
	protected ref BIA_ItemListPanel m_Contents;
	protected Widget m_wContentsAction;
	protected SCR_ButtonTextComponent m_ContentsButton;
	protected ref BIA_ReceiverBrowser m_Receiver;
	protected ref BIA_HardpointRail m_Rail;
	protected ref array<ref BIA_ItemEntry> m_aAttachmentPool = {};

	protected ResourceName m_StagedPrefab;
	protected int m_iStagedWeaponSlot = -1;
	protected int m_iStagedClothingSlot = -1;
	protected string m_sSyncedAttachments;
	protected int m_iSelectedSlot = -1;
	protected int m_iStripMode = STRIP_HIDDEN;
	protected int m_iPreferredSlot = -1;
	protected ref array<ResourceName> m_aMagazineContainers = {};

	protected static BIA_GunsmithScreen s_ActiveInstance;
	protected static bool s_bPendingClothingInspection;
	protected static int s_iPendingClothingSlot;

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//! The one character-mutation verb: WEAR applies the current draft through the server pipeline;
	//! the shell's shared status line reports the verdict.
	protected void OnWearChip(SCR_InputButtonComponent chip, string action)
	{
		BIA_DraftService service = BIA_DraftService.Get();
		if (service)
			service.RequestApplyDraft();
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);

		m_wStageWorld = RenderTargetWidget.Cast(m_wRoot.FindAnyWidget("StageWorld"));
		if (!m_wStageWorld)
			m_wStageWorld = BIA_WeaponStage.CreateFallbackRender(m_wRoot);
		m_wCalloutRoot = m_wRoot.FindAnyWidget("CalloutLayer");
		m_wCounter = TextWidget.Cast(m_wRoot.FindAnyWidget("HardpointCounter"));

		m_Stage = BIA_StageHub.Get().GetWeapon();
		m_CalloutLayer = new BIA_CalloutLayer(m_wCalloutRoot, m_Stage);

		m_WearChip = CreateNavigationButton("MenuSave", "Wear", true, true);
		if (m_WearChip)
			m_WearChip.m_OnActivated.Insert(OnWearChip);
		m_CalloutLayer.m_OnSlotClicked.Insert(OnSlotClicked);

		m_Stats = new BIA_StatsBlock(m_wRoot);
		m_Strip = new BIA_TileStrip(m_wRoot);
		m_Strip.m_OnTileClicked.Insert(OnStripTileClicked);
		m_Strip.m_OnClassClicked.Insert(OnStripClassClicked);

		m_Receiver = new BIA_ReceiverBrowser(m_wRoot, m_Strip);
		m_Receiver.m_OnCardClicked.Insert(OpenWeaponBrowser);
		m_Receiver.m_OnWeaponPicked.Insert(OnWeaponPicked);

		m_Contents = new BIA_ItemListPanel(m_wRoot, "ItemListPanel", "ItemListTitle", "ItemList", "ItemScroll", "ItemSearchBox", "ItemListBackControls", "ItemListFilters");
		m_Contents.m_OnItemClicked.Insert(OnContentsItemClicked);
		m_Contents.m_OnQtyDelta.Insert(OnContentsQuantityChanged);
		m_Contents.m_OnDone.Insert(OnContentsDone);
		m_wContentsAction = m_wRoot.FindAnyWidget("ContentsAction");
		m_ContentsButton = SCR_ButtonTextComponent.GetButtonText("ContentsButton", m_wRoot);
		if (m_ContentsButton)
		{
			m_ContentsButton.SetText("OPEN CONTENTS");
			m_ContentsButton.m_OnClicked.Insert(OnContentsButtonClicked);
		}

		m_Rail = new BIA_HardpointRail(m_wRoot, m_CalloutLayer);
		m_Rail.m_OnRowClicked.Insert(OnRailRowClicked);
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabShow()
	{
		super.OnTabShow();

		s_ActiveInstance = this;
		m_iStagedClothingSlot = -1;
		if (s_bPendingClothingInspection)
		{
			m_iStagedClothingSlot = s_iPendingClothingSlot;
			s_bPendingClothingInspection = false;
		}

		BIA_DraftService service = BIA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Insert(RefreshStage);

		if (m_Rail)
			m_Rail.Enable();

		//! Front the shared studio through this tab's render node before any staging — an empty
		//! bench is a valid scene — then glide the camera over to the bench station.
		if (m_Stage && m_wStageWorld)
		{
			m_Stage.SetDraftClothingSlot(m_iStagedClothingSlot);
			m_Stage.ShowOn(m_wStageWorld);
		}
		RefreshStage();
		BIA_Theme.Apply(m_wRoot);
		if (m_Stage)
			m_Stage.FrameStation();
	}

	//------------------------------------------------------------------------------------------------
	//! Lets the shell's back action step out of the strip or a selected hardpoint first — only an
	//! empty-handed back should close the whole editor.
	static bool ConsumeBack()
	{
		BIA_GunsmithScreen screen = s_ActiveInstance;
		if (!screen)
			return false;

		if (screen.m_Contents && screen.m_Contents.IsOpen())
		{
			screen.CloseContents();
			return true;
		}

		bool consumed;
		if (screen.m_iStripMode != STRIP_HIDDEN)
		{
			screen.CloseStrip();
			if (screen.m_Rail && screen.m_Rail.IsPadMode() && screen.m_iSelectedSlot < 0)
				screen.m_Rail.FocusFirstRow();
			consumed = true;
		}

		if (screen.m_iSelectedSlot >= 0)
		{
			int wasSelected = screen.m_iSelectedSlot;
			screen.m_iSelectedSlot = -1;
			if (screen.m_CalloutLayer)
				screen.m_CalloutLayer.SetLineSelection(-1);
			if (screen.m_Stage)
				screen.m_Stage.GoHome();
			if (screen.m_Rail && screen.m_Rail.IsPadMode())
				screen.m_Rail.FocusRow(wasSelected);
			consumed = true;
		}

		return consumed;
	}

	//------------------------------------------------------------------------------------------------
	static bool HasActiveAttachmentEdit()
	{
		BIA_GunsmithScreen screen = s_ActiveInstance;
		return screen && screen.m_iStripMode == STRIP_ATTACHMENTS
			&& screen.m_iSelectedSlot >= 0 && screen.HasStagedDraftItem();
	}

	//------------------------------------------------------------------------------------------------
	static void QueueClothingInspection(int clothingSlot)
	{
		s_iPendingClothingSlot = clothingSlot;
		s_bPendingClothingInspection = clothingSlot >= 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Attachment choices already live in the draft. Accepting the transition closes the editor
	//! cleanly while preserving those choices for Soldier, Kits, Settings and Wear.
	static void SaveAndCloseAttachmentEdit()
	{
		BIA_GunsmithScreen screen = s_ActiveInstance;
		if (!screen)
			return;

		int selectedSlot = screen.m_iSelectedSlot;
		screen.CloseStrip();
		screen.m_iSelectedSlot = -1;
		if (screen.m_CalloutLayer)
			screen.m_CalloutLayer.SetLineSelection(-1);
		if (screen.m_Stage)
			screen.m_Stage.GoHome();
		if (screen.m_Rail && screen.m_Rail.IsPadMode() && selectedSlot >= 0)
			screen.m_Rail.FocusRow(selectedSlot);
	}

	//------------------------------------------------------------------------------------------------
	//! Returns keyboard or gamepad focus from the hidden confirmation prompt to the open editor.
	static void RestoreAttachmentEditFocus()
	{
		BIA_GunsmithScreen screen = s_ActiveInstance;
		if (!screen || screen.m_iStripMode != STRIP_ATTACHMENTS)
			return;

		if (screen.m_Rail && screen.m_iSelectedSlot >= 0)
			screen.m_Rail.FocusRow(screen.m_iSelectedSlot);
		if (screen.m_Strip)
			screen.m_Strip.FocusFirstTile();
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabHide()
	{
		super.OnTabHide();

		if (s_ActiveInstance == this)
			s_ActiveInstance = null;

		BIA_DraftService service = BIA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Remove(RefreshStage);

		GetGame().GetCallqueue().Remove(RefreshOpenBrowser);
		GetGame().GetCallqueue().Remove(ApplyCandidateChange);
		GetGame().GetCallqueue().Remove(CloseContents);
		CloseContents();
		CloseStrip();
		if (m_Rail)
			m_Rail.Disable();
		if (m_Stats)
			m_Stats.Hide();
		if (m_CalloutLayer)
			m_CalloutLayer.Clear();
		bool wasClothingInspection = m_iStagedClothingSlot >= 0;
		if (m_Stage)
		{
			m_Stage.SetDraftClothingSlot(-1);
			if (wasClothingInspection)
				m_Stage.ClearStage();
		}

		//! The weapon stays on the bench — it reads as the backdrop from the soldier station; the
		//! screen-side staging state resets so the next show rebuilds callouts and strips.
		m_StagedPrefab = ResourceName.Empty;
		m_iStagedWeaponSlot = -1;
		m_iStagedClothingSlot = -1;
		m_sSyncedAttachments = string.Empty;
		m_iSelectedSlot = -1;
		m_iPreferredSlot = -1;
		m_aAttachmentPool.Clear();
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabRemove()
	{
		GetGame().GetCallqueue().Remove(ApplyCandidateChange);
		GetGame().GetCallqueue().Remove(CloseContents);
		if (m_ContentsButton)
			m_ContentsButton.m_OnClicked.Remove(OnContentsButtonClicked);
		if (m_Contents)
		{
			m_Contents.Close();
			m_Contents.m_OnItemClicked.Remove(OnContentsItemClicked);
			m_Contents.m_OnQtyDelta.Remove(OnContentsQuantityChanged);
			m_Contents.m_OnDone.Remove(OnContentsDone);
			m_Contents.Destroy();
			m_Contents = null;
		}
		super.OnTabRemove();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		if (m_CalloutLayer)
			m_CalloutLayer.Reposition();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshStage()
	{
		BIA_DraftService service = BIA_DraftService.Get();
		if (!service || !service.m_Draft || !m_wStageWorld || !m_Stage)
			return;

		//! The visible screen owns the shared render target. Reassert it before processing a draft
		//! change so a resident background host can never leave input bound to a hidden tab.
		m_Stage.ShowOn(m_wStageWorld);

		ResourceName draftPrefab;
		array<ResourceName> draftAttachments;
		array<int> draftPins;
		int draftWeaponSlot = -1;
		if (m_iStagedClothingSlot >= 0)
		{
			BIA_KitClothing draftClothing = service.m_Draft.FindClothing(m_iStagedClothingSlot);
			if (draftClothing)
			{
				draftClothing.EnsurePins();
				draftPrefab = draftClothing.m_Prefab;
				draftAttachments = draftClothing.m_aAttachments;
				draftPins = draftClothing.m_aAttachmentSlots;
			}
		}
		else
		{
			BIA_KitWeapon draftWeapon = PreferredDraftWeapon(service);
			if (draftWeapon)
			{
				draftWeapon.EnsurePins();
				draftPrefab = draftWeapon.m_Prefab;
				draftAttachments = draftWeapon.m_aAttachments;
				draftPins = draftWeapon.m_aAttachmentSlots;
				draftWeaponSlot = draftWeapon.m_iSlotIdx;
			}
		}

		if (draftPrefab.IsEmpty() || !draftAttachments || !draftPins)
		{
			CloseContents();
			CloseStrip();
			if (m_CalloutLayer)
				m_CalloutLayer.Clear();
			m_Stage.ClearStage();
			m_StagedPrefab = ResourceName.Empty;
			m_iStagedWeaponSlot = -1;
			m_sSyncedAttachments = string.Empty;
			m_iSelectedSlot = -1;
			RefreshCounter();
			if (m_Receiver)
				m_Receiver.SetStaged(ResourceName.Empty, -1);
			if (m_Rail)
				m_Rail.Rebuild(null);
			RefreshOverlayPanels();
			return;
		}

		string attachmentsSignature = BuildAttachmentsSignature(draftAttachments, draftPins);
		bool samePrefab = draftPrefab == m_StagedPrefab && m_Stage.IsAlive();

		if (samePrefab && attachmentsSignature == m_sSyncedAttachments)
		{
			RefreshContentsCounts(service);
			if (m_iStagedClothingSlot < 0 && m_iStripMode == STRIP_MAGAZINES)
				RefreshMagazineCounts(service);
			return;
		}

		IEntity slotSource;
		if (samePrefab)
		{
			//! Build change on the staged item: mutate the pooled source, re-clone, relabel.
			slotSource = m_Stage.SyncAttachments(draftAttachments, draftPins);
		}
		else
		{
			slotSource = m_Stage.ShowWeapon(draftPrefab, m_wStageWorld);
			if (slotSource)
				slotSource = m_Stage.SyncAttachments(draftAttachments, draftPins);
			m_iSelectedSlot = -1;
			//! An open receiver browser survives a weapon swap; every attachment strip belongs to
			//! the old staged item's hardpoints and folds.
			if (m_iStripMode != STRIP_WEAPONS)
				CloseStrip();
			RefreshAttachmentPool(service);
		}

		if (!slotSource)
		{
			return;
		}

		m_StagedPrefab = draftPrefab;
		m_iStagedWeaponSlot = draftWeaponSlot;
		m_sSyncedAttachments = attachmentsSignature;

		if (m_CalloutLayer)
			m_CalloutLayer.Build(slotSource, service.GetBrowseFaction());
		RefreshCounter();
		if (m_Receiver)
		{
			if (m_iStagedClothingSlot >= 0)
				m_Receiver.SetStaged(ResourceName.Empty, -1);
			else
				m_Receiver.SetStaged(m_StagedPrefab, m_iStagedWeaponSlot);

			//! Deferred: the pick that triggered this refresh is still walking the old tile's
			//! click chain — rebuilding the tiles under it would tear the invoking row down.
			if (m_iStagedClothingSlot < 0 && m_iStripMode == STRIP_WEAPONS)
			{
				GetGame().GetCallqueue().Remove(RefreshOpenBrowser);
				GetGame().GetCallqueue().CallLater(RefreshOpenBrowser, 50, false);
			}
		}
		if (m_Rail)
			m_Rail.Rebuild(service.GetBrowseFaction(), m_iStagedClothingSlot >= 0);
		RefreshOverlayPanels();

		if (m_CalloutLayer)
		{
			if (m_iSelectedSlot >= 0 && !m_CalloutLayer.GetEntry(m_iSelectedSlot))
				m_iSelectedSlot = -1;
			m_CalloutLayer.SetLineSelection(m_iSelectedSlot);
		}

		//! A swap keeps the hardpoint selected: refresh the open strip's mounted markers, and on
		//! pad reseed tile focus — the rebuilt tiles destroyed the previously focused widget.
		if (m_iSelectedSlot >= 0)
		{
			ShowCandidates(m_iSelectedSlot);
			if (m_Rail && m_Rail.IsPadMode() && m_Strip)
				m_Strip.FocusFirstTile();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasStagedDraftItem()
	{
		return m_iStagedWeaponSlot >= 0 || m_iStagedClothingSlot >= 0;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshAttachmentPool(notnull BIA_DraftService service)
	{
		m_aAttachmentPool.Clear();
		map<ResourceName, bool> seen = new map<ResourceName, bool>();
		array<ref BIA_ItemEntry> tabItems = {};
		if (m_iStagedClothingSlot >= 0)
		{
			BIA_CatalogService.CollectClothingMountItems(service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType(), tabItems);
			AppendAttachmentPool(tabItems, seen);
		}

		BIA_CatalogService.CollectTabItems(service.m_Config, BIA_EArmoryTab.GEAR, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType(), tabItems);
		AppendAttachmentPool(tabItems, seen);
		BIA_CatalogService.CollectTabItems(service.m_Config, BIA_EArmoryTab.WEAPONS, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType(), tabItems);
		AppendAttachmentPool(tabItems, seen);
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendAttachmentPool(notnull array<ref BIA_ItemEntry> source, notnull map<ResourceName, bool> seen)
	{
		foreach (BIA_ItemEntry item : source)
		{
			if (!item || seen.Contains(item.m_Prefab))
				continue;

			seen.Insert(item.m_Prefab, true);
			m_aAttachmentPool.Insert(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Chip toggle: select glides onto the mount and opens candidates, re-select glides home.
	protected void OnSlotClicked(int index)
	{
		CloseContents();
		if (!m_Stage)
			return;

		if (index == m_iSelectedSlot)
		{
			m_iSelectedSlot = -1;
			if (m_CalloutLayer)
				m_CalloutLayer.SetLineSelection(-1);
			CloseStrip();
			m_Stage.GoHome();
			return;
		}

		vector offset;
		if (m_CalloutLayer && m_CalloutLayer.GetOffset(index, offset))
		{
			m_iSelectedSlot = index;
			m_CalloutLayer.SetLineSelection(index);
			m_Stage.FocusPoint(offset);
			ShowCandidates(index);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowCandidates(int index)
	{
		if (!m_Strip || !m_CalloutLayer)
			return;

		BIA_CalloutEntry entry = m_CalloutLayer.GetEntry(index);
		if (!entry)
			return;

		if (m_Receiver)
			m_Receiver.CloseBrowser();
		m_Strip.HideClasses();

		BIA_DraftService service = BIA_DraftService.Get();
		bool usesSupplies = service && service.UsesSupplies();

		if (entry.m_bMagazine && m_iStagedClothingSlot < 0)
		{
			m_iStripMode = STRIP_MAGAZINES;
			array<typename> wells = {};
			if (m_Stage)
				DCO_ArsenalCompat.GetWeaponMagWells(m_Stage.SlotSource(), wells);

			array<BIA_ItemEntry> compatibleMagazines = {};
			foreach (BIA_ItemEntry item : m_aAttachmentPool)
			{
				if (item && DCO_ArsenalCompat.IsMagazinePrefab(item.m_Prefab) && DCO_ArsenalCompat.MagazineFitsAny(item.m_Prefab, wells))
					compatibleMagazines.Insert(item);
			}

			string title = "MAGAZINES";
			if (service)
				title += " - " + service.GetTargetContainerDisplayName();
			m_Strip.ShowTiles(title, compatibleMagazines, ResourceName.Empty, string.Empty, usesSupplies);
			BuildMagazineContainerChips(service);
			RefreshMagazineCounts(service);
			RefreshOverlayPanels();
			return;
		}

		m_iStripMode = STRIP_ATTACHMENTS;

		if (!entry.m_SlotTypename && m_iStagedClothingSlot < 0)
		{
			m_Strip.ShowMessage(entry.m_sTypeLabel);
			RefreshOverlayPanels();
			return;
		}

		BaseInventoryStorageComponent mountStorage = BIA_ItemIntel.AttachmentStorage(m_Stage.SlotSource());
		InventoryStorageSlot mountSlot;
		if (mountStorage) mountSlot = mountStorage.GetSlot(entry.m_iStorageSlot);
		array<BIA_ItemEntry> compatible = {};
		foreach (BIA_ItemEntry item : m_aAttachmentPool)
		{
			if (item && BIA_ItemIntel.MountAccepts(mountSlot, item.m_Prefab))
				compatible.Insert(item);
		}

		if (compatible.IsEmpty())
			m_Strip.ShowMessage(entry.m_sTypeLabel + " - NO COMPATIBLE PARTS AVAILABLE");
		else
			m_Strip.ShowTiles(entry.m_sTypeLabel, compatible, entry.m_AttachedPrefab, "MOUNTED", usesSupplies);
		RefreshOverlayPanels();
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildMagazineContainerChips(BIA_DraftService service)
	{
		m_aMagazineContainers.Clear();
		m_aMagazineContainers.Insert(ResourceName.Empty);

		array<string> labels = {};
		labels.Insert("AUTO");
		int selected = 0;
		if (service)
		{
			array<ResourceName> containers = {};
			service.GetDraftContainers(containers);
			foreach (ResourceName container : containers)
			{
				m_aMagazineContainers.Insert(container);
				string label = service.GetContainerDisplayName(container);
				label.ToUpper();
				labels.Insert(label);
			}

			selected = m_aMagazineContainers.Find(service.GetTargetContainer());
			if (selected < 0)
				selected = 0;
		}

		m_Strip.ShowClasses(labels, selected);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshMagazineCounts(BIA_DraftService service)
	{
		if (!service || !service.m_Draft || !m_Strip)
			return;

		ResourceName target = service.GetTargetContainer();
		map<ResourceName, int> counts = new map<ResourceName, int>();
		foreach (BIA_KitExtra extra : service.m_Draft.m_aExtras)
		{
			if (extra.m_Container == target)
				counts.Set(extra.m_Prefab, extra.m_iCount);
		}

		m_Strip.SetCounts(counts);
		m_Strip.SetTitle("MAGAZINES - " + service.GetTargetContainerDisplayName());
	}

	//------------------------------------------------------------------------------------------------
	//! Rail activation mirrors a chip click; a drill-in that opened the strip seeds tile focus so
	//! pad navigation lands inside it.
	protected void OnRailRowClicked(int index)
	{
		OnSlotClicked(index);
		if (m_Rail && m_Rail.IsPadMode() && m_iStripMode != STRIP_HIDDEN && m_Strip)
			m_Strip.FocusFirstTile();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnStripTileClicked(BIA_ItemRowComponent row)
	{
		if (m_Receiver && m_Receiver.HandleTileClicked(row))
			return;

		OnCandidateClicked(row);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnStripClassClicked(int index)
	{
		if (m_iStripMode == STRIP_MAGAZINES)
		{
			if (index < 0 || index >= m_aMagazineContainers.Count())
				return;

			BIA_DraftService service = BIA_DraftService.Get();
			if (!service)
				return;

			service.SetTargetContainer(m_aMagazineContainers[index]);
			ShowCandidates(m_iSelectedSlot);
			return;
		}

		if (m_Receiver && m_iStripMode == STRIP_WEAPONS)
			m_Receiver.ShowClass(index);
	}

	//------------------------------------------------------------------------------------------------
	//! Mount on click; clicking the mounted item unmounts it. The draft change loops back through
	//! RefreshStage, which re-clones the stage and refreshes the strip.
	protected void OnCandidateClicked(BIA_ItemRowComponent row)
	{
		if (!row || !row.GetEntry() || m_iSelectedSlot < 0 || !HasStagedDraftItem())
			return;

		BIA_DraftService service = BIA_DraftService.Get();
		if (!service || !m_CalloutLayer)
			return;

		BIA_CalloutEntry entry = m_CalloutLayer.GetEntry(m_iSelectedSlot);
		if (!entry)
			return;

		ResourceName clicked = row.GetEntry().m_Prefab;
		if (entry.m_bMagazine && m_iStagedClothingSlot < 0)
		{
			ResourceName target = service.GetTargetContainer();
			BIA_EExtraChangeResult result = service.ChangeDraftExtraTo(clicked, 1, target);
			if (result != BIA_EExtraChangeResult.ADDED)
			{
				m_Strip.SetTitle("MAGAZINES - " + service.GetTargetContainerDisplayName() + " - " + ExtraRejectLabel(result));
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK_FAIL);
				return;
			}

			BIA_ShellMenu.ShowStatus("ITEM ADDED", true);
			RefreshMagazineCounts(service);
			return;
		}

		//! Candidate swaps rebuild this strip. Defer the draft mutation until the active tile's input
		//! invoker has returned, and carry values rather than the row that will be destroyed.
		GetGame().GetCallqueue().Remove(ApplyCandidateChange);
		GetGame().GetCallqueue().CallLater(ApplyCandidateChange, 0, false,
			m_iSelectedSlot, m_iStagedClothingSlot, m_iStagedWeaponSlot,
			entry.m_AttachedPrefab, clicked, entry.m_iStorageSlot);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyCandidateChange(int selectedSlot, int clothingSlot, int weaponSlot,
		ResourceName previous, ResourceName clicked, int storageSlot)
	{
		if (m_iSelectedSlot != selectedSlot || m_iStagedClothingSlot != clothingSlot
			|| m_iStagedWeaponSlot != weaponSlot || !m_CalloutLayer)
			return;

		BIA_CalloutEntry entry = m_CalloutLayer.GetEntry(selectedSlot);
		if (!entry || entry.m_iStorageSlot != storageSlot || entry.m_AttachedPrefab != previous)
			return;

		BIA_DraftService service = BIA_DraftService.Get();
		if (!service)
			return;

		ResourceName replacement = clicked;
		if (clicked == previous)
			replacement = ResourceName.Empty;

		if (clothingSlot >= 0)
			service.SwapDraftClothingAttachment(clothingSlot, previous, replacement, storageSlot);
		else
			service.SwapDraftAttachment(weaponSlot, previous, replacement);

		//! The draft event resyncs the stage synchronously; when the engine refuses the mount,
		//! undo the swap instead of leaving the slot stripped, and say so the base-game way.
		if (!replacement.IsEmpty() && m_Stage && m_Stage.GetUnplaced().Contains(replacement))
		{
			if (clothingSlot >= 0)
				service.SwapDraftClothingAttachment(clothingSlot, replacement, previous, storageSlot);
			else
				service.SwapDraftAttachment(weaponSlot, replacement, previous);
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK_FAIL);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Receiver card entry point: folds any hardpoint selection, hands the strip to the browser.
	protected void OpenWeaponBrowser()
	{
		if (!m_Receiver || m_iStagedClothingSlot >= 0)
			return;

		m_iSelectedSlot = -1;
		if (m_CalloutLayer)
			m_CalloutLayer.SetLineSelection(-1);
		m_iStripMode = STRIP_WEAPONS;
		m_Receiver.OpenBrowser();
		if (!m_Receiver.IsBrowsing())
			m_iStripMode = STRIP_HIDDEN;
		RefreshOverlayPanels();
	}

	//------------------------------------------------------------------------------------------------
	//! Picking a receiver seeds that weapon slot's draft with the prefab's factory attachments and
	//! stages it; the browser stays open so receivers can be flipped through back to back.
	protected void OnWeaponPicked(int weaponSlot, ResourceName prefab)
	{
		BIA_DraftService service = BIA_DraftService.Get();
		if (!service)
			return;

		m_iPreferredSlot = weaponSlot;
		service.SetDraftWeapon(weaponSlot, prefab);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshOpenBrowser()
	{
		if (m_Receiver && m_iStripMode == STRIP_WEAPONS)
		{
			m_Receiver.RefreshBrowser();
			if (m_Rail && m_Rail.IsPadMode() && m_Strip)
				m_Strip.FocusFirstTile();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseStrip()
	{
		m_iStripMode = STRIP_HIDDEN;
		m_aMagazineContainers.Clear();
		if (m_Receiver)
			m_Receiver.CloseBrowser();
		if (m_Strip)
			m_Strip.Hide();
		RefreshOverlayPanels();
	}

	//------------------------------------------------------------------------------------------------
	//! Stats and the receiver card share the bottom band with the strip; they yield while it is open.
	protected void RefreshOverlayPanels()
	{
		bool stripHidden = m_iStripMode == STRIP_HIDDEN;
		bool clothingInspection = m_iStagedClothingSlot >= 0;
		if (m_wContentsAction)
			m_wContentsAction.SetVisible(clothingInspection && (!m_Contents || !m_Contents.IsOpen()));
		if (m_Rail && m_Strip) m_Rail.ReserveCandidateSpace(m_Strip.GetVisibleHeight());

		if (m_Stats)
		{
			if (stripHidden && !clothingInspection && !m_StagedPrefab.IsEmpty())
				RefreshStats();
			else
				m_Stats.Hide();
		}

		if (m_Receiver)
			m_Receiver.SetCardVisible(stripHidden && !clothingInspection);
	}

	//------------------------------------------------------------------------------------------------

	protected void OnContentsButtonClicked(SCR_ButtonBaseComponent button)
	{
		BIA_DraftService service = BIA_DraftService.Get();
		if (!service || !service.m_Draft || m_iStagedClothingSlot < 0 || !m_Contents)
			return;
		BIA_KitClothing clothing = service.m_Draft.FindClothing(m_iStagedClothingSlot);
		if (!clothing || clothing.m_Prefab.IsEmpty())
			return;
		m_iSelectedSlot = -1;
		if (m_CalloutLayer) m_CalloutLayer.SetLineSelection(-1);
		CloseStrip();
		array<ref BIA_ItemEntry> items = {};
		BIA_CatalogService.CollectContentsItems(service, items);
		m_Contents.Open("CONTENTS", items, ResourceName.Empty, string.Empty, service.UsesSupplies(), true);
		RefreshContentsCounts(service);
		RefreshOverlayPanels();
	}

	protected void OnContentsDone()
	{
		GetGame().GetCallqueue().Remove(CloseContents);
		GetGame().GetCallqueue().CallLater(CloseContents, 0, false);
	}

	protected void CloseContents()
	{
		GetGame().GetCallqueue().Remove(CloseContents);
		if (m_Contents) m_Contents.Close();
		RefreshOverlayPanels();
	}

	protected void OnContentsItemClicked(BIA_ItemRowComponent row)
	{
		OnContentsQuantityChanged(row, 1);
	}

	protected void OnContentsQuantityChanged(BIA_ItemRowComponent row, int delta)
	{
		if (!row || !row.GetEntry() || !m_Contents || !m_Contents.IsOpen())
			return;
		BIA_DraftService service = BIA_DraftService.Get();
		if (!service || !service.m_Draft)
			return;
		BIA_KitClothing clothing = service.m_Draft.FindClothing(m_iStagedClothingSlot);
		if (!clothing || clothing.m_Prefab.IsEmpty())
			return;
		ResourceName item = row.GetEntry().m_Prefab;
		IEntity preview;
		if (m_Stage) preview = m_Stage.SlotSource();
		BIA_EExtraChangeResult result = service.ChangeDraftExtraTo(item, delta, clothing.m_Prefab, preview);
		RefreshContentsCounts(service);
		if (result == BIA_EExtraChangeResult.ADDED)
			BIA_ShellMenu.ShowStatus("ITEM ADDED", true);
		else if (result != BIA_EExtraChangeResult.REMOVED)
		{
			m_Contents.SetTitle("CONTENTS - " + ExtraRejectLabel(result));
			if (delta > 0) SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK_FAIL);
		}
	}

	protected void RefreshContentsCounts(BIA_DraftService service)
	{
		if (!m_Contents || !m_Contents.IsOpen() || !service || !service.m_Draft)
			return;
		BIA_KitClothing clothing = service.m_Draft.FindClothing(m_iStagedClothingSlot);
		if (!clothing || clothing.m_Prefab.IsEmpty())
		{
			CloseContents();
			return;
		}
		map<ResourceName, int> counts = new map<ResourceName, int>();
		foreach (BIA_KitExtra extra : service.m_Draft.m_aExtras)
		{
			if (extra.m_Container == clothing.m_Prefab)
				counts.Set(extra.m_Prefab, extra.m_iCount);
		}
		m_Contents.SetCounts(counts);
		float weight, volume;
		service.GetContainerUsage(clothing.m_Prefab, weight, volume);
		string title = "CONTENTS - " + service.GetContainerDisplayName(clothing.m_Prefab);
		title += " - " + weight.ToString(-1, 1) + " KG";
		IEntity stagedContainer;
		if (m_Stage)
			stagedContainer = m_Stage.SlotSource();
		float maxVolume = BIA_ItemIntel.GetLiveStorageMaxVolume(stagedContainer);
		if (maxVolume > 0)
			title += " - " + volume.ToString(-1, 1) + " / " + maxVolume.ToString(-1, 1) + " CAPACITY";
		m_Contents.SetTitle(title);
	}

	protected void RefreshStats()
	{
		BIA_DraftService service = BIA_DraftService.Get();
		if (!service || !m_Stats || !m_Stage || m_iStagedClothingSlot >= 0)
			return;

		IEntity source = m_Stage.SlotSource();
		if (!source)
		{
			m_Stats.Hide();
			return;
		}

		BIA_ItemEntry entry;
		if (m_Receiver)
			entry = m_Receiver.GetStagedEntry();
		m_Stats.Refresh(source, m_StagedPrefab, entry, BIA_CatalogService.GetDisplayName(m_StagedPrefab, service.GetBrowseFaction()), service.GetLocalCharacter());
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshCounter()
	{
		if (!m_wCounter)
			return;
		Widget panel = m_wRoot.FindAnyWidget("HardpointCounterPanel");
		if (panel) panel.SetVisible(m_CalloutLayer && m_CalloutLayer.GetTotalCount() > 0);

		if (!m_CalloutLayer || m_CalloutLayer.GetTotalCount() == 0)
		{
			m_wCounter.SetText(string.Empty);
			return;
		}

		m_wCounter.SetTextFormat("ATTACHMENTS %1 / %2", m_CalloutLayer.GetMountedCount(), m_CalloutLayer.GetTotalCount());
	}

	//------------------------------------------------------------------------------------------------
	//! The slot the player last picked a weapon into stays the staged one while it holds a weapon;
	//! otherwise the first armed slot stages, the original behavior.
	protected BIA_KitWeapon PreferredDraftWeapon(notnull BIA_DraftService service)
	{
		if (m_iPreferredSlot >= 0)
		{
			BIA_KitWeapon preferred = service.m_Draft.FindWeapon(m_iPreferredSlot);
			if (preferred && !preferred.m_Prefab.IsEmpty())
				return preferred;
		}

		foreach (BIA_KitWeapon weapon : service.m_Draft.m_aWeapons)
		{
			if (!weapon.m_Prefab.IsEmpty())
				return weapon;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Pins are part of the signature: a position change on an unchanged part set must still
	//! resync the stage.
	protected string BuildAttachmentsSignature(notnull array<ResourceName> attachments, notnull array<int> pins)
	{
		string signature;
		foreach (int i, ResourceName attachment : attachments)
			signature += attachment + ":" + pins[i].ToString() + ";";
		return signature;
	}

	//------------------------------------------------------------------------------------------------
	protected string ExtraRejectLabel(BIA_EExtraChangeResult result)
	{
		switch (result)
		{
			case BIA_EExtraChangeResult.VOLUME_LIMIT:
				return "NO SPACE";
			case BIA_EExtraChangeResult.WEIGHT_LIMIT:
				return "TOO HEAVY";
			case BIA_EExtraChangeResult.INCOMPATIBLE:
				return "DOES NOT FIT";
		}

		return "UNAVAILABLE";
	}
}
