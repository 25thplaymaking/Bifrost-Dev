// Scrollable mount list for gamepad and clothing; mouse weapon inspection uses callout chips.
class BIA_HardpointRail
{
	protected static const ResourceName ROW_LAYOUT = "{4A47972BDCB8148E}UI/layouts/Menus/Armory/GRSA_ItemRow.layout";

	protected Widget m_wRoot;
	protected Widget m_wList;
	protected BIA_CalloutLayer m_Layer;
	protected ref array<BIA_ItemRowComponent> m_aRows = {};
	protected SCR_Faction m_Faction;
	protected bool m_bEnabled;
	protected bool m_bPadMode;
	protected bool m_bClothing;

	//! (int index) rail row activated — same meaning as a callout chip click.
	ref ScriptInvoker m_OnRowClicked = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	void BIA_HardpointRail(Widget screenRoot, BIA_CalloutLayer layer)
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
	void Rebuild(SCR_Faction faction, bool clothing = false)
	{
		m_Faction = faction;
		m_bClothing = clothing;
		RebuildRows();
	}

	//------------------------------------------------------------------------------------------------
	// Keep the scrolling mount list above the active candidate and position controls.
	void ReserveCandidateSpace(float height)
	{
		if (!m_wRoot) return;
		float left, top, right, bottom;
		AlignableSlot.GetPadding(m_wRoot, left, top, right, bottom);
		bottom = 96;
		if (height > 0) bottom = height + 80;
		AlignableSlot.SetPadding(m_wRoot, left, top, right, bottom);
	}

	void FocusRow(int index)
	{
		if (index < 0 || index >= m_aRows.Count())
			return;

		BIA_ItemRowComponent row = m_aRows[index];
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
			m_Layer.SetChipsVisible(!padMode && !m_bClothing);
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
		bool useRail = m_bPadMode || m_bClothing;
		m_Layer.SetChipsVisible(!useRail);
		bool show = m_bEnabled && useRail && count > 0;
		m_wRoot.SetVisible(show);
		if (!show)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		for (int i = 0; i < count; ++i)
		{
			BIA_CalloutEntry entry = m_Layer.GetEntry(i);
			if (!entry)
				continue;

			Widget rowRoot = workspace.CreateWidgets(ROW_LAYOUT, m_wList);
			if (!rowRoot)
				continue;

			BIA_ItemRowComponent row = BIA_ItemRowComponent.Cast(rowRoot.FindHandler(BIA_ItemRowComponent));
			if (!row)
			{
				rowRoot.RemoveFromHierarchy();
				continue;
			}

			string state = "EMPTY";
			if (!entry.m_AttachedPrefab.IsEmpty())
				state = BIA_CatalogService.GetDisplayName(entry.m_AttachedPrefab, m_Faction);
			row.SetSlotDisplay(entry.m_sTypeLabel, state, entry.m_AttachedPrefab);
			row.SetActivateOnPress(true);
			row.m_OnEntryClicked.Insert(OnRowClicked);
			row.m_OnEntryFocused.Insert(OnRowFocused);
			m_aRows.Insert(row);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowClicked(BIA_ItemRowComponent row)
	{
		int index = m_aRows.Find(row);
		if (index >= 0)
			m_OnRowClicked.Invoke(index);
	}

	//------------------------------------------------------------------------------------------------
	//! Focused rail row lights its anchor dot on the weapon — the in-sync highlight of the brief.
	protected void OnRowFocused(BIA_ItemRowComponent row)
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
		foreach (BIA_ItemRowComponent row : m_aRows)
		{
			if (row && row.GetRootWidget())
				row.GetRootWidget().RemoveFromHierarchy();
		}
		m_aRows.Clear();
	}
}
