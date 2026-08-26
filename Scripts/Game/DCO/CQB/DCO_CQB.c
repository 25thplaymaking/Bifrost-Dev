// CQB / building assault.
modded class SCR_AIGroupUtilityComponent
{
	protected float			m_fDCO_LastCqbTime		= -1;
	protected vector		m_vDCO_LastCqbOrder;
	protected bool			m_bDCO_HasCqbOrder		= false;
	protected float			m_fDCO_CqbOrderTime		= -1;
	protected ref array<IEntity>	m_aDCO_CqbBuildings;	// scratch for the building query.

	// World time of the most recent building-assault order, or -1.
	float DCO_GetLastCqbOrderTime()
	{
		return m_fDCO_CqbOrderTime;
	}

	void DCO_UpdateCQB()
	{
		if (!Replication.IsServer())
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		if (!cfg.m_bEnableCQB || !m_Owner || !m_Perception)
			return;

		if (DCO_CqbClearUtil.IsClearingActive(this))
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_LastCqbTime >= 0 && (now - m_fDCO_LastCqbTime) < cfg.m_fCqbCheckSec * 1000.0)
			return;
		m_fDCO_LastCqbTime = now;

		// Below the throttle: IsGroupInVehicle is a per-member FindComponent scan.
		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
			return;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return;
		vector lead = leader.GetOrigin();

		// Nearest perceived enemy.
		array<IEntity> targets = m_Perception.m_aTargetEntities;
		if (!targets || targets.IsEmpty())
		{
			// Contact over: release the re-order latch.
			m_bDCO_HasCqbOrder = false;
			return;
		}
		vector enemyPos;
		bool hasEnemy = false;
		float bestSq = 1000000000.0;
		foreach (IEntity t : targets)
		{
			if (!t)
				continue;
			float d = vector.DistanceSq(t.GetOrigin(), lead);
			if (d < bestSq)
			{
				bestSq = d;
				enemyPos = t.GetOrigin();
				hasEnemy = true;
			}
		}
		if (!hasEnemy)
			return;

		// Only assault when reasonably close; long-range contacts aren't a building assault.
		if (vector.DistanceSq(enemyPos, lead) > cfg.m_fCqbEngageRange * cfg.m_fCqbEngageRange)
			return;

		// Is the enemy co-located with a building?
		IEntity building = DCO_CqbFindBuildingNear(world, enemyPos, cfg.m_fCqbBuildingScan);
		if (!building)
			return;

		// Already near the enemy?
		if (vector.DistanceSq(enemyPos, lead) < cfg.m_fCqbArriveDist * cfg.m_fCqbArriveDist)
			return;

		// Re-order only as the enemy moves meaningfully.
		if (m_bDCO_HasCqbOrder && vector.DistanceSq(enemyPos, m_vDCO_LastCqbOrder) < cfg.m_fCqbReorderDist * cfg.m_fCqbReorderDist)
			return;

		m_vDCO_LastCqbOrder = enemyPos;
		m_bDCO_HasCqbOrder = true;
		m_fDCO_CqbOrderTime = now;	// stamp so the standoff layer yields while we close.
		// Push to the enemy; the engine paths the group in through the building's entry.
		DCO_VehicleUtil.OrderGroupMoveToPosition(m_Owner, enemyPos, m_Mailbox);

		if (cfg.m_bDebug)
			DCO_Debug.LogGroup("CQB", leader, string.Format("assaulting building at %1 (enemy inside)", enemyPos));
	}

	// Real enterable building near pos, or null.
	protected static const float DCO_CQB_MIN_BUILDING_HEIGHT_M = 2.2;
	protected static const float DCO_CQB_MIN_BUILDING_SPAN_M = 3.0;
	protected static const float DCO_CQB_MIN_BUILDING_VOLUME_M3 = 25.0;

	protected IEntity DCO_CqbFindBuildingNear(BaseWorld world, vector pos, float radius)
	{
		m_aDCO_CqbBuildings = {};
		world.QueryEntitiesBySphere(pos, radius, DCO_CqbCollect);

		IEntity bestContain;
		float bestContainVol = 0;
		IEntity bestNear;
		float bestNearSq = radius * radius + 1;
		foreach (IEntity b : m_aDCO_CqbBuildings)
		{
			if (!b)
				continue;
			vector mins;
			vector maxs;
			b.GetBounds(mins, maxs);
			float sizeX = Math.AbsFloat(maxs[0] - mins[0]);
			float sizeY = Math.AbsFloat(maxs[1] - mins[1]);
			float sizeZ = Math.AbsFloat(maxs[2] - mins[2]);
			float volume = sizeX * sizeY * sizeZ;
			if (sizeY < DCO_CQB_MIN_BUILDING_HEIGHT_M)
				continue;
			if (Math.Max(sizeX, sizeZ) < DCO_CQB_MIN_BUILDING_SPAN_M)
				continue;
			if (volume < DCO_CQB_MIN_BUILDING_VOLUME_M3)
				continue;

			vector local = b.CoordToLocal(pos);
			bool contains = local[0] >= mins[0] - 0.5 && local[0] <= maxs[0] + 0.5
				&& local[2] >= mins[2] - 0.5 && local[2] <= maxs[2] + 0.5
				&& local[1] >= mins[1] - 2.0 && local[1] <= maxs[1] + 2.0;
			if (contains && volume > bestContainVol)
			{
				bestContain = b;
				bestContainVol = volume;
			}
			float d = vector.DistanceSq(b.GetOrigin(), pos);
			if (d < bestNearSq)
			{
				bestNearSq = d;
				bestNear = b;
			}
		}
		if (bestContain)
			return bestContain;
		return bestNear;
	}

	// QueryEntitiesBySphere callback: collect building entities.
	protected bool DCO_CqbCollect(IEntity e)
	{
		if (!e)
			return true;
		if (Building.Cast(e) || e.FindComponent(SCR_DestructibleBuildingComponent))
			m_aDCO_CqbBuildings.Insert(e);
		return true;
	}
}
