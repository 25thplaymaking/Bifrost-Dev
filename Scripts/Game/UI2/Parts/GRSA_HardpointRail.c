//! Pad-first surface of the gunsmith hardpoints: the same callout set as a right-side focusable
//! list. On gamepad the chip columns and leader lines hide, the rail shows, and the focused row's
//! anchor dot lights up on the weapon — native linear focus order instead of spatial jumps across
//! floating chips. On mouse and keyboard the rail hides and the chip columns return. The
//! callout layer is handed in as the view this part drives; row activation goes back to the
//! screen through one typed invoker.
class GRSA_HardpointRail
{
	protected static const ResourceName ROW_LAYOUT = "{4A47972BDCB8148E}UI/layouts/Menus/Armory/GRSA_ItemRow.layout";

	protected Widget m_wRoot;
	protected Widget m_wList;
	protected GRSA_CalloutLayer m_Layer;
	protected ref array<GRSA_ItemRowComponent> m_aRows = {};
	protected SCR_Faction m_Faction;
	protected bool m_bEnabled;
	protected bool m_bPadMode;

	//! (int index) rail row activated — same meaning as a callout chip click.
	ref ScriptInvoker m_OnRowClicked = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	void GRSA_HardpointRail(Widget screenRoot, GRSA_CalloutLayer layer)
	{
		m_Layer = layer;
		if (!screenRoot)
			return;

		m_wRoot = screenRoot.FindAnyWidget("HardpointRail");
		if (m_wRoot)
			m_wList = m_wRoot.FindAnyWidget("HardpointRailList");
	}

	//------------------------------------------------------------------------------------------------
	bool IsPadMode()
	{
		return m_bPadMode;
	}

	//------------------------------------------------------------------------------------------------
	//! Tab shown: watch device switches and apply the current one.
	void Enable()
	{
		if (m_bEnabled)
			return;

		m_bEnabled = true;
		GetGame().OnInputDeviceIsGamepadInvoker().Insert(OnInputDeviceIsGamepad);
		ApplyMode(!GetGame().GetInputManager().IsUsingMouseAndKeyboard());
	}

	//------------------------------------------------------------------------------------------------
	void Disable()
	{
		if (!m_bEnabled)
			return;

		m_bEnabled = false;
		GetGame().OnInputDeviceIsGamepadInvoker().Remove(OnInputDeviceIsGamepad);
		ClearRows();
		if (m_wRoot)
			m_wRoot.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds rows from the callout layer's current entries; call after every callout build.
	void Rebuild(SCR_Faction faction)
	{
		m_Faction = faction;
		RebuildRows();
	}

	//------------------------------------------------------------------------------------------------
	void FocusRow(int index)
	{
		if (index < 0 || index >= m_aRows.Count())
			return;

		GRSA_ItemRowComponent row = m_aRows[index];
		if (row && row.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(row.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	void FocusFirstRow()
	{
		FocusRow(0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnInputDeviceIsGamepad(bool isGamepad)
	{
		ApplyMode(isGamepad);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyMode(bool padMode)
	{
		m_bPadMode = padMode;
		if (m_Layer)
		{
			m_Layer.SetChipsVisible(!padMode);
			if (!padMode)
				m_Layer.SetHighlight(-1);
		}

		RebuildRows();
		if (padMode && !m_aRows.IsEmpty())
			FocusFirstRow();
	}

	//------------------------------------------------------------------------------------------------
	protected void RebuildRows()
	{
		ClearRows();
		if (!m_wRoot || !m_wList || !m_Layer)
			return;

		int count = m_Layer.GetTotalCount();
		bool show = m_bEnabled && m_bPadMode && count > 0;
		m_wRoot.SetVisible(show);
		if (!show)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		for (int i = 0; i < count; ++i)
		{
			GRSA_CalloutEntry entry = m_Layer.GetEntry(i);
			if (!entry)
				continue;

			Widget rowRoot = workspace.CreateWidgets(ROW_LAYOUT, m_wList);
			if (!rowRoot)
				continue;

			GRSA_ItemRowComponent row = GRSA_ItemRowComponent.Cast(rowRoot.FindHandler(GRSA_ItemRowComponent));
			if (!row)
			{
				rowRoot.RemoveFromHierarchy();
				continue;
			}

			string state = "EMPTY";
			if (!entry.m_AttachedPrefab.IsEmpty())
				state = GRSA_CatalogService.GetDisplayName(entry.m_AttachedPrefab, m_Faction);
			row.SetSlotDisplay(entry.m_sTypeLabel, state, entry.m_AttachedPrefab);
			row.m_OnEntryClicked.Insert(OnRowClicked);
			row.m_OnEntryFocused.Insert(OnRowFocused);
			m_aRows.Insert(row);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowClicked(GRSA_ItemRowComponent row)
	{
		int index = m_aRows.Find(row);
		if (index >= 0)
			m_OnRowClicked.Invoke(index);
	}

	//------------------------------------------------------------------------------------------------
	//! Focused rail row lights its anchor dot on the weapon — the in-sync highlight of the brief.
	protected void OnRowFocused(GRSA_ItemRowComponent row)
	{
		if (!m_Layer)
			return;

		int index = m_aRows.Find(row);
		if (index >= 0)
			m_Layer.SetHighlight(index);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearRows()
	{
		foreach (GRSA_ItemRowComponent row : m_aRows)
		{
			if (row && row.GetRootWidget())
				row.GetRootWidget().RemoveFromHierarchy();
		}
		m_aRows.Clear();
	}
}
