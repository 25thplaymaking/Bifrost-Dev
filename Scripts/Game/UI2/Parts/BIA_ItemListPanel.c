//! Drill-in item list: title, search box, batched scrolling rows with an optional marked entry.
//! Owns only widgets, filtering and row lifecycle — what an item means and what clicking it does
//! stays with the screen. Widget names are handed in so any screen layout can host one.
enum BIA_EItemListFilter
{
	ALL,
	PACKED,
	WEAPONS,
	MAGAZINES,
	ATTACHMENTS,
	THROWABLES,
	EXPLOSIVES,
	MEDICAL,
	EQUIPMENT
}

class BIA_ItemListPanel
{
	protected static const ResourceName ROW_LAYOUT = "{4A47972BDCB8148E}UI/layouts/Menus/Armory/GRSA_ItemRow.layout";
	protected static const int ROW_SPAWN_BATCH = 40;
	protected static const int MAX_ROWS = 300;

	protected Widget m_wPanel;
	protected TextWidget m_wTitle;
	protected Widget m_wList;
	protected ScrollLayoutWidget m_wScroll;
	protected SCR_EditBoxComponent m_SearchBox;
	protected Widget m_wDone;
	protected SCR_ButtonTextComponent m_DoneButton;
	protected Widget m_wFilters;
	protected ref array<Widget> m_aFilterWidgets = {};
	protected ref array<SCR_ButtonTextComponent> m_aFilterButtons = {};
	protected ref array<BIA_EItemListFilter> m_aFilterKinds = {};

	protected ref array<BIA_ItemEntry> m_aItems = {};
	protected ref array<BIA_ItemEntry> m_aVisibleItems = {};
	protected ref array<BIA_ItemRowComponent> m_aRows = {};
	protected ref map<ResourceName, int> m_mCounts = new map<ResourceName, int>();
	protected ResourceName m_MarkedPrefab;
	protected string m_sMarkedText;
	protected bool m_bUsesSupplies;
	protected bool m_bQuantityControls;
	protected string m_sSearchFilter;
	protected SCR_ComboBoxComponent m_ModFilter;
	protected ref array<string> m_aModIds = {};
	protected string m_sSelectedMod;
	protected bool m_bSyncingMods;
	protected bool m_bSyncingSearch;
	protected int m_iPendingSpawnIndex;
	protected BIA_EItemListFilter m_eFilter;

	//! (BIA_ItemRowComponent row) list row activated.
	ref ScriptInvoker m_OnItemClicked = new ScriptInvoker();

	//! (BIA_ItemRowComponent row, int delta) quantity stepper activated.
	ref ScriptInvoker m_OnQtyDelta = new ScriptInvoker();

