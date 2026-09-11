enum BIA_ECatalogScope
{
	ALL_FACTIONS = 0,
	PLAYER_FACTION = 1,
	STATION_ARSENAL = 2
}

//! Server-owned Arsenal policy mirrored to each client through its player controller.
class BIA_ArsenalScenarioSettings
{
	protected static const int SCOPE_MASK = 3;
	protected static const int ALLOW_WEAPONS = 4;
	protected static const int ALLOW_WEARABLES = 8;
	protected static const int ALLOW_FIELD_GEAR = 16;
	protected static const int ALLOW_KIT_CHANGES = 32;
	protected static const int USE_RANK_LOCKS = 64;
	protected static const int USE_SUPPLIES = 128;
	protected static const int RESTRICT_KIT_FACTION = 256;

	protected static ref BIA_ArsenalScenarioSettings s_Instance;

	BIA_ECatalogScope m_eCatalogScope = BIA_ECatalogScope.ALL_FACTIONS;
	bool m_bAllowWeapons = true;
	bool m_bAllowWearables = true;
	bool m_bAllowFieldGear = true;
	bool m_bAllowKitChanges = true;
	bool m_bUseRankLocks;
	bool m_bUseSupplies;
	bool m_bRestrictKitFaction;

	//------------------------------------------------------------------------------------------------
	static BIA_ArsenalScenarioSettings Get()
	{
		if (!s_Instance)
			s_Instance = new BIA_ArsenalScenarioSettings();
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	int Pack()
	{
		int packed = m_eCatalogScope & SCOPE_MASK;
		if (m_bAllowWeapons)
			packed |= ALLOW_WEAPONS;
		if (m_bAllowWearables)
			packed |= ALLOW_WEARABLES;
		if (m_bAllowFieldGear)
			packed |= ALLOW_FIELD_GEAR;
		if (m_bAllowKitChanges)
			packed |= ALLOW_KIT_CHANGES;
		if (m_bUseRankLocks)
			packed |= USE_RANK_LOCKS;
		if (m_bUseSupplies)
			packed |= USE_SUPPLIES;
		if (m_bRestrictKitFaction)
			packed |= RESTRICT_KIT_FACTION;
		return packed;
	}

	//------------------------------------------------------------------------------------------------
	void ApplyPacked(int packed)
	{
		int scope = packed & SCOPE_MASK;
		if (scope < BIA_ECatalogScope.ALL_FACTIONS || scope > BIA_ECatalogScope.STATION_ARSENAL)
			scope = BIA_ECatalogScope.ALL_FACTIONS;

		m_eCatalogScope = scope;
		m_bAllowWeapons = (packed & ALLOW_WEAPONS) != 0;
		m_bAllowWearables = (packed & ALLOW_WEARABLES) != 0;
		m_bAllowFieldGear = (packed & ALLOW_FIELD_GEAR) != 0;
		m_bAllowKitChanges = (packed & ALLOW_KIT_CHANGES) != 0;
		m_bUseRankLocks = (packed & USE_RANK_LOCKS) != 0;
		m_bUseSupplies = (packed & USE_SUPPLIES) != 0;
		m_bRestrictKitFaction = (packed & RESTRICT_KIT_FACTION) != 0;
	}

	//------------------------------------------------------------------------------------------------
	bool AllowsAllItemSets()
	{
		return m_bAllowWeapons && m_bAllowWearables && m_bAllowFieldGear;
	}

	//------------------------------------------------------------------------------------------------
	bool AllowsArsenalItem(SCR_ArsenalItem item)
	{
		if (!item)
			return false;

		EDCO_ArsenalCategory category = DCO_ArsenalCatalog.CategoryOf(item.GetItemType(), item.GetItemMode());
		switch (category)
		{
			case EDCO_ArsenalCategory.PRIMARY:
			case EDCO_ArsenalCategory.PISTOL:
			case EDCO_ArsenalCategory.LAUNCHER:
			case EDCO_ArsenalCategory.MAGAZINES:
			case EDCO_ArsenalCategory.ATTACHMENTS:
				return m_bAllowWeapons;

			case EDCO_ArsenalCategory.UNIFORM:
			case EDCO_ArsenalCategory.VEST:
			case EDCO_ArsenalCategory.BACKPACK:
			case EDCO_ArsenalCategory.HEADGEAR:
				return m_bAllowWearables;
		}

		return m_bAllowFieldGear;
	}

	//------------------------------------------------------------------------------------------------
	//! Pushes the current authority value to every connected controller after a GM change.
	void Broadcast()
	{
		if (!Replication.IsServer())
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		int packed = Pack();
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			SCR_PlayerController controller = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
			if (controller)
				controller.BIA_SetArsenalScenarioPolicy(packed);
		}
	}
}

modded class SCR_PlayerController
{
	[RplProp(onRplName: "BIA_OnArsenalScenarioPolicyChanged")]
	protected int m_iBIAArsenalScenarioPolicy;

	//------------------------------------------------------------------------------------------------
	//! Initial authority value is included in the controller's first replicated state for JIP.
	void BIA_InitializeArsenalScenarioPolicy(int packed)
	{
		if (Replication.IsServer())
			m_iBIAArsenalScenarioPolicy = packed;
	}

	//------------------------------------------------------------------------------------------------
	void BIA_SetArsenalScenarioPolicy(int packed)
	{
		if (!Replication.IsServer() || m_iBIAArsenalScenarioPolicy == packed)
			return;

		m_iBIAArsenalScenarioPolicy = packed;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void BIA_OnArsenalScenarioPolicyChanged()
	{
		BIA_ArsenalScenarioSettings.Get().ApplyPacked(m_iBIAArsenalScenarioPolicy);
		BIA_CatalogService.ClearSessionCache();
	}
}
