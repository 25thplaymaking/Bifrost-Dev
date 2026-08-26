class DCO_FxAircraftCatalog
{
	protected static ref array<ResourceName> s_aPrefabs;
	protected static ref array<string> s_aNames;

	static void Build()
	{
		if (s_aPrefabs)
			return;
		s_aPrefabs = {};
		s_aNames = {};
		s_aPrefabs.Insert(DCO_FxExplosionComponent.FLYBY_AIRCRAFT);
		s_aNames.Insert("UH-1H (default)");

		SCR_EntityCatalogManagerComponent mgr = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!mgr)
			return;

		array<SCR_EntityCatalog> catalogs = {};
		SCR_EntityCatalog general = mgr.GetEntityCatalogOfType(EEntityCatalogType.VEHICLE, false);
		if (general)
			catalogs.Insert(general);
		FactionManager fm = GetGame().GetFactionManager();
		if (fm)
		{
			array<Faction> factions = {};
			fm.GetFactionsList(factions);
			foreach (Faction f : factions)
			{
				SCR_Faction sf = SCR_Faction.Cast(f);
				if (!sf)
					continue;
				SCR_EntityCatalog fc = mgr.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, sf, false);
				if (fc && catalogs.Find(fc) < 0)
					catalogs.Insert(fc);
			}
		}

		foreach (SCR_EntityCatalog cat : catalogs)
		{
			array<SCR_EntityCatalogEntry> entries = {};
			cat.GetEntityListWithLabel(EEditableEntityLabel.VEHICLE_HELICOPTER, entries);
			foreach (SCR_EntityCatalogEntry entry : entries)
			{
				if (!entry)
					continue;
				ResourceName prefab = entry.GetPrefab();
				if (prefab.IsEmpty() || s_aPrefabs.Find(prefab) >= 0)
					continue;
				string name = entry.GetEntityName();
				if (name.IsEmpty())
					name = prefab;
				s_aPrefabs.Insert(prefab);
				s_aNames.Insert(name);
			}
		}
		Print(string.Format("[DCO-FX] aircraft catalog: %1 helicopter(s) discovered", s_aPrefabs.Count()), LogLevel.NORMAL);
	}

	static int Count()
	{
		Build();
		return s_aPrefabs.Count();
	}

	static string NameAt(int i)
	{
		Build();
		if (i < 0 || i >= s_aNames.Count())
			return "?";
		return s_aNames[i];
	}

	static ResourceName PrefabAt(int i)
	{
		Build();
		if (i < 0 || i >= s_aPrefabs.Count())
			return DCO_FxExplosionComponent.FLYBY_AIRCRAFT;
		return s_aPrefabs[i];
	}

	// Index of a stored prefab, or 0 (the default) when it is unknown in THIS session's mod set.
	static int IndexOf(ResourceName prefab)
	{
		Build();
		if (prefab.IsEmpty())
			return 0;
		int at = s_aPrefabs.Find(prefab);
		if (at < 0)
			return 0;
		return at;
	}
}
