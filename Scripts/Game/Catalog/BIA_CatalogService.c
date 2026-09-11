class BIA_ItemEntry
{
	ResourceName m_Prefab;
	string m_sDisplayName;
	SCR_EArsenalItemType m_eType;
	SCR_EArsenalItemMode m_eMode;
	int m_iSupplyCost;
	SCR_ECharacterRank m_eRequiredRank;
	bool m_bUseMilitarySupplyAllocation;
	protected ref array<string> m_aSourceMods;
	protected string m_sSearchText;

	array<string> GetSourceMods()
	{
		if (!m_aSourceMods)
			m_aSourceMods = SCR_AddonTool.GetResourceAddons(m_Prefab);
		return m_aSourceMods;
	}

	string GetSearchText()
	{
		if (m_sSearchText.IsEmpty())
		{
			m_sSearchText = m_sDisplayName + " " + m_Prefab;
			foreach (string addon : GetSourceMods())
				m_sSearchText += " " + addon + " " + BIA_CatalogService.GetModTitle(addon);
			m_sSearchText.ToLower();
		}
		return m_sSearchText;
	}
}

class BIA_CatalogService
{
	protected static ref map<string, string> s_mModTitles;

	static string GetModTitle(string addonId)
	{
		if (!s_mModTitles)
		{
			s_mModTitles = new map<string, string>();
			array<string> guids = {};
			GameProject.GetLoadedAddons(guids);
			foreach (string guid : guids)
				s_mModTitles.Set(GameProject.GetAddonID(guid), GameProject.GetAddonTitle(guid));
		}
		string title;
		if (s_mModTitles.Find(addonId, title) && !title.IsEmpty())
			return title;
		return addonId;
	}

	static void CollectContentsItems(notnull BIA_DraftService service, notnull array<ref BIA_ItemEntry> outItems)
	{
		outItems.Clear();
		map<ResourceName, bool> seen = new map<ResourceName, bool>();
		array<ref BIA_ItemEntry> tabItems = {};
		BIA_CatalogService.CollectTabItems(service.m_Config, BIA_EArmoryTab.GEAR, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType(), tabItems);
		AppendLooseItems(tabItems, outItems, seen);

		tabItems.Clear();
		BIA_CatalogService.CollectTabItems(service.m_Config, BIA_EArmoryTab.WEAPONS, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType(), tabItems);
		AppendLooseItems(tabItems, outItems, seen);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendLooseItems(notnull array<ref BIA_ItemEntry> source, notnull array<ref BIA_ItemEntry> destination, notnull map<ResourceName, bool> seen)
	{
		foreach (BIA_ItemEntry item : source)
		{
			if (!item || seen.Contains(item.m_Prefab) || !BIA_ItemIntel.GetClothAreaType(item.m_Prefab).IsEmpty())
				continue;

			seen.Insert(item.m_Prefab, true);
			destination.Insert(item);
		}
	}

	//------------------------------------------------------------------------------------------------

	// Built on first use - eager static initializers charge the module-init budget shared by every loaded mod.
	protected static ref map<string, ref array<ref BIA_ItemEntry>> s_mCategoryCache;
	protected static ref map<ResourceName, string> s_mDisplayNameCache;
	protected static ref array<ref BIA_ArmoryCategory> s_aAutoCategories;

	protected static void EnsureCaches()
	{
		if (s_mCategoryCache)
			return;

		s_mCategoryCache = new map<string, ref array<ref BIA_ItemEntry>>();
		s_mDisplayNameCache = new map<ResourceName, string>();
		s_aAutoCategories = new array<ref BIA_ArmoryCategory>();
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

		BIA_ArsenalScenarioSettings settings = BIA_ArsenalScenarioSettings.Get();
		BIA_ECatalogScope scope = settings.m_eCatalogScope;
		BIA_ArmoryConfig config = BIA_ConfigHolder.GetDefault();
		if (scope == BIA_ECatalogScope.ALL_FACTIONS && config && config.m_bRestrictToArsenalConfig)
			scope = BIA_ECatalogScope.STATION_ARSENAL;

		if (scope == BIA_ECatalogScope.STATION_ARSENAL && arsenal)
		{
			arsenal.GetFilteredArsenalItems(outItems);
			return;
		}

		if (scope == BIA_ECatalogScope.STATION_ARSENAL)
			scope = BIA_ECatalogScope.PLAYER_FACTION;

		SCR_EArsenalGameModeType gameModeType = SCR_ArsenalManagerComponent.GetArsenalGameModeType_Static();
		if (scope == BIA_ECatalogScope.PLAYER_FACTION)
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
	static void BuildCharacterSlotCategories(notnull array<BIA_ArmoryCategory> categories, GameEntity character, SCR_ArsenalComponent arsenal, SCR_Faction faction)
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

		array<BIA_ArmoryCategory> ordered = {};
		array<string> coveredAreas = {};
		foreach (BIA_ArmoryCategory category : categories)
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
			if (!arsenalItem || !(arsenalItem.GetItemMode() & SCR_EArsenalItemMode.DEFAULT) || !BIA_ArsenalScenarioSettings.Get().AllowsArsenalItem(arsenalItem))
				continue;

			ResourceName itemPrefab = arsenalItem.GetItemResourceName();
			if (itemPrefab.IsEmpty())
				continue;

			string itemArea = BIA_ItemIntel.GetClothAreaType(itemPrefab);
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

				BIA_ArmoryCategory autoCategory = new BIA_ArmoryCategory();
				autoCategory.m_sDisplayName = PrettyAreaName(area);
				autoCategory.m_eItemTypes = -1;
				autoCategory.m_eItemModes = SCR_EArsenalItemMode.DEFAULT;
				autoCategory.m_eTab = BIA_EArmoryTab.OUTFIT;
				autoCategory.m_iWeaponSlot = -1;
				autoCategory.m_sClothArea = area;
				s_aAutoCategories.Insert(autoCategory);
			}
		}

