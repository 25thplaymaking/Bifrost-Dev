//! Receiver switching for the gunsmith: the bottom-right receiver card and the weapon browser it
//! opens in the tile strip, class chips being the config's weapon-slot categories. Owns the card
//! row and the browser state; the picked weapon goes back to the screen through typed invokers,
//! and the strip it draws into is handed in as a view — this part never reaches into the screen.
class GRSA_ReceiverBrowser
{
	protected static const ResourceName ROW_LAYOUT = "{4A47972BDCB8148E}UI/layouts/Menus/Armory/GRSA_ItemRow.layout";

	protected Widget m_wCard;
	protected GRSA_ItemRowComponent m_Row;
	protected GRSA_TileStrip m_Strip;
	protected ref array<GRSA_ArmoryCategory> m_aCategories = {};
	protected ref array<ref GRSA_ItemEntry> m_aItems;
	protected int m_iClass = -1;
	protected ResourceName m_StagedPrefab;
	protected int m_iStagedSlot = -1;

	//! Card clicked; the screen decides whether the browser may open.
	ref ScriptInvoker m_OnCardClicked = new ScriptInvoker();

	//! (int weaponSlot, ResourceName prefab) receiver picked in the browser.
	ref ScriptInvoker m_OnWeaponPicked = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	void GRSA_ReceiverBrowser(Widget screenRoot, GRSA_TileStrip strip)
	{
		m_Strip = strip;
		if (!screenRoot)
			return;

		m_wCard = screenRoot.FindAnyWidget("ReceiverCard");
		if (!m_wCard)
			return;

		Widget cardSlot = m_wCard.FindAnyWidget("ReceiverCardSlot");
		if (!cardSlot)
			return;

		Widget rowRoot = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, cardSlot);
		if (!rowRoot)
			return;

		m_Row = GRSA_ItemRowComponent.Cast(rowRoot.FindHandler(GRSA_ItemRowComponent));
		if (m_Row)
			m_Row.m_OnEntryClicked.Insert(OnRowClicked);
	}

	//------------------------------------------------------------------------------------------------
	void SetCardVisible(bool visible)
	{
		if (m_wCard)
			m_wCard.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	bool IsBrowsing()
	{
		return m_iClass >= 0;
	}

	//------------------------------------------------------------------------------------------------
	//! The staged weapon the card mirrors; refreshes the card row.
	void SetStaged(ResourceName prefab, int weaponSlot)
	{
		m_StagedPrefab = prefab;
		m_iStagedSlot = weaponSlot;
		RefreshCard();
	}

	//------------------------------------------------------------------------------------------------
	//! Catalog entry of the staged weapon, shared with the stats block so both read one lookup.
	GRSA_ItemEntry GetStagedEntry()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || m_StagedPrefab.IsEmpty())
			return null;

		EnsureCategories(service);
		return GRSA_CatalogService.FindEntry(m_StagedPrefab, m_aCategories, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType());
	}

	//------------------------------------------------------------------------------------------------
	//! Opens the class-chip browser in the strip, landing on the staged weapon's own class.
	void OpenBrowser()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !m_Strip)
			return;

		EnsureCategories(service);
		if (m_aCategories.IsEmpty())
			return;

		int initial = 0;
		foreach (int i, GRSA_ArmoryCategory category : m_aCategories)
		{
			if (category.m_iWeaponSlot == m_iStagedSlot)
			{
				initial = i;
				break;
			}
		}

		array<string> labels = {};
		foreach (GRSA_ArmoryCategory chipCategory : m_aCategories)
			labels.Insert(chipCategory.m_sDisplayName);

		m_Strip.ShowClasses(labels, initial);
		ShowClass(initial);
	}

	//------------------------------------------------------------------------------------------------
	void ShowClass(int index)
	{
		if (index < 0 || index >= m_aCategories.Count() || !m_Strip)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
			return;

		m_iClass = index;
		m_Strip.SelectClass(index);

		GRSA_ArmoryCategory category = m_aCategories[index];
		m_aItems = GRSA_CatalogService.GetItems(category, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType());

		array<GRSA_ItemEntry> display = {};
		if (m_aItems)
		{
			foreach (GRSA_ItemEntry item : m_aItems)
				display.Insert(item);
		}

		string title = category.m_sDisplayName;
		title.ToUpper();
		m_Strip.ShowTiles(string.Format("SELECT WEAPON: %1", title), display, m_StagedPrefab, "EQUIPPED", service.UsesSupplies());
	}

	//------------------------------------------------------------------------------------------------
	//! Routes a strip tile to the picked invoker while the browser owns the strip. Returns false
	//! when not browsing so the screen can treat the tile as an attachment candidate instead.
	bool HandleTileClicked(GRSA_ItemRowComponent row)
	{
		if (!IsBrowsing())
			return false;

		if (!row || !row.GetEntry())
			return true;

		int slot = m_aCategories[m_iClass].m_iWeaponSlot;
		if (slot < 0)
			return true;

		m_OnWeaponPicked.Invoke(slot, row.GetEntry().m_Prefab);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Re-renders the open class so the EQUIPPED marker follows the staged weapon.
	void RefreshBrowser()
	{
		if (IsBrowsing())
			ShowClass(m_iClass);
	}

	//------------------------------------------------------------------------------------------------
	void CloseBrowser()
	{
		m_iClass = -1;
	}

	//------------------------------------------------------------------------------------------------
	protected void EnsureCategories(notnull GRSA_DraftService service)
	{
		if (m_aCategories.IsEmpty())
			GRSA_CatalogService.GetWeaponSlotCategories(service.m_Config, m_aCategories);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshCard()
	{
		if (!m_Row)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
			return;

		if (m_StagedPrefab.IsEmpty())
		{
			m_Row.SetSlotDisplay("NO WEAPON", "SELECT", ResourceName.Empty);
			return;
		}

		GRSA_ItemEntry entry = GetStagedEntry();
		if (entry)
			m_Row.SetEntry(entry, service.UsesSupplies());
		else
			m_Row.SetSlotDisplay(GRSA_CatalogService.GetDisplayName(m_StagedPrefab, service.GetBrowseFaction()), "", m_StagedPrefab);

		m_Row.SetStateText("SWITCH");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowClicked(GRSA_ItemRowComponent row)
	{
		m_OnCardClicked.Invoke();
	}
}
