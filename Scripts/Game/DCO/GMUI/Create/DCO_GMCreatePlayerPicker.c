// Bifrost GM "Create Player" picker.
class DCO_GMCreatePlayerPicker
{
	static const int PER_PAGE = 16;
	static const int ID_PREV = 9000;
	static const int ID_NEXT = 9001;
	static const int ID_NOOP = 9002;

	protected Widget m_wRoot;
	protected DCO_GMContextMenu m_Menu;	// shared, owned by the controller.
	protected ref DCO_PlacementCatalog m_Catalog;
	protected ref ScriptInvoker m_Cb = new ScriptInvoker();

	protected ref array<string> m_Names = {};
	protected ref array<ResourceName> m_Prefabs = {};
	protected vector m_WorldPos;
	protected int m_Page;

	void Init(Widget shellRoot, DCO_GMContextMenu menu)
	{
		m_wRoot = shellRoot;
		m_Menu = menu;
		m_Cb.Insert(OnPick);
	}

	void Shutdown()
	{
		m_Catalog = null;
		if (m_Menu)
			m_Menu.Hide();
	}

	// Open the picker for a right-click at worldPos: build the faction-scoped MEN list and show page 0 centered.
	void Open(vector worldPos)
	{
		m_WorldPos = worldPos;
		if (!m_Catalog)
		{
			m_Catalog = new DCO_PlacementCatalog();
			m_Catalog.Build();
		}
		BuildList();
		m_Page = 0;
		ShowPage();
	}

	protected void BuildList()
	{
		m_Names.Clear();
		m_Prefabs.Clear();

		FactionKey key = "";
		Faction f = SCR_FactionManager.SGetLocalPlayerFaction();
		if (f)
			key = f.GetFactionKey();

		array<ref DCO_CatalogRow> rows = m_Catalog.Query(DCO_PlacementCatalog.CAT_MAN, key, "");
		foreach (DCO_CatalogRow row : rows)
		{
			if (row.m_bHeader)
				continue;	// section headers are presentation only - we want a flat pick list.
			m_Names.Insert(row.m_Label);
			m_Prefabs.Insert(row.m_Prefab);
		}
		Print(string.Format("[DCO-GM] create-player picker: %1 MEN units (faction='%2')", m_Names.Count(), key), LogLevel.NORMAL);
	}

	// Render the current page into the shared context menu: up to PER_PAGE units + Prev/Next nav rows.
	protected void ShowPage()
	{
		if (!m_Menu)
			return;

		int total = m_Names.Count();
		int pages = (total + PER_PAGE - 1) / PER_PAGE;
		if (pages < 1)
			pages = 1;
		if (m_Page >= pages)
			m_Page = pages - 1;
		if (m_Page < 0)
			m_Page = 0;

		array<string> labels = {};
		array<int> ids = {};

		if (total == 0)
		{
			labels.Insert("(no units available)");
			ids.Insert(ID_NOOP);
		}
		else
		{
			int start = m_Page * PER_PAGE;
			int end = start + PER_PAGE;
			if (end > total)
				end = total;
			for (int i = start; i < end; i++)
			{
				labels.Insert(m_Names[i]);
				ids.Insert(i);	// absolute index into m_Prefabs = the action id.
			}
			if (m_Page > 0)
			{
				labels.Insert(string.Format("<< Previous  (page %1/%2)", m_Page, pages));
				ids.Insert(ID_PREV);
			}
			if (m_Page < pages - 1)
			{
				labels.Insert(string.Format("More units >>  (page %1/%2)", m_Page + 2, pages));
				ids.Insert(ID_NEXT);
			}
		}

		m_Menu.Show(labels, ids, 760, 300, m_Cb, null);
	}

	// Menu pick: nav rows page the list; an item id spawns that character as the player at the stored world position.
	protected void OnPick(int actionId, SCR_EditableEntityComponent e)
	{
		if (actionId == ID_NOOP)
			return;
		if (actionId == ID_PREV)
		{
			m_Page--;
			ShowPage();
			return;
		}
		if (actionId == ID_NEXT)
		{
			m_Page++;
			ShowPage();
			return;
		}
		if (actionId < 0 || actionId >= m_Prefabs.Count())
			return;
		if (m_Catalog)
			m_Catalog.PlaceAsPlayer(m_Prefabs[actionId], m_WorldPos);
	}
}
