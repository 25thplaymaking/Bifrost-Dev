// Group morale on the AI group brain.

modded class SCR_AIGroupUtilityComponent
{
	protected float	m_fDCO_Morale			= 100.0;
	protected int	m_iDCO_MaxStrength		= 0;
	protected float	m_fDCO_LastUpdateTime	= -1;
	protected bool	m_bDCO_Broken			= false;

	protected float				m_fDCO_LastContagionCheck	= -1;

	protected vector	m_vDCO_LastThreatPos;
	protected bool		m_bDCO_HasLastThreat	= false;

	// Panic + morale-to-accuracy runtime state.
	protected bool	m_bDCO_Panicking			= false;
	protected float	m_fDCO_LastPanicTime		= -1;
	protected int	m_iDCO_LastAccuracyBand		= -1;	// -1 unset, 0 default skill, 1 regular, 2 rookie.
	protected float	m_fDCO_LastArmorAvoidTime	= -1;	// hide-from-armour throttle.

	// Living member count last tick, for the per-casualty morale hit.
	protected int	m_iDCO_LastStrength			= -1;

	protected IEntity	m_DCO_LastLeader;
	protected bool		m_bDCO_HadLeader		= false;
	protected float		m_fDCO_LastCasualtyMs	= -1;

	protected static const float DCO_MORALE_MAX				= 100.0;


	protected void DCO_UpdateMorale()
	{
		if (!Replication.IsServer())
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		if (!cfg.m_bEnabled || !m_Owner)
			return;	// m_Owner can be null during group teardown/init - the leader read below would VME.

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		if (m_fDCO_LastUpdateTime >= 0 && (now - m_fDCO_LastUpdateTime) < cfg.UpdateIntervalMs())
			return;
		m_fDCO_LastUpdateTime = now;

		int strength;
		if (m_aInfoComponents)
			strength = m_aInfoComponents.Count();

		if (strength > m_iDCO_MaxStrength)
			m_iDCO_MaxStrength = strength;

		if (m_iDCO_MaxStrength <= 0 || strength <= 0)
			return;

		float strengthRatio	= strength / (float)m_iDCO_MaxStrength;
		float lossFrac		= 1.0 - strengthRatio;
		float threat		= GetThreatMeasure();

		bool casualtyThisTick = (m_iDCO_LastStrength >= 0 && strength < m_iDCO_LastStrength);
		if (casualtyThisTick && cfg.m_fMoraleLostPerCasualty > 0)
		{
			int lost = m_iDCO_LastStrength - strength;
			m_fDCO_Morale -= lost * cfg.m_fMoraleLostPerCasualty;
		}
		if (casualtyThisTick)
			m_fDCO_LastCasualtyMs = now;
		m_iDCO_LastStrength = strength;

		if (threat >= cfg.m_fHeavyThreat)
		{
			m_fDCO_Morale -= cfg.m_fDrainPerTick + (lossFrac * cfg.m_fCasualtyWeight);
			// Refresh the last-known enemy centroid while under fire, so the flee already has a valid "away" direction once morale collapses.
			vector tp;
			DCO_GetThreatPosition(tp);
		}
		else
			m_fDCO_Morale += cfg.m_fRecoveryPerTick;

		// Leader-loss shock: losing the leader under fire is a cohesion blow as the squad reorganizes.
		IEntity curLeader = m_Owner.GetLeaderEntity();
		bool recentCasualty = (m_fDCO_LastCasualtyMs >= 0 && (now - m_fDCO_LastCasualtyMs) <= 4000.0);
		if (m_bDCO_HadLeader && curLeader != m_DCO_LastLeader && recentCasualty && cfg.m_fMoraleLostOnLeaderLoss > 0)
		{
			m_fDCO_Morale -= cfg.m_fMoraleLostOnLeaderLoss;
			DCO_Debug.LogGroup("MORALE", curLeader, string.Format("leader-loss shock -%1 -> morale %2", cfg.m_fMoraleLostOnLeaderLoss, m_fDCO_Morale));
		}
		m_DCO_LastLeader = curLeader;
		m_bDCO_HadLeader = (curLeader != null);

		DCO_UpdateMoraleContagion(now);

		m_fDCO_Morale = Math.Clamp(m_fDCO_Morale, 0.0, DCO_MORALE_MAX);

		if (m_fDCO_Morale <= cfg.m_fFleeThreshold)
		{
			// Fanatic troop grade: hold ground - never break/flee.
			if (DCO_BaseSettings.Get().m_bEnableBaseSettings && DCO_BaseSettings.Get().m_bFanaticHoldsGround)
			{
				// stay and fight; skip the flee logic entirely.
			}
			else if (!m_bDCO_Broken)
			{
				// First break: roll the flee chance so it doesn't trigger the instant morale dips.
				if (Math.RandomFloat01() <= cfg.m_fFleeChancePerTick)
					DCO_BreakAndFlee();
			}
			else
			{
				DCO_BreakAndFlee();
			}
		}
		else if (cfg.m_bEnablePanic && !m_bDCO_Broken && m_fDCO_Morale <= cfg.m_fFleeThreshold + cfg.m_fPanicBand)
		{
			DCO_MaybePanic();
		}
		else if (m_bDCO_Broken && m_fDCO_Morale >= cfg.m_fRallyThreshold)
		{
			m_bDCO_Broken = false;
		}

		if (cfg.m_bEnableMoraleAccuracy)
			DCO_ApplyMoraleAccuracy();
		else if (m_Owner && m_iDCO_LastAccuracyBand > 0)
		{
			// Turned off while degraded: restore default skill + fire rate once.
			array<AIAgent> ar = {};
			m_Owner.GetAgents(ar);
			foreach (AIAgent aa : ar)
			{
				if (!aa) continue;
				IEntity ae = aa.GetControlledEntity();
				if (!ae) continue;
				if (DCO_PlayerUtil.IsPlayer(ae)) continue;	// never touch a player's skill/fire-rate.
				SCR_AICombatComponent ac = SCR_AICombatComponent.Cast(ae.FindComponent(SCR_AICombatComponent));
				if (!ac) continue;
				ac.ResetAISkill();
				ac.SetFireRateCoef(1.0, false);
			}
			m_iDCO_LastAccuracyBand = -1;
		}
	}

	protected void DCO_BreakAndFlee()
	{
		if (!m_Mailbox || !m_Owner)
			return;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return;

		bool wasBroken = m_bDCO_Broken;	// so smoke deploys once per break, not on every flee re-issue.

		vector leaderPos = leader.GetOrigin();
		vector fleePos = leaderPos;

		vector threatPos;
		bool hasThreat = DCO_GetThreatPosition(threatPos);
		if (!hasThreat && m_bDCO_HasLastThreat)
		{
			threatPos = m_vDCO_LastThreatPos;
			hasThreat = true;
		}
		if (hasThreat)
		{
			vector dir = leaderPos - threatPos;
			dir[1] = 0;
			if (dir.LengthSq() > 0.01)
			{
				dir.Normalize();
				fleePos = leaderPos + dir * DCO_MoraleSettings.Get().m_fFleeDistance;
			}
		}

		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
		{
			DCO_VehicleUtil.OrderGroupMoveToPosition(m_Owner, fleePos, m_Mailbox);
			m_bDCO_Broken = true;

			if (DCO_Debug.Enabled())
				DCO_Debug.LogGroup("FLEE", leader, string.Format("mounted break -> vehicle withdraw to %1 (hasThreat=%2, morale=%3)", fleePos, hasThreat, m_fDCO_Morale));
			return;
		}

		SCR_AIMessage_Flee msg = new SCR_AIMessage_Flee();
		if (!msg)
			return;

		msg.SetPosition(fleePos);
		m_Mailbox.RequestBroadcast(msg);

		if (DCO_Debug.Enabled())
			DCO_Debug.LogGroup("FLEE", leader, string.Format("break&flee -> %1 (hasThreat=%2, morale=%3)", fleePos, hasThreat, m_fDCO_Morale));

		m_bDCO_Broken = true;

		// Smoke-covered retreat: screen the withdrawal toward the enemy, once per break.
		if (!wasBroken && hasThreat && DCO_MoraleSettings.Get().m_bEnableFleeSmoke)
			DCO_DeploySmoke(threatPos);
	}

	// Deploy a smoke screen between the fleeing group and the threat, via the native smoke-cover feature.
	protected void DCO_DeploySmoke(vector threatPos)
	{
		if (!m_Owner)
			return;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return;

		// Offset the screen point toward the enemy so it stays within the feature's ~40 m thrower range.
		vector leaderPos = leader.GetOrigin();
		vector dir = threatPos - leaderPos;
		dir[1] = 0;
		if (dir.LengthSq() > 0.01)
			dir.Normalize();
		vector screenPos = leaderPos + dir * 12.0;

		SCR_AIActivitySmokeCoverFeature feat = new SCR_AIActivitySmokeCoverFeature();
		if (!feat)
			return;

		array<AIAgent> avoid = {};
		array<AIAgent> exclude = {};
		feat.Execute(this, screenPos, SCR_AIActivitySmokeCoverFeatureProperties.NONE, avoid, exclude, 1, null);
	}

	void DCO_UpdateArmorAvoid()
	{
		if (!Replication.IsServer())
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		if (!cfg.m_bEnableFleeFromArmor || !m_Owner || !m_Perception)
			return;

		if (m_bDCO_Broken)
			return;	// already out of the fight.

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_LastArmorAvoidTime >= 0 && (now - m_fDCO_LastArmorAvoidTime) < cfg.m_fFleeFromArmorCheckSec * 1000.0)
			return;
		m_fDCO_LastArmorAvoidTime = now;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return;

		// If we're crewing a vehicle ourselves, don't flee from armour.
		if (CompartmentAccessComponent.GetVehicleIn(leader))
			return;

		array<IEntity> targets = m_Perception.m_aTargetEntities;
		if (!targets || targets.IsEmpty())
			return;

		vector leaderPos = leader.GetOrigin();
		float rangeSq = cfg.m_fFleeFromArmorRange * cfg.m_fFleeFromArmorRange;

		foreach (IEntity t : targets)
		{
			if (!t)
				continue;
			if (!t.FindComponent(BaseVehicleControllerComponent))
				continue;	// only enemy vehicles trigger this.
			if (vector.DistanceSq(t.GetOrigin(), leaderPos) <= rangeSq)
			{
				DCO_BreakAndFlee();	// run from the armour.
				return;
			}
		}
	}

	protected void DCO_UpdateMoraleContagion(float now)
	{
		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		if (!cfg.m_bEnableMoraleContagion || !m_Owner)
			return;

		if (m_fDCO_LastContagionCheck >= 0 && (now - m_fDCO_LastContagionCheck) < cfg.m_fContagionCheckSec * 1000.0)
			return;
		m_fDCO_LastContagionCheck = now;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return;
		vector selfPos = leader.GetOrigin();
		Faction selfFaction = m_Owner.GetFaction();

		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return;

		float radiusSq = cfg.m_fContagionRadius * cfg.m_fContagionRadius;
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);

		set<SCR_AIGroup> seen = new set<SCR_AIGroup>();
		seen.Insert(m_Owner);
		int brokenNearby = 0;

		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			SCR_AIGroup grp = SCR_AIGroup.Cast(a.GetParentGroup());
			if (!grp || seen.Contains(grp))
				continue;
			seen.Insert(grp);

			if (grp.GetFaction() != selfFaction)
				continue;
			IEntity gl = grp.GetLeaderEntity();
			if (!gl || vector.DistanceSq(gl.GetOrigin(), selfPos) > radiusSq)
				continue;

			SCR_AIGroupUtilityComponent gu = grp.GetGroupUtilityComponent();
			if (gu && gu.m_bDCO_Broken)
				brokenNearby++;
		}

		if (brokenNearby <= 0)
			return;

		float loss = brokenNearby * cfg.m_fContagionMoralePerBrokenGroup;
		if (loss > cfg.m_fContagionMaxLossPerTick)
			loss = cfg.m_fContagionMaxLossPerTick;
		m_fDCO_Morale -= loss;

		DCO_Debug.LogGroup("MORALE", leader, string.Format("contagion: %1 broken nearby -> -%2", brokenNearby, loss));
	}

	protected void DCO_MaybePanic()
	{
		if (m_bDCO_Panicking)
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		if (m_fDCO_LastPanicTime >= 0 && (now - m_fDCO_LastPanicTime) < cfg.m_fPanicCooldownSec * 1000.0)
			return;

		if (Math.RandomFloat01() > cfg.m_fPanicChancePerTick)
			return;

		m_fDCO_LastPanicTime = now;
		DCO_Panic();
	}

	// Per member: hold fire and drop to a crouch for the panic duration.
	protected void DCO_Panic()
	{
		if (!m_Owner)
			return;

		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
			return;

		m_bDCO_Panicking = true;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		int panicMs = (int)(cfg.m_fPanicDurationSec * 1000.0);

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			IEntity ent = agent.GetControlledEntity();
			if (!ent)
				continue;
			if (DCO_PlayerUtil.IsPlayer(ent))
				continue;	// never panic-freeze a player.

			SCR_CharacterControllerComponent cc = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
			if (cc)
			{
				cc.SetWeaponNoFireTime(cfg.m_fPanicDurationSec);
				DCO_StanceUtil.TrySetStance(ent, ECharacterStance.CROUCH, cfg.m_fStanceCooldownSec * 1000.0);
			}
		}

		GetGame().GetCallqueue().CallLater(DCO_EndPanic, panicMs, false);
	}

	protected void DCO_EndPanic()
	{
		m_bDCO_Panicking = false;
	}

	protected void DCO_ApplyMoraleAccuracy()
	{
		if (!m_Owner)
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();

		int band;
		if (m_fDCO_Morale <= cfg.m_fAccuracyRookieMorale)
			band = 2;
		else if (m_fDCO_Morale <= cfg.m_fAccuracyRegularMorale)
			band = 1;
		else
			band = 0;

		if (band == m_iDCO_LastAccuracyBand)
			return;
		m_iDCO_LastAccuracyBand = band;

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			IEntity ent = agent.GetControlledEntity();
			if (!ent)
				continue;
			if (DCO_PlayerUtil.IsPlayer(ent))
				continue;	// never touch a player's fire-rate/skill.

			SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
			if (!combat)
				continue;

			if (band == 2)
			{
				combat.SetAISkill(EAISkill.ROOKIE);
				combat.SetFireRateCoef(cfg.m_fLowMoraleFireRateCoef, false);
			}
			else if (band == 1)
			{
				combat.SetAISkill(EAISkill.REGULAR);
				combat.SetFireRateCoef(1.0, false);
			}
			else
			{
				if (DCO_BaseSettings.Get().m_bEnableBaseSettings)
				{
					combat.SetAISkill(DCO_BaseSettingsUtil.BaselineSkill());
					combat.SetFireRateCoef(DCO_BaseSettingsUtil.BaselineFireRateCoef(), false);
				}
				else
				{
					combat.ResetAISkill();
					combat.SetFireRateCoef(1.0, false);
				}
			}
		}
	}

	protected bool DCO_GetThreatPosition(out vector outPos)
	{
		if (!m_Perception)
			return false;

		array<IEntity> targets = m_Perception.m_aTargetEntities;
		if (!targets || targets.IsEmpty())
			return false;

		vector sum = vector.Zero;
		int n = 0;
		foreach (IEntity e : targets)
		{
			if (e)
			{
				sum += e.GetOrigin();
				n++;
			}
		}

		if (n == 0)
			return false;

		outPos = sum / n;
		m_vDCO_LastThreatPos = outPos;	// cache for a coherent flee / anti-funnel after perception goes quiet.
		m_bDCO_HasLastThreat = true;
		return true;
	}

	// Public: current perceived-enemy centroid, or the last-known one if perception has gone quiet.
	bool DCO_GetThreatOrLastPosition(out vector outPos)
	{
		if (DCO_GetThreatPosition(outPos))
			return true;

		if (m_bDCO_HasLastThreat)
		{
			outPos = m_vDCO_LastThreatPos;
			return true;
		}
		return false;
	}
}
