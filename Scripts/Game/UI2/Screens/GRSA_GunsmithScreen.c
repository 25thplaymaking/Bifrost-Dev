//! GUNSMITH tab: the draft's weapon on its own stage in a fixed hero pose. Selecting a hardpoint
//! glides the camera onto the mount and opens the compatible-candidates strip; picking one swaps
//! it into the draft, the stage re-clones with the new build, and the callouts relabel. The
//! receiver card bottom-right opens the weapon browser to switch the staged weapon; the stats
//! block bottom-left reads honest engine data off the staged build. Manual orbit stays on drag.
class GRSA_GunsmithScreen : SCR_SubMenuBase
{
	protected static const int STRIP_HIDDEN = 0;
	protected static const int STRIP_ATTACHMENTS = 1;
	protected static const int STRIP_WEAPONS = 2;
	protected static const int STRIP_MAGAZINES = 3;

	protected RenderTargetWidget m_wStageWorld;
	protected Widget m_wCalloutRoot;
	protected TextWidget m_wCounter;
	protected Widget m_wPositionRow;
	protected TextWidget m_wPositionValue;
	protected SCR_SliderComponent m_PositionSlider;

	protected ref GRSA_WeaponStage m_Stage;
	protected SCR_InputButtonComponent m_WearChip;
	protected ref GRSA_CalloutLayer m_CalloutLayer;
	protected ref GRSA_StatsBlock m_Stats;
	protected ref GRSA_TileStrip m_Strip;
	protected ref GRSA_ReceiverBrowser m_Receiver;
	protected ref GRSA_HardpointRail m_Rail;
	protected ref array<ref GRSA_ItemEntry> m_aAttachmentPool = {};

	protected ResourceName m_StagedPrefab;
	protected int m_iStagedWeaponSlot = -1;
	protected int m_iStagedClothingSlot = -1;
	protected string m_sSyncedAttachments;
	protected int m_iSelectedSlot = -1;
	protected int m_iStripMode = STRIP_HIDDEN;
	protected int m_iPreferredSlot = -1;
	protected ref array<int> m_aSliderSlots = {};
	protected ref array<ResourceName> m_aMagazineContainers = {};
	protected bool m_bSyncingSlider;

	//! After a position change the selection follows the moved item onto its new hardpoint.
	protected ResourceName m_FollowAttachment;

