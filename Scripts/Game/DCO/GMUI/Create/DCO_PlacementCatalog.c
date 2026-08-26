// Bifrost GM CREATE-panel placement catalog.

class DCO_CatalogEntry
{
	ResourceName m_Prefab;
	string m_Name;
	ResourceName m_Icon;
	FactionKey m_Faction;
	int m_Type;	// EEditableEntityType.
	int m_Category;
	string m_BudgetText;	// "" when free.
	ResourceName m_App6Icon;
	string m_SubCat;
}

class DCO_CatalogRow
{
	bool m_bHeader;
	bool m_bCollapsed;	// header only.
	string m_Label;
	ResourceName m_Icon;	// item only.
	ResourceName m_Prefab;
	string m_BudgetText;
	string m_SectionKey;
	FactionKey m_Faction;
	int m_Type;
	ResourceName m_App6Icon;

	// Tree placement.
	int m_Depth;
	int m_Level;	// folder only.
	int m_Category;	// folder only - CAT_* of its contents, so the panel can pick the level-0 tab icon.
}

class DCO_PlacementCatalog
{
	static const int CAT_ALL    = -1;
	static const int CAT_MAN    = 0;	// CHARACTER.
	static const int CAT_GROUP  = 1;	// GROUP.
	static const int CAT_OBJECT = 2;	// VEHICLE / GENERIC / SLOT / ITEM / ...
	static const int CAT_MODULE = 3;	// SYSTEM.
	static const int CAT_EFFECTS = 4;

	protected ref array<ref DCO_CatalogEntry> m_Entries = {};
	protected ref array<FactionKey> m_FactionKeys = {};	// distinct factions that actually have content.
	protected ref map<string, bool> m_Expanded = new map<string, bool>();
	protected bool m_bSeedSubCatsOpen;	// set per-Query: a category tab is active, so its sub-category folders seed open.
	protected SCR_PlacingEditorComponent m_Placing;
	protected bool m_bBuilt;

	bool IsBuilt() { return m_bBuilt; }
	int GetEntryCount() { return m_Entries.Count(); }

	// Resolve the editor components and enumerate every placeable entity.
	bool Build()
	{
		m_Entries.Clear();
		m_FactionKeys.Clear();

		m_Placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		SCR_ContentBrowserEditorComponent cb = SCR_ContentBrowserEditorComponent.Cast(SCR_ContentBrowserEditorComponent.GetInstance(SCR_ContentBrowserEditorComponent, true));

		int built = 0;
		bool cacheReady = cb && cb.GetInfoCount() > 0;
		if (cacheReady)
			built = BuildFromBrowser(cb);
		if (built == 0 && m_Placing)
			built = BuildFromPlacing(m_Placing);

		CollectFactions();
		m_bBuilt = true;

		Print(string.Format("[DCO-GM] placement catalog: %1 entries, %2 factions (browser=%3 cacheReady=%4 placing=%5)",
			m_Entries.Count(), m_FactionKeys.Count(), cb != null, cacheReady, m_Placing != null), LogLevel.NORMAL);
		return !m_Entries.IsEmpty();
	}

	protected int BuildFromBrowser(SCR_ContentBrowserEditorComponent cb)
	{
		int count = cb.GetInfoCount();
		for (int i = 0; i < count; i++)
		{
			SCR_EditableEntityUIInfo info = cb.GetInfo(i);
			if (!info)
				continue;
			ResourceName res = cb.GetResourceNamePrefabID(i);
			if (res.IsEmpty())
				continue;
			AddEntry(res, info);
		}
		return m_Entries.Count();
	}

	protected int BuildFromPlacing(SCR_PlacingEditorComponent placing)
	{
		SCR_PlacingEditorComponentClass placingData = SCR_PlacingEditorComponentClass.Cast(placing.GetEditorComponentData());
		if (!placingData)
			return m_Entries.Count();
		array<ResourceName> prefabs = {};
		placingData.GetPrefabs(prefabs, true);
		foreach (ResourceName res : prefabs)
		{
			if (res.IsEmpty())
				continue;
			SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.ExtractEditableUIInfoFromPrefab(res);
			if (!info)
				continue;
			AddEntry(res, info);
		}
		return m_Entries.Count();
	}