	//! Contents editing finished by the visible action.
	ref ScriptInvoker m_OnDone = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	void BIA_ItemListPanel(Widget screenRoot, string panelName, string titleName, string listName, string scrollName, string searchName, string doneName, string filtersName)
	{
		if (!screenRoot)
			return;

		m_wPanel = screenRoot.FindAnyWidget(panelName);
		if (!m_wPanel && screenRoot.GetName() == panelName)
			m_wPanel = screenRoot;
		m_wTitle = TextWidget.Cast(screenRoot.FindAnyWidget(titleName));
		m_wList = screenRoot.FindAnyWidget(listName);
		m_wScroll = ScrollLayoutWidget.Cast(screenRoot.FindAnyWidget(scrollName));
		m_wDone = screenRoot.FindAnyWidget(doneName);
		if (m_wDone)
		{
			Widget doneButtonWidget = m_wDone.FindAnyWidget(doneName + "Button");
			if (!doneButtonWidget)
				doneButtonWidget = m_wDone;
			m_DoneButton = SCR_ButtonTextComponent.Cast(doneButtonWidget.FindHandler(SCR_ButtonTextComponent));
		}
		if (m_DoneButton)
		{
			m_DoneButton.SetText("< BACK");
			m_DoneButton.m_OnClicked.Insert(OnDoneClicked);
		}
		if (m_wDone)
			m_wDone.SetVisible(false);

		m_wFilters = screenRoot.FindAnyWidget(filtersName);
		BindFilter(screenRoot, "ContentFilterAll", BIA_EItemListFilter.ALL, "ALL");
		BindFilter(screenRoot, "ContentFilterPacked", BIA_EItemListFilter.PACKED, "PACKED");
		BindFilter(screenRoot, "ContentFilterWeapons", BIA_EItemListFilter.WEAPONS, "WEAPONS");
		BindFilter(screenRoot, "ContentFilterMagazines", BIA_EItemListFilter.MAGAZINES, "MAGAZINES");
		BindFilter(screenRoot, "ContentFilterAttachments", BIA_EItemListFilter.ATTACHMENTS, "ATTACHMENTS");
		BindFilter(screenRoot, "ContentFilterThrowables", BIA_EItemListFilter.THROWABLES, "THROWABLES");
		BindFilter(screenRoot, "ContentFilterExplosives", BIA_EItemListFilter.EXPLOSIVES, "EXPLOSIVES");
		BindFilter(screenRoot, "ContentFilterMedical", BIA_EItemListFilter.MEDICAL, "MEDICAL");
		BindFilter(screenRoot, "ContentFilterEquipment", BIA_EItemListFilter.EQUIPMENT, "EQUIPMENT");
		if (m_wFilters)
			m_wFilters.SetVisible(false);

		m_ModFilter = SCR_ComboBoxComponent.GetComboBoxComponent("ItemModFilter", screenRoot);
		if (m_ModFilter)
		{
			m_ModFilter.UseLabel(false);
			m_ModFilter.m_OnChanged.Insert(OnModChanged);
		}

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
	void Open(string title, notnull array<ref BIA_ItemEntry> items, ResourceName markedPrefab, string markedText, bool usesSupplies, bool quantityControls)
	{
		if (!m_wPanel || !m_wList)
			return;

		m_aItems.Clear();
		foreach (BIA_ItemEntry entry : items)
		{
			if (entry)
				m_aItems.Insert(entry);
		}

		BuildModFilter();
		m_MarkedPrefab = markedPrefab;
		m_sMarkedText = markedText;
		m_bUsesSupplies = usesSupplies;
		m_bQuantityControls = quantityControls;
		m_mCounts.Clear();
		if (m_wDone)
			m_wDone.SetVisible(quantityControls);
		if (m_wFilters)
			m_wFilters.SetVisible(quantityControls);
		SetFilter(BIA_EItemListFilter.ALL, false);
		RefreshFilterVisibility();

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
		if (!GetGame().GetInputManager().IsUsingMouseAndKeyboard())
			FocusFirstRow();
	}

	//------------------------------------------------------------------------------------------------
	//! Re-marks the rows in place after a draft change; a rebuild would drop pad focus mid-browse.
	void SetMarked(ResourceName markedPrefab)
	{
		m_MarkedPrefab = markedPrefab;
		foreach (BIA_ItemRowComponent row : m_aRows)
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
	void SetCounts(notnull map<ResourceName, int> counts)
	{
		m_mCounts.Clear();
		foreach (ResourceName prefab, int value : counts)
			m_mCounts.Insert(prefab, value);
		RefreshFilterVisibility();
		if (m_eFilter == BIA_EItemListFilter.PACKED)
			RebuildRows();

		foreach (BIA_ItemRowComponent row : m_aRows)
		{
			if (!row || !row.GetEntry())
				continue;

			int count;
			counts.Find(row.GetEntry().m_Prefab, count);
			row.SetCount(count);
			if (count > 0)
				row.SetStateText(string.Format("PACKED x%1", count));
			else
				row.SetStateText(string.Empty);
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetTitle(string title)
	{
		if (m_wTitle)
			m_wTitle.SetText(title);
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		if (m_ModFilter) m_ModFilter.CloseList();
		GetGame().GetCallqueue().Remove(SpawnBatch);
		ClearRows();
		m_aItems.Clear();
		m_mCounts.Clear();
		m_bQuantityControls = false;
		if (m_wDone)
			m_wDone.SetVisible(false);
		if (m_wFilters)
			m_wFilters.SetVisible(false);
		if (m_wPanel)
			m_wPanel.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void FocusFirstRow()
	{
		if (m_aRows.IsEmpty())
		{
			if (m_DoneButton && m_wDone && m_wDone.IsVisible())
				GetGame().GetWorkspace().SetFocusedWidget(m_DoneButton.GetRootWidget());
			return;
		}

		BIA_ItemRowComponent first = m_aRows[0];
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
		if (m_ModFilter)
		{
			m_ModFilter.CloseList();
			m_ModFilter.m_OnChanged.Remove(OnModChanged);
		}
		if (m_DoneButton)
			m_DoneButton.m_OnClicked.Remove(OnDoneClicked);
		foreach (SCR_ButtonTextComponent filterButton : m_aFilterButtons)
		{
			if (filterButton)
				filterButton.m_OnClicked.Remove(OnFilterClicked);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildModFilter()
	{
		m_sSelectedMod = string.Empty;
		m_aModIds.Clear();
		if (!m_ModFilter)
			return;
		m_bSyncingMods = true;
		m_ModFilter.CloseList();
		m_ModFilter.ClearAll();
		m_ModFilter.AddItem("ALL MODS");
		m_aModIds.Insert(string.Empty);
		array<string> mods = {};
		foreach (BIA_ItemEntry entry : m_aItems)
		{
			foreach (string addon : entry.GetSourceMods())
			{
				if (!addon.IsEmpty() && mods.Find(addon) < 0)
					mods.Insert(addon);
			}
		}
		mods.Sort();
		foreach (string addon : mods)
		{
			m_aModIds.Insert(addon);
			m_ModFilter.AddItem(BIA_CatalogService.GetModTitle(addon));
		}
		m_ModFilter.SetCurrentItem(0);
		m_bSyncingMods = false;
	}

	protected void OnModChanged(SCR_SelectionWidgetComponent selection, int index)
	{
		if (m_bSyncingMods || index < 0 || index >= m_aModIds.Count())
			return;
		m_sSelectedMod = m_aModIds[index];
		RebuildRows();
	}

	protected void RebuildRows()
	{
		GetGame().GetCallqueue().Remove(SpawnBatch);
		ClearRows();

		if (m_wScroll)
			m_wScroll.SetSliderPos(0, 0);

		m_aVisibleItems.Clear();
		string filter = m_sSearchFilter;
		filter.ToLower();
		foreach (BIA_ItemEntry entry : m_aItems)
		{
			if (!m_sSelectedMod.IsEmpty() && entry.GetSourceMods().Find(m_sSelectedMod) < 0)
				continue;
			if (!MatchesFilter(entry, m_eFilter))
				continue;

			if (!filter.IsEmpty())
			{
				string searchable = entry.GetSearchText();
				searchable.ToLower();
				if (!searchable.Contains(filter))
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
			BIA_ItemEntry entry = m_aVisibleItems[m_iPendingSpawnIndex];
			m_iPendingSpawnIndex++;

			Widget rowRoot = workspace.CreateWidgets(ROW_LAYOUT, m_wList);
			if (!rowRoot)
				continue;

			BIA_ItemRowComponent row = BIA_ItemRowComponent.Cast(rowRoot.FindHandler(BIA_ItemRowComponent));
			if (!row)
			{
				rowRoot.RemoveFromHierarchy();
				continue;
			}

			row.SetEntry(entry, m_bUsesSupplies);
			// Quantity rows retain their child-control-aware click handling.
			row.SetActivateOnPress(!m_bQuantityControls);
			row.SetQuantityControlsEnabled(m_bQuantityControls);
			int count;
			m_mCounts.Find(entry.m_Prefab, count);
			row.SetCount(count);
			if (count > 0)
				row.SetStateText(string.Format("PACKED x%1", count));
			if (!m_MarkedPrefab.IsEmpty() && entry.m_Prefab == m_MarkedPrefab)
				row.SetStateText(m_sMarkedText);
			row.m_OnEntryClicked.Insert(OnRowClicked);
			row.m_OnQtyDelta.Insert(OnRowQtyDelta);
			m_aRows.Insert(row);
			spawned++;
		}

		if (m_iPendingSpawnIndex < m_aVisibleItems.Count())
			GetGame().GetCallqueue().CallLater(SpawnBatch, 1, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowClicked(BIA_ItemRowComponent row)
	{
		m_OnItemClicked.Invoke(row);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowQtyDelta(BIA_ItemRowComponent row, int delta)
	{
		m_OnQtyDelta.Invoke(row, delta);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDoneClicked(SCR_ButtonBaseComponent button)
	{
		if (button)
			button.SetToggled(false, true, false);
		m_OnDone.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void BindFilter(Widget screenRoot, string widgetName, BIA_EItemListFilter kind, string label)
	{
		Widget filterWidget = screenRoot.FindAnyWidget(widgetName);
		if (!filterWidget)
			return;

		SCR_ButtonTextComponent filterButton = SCR_ButtonTextComponent.Cast(filterWidget.FindHandler(SCR_ButtonTextComponent));
		if (!filterButton)
			return;

		filterButton.SetText(label);
		filterButton.m_OnClicked.Insert(OnFilterClicked);
		m_aFilterWidgets.Insert(filterWidget);
		m_aFilterButtons.Insert(filterButton);
		m_aFilterKinds.Insert(kind);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFilterClicked(SCR_ButtonBaseComponent button)
	{
		int index = m_aFilterButtons.Find(SCR_ButtonTextComponent.Cast(button));
		if (index < 0)
			return;

		SetFilter(m_aFilterKinds[index], true);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetFilter(BIA_EItemListFilter filter, bool rebuild)
	{
		m_eFilter = filter;
		foreach (int i, SCR_ButtonTextComponent filterButton : m_aFilterButtons)
		{
			if (filterButton)
				filterButton.SetToggled(m_aFilterKinds[i] == filter, true, false);
		}

		if (rebuild)
			RebuildRows();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshFilterVisibility()
	{
		if (!m_bQuantityControls)
			return;

		bool selectedVisible;
		foreach (int i, Widget filterWidget : m_aFilterWidgets)
		{
			BIA_EItemListFilter kind = m_aFilterKinds[i];
			bool visible = kind == BIA_EItemListFilter.ALL || HasFilterItems(kind);
			filterWidget.SetVisible(visible);
			if (visible && kind == m_eFilter)
				selectedVisible = true;
		}

		if (!selectedVisible)
			SetFilter(BIA_EItemListFilter.ALL, true);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasFilterItems(BIA_EItemListFilter filter)
	{
		foreach (BIA_ItemEntry entry : m_aItems)
		{
			if (MatchesFilter(entry, filter))
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool MatchesFilter(BIA_ItemEntry entry, BIA_EItemListFilter filter)
	{
		if (!entry)
			return false;
		if (filter == BIA_EItemListFilter.ALL)
			return true;

		if (filter == BIA_EItemListFilter.PACKED)
		{
			int count;
			m_mCounts.Find(entry.m_Prefab, count);
			return count > 0;
		}

		if (filter == BIA_EItemListFilter.MAGAZINES)
			return (entry.m_eMode & SCR_EArsenalItemMode.AMMUNITION) != 0;
		if (filter == BIA_EItemListFilter.ATTACHMENTS)
			return (entry.m_eType & SCR_EArsenalItemType.WEAPON_ATTACHMENT) != 0;
		if (filter == BIA_EItemListFilter.THROWABLES)
			return (entry.m_eType & (SCR_EArsenalItemType.LETHAL_THROWABLE | SCR_EArsenalItemType.NON_LETHAL_THROWABLE)) != 0;
		if (filter == BIA_EItemListFilter.EXPLOSIVES)
			return (entry.m_eType & (SCR_EArsenalItemType.EXPLOSIVES | SCR_EArsenalItemType.MORTARS)) != 0;
		if (filter == BIA_EItemListFilter.MEDICAL)
			return (entry.m_eType & SCR_EArsenalItemType.HEAL) != 0;
		if (filter == BIA_EItemListFilter.EQUIPMENT)
			return (entry.m_eType & SCR_EArsenalItemType.EQUIPMENT) != 0;

		if (filter == BIA_EItemListFilter.WEAPONS)
		{
			SCR_EArsenalItemType weaponTypes = SCR_EArsenalItemType.RIFLE
				| SCR_EArsenalItemType.SNIPER_RIFLE
				| SCR_EArsenalItemType.MACHINE_GUN
				| SCR_EArsenalItemType.PISTOL
				| SCR_EArsenalItemType.ROCKET_LAUNCHER;
			return (entry.m_eType & weaponTypes) != 0
				&& (entry.m_eMode & (SCR_EArsenalItemMode.WEAPON | SCR_EArsenalItemMode.WEAPON_VARIANTS)) != 0;
		}

		return false;
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
		foreach (BIA_ItemRowComponent row : m_aRows)
		{
			if (row && row.GetRootWidget())
				row.GetRootWidget().RemoveFromHierarchy();
		}
		m_aRows.Clear();
	}
}
