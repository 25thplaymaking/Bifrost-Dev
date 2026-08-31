//! KITS tab: full-width two-line rows over the saved kit slots. Activating a row WEARS the kit
//! (atomic load + apply, standard loadout semantics); the focused or hovered row is the implicit
//! target of the footer verbs OVERWRITE / RENAME / DELETE. SAVE CURRENT captures the DRAFT — what
//! the stage shows is what lands in the slot — and drops straight into an inline rename. The
//! server's apply verdict lights a result chip on the loading kit's own row.
class GRSA_KitsScreen : SCR_SubMenuBase
{
	protected static const ResourceName EMPTY_SLOT_LAYOUT = "{3B8D42E90A176C54}UI/layouts/Menus/Armory/GRSA_KitEmptySlot.layout";
	protected static const string SAVE_LABEL_DEFAULT = "+ SAVE CURRENT KIT";
	protected static const string SAVE_LABEL_FULL = "LOCKER FULL";
	protected static const string SAVE_LABEL_DISABLED = "SAVING DISABLED";
	protected static const string HINT_DEFAULT = "Select a kit to wear it";
	protected static const int LOADED_CHIP_MS = 1400;

	protected Widget m_wKitList;
	protected TextWidget m_wCount;
	protected TextWidget m_wHint;
	protected RenderTargetWidget m_wStageWorld;
	protected ScrollLayoutWidget m_wKitScroll;
	protected SCR_ButtonTextComponent m_SaveBtn;
	protected SCR_InputButtonComponent m_OverwriteChip;
	protected SCR_InputButtonComponent m_RenameChip;
	protected SCR_InputButtonComponent m_DeleteChip;
	protected SCR_InputButtonComponent m_ImportChip;

	protected ref GRSA_SoldierStage m_Stage;
	protected ref array<ref GRSA_KitRowView> m_aRows = {};
	protected ref array<Widget> m_aEmptySlotWidgets = {};
	protected int m_iHoverRow = -1;
	protected int m_iFocusRow = -1;
	protected int m_iPendingLoadSlot = -1;

	protected static GRSA_KitsScreen s_ActiveInstance;