	static const ref array<string> TACTICS_OWNED_ZONES = {
		"/E_DCO_TaskZone.et",
		"/E_DCO_TaskZone_Ambush.et",
		"/E_DCO_TaskZone_AmbushKillZone.et",
		"/E_DCO_TaskZone_Defend.et",
		"/E_AIWaypoint_DCO_CqbClear.et",
	};

	protected void AddEntry(ResourceName res, SCR_EditableEntityUIInfo info)
	{
		foreach (string zonePath : TACTICS_OWNED_ZONES)
		{
			if (res.Contains(zonePath))
				return;	// tactics tab is the one truth for this zone.
		}

		DCO_CatalogEntry e = new DCO_CatalogEntry();
		e.m_Prefab = res;
		e.m_Name = info.GetName();
		e.m_Icon = info.GetImage();
		e.m_Faction = info.GetFactionKey();
		e.m_Type = info.GetEntityType();
		e.m_Category = CategoryForType(info.GetEntityType());
		// Bifrost FX placeables live on their own EFFECTS tab regardless of editable type.
		if (res.Contains("/E_DCO_Fx"))
			e.m_Category = CAT_EFFECTS;
		if (res.Contains("/E_DCO_Trigger"))
			e.m_Category = CAT_MODULE;
		e.m_BudgetText = BudgetText(info);
		array<EEditableEntityLabel> labels = {};
		info.GetEntityLabels(labels);
		e.m_App6Icon = DCO_App6Icons.GetIcon(labels, e.m_Name, e.m_Faction, e.m_Type);
		e.m_SubCat = SubCatFor(res);
		m_Entries.Insert(e);
	}

	// Path-derived sub-category for the OBJECTS tab.
	protected string SubCatFor(ResourceName res)
	{
		string p = res;
		int brace = p.IndexOf("}");
		if (brace >= 0)
			p = p.Substring(brace + 1, p.Length() - brace - 1);
		array<string> segs = {};
		p.Split("/", segs, true);
		int i = 0;
		if (segs.Count() > 0 && (segs[0] == "Prefabs" || segs[0] == "PrefabsEditable"))
			i = 1;
		if (i >= segs.Count() - 1)
			return "";	// nothing between the root and the filename - no folder to name a bucket after.
		string top = segs[i];
		if (top == "Props" && i + 1 < segs.Count() - 1)
			top = segs[i + 1];
		top.Replace("_", " ");
		return top;
	}

	static int CategoryForType(EEditableEntityType t)
	{
		switch (t)
		{
			case EEditableEntityType.CHARACTER: return CAT_MAN;
			case EEditableEntityType.GROUP:     return CAT_GROUP;
			case EEditableEntityType.SYSTEM:    return CAT_MODULE;
		}
		return CAT_OBJECT;	// VEHICLE / GENERIC / SLOT / ITEM / WAYPOINT / ...
	}

	protected string BudgetText(SCR_EditableEntityUIInfo info)
	{
		array<ref SCR_EntityBudgetValue> costs = {};
		if (!info.GetEntityBudgetCost(costs) || costs.IsEmpty())
			return "";
		int total = 0;
		foreach (SCR_EntityBudgetValue v : costs)
		{
			total += v.GetBudgetValue();
		}
		if (total <= 0)
			return "";
		return total.ToString();
	}

	protected void CollectFactions()
	{
		foreach (DCO_CatalogEntry e : m_Entries)
		{
			if (e.m_Faction.IsEmpty())
				continue;
			if (!m_FactionKeys.Contains(e.m_Faction))
				m_FactionKeys.Insert(e.m_Faction);
		}
	}

	void GetFactionKeys(out notnull array<FactionKey> keys)
	{
		keys.Clear();
		foreach (FactionKey k : m_FactionKeys)
		{
			keys.Insert(k);
		}
	}

