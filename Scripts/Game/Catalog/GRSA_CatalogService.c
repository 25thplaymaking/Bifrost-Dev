class GRSA_ItemEntry
{
	ResourceName m_Prefab;
	string m_sDisplayName;
	SCR_EArsenalItemType m_eType;
	SCR_EArsenalItemMode m_eMode;
	int m_iSupplyCost;
	SCR_ECharacterRank m_eRequiredRank;
	bool m_bUseMilitarySupplyAllocation;
}

class GRSA_CatalogService
{
	// Built on first use - eager static initializers charge the module-init budget shared by every loaded mod.
	protected static ref map<string, ref array<ref GRSA_ItemEntry>> s_mCategoryCache;
	protected static ref map<ResourceName, string> s_mDisplayNameCache;
	protected static ref array<ref GRSA_ArmoryCategory> s_aAutoCategories;

	protected static void EnsureCaches()
	{
		if (s_mCategoryCache)
			return;

		s_mCategoryCache = new map<string, ref array<ref GRSA_ItemEntry>>();
		s_mDisplayNameCache = new map<ResourceName, string>();
		s_aAutoCategories = new array<ref GRSA_ArmoryCategory>();
	}

	//------------------------------------------------------------------------------------------------
	//! Session cache is keyed by faction and category, cleared every menu open so GM arsenal edits show up.
	static void ClearSessionCache()
	{
		EnsureCaches();
		s_mCategoryCache.Clear();
		s_aAutoCategories.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves the active catalog source once for every browser path. All Factions combines the
	//! general catalog with every loaded faction catalog while preserving their authored order.
	protected static void CollectSourceItems(notnull out array<SCR_ArsenalItem> outItems, SCR_Faction faction, SCR_ArsenalComponent arsenal, SCR_EArsenalItemType typeMask, SCR_EArsenalItemMode modeMask)
	{
		outItems.Clear();
		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
			return;

		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		GRSA_ECatalogScope scope = settings.m_eCatalogScope;
		GRSA_ArmoryConfig config = GRSA_ConfigHolder.GetDefault();
		if (scope == GRSA_ECatalogScope.ALL_FACTIONS && config && config.m_bRestrictToArsenalConfig)
			scope = GRSA_ECatalogScope.STATION_ARSENAL;

		if (scope == GRSA_ECatalogScope.STATION_ARSENAL && arsenal)
		{
			arsenal.GetFilteredArsenalItems(outItems);
			return;
		}

		if (scope == GRSA_ECatalogScope.STATION_ARSENAL)
			scope = GRSA_ECatalogScope.PLAYER_FACTION;

		SCR_EArsenalGameModeType gameModeType = SCR_ArsenalManagerComponent.GetArsenalGameModeType_Static();
		if (scope == GRSA_ECatalogScope.PLAYER_FACTION)
		{
			if (faction)
				catalogManager.GetFactionArsenalItems(outItems, faction, typeMask, modeMask, gameModeType);
			else
				catalogManager.GetArsenalItems(outItems, typeMask, modeMask, gameModeType);
			return;
		}

		map<ResourceName, bool> seen = new map<ResourceName, bool>();
		array<SCR_ArsenalItem> catalogItems = {};
		catalogManager.GetArsenalItems(catalogItems, typeMask, modeMask, gameModeType);
		AppendUniqueItems(catalogItems, outItems, seen);

		FactionManager factionManager = GetGame().GetFactionManager();
		if (factionManager)
		{
			array<Faction> factions = {};
			factionManager.GetFactionsList(factions);
			foreach (Faction loadedFaction : factions)
			{
				SCR_Faction scrFaction = SCR_Faction.Cast(loadedFaction);
				if (!scrFaction)
					continue;

				catalogItems.Clear();
				if (catalogManager.GetFactionArsenalItems(catalogItems, scrFaction, typeMask, modeMask, gameModeType))
					AppendUniqueItems(catalogItems, outItems, seen);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendUniqueItems(notnull array<SCR_ArsenalItem> source, notnull array<SCR_ArsenalItem> destination, notnull map<ResourceName, bool> seen)
	{
		foreach (SCR_ArsenalItem item : source)
		{
			if (!item)
				continue;

			ResourceName prefab = item.GetItemResourceName();
			if (prefab.IsEmpty() || seen.Contains(prefab))
				continue;

			seen.Insert(prefab, true);
			destination.Insert(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds the outfit rail from the slots the character ACTUALLY has: authored categories keep
	//! their order when the character carries that area, every remaining character slot area gets an
	//! auto category, and authored areas with no slot on this character are dropped. Item-side
	//! accessory areas (vest rails, PTT mounts) never reach the rail because they are not character
	//! loadout slots.
	static void BuildCharacterSlotCategories(notnull array<GRSA_ArmoryCategory> categories, GameEntity character, SCR_ArsenalComponent arsenal, SCR_Faction faction)
	{
		EnsureCaches();
		if (!character)
			return;

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
		if (!loadoutStorage)
			return;

		array<string> characterAreas = {};
		int slotsCount = loadoutStorage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			LoadoutSlotInfo slotInfo = LoadoutSlotInfo.Cast(loadoutStorage.GetSlot(i));
			if (!slotInfo || !slotInfo.GetAreaType())
				continue;

			string area = slotInfo.GetAreaType().Type().ToString();
			if (!area.IsEmpty() && characterAreas.Find(area) < 0)
				characterAreas.Insert(area);
		}

		if (characterAreas.IsEmpty())
			return;

		array<GRSA_ArmoryCategory> ordered = {};
		array<string> coveredAreas = {};
		foreach (GRSA_ArmoryCategory category : categories)
		{
			if (!category)
				continue;

			if (category.m_sClothArea.IsEmpty())
			{
				ordered.Insert(category);
				continue;
			}

			if (characterAreas.Find(category.m_sClothArea) < 0)
				continue;

			ordered.Insert(category);
			coveredAreas.Insert(category.m_sClothArea);
		}

		array<string> stockedAreas = {};
		array<SCR_ArsenalItem> arsenalItems = {};
		CollectSourceItems(arsenalItems, faction, arsenal, -1, SCR_EArsenalItemMode.DEFAULT);

		foreach (SCR_ArsenalItem arsenalItem : arsenalItems)
		{
			if (!arsenalItem || !(arsenalItem.GetItemMode() & SCR_EArsenalItemMode.DEFAULT) || !GRSA_ArsenalScenarioSettings.Get().AllowsArsenalItem(arsenalItem))
				continue;

			ResourceName itemPrefab = arsenalItem.GetItemResourceName();
			if (itemPrefab.IsEmpty())
				continue;

			string itemArea = GRSA_ItemIntel.GetClothAreaType(itemPrefab);
			if (!itemArea.IsEmpty() && stockedAreas.Find(itemArea) < 0)
				stockedAreas.Insert(itemArea);
		}

		if (s_aAutoCategories.IsEmpty())
		{
			foreach (string area : characterAreas)
			{
				if (coveredAreas.Find(area) >= 0)
					continue;

				if (stockedAreas.Find(area) < 0)
				{
					continue;
				}

				GRSA_ArmoryCategory autoCategory = new GRSA_ArmoryCategory();
				autoCategory.m_sDisplayName = PrettyAreaName(area);
				autoCategory.m_eItemTypes = -1;
				autoCategory.m_eItemModes = SCR_EArsenalItemMode.DEFAULT;
				autoCategory.m_eTab = GRSA_EArmoryTab.OUTFIT;
				autoCategory.m_iWeaponSlot = -1;
				autoCategory.m_sClothArea = area;
				s_aAutoCategories.Insert(autoCategory);
			}
		}

		foreach (GRSA_ArmoryCategory autoCategory : s_aAutoCategories)
		{
			if (coveredAreas.Find(autoCategory.m_sClothArea) < 0 && characterAreas.Find(autoCategory.m_sClothArea) >= 0)
				ordered.Insert(autoCategory);
		}

		categories.Clear();
		foreach (GRSA_ArmoryCategory category : ordered)
		{
			categories.Insert(category);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! "ZEL_NeckArea" reads Neck, "LoadoutSalineBagArea" reads Saline Bag.
	static string PrettyAreaName(string areaClass)
	{
		string name = areaClass;
		int underscore = name.LastIndexOf("_");
		if (underscore >= 0)
			name = name.Substring(underscore + 1, name.Length() - underscore - 1);

		if (name.EndsWith("Area"))
			name = name.Substring(0, name.Length() - 4);

		if (name.IndexOf("Loadout") == 0)
			name = name.Substring(7, name.Length() - 7);

		name.Replace("Area", "");

		if (name.IsEmpty())
			return areaClass;

		string spaced;
		for (int i = 0; i < name.Length(); i++)
		{
			string ch = name.Get(i);
			string lower = ch;
			lower.ToLower();
			if (i > 0 && ch != lower)
			{
				string prev = name.Get(i - 1);
				string prevLower = prev;
				prevLower.ToLower();
				if (prev == prevLower)
					spaced += " ";
			}
			spaced += ch;
		}
		return spaced;
	}

	//------------------------------------------------------------------------------------------------
	//! Rows keep the catalog's hand-curated order (variants clustered, quartermaster-shelf layout,
	//! m_iCatalogIndex) — never re-sort the result, alphabetical order scatters the variant clusters.
	static array<ref GRSA_ItemEntry> GetItems(notnull GRSA_ArmoryCategory category, SCR_Faction faction, SCR_ArsenalComponent arsenal, SCR_EArsenalSupplyCostType costType)
	{
		string factionKey = "none";
		if (faction)
			factionKey = faction.GetFactionKey();

		EnsureCaches();
		string cacheKey = string.Format("%1|%2|%3|%4", factionKey, category.m_sDisplayName, category.m_eTab, GRSA_ArsenalScenarioSettings.Get().Pack());
		array<ref GRSA_ItemEntry> cached = s_mCategoryCache.Get(cacheKey);
		if (cached)
			return cached;

		array<ref GRSA_ItemEntry> result = {};
		s_mCategoryCache.Insert(cacheKey, result);

		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
			return result;

		SCR_EArsenalItemType typeMask = category.m_eItemTypes;
		SCR_EArsenalItemMode modeMask = category.m_eItemModes;
		if (modeMask == 0)
			modeMask = SCR_EArsenalItemMode.DEFAULT | SCR_EArsenalItemMode.WEAPON | SCR_EArsenalItemMode.WEAPON_VARIANTS;

		if (typeMask == 0 || modeMask == 0)
			return result;

		array<SCR_ArsenalItem> arsenalItems = {};
		CollectSourceItems(arsenalItems, faction, arsenal, typeMask, modeMask);

		foreach (SCR_ArsenalItem arsenalItem : arsenalItems)
		{
			if (!arsenalItem)
				continue;
			if (!GRSA_ArsenalScenarioSettings.Get().AllowsArsenalItem(arsenalItem))
				continue;

			if (!(arsenalItem.GetItemType() & typeMask))
				continue;

			if (!(arsenalItem.GetItemMode() & modeMask))
				continue;

			ResourceName prefab = arsenalItem.GetItemResourceName();
			if (prefab.IsEmpty())
				continue;

			if (!category.m_sClothArea.IsEmpty())
			{
				if (GRSA_ItemIntel.GetClothAreaType(prefab) != category.m_sClothArea)
					continue;
			}
			else if (category.m_eTab == GRSA_EArmoryTab.GEAR && !GRSA_ItemIntel.GetClothAreaType(prefab).IsEmpty() && !(arsenalItem.GetItemType() & SCR_EArsenalItemType.HEAL) && !(arsenalItem.GetItemMode() & SCR_EArsenalItemMode.CONSUMABLE))
			{
				continue;
			}

			GRSA_ItemEntry entry = new GRSA_ItemEntry();
			entry.m_Prefab = prefab;
			entry.m_sDisplayName = GetDisplayName(prefab, faction);
			entry.m_eType = arsenalItem.GetItemType();
			entry.m_eMode = arsenalItem.GetItemMode();
			entry.m_iSupplyCost = arsenalItem.GetSupplyCost(costType);
			entry.m_eRequiredRank = arsenalItem.GetRequiredRank();
			entry.m_bUseMilitarySupplyAllocation = arsenalItem.GetUseMilitarySupplyAllocation();
			result.Insert(entry);
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Catalog name decorated with filename variant tokens, so M16A2 variants read as
	//! "M16A2 Carbine M203 OliveGreen" instead of six identical rows.
	static string GetDisplayName(ResourceName prefab, SCR_Faction faction)
	{
		EnsureCaches();
		string cached = s_mDisplayNameCache.Get(prefab);
		if (!cached.IsEmpty())
			return cached;

		string name;
		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (catalogManager)
		{
			SCR_EntityCatalogEntry entry;
			if (faction)
				entry = catalogManager.GetEntryWithPrefabFromGeneralOrFactionCatalog(EEntityCatalogType.ITEM, prefab, faction);
			else
				entry = catalogManager.GetEntryWithPrefabFromCatalog(EEntityCatalogType.ITEM, prefab);
			if (!entry)
				entry = catalogManager.GetEntryWithPrefabFromAnyCatalog(EEntityCatalogType.ITEM, prefab);
			if (entry)
			{
				string nameId = entry.GetEntityName();
				if (!HasKnownMissingBaseName(nameId))
					name = WidgetManager.Translate(nameId);
			}
		}

		if (name.IsEmpty())
			name = NameFromPrefabPath(prefab);
		else
			name = DecorateWithVariant(name, prefab);

		s_mDisplayNameCache.Insert(prefab, name);
		return name;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasKnownMissingBaseName(string nameId)
	{
		nameId.Replace("#", string.Empty);
		return nameId == "AR-Weapon_ExplosiveCharge_M112_Name"
			|| nameId == "AR-Weapon_ExplosiveCharge_TNT400g_Name"
			|| nameId == "AR-AmmunitionID_556x45";
	}

	//------------------------------------------------------------------------------------------------
	protected static string DecorateWithVariant(string baseName, ResourceName prefab)
	{
		string fileName = FileStem(prefab);
		if (fileName.IsEmpty())
			return baseName;

		string baseNormalized = NormalizeForCompare(baseName);

		array<string> tokens = {};
		fileName.Split("_", tokens, true);

		string suffix;
		string appendedNormalized;
		foreach (string token : tokens)
		{
			if (token.IsEmpty())
				continue;

			string tokenNormalized = NormalizeForCompare(token);
			if (tokenNormalized.IsEmpty())
				continue;

			if (tokenNormalized == "rifle" || tokenNormalized == "pistol" || tokenNormalized == "handgun" || tokenNormalized == "launcher" || tokenNormalized == "weapon" || tokenNormalized == "mg" || tokenNormalized == "smg" || tokenNormalized == "sniper")
				continue;
			if (tokenNormalized == "base" || tokenNormalized == "tutorial" || tokenNormalized == "gadget" || tokenNormalized == "freeroambuilding" || tokenNormalized == "item" || tokenNormalized == "us" || tokenNormalized == "etool")
				continue;
			if (baseNormalized.IndexOf(tokenNormalized) >= 0)
				continue;
			if (appendedNormalized.IndexOf("|" + tokenNormalized + "|") >= 0)
				continue;

			appendedNormalized += "|" + tokenNormalized + "|";
			if (!suffix.IsEmpty())
				suffix += " ";
			suffix += HumanizeToken(token);
		}

		if (suffix.IsEmpty())
			return baseName;

		return string.Format("%1 %2", baseName, suffix);
	}

	//------------------------------------------------------------------------------------------------
	//! Lowercased alphanumerics only, so "AN/PRC-68" and "ANPRC68" compare equal.
	protected static string NormalizeForCompare(string value)
	{
		string lower = value;
		lower.ToLower();

		string result;
		int length = lower.Length();
		for (int i = 0; i < length; i++)
		{
			string ch = lower.Get(i);
			if (KEEP_CHARS.IndexOf(ch) >= 0)
				result += ch;
		}
		return result;
	}

	protected static const string KEEP_CHARS = "abcdefghijklmnopqrstuvwxyz0123456789";

	//------------------------------------------------------------------------------------------------
	protected static string CapFirst(string value)
	{
		if (value.IsEmpty())
			return value;

		string first = value.Substring(0, 1);
		first.ToUpper();
		return first + value.Substring(1, value.Length() - 1);
	}

	//------------------------------------------------------------------------------------------------
	//! Turns filename variant tokens such as OliveGreen and SandStripes into readable labels.
	protected static string HumanizeToken(string value)
	{
		string result;
		for (int i = 0; i < value.Length(); i++)
		{
			string ch = value.Get(i);
			if (i > 0)
			{
				string previous = value.Get(i - 1);
				string previousLower = previous;
				string previousUpper = previous;
				string currentLower = ch;
				string currentUpper = ch;
				previousLower.ToLower();
				previousUpper.ToUpper();
				currentLower.ToLower();
				currentUpper.ToUpper();

				bool previousIsLower = previous == previousLower && previous != previousUpper;
				bool currentIsUpper = ch == currentUpper && ch != currentLower;
				if (previousIsLower && currentIsUpper)
					result += " ";
			}

			result += ch;
		}

		return CapFirst(result);
	}

	//------------------------------------------------------------------------------------------------
	protected static string FileStem(ResourceName prefab)
	{
		string path = prefab;
		int brace = path.LastIndexOf("}");
		if (brace >= 0)
			path = path.Substring(brace + 1, path.Length() - brace - 1);
		int slash = path.LastIndexOf("/");
		if (slash >= 0)
			path = path.Substring(slash + 1, path.Length() - slash - 1);
		int dot = path.LastIndexOf(".");
		if (dot >= 0)
			path = path.Substring(0, dot);
		return path;
	}

	//------------------------------------------------------------------------------------------------
	//! Every item any category of a tab lists, deduplicated by prefab — the raw pool callers filter
	//! with their own predicate (e.g. per-hardpoint compatibility).
	static void CollectTabItems(GRSA_ArmoryConfig config, GRSA_EArmoryTab tab, SCR_Faction faction, SCR_ArsenalComponent arsenal, SCR_EArsenalSupplyCostType costType, notnull out array<ref GRSA_ItemEntry> outItems)
	{
		outItems.Clear();
		if (!config)
			return;

		map<ResourceName, bool> seen = new map<ResourceName, bool>();
		array<GRSA_ArmoryCategory> categories = {};
		config.GetCategoriesForTab(tab, categories);

		foreach (GRSA_ArmoryCategory category : categories)
		{
			array<ref GRSA_ItemEntry> items = GetItems(category, faction, arsenal, costType);
			if (!items)
				continue;

			foreach (GRSA_ItemEntry item : items)
			{
				if (!item || seen.Contains(item.m_Prefab))
					continue;

				seen.Insert(item.m_Prefab, true);
				outItems.Insert(item);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Weapon-slot categories of the WEAPONS tab — the classes a receiver browser pages through.
	static void GetWeaponSlotCategories(GRSA_ArmoryConfig config, notnull out array<GRSA_ArmoryCategory> categories)
	{
		categories.Clear();
		if (!config)
			return;

		array<GRSA_ArmoryCategory> tabCategories = {};
		config.GetCategoriesForTab(GRSA_EArmoryTab.WEAPONS, tabCategories);
		foreach (GRSA_ArmoryCategory category : tabCategories)
		{
			if (category && category.m_iWeaponSlot >= 0)
				categories.Insert(category);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Entry for a prefab across the given categories. Entries are owned by the session cache, so
	//! the returned reference stays valid until the cache is cleared on the next menu open.
	static GRSA_ItemEntry FindEntry(ResourceName prefab, notnull array<GRSA_ArmoryCategory> categories, SCR_Faction faction, SCR_ArsenalComponent arsenal, SCR_EArsenalSupplyCostType costType)
	{
		if (prefab.IsEmpty())
			return null;

		foreach (GRSA_ArmoryCategory category : categories)
		{
			if (!category)
				continue;

			array<ref GRSA_ItemEntry> items = GetItems(category, faction, arsenal, costType);
			if (!items)
				continue;

			foreach (GRSA_ItemEntry item : items)
			{
				if (item && item.m_Prefab == prefab)
					return item;
			}
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	static SCR_ArsenalItem FindArsenalItemData(ResourceName prefab, SCR_Faction faction)
	{
		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
			return null;

		SCR_EntityCatalogEntry entry = catalogManager.GetEntryWithPrefabFromAnyCatalog(EEntityCatalogType.ITEM, prefab);
		if (!entry)
			return null;

		return SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
	}

	//------------------------------------------------------------------------------------------------
	protected static string NameFromPrefabPath(ResourceName prefab)
	{
		string fileName = FilePath.StripPath(prefab);
		fileName = FilePath.StripExtension(fileName);
		fileName.Replace("_", " ");
		return fileName;
	}
}
