class DCO_TriggerGroupEntry
{
	ResourceName m_Prefab;
	string m_Name;
	FactionKey m_FactionKey;
}

class DCO_TriggerGroupCatalog
{
	protected static ref array<ref DCO_TriggerGroupEntry> s_Entries;
	protected static ref array<FactionKey> s_FactionKeys;
	protected static SCR_EntityCatalogManagerComponent s_Manager;
	protected static bool s_bSubscribed;

	protected static void EnsureSubscription()
	{
		if (s_bSubscribed)
			return;
		SCR_EntityCatalogManagerComponent.GetOnEntityCatalogInitialized().Insert(Invalidate);
		s_bSubscribed = true;
	}

	static void Invalidate()
	{
		s_Entries = null;
		s_FactionKeys = null;
	}

	protected static void Build()
	{
		SCR_EntityCatalogManagerComponent manager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (s_Manager != manager)
		{
			s_Manager = manager;
			Invalidate();
		}
		EnsureSubscription();
		if (s_Entries)
			return;

		s_Entries = {};
		s_FactionKeys = {};
		if (!manager)
			return;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (factionManager)
		{
			array<Faction> factions = {};
			factionManager.GetFactionsList(factions);
			foreach (Faction faction : factions)
			{
				SCR_Faction scriptedFaction = SCR_Faction.Cast(faction);
				if (!scriptedFaction)
					continue;
				SCR_EntityCatalog catalog = manager.GetFactionEntityCatalogOfType(EEntityCatalogType.GROUP, scriptedFaction, false);
				if (catalog)
					AddCatalog(catalog, scriptedFaction.GetFactionKey());
			}
		}
		SCR_EntityCatalog general = manager.GetEntityCatalogOfType(EEntityCatalogType.GROUP, false);
		if (general)
			AddCatalog(general, "");
		DCO_FactionCatalog.SortSubset(s_FactionKeys);
		Print(string.Format("[DCO-TRIGGER] group catalog: %1 groups across %2 factions", s_Entries.Count(), s_FactionKeys.Count()), LogLevel.NORMAL);
	}

	protected static void AddCatalog(SCR_EntityCatalog catalog, FactionKey catalogFaction)
	{
		array<SCR_EntityCatalogEntry> entries = {};
		catalog.GetEntityList(entries);
		foreach (SCR_EntityCatalogEntry catalogEntry : entries)
		{
			if (!catalogEntry)
				continue;
			ResourceName prefab = catalogEntry.GetPrefab();
			if (prefab.IsEmpty() || FindByPrefab(prefab))
				continue;

			FactionKey factionKey = catalogFaction;
			if (factionKey.IsEmpty())
			{
				SCR_EditableEntityUIInfo editableInfo = SCR_EditableEntityUIInfo.Cast(catalogEntry.GetEntityUiInfo());
				if (editableInfo)
					factionKey = editableInfo.GetFactionKey();
			}

			DCO_TriggerGroupEntry entry = new DCO_TriggerGroupEntry();
			entry.m_Prefab = prefab;
			entry.m_Name = catalogEntry.GetEntityName();
			if (entry.m_Name.IsEmpty())
				entry.m_Name = PrefabName(prefab);
			entry.m_FactionKey = factionKey;
			InsertSorted(entry);
			if (!factionKey.IsEmpty() && !s_FactionKeys.Contains(factionKey))
				s_FactionKeys.Insert(factionKey);
		}
	}

	protected static void InsertSorted(DCO_TriggerGroupEntry entry)
	{
		string token = entry.m_Prefab;
		token.ToLower();
		int insertAt = s_Entries.Count();
		for (int i = 0; i < s_Entries.Count(); i++)
		{
			string existing = s_Entries[i].m_Prefab;
			existing.ToLower();
			if (token.Compare(existing) < 0)
			{
				insertAt = i;
				break;
			}
		}
		s_Entries.InsertAt(entry, insertAt);
	}

	protected static string PrefabName(ResourceName prefab)
	{
		string name = prefab.GetPath();
		int slash = name.LastIndexOf("/");
		if (slash >= 0)
			name = name.Substring(slash + 1, name.Length() - slash - 1);
		name.Replace(".et", "");
		name.Replace("_", " ");
		return name;
	}

	static DCO_TriggerGroupEntry FindByPrefab(ResourceName prefab)
	{
		if (!s_Entries)
			return null;
		foreach (DCO_TriggerGroupEntry entry : s_Entries)
		{
			if (entry.m_Prefab == prefab)
				return entry;
		}
		return null;
	}

	static int FactionCount()
	{
		Build();
		return s_FactionKeys.Count();
	}

	static FactionKey FactionKeyAt(int index)
	{
		Build();
		if (index < 0 || index >= s_FactionKeys.Count())
			return "";
		return s_FactionKeys[index];
	}

	static string FactionNameAt(int index)
	{
		return DCO_FactionCatalog.NameFor(FactionKeyAt(index));
	}

	static int FactionIndexOf(FactionKey factionKey)
	{
		Build();
		return s_FactionKeys.Find(factionKey);
	}

	static int GroupCount(FactionKey factionKey)
	{
		Build();
		int count;
		foreach (DCO_TriggerGroupEntry entry : s_Entries)
		{
			if (factionKey.IsEmpty() || entry.m_FactionKey == factionKey)
				count++;
		}
		return count;
	}

	static DCO_TriggerGroupEntry GroupAt(FactionKey factionKey, int index)
	{
		Build();
		int current;
		foreach (DCO_TriggerGroupEntry entry : s_Entries)
		{
			if (!factionKey.IsEmpty() && entry.m_FactionKey != factionKey)
				continue;
			if (current == index)
				return entry;
			current++;
		}
		return null;
	}

	static int GroupIndexOf(FactionKey factionKey, ResourceName prefab)
	{
		Build();
		int current;
		foreach (DCO_TriggerGroupEntry entry : s_Entries)
		{
			if (!factionKey.IsEmpty() && entry.m_FactionKey != factionKey)
				continue;
			if (entry.m_Prefab == prefab)
				return current;
			current++;
		}
		return -1;
	}

	static FactionKey FactionForPrefab(ResourceName prefab)
	{
		Build();
		DCO_TriggerGroupEntry entry = FindByPrefab(prefab);
		if (!entry)
			return "";
		return entry.m_FactionKey;
	}
}
