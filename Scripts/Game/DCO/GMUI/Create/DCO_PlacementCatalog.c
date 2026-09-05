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
	string m_SearchMetadata;
	int m_iMissionTool;
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
	int m_iMissionTool;

	// Tree placement.
	int m_Depth;
	int m_Level;	// folder only.
	int m_Category;	// folder only - CAT_* of its contents, so the panel can pick the level-0 tab icon.
}

class DCO_PlacementCatalog
{
	static const ResourceName ANIMATION_FX_RESOURCE = "DCO_ANIMATIONS_FX";
	static const ResourceName ARSENAL_ACCESS_RESOURCE = "DCO_ARSENAL_ACCESS";
	static const string MISSION_TOOL_PREFIX = "DCO_MISSION_TOOL_";
	static const ResourceName TERRAIN_AREA_RESOURCE = "{DCA6090410000000}Prefabs/E_DCO_TerrainArea.et";
	static const ResourceName VEHICLE_SERVICE_RESOURCE = "{C6A17D4B92E83F50}Prefabs/E_DCO_VehicleServiceZone.et";
	static const int CAT_ALL    = -1;
	static const int CAT_MAN    = 0;	// CHARACTER.
	static const int CAT_GROUP  = 1;	// GROUP.
	static const int CAT_OBJECT = 2;	// VEHICLE / GENERIC / SLOT / ITEM / ...
	static const int CAT_MODULE = 3;	// SYSTEM.
	static const int CAT_EFFECTS = 4;

	protected ref array<ref DCO_CatalogEntry> m_Entries = {};
	protected ref array<FactionKey> m_FactionKeys = {};	// distinct factions that actually have content.
	protected static ref map<string, bool> s_Expanded;
	protected static BaseWorld s_ExpandedWorld;
	protected SCR_PlacingEditorComponent m_Placing;
	protected bool m_bBuilt;
	protected int m_iBrowserInfoCount;
	protected int m_iPlacingPrefabCount;
	protected ref array<string> m_SourceIdentity = {};

	bool IsBuilt() { return m_bBuilt; }
	int GetEntryCount() { return m_Entries.Count(); }
	static bool IsAnimationFxResource(ResourceName resource) { return resource == ANIMATION_FX_RESOURCE; }
	static bool IsArsenalAccessResource(ResourceName resource) { return resource == ARSENAL_ACCESS_RESOURCE; }
	static bool IsVehicleServiceResource(ResourceName resource) { return resource == VEHICLE_SERVICE_RESOURCE; }
	static bool IsBifrostResource(ResourceName resource)
	{
		return resource.Contains("/E_DCO_") || resource.Contains("/E_AIWaypoint_DCO_")
			|| IsAnimationFxResource(resource) || IsArsenalAccessResource(resource) || resource.StartsWith(MISSION_TOOL_PREFIX);
	}
	static bool IsGlobalUtilityResource(ResourceName resource)
	{
		return IsBifrostResource(resource);
	}

	// Resolve the editor components and enumerate every placeable entity.
	bool Build()
	{
		m_Entries.Clear();
		m_FactionKeys.Clear();
		m_iBrowserInfoCount = 0;
		m_iPlacingPrefabCount = 0;

		m_Placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		SCR_ContentBrowserEditorComponent cb = SCR_ContentBrowserEditorComponent.Cast(SCR_ContentBrowserEditorComponent.GetInstance(SCR_ContentBrowserEditorComponent, true));

		bool cacheReady = cb && cb.GetInfoCount() > 0;
		if (cacheReady)
			BuildFromBrowser(cb);
		if (m_Placing)
			BuildFromPlacing(m_Placing);
		AddAnimationFxEntry();
		AddArsenalAccessEntry();
		AddMissionToolEntries();

		CollectFactions();
		CaptureSourceIdentity(cb, m_Placing, m_SourceIdentity);
		m_bBuilt = !m_Entries.IsEmpty();
		return m_bBuilt;
	}