	protected static GRSA_GunsmithScreen s_ActiveInstance;
	protected static bool s_bPendingClothingInspection;
	protected static int s_iPendingClothingSlot;

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//! The one character-mutation verb: WEAR applies the current draft through the server pipeline;
	//! the shell's shared status line reports the verdict.
	protected void OnWearChip(SCR_InputButtonComponent chip, string action)
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.RequestApplyDraft();
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);

		m_wStageWorld = RenderTargetWidget.Cast(m_wRoot.FindAnyWidget("StageWorld"));
		if (!m_wStageWorld)
			m_wStageWorld = GRSA_WeaponStage.CreateFallbackRender(m_wRoot);
		m_wCalloutRoot = m_wRoot.FindAnyWidget("CalloutLayer");
		m_wCounter = TextWidget.Cast(m_wRoot.FindAnyWidget("HardpointCounter"));
		m_wPositionRow = m_wRoot.FindAnyWidget("PositionRow");
		m_wPositionValue = TextWidget.Cast(m_wRoot.FindAnyWidget("PositionValue"));
		Widget sliderRoot = m_wRoot.FindAnyWidget("PositionSlider");
		if (sliderRoot)
		{
			m_PositionSlider = SCR_SliderComponent.Cast(sliderRoot.FindHandler(SCR_SliderComponent));
			if (m_PositionSlider)
				m_PositionSlider.m_OnChanged.Insert(OnPositionSliderChanged);
		}

		m_Stage = GRSA_StageHub.Get().GetWeapon();
		m_CalloutLayer = new GRSA_CalloutLayer(m_wCalloutRoot, m_Stage);

		m_WearChip = CreateNavigationButton("MenuSave", "Wear", true, true);
		if (m_WearChip)
			m_WearChip.m_OnActivated.Insert(OnWearChip);
		m_CalloutLayer.m_OnSlotClicked.Insert(OnSlotClicked);

		m_Stats = new GRSA_StatsBlock(m_wRoot);
		m_Strip = new GRSA_TileStrip(m_wRoot);
		m_Strip.m_OnTileClicked.Insert(OnStripTileClicked);
		m_Strip.m_OnClassClicked.Insert(OnStripClassClicked);

		m_Receiver = new GRSA_ReceiverBrowser(m_wRoot, m_Strip);
		m_Receiver.m_OnCardClicked.Insert(OpenWeaponBrowser);
		m_Receiver.m_OnWeaponPicked.Insert(OnWeaponPicked);

		m_Rail = new GRSA_HardpointRail(m_wRoot, m_CalloutLayer);
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

		GRSA_DraftService service = GRSA_DraftService.Get();
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
		GRSA_Theme.Apply(m_wRoot);
		if (m_Stage)
			m_Stage.FrameStation();
	}

	//------------------------------------------------------------------------------------------------
	//! Lets the shell's back action step out of the strip or a selected hardpoint first — only an
	//! empty-handed back should close the whole editor.
	static bool ConsumeBack()
	{
		GRSA_GunsmithScreen screen = s_ActiveInstance;
		if (!screen)
			return false;

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
		GRSA_GunsmithScreen screen = s_ActiveInstance;
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
		GRSA_GunsmithScreen screen = s_ActiveInstance;
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
		GRSA_GunsmithScreen screen = s_ActiveInstance;
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

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Remove(RefreshStage);

		GetGame().GetCallqueue().Remove(RefreshOpenBrowser);
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
	override void OnMenuUpdate(float tDelta)
	{
		if (m_CalloutLayer)
			m_CalloutLayer.Reposition();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshStage()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
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
			GRSA_KitClothing draftClothing = service.m_Draft.FindClothing(m_iStagedClothingSlot);
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
			GRSA_KitWeapon draftWeapon = PreferredDraftWeapon(service);
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
			return;

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
			m_Rail.Rebuild(service.GetBrowseFaction());
		RefreshOverlayPanels();

		//! A position change rebuilt the callouts around the moved item: the selection follows it
		//! onto its new hardpoint and the camera glides along.
		if (!m_FollowAttachment.IsEmpty() && m_CalloutLayer)
		{
			int followIdx = m_CalloutLayer.FindEntryByAttachment(m_FollowAttachment);
			m_FollowAttachment = ResourceName.Empty;
			if (followIdx >= 0)
			{
				m_iSelectedSlot = followIdx;
				vector followOffset;
				if (m_CalloutLayer.GetOffset(followIdx, followOffset))
					m_Stage.FocusPoint(followOffset);
			}
		}

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
	protected void RefreshAttachmentPool(notnull GRSA_DraftService service)
	{
		m_aAttachmentPool.Clear();
		map<ResourceName, bool> seen = new map<ResourceName, bool>();
		array<ref GRSA_ItemEntry> tabItems = {};

		GRSA_CatalogService.CollectTabItems(service.m_Config, GRSA_EArmoryTab.GEAR, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType(), tabItems);
		AppendAttachmentPool(tabItems, seen);
		GRSA_CatalogService.CollectTabItems(service.m_Config, GRSA_EArmoryTab.WEAPONS, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType(), tabItems);
		AppendAttachmentPool(tabItems, seen);
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendAttachmentPool(notnull array<ref GRSA_ItemEntry> source, notnull map<ResourceName, bool> seen)
	{
		foreach (GRSA_ItemEntry item : source)
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

		GRSA_CalloutEntry entry = m_CalloutLayer.GetEntry(index);
		if (!entry)
			return;

		if (m_Receiver)
			m_Receiver.CloseBrowser();
		m_Strip.HideClasses();
		HidePositionSlider();

		GRSA_DraftService service = GRSA_DraftService.Get();
		bool usesSupplies = service && service.UsesSupplies();

		if (entry.m_bMagazine && m_iStagedClothingSlot < 0)
		{
			m_iStripMode = STRIP_MAGAZINES;
			array<typename> wells = {};
			if (m_Stage)
				DCO_ArsenalCompat.GetWeaponMagWells(m_Stage.SlotSource(), wells);

			array<GRSA_ItemEntry> compatibleMagazines = {};
			foreach (GRSA_ItemEntry item : m_aAttachmentPool)
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

		if (!entry.m_SlotTypename)
		{
			m_Strip.ShowMessage(entry.m_sTypeLabel);
			RefreshOverlayPanels();
			return;
		}

		array<GRSA_ItemEntry> compatible = {};
		foreach (GRSA_ItemEntry item : m_aAttachmentPool)
		{
			if (item && GRSA_ItemIntel.SlotAcceptsItem(entry.m_SlotTypename, item.m_Prefab))
				compatible.Insert(item);
		}

		m_Strip.ShowTiles(entry.m_sTypeLabel, compatible, entry.m_AttachedPrefab, "MOUNTED", usesSupplies);
		RefreshPositionSlider(entry);
		RefreshOverlayPanels();
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildMagazineContainerChips(GRSA_DraftService service)
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
	protected void RefreshMagazineCounts(GRSA_DraftService service)
	{
		if (!service || !service.m_Draft || !m_Strip)
			return;

		ResourceName target = service.GetTargetContainer();
		map<ResourceName, int> counts = new map<ResourceName, int>();
		foreach (GRSA_KitExtra extra : service.m_Draft.m_aExtras)
		{
			if (extra.m_Container == target)
				counts.Set(extra.m_Prefab, extra.m_iCount);
		}

		m_Strip.SetCounts(counts);
		m_Strip.SetTitle("MAGAZINES - " + service.GetTargetContainerDisplayName());
	}

	//------------------------------------------------------------------------------------------------
	//! The Operator-style position control: a mounted item whose weapon offers more than one
	//! type-compatible hardpoint gets a slider that walks it along those stations. Steps are the
	//! compatible slots sorted by their authored inspection anchor, so the slider reads as a
	//! physical move along the weapon.
	protected void RefreshPositionSlider(notnull GRSA_CalloutEntry entry)
	{
		if (!m_wPositionRow || !m_PositionSlider)
			return;

		m_aSliderSlots.Clear();
		if (!entry.m_AttachedPrefab.IsEmpty() && entry.m_SlotTypename && m_Stage)
			CollectCompatibleSlots(entry, m_aSliderSlots);

		int current = m_aSliderSlots.Find(entry.m_iStorageSlot);
		if (m_aSliderSlots.Count() < 2 || current < 0)
		{
			HidePositionSlider();
			return;
		}

		m_bSyncingSlider = true;
		m_PositionSlider.SetSliderSettings(0, m_aSliderSlots.Count() - 1, 1);
		m_PositionSlider.SetValue(current);
		m_bSyncingSlider = false;

		if (m_wPositionValue)
			m_wPositionValue.SetTextFormat("%1 / %2", current + 1, m_aSliderSlots.Count());

		m_wPositionRow.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	protected void HidePositionSlider()
	{
		if (m_wPositionRow)
			m_wPositionRow.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Top-level hardpoints the mounted item could occupy: type-compatible and empty (or its own),
	//! ordered along the weapon by the slots' authored inspection anchors.
	protected void CollectCompatibleSlots(notnull GRSA_CalloutEntry entry, notnull array<int> outSlots)
	{
		IEntity source = m_Stage.SlotSource();
		if (!source)
			return;

		WeaponAttachmentsStorageComponent storage = WeaponAttachmentsStorageComponent.Cast(source.FindComponent(WeaponAttachmentsStorageComponent));
		if (!storage)
			return;

		array<float> keys = {};
		int count = storage.GetSlotsCount();
		for (int i = 0; i < count; ++i)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			if (!slot)
				continue;

			AttachmentSlotComponent attachment = AttachmentSlotComponent.Cast(slot.GetParentContainer());
			if (!attachment || !attachment.GetAttachmentSlotType())
				continue;

			if (!GRSA_ItemIntel.SlotAcceptsItem(attachment.GetAttachmentSlotType().Type(), entry.m_AttachedPrefab))
				continue;

			if (slot.GetAttachedEntity() && i != entry.m_iStorageSlot)
				continue;

			vector anchor = slot.GetInspectionWidgetOffset();
			float key = anchor[2];
			int insertAt = outSlots.Count();
			for (int k = 0; k < keys.Count(); ++k)
			{
				if (key < keys[k])
				{
					insertAt = k;
					break;
				}
			}
			outSlots.InsertAt(i, insertAt);
			keys.InsertAt(key, insertAt);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPositionSliderChanged(SCR_SliderComponent slider, float value)
	{
		if (m_bSyncingSlider || m_iSelectedSlot < 0 || !HasStagedDraftItem() || !m_CalloutLayer)
			return;

		GRSA_CalloutEntry entry = m_CalloutLayer.GetEntry(m_iSelectedSlot);
		if (!entry || entry.m_AttachedPrefab.IsEmpty())
			return;

		int stepIdx = Math.Round(value);
		if (stepIdx < 0 || stepIdx >= m_aSliderSlots.Count())
			return;

		int targetSlot = m_aSliderSlots[stepIdx];
		if (targetSlot == entry.m_iStorageSlot)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
			return;

		m_FollowAttachment = entry.m_AttachedPrefab;
		if (m_iStagedClothingSlot >= 0)
			service.SetDraftClothingAttachmentPin(m_iStagedClothingSlot, entry.m_AttachedPrefab, targetSlot, entry.m_iStorageSlot);
		else
			service.SetDraftAttachmentPin(m_iStagedWeaponSlot, entry.m_AttachedPrefab, targetSlot);
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
	protected void OnStripTileClicked(GRSA_ItemRowComponent row)
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

			GRSA_DraftService service = GRSA_DraftService.Get();
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
	protected void OnCandidateClicked(GRSA_ItemRowComponent row)
	{
		if (!row || !row.GetEntry() || m_iSelectedSlot < 0 || !HasStagedDraftItem())
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !m_CalloutLayer)
			return;

		GRSA_CalloutEntry entry = m_CalloutLayer.GetEntry(m_iSelectedSlot);
		if (!entry)
			return;

		ResourceName clicked = row.GetEntry().m_Prefab;
		if (entry.m_bMagazine && m_iStagedClothingSlot < 0)
		{
			ResourceName target = service.GetTargetContainer();
			GRSA_EExtraChangeResult result = service.ChangeDraftExtraTo(clicked, 1, target);
			if (result != GRSA_EExtraChangeResult.ADDED)
			{
				m_Strip.SetTitle("MAGAZINES - " + service.GetTargetContainerDisplayName() + " - " + ExtraRejectLabel(result));
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK_FAIL);
				return;
			}

			GRSA_ShellMenu.ShowStatus("ITEM ADDED", true);
			RefreshMagazineCounts(service);
			return;
		}

		if (clicked == entry.m_AttachedPrefab)
		{
			if (m_iStagedClothingSlot >= 0)
				service.SwapDraftClothingAttachment(m_iStagedClothingSlot, clicked, ResourceName.Empty, entry.m_iStorageSlot);
			else
				service.SwapDraftAttachment(m_iStagedWeaponSlot, clicked, ResourceName.Empty);
			return;
		}

		ResourceName previous = entry.m_AttachedPrefab;
		if (m_iStagedClothingSlot >= 0)
			service.SwapDraftClothingAttachment(m_iStagedClothingSlot, previous, clicked, entry.m_iStorageSlot);
		else
			service.SwapDraftAttachment(m_iStagedWeaponSlot, previous, clicked);

		//! The draft event resyncs the stage synchronously; when the engine refuses the mount,
		//! undo the swap instead of leaving the slot stripped, and say so the base-game way.
		if (m_Stage && m_Stage.GetUnplaced().Contains(clicked))
		{
			if (m_iStagedClothingSlot >= 0)
				service.SwapDraftClothingAttachment(m_iStagedClothingSlot, clicked, previous, entry.m_iStorageSlot);
			else
				service.SwapDraftAttachment(m_iStagedWeaponSlot, clicked, previous);
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
		HidePositionSlider();
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
		GRSA_DraftService service = GRSA_DraftService.Get();
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
		HidePositionSlider();
		RefreshOverlayPanels();
	}

	//------------------------------------------------------------------------------------------------
	//! Stats and the receiver card share the bottom band with the strip; they yield while it is open.
	protected void RefreshOverlayPanels()
	{
		bool stripHidden = m_iStripMode == STRIP_HIDDEN;
		bool clothingInspection = m_iStagedClothingSlot >= 0;

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
	protected void RefreshStats()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !m_Stats || !m_Stage || m_iStagedClothingSlot >= 0)
			return;

		IEntity source = m_Stage.SlotSource();
		if (!source)
		{
			m_Stats.Hide();
			return;
		}

		GRSA_ItemEntry entry;
		if (m_Receiver)
			entry = m_Receiver.GetStagedEntry();
		m_Stats.Refresh(source, m_StagedPrefab, entry, GRSA_CatalogService.GetDisplayName(m_StagedPrefab, service.GetBrowseFaction()), service.GetLocalCharacter());
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshCounter()
	{
		if (!m_wCounter)
			return;

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
	protected GRSA_KitWeapon PreferredDraftWeapon(notnull GRSA_DraftService service)
	{
		if (m_iPreferredSlot >= 0)
		{
			GRSA_KitWeapon preferred = service.m_Draft.FindWeapon(m_iPreferredSlot);
			if (preferred && !preferred.m_Prefab.IsEmpty())
				return preferred;
		}

		foreach (GRSA_KitWeapon weapon : service.m_Draft.m_aWeapons)
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
	protected string ExtraRejectLabel(GRSA_EExtraChangeResult result)
	{
		switch (result)
		{
			case GRSA_EExtraChangeResult.VOLUME_LIMIT:
				return "NO SPACE";
			case GRSA_EExtraChangeResult.WEIGHT_LIMIT:
				return "TOO HEAVY";
			case GRSA_EExtraChangeResult.INCOMPATIBLE:
				return "DOES NOT FIT";
		}

		return "UNAVAILABLE";
	}
}