	// Lightweight count for filter badges and impossible-filter recovery.
	int CountEntries(int categoryFilter, FactionKey factionFilter)
	{
		int count;
		foreach (DCO_CatalogEntry entry : m_Entries)
		{
			if (categoryFilter != CAT_ALL && entry.m_Category != categoryFilter)
				continue;
			if (!factionFilter.IsEmpty() && entry.m_Faction != factionFilter)
				continue;
			count++;
		}
		return count;
	}

	Color GetFactionColor(FactionKey key)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (fm)
		{
			SCR_Faction f = SCR_Faction.Cast(fm.GetFactionByKey(key));
			if (f)
				return f.GetOutlineFactionColor();
		}
		return DCO_GMTheme.Get().m_MutedColor;
	}

	string GetFactionLabel(FactionKey key)
	{
		if (key.IsEmpty())
			return "Neutral";
		FactionManager fm = GetGame().GetFactionManager();
		if (fm)
		{
			Faction f = fm.GetFactionByKey(key);
			if (f)
			{
				string nm = f.GetFactionName();
				if (!nm.IsEmpty())
					return nm;
			}
		}
		return key;
	}

	string GetFactionTabLabel(FactionKey key)
	{
		if (key.IsEmpty())
			return "NEU";
		string k = key;
		if (k.Length() > 7)
			k = k.Substring(0, 7);
		return k;
	}

	void ToggleSection(string sectionKey)
	{
		bool expanded = false;
		m_Expanded.Find(sectionKey, expanded);
		m_Expanded.Set(sectionKey, !expanded);
	}

	// Nested folders default collapsed so the 3-level browser never emits ~1500 item rows on entry.
	protected bool IsCollapsed(string sectionKey)
	{
		bool expanded = false;
		m_Expanded.Find(sectionKey, expanded);
		return !expanded;
	}

	// Level-3 folder name - the EEditableEntityType of the entries beneath it.
	static string TypeLabel(int type)
	{
		switch (type)
		{
			case EEditableEntityType.VEHICLE:   return "Vehicles";
			case EEditableEntityType.CHARACTER: return "Units";
			case EEditableEntityType.GROUP:     return "Groups";
			case EEditableEntityType.ITEM:      return "Items";
			case EEditableEntityType.SYSTEM:    return "Systems";
		}
		return "Props";	// GENERIC and anything unmapped.
	}

	// The folder name an entry falls under at a given tree level.
	protected string LevelLabel(DCO_CatalogEntry e, int level)
	{
		if (level == 0)
			return CategoryLabel(e.m_Category);
		if (level == 1)
			return GetFactionLabel(e.m_Faction);
		if (e.m_Category == CAT_OBJECT && !e.m_SubCat.IsEmpty())
			return e.m_SubCat;
		return TypeLabel(e.m_Type);
	}

	protected void GroupBy(notnull array<ref DCO_CatalogEntry> src, int level, notnull array<string> order, notnull map<string, ref array<ref DCO_CatalogEntry>> buckets)
	{
		foreach (DCO_CatalogEntry e : src)
		{
			string k = LevelLabel(e, level);
			array<ref DCO_CatalogEntry> b;
			if (!buckets.Find(k, b))
			{
				b = {};
				buckets.Set(k, b);
				order.Insert(k);
			}
			b.Insert(e);
		}
	}

	protected DCO_CatalogRow MakeFolderRow(string path, string label, int count, int depth, int level, int category, FactionKey faction, bool collapsed)
	{
		DCO_CatalogRow r = new DCO_CatalogRow();
		r.m_bHeader = true;
		r.m_bCollapsed = collapsed;
		r.m_SectionKey = path;
		r.m_Label = string.Format("%1   ·   %2", label, count);
		r.m_Depth = depth;
		r.m_Level = level;
		r.m_Category = category;
		r.m_Faction = faction;
		return r;
	}

	protected DCO_CatalogRow MakeItemRow(DCO_CatalogEntry e, string path, int depth)
	{
		DCO_CatalogRow r = new DCO_CatalogRow();
		r.m_bHeader = false;
		r.m_Label = e.m_Name;
		r.m_Icon = e.m_Icon;
		r.m_Prefab = e.m_Prefab;
		r.m_BudgetText = e.m_BudgetText;
		r.m_SectionKey = path;
		r.m_Faction = e.m_Faction;
		r.m_Type = e.m_Type;
		r.m_App6Icon = e.m_App6Icon;
		r.m_Depth = depth;
		return r;
	}

	// Emit one tree level and recurse.
	protected void EmitLevel(notnull array<ref DCO_CatalogEntry> src, string pathPrefix, int level, int depth, notnull array<ref DCO_CatalogRow> rows)
	{
		if (level > 2)
		{
			foreach (DCO_CatalogEntry e : src)
			{
				rows.Insert(MakeItemRow(e, pathPrefix, depth));
			}
			return;
		}

		if (level == 1)
		{
			EmitLevel(src, pathPrefix, 2, depth, rows);
			return;
		}

		array<string> order = {};
		map<string, ref array<ref DCO_CatalogEntry>> buckets = new map<string, ref array<ref DCO_CatalogEntry>>();
		GroupBy(src, level, order, buckets);
		if (level == 2)
			order.Sort();	// the sub-category level can hold dozens of path-derived folders - alphabetical beats first-seen.

		if (order.Count() <= 1)
		{
			EmitLevel(src, pathPrefix, level + 1, depth, rows);	// degenerate level -> skip it.
			return;
		}

		foreach (string k : order)
		{
			array<ref DCO_CatalogEntry> b;
			buckets.Find(k, b);
			string path = k;
			if (!pathPrefix.IsEmpty())
				path = pathPrefix + "/" + k;
			// Seed the top-level categories open so ALL uses the full chooser viewport instead of presenting five rows over a large blank canvas.
			if (level == 0 || (level == 2 && m_bSeedSubCatsOpen))
			{
				bool exp;
				if (!m_Expanded.Find(path, exp))
					m_Expanded.Set(path, true);
			}
			bool collapsed = IsCollapsed(path);
			rows.Insert(MakeFolderRow(path, k, b.Count(), depth, level, b[0].m_Category, b[0].m_Faction, collapsed));
			if (collapsed)
				continue;
			EmitLevel(b, path, level + 1, depth + 1, rows);
		}
	}

	// Filter by category + faction + search, group into collapsible sections, and return the flat render rows.
	array<ref DCO_CatalogRow> Query(int categoryFilter, FactionKey factionFilter, string search)
	{
		m_bSeedSubCatsOpen = categoryFilter != CAT_ALL;	// see EmitLevel: a tab click opens its sub-category folders.

		// 1.
		array<ref DCO_CatalogEntry> candidates = {};
		foreach (DCO_CatalogEntry e : m_Entries)
		{
			if (categoryFilter != CAT_ALL && e.m_Category != categoryFilter)
				continue;
			if (!factionFilter.IsEmpty() && e.m_Faction != factionFilter)
				continue;
			candidates.Insert(e);
		}

		// 2.
		if (!search.IsEmpty())
		{
			array<string> keys = {};
			foreach (DCO_CatalogEntry e : candidates)
			{
				keys.Insert(e.m_Name);
			}
			array<int> hits = {};
			WidgetManager.SearchLocalized(search, keys, hits);
			array<ref DCO_CatalogEntry> matched = {};
			foreach (int idx : hits)
			{
				if (idx >= 0 && idx < candidates.Count())
					matched.Insert(candidates[idx]);
			}
			candidates = matched;
		}

		// 3.
		array<ref DCO_CatalogRow> rows = {};
		if (!search.IsEmpty())
		{
			map<string, bool> seenPrefab = new map<string, bool>();
			array<int> catOrder = {CAT_MAN, CAT_GROUP, CAT_OBJECT, CAT_MODULE, CAT_EFFECTS};
			foreach (int cat : catOrder)
			{
				array<ref DCO_CatalogEntry> inCat = {};
				map<string, int> nameCount = new map<string, int>();
				foreach (DCO_CatalogEntry e : candidates)
				{
					if (e.m_Category != cat)
						continue;
					if (seenPrefab.Contains(e.m_Prefab))
						continue;
					seenPrefab.Set(e.m_Prefab, true);
					inCat.Insert(e);
					int n = 0;
					nameCount.Find(e.m_Name, n);
					nameCount.Set(e.m_Name, n + 1);
				}
				if (inCat.IsEmpty())
					continue;

				// One header per category.
				string path = "search:" + CategoryLabel(cat);
				bool exp;
				if (!m_Expanded.Find(path, exp))
					m_Expanded.Set(path, true);
				bool collapsed = IsCollapsed(path);
				rows.Insert(MakeFolderRow(path, CategoryLabel(cat), inCat.Count(), 0, 0, cat, "", collapsed));
				if (collapsed)
					continue;

				foreach (DCO_CatalogEntry e : inCat)
				{
					DCO_CatalogRow r = MakeItemRow(e, path, 1);
					int dupes = 0;
					nameCount.Find(e.m_Name, dupes);
					if (dupes > 1 && factionFilter.IsEmpty() && !e.m_Faction.IsEmpty())
						r.m_Label = string.Format("%1 · %2", e.m_Name, e.m_Faction);
					rows.Insert(r);
				}
			}
			return rows;
		}

		EmitLevel(candidates, "", 0, 0, rows);
		return rows;
	}

	static string CategoryLabel(int cat)
	{
		switch (cat)
		{
			case CAT_MAN:    return "Men";
			case CAT_GROUP:  return "Groups";
			case CAT_MODULE: return "Systems";
			case CAT_EFFECTS: return "Effects";
		}
		return "Objects";
	}

	bool Place(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return false;
		if (!m_Placing)
			m_Placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (!m_Placing)
		{
			Print("[DCO-GM] Place FAIL: no placing component", LogLevel.WARNING);
			return false;
		}
		UnlockBrowserForPlacing();
		m_Placing.SetSelectedPrefab(prefab);
		Print("[DCO-GM] placing selected: " + prefab, LogLevel.NORMAL);
		return true;
	}

	bool PlaceAsPlayer(ResourceName prefab, vector worldPos)
	{
		if (prefab.IsEmpty())
			return false;
		if (!m_Placing)
			m_Placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (!m_Placing)
		{
			Print("[DCO-GM] PlaceAsPlayer FAIL: no placing component", LogLevel.WARNING);
			return false;
		}
		if (m_Placing.IsPlacing())
			m_Placing.SetSelectedPrefab(ResourceName.Empty);	// cancel any in-progress placement first.

		vector transform[4];
		transform[3] = worldPos;
		SCR_ManualCamera camera = SCR_CameraEditorComponent.GetCameraInstance();
		if (camera)
			Math3D.AnglesToMatrix(Vector(camera.GetAngles()[0], 0, 0), transform);	// yaw only; preserves transform[3].
		else
			Math3D.MatrixIdentity3(transform);

		m_Placing.SetPlacingFlag(EEditorPlacingFlags.CHARACTER_PLAYER, true);
		m_Placing.SetInstantPlacing(SCR_EditorPreviewParams.CreateParams(transform));
		UnlockBrowserForPlacing();
		bool ok = m_Placing.SetSelectedPrefab(prefab);
		Print(string.Format("[DCO-GM] PlaceAsPlayer prefab=%1 ok=%2", prefab, ok), LogLevel.NORMAL);
		return ok;
	}

	protected void UnlockBrowserForPlacing()
	{
		if (!m_Placing)
			return;
		IEntity owner = m_Placing.GetOwner();
		if (!owner)
			return;
		SCR_ContentBrowserEditorComponent cb = SCR_ContentBrowserEditorComponent.Cast(owner.FindComponent(SCR_ContentBrowserEditorComponent));
		if (!cb)
		{
			Print("[DCO-GM] UnlockBrowserForPlacing: no content browser on placing owner (placement availability gate may block)", LogLevel.WARNING);
			return;
		}
		cb.ResetAllLabels(false);
		cb.SetCurrentSearch("");
		cb.FilterEntries();
		Print(string.Format("[DCO-GM] placement availability unlocked: %1 prefabs now placeable", cb.GetFilteredPrefabCount()), LogLevel.NORMAL);
	}

}