	bool RefreshIfChanged()
	{
		SCR_ContentBrowserEditorComponent browser = SCR_ContentBrowserEditorComponent.Cast(SCR_ContentBrowserEditorComponent.GetInstance(SCR_ContentBrowserEditorComponent, false));
		int browserCount;
		if (browser)
			browserCount = browser.GetInfoCount();

		int placingCount;
		SCR_PlacingEditorComponent placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (placing)
		{
			SCR_PlacingEditorComponentClass placingData = SCR_PlacingEditorComponentClass.Cast(placing.GetEditorComponentData());
			if (placingData)
			{
				array<ResourceName> prefabs = {};
				placingData.GetPrefabs(prefabs, true);
				placingCount = prefabs.Count();
			}
		}

		array<string> currentIdentity = {};
		CaptureSourceIdentity(browser, placing, currentIdentity);
		if (m_bBuilt && browserCount == m_iBrowserInfoCount && placingCount == m_iPlacingPrefabCount && IdentityEqual(currentIdentity))
			return false;
		return Build();
	}

	protected void CaptureSourceIdentity(SCR_ContentBrowserEditorComponent browser, SCR_PlacingEditorComponent placing, notnull array<string> identity)
	{
		identity.Clear();
		if (browser)
		{
			for (int i = 0; i < browser.GetInfoCount(); i++)
				InsertIdentity(identity, browser.GetResourceNamePrefabID(i));
		}
		if (placing)
		{
			SCR_PlacingEditorComponentClass placingData = SCR_PlacingEditorComponentClass.Cast(placing.GetEditorComponentData());
			if (placingData)
			{
				array<ResourceName> prefabs = {};
				placingData.GetPrefabs(prefabs, true);
				foreach (ResourceName prefab : prefabs)
					InsertIdentity(identity, prefab);
			}
		}
	}

	protected void InsertIdentity(notnull array<string> identity, ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return;
		string value = prefab;
		if (!identity.Contains(value))
			identity.Insert(value);
	}

	protected bool IdentityEqual(notnull array<string> currentIdentity)
	{
		if (currentIdentity.Count() != m_SourceIdentity.Count())
			return false;
		foreach (string identity : currentIdentity)
		{
			if (!m_SourceIdentity.Contains(identity))
				return false;
		}
		return true;
	}