	//------------------------------------------------------------------------------------------------
	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);

		m_wKitList = m_wRoot.FindAnyWidget("KitList");
		m_wCount = TextWidget.Cast(m_wRoot.FindAnyWidget("KitCount"));
		m_wHint = TextWidget.Cast(m_wRoot.FindAnyWidget("KitsHint"));
		m_wKitScroll = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("KitScroll"));

		//! The legacy pooled preview node stays hidden so the studio render can never double-draw;
		//! this screen has no authored render node, so it fronts the studio through a runtime one.
		Widget legacyPreview = m_wRoot.FindAnyWidget("KitsStage");
		if (legacyPreview)
			legacyPreview.SetVisible(false);

		m_wStageWorld = RenderTargetWidget.Cast(m_wRoot.FindAnyWidget("KitsStageWorld"));
		if (!m_wStageWorld && legacyPreview)
			m_wStageWorld = GRSA_WeaponStage.CreateFallbackRender(legacyPreview.GetParent());
		if (!m_wStageWorld)
			GRSA_Log.Error("Kits screen: no render widget, mannequin cannot draw");

		m_Stage = GRSA_StageHub.Get().GetSoldier();

		m_SaveBtn = SCR_ButtonTextComponent.GetButtonText("SaveCurrentBtn", m_wRoot);
		if (m_SaveBtn)
			m_SaveBtn.m_OnClicked.Insert(OnSaveCurrent);

		m_OverwriteChip = CreateNavigationButton("MenuSave", "Overwrite");
		if (m_OverwriteChip)
			m_OverwriteChip.m_OnActivated.Insert(OnOverwriteChip);
		m_RenameChip = CreateNavigationButton("MenuFilter", "Rename");
		if (m_RenameChip)
			m_RenameChip.m_OnActivated.Insert(OnRenameChip);
		m_DeleteChip = CreateNavigationButton("MenuDelete", "Delete");
		if (m_DeleteChip)
			m_DeleteChip.m_OnActivated.Insert(OnDeleteChip);
		m_ImportChip = CreateNavigationButton("MenuRefresh", "Import Older Kits", true);
		if (m_ImportChip)
			m_ImportChip.m_OnActivated.Insert(OnImportChip);
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabShow()
	{
		super.OnTabShow();

		s_ActiveInstance = this;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnKitListChanged.Insert(Rebuild);

		SCR_ResourcePlayerControllerInventoryComponent.GRSA_GetOnApplyResult().Insert(OnApplyResult);

		Rebuild();
		GRSA_Theme.Apply(m_wRoot);

		if (m_Stage && service)
		{
			if (m_wStageWorld)
				m_Stage.Bind(m_wStageWorld);
			m_Stage.RefreshFromDraft(service);
			m_Stage.FrameStation();
		}

		if (service && service.CanChangeSavedKits() && m_SaveBtn && m_SaveBtn.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(m_SaveBtn.GetRootWidget());
		else if (!m_aRows.IsEmpty() && m_aRows[0].m_Button)
			GetGame().GetWorkspace().SetFocusedWidget(m_aRows[0].m_Button.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	//! A live rename commits before the shell may close the editor.
	static bool ConsumeBack()
	{
		GRSA_KitsScreen screen = s_ActiveInstance;
		if (!screen)
			return false;

		GRSA_KitRowView renaming = screen.RenamingRow();
		if (!renaming)
			return false;

		renaming.CommitRename();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabHide()
	{
		super.OnTabHide();

		if (s_ActiveInstance == this)
			s_ActiveInstance = null;

		GRSA_KitRowView renaming = RenamingRow();
		if (renaming)
			renaming.CommitRename();

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnKitListChanged.Remove(Rebuild);

		SCR_ResourcePlayerControllerInventoryComponent.GRSA_GetOnApplyResult().Remove(OnApplyResult);

		//! A focused kit may still be dressed on the shared mannequin — leaving this tab hands the
		//! draft's own look back to the studio.
		if (m_Stage && service)
			m_Stage.RefreshFromDraft(service);

		GetGame().GetCallqueue().Remove(HideLoadedChip);
		m_iPendingLoadSlot = -1;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsCursorDevice()
	{
		return GetGame().GetInputManager().GetLastUsedInputDevice() == EInputDeviceType.MOUSE;
	}

	//------------------------------------------------------------------------------------------------
	protected GRSA_KitRowView RenamingRow()
	{
		foreach (GRSA_KitRowView row : m_aRows)
		{
			if (row && row.IsRenaming())
				return row;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected int MaxSlots()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service && service.m_Config)
			return service.m_Config.m_iKitSlotCount;
		return 25;
	}

	//------------------------------------------------------------------------------------------------
	protected void Rebuild()
	{
		foreach (GRSA_KitRowView row : m_aRows)
		{
			if (row)
				row.Remove();
		}
		m_aRows.Clear();

		foreach (Widget slotWidget : m_aEmptySlotWidgets)
		{
			if (slotWidget)
				slotWidget.RemoveFromHierarchy();
		}
		m_aEmptySlotWidgets.Clear();

		m_iHoverRow = -1;
		m_iFocusRow = -1;
		UpdateFooterChips();

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !m_wKitList)
			return;

		array<ref GRSA_KitFile> kits = {};
		service.GetKitSlots(kits);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		foreach (int slot, GRSA_KitFile kit : kits)
		{
			if (!kit)
				continue;

			GRSA_KitRowView row = GRSA_KitRowView.Create(m_wKitList, slot, kit);
			if (!row)
				continue;

			row.FillMeta(service);
			row.m_OnRenameCommitted.Insert(OnRowRenameCommitted);
			if (row.m_Button)
			{
				row.m_Button.m_OnClicked.Insert(OnRowClicked);
				row.m_Button.m_OnFocus.Insert(OnRowFocusGained);
				row.m_Button.m_OnFocusLost.Insert(OnRowFocusLost);
				row.m_Button.m_OnMouseEnter.Insert(OnRowHoverEnter);
				row.m_Button.m_OnMouseLeave.Insert(OnRowHoverLeave);
			}
			m_aRows.Insert(row);
		}

		int emptyCount = MaxSlots() - m_aRows.Count();
		for (int i = 0; i < emptyCount; i++)
		{
			Widget slotWidget = workspace.CreateWidgets(EMPTY_SLOT_LAYOUT, m_wKitList);
			if (slotWidget)
				m_aEmptySlotWidgets.Insert(slotWidget);
		}

		UpdateChrome();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateChrome()
	{
		int count = m_aRows.Count();
		int maxSlots = MaxSlots();
		GRSA_DraftService service = GRSA_DraftService.Get();
		bool canSave = service && service.CanChangeSavedKits();

		if (m_wCount)
			m_wCount.SetTextFormat("%1 / %2", count, maxSlots);
		if (canSave)
			SetHint(HINT_DEFAULT);
		else
			SetHint("Kit saving is disabled in Scenario Settings");

		if (m_SaveBtn)
		{
			bool atLimit = count >= maxSlots;
			if (!canSave)
				m_SaveBtn.SetText(SAVE_LABEL_DISABLED);
			else if (atLimit)
				m_SaveBtn.SetText(SAVE_LABEL_FULL);
			else
				m_SaveBtn.SetText(SAVE_LABEL_DEFAULT);
			m_SaveBtn.SetEnabled(canSave && !atLimit);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SetHint(string text)
	{
		if (m_wHint)
			m_wHint.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected int FindRowByModular(SCR_ModularButtonComponent comp)
	{
		for (int i = 0; i < m_aRows.Count(); i++)
		{
			if (m_aRows[i].m_Button == comp)
				return i;
		}
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected int FindRowBySlot(int slot)
	{
		for (int i = 0; i < m_aRows.Count(); i++)
		{
			if (m_aRows[i].m_iSlot == slot)
				return i;
		}
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! Verb target: hovered row (mouse) wins, then the pad-focused row.
	protected int VerbTargetRow()
	{
		if (m_iHoverRow >= 0 && m_iHoverRow < m_aRows.Count())
			return m_iHoverRow;
		if (m_iFocusRow >= 0 && m_iFocusRow < m_aRows.Count())
			return m_iFocusRow;
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateFooterChips()
	{
		bool hasTarget = VerbTargetRow() >= 0;
		GRSA_DraftService service = GRSA_DraftService.Get();
		bool canSave = service && service.CanChangeSavedKits();
		SetNavigationButtonVisible(m_OverwriteChip, hasTarget && canSave);
		SetNavigationButtonVisible(m_RenameChip, hasTarget && canSave);
		SetNavigationButtonVisible(m_DeleteChip, hasTarget);
		SetNavigationButtonVisible(m_ImportChip, canSave);
	}

	//------------------------------------------------------------------------------------------------
	//! Activating a row wears the kit — atomic load and apply, the standard loadout semantic.
	protected void OnRowClicked(SCR_ModularButtonComponent comp)
	{
		int idx = FindRowByModular(comp);
		if (idx < 0)
			return;

		GRSA_KitRowView renaming = RenamingRow();
		if (renaming)
		{
			renaming.CommitRename();
			if (renaming == m_aRows[idx])
				return;
		}

		LoadKitAt(idx);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowFocusGained(SCR_ModularButtonComponent comp)
	{
		int idx = FindRowByModular(comp);
		if (idx < 0)
			return;

		m_iFocusRow = idx;
		UpdateFooterChips();
		PreviewRow(idx);

		if (IsCursorDevice() || !m_wKitScroll)
			return;

		int denom = m_aRows.Count() + m_aEmptySlotWidgets.Count() - 1;
		if (denom < 1)
			denom = 1;
		float frac = idx;
		frac = frac / denom;
		m_wKitScroll.SetSliderPos(0, frac);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowFocusLost(SCR_ModularButtonComponent comp)
	{
		int idx = FindRowByModular(comp);
		if (idx >= 0 && idx == m_iFocusRow)
		{
			m_iFocusRow = -1;
			UpdateFooterChips();

			GRSA_DraftService service = GRSA_DraftService.Get();
			if (m_Stage && service)
				m_Stage.RefreshFromDraft(service);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Focusing a kit shows it on the mannequin without committing anything, like deploy-menu tiles.
	protected void PreviewRow(int idx)
	{
		if (!m_Stage || idx < 0 || idx >= m_aRows.Count())
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
			return;

		GRSA_KitFile kit = m_aRows[idx].m_Kit;
		if (!kit)
			return;

		GRSA_Kit previewKit = new GRSA_Kit();
		GRSA_KitConvert.FlattenToDraft(kit, service.GetLocalCharacter(), previewKit);
		m_Stage.RefreshFromKit(previewKit, service);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowHoverEnter(SCR_ModularButtonComponent comp, bool mouseInput)
	{
		int idx = FindRowByModular(comp);
		if (idx < 0)
			return;
		m_iHoverRow = idx;
		UpdateFooterChips();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowHoverLeave(SCR_ModularButtonComponent comp, bool mouseInput)
	{
		int idx = FindRowByModular(comp);
		if (idx >= 0 && idx == m_iHoverRow)
		{
			m_iHoverRow = -1;
			UpdateFooterChips();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadKitAt(int idx)
	{
		if (idx < 0 || idx >= m_aRows.Count())
			return;

		GRSA_KitRowView row = m_aRows[idx];
		if (!row.m_Kit)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
			return;

		HideAllResultChips();
		m_iPendingLoadSlot = row.m_iSlot;

		string kitName = row.m_Kit.m_sName;
		service.LoadKitIntoDraft(row.m_Kit);
		service.RequestApplyDraft();
		SetHint(string.Format("Equipping '%1'...", kitName));
	}

	//------------------------------------------------------------------------------------------------
	//! Server verdict lands on the loading kit's own row, resolved by SLOT — row indices can go
	//! stale if the list rebuilt before the async result arrived.
	protected void OnApplyResult(GRSA_EApplyStatus status, int applied, int skipped, float suppliesCharged, string skippedSample)
	{
		if (m_iPendingLoadSlot < 0)
			return;

		int chipRow = FindRowBySlot(m_iPendingLoadSlot);
		m_iPendingLoadSlot = -1;
		if (chipRow < 0)
			return;

		HideAllResultChips();

		if (status == GRSA_EApplyStatus.SUCCESS || status == GRSA_EApplyStatus.PARTIAL)
		{
			if (skipped > 0)
				SetHint(string.Format("Equipped %1 items; %2 unavailable", applied, skipped));
			else
				SetHint(string.Format("Equipped %1 items", applied));
			m_aRows[chipRow].ShowResultChip("EQUIPPED", false);
		}
		else
		{
			if (status == GRSA_EApplyStatus.FAILED_RANK)
				SetHint("This kit requires a higher rank");
			else if (status == GRSA_EApplyStatus.FAILED_SUPPLIES)
				SetHint("Not enough supplies for this kit");
			else
				SetHint("This kit could not be equipped");
			m_aRows[chipRow].ShowResultChip("FAILED", true);
		}

		GetGame().GetCallqueue().CallLater(HideLoadedChip, LOADED_CHIP_MS, false, chipRow);
	}

	//------------------------------------------------------------------------------------------------
	protected void HideAllResultChips()
	{
		GetGame().GetCallqueue().Remove(HideLoadedChip);
		foreach (GRSA_KitRowView row : m_aRows)
		{
			if (row)
				row.HideResultChip();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void HideLoadedChip(int idx)
	{
		if (idx >= 0 && idx < m_aRows.Count() && m_aRows[idx])
			m_aRows[idx].HideResultChip();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOverwriteChip(SCR_InputButtonComponent chip, string action)
	{
		if (RenamingRow())
			return;

		int idx = VerbTargetRow();
		if (idx < 0)
			return;

		GRSA_KitRowView row = m_aRows[idx];
		if (!row.m_Kit)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
			return;

		string kitName = row.m_Kit.m_sName;
		if (!service.OverwriteSlotWithDraft(row.m_iSlot, kitName, false))
		{
			SetHint("Could not update this kit");
			return;
		}

		array<ref GRSA_KitFile> kits = {};
		service.GetKitSlots(kits);
		if (row.m_iSlot < kits.Count() && kits[row.m_iSlot])
			row.SetKit(kits[row.m_iSlot]);

		row.FillMeta(service);
		SetHint(string.Format("Updated '%1' with your current equipment", kitName));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRenameChip(SCR_InputButtonComponent chip, string action)
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.CanChangeSavedKits())
			return;

		GRSA_KitRowView renaming = RenamingRow();
		if (renaming)
		{
			renaming.CommitRename();
			return;
		}

		int idx = VerbTargetRow();
		if (idx >= 0)
			m_aRows[idx].StartRename();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowRenameCommitted(GRSA_KitRowView row, string newName)
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.RenameKitSlot(row.m_iSlot, newName))
		{
			SetHint("Could not rename this kit");
			return;
		}

		row.ApplyName(newName);
		SetHint(string.Format("Renamed to '%1'", newName));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDeleteChip(SCR_InputButtonComponent chip, string action)
	{
		if (RenamingRow())
			return;

		int idx = VerbTargetRow();
		if (idx < 0)
			return;

		GRSA_KitRowView row = m_aRows[idx];
		if (!row.m_Kit)
			return;

		string kitName = row.m_Kit.m_sName;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.DeleteKitSlot(row.m_iSlot))
		{
			SetHint("Could not delete this kit");
			return;
		}

		SetHint(string.Format("Deleted '%1'", kitName));

		if (m_SaveBtn && m_SaveBtn.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(m_SaveBtn.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	//! Captures the DRAFT into the first free slot — what the stage shows is what saves — then
	//! drops straight into an inline rename on the fresh row.
	protected void OnSaveCurrent(SCR_ButtonBaseComponent comp)
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.CanChangeSavedKits())
			return;

		array<ref GRSA_KitFile> kits = {};
		service.GetKitSlots(kits);

		int freeSlot = -1;
		foreach (int slot, GRSA_KitFile kit : kits)
		{
			if (!kit)
			{
				freeSlot = slot;
				break;
			}
		}
		if (freeSlot < 0)
			return;

		string kitName;
		for (int n = 1; n < 100; n++)
		{
			string candidate = string.Format("New Kit %1", n);
			bool taken = false;
			foreach (GRSA_KitFile existing : kits)
			{
				if (existing && existing.m_sName == candidate)
				{
					taken = true;
					break;
				}
			}
			if (!taken)
			{
				kitName = candidate;
				break;
			}
		}
		if (kitName.IsEmpty())
			return;

		if (!service.SaveDraftToSlot(freeSlot, kitName))
		{
			SetHint("Could not save this kit");
			return;
		}

		SetHint(string.Format("Saved '%1'", kitName));

		int idx = FindRowBySlot(freeSlot);
		if (idx >= 0)
			m_aRows[idx].StartRename();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnImportChip(SCR_InputButtonComponent chip, string action)
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.CanChangeSavedKits())
			return;

		int imported = service.ImportLegacyKits();
		if (imported == 1)
			SetHint("Imported 1 older kit");
		else
			SetHint(string.Format("Imported %1 older kits", imported));
	}
}
