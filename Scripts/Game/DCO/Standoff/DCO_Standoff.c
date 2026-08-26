// Standoff: minimum engagement distance, no suicide-charge.
modded class SCR_AIFindFirePositionBehavior
{
	protected static const float DCO_STANDOFF_CQB_WINDOW_MS = 12000.0;	// yield the floor for this long after a CQB assault order.
	protected static const float DCO_STANDOFF_POINTBLANK    = 15.0;	// metres: at/under this the floor yields at point-blank.

	override void InitParameters(vector targetPos, float minDistance, float maxDistance, float duration)
	{
		float newMin = minDistance;
		float newMax = maxDistance;

		DCO_TacticalMoveSettings cfg = DCO_TacticalMoveSettings.Get();
		if (cfg && cfg.m_bEnableStandoff && Replication.IsServer() && !DCO_StandoffYields() && !DCO_StandoffOwnerMounted()
			&& !DCO_StandoffPointBlank(targetPos) && !DCO_StandoffIndoors())
		{
			float floor = DCO_StandoffFloor(cfg);
			// Anti-ping-pong: never push an already-engaged unit BACKWARD to the floor.
			float curDist = DCO_StandoffCurrentDist(targetPos);
			if (curDist > 0 && floor > curDist)
				floor = curDist;
			if (newMin < floor)
				newMin = floor;
			// Keep a valid engagement band so the AI don't sit uselessly beyond their weapon's reach.
			if (newMax < newMin + cfg.m_fStandoffBand)
				newMax = newMin + cfg.m_fStandoffBand;
		}

		super.InitParameters(targetPos, newMin, newMax, duration);
	}

	protected bool DCO_StandoffPointBlank(vector targetPos)
	{
		if (!m_Utility || !m_Utility.m_OwnerEntity)
			return false;
		return vector.DistanceSq(m_Utility.m_OwnerEntity.GetOrigin(), targetPos) < (DCO_STANDOFF_POINTBLANK * DCO_STANDOFF_POINTBLANK);
	}

	// Current ground-range from this agent to the target it is finding a fire position around.
	protected float DCO_StandoffCurrentDist(vector targetPos)
	{
		if (!m_Utility || !m_Utility.m_OwnerEntity)
			return -1;
		return vector.Distance(m_Utility.m_OwnerEntity.GetOrigin(), targetPos);
	}

	// Weapon-aware minimum engagement distance for this agent.
	protected float DCO_StandoffFloor(DCO_TacticalMoveSettings cfg)
	{
		if (!m_Utility || !m_Utility.m_CombatComponent)
			return cfg.m_fStandoffRifle;

		switch (m_Utility.m_CombatComponent.GetCurrentWeaponType())
		{
			case EWeaponType.WT_MACHINEGUN:		return cfg.m_fStandoffMG;
			case EWeaponType.WT_SNIPERRIFLE:	return cfg.m_fStandoffSniper;
			case EWeaponType.WT_ROCKETLAUNCHER:	return cfg.m_fStandoffLauncher;
			case EWeaponType.WT_HANDGUN:		return cfg.m_fStandoffPistol;
		}
		return cfg.m_fStandoffRifle;	// rifle / grenade-launcher / default.
	}

	// True if this agent is crewing a vehicle.
	protected bool DCO_StandoffOwnerMounted()
	{
		if (!m_Utility || !m_Utility.m_OwnerEntity)
			return false;
		return CompartmentAccessComponent.GetVehicleIn(m_Utility.m_OwnerEntity) != null;
	}

	// True under a roof.
	protected bool DCO_StandoffIndoors()
	{
		if (!m_Utility || !m_Utility.m_OwnerEntity)
			return false;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;
		vector origin = m_Utility.m_OwnerEntity.GetOrigin();
		TraceParam probe = new TraceParam();
		probe.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		probe.Exclude = m_Utility.m_OwnerEntity;
		probe.Start = origin + Vector(0, 2.0, 0);
		probe.End = origin + Vector(0, 5.0, 0);
		return world.TraceMove(probe, null) < 0.999;
	}

	protected bool DCO_StandoffYields()
	{
		SCR_AIGroupUtilityComponent gu = DCO_StandoffGroupUtil();
		if (!gu)
			return false;

		if (DCO_CqbClearUtil.IsClearingActive(gu))
			return true;

		if (gu.DCO_GetCOA() == EDCO_COA.ASSAULT_FLANK)
			return true;

		float cqbOrderTime = gu.DCO_GetLastCqbOrderTime();
		if (cqbOrderTime >= 0)
		{
			BaseWorld world = GetGame().GetWorld();
			if (world && (world.GetWorldTime() - cqbOrderTime) < DCO_STANDOFF_CQB_WINDOW_MS)
				return true;
		}
		return false;
	}

	protected SCR_AIGroupUtilityComponent DCO_StandoffGroupUtil()
	{
		if (!m_Utility || !m_Utility.m_OwnerEntity)
			return null;

		AIControlComponent aic = AIControlComponent.Cast(m_Utility.m_OwnerEntity.FindComponent(AIControlComponent));
		if (!aic)
			return null;

		AIAgent agent = aic.GetControlAIAgent();
		if (!agent)
			return null;

		SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
		if (!grp)
			return null;

		return grp.GetGroupUtilityComponent();
	}
}