	protected int BuildFromBrowser(SCR_ContentBrowserEditorComponent cb)
	{
		int count = cb.GetInfoCount();
		m_iBrowserInfoCount = count;
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

	protected void AddAnimationFxEntry()
	{
		DCO_CatalogEntry entry = new DCO_CatalogEntry();
		entry.m_Prefab = ANIMATION_FX_RESOURCE;
		entry.m_Name = "Animations FX";
		entry.m_Type = EEditableEntityType.CHARACTER;
		entry.m_Category = CAT_EFFECTS;
		entry.m_SubCat = "FX";
		array<EEditableEntityLabel> labels = {};
		entry.m_App6Icon = DCO_App6Icons.GetIcon(labels, entry.m_Name, "", entry.m_Type);
		entry.m_SearchMetadata = "animations animation ai pose emote smoke sit chair lean pushups loiter officer";
		m_Entries.Insert(entry);
	}

	// Keep the object-targeted arsenal installer in the Bifrost utility folder.
	protected void AddArsenalAccessEntry()
	{
		DCO_CatalogEntry entry = new DCO_CatalogEntry();
		entry.m_Prefab = ARSENAL_ACCESS_RESOURCE;
		entry.m_Name = "Arsenal Access";
		entry.m_Type = EEditableEntityType.CHARACTER;
		entry.m_Category = CAT_EFFECTS;
		entry.m_SubCat = "Bifrost";
		entry.m_SearchMetadata = "arsenal access inventory loadout equipment object vehicle prop interaction attachments";
		entry.m_Icon = "{24C2C142CE0F1758}img/icons/ars-crate.edds";
		m_Entries.Insert(entry);
	}

	protected void AddMissionToolEntries()
	{
		for (int tool = DCO_GMMissionTool.RESTORE; tool <= DCO_GMMissionTool.TARGET; tool++)
		{
			DCO_CatalogEntry entry = new DCO_CatalogEntry();
			entry.m_Prefab = MISSION_TOOL_PREFIX + tool.ToString();
			entry.m_iMissionTool = tool;
			entry.m_Name = DCO_GMMissionTool.Name(tool);
			entry.m_Type = EEditableEntityType.SYSTEM;
			entry.m_Category = CAT_EFFECTS;
			entry.m_SubCat = "Bifrost";
			entry.m_BudgetText = "SETUP";
			entry.m_SearchMetadata = "bifrost mission tool action " + entry.m_Name;
			entry.m_SearchMetadata.ToLower();
			entry.m_Icon = "{D6B46B6655BC3FD5}UI/Textures/Editor/ContextMenu/ContextAction_LightningStrike.edds";
			m_Entries.Insert(entry);
		}
	}

	protected int BuildFromPlacing(SCR_PlacingEditorComponent placing)
	{
		SCR_PlacingEditorComponentClass placingData = SCR_PlacingEditorComponentClass.Cast(placing.GetEditorComponentData());
		if (!placingData)
			return m_Entries.Count();
		array<ResourceName> prefabs = {};
		placingData.GetPrefabs(prefabs, true);
		m_iPlacingPrefabCount = prefabs.Count();
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

	protected void AddEntry(ResourceName res, SCR_EditableEntityUIInfo info)
	{
		if (IsArsenalAccessResource(res))
			return; // The explicit utility entry below owns its location and prevents a duplicate Objects row.

		foreach (DCO_CatalogEntry existing : m_Entries)
		{
			if (existing.m_Prefab == res)
				return;
		}
		DCO_CatalogEntry e = new DCO_CatalogEntry();
		e.m_Prefab = res;
		e.m_Name = DCO_GMDisplayName.Resolve(info.GetName(), res, "Entity");
		e.m_Icon = info.GetImage();
		e.m_Faction = info.GetFactionKey();
		e.m_Type = info.GetEntityType();
		e.m_Category = CategoryForType(info.GetEntityType());
		// Every Bifrost-authored placeable is discoverable under the Lightning tab.
		if (IsBifrostResource(res))
			e.m_Category = CAT_EFFECTS;
		e.m_BudgetText = BudgetText(info);
		array<EEditableEntityLabel> labels = {};
		info.GetEntityLabels(labels);
		e.m_App6Icon = DCO_App6Icons.GetIcon(labels, e.m_Name, e.m_Faction, e.m_Type);
		e.m_SubCat = SubCatFor(res);
		if (e.m_Category == CAT_EFFECTS)
		{
			if (res.Contains("/E_DCO_Fx"))
				e.m_SubCat = "FX";
			else
				e.m_SubCat = "Bifrost";
		}
		e.m_SearchMetadata = string.Format("%1 %2 %3 %4 %5", res, e.m_Faction, CategoryLabel(e.m_Category), TypeLabel(e.m_Type), e.m_SubCat);
		e.m_SearchMetadata.ToLower();
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
		DCO_FactionCatalog.SortSubset(m_FactionKeys);
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
			if (!factionFilter.IsEmpty() && entry.m_Faction != factionFilter && !IsGlobalUtilityResource(entry.m_Prefab))
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
		return DCO_FactionCatalog.NameFor(key);
	}

	string GetFactionTabLabel(FactionKey key)
	{
		if (key.IsEmpty())
			return "NEU";
		return key;
	}

	void ToggleSection(string sectionKey)
	{
		map<string, bool> expandedState = ExpansionState();
		bool expanded = false;
		expandedState.Find(sectionKey, expanded);
		expandedState.Set(sectionKey, !expanded);
	}

	protected static map<string, bool> ExpansionState()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!s_Expanded || s_ExpandedWorld != world)
		{
			s_ExpandedWorld = world;
			s_Expanded = new map<string, bool>();
		}
		return s_Expanded;
	}