		foreach (BIA_ArmoryCategory autoCategory : s_aAutoCategories)
		{
			if (coveredAreas.Find(autoCategory.m_sClothArea) < 0 && characterAreas.Find(autoCategory.m_sClothArea) >= 0)
				ordered.Insert(autoCategory);
		}

		categories.Clear();
		foreach (BIA_ArmoryCategory category : ordered)
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
	static array<ref BIA_ItemEntry> GetItems(notnull BIA_ArmoryCategory category, SCR_Faction faction, SCR_ArsenalComponent arsenal, SCR_EArsenalSupplyCostType costType)
	{
		string factionKey = "none";
		if (faction)
			factionKey = faction.GetFactionKey();

		EnsureCaches();
		string cacheKey = string.Format("%1|%2|%3|%4", factionKey, category.m_sDisplayName, category.m_eTab, BIA_ArsenalScenarioSettings.Get().Pack());
		array<ref BIA_ItemEntry> cached = s_mCategoryCache.Get(cacheKey);
		if (cached)
			return cached;

		array<ref BIA_ItemEntry> result = {};
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
			if (!BIA_ArsenalScenarioSettings.Get().AllowsArsenalItem(arsenalItem))
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
				if (BIA_ItemIntel.GetClothAreaType(prefab) != category.m_sClothArea)
					continue;
			}
			else if (category.m_eTab == BIA_EArmoryTab.GEAR && !BIA_ItemIntel.GetClothAreaType(prefab).IsEmpty() && !(arsenalItem.GetItemType() & SCR_EArsenalItemType.HEAL) && !(arsenalItem.GetItemMode() & SCR_EArsenalItemMode.CONSUMABLE))
			{
				continue;
			}

			BIA_ItemEntry entry = new BIA_ItemEntry();
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
	static void CollectTabItems(BIA_ArmoryConfig config, BIA_EArmoryTab tab, SCR_Faction faction, SCR_ArsenalComponent arsenal, SCR_EArsenalSupplyCostType costType, notnull out array<ref BIA_ItemEntry> outItems)
	{
		outItems.Clear();
		if (!config)
			return;

		map<ResourceName, bool> seen = new map<ResourceName, bool>();
		array<BIA_ArmoryCategory> categories = {};
		config.GetCategoriesForTab(tab, categories);

		foreach (BIA_ArmoryCategory category : categories)
		{
			array<ref BIA_ItemEntry> items = GetItems(category, faction, arsenal, costType);
			if (!items)
				continue;

			foreach (BIA_ItemEntry item : items)
			{
				if (!item || seen.Contains(item.m_Prefab))
					continue;

				seen.Insert(item.m_Prefab, true);
				outItems.Insert(item);
			}
		}
	}

	// Clothing mounts use catalog entries that character-slot and cargo tabs intentionally omit.
	static void CollectClothingMountItems(SCR_Faction faction, SCR_ArsenalComponent arsenal, SCR_EArsenalSupplyCostType costType, notnull out array<ref BIA_ItemEntry> outItems)
	{
		outItems.Clear();
		array<SCR_ArsenalItem> items = {};
		CollectSourceItems(items, faction, arsenal, -1, SCR_EArsenalItemMode.DEFAULT | SCR_EArsenalItemMode.ATTACHMENT);
		foreach (SCR_ArsenalItem item : items)
		{
			if (!item || !BIA_ArsenalScenarioSettings.Get().AllowsArsenalItem(item))
				continue;
			ResourceName prefab = item.GetItemResourceName();
			if (prefab.IsEmpty() || BIA_ItemIntel.GetClothAreaType(prefab).IsEmpty())
				continue;
			BIA_ItemEntry entry = new BIA_ItemEntry();
			entry.m_Prefab = prefab;
			entry.m_sDisplayName = GetDisplayName(prefab, faction);
			entry.m_eType = item.GetItemType();
			entry.m_eMode = item.GetItemMode();
			entry.m_iSupplyCost = item.GetSupplyCost(costType);
			entry.m_eRequiredRank = item.GetRequiredRank();
			entry.m_bUseMilitarySupplyAllocation = item.GetUseMilitarySupplyAllocation();
			outItems.Insert(entry);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Weapon-slot categories of the WEAPONS tab — the classes a receiver browser pages through.
	static void GetWeaponSlotCategories(BIA_ArmoryConfig config, notnull out array<BIA_ArmoryCategory> categories)
	{
		categories.Clear();
		if (!config)
			return;

		array<BIA_ArmoryCategory> tabCategories = {};
		config.GetCategoriesForTab(BIA_EArmoryTab.WEAPONS, tabCategories);
		foreach (BIA_ArmoryCategory category : tabCategories)
		{
			if (category && category.m_iWeaponSlot >= 0)
				categories.Insert(category);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Entry for a prefab across the given categories. Entries are owned by the session cache, so
	//! the returned reference stays valid until the cache is cleared on the next menu open.
	static BIA_ItemEntry FindEntry(ResourceName prefab, notnull array<BIA_ArmoryCategory> categories, SCR_Faction faction, SCR_ArsenalComponent arsenal, SCR_EArsenalSupplyCostType costType)
	{
		if (prefab.IsEmpty())
			return null;

		foreach (BIA_ArmoryCategory category : categories)
		{
			if (!category)
				continue;

			array<ref BIA_ItemEntry> items = GetItems(category, faction, arsenal, costType);
			if (!items)
				continue;

			foreach (BIA_ItemEntry item : items)
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
