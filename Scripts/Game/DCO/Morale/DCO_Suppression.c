// Real suppression.
modded class SCR_AIGroupUtilityComponent
{
	protected float	m_fDCO_LastSuppressionTime	= -1;
	protected bool	m_bDCO_SuppressionSmokeUp	= false;
	protected ref map<IEntity, float> m_mDCO_CoverSettleUntil;
	protected ref map<IEntity, bool> m_mDCO_SuppFireDropped;	// members whose fire rate THIS feature degraded - only these get the 1.0 restore.

	protected static const float DCO_COVER_SETTLE_MIN_SEC = 1.25;
	protected static const float DCO_COVER_SETTLE_MAX_SEC = 3.0;
	protected static const float DCO_COVER_SCAN_MIN_DEG = 18.0;
	protected static const float DCO_COVER_SCAN_MAX_DEG = 42.0;
	protected static const float DCO_COVER_SCAN_DISTANCE = 40.0;
	protected static const float DCO_DEG2RAD_SUPP = 0.0174533;

	void DCO_UpdateSuppression()
	{
		if (!Replication.IsServer())
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		if (!cfg.m_bEnableSuppression || !m_Owner || !m_Mailbox)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_LastSuppressionTime >= 0 && (now - m_fDCO_LastSuppressionTime) < cfg.m_fSuppressionCheckSec * 1000.0)
			return;
		m_fDCO_LastSuppressionTime = now;

		// Vehicle crews handled elsewhere.
		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
			return;

		vector threatPos;
		bool hasThreat = DCO_GetThreatOrLastPosition(threatPos);

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		AICommunicationComponent comms = m_Mailbox;

		int total = 0;
		int suppressed = 0;

		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity ent = a.GetControlledEntity();
			if (!ent)
				continue;
			if (DCO_PlayerUtil.IsPlayer(ent))
				continue;	// never suppress/seek-cover a player.
			total++;

			SCR_ChimeraAIAgent chim = SCR_ChimeraAIAgent.Cast(a);
			if (!chim || !chim.m_UtilityComponent || !chim.m_UtilityComponent.m_ThreatSystem)
				continue;

			float supp = chim.m_UtilityComponent.m_ThreatSystem.GetSuppressionMeasure();
			bool isSupp = supp >= cfg.m_fSuppressionThreshold;

			SCR_AICombatComponent combat = chim.m_UtilityComponent.m_CombatComponent;
			if (combat && !combat.DCO_GetUnitOverride())
			{
				if (!m_mDCO_SuppFireDropped)
					m_mDCO_SuppFireDropped = new map<IEntity, bool>();
				if (isSupp)
				{
					combat.SetFireRateCoef(cfg.m_fSuppressedFireRateCoef, false);
					m_mDCO_SuppFireDropped.Set(ent, true);
				}
				else if (m_mDCO_SuppFireDropped.Contains(ent))
				{
					combat.SetFireRateCoef(1.0, false);	// released; the applier/morale re-cover their own coef next tick.
					m_mDCO_SuppFireDropped.Remove(ent);
				}
			}

			if (isSupp)
			{
				suppressed++;
				if (cfg.m_bSuppressionSeekCover && hasThreat)
					DCO_SuppSeekCover(a, ent, threatPos, comms, cfg, now);
			}
			else if (m_mDCO_CoverSettleUntil)
			{
				m_mDCO_CoverSettleUntil.Remove(ent);
			}
		}

		if (total > 0 && hasThreat)
		{
			float frac = suppressed / (float)total;
			if (frac >= cfg.m_fSuppressionSmokeFraction)
			{
				if (!m_bDCO_SuppressionSmokeUp)
				{
					DCO_DeploySmoke(threatPos);
					m_bDCO_SuppressionSmokeUp = true;
				}
			}
			else
			{
				m_bDCO_SuppressionSmokeUp = false;
			}
		}

		if (suppressed > 0)
			DCO_Debug.LogGroup("SUPPRESS", m_Owner.GetLeaderEntity(), string.Format("%1/%2 pinned (threshold %3)", suppressed, total, cfg.m_fSuppressionThreshold));
	}

	protected void DCO_SuppSeekCover(AIAgent a, IEntity ent, vector threatPos, AICommunicationComponent comms, DCO_MoraleSettings cfg, float now)
	{
		AIPathfindingComponent pf = AIPathfindingComponent.Cast(ent.FindComponent(AIPathfindingComponent));
		vector pos = ent.GetOrigin();

		if (DCO_SuppRayBlocked(pf, threatPos, pos))
			return;	// already in cover/concealment - hold here.

		// In dig-in mode, only look for cover within a SHORT dash so AI don't sprint across open ground.
		float searchRadius = cfg.m_fSuppressionCoverRadius;
		if (cfg.m_bSuppressionDigIn && cfg.m_fSuppressionDashMax < searchRadius)
			searchRadius = cfg.m_fSuppressionDashMax;

		vector best;
		bool found = false;
		float bestSq = 0;
		float maxClimb = DCO_TacticalMoveSettings.Get().m_fCoverMaxClimb;
		for (int i = 0; i < 8; i++)
		{
			float ang = (i / 8.0) * 6.2831853;
			vector cand = pos + Vector(Math.Cos(ang), 0, Math.Sin(ang)) * searchRadius;
			DCO_SuppSnapNav(pf, cand, cand);
			if (cand[1] > pos[1] + maxClimb)
				continue;	// stay on this floor - don't dash up to the attic.
			if (cfg.m_bSuppressionDigIn && cand[1] < pos[1] - cfg.m_fSuppressionMaxDescend)
				continue;	// don't abandon the high ground / drop into a hole below the position.
			if (!DCO_SuppRayBlocked(pf, threatPos, cand))
				continue;	// candidate must actually break LOS to the threat.
			float d = vector.DistanceSq(cand, pos);
			if (!found || d < bestSq)
			{
				bestSq = d;
				best = cand;
				found = true;
			}
		}

		if (found)
		{
			if (!m_mDCO_CoverSettleUntil)
				m_mDCO_CoverSettleUntil = new map<IEntity, float>();
			float settleUntil;
			if (m_mDCO_CoverSettleUntil.Find(ent, settleUntil) && now < settleUntil)
				return;	// let the current bound and arrival scan finish before issuing another move.

			// Close, position-holding cover exists: short bound to it.
			SCR_AIMessage_Move msg = SCR_AIMessage_Move.Create(null, best, EMovementType.SPRINT, false, null);
			if (msg)
			{
				msg.SetReceiver(a);
				comms.RequestBroadcast(msg, a);

				float dashSec = Math.Sqrt(bestSq) / 5.0;
				float settleSec = Math.RandomFloat(DCO_COVER_SETTLE_MIN_SEC, DCO_COVER_SETTLE_MAX_SEC);
				float scanSide = 1.0;
				if (Math.RandomFloat01() < 0.5)
					scanSide = -1.0;
				m_mDCO_CoverSettleUntil.Set(ent, now + (dashSec + settleSec) * 1000.0);

				CharacterControllerComponent cc = CharacterControllerComponent.Cast(ent.FindComponent(CharacterControllerComponent));
				if (cc)
					cc.SetWeaponNoFireTime(dashSec + settleSec);

				GetGame().GetCallqueue().CallLater(DCO_SuppBeginCoverSettle, (int)(dashSec * 1000.0), false, a, ent, best, threatPos, settleSec, scanSide);
			}
		}
		else if (cfg.m_bSuppressionDigIn)
		{
			// No close cover that holds the position: dig in, hit the dirt and fight from here rather than running into the open to look for cover.
			DCO_StanceUtil.TrySetStance(ent, ECharacterStance.PRONE, cfg.m_fStanceCooldownSec * 1000.0);
		}
	}

	// The engine normally reacquires its enemy the instant a cover bound ends.
	protected void DCO_SuppBeginCoverSettle(AIAgent a, IEntity ent, vector coverPos, vector threatPos, float settleSec, float scanSide)
	{
		if (!Replication.IsServer() || !a || !ent || DCO_PlayerUtil.IsPlayer(ent))
			return;
		if (vector.DistanceSq(ent.GetOrigin(), coverPos) > 25.0)
			return;	// the bound failed or was replaced; do not force a scan in the open.

		CharacterControllerComponent cc = CharacterControllerComponent.Cast(ent.FindComponent(CharacterControllerComponent));
		if (cc)
		{
			cc.SetWeaponRaised(false);
			cc.SetWeaponNoFireTime(settleSec);
		}

		SCR_ChimeraAIAgent chim = SCR_ChimeraAIAgent.Cast(a);
		if (!chim || !chim.m_UtilityComponent)
			return;

		float firstSec = settleSec * 0.5;
		vector firstLook = DCO_SuppSectorLook(ent.GetOrigin(), threatPos, scanSide);
		chim.m_UtilityComponent.LookAt(firstLook, firstSec);
		GetGame().GetCallqueue().CallLater(DCO_SuppContinueCoverScan, (int)(firstSec * 1000.0), false, a, ent, threatPos, settleSec - firstSec, -scanSide);
	}

	protected void DCO_SuppContinueCoverScan(AIAgent a, IEntity ent, vector threatPos, float durationSec, float scanSide)
	{
		if (!Replication.IsServer() || !a || !ent || DCO_PlayerUtil.IsPlayer(ent))
			return;

		CharacterControllerComponent cc = CharacterControllerComponent.Cast(ent.FindComponent(CharacterControllerComponent));
		if (cc)
			cc.SetWeaponRaised(false);

		SCR_ChimeraAIAgent chim = SCR_ChimeraAIAgent.Cast(a);
		if (!chim || !chim.m_UtilityComponent)
			return;
		chim.m_UtilityComponent.LookAt(DCO_SuppSectorLook(ent.GetOrigin(), threatPos, scanSide), durationSec);
	}

	protected vector DCO_SuppSectorLook(vector pos, vector threatPos, float side)
	{
		vector toThreat = threatPos - pos;
		toThreat[1] = 0;
		float length = toThreat.Length();
		if (length < 1.0)
			return threatPos;
		vector dir = toThreat / length;
		float angle = Math.RandomFloat(DCO_COVER_SCAN_MIN_DEG, DCO_COVER_SCAN_MAX_DEG) * DCO_DEG2RAD_SUPP * side;
		float s = Math.Sin(angle);
		float c = Math.Cos(angle);
		vector scanDir = Vector(dir[0] * c - dir[2] * s, 0, dir[0] * s + dir[2] * c);
		return pos + Vector(0, 1.5, 0) + scanDir * DCO_COVER_SCAN_DISTANCE;
	}

	protected bool DCO_SuppRayBlocked(AIPathfindingComponent pf, vector threatPos, vector pos)
	{
		if (!pf)
			return false;
		vector eye = Vector(0, 1.5, 0);
		vector hit;
		return pf.RayTrace(threatPos + eye, pos + eye, hit);
	}

	protected void DCO_SuppSnapNav(AIPathfindingComponent pf, vector inPos, out vector outPos)
	{
		outPos = inPos;
		if (!pf)
			return;
		vector corrected;
		if (pf.GetClosestPositionOnNavmesh(inPos, Vector(6, 2, 6), corrected))
			outPos = corrected;
	}
}