	// Nested folders default collapsed so the 3-level browser never emits ~1500 item rows on entry.
	protected bool IsCollapsed(string sectionKey)
	{
		bool expanded = false;
		ExpansionState().Find(sectionKey, expanded);
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
		if ((e.m_Category == CAT_OBJECT || e.m_Category == CAT_EFFECTS) && !e.m_SubCat.IsEmpty())
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
		r.m_iMissionTool = e.m_iMissionTool;
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
		// 1.
		array<ref DCO_CatalogEntry> candidates = {};
		foreach (DCO_CatalogEntry e : m_Entries)
		{
			if (categoryFilter != CAT_ALL && e.m_Category != categoryFilter)
				continue;
			if (!factionFilter.IsEmpty() && e.m_Faction != factionFilter && !IsGlobalUtilityResource(e.m_Prefab))
				continue;
			candidates.Insert(e);
		}

		// 2.
		if (!search.IsEmpty())
		{
			array<string> tokens = {};
			search.Split(" ", tokens, true);
			foreach (string rawToken : tokens)
			{
				string token = rawToken;
				token.ToLower();
				if (token.IsEmpty())
					continue;

				array<string> names = {};
				array<string> factionNames = {};
				foreach (DCO_CatalogEntry candidate : candidates)
				{
					names.Insert(candidate.m_Name);
					factionNames.Insert(GetFactionLabel(candidate.m_Faction));
				}
				array<int> nameHits = {};
				array<int> factionHits = {};
				WidgetManager.SearchLocalized(rawToken, names, nameHits);
				WidgetManager.SearchLocalized(rawToken, factionNames, factionHits);
				map<int, bool> localizedHits = new map<int, bool>();
				foreach (int hit : nameHits)
					localizedHits.Set(hit, true);
				foreach (int hit : factionHits)
					localizedHits.Set(hit, true);

				array<ref DCO_CatalogEntry> matched = {};
				for (int candidateIndex = 0; candidateIndex < candidates.Count(); candidateIndex++)
				{
					DCO_CatalogEntry candidate = candidates[candidateIndex];
					if (candidate.m_SearchMetadata.Contains(token) || localizedHits.Contains(candidateIndex))
						matched.Insert(candidate);
				}
				candidates = matched;
				if (candidates.IsEmpty())
					break;
			}
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
				map<string, bool> expandedState = ExpansionState();
				bool exp;
				if (!expandedState.Find(path, exp))
					expandedState.Set(path, true);
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
		SCR_ContentBrowserEditorComponent browser;
		SCR_EditorContentBrowserSaveStateData browserState;
		int browserStateIndex;
		BeginPlacementAvailability(browser, browserState, browserStateIndex);
		bool ok = m_Placing.SetSelectedPrefab(prefab);
		RestorePlacementAvailability(browser, browserState, browserStateIndex);
		return ok;
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
		SCR_ContentBrowserEditorComponent browser;
		SCR_EditorContentBrowserSaveStateData browserState;
		int browserStateIndex;
		BeginPlacementAvailability(browser, browserState, browserStateIndex);
		bool ok = m_Placing.SetSelectedPrefab(prefab);
		RestorePlacementAvailability(browser, browserState, browserStateIndex);
		return ok;
	}

	void CancelPlacement()
	{
		if (!m_Placing)
			m_Placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (m_Placing && m_Placing.IsPlacing())
			m_Placing.SetSelectedPrefab(ResourceName.Empty);
	}

	protected bool BeginPlacementAvailability(out SCR_ContentBrowserEditorComponent browser, out SCR_EditorContentBrowserSaveStateData state, out int stateIndex)
	{
		if (!m_Placing)
			return false;
		IEntity owner = m_Placing.GetOwner();
		if (!owner)
			return false;
		browser = SCR_ContentBrowserEditorComponent.Cast(owner.FindComponent(SCR_ContentBrowserEditorComponent));
		if (!browser)
		{
			Print("[DCO-GM] placement availability: no content browser on placing owner", LogLevel.WARNING);
			return false;
		}

		array<EEditableEntityLabel> labels = {};
		browser.GetActiveLabels(labels);
		state = new SCR_EditorContentBrowserSaveStateData();
		state.SetLabels(labels);
		state.SetSearchString(browser.GetCurrentSearch());
		state.SetPageIndex(browser.GetPageIndex());
		stateIndex = browser.GetBrowserStateIndex();

		browser.ResetAllLabels(false);
		browser.SetCurrentSearch("");
		browser.FilterEntries();
		return true;
	}

	protected void RestorePlacementAvailability(SCR_ContentBrowserEditorComponent browser, SCR_EditorContentBrowserSaveStateData state, int stateIndex)
	{
		if (!browser || !state)
			return;
		browser.SetBrowserStateIndex(stateIndex);
		browser.SetCustomBrowserState(state, false);
	}

}
