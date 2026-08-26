// Ambush / hold-fire, per group.
modded class SCR_AIGroupUtilityComponent
{
	protected bool	m_bDCO_IsAmbusher		= false;
	protected float	m_fDCO_AmbushRange		= 50.0;
	protected bool	m_bDCO_AmbushSprung		= false;
	protected float	m_fDCO_LastAmbushTime	= -1;
	protected bool	m_bDCO_ManualHold		= false;	// GM right-click hold-fire, independent of the ambush trigger.
	protected float	m_fDCO_LastManualHoldTopUp = -1;

	protected static const float DCO_AMBUSH_CHECK_INTERVAL_MS	= 1000.0;
	protected static const float DCO_AMBUSH_HOLD_NOFIRE_SEC		= 5.0;	// re-applied so the hold never lapses.

	bool DCO_IsAmbusher()
	{
		return m_bDCO_IsAmbusher;
	}

	void DCO_SetAmbusher(bool enable)
	{
		m_bDCO_IsAmbusher = enable;
		if (!enable)
			m_bDCO_AmbushSprung = false;	// disarming clears the sprung latch so it can be re-armed.
	}

	float DCO_GetAmbushRange()
	{
		return m_fDCO_AmbushRange;
	}

	void DCO_SetAmbushRange(float range)
	{
		m_fDCO_AmbushRange = range;
	}

	// Manual hold-fire context action.
	bool DCO_GetManualHold()
	{
		return m_bDCO_ManualHold;
	}

	void DCO_SetManualHold(bool hold)
	{
		if (m_bDCO_ManualHold == hold)
			return;
		m_bDCO_ManualHold = hold;
		int affected = 0;
		vector groupPos = vector.Zero;
		if (m_Owner)
		{
			groupPos = m_Owner.GetOrigin();
			if (hold)
				affected = DCO_SetGroupNoFireTime(DCO_AMBUSH_HOLD_NOFIRE_SEC);
			else
				affected = DCO_SetGroupNoFireTime(0);
		}
		Print(string.Format("[DCO-WPI] manual fire control: hold=%1 group=%2 direct AI affected=%3",
			hold, groupPos, affected), LogLevel.NORMAL);
	}

// Springs a paired ambush when an enemy enters its detached kill zone.
	void DCO_SpringAmbush()
	{
		if (!m_Owner || m_bDCO_AmbushSprung)
			return;
		DCO_SetGroupNoFireTime(0);
		m_bDCO_AmbushSprung = true;
	}

	void DCO_UpdateAmbush()
	{
		if (!Replication.IsServer())
			return;

		if (m_bDCO_ManualHold && m_Owner)
		{
			BaseWorld holdWorld = GetGame().GetWorld();
			if (holdWorld)
			{
				float holdNow = holdWorld.GetWorldTime();
				if (m_fDCO_LastManualHoldTopUp < 0 || (holdNow - m_fDCO_LastManualHoldTopUp) >= 1000.0)
				{
					m_fDCO_LastManualHoldTopUp = holdNow;
					DCO_SetGroupNoFireTime(DCO_AMBUSH_HOLD_NOFIRE_SEC);
				}
			}
		}

		if (!m_bDCO_IsAmbusher || m_bDCO_AmbushSprung || !m_Owner)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		if (m_fDCO_LastAmbushTime >= 0 && (now - m_fDCO_LastAmbushTime) < DCO_AMBUSH_CHECK_INTERVAL_MS)
			return;
		m_fDCO_LastAmbushTime = now;

		bool enemyInRange = DCO_AmbushEnemyInRange();

		if (enemyInRange)
		{
			// Spring the ambush: release the hold so the group can fire, then stop managing it.
			DCO_SetGroupNoFireTime(0);
			m_bDCO_AmbushSprung = true;
		}
		else
		{
			// Still waiting: keep the hold topped up so it never lapses between checks.
			DCO_SetGroupNoFireTime(DCO_AMBUSH_HOLD_NOFIRE_SEC);
		}
	}

	// True if any perceived enemy is within trigger range of the group leader.
	protected bool DCO_AmbushEnemyInRange()
	{
		if (!m_Perception)
			return false;

		array<IEntity> targets = m_Perception.m_aTargetEntities;
		if (!targets || targets.IsEmpty())
			return false;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return false;

		vector leaderPos = leader.GetOrigin();
		float rangeSq = m_fDCO_AmbushRange * m_fDCO_AmbushRange;

		foreach (IEntity t : targets)
		{
			if (t && vector.DistanceSq(t.GetOrigin(), leaderPos) <= rangeSq)
				return true;
		}
		return false;
	}

	protected int DCO_SetGroupNoFireTime(float seconds)
	{
		if (!m_Owner)
			return 0;
		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		int affected = 0;
		foreach (AIAgent agent : agents)
		{
			// GetAgents can include nested group structure in some editor compositions.
			if (!agent || agent.GetParentGroup() != m_Owner)
				continue;
			IEntity ent = agent.GetControlledEntity();
			if (!ent)
				continue;
			if (DCO_PlayerUtil.IsPlayer(ent))
				continue;	// never hold a player's fire.

			SCR_CharacterControllerComponent cc = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
			if (cc)
			{
				cc.SetWeaponNoFireTime(seconds);
				affected++;
			}
		}
		return affected;
	}
}
