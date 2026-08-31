//! Drill-in item list: title, search box, batched scrolling rows with an optional marked entry.
//! Owns only widgets, filtering and row lifecycle — what an item means and what clicking it does
//! stays with the screen. Widget names are handed in so any screen layout can host one.
class GRSA_ItemListPanel
{
	protected static const ResourceName ROW_LAYOUT = "{4A47972BDCB8148E}UI/layouts/Menus/Armory/GRSA_ItemRow.layout";
	protected static const int ROW_SPAWN_BATCH = 40;
	protected static const int MAX_ROWS = 300;

	protected Widget m_wPanel;
	protected TextWidget m_wTitle;
	protected Widget m_wList;
	protected ScrollLayoutWidget m_wScroll;
	protected SCR_EditBoxComponent m_SearchBox;

	protected ref array<GRSA_ItemEntry> m_aItems = {};
	protected ref array<GRSA_ItemEntry> m_aVisibleItems = {};
	protected ref array<GRSA_ItemRowComponent> m_aRows = {};
	protected ResourceName m_MarkedPrefab;
	protected string m_sMarkedText;
	protected bool m_bUsesSupplies;
	protected string m_sSearchFilter;
	protected bool m_bSyncingSearch;
	protected int m_iPendingSpawnIndex;

	//! (GRSA_ItemRowComponent row) list row activated.
	ref ScriptInvoker m_OnItemClicked = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	void GRSA_ItemListPanel(Widget screenRoot, string panelName, string titleName, string listName, string scrollName, string searchName)
	{
		if (!screenRoot)
			return;

		m_wPanel = screenRoot.FindAnyWidget(panelName);
		m_wTitle = TextWidget.Cast(screenRoot.FindAnyWidget(titleName));
		m_wList = screenRoot.FindAnyWidget(listName);
		m_wScroll = ScrollLayoutWidget.Cast(screenRoot.FindAnyWidget(scrollName));

		m_SearchBox = SCR_EditBoxComponent.GetEditBoxComponent(searchName, screenRoot);
		if (m_SearchBox)
		{
			m_SearchBox.UseLabel(false);
			m_SearchBox.m_OnTextChange.Insert(OnSearchTextChanged);
			m_SearchBox.m_OnChanged.Insert(OnSearchCommitted);
		}
	}

	//------------------------------------------------------------------------------------------------
	bool IsOpen()
	{
		return m_wPanel && m_wPanel.IsVisible();
	}

	//------------------------------------------------------------------------------------------------
	//! Opens the panel on an item set; the entry matching markedPrefab gets markedText as its
	//! state. Clears any previous search and seeds focus on the first row.
	void Open(string title, notnull array<ref GRSA_ItemEntry> items, ResourceName markedPrefab, string markedText, bool usesSupplies)
	{
		if (!m_wPanel || !m_wList)
			return;

		m_aItems.Clear();
		foreach (GRSA_ItemEntry entry : items)
		{
			if (entry)
				m_aItems.Insert(entry);
		}

		m_MarkedPrefab = markedPrefab;
		m_sMarkedText = markedText;
		m_bUsesSupplies = usesSupplies;

		if (m_wTitle)
			m_wTitle.SetText(title);

		m_sSearchFilter = string.Empty;
		if (m_SearchBox && !m_SearchBox.GetValue().IsEmpty())
		{
			m_bSyncingSearch = true;
			m_SearchBox.SetValue(string.Empty);
			m_bSyncingSearch = false;
		}

		m_wPanel.SetVisible(true);
		RebuildRows();
		FocusFirstRow();
	}

	//------------------------------------------------------------------------------------------------
	//! Re-marks the rows in place after a draft change; a rebuild would drop pad focus mid-browse.
	void SetMarked(ResourceName markedPrefab)
	{
		m_MarkedPrefab = markedPrefab;
		foreach (GRSA_ItemRowComponent row : m_aRows)
		{
			if (!row || !row.GetEntry())
				continue;

			if (row.GetEntry().m_Prefab == m_MarkedPrefab)
				row.SetStateText(m_sMarkedText);
			else
				row.SetStateText(string.Empty);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		GetGame().GetCallqueue().Remove(SpawnBatch);
		ClearRows();
		m_aItems.Clear();
		if (m_wPanel)
			m_wPanel.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void FocusFirstRow()
	{
		if (m_aRows.IsEmpty())
			return;

		GRSA_ItemRowComponent first = m_aRows[0];
		if (first && first.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(first.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	//! Symmetric teardown for the search subscriptions and the spawn timer.
	void Destroy()
	{
		GetGame().GetCallqueue().Remove(SpawnBatch);
		if (m_SearchBox)
		{
			m_SearchBox.m_OnTextChange.Remove(OnSearchTextChanged);
			m_SearchBox.m_OnChanged.Remove(OnSearchCommitted);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RebuildRows()
	{
		GetGame().GetCallqueue().Remove(SpawnBatch);
		ClearRows();

		if (m_wScroll)
			m_wScroll.SetSliderPos(0, 0);

		m_aVisibleItems.Clear();
		string filter = m_sSearchFilter;
		filter.ToLower();
		foreach (GRSA_ItemEntry entry : m_aItems)
		{
			if (!filter.IsEmpty())
			{
				string name = entry.m_sDisplayName;
				name.ToLower();
				if (!name.Contains(filter))
					continue;
			}

			m_aVisibleItems.Insert(entry);
			if (m_aVisibleItems.Count() >= MAX_ROWS)
				break;
		}

		m_iPendingSpawnIndex = 0;
		SpawnBatch();
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnBatch()
	{
		if (!m_wList)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int spawned = 0;
		while (m_iPendingSpawnIndex < m_aVisibleItems.Count() && spawned < ROW_SPAWN_BATCH)
		{
			GRSA_ItemEntry entry = m_aVisibleItems[m_iPendingSpawnIndex];
			m_iPendingSpawnIndex++;

			Widget rowRoot = workspace.CreateWidgets(ROW_LAYOUT, m_wList);
			if (!rowRoot)
				continue;

			GRSA_ItemRowComponent row = GRSA_ItemRowComponent.Cast(rowRoot.FindHandler(GRSA_ItemRowComponent));
			if (!row)
			{
				rowRoot.RemoveFromHierarchy();
				continue;
			}

			row.SetEntry(entry, m_bUsesSupplies);
			if (!m_MarkedPrefab.IsEmpty() && entry.m_Prefab == m_MarkedPrefab)
				row.SetStateText(m_sMarkedText);
			row.m_OnEntryClicked.Insert(OnRowClicked);
			m_aRows.Insert(row);
			spawned++;
		}

		if (m_iPendingSpawnIndex < m_aVisibleItems.Count())
			GetGame().GetCallqueue().CallLater(SpawnBatch, 1, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowClicked(GRSA_ItemRowComponent row)
	{
		m_OnItemClicked.Invoke(row);
	}

	//------------------------------------------------------------------------------------------------
	//! ScriptInvokerString passes the PREVIOUS text, the current value is always read from the box.
	protected void OnSearchTextChanged(string previousText)
	{
		if (m_bSyncingSearch || !m_SearchBox)
			return;

		m_sSearchFilter = m_SearchBox.GetValue();
		RebuildRows();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSearchCommitted(SCR_EditBoxComponent editBox, string value)
	{
		if (m_bSyncingSearch)
			return;

		m_sSearchFilter = value;
		RebuildRows();
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
