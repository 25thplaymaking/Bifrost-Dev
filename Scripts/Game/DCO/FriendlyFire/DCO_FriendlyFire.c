// Fratricide avoidance / friendly-fire LOS hold + reposition.
modded class SCR_AIGroupUtilityComponent
{
	protected static const float DCO_FF_REPOSITION_DIST	= 4.0;	// metres to sidestep when a friendly blocks the lane.
	protected static const float DCO_FF_REORDER_SEC		= 3.0;	// per-member throttle between reposition orders.

	protected float				m_fDCO_LastFFTime	= -1;
	protected ref array<IEntity>	m_aDCO_FFLineHits;
	protected IEntity			m_eDCO_FFShooter;
	protected Faction			m_DCO_FFSelfFaction;
	protected ref map<IEntity, float>	m_mDCO_FFLastReposition;

	void DCO_UpdateFriendlyFire()
	{
		if (!Replication.IsServer())
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		if (!cfg.m_bEnableFriendlyFire || !m_Owner || !m_Perception)
			return;
		// The CQB state machine already serializes point/cover lanes.
		if (DCO_CqbIsClearingActive())
			return;

		array<IEntity> targets = m_Perception.m_aTargetEntities;
		if (!targets || targets.IsEmpty())
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		if (m_fDCO_LastFFTime >= 0 && (now - m_fDCO_LastFFTime) < cfg.m_fFriendlyLaneCheckSec * 1000.0)
			return;
		m_fDCO_LastFFTime = now;

		m_DCO_FFSelfFaction = m_Owner.GetFaction();

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent || agent.GetParentGroup() != m_Owner)
				continue;
			IEntity ent = agent.GetControlledEntity();
			if (!ent)
				continue;
			if (DCO_PlayerUtil.IsPlayer(ent))
				continue;	// never touch a player's fire/movement.

			vector shooterPos = ent.GetOrigin();
			IEntity nearestEnemy;
			float nearestSq = 1000000000.0;	// ~31 km^2 cap; perceived targets are well within this.
			foreach (IEntity t : targets)
			{
				if (!t)
					continue;
				float dSq = vector.DistanceSq(t.GetOrigin(), shooterPos);
				if (dSq < nearestSq)
				{
					nearestSq = dSq;
					nearestEnemy = t;
				}
			}
			if (!nearestEnemy)
				continue;

			vector enemyPos = nearestEnemy.GetOrigin();
			if (!DCO_FriendlyInLane(world, shooterPos, enemyPos, ent))
				continue;	// barrel line to the enemy is clear - fire freely.

// Prevents friendly fire by holding fire whenever an ally crosses the barrel line.
			SCR_CharacterControllerComponent cc = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
			if (cc)
				cc.SetWeaponNoFireTime(cfg.m_fFriendlyFireHold);

			DCO_FFTryReposition(world, agent, ent, shooterPos, enemyPos, now);
		}
	}

	// Sidestep perpendicular to the firing line to clear the lane.
	protected void DCO_FFTryReposition(BaseWorld world, AIAgent agent, IEntity ent, vector shooterPos, vector enemyPos, float now)
	{
		if (!m_Mailbox)
			return;

		if (!m_mDCO_FFLastReposition)
			m_mDCO_FFLastReposition = new map<IEntity, float>();
		// Bounded, not pruned per-entity: keys are raw IEntity and dead members' entries would otherwise accumulate for the group's lifetime.
		if (m_mDCO_FFLastReposition.Count() > 64)
			m_mDCO_FFLastReposition.Clear();
		float last;
		if (m_mDCO_FFLastReposition.Find(ent, last) && (now - last) < DCO_FF_REORDER_SEC * 1000.0)
			return;	// recently sidestepped - let it settle.

		// Ground-plane perpendicular to the shooter->enemy direction.
		vector toEnemy = enemyPos - shooterPos;
		toEnemy[1] = 0;
		float len = toEnemy.Length();
		if (len < 1.0)
			return;	// basically on top of the enemy - nowhere useful to sidestep.
		vector dir = toEnemy / len;
		vector perp = Vector(-dir[2], 0, dir[0]);

		vector left  = shooterPos + perp * DCO_FF_REPOSITION_DIST;
		vector right = shooterPos - perp * DCO_FF_REPOSITION_DIST;

		vector dest;
		bool haveDest = false;
		if (!DCO_FriendlyInLane(world, left, enemyPos, ent))
		{
			dest = left;
			haveDest = true;
		}
		else if (!DCO_FriendlyInLane(world, right, enemyPos, ent))
		{
			dest = right;
			haveDest = true;
		}
		if (!haveDest)
			return;	// both sides still blocked - just hold fire this tick.

		SCR_AIMessage_Move msg = SCR_AIMessage_Move.Create(null, dest, EMovementType.WALK, false, null);
		if (msg)
		{
			msg.SetReceiver(agent);
			m_Mailbox.RequestBroadcast(msg, agent);
			m_mDCO_FFLastReposition.Set(ent, now);
		}
	}

	// True if a same-faction character sits on the firing lane from->to, excluding the shooter.
	protected bool DCO_FriendlyInLane(BaseWorld world, vector from, vector to, IEntity shooter)
	{
		if (!m_aDCO_FFLineHits)
			m_aDCO_FFLineHits = {};
		m_aDCO_FFLineHits.Clear();
		m_eDCO_FFShooter = shooter;

		vector eye = Vector(0, 1.4, 0);	// rough muzzle/eye height.
		world.QueryEntitiesByLine(from + eye, to + eye, DCO_FFLineCollect);

		foreach (IEntity e : m_aDCO_FFLineHits)
		{
			if (!e || e == shooter)
				continue;

			// Only a LIVING CHARACTER - or a same-faction vehicle with CREW ABOARD - blocks the lane.
			ChimeraCharacter ch = ChimeraCharacter.Cast(e);
			if (!ch)
			{
				SCR_BaseCompartmentManagerComponent crew = SCR_BaseCompartmentManagerComponent.Cast(e.FindComponent(SCR_BaseCompartmentManagerComponent));
				if (!crew || crew.GetOccupantCount() <= 0)
					continue;
				FactionAffiliationComponent vfac = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
				if (vfac && vfac.GetAffiliatedFaction() == m_DCO_FFSelfFaction)
					return true;	// friendlies mounted in the lane.
				continue;
			}
			CharacterControllerComponent cc = ch.GetCharacterController();
			if (cc && cc.IsDead())
				continue;

			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
			if (fac && fac.GetAffiliatedFaction() == m_DCO_FFSelfFaction)
				return true;	// a living friendly is in the lane.
		}
		return false;
	}

	// QueryEntitiesByLine callback: collect entities on the lane, excluding the shooter.
	protected bool DCO_FFLineCollect(IEntity e)
	{
		if (e && e != m_eDCO_FFShooter)
			m_aDCO_FFLineHits.Insert(e);
		return true;
	}
}
