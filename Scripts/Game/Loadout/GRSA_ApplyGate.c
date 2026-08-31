class GRSA_ArmoryCodes
{
	static const string OK = "";
	static const string ITEM_NOT_IN_ARSENAL = "A01";
	static const string ITEM_RANK_LOCKED = "K01";
	static const string ITEM_UNRESOLVABLE = "X02";
}

//! Per-item apply gate built server-side from the replicated Scenario Settings policy.
//! Items failing the gate are SKIPPED and reported, never used to reject the whole kit.
class GRSA_ApplyGate
{
	protected bool m_bRestrictToArsenal;
	protected bool m_bRestrictToPlayerCatalog;
	protected bool m_bRestrictItemSets;
	protected bool m_bRankLocked;
	protected int m_iPlayerRank;
	protected SCR_Faction m_Faction;
	protected ref set<ResourceName> m_AllowedPrefabs = new set<ResourceName>();
	protected ref map<ResourceName, string> m_VerdictCache = new map<ResourceName, string>();

	//------------------------------------------------------------------------------------------------
	static GRSA_ApplyGate Build(SCR_ArsenalComponent arsenal, notnull GameEntity character, notnull GRSA_ArmoryConfig config)
	{
		GRSA_ApplyGate gate = new GRSA_ApplyGate();

		SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(character);
		if (chimeraCharacter)
			gate.m_Faction = SCR_Faction.Cast(chimeraCharacter.GetFaction());

		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		GRSA_ECatalogScope scope = settings.m_eCatalogScope;
		if (scope == GRSA_ECatalogScope.ALL_FACTIONS && config.m_bRestrictToArsenalConfig)
			scope = GRSA_ECatalogScope.STATION_ARSENAL;

		if (scope == GRSA_ECatalogScope.STATION_ARSENAL && arsenal)
		{
			array<SCR_ArsenalItem> available = {};
			if (arsenal.GetFilteredArsenalItems(available) && !available.IsEmpty())
			{
				gate.m_bRestrictToArsenal = true;
				foreach (SCR_ArsenalItem item : available)
				{
					if (item && !item.GetItemResourceName().IsEmpty())
						gate.m_AllowedPrefabs.Insert(item.GetItemResourceName());
				}
			}
			else
			{
				gate.m_bRestrictToPlayerCatalog = true;
				GRSA_Log.Warn("Apply gate: station inventory unavailable, using the player's faction catalog");
			}
		}
		else if (scope == GRSA_ECatalogScope.PLAYER_FACTION || scope == GRSA_ECatalogScope.STATION_ARSENAL)
			gate.m_bRestrictToPlayerCatalog = true;

		gate.m_bRestrictItemSets = !settings.AllowsAllItemSets();

		SCR_ArsenalManagerComponent arsenalManager;
		if ((settings.m_bUseRankLocks || config.m_bUseRankLocks) && SCR_ArsenalManagerComponent.GetArsenalManager(arsenalManager) && arsenalManager.AreItemsRankLocked())
		{
			gate.m_bRankLocked = true;
			gate.m_iPlayerRank = SCR_CharacterRankComponent.GetCharacterRank(character);
		}

		return gate;
	}

	//------------------------------------------------------------------------------------------------
	bool IsRestricting()
	{
		return m_bRestrictToArsenal || m_bRestrictToPlayerCatalog || m_bRestrictItemSets || m_bRankLocked;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns GRSA_ArmoryCodes.OK to allow, else the skip-reason code.
	string Verdict(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return GRSA_ArmoryCodes.ITEM_UNRESOLVABLE;

		string cached;
		if (m_VerdictCache.Find(prefab, cached))
			return cached;

		string verdict = ComputeVerdict(prefab);
		m_VerdictCache.Insert(prefab, verdict);
		return verdict;
	}

	//------------------------------------------------------------------------------------------------
	protected string ComputeVerdict(ResourceName prefab)
	{
		if (m_bRestrictToArsenal && !m_AllowedPrefabs.Contains(prefab))
			return GRSA_ArmoryCodes.ITEM_NOT_IN_ARSENAL;

		if (m_bRestrictToPlayerCatalog && !ExistsInPlayerCatalog(prefab))
			return GRSA_ArmoryCodes.ITEM_NOT_IN_ARSENAL;

		SCR_ArsenalItem itemData;
		if (m_bRestrictItemSets || m_bRankLocked)
			itemData = GRSA_CatalogService.FindArsenalItemData(prefab, m_Faction);

		if (m_bRestrictItemSets && !GRSA_ArsenalScenarioSettings.Get().AllowsArsenalItem(itemData))
			return GRSA_ArmoryCodes.ITEM_NOT_IN_ARSENAL;

		if (m_bRankLocked)
		{
			if (itemData && itemData.GetRequiredRank() > m_iPlayerRank)
				return GRSA_ArmoryCodes.ITEM_RANK_LOCKED;
		}

		return GRSA_ArmoryCodes.OK;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ExistsInPlayerCatalog(ResourceName prefab)
	{
		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
			return true;

		if (m_Faction)
			return catalogManager.GetEntryWithPrefabFromGeneralOrFactionCatalog(EEntityCatalogType.ITEM, prefab, m_Faction) != null;

		return catalogManager.GetEntryWithPrefabFromCatalog(EEntityCatalogType.ITEM, prefab) != null;
	}

	//------------------------------------------------------------------------------------------------
	bool Allows(ResourceName prefab)
	{
		return Verdict(prefab) == GRSA_ArmoryCodes.OK;
	}
}
