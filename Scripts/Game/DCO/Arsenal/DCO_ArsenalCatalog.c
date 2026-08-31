// Builds a familiar GM arsenal by merging faction item catalogs into shared equipment categories.

enum EDCO_ArsenalCategory
{
	PRIMARY,
	PISTOL,
	LAUNCHER,
	UNIFORM,
	VEST,
	BACKPACK,
	HEADGEAR,
	ITEMS,
	GRENADES,
	MAGAZINES,
	ATTACHMENTS
}

class DCO_ArsenalEntry
{
	ResourceName m_Prefab;
	string m_sName;
	EDCO_ArsenalCategory m_eCategory;
	string m_sFactionKey;
	SCR_EArsenalItemType m_eType;
}

class DCO_ArsenalCatalog
{
	protected static ref DCO_ArsenalCatalog s_Inst;
	static DCO_ArsenalCatalog Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_ArsenalCatalog();
		return s_Inst;
	}

	protected ref array<ref DCO_ArsenalEntry> m_Entries = {};
	protected ref array<string> m_FactionKeys = {};
	protected bool m_bReady;

	bool IsReady()
	{
		return m_bReady;
	}

	int GetFactionKeys(notnull array<string> outKeys)
	{
		outKeys.Clear();
		foreach (string k : m_FactionKeys)
			outKeys.Insert(k);
		return outKeys.Count();
	}

	// Category of a type+mode pair.
	static EDCO_ArsenalCategory CategoryOf(SCR_EArsenalItemType t, SCR_EArsenalItemMode m)
	{
		if (m == SCR_EArsenalItemMode.AMMUNITION)
			return EDCO_ArsenalCategory.MAGAZINES;
		switch (t)
		{
			case SCR_EArsenalItemType.RIFLE:                return EDCO_ArsenalCategory.PRIMARY;
			case SCR_EArsenalItemType.MACHINE_GUN:          return EDCO_ArsenalCategory.PRIMARY;
			case SCR_EArsenalItemType.SNIPER_RIFLE:         return EDCO_ArsenalCategory.PRIMARY;
			case SCR_EArsenalItemType.PISTOL:               return EDCO_ArsenalCategory.PISTOL;
			case SCR_EArsenalItemType.ROCKET_LAUNCHER:      return EDCO_ArsenalCategory.LAUNCHER;
			case SCR_EArsenalItemType.MORTARS:              return EDCO_ArsenalCategory.LAUNCHER;
			case SCR_EArsenalItemType.TORSO:                return EDCO_ArsenalCategory.UNIFORM;
			case SCR_EArsenalItemType.LEGS:                 return EDCO_ArsenalCategory.UNIFORM;
			case SCR_EArsenalItemType.FOOTWEAR:             return EDCO_ArsenalCategory.UNIFORM;
			case SCR_EArsenalItemType.HANDWEAR:             return EDCO_ArsenalCategory.UNIFORM;
			case SCR_EArsenalItemType.VEST_AND_WAIST:       return EDCO_ArsenalCategory.VEST;
			case SCR_EArsenalItemType.BACKPACK:             return EDCO_ArsenalCategory.BACKPACK;
			case SCR_EArsenalItemType.RADIO_BACKPACK:       return EDCO_ArsenalCategory.BACKPACK;
			case SCR_EArsenalItemType.HEADWEAR:             return EDCO_ArsenalCategory.HEADGEAR;
			case SCR_EArsenalItemType.LETHAL_THROWABLE:     return EDCO_ArsenalCategory.GRENADES;
			case SCR_EArsenalItemType.NON_LETHAL_THROWABLE: return EDCO_ArsenalCategory.GRENADES;
			case SCR_EArsenalItemType.EXPLOSIVES:           return EDCO_ArsenalCategory.GRENADES;
			case SCR_EArsenalItemType.WEAPON_ATTACHMENT:    return EDCO_ArsenalCategory.ATTACHMENTS;
		}
		return EDCO_ArsenalCategory.ITEMS;	// HEAL / EQUIPMENT / anything future.
	}

	void Build()
	{
		if (m_bReady)
			return;
		SCR_EntityCatalogManagerComponent mgr = SCR_EntityCatalogManagerComponent.GetInstance();
		FactionManager fm = GetGame().GetFactionManager();
		if (!mgr || !fm)
		{
			Print("[DCO-ARS] catalog build: no catalog manager / faction manager yet", LogLevel.WARNING);
			return;
		}

		array<Faction> factions = {};
		fm.GetFactionsList(factions);
		foreach (Faction f : factions)
		{
			SCR_Faction sf = SCR_Faction.Cast(f);
			if (!sf)
				continue;
			SCR_EntityCatalog cat = mgr.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM, sf, false);
			if (!cat)
				continue;

			array<SCR_EntityCatalogEntry> entries = {};
			array<SCR_BaseEntityCatalogData> dataList = {};
			cat.GetEntityListWithData(SCR_ArsenalItem, entries, dataList);

			string fkey = sf.GetFactionKey();
			bool factionUsed = false;
			for (int i = 0; i < entries.Count(); i++)
			{
				SCR_ArsenalItem data = SCR_ArsenalItem.Cast(dataList[i]);
				if (!data)
					continue;
				SCR_EArsenalItemType t = data.GetItemType();
				if (t == SCR_EArsenalItemType.VEHICLE || t == SCR_EArsenalItemType.HELICOPTER)
					continue;	// vehicle-depot entries, not character gear.

				DCO_ArsenalEntry e = new DCO_ArsenalEntry();
				e.m_Prefab = data.GetItemResourceName();
				if (e.m_Prefab.IsEmpty())
					e.m_Prefab = entries[i].GetPrefab();
				e.m_sName = DisplayNameOf(entries[i], e.m_Prefab);
				e.m_eType = t;
				e.m_eCategory = CategoryOf(t, data.GetItemMode());
				e.m_sFactionKey = fkey;
				m_Entries.Insert(e);
				factionUsed = true;
			}
			if (factionUsed && !m_FactionKeys.Contains(fkey))
				m_FactionKeys.Insert(fkey);
		}
		SortByName();
		m_bReady = !m_Entries.IsEmpty();
	}

	protected void SortByName()
	{
		array<string> keys = {};
		for (int i = 0; i < m_Entries.Count(); i++)
			keys.Insert(m_Entries[i].m_sName + "|" + i.ToString());
		keys.Sort();
		array<ref DCO_ArsenalEntry> sorted = {};
		foreach (string k : keys)
		{
			int bar = k.LastIndexOf("|");
			if (bar < 0)
				continue;
			int idx = k.Substring(bar + 1, k.Length() - bar - 1).ToInt();
			sorted.Insert(m_Entries[idx]);
		}
		m_Entries = sorted;
	}

	protected string DisplayNameOf(SCR_EntityCatalogEntry entry, ResourceName prefab)
	{
		SCR_UIInfo info = entry.GetEntityUiInfo();
		if (info)
		{
			string nm = info.GetName();
			if (!nm.IsEmpty())
				return nm;
		}
		string s = prefab.GetPath();
		int slash = s.LastIndexOf("/");
		if (slash >= 0)
			s = s.Substring(slash + 1, s.Length() - slash - 1);
		s.Replace(".et", "");
		return s;
	}

	// Entry for an exact prefab, or null.
	DCO_ArsenalEntry FindByPrefab(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return null;
		foreach (DCO_ArsenalEntry e : m_Entries)
		{
			if (e.m_Prefab == prefab)
				return e;
		}
		return null;
	}

	int GetEntries(EDCO_ArsenalCategory cat, string search, string factionKey, notnull array<DCO_ArsenalEntry> outEntries)
	{
		outEntries.Clear();
		string needle = search;
		needle.ToLower();
		foreach (DCO_ArsenalEntry e : m_Entries)
		{
			if (e.m_eCategory != cat)
				continue;
			if (!factionKey.IsEmpty() && e.m_sFactionKey != factionKey)
				continue;
			if (!needle.IsEmpty())
			{
				string hay = e.m_sName;
				hay.ToLower();
				if (!hay.Contains(needle))
					continue;
			}
			outEntries.Insert(e);
		}
		return outEntries.Count();
	}
}
