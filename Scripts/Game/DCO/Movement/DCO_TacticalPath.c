// Procedural tactical pathing - "smart maneuver" pathing re-decided as the group moves, instead of pathing straight to the destination.
modded class SCR_AIGroupUtilityComponent
{
	// Procedural path state.
	protected vector			m_vDCO_PathDest;	// the fixed end goal.
	protected bool				m_bDCO_HasPathDest		= false;
	protected float				m_fDCO_LastPathReassess	= -1;
	protected vector			m_vDCO_PathNextLeg;	// the leg we are currently bounding to.
	protected bool				m_bDCO_HasNextLeg		= false;
	protected ref array<vector>	m_aDCO_PathPreview;
	protected ref array<ref Shape>	m_aDCO_PathShapes;
	protected bool				m_bDCO_BoundFlip		= false;	// leapfrog toggle: alternates which element bounds vs overwatches.
	protected ref array<vector>	m_aDCO_BaseElemPositions;
	protected int				m_iDCO_MoveTechnique	= 0;

	protected static const float DCO_DEG2RAD = 0.0174533;

	void DCO_SetPathDestination(vector dest)
	{
		m_vDCO_PathDest		= dest;
		m_bDCO_HasPathDest	= true;
		m_fDCO_LastPathReassess = -1;	// force an immediate reassessment on the next tick.
		m_bDCO_HasNextLeg	= false;
	}

	void DCO_UpdateTacticalPath()
	{
		if (!Replication.IsServer())
			return;

		DCO_TacticalMoveSettings cfg = DCO_TacticalMoveSettings.Get();
		if (!cfg || !cfg.m_bEnableProceduralPath || !m_bDCO_HasPathDest || !m_Owner)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		if (m_fDCO_LastPathReassess >= 0 && (now - m_fDCO_LastPathReassess) < cfg.m_fPathReassessSec * 1000.0)
			return;
		m_fDCO_LastPathReassess = now;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
		{
			DCO_ClearPath();
			return;
		}
		vector pos = leader.GetOrigin();

		// Arrived at the end goal: done.
		if (vector.DistanceSq(pos, m_vDCO_PathDest) < cfg.m_fPathArriveDist * cfg.m_fPathArriveDist)
		{
			DCO_ClearPath();
			return;
		}

		// Contact takes precedence over the move.
		if (cfg.m_bPathHaltOnContact && m_Perception)
		{
			array<IEntity> seen = m_Perception.m_aTargetEntities;
			if (seen && !seen.IsEmpty())
			{
				float haltSq = cfg.m_fPathContactHaltRange * cfg.m_fPathContactHaltRange;
				foreach (IEntity t : seen)
				{
					if (!t)
						continue;
					if (vector.DistanceSq(t.GetOrigin(), pos) <= haltSq)
					{
						m_bDCO_HasNextLeg = false;	// stop issuing advance bounds.
						DCO_ClearPathVisualsOnly();
						DCO_Debug.LogGroup("PATH", leader, "in contact - halting tactical advance (fight from here, preserve the element)");
						return;
					}
				}
			}
		}

		float threat = GetThreatMeasure();
		vector threatPos;
		bool hasThreat = DCO_GetThreatOrLastPosition(threatPos);
		if (threat < cfg.m_fPathThreatActivation || !hasThreat)
		{
			// No danger: clear any cover-detour visuals; the group proceeds to the goal normally.
			DCO_ClearPathVisualsOnly();
			return;
		}

		AIPathfindingComponent pf = AIPathfindingComponent.Cast(leader.FindComponent(AIPathfindingComponent));

		// Build the intended path: greedily pick the best next leg through cover, repeating toward the goal.
		DCO_BuildProceduralPath(pos, threatPos, cfg, pf);

		int tech = 0;	// traveling.
		if (threat >= cfg.m_fPathSplitThreatActivation)
			tech = 2;	// bounding overwatch.
		else if (threat >= cfg.m_fPathTravelOverwatchThreat)
			tech = 1;	// traveling overwatch.
		m_iDCO_MoveTechnique = tech;

		bool mounted = DCO_VehicleUtil.IsGroupInVehicle(m_Owner);
		bool handled = false;
		if (tech == 2 && cfg.m_bEnableBaseOfFireSplit && !mounted)
			handled = DCO_TryBoundingOverwatch(cfg, threatPos, pf);
		else if (tech == 1 && cfg.m_bEnableTravelingOverwatch && !mounted)
			handled = DCO_TryTravelingOverwatch(cfg);

		if (!handled && m_bDCO_HasNextLeg)
		{
			if (m_aDCO_BaseElemPositions)
				m_aDCO_BaseElemPositions.Clear();	// no separate element this tick.
			// Traveling: whole unit moves together.
			DCO_VehicleUtil.OrderGroupMoveToPosition(m_Owner, m_vDCO_PathNextLeg, m_Mailbox);
		}

		// Debug: redraw the live path so the adjustment is visible.
		if (cfg.m_bPathDebugDraw || DCO_MoraleSettings.Get().m_bDebug)
			DCO_DrawPath(pos);
		else
			DCO_ClearPathVisualsOnly();

		int legCount = 0;	// Enforce has no ternary operator - precompute for the log.
		if (m_aDCO_PathPreview)
			legCount = m_aDCO_PathPreview.Count();
		DCO_Debug.LogGroup("PATH", leader, string.Format("reassess: threat=%1 legs=%2 next=%3", threat, legCount, m_vDCO_PathNextLeg));
	}

	protected void DCO_BuildProceduralPath(vector startPos, vector threatPos, DCO_TacticalMoveSettings cfg, AIPathfindingComponent pf)
	{
		if (!m_aDCO_PathPreview)
			m_aDCO_PathPreview = {};
		m_aDCO_PathPreview.Clear();

		vector cur = startPos;
		m_bDCO_HasNextLeg = false;

		for (int leg = 0; leg < cfg.m_iPathMaxLegs; leg++)
		{
			// Close enough to commit straight to the goal.
			if (vector.DistanceSq(cur, m_vDCO_PathDest) <= cfg.m_fPathLegLength * cfg.m_fPathLegLength)
			{
				m_aDCO_PathPreview.Insert(m_vDCO_PathDest);
				break;
			}

			vector best;
			if (!DCO_PickBestLeg(cur, threatPos, cfg, pf, best))
			{
				// No good cover candidate - step straight toward the goal so we don't stall.
				vector dir = m_vDCO_PathDest - cur;
				dir[1] = 0;
				dir.Normalize();
				best = cur + dir * cfg.m_fPathLegLength;
				DCO_SnapToNavmesh(pf, best, best);
			}

			m_aDCO_PathPreview.Insert(best);
			cur = best;
		}

		if (!m_aDCO_PathPreview.IsEmpty())
		{
			m_vDCO_PathNextLeg = m_aDCO_PathPreview[0];
			m_bDCO_HasNextLeg = true;
		}
	}

	protected bool DCO_PickBestLeg(vector cur, vector threatPos, DCO_TacticalMoveSettings cfg, AIPathfindingComponent pf, out vector best)
	{
		vector toGoal = m_vDCO_PathDest - cur;
		toGoal[1] = 0;
		float goalDist = toGoal.Length();
		if (goalDist < 1.0)
			return false;
		vector fwd = toGoal / goalDist;

		float bestScore = -99999.0;
		bool found = false;

		int samples = cfg.m_iPathConeSamples;
		if (samples < 1)
			samples = 1;

		for (int i = 0; i < samples; i++)
		{
			// Spread samples evenly across [-cone, +cone] around the bearing to the goal.
			float t = 0.0;
			if (samples > 1)
				t = (i / (float)(samples - 1)) * 2.0 - 1.0;	// -1..+1.
			float ang = t * cfg.m_fPathConeHalfAngleDeg * DCO_DEG2RAD;
			float s = Math.Sin(ang);
			float c = Math.Cos(ang);

			vector dir = Vector(fwd[0] * c - fwd[2] * s, 0, fwd[0] * s + fwd[2] * c);
			vector cand = cur + dir * cfg.m_fPathLegLength;
			DCO_SnapToNavmesh(pf, cand, cand);

			// Don't route a leg up onto an upper floor / attic - keep the bound on the current level.
			if (cand[1] > cur[1] + cfg.m_fCoverMaxClimb)
				continue;

			// Concealment: is the candidate hidden from the threat?
			bool concealed = DCO_IsConcealedFromThreat(pf, threatPos, cand, cfg);

			float progress = goalDist - vector.Distance(cand, m_vDCO_PathDest);

			// Score: progress, with a strong concealment bonus and a mild penalty for veering off-axis.
			float score = progress;
			if (concealed)
				score += cfg.m_fPathConcealmentBonus;
			score -= Math.AbsFloat(t) * cfg.m_fPathStraightBias;

			if (score > bestScore)
			{
				bestScore = score;
				best = cand;
				found = true;
			}
		}

		return found;
	}

	protected bool DCO_IsConcealedFromThreat(AIPathfindingComponent pf, vector threatPos, vector pos, DCO_TacticalMoveSettings cfg)
	{
		if (!pf)
			return false;

		vector eye = Vector(0, 1.5, 0);
		vector hit;
		return pf.RayTrace(threatPos + eye, pos + eye, hit);
	}

	// Snap a candidate point onto the navmesh so legs land on walkable ground.
	protected void DCO_SnapToNavmesh(AIPathfindingComponent pf, vector inPos, out vector outPos)
	{
		outPos = inPos;
		if (!pf)
			return;

		vector corrected;
		if (pf.GetClosestPositionOnNavmesh(inPos, Vector(6, 2, 6), corrected))
			outPos = corrected;
	}

	protected bool DCO_TryTravelingOverwatch(DCO_TacticalMoveSettings cfg)
	{
		if (!m_FireteamMgr || !m_Mailbox || !m_bDCO_HasNextLeg)
			return false;

		array<ref SCR_AIGroupFireteam> fireteams = {};
		DCO_FireteamCompat.GetAllFireteams(m_FireteamMgr, m_Owner, fireteams);
		if (fireteams.Count() < 2)
			return false;

		SCR_AIGroupFireteam ftLead;
		SCR_AIGroupFireteam ftTrail;
		foreach (SCR_AIGroupFireteam ft : fireteams)
		{
			if (!ft || ft.GetMemberCount() <= 0)
				continue;
			if (!ftLead)
				ftLead = ft;
			else
			{
				ftTrail = ft;
				break;
			}
		}
		if (!ftLead || !ftTrail)
			return false;

		AICommunicationComponent comms = m_Mailbox;

		// Lead element advances to the next path leg.
		array<AIAgent> leadMembers = {};
		ftLead.GetMembers(leadMembers);
		foreach (AIAgent lm : leadMembers)
		{
			if (!lm)
				continue;
			IEntity lmEnt = lm.GetControlledEntity();
			if (!lmEnt || DCO_PlayerUtil.IsPlayer(lmEnt))
				continue;	// never issue traveling-overwatch moves to a player-controlled member.
			SCR_AIMessage_Move msg = SCR_AIMessage_Move.Create(null, m_vDCO_PathNextLeg, EMovementType.RUN, false, null);
			if (!msg)
				continue;
			msg.SetReceiver(lm);
			comms.RequestBroadcast(msg, lm);
		}

		IEntity leader = m_Owner.GetLeaderEntity();
		vector trailPos = m_vDCO_PathNextLeg;	// no ternary in Enforce.
		if (leader)
			trailPos = leader.GetOrigin();

		array<AIAgent> trailMembers = {};
		ftTrail.GetMembers(trailMembers);
		foreach (AIAgent tm : trailMembers)
		{
			if (!tm)
				continue;
			IEntity tmEnt = tm.GetControlledEntity();
			if (!tmEnt || DCO_PlayerUtil.IsPlayer(tmEnt))
				continue;	// never issue traveling-overwatch moves to a player-controlled member.
			SCR_AIMessage_Move msg2 = SCR_AIMessage_Move.Create(null, trailPos, EMovementType.WALK, false, null);
			if (!msg2)
				continue;
			msg2.SetReceiver(tm);
			comms.RequestBroadcast(msg2, tm);
		}

		if (m_aDCO_BaseElemPositions)
			m_aDCO_BaseElemPositions.Clear();	// trailing element is moving, not a static base.

		return true;
	}

	protected bool DCO_TryBoundingOverwatch(DCO_TacticalMoveSettings cfg, vector threatPos, AIPathfindingComponent pf)
	{
		if (!m_FireteamMgr || !m_Mailbox || !m_bDCO_HasNextLeg)
			return false;

		array<ref SCR_AIGroupFireteam> fireteams = {};
		DCO_FireteamCompat.GetAllFireteams(m_FireteamMgr, m_Owner, fireteams);
		if (fireteams.Count() < 2)
			return false;	// need at least two elements to split.

		// First two non-empty fireteams.
		SCR_AIGroupFireteam ftA;
		SCR_AIGroupFireteam ftB;
		foreach (SCR_AIGroupFireteam ft : fireteams)
		{
			if (!ft || ft.GetMemberCount() <= 0)
				continue;
			if (!ftA)
				ftA = ft;
			else
			{
				ftB = ft;
				break;
			}
		}
		if (!ftA || !ftB)
			return false;

		// Assign maneuver vs base.
		SCR_AIGroupFireteam leaderFt;
		AIAgent leaderAgent = m_Owner.GetLeaderAgent();
		if (leaderAgent)
			leaderFt = m_FireteamMgr.FindFireteam(leaderAgent);

		SCR_AIGroupFireteam maneuverFt;
		SCR_AIGroupFireteam baseFt;
		if (cfg.m_bPathLeaderInBase && leaderFt && (leaderFt == ftA || leaderFt == ftB))
		{
			baseFt = leaderFt;
			if (leaderFt == ftA)
				maneuverFt = ftB;
			else
				maneuverFt = ftA;
		}
		else
		{
			// Leapfrog: alternate which element bounds vs.
			maneuverFt = ftA;
			baseFt = ftB;
			if (m_bDCO_BoundFlip)
			{
				maneuverFt = ftB;
				baseFt = ftA;
			}
			m_bDCO_BoundFlip = !m_bDCO_BoundFlip;
		}

		AICommunicationComponent comms = m_Mailbox;

		array<AIAgent> maneuverMembers = {};
		maneuverFt.GetMembers(maneuverMembers);
		foreach (AIAgent mv : maneuverMembers)
		{
			if (!mv)
				continue;
			IEntity mvEnt = mv.GetControlledEntity();
			if (!mvEnt || DCO_PlayerUtil.IsPlayer(mvEnt))
				continue;	// never issue bounding moves to a player-controlled member.
			SCR_AIMessage_Move msg = SCR_AIMessage_Move.Create(null, m_vDCO_PathNextLeg, EMovementType.RUN, false, null);
			if (!msg)
				continue;
			msg.SetReceiver(mv);
			comms.RequestBroadcast(msg, mv);
		}

		// Base-of-fire element overwatches - but it must stay useful.
		if (!m_aDCO_BaseElemPositions)
			m_aDCO_BaseElemPositions = {};
		m_aDCO_BaseElemPositions.Clear();

		array<AIAgent> baseMembers = {};
		baseFt.GetMembers(baseMembers);
		foreach (AIAgent bm : baseMembers)
		{
			if (!bm)
				continue;
			IEntity be = bm.GetControlledEntity();
			if (!be)
				continue;
			if (DCO_PlayerUtil.IsPlayer(be))
				continue;	// never issue overwatch hold-moves to a player-controlled member.
			vector bp = be.GetOrigin();

			vector holdPos = bp;
			if (cfg.m_bPathBaseMaintainLos && DCO_IsConcealedFromThreat(pf, threatPos, bp, cfg))
			{
				vector losPos;
				if (DCO_FindOverwatchPos(pf, bp, threatPos, cfg, losPos))
					holdPos = losPos;
			}
			m_aDCO_BaseElemPositions.Insert(holdPos);

			SCR_AIMessage_Move hold = SCR_AIMessage_Move.Create(null, holdPos, EMovementType.WALK, false, null);
			if (!hold)
				continue;
			hold.SetReceiver(bm);
			comms.RequestBroadcast(hold, bm);
		}

		return true;
	}

	protected bool DCO_FindOverwatchPos(AIPathfindingComponent pf, vector pos, vector threatPos, DCO_TacticalMoveSettings cfg, out vector best)
	{
		float maxClimb = cfg.m_fCoverMaxClimb;
		float bestScore = -99999.0;
		bool found = false;

		for (int i = 0; i < 8; i++)
		{
			float ang = (i / 8.0) * 6.2831853;
			vector cand = pos + Vector(Math.Cos(ang), 0, Math.Sin(ang)) * cfg.m_fPathBaseLosRadius;
			DCO_SnapToNavmesh(pf, cand, cand);
			if (cand[1] > pos[1] + maxClimb)
				continue;	// don't climb to an attic.
			if (DCO_IsConcealedFromThreat(pf, threatPos, cand, cfg))
				continue;	// must be able to see the threat from here.

			float score = (cand[1] - pos[1]) * 5.0 - vector.Distance(cand, pos) * 0.1;	// prefer higher, nearer.
			if (score > bestScore)
			{
				bestScore = score;
				best = cand;
				found = true;
			}
		}
		return found;
	}

	protected void DCO_DrawPath(vector startPos)
	{
		DCO_ClearPathVisualsOnly();
		if (System.IsConsoleApp())
			return;	// headless dedi: path arrows render nowhere - don't churn them.
		if (!m_aDCO_PathPreview || m_aDCO_PathPreview.IsEmpty())
			return;

		m_aDCO_PathShapes = {};

		int COL_CURRENT		= 0xFF39FF14;	// bright green = current leg.
		int COL_INTENDED	= 0x80FFC400;	// translucent amber = intended legs.
		ShapeFlags flags = ShapeFlags.NOZBUFFER;
		vector lift = Vector(0, 0.5, 0);	// lift slightly off the ground so the line is visible.

		vector prev = startPos;
		for (int i = 0; i < m_aDCO_PathPreview.Count(); i++)
		{
			vector leg = m_aDCO_PathPreview[i];
			int col = COL_INTENDED;
			if (i == 0)
				col = COL_CURRENT;

			Shape arrow = Shape.CreateArrow(prev + lift, leg + lift, 0.6, col, flags);
			if (arrow)
				m_aDCO_PathShapes.Insert(arrow);

			prev = leg;
		}

		if (m_aDCO_BaseElemPositions && !m_aDCO_BaseElemPositions.IsEmpty())
		{
			int COL_BASE = 0xFF00E5FF;	// cyan = base of fire / overwatch.
			foreach (vector bp : m_aDCO_BaseElemPositions)
			{
				Shape s = Shape.CreateSphere(COL_BASE, flags, bp + lift, 0.6);
				if (s)
					m_aDCO_PathShapes.Insert(s);
			}
		}
	}

	protected void DCO_ClearPathVisualsOnly()
	{
		if (m_aDCO_PathShapes)
			m_aDCO_PathShapes.Clear();	// releasing the held Shape refs frees the visualizers.
	}

	// Full reset: drop the goal, the planned legs and the debug visuals.
	protected void DCO_ClearPath()
	{
		m_bDCO_HasPathDest	= false;
		m_bDCO_HasNextLeg	= false;
		if (m_aDCO_PathPreview)
			m_aDCO_PathPreview.Clear();
		if (m_aDCO_BaseElemPositions)
			m_aDCO_BaseElemPositions.Clear();
		DCO_ClearPathVisualsOnly();
	}
}
