class DCO_FactionCatalog
{
	protected static const ref array<FactionKey> CANONICAL_KEYS = {"US", "USSR", "FIA", "CIV"};
	protected static ref array<FactionKey> s_Keys;
	protected static ref array<string> s_Names;
	protected static ref array<FactionKey> s_SourceKeys;
	protected static FactionManager s_Manager;

	static void Invalidate()
	{
		s_Keys = null;
		s_Names = null;
		s_SourceKeys = null;
		s_Manager = null;
	}

	protected static void Build()
	{
		FactionManager manager = GetGame().GetFactionManager();
		array<FactionKey> currentKeys = {};
		if (manager)
		{
			array<Faction> currentFactions = {};
			manager.GetFactionsList(currentFactions);
			foreach (Faction currentFaction : currentFactions)
			{
				if (!currentFaction || currentFaction.GetFactionKey().IsEmpty())
					continue;
				InsertSorted(currentKeys, currentFaction.GetFactionKey());
			}
		}
		if (s_Keys && s_Manager == manager && KeysEqual(currentKeys, s_SourceKeys))
			return;

		s_Manager = manager;
		s_SourceKeys = currentKeys;
		s_Keys = {};
		s_Names = {};
		if (!manager)
			return;

		array<Faction> factions = {};
		manager.GetFactionsList(factions);
		foreach (FactionKey canonicalKey : CANONICAL_KEYS)
		{
			Faction canonical = manager.GetFactionByKey(canonicalKey);
			if (canonical)
				Insert(canonical.GetFactionKey(), canonical.GetFactionName());
		}

		array<FactionKey> customKeys = {};
		foreach (Faction faction : factions)
		{
			if (!faction)
				continue;
			FactionKey key = faction.GetFactionKey();
			if (key.IsEmpty() || IsCanonical(key) || customKeys.Contains(key))
				continue;
			InsertSorted(customKeys, key);
		}
		foreach (FactionKey customKey : customKeys)
		{
			Faction custom = manager.GetFactionByKey(customKey);
			string name = customKey;
			if (custom && !custom.GetFactionName().IsEmpty())
				name = custom.GetFactionName();
			Insert(customKey, name);
		}
	}

	protected static bool KeysEqual(array<FactionKey> left, array<FactionKey> right)
	{
		if (!left || !right || left.Count() != right.Count())
			return false;
		for (int i = 0; i < left.Count(); i++)
		{
			if (left[i] != right[i])
				return false;
		}
		return true;
	}

	protected static void Insert(FactionKey key, string name)
	{
		if (s_Keys.Contains(key))
			return;
		if (name.IsEmpty())
			name = key;
		s_Keys.Insert(key);
		s_Names.Insert(name);
	}

	protected static void InsertSorted(notnull array<FactionKey> keys, FactionKey key)
	{
		if (keys.Contains(key))
			return;
		string token = SortToken(key);
		int insertAt = keys.Count();
		for (int i = 0; i < keys.Count(); i++)
		{
			if (token.Compare(SortToken(keys[i])) < 0)
			{
				insertAt = i;
				break;
			}
		}
		keys.InsertAt(key, insertAt);
	}

	protected static string SortToken(FactionKey key)
	{
		string token = key;
		token.ToLower();
		return token;
	}

	static bool IsCanonical(FactionKey key)
	{
		foreach (FactionKey canonicalKey : CANONICAL_KEYS)
		{
			if (key == canonicalKey)
				return true;
		}
		return false;
	}

	static int Compare(FactionKey left, FactionKey right)
	{
		return SortToken(left).Compare(SortToken(right));
	}

	static int Count()
	{
		Build();
		return s_Keys.Count();
	}

	static int TargetCount()
	{
		return Count() + 1;
	}

	static FactionKey TargetKeyAt(int index)
	{
		if (index <= 0)
			return "";
		return KeyAt(index - 1);
	}

	static string TargetNameAt(int index)
	{
		if (index <= 0)
			return "Off - emitter point";
		return NameAt(index - 1);
	}

	static FactionKey KeyAt(int index)
	{
		Build();
		if (index < 0 || index >= s_Keys.Count())
			return "";
		return s_Keys[index];
	}

	static string NameAt(int index)
	{
		Build();
		if (index < 0 || index >= s_Names.Count())
			return "Unknown faction";
		return s_Names[index];
	}

	static string NameFor(FactionKey key)
	{
		if (key.IsEmpty())
			return "Neutral";
		FactionManager manager = GetGame().GetFactionManager();
		if (manager)
		{
			Faction faction = manager.GetFactionByKey(key);
			if (faction && !faction.GetFactionName().IsEmpty())
				return faction.GetFactionName();
		}
		return key;
	}

	static int IndexOf(FactionKey key)
	{
		Build();
		return s_Keys.Find(key);
	}

	static void GetKeys(out notnull array<FactionKey> keys)
	{
		Build();
		keys.Clear();
		foreach (FactionKey key : s_Keys)
			keys.Insert(key);
	}

	static void SortSubset(notnull array<FactionKey> keys)
	{
		array<FactionKey> sorted = {};
		foreach (FactionKey known : CANONICAL_KEYS)
		{
			if (keys.Contains(known))
				sorted.Insert(known);
		}
		array<FactionKey> custom = {};
		foreach (FactionKey key : keys)
		{
			if (!IsCanonical(key))
				InsertSorted(custom, key);
		}
		foreach (FactionKey key : custom)
			sorted.Insert(key);
		keys.Copy(sorted);
	}
}
