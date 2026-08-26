// Machine-gunner emplacement.
modded class SCR_AIGroupUtilityComponent
{
	protected float	m_fDCO_LastMGTime	= -1;

	// Per-gunner emplacement state, keyed by the gunner's controlled entity.
	protected ref map<IEntity, float>	m_mDCO_MGDeployed;
	protected ref map<IEntity, float>	m_mDCO_MGMoveSince;

	protected static const float DCO_MG_REPATH_MS = 4000.0;

	void DCO_UpdateMachineGunner()
	{
		if (!Replication.IsServer())
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		if (!cfg.m_bEnableMachineGunner || !m_Owner || !m_Mailbox || !m_Perception)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_LastMGTime >= 0 && (now - m_fDCO_LastMGTime) < cfg.m_fMGCheckSec * 1000.0)
			return;
		m_fDCO_LastMGTime = now;

		// Below the throttle: IsGroupInVehicle is a per-member FindComponent scan.
		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
			return;

		if (!DCO_ContactUtil.HasLiveContact(m_Perception, DCO_ContactUtil.FRESH_CONTACT_S))
		{
			DCO_MGClearAll();
			return;
		}

		if (!m_mDCO_MGDeployed)
			m_mDCO_MGDeployed = new map<IEntity, float>();
		if (!m_mDCO_MGMoveSince)
			m_mDCO_MGMoveSince = new map<IEntity, float>();

		for (int di = m_mDCO_MGDeployed.Count() - 1; di >= 0; di--)
		{
			if (!m_mDCO_MGDeployed.GetKey(di))
				m_mDCO_MGDeployed.RemoveElement(di);
		}
		for (int di = m_mDCO_MGMoveSince.Count() - 1; di >= 0; di--)
		{
			if (!m_mDCO_MGMoveSince.GetKey(di))
				m_mDCO_MGMoveSince.RemoveElement(di);
		}

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			SCR_ChimeraAIAgent chim = SCR_ChimeraAIAgent.Cast(a);
			if (!chim || !chim.m_UtilityComponent || !chim.m_UtilityComponent.m_CombatComponent)
				continue;

			// MG carriers only.
			if (chim.m_UtilityComponent.m_CombatComponent.GetCurrentWeaponType() != EWeaponType.WT_MACHINEGUN)
				continue;

			IEntity ent = a.GetControlledEntity();
			if (!ent)
				continue;
			if (DCO_PlayerUtil.IsPlayer(ent))
				continue;	// a player carrying an MG is never ordered to a firing position.
			vector pos = ent.GetOrigin();

			// Nearest LIVE enemy within MG engagement range.
			vector enemy;
			if (!DCO_ContactUtil.GetLiveThreatNear(m_Perception, pos, cfg.m_fMGEngageRange, DCO_ContactUtil.FRESH_CONTACT_S, enemy))
			{
				m_mDCO_MGDeployed.Remove(ent);
				m_mDCO_MGMoveSince.Remove(ent);
				continue;
			}

			AIPathfindingComponent pf = AIPathfindingComponent.Cast(ent.FindComponent(AIPathfindingComponent));
			float eye = cfg.m_fMGSightHeight;
			bool hasLos = !DCO_MGLosBlocked(pf, pos, enemy, eye);

			// Already emplaced: just hold.
			if (m_mDCO_MGDeployed.Contains(ent))
			{
				if (hasLos)
				{
					DCO_StanceUtil.TrySetStance(ent, ECharacterStance.PRONE, cfg.m_fStanceCooldownSec * 1000.0);
					continue;
				}
				m_mDCO_MGDeployed.Remove(ent);	// lost LOS - tear down and find a new firing position.
			}

			if (hasLos)
			{
				DCO_MGDeploy(a, ent, pos);
				m_mDCO_MGDeployed.Set(ent, now);
				m_mDCO_MGMoveSince.Remove(ent);
				if (cfg.m_bDebug)
					DCO_Debug.LogGroup("MG", ent, "emplaced - holding firing position");
				continue;
			}

			// No LOS: path to a firing position first, then emplace next tick on arrival.
			if (!cfg.m_bMGReposition)
				continue;

			// Recently ordered to a firing position?
			float lastMove;
			if (m_mDCO_MGMoveSince.Find(ent, lastMove) && (now - lastMove) < DCO_MG_REPATH_MS)
				continue;

			vector firePos;
			if (DCO_MGFindFiringPos(pf, pos, enemy, eye, cfg, firePos))
			{
				SCR_AIMessage_Move msg = SCR_AIMessage_Move.Create(null, firePos, EMovementType.RUN, false, null);
				if (msg)
				{
					msg.SetReceiver(a);
					m_Mailbox.RequestBroadcast(msg, a);
				}
				m_mDCO_MGMoveSince.Set(ent, now);
				if (cfg.m_bDebug)
					DCO_Debug.LogGroup("MG", ent, "pathing to firing position");
			}
		}
	}

	protected void DCO_MGClearAll()
	{
		if (m_mDCO_MGDeployed)
			m_mDCO_MGDeployed.Clear();
		if (m_mDCO_MGMoveSince)
			m_mDCO_MGMoveSince.Clear();
	}

	protected void DCO_MGDeploy(AIAgent a, IEntity ent, vector pos)
	{
		SCR_AIMessage_Move hold = SCR_AIMessage_Move.Create(null, pos, EMovementType.WALK, false, null);
		if (hold)
		{
			hold.SetReceiver(a);
			m_Mailbox.RequestBroadcast(hold, a);
		}

		DCO_StanceUtil.TrySetStance(ent, ECharacterStance.PRONE, DCO_MoraleSettings.Get().m_fStanceCooldownSec * 1000.0);
	}

	// Ring-sample for the nearest walkable spot with LOS to the enemy, preferring higher ground.
	protected bool DCO_MGFindFiringPos(AIPathfindingComponent pf, vector pos, vector enemy, float eye, DCO_MoraleSettings cfg, out vector best)
	{
		float maxClimb = DCO_TacticalMoveSettings.Get().m_fCoverMaxClimb;
		float bestScore = -99999.0;
		bool found = false;

		for (int i = 0; i < 8; i++)
		{
			float ang = (i / 8.0) * 6.2831853;
			vector cand = pos + Vector(Math.Cos(ang), 0, Math.Sin(ang)) * cfg.m_fMGRepositionRadius;
			DCO_MGSnapNav(pf, cand, cand);
			if (cand[1] > pos[1] + maxClimb)
				continue;	// don't climb to an attic.
			if (DCO_MGLosBlocked(pf, cand, enemy, eye))
				continue;	// must actually see the enemy from here.

			float score = (cand[1] - pos[1]) * 5.0 - vector.Distance(cand, pos) * 0.1;
			if (score > bestScore)
			{
				bestScore = score;
				best = cand;
				found = true;
			}
		}
		return found;
	}

	protected bool DCO_MGLosBlocked(AIPathfindingComponent pf, vector fromPos, vector enemyPos, float eye)
	{
		if (!pf)
			return false;
		vector hit;
		return pf.RayTrace(fromPos + Vector(0, eye, 0), enemyPos + Vector(0, eye, 0), hit);
	}

	// Snap a candidate onto the navmesh; unchanged if no pathfinding/no hit.
	protected void DCO_MGSnapNav(AIPathfindingComponent pf, vector inPos, out vector outPos)
	{
		outPos = inPos;
		if (!pf)
			return;
		vector corrected;
		if (pf.GetClosestPositionOnNavmesh(inPos, Vector(8, 2, 8), corrected))
			outPos = corrected;
	}
}
