class DCO_CqbClearSettings
{
	protected static ref DCO_CqbClearSettings s_Instance;

	bool m_bDebug;
	bool m_bDebugCqbClear;
	float m_fCqbClearRadius = 80.0;
	float m_fCqbNodeDwellSec = 3.0;
	float m_fCqbClearCheckSec = 2.0;
	float m_fCqbApproachTimeoutSec = 30.0;
	float m_fCqbPerceptionBoost = 2.0;

	static DCO_CqbClearSettings Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_CqbClearSettings();
		return s_Instance;
	}
}

// Directed CQB building clear.
modded class SCR_AIGroupUtilityComponent
{
	protected EDCO_CqbState		m_eDCO_CqbState = EDCO_CqbState.IDLE;
	protected ref DCO_CqbJob	m_DCO_CqbJob;
	protected float				m_fDCO_LastCqbClearTime = -1;
	protected bool				m_bDCO_CqbOwnsMove = false;
	protected ref array<IEntity> m_aDCO_CqbBuildings;

	protected static const float DCO_CQB_CELL_ARRIVE_M = 1.5;
	protected static const float DCO_CQB_COMPLETED_NEAR_M = 2.25;
	protected static const float DCO_CQB_BUILDING_THREAT_M = 6.0;
	protected static const float DCO_CQB_DOOR_ASSIST_M = 3.5;
	protected static const float DCO_CQB_WINGMAN_RELEASE_M = 3.25;
	protected static const float DCO_CQB_DOOR_STALL_MS = 4500.0;
	protected static const float DCO_CQB_RETRY_STALL_MS = 14000.0;
	protected static const float DCO_CQB_PROGRESS_EPS_SQ = 1.0;
	protected static const int DCO_CQB_MAX_RETRIES = 1;
	protected static const int DCO_CQB_ENTRY_TEAM_SIZE = 4;
	protected static const float DCO_CQB_STACK_READY_M = 2.0;
	protected static const float DCO_CQB_STACK_TIMEOUT_MS = 9000.0;
	protected static const float DCO_CQB_STACK_BACK_M = 0.45;
	protected static const float DCO_CQB_STACK_LATERAL_M = 1.5;
	protected static const float DCO_CQB_STACK_STEP_M = 0.7;
	protected static const float DCO_CQB_STACK_PAIR_M = 3.0;
	protected static const float DCO_CQB_BREACH_DOOR_M = 1.6;
	protected static const float DCO_CQB_BREACH_GATE_M = 1.2;
	protected static const float DCO_CQB_BREACH_TIMEOUT_MS = 2500.0;
	protected static const float DCO_CQB_FUNNEL_DEPTH_M = 0.8;
	protected static const float DCO_CQB_RELEASE_TIMEOUT_MS = 6000.0;
	protected static const float DCO_CQB_THREAT_MARGIN_M = 1.0;
	protected static const float DCO_CQB_SMALL_ROOM_M2 = 12.0;
	protected static const float DCO_CQB_DWELL_NEAR_M = 3.0;
	protected static const float DCO_CQB_DWELL_FORCE_MS = 45000.0;	// quiet-sector watchdog: resolve a DWELL nobody can physically reach.
	protected static const int DCO_CQB_MAX_RECOVERY_PASSES = 5;
	protected static const float DCO_CQB_MIN_BUILDING_HEIGHT_M = 2.2;
	protected static const float DCO_CQB_MIN_BUILDING_SPAN_M = 3.0;
	protected static const float DCO_CQB_MIN_BUILDING_VOLUME_M3 = 25.0;

	// Keeps detailed CQB flow diagnostics behind the debug setting.
	protected void DCO_CqbLog(string msg)
	{
		if (DCO_CqbClearSettings.Get().m_bDebugCqbClear)
			Print(msg, LogLevel.NORMAL);
	}

	// True while this state machine owns group movement, so the reactive CQB push yields.
	bool DCO_CqbIsClearingActive()
	{
		return m_eDCO_CqbState == EDCO_CqbState.SURVEY
			|| m_eDCO_CqbState == EDCO_CqbState.CLEAR;
	}

	protected bool DCO_CqbDirected()
	{
		return DCO_WaypointIntentUtil.IsCqbDirected(this);
	}

	protected bool DCO_CqbApplies(DCO_CqbClearSettings cfg)
	{
		if (!DCO_CqbDirected())
			return false;
		if (!m_Owner || !m_Mailbox)
			return false;
		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
			return false;
		return true;
	}

	void DCO_UpdateCqbClear()
	{
		if (!Replication.IsServer())
			return;

		DCO_CqbClearSettings cfg = DCO_CqbClearSettings.Get();
		if (!DCO_CqbApplies(cfg))
		{
			DCO_CqbAbort();
			return;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_LastCqbClearTime >= 0 && (now - m_fDCO_LastCqbClearTime) < cfg.m_fCqbClearCheckSec * 1000.0)
			return;
		m_fDCO_LastCqbClearTime = now;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return;
		vector lead = leader.GetOrigin();

		switch (m_eDCO_CqbState)
		{
			case EDCO_CqbState.IDLE:
			case EDCO_CqbState.SELECT:
			case EDCO_CqbState.DONE:
			{
				DCO_CqbSelect(world, leader, lead, cfg, now);
				break;
			}
			case EDCO_CqbState.SURVEY:
			{
				DCO_CqbSurvey(leader, lead, cfg, now);
				break;
			}
			case EDCO_CqbState.CLEAR:
			{
				DCO_CqbClearStep(world, leader, cfg, now);
				break;
			}
		}

	}

	protected AIPathfindingComponent DCO_CqbGetPathfinding(IEntity leader)
	{
		AIPathfindingComponent pf = AIPathfindingComponent.Cast(m_Owner.FindComponent(AIPathfindingComponent));
		if (!pf && leader)
			pf = AIPathfindingComponent.Cast(leader.FindComponent(AIPathfindingComponent));
		return pf;
	}

	protected IEntity DCO_CqbFindBuildingNear(BaseWorld world, vector pos, float radius)
	{
		m_aDCO_CqbBuildings = {};
		world.QueryEntitiesBySphere(pos, radius, DCO_CqbCollectBuilding);

		IEntity bestContain;
		float bestContainVolume;
		IEntity bestNear;
		float bestNearSq = radius * radius + 1;
		foreach (IEntity building : m_aDCO_CqbBuildings)
		{
			if (!building)
				continue;
			vector minimum;
			vector maximum;
			building.GetBounds(minimum, maximum);
			float sizeX = Math.AbsFloat(maximum[0] - minimum[0]);
			float sizeY = Math.AbsFloat(maximum[1] - minimum[1]);
			float sizeZ = Math.AbsFloat(maximum[2] - minimum[2]);
			float volume = sizeX * sizeY * sizeZ;
			if (sizeY < DCO_CQB_MIN_BUILDING_HEIGHT_M || Math.Max(sizeX, sizeZ) < DCO_CQB_MIN_BUILDING_SPAN_M
				|| volume < DCO_CQB_MIN_BUILDING_VOLUME_M3)
				continue;

			vector local = building.CoordToLocal(pos);
			bool contains = local[0] >= minimum[0] - 0.5 && local[0] <= maximum[0] + 0.5
				&& local[2] >= minimum[2] - 0.5 && local[2] <= maximum[2] + 0.5
				&& local[1] >= minimum[1] - 2.0 && local[1] <= maximum[1] + 2.0;
			if (contains && volume > bestContainVolume)
			{
				bestContain = building;
				bestContainVolume = volume;
			}
			float distanceSq = vector.DistanceSq(building.GetOrigin(), pos);
			if (distanceSq < bestNearSq)
			{
				bestNearSq = distanceSq;
				bestNear = building;
			}
		}
		if (bestContain)
			return bestContain;
		return bestNear;
	}

	protected bool DCO_CqbCollectBuilding(IEntity entity)
	{
		if (!entity)
			return true;
		if (Building.Cast(entity) || entity.FindComponent(SCR_DestructibleBuildingComponent))
			m_aDCO_CqbBuildings.Insert(entity);
		return true;
	}

	protected void DCO_CqbSelect(BaseWorld world, IEntity leader, vector lead, DCO_CqbClearSettings cfg, float now)
	{
		vector wpPos = DCO_WaypointIntentUtil.GetIntentPos(this);
		IEntity building = DCO_CqbFindBuildingNear(world, wpPos, cfg.m_fCqbClearRadius);
		if (!building)
		{
			int collected = 0;
			if (m_aDCO_CqbBuildings)
				collected = m_aDCO_CqbBuildings.Count();
			Print(string.Format("[DCO-GM] CQB clear failed: no building within %1m of waypoint %2 (candidates=%3)",
				cfg.m_fCqbClearRadius, wpPos, collected), LogLevel.WARNING);
			DCO_WaypointIntentUtil.Complete(this);
			m_DCO_CqbJob = null;
			m_eDCO_CqbState = EDCO_CqbState.IDLE;
			return;
		}
		Print(string.Format("[DCO-GM] CQB clear engaged: group=%1 waypoint=%2 building=%3", m_Owner, wpPos, building.GetOrigin()), LogLevel.NORMAL);

		DCO_CqbRegistry.Get().Claim(building, m_Owner);
		m_DCO_CqbJob = new DCO_CqbJob();
		m_DCO_CqbJob.m_Building = building;
		m_DCO_CqbJob.m_fSurveyStart = now;

		DCO_WaypointIntentUtil.ReleaseWaypointMovement(this);
		m_bDCO_CqbOwnsMove = true;

		DCO_CqbPlan cached = DCO_CqbRegistry.Get().GetPlan(building);
		if (cached && !cached.m_aSectors.IsEmpty())
		{
			m_DCO_CqbJob.m_Plan = cached.DeepCopy();
			if (!DCO_CqbBuildTeams())
			{
				DCO_CqbFinishNoInterior(leader, "no direct non-player AI available for the entry element");
				return;
			}
			m_eDCO_CqbState = EDCO_CqbState.CLEAR;
			DCO_CqbLog(string.Format("[DCO-WPI] CQB plan: cache hit at %1 - %2 sectors / %3 room cells; entry team=%4 security=%5",
				building.GetOrigin(), cached.m_aSectors.Count(), cached.RoomCellCount(),
				m_DCO_CqbJob.m_aEntryTeam.Count(), m_DCO_CqbJob.m_aSecurityTeam.Count()));
			DCO_CqbClearStep(world, leader, cfg, now);
			return;
		}

		m_eDCO_CqbState = EDCO_CqbState.SURVEY;
		vector bMins;
		vector bMaxs;
		building.GetBounds(bMins, bMaxs);
		DCO_CqbLog(string.Format("[DCO-WPI] CQB: target selected at %1 (bounds %2 x %3 x %4); marker movement released, interior survey started",
			building.GetOrigin(), bMaxs[0] - bMins[0], bMaxs[1] - bMins[1], bMaxs[2] - bMins[2]));

		// Start streaming/surveying on the selection tick instead of spending one throttle interval idling at the marker handoff.
		DCO_CqbSurvey(leader, lead, cfg, now);
	}

	// SURVEY: wait for the navmesh tiles, then build the sector plan, cache it, and immediately issue the first sector.
	protected void DCO_CqbSurvey(IEntity leader, vector lead, DCO_CqbClearSettings cfg, float now)
	{
		if (!m_DCO_CqbJob || !m_DCO_CqbJob.m_Building)
		{
			m_eDCO_CqbState = EDCO_CqbState.SELECT;
			return;
		}
		DCO_WaypointIntentUtil.ReleaseWaypointMovement(this);

		AIPathfindingComponent pf = DCO_CqbGetPathfinding(leader);
		array<vector> rawCells = {};
		array<bool> rawDoors = {};
		EDCO_CqbSurveyResult result = DCO_CqbClearUtil.CollectInteriorCells(pf, m_DCO_CqbJob.m_Building, rawCells, rawDoors);
		if (result == EDCO_CqbSurveyResult.WAITING)
		{
			if ((now - m_DCO_CqbJob.m_fSurveyStart) < cfg.m_fCqbApproachTimeoutSec * 1000.0)
				return;
			DCO_CqbFinishNoInterior(leader, "navmesh survey timed out");
			return;
		}

		if (result == EDCO_CqbSurveyResult.INVALID)
		{
			DCO_CqbFinishNoInterior(leader, "missing pathfinding/navmesh");
			return;
		}

		DCO_CqbPlan plan = new DCO_CqbPlan();
		DCO_CqbClearUtil.BuildPlan(pf, rawCells, rawDoors, lead, plan);
		if (plan.m_aSectors.IsEmpty() || plan.RoomCellCount() <= 0)
		{
			DCO_CqbFinishNoInterior(leader, "no floor-proven interior navmesh cells");
			return;
		}

		DCO_CqbRegistry.Get().StorePlan(m_DCO_CqbJob.m_Building, plan.DeepCopy());
		m_DCO_CqbJob.m_Plan = plan;
		if (!DCO_CqbBuildTeams())
		{
			DCO_CqbFinishNoInterior(leader, "no direct non-player AI available for the entry element");
			return;
		}

		m_eDCO_CqbState = EDCO_CqbState.CLEAR;
		for (int s = 0; s < plan.m_aSectors.Count(); s++)
		{
			DCO_CqbSector sector = plan.m_aSectors[s];
			DCO_CqbLog(string.Format("[DCO-WPI] CQB plan: sector %1/%2 floor=%3 cells=%4 portals=%5 entry=%6",
				s + 1, plan.m_aSectors.Count(), sector.m_iFloor, sector.m_aCellIdx.Count(),
				sector.m_aPortalIdx.Count(), sector.m_iEntryPortal));
		}
		DCO_CqbLog(string.Format("[DCO-WPI] CQB: survey READY - %1 sectors / %2 room cells; entry team=%3 security=%4",
			plan.m_aSectors.Count(), plan.RoomCellCount(),
			m_DCO_CqbJob.m_aEntryTeam.Count(), m_DCO_CqbJob.m_aSecurityTeam.Count()));
		DCO_CqbClearStep(GetGame().GetWorld(), leader, cfg, now);
	}

	protected void DCO_CqbFinishNoInterior(IEntity leader, string reason)
	{
		if (!m_DCO_CqbJob || !m_DCO_CqbJob.m_Building)
			return;
		Print(string.Format("[DCO-GM] CQB clear failed: %1 at building %2",
			reason, m_DCO_CqbJob.m_Building.GetOrigin()), LogLevel.WARNING);
		DCO_CqbSetPerception(1.0);
		DCO_CqbRegistry.Get().Release(m_DCO_CqbJob.m_Building);
		DCO_WaypointIntentUtil.Complete(this);
		m_DCO_CqbJob = null;
		m_eDCO_CqbState = EDCO_CqbState.IDLE;
		m_bDCO_CqbOwnsMove = false;
	}

	protected bool DCO_CqbCanUseAgent(AIAgent agent)
	{
		if (!agent || agent.GetParentGroup() != m_Owner)
			return false;
		IEntity entity = agent.GetControlledEntity();
		return entity && !DCO_PlayerUtil.IsPlayer(entity);
	}

	protected bool DCO_CqbBuildTeams()
	{
		if (!m_DCO_CqbJob)
			return false;
		m_DCO_CqbJob.m_aFireteamLocks.Clear();
		m_DCO_CqbJob.m_aEntryTeam.Clear();
		m_DCO_CqbJob.m_aSecurityTeam.Clear();

		array<SCR_AIGroupFireteam> fireTeams = {};
		if (m_FireteamMgr)
			m_FireteamMgr.GetFreeFireteams(fireTeams, SCR_AIGroupFireteam);
		foreach (SCR_AIGroupFireteam fireTeam : fireTeams)
		{
			if (m_DCO_CqbJob.m_aEntryTeam.Count() >= DCO_CQB_ENTRY_TEAM_SIZE)
				break;
			if (!fireTeam)
				continue;
			SCR_AIGroupFireteamLock fireTeamLock = fireTeam.TryLock();
			if (!fireTeamLock)
				continue;
			int before = m_DCO_CqbJob.m_aEntryTeam.Count();
			array<AIAgent> members = {};
			fireTeam.GetMembers(members);
			foreach (AIAgent member : members)
			{
				if (!DCO_CqbCanUseAgent(member))
					continue;
				if (m_DCO_CqbJob.m_aEntryTeam.Find(member) >= 0)
					continue;
				m_DCO_CqbJob.m_aEntryTeam.Insert(member);
				if (m_DCO_CqbJob.m_aEntryTeam.Count() >= DCO_CQB_ENTRY_TEAM_SIZE)
					break;
			}
			if (m_DCO_CqbJob.m_aEntryTeam.Count() > before)
				m_DCO_CqbJob.m_aFireteamLocks.Insert(fireTeamLock);
			// A fireteam that contributed nothing drops its lock here and stays free.
		}

		if (m_DCO_CqbJob.m_aEntryTeam.Count() < DCO_CQB_ENTRY_TEAM_SIZE)
		{
			array<AIAgent> groupAgents = {};
			m_Owner.GetAgents(groupAgents);
			foreach (AIAgent groupAgent : groupAgents)
			{
				if (!DCO_CqbCanUseAgent(groupAgent))
					continue;
				if (m_DCO_CqbJob.m_aEntryTeam.Find(groupAgent) >= 0)
					continue;
				m_DCO_CqbJob.m_aEntryTeam.Insert(groupAgent);
				if (m_DCO_CqbJob.m_aEntryTeam.Count() >= DCO_CQB_ENTRY_TEAM_SIZE)
					break;
			}
		}

		array<AIAgent> allAgents = {};
		m_Owner.GetAgents(allAgents);
		foreach (AIAgent agent : allAgents)
		{
			if (!DCO_CqbCanUseAgent(agent) || m_DCO_CqbJob.m_aEntryTeam.Find(agent) >= 0)
				continue;
			m_DCO_CqbJob.m_aSecurityTeam.Insert(agent);
		}
		return !m_DCO_CqbJob.m_aEntryTeam.IsEmpty();
	}

	// Native Move owns path planning and navlinks, while this state machine owns physical arrival and the subsequent search.
	protected void DCO_CqbMoveMember(AIAgent agent, vector pos)
	{
		if (!agent)
			return;
		IEntity entity = agent.GetControlledEntity();
		if (!entity || DCO_PlayerUtil.IsPlayer(entity))
			return;
		DCO_CqbReadyMember(agent);
		SCR_AIMessage_Move msg = SCR_AIMessage_Move.Create(null, pos, EMovementType.WALK, false, null);
		if (msg)
			m_Mailbox.RequestBroadcast(msg, agent);
	}

	// CQB is a deliberate combat movement state: stand and keep the weapon raised.
	protected void DCO_CqbReadyMember(AIAgent agent)
	{
		if (!agent)
			return;
		IEntity entity = agent.GetControlledEntity();
		if (!entity || DCO_PlayerUtil.IsPlayer(entity))
			return;
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(
			entity.FindComponent(SCR_CharacterControllerComponent));
		if (controller)
		{
			SCR_AIStanceHandling.SetStance(controller, ECharacterStance.STAND);
			controller.SetWeaponRaised(true);
		}
	}

	protected bool DCO_CqbHasLiveContact()
	{
		vector threatPos;
		return DCO_ContactUtil.GetLiveThreatNear(m_Perception, vector.Zero, -1,
			DCO_ContactUtil.FRESH_CONTACT_S, threatPos);
	}

	protected void DCO_CqbLookAt(AIAgent agent, vector focus, float duration)
	{
		if (!DCO_CqbCanUseAgent(agent))
			return;
		DCO_CqbReadyMember(agent);
		if (DCO_CqbHasLiveContact())
			return;
		SCR_ChimeraAIAgent chimera = SCR_ChimeraAIAgent.Cast(agent);
		if (chimera && chimera.m_UtilityComponent)
			chimera.m_UtilityComponent.LookAt(focus + Vector(0, 1.4, 0), duration);
	}

	// Members outside the entry element are security, not extra movers.
	protected void DCO_CqbCoverSecurity(vector focus, float duration)
	{
		if (!m_DCO_CqbJob)
			return;
		foreach (AIAgent security : m_DCO_CqbJob.m_aSecurityTeam)
			DCO_CqbLookAt(security, focus, duration);
	}

	protected void DCO_CqbSetPerception(float factor)
	{
		if (!m_DCO_CqbJob || factor <= 0)
			return;
		for (int t = 0; t < 2; t++)
		{
			array<AIAgent> team = m_DCO_CqbJob.m_aEntryTeam;
			if (t == 1)
				team = m_DCO_CqbJob.m_aSecurityTeam;
			foreach (AIAgent agent : team)
			{
				if (!DCO_CqbCanUseAgent(agent))
					continue;
				IEntity entity = agent.GetControlledEntity();
				SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(entity.FindComponent(SCR_AICombatComponent));
				if (!combat)
					continue;
				combat.SetPerceptionFactor(factor);
			}
		}
	}

	protected void DCO_CqbReportClear()
	{
		if (!m_DCO_CqbJob)
			return;
		for (int i = 0; i < m_DCO_CqbJob.m_iReleased && i < m_DCO_CqbJob.m_aActiveEntry.Count(); i++)
		{
			AIAgent speaker = m_DCO_CqbJob.m_aActiveEntry[i];
			if (!DCO_CqbCanUseAgent(speaker))
				continue;
			SCR_AICommsHandler commsHandler = SCR_AISoundHandling.FindCommsHandler(speaker);
			if (!commsHandler || commsHandler.CanBypass())
				return;
			SCR_AITalkRequest rq = new SCR_AITalkRequest(ECommunicationType.REPORT_CLEAR, null, vector.Zero, 0,
				false, false, SCR_EAITalkRequestPreset.MEDIUM);
			commsHandler.AddRequest(rq);
			return;
		}
	}

	protected DCO_CqbSector DCO_CqbActiveSector()
	{
		if (!m_DCO_CqbJob || !m_DCO_CqbJob.m_Plan)
			return null;
		if (!m_DCO_CqbJob.m_Plan.m_aSectors.IsIndexValid(m_DCO_CqbJob.m_iActiveSector))
			return null;
		return m_DCO_CqbJob.m_Plan.m_aSectors[m_DCO_CqbJob.m_iActiveSector];
	}

	// Next sector to clear.
	protected int DCO_CqbFindPendingSector()
	{
		if (!m_DCO_CqbJob || !m_DCO_CqbJob.m_Plan)
			return -1;
		int first = -1;
		for (int i = 0; i < m_DCO_CqbJob.m_Plan.m_aSectors.Count(); i++)
		{
			DCO_CqbSector sector = m_DCO_CqbJob.m_Plan.m_aSectors[i];
			if (!sector || sector.m_eState != EDCO_CqbSectorState.PENDING)
				continue;
			if (first < 0)
				first = i;
			vector threatPos;
			if (DCO_CqbClearUtil.GetLiveThreatInSector(m_Perception, sector, DCO_CQB_THREAT_MARGIN_M, threatPos))
			{
				DCO_CqbLog(string.Format("[DCO-WPI] CQB: live contact in pending sector %1 - it jumps the clear order",
					i + 1));
				return i;
			}
		}
		return first;
	}

	protected int DCO_CqbMemberCell(DCO_CqbSector sector, AIAgent member)
	{
		if (!sector || !member || !m_DCO_CqbJob || !m_DCO_CqbJob.m_Plan)
			return -1;
		foreach (int cellIdx : sector.m_aCellIdx)
		{
			DCO_CqbCell cell = m_DCO_CqbJob.m_Plan.m_aCells[cellIdx];
			if (cell && cell.m_eState == EDCO_CqbCellState.MOVING && cell.m_Assignee == member)
				return cellIdx;
		}
		return -1;
	}

	protected int DCO_CqbNextFreeCell(DCO_CqbSector sector)
	{
		if (!sector || !m_DCO_CqbJob || !m_DCO_CqbJob.m_Plan)
			return -1;
		vector threatPos;
		if (DCO_CqbClearUtil.GetLiveThreatInSector(m_Perception, sector, DCO_CQB_THREAT_MARGIN_M, threatPos))
		{
			int bestIdx = -1;
			float bestSq = 1000000000.0;
			foreach (int threatCellIdx : sector.m_aCellIdx)
			{
				DCO_CqbCell threatCell = m_DCO_CqbJob.m_Plan.m_aCells[threatCellIdx];
				if (!threatCell || threatCell.m_eState != EDCO_CqbCellState.PENDING)
					continue;
				float dSq = vector.DistanceSq(threatCell.m_vPos, threatPos);
				if (bestIdx < 0 || dSq < bestSq)
				{
					bestIdx = threatCellIdx;
					bestSq = dSq;
				}
			}
			if (bestIdx >= 0)
				return bestIdx;
		}
		foreach (int cellIdx : sector.m_aCellIdx)
		{
			DCO_CqbCell cell = m_DCO_CqbJob.m_Plan.m_aCells[cellIdx];
			if (cell && cell.m_eState == EDCO_CqbCellState.PENDING)
				return cellIdx;
		}
		return -1;
	}

	protected void DCO_CqbAssignCellTo(AIAgent member, int cellIdx, float now)
	{
		DCO_CqbCell cell = m_DCO_CqbJob.m_Plan.m_aCells[cellIdx];
		cell.m_Assignee = member;
		cell.m_eState = EDCO_CqbCellState.MOVING;
		cell.m_fIssuedTime = now;
		cell.m_fLastProgressTime = now;
		cell.m_fLastDoorAssistTime = -1;
		cell.m_fBestDistSq = 1000000000.0;
		DCO_CqbMoveMember(member, cell.m_vPos);
	}

	protected void DCO_CqbReleaseNext(DCO_CqbSector sector, float now)
	{
		if (!m_DCO_CqbJob || m_DCO_CqbJob.m_iReleased >= m_DCO_CqbJob.m_aActiveEntry.Count()
			|| m_DCO_CqbJob.m_iReleased >= m_DCO_CqbJob.m_iEntryCap)
			return;
		AIAgent member = m_DCO_CqbJob.m_aActiveEntry[m_DCO_CqbJob.m_iReleased];
		m_DCO_CqbJob.m_iReleased++;
		m_DCO_CqbJob.m_fLastReleaseTime = now;
		if (!DCO_CqbCanUseAgent(member))
			return;
		int cellIdx = DCO_CqbNextFreeCell(sector);
		if (cellIdx >= 0)
		{
			DCO_CqbAssignCellTo(member, cellIdx, now);
			DCO_CqbLog(string.Format("[DCO-WPI] CQB: sector %1 member %2/%3 released into the room",
				m_DCO_CqbJob.m_iActiveSector + 1, m_DCO_CqbJob.m_iReleased,
				m_DCO_CqbJob.m_aActiveEntry.Count()));
		}
		// No free cell: the member holds as in-room cover; smart looks pick him up.
	}

	protected bool DCO_CqbIssueSector(BaseWorld world, int sectorIdx, DCO_CqbClearSettings cfg, float now)
	{
		DCO_CqbPlan plan = m_DCO_CqbJob.m_Plan;
		DCO_CqbSector sector = plan.m_aSectors[sectorIdx];
		if (!sector)
			return false;

		m_DCO_CqbJob.m_aActiveEntry.Clear();
		foreach (AIAgent candidate : m_DCO_CqbJob.m_aEntryTeam)
		{
			if (DCO_CqbCanUseAgent(candidate))
				m_DCO_CqbJob.m_aActiveEntry.Insert(candidate);
		}
		if (m_DCO_CqbJob.m_aActiveEntry.IsEmpty())
			return false;

		m_DCO_CqbJob.m_iActiveSector = sectorIdx;
		m_DCO_CqbJob.m_iReleased = 0;
		m_DCO_CqbJob.m_fSectorIssueTime = now;
		m_DCO_CqbJob.m_fLastReleaseTime = now;
		m_DCO_CqbJob.m_aStackSlots.Clear();
		sector.m_fDwellStart = -1;

		int roomCells = sector.m_aCellIdx.Count();
		float roomSpanX = sector.m_vMaxs[0] - sector.m_vMins[0];
		float roomSpanZ = sector.m_vMaxs[2] - sector.m_vMins[2];
		m_DCO_CqbJob.m_iEntryCap = DCO_CQB_ENTRY_TEAM_SIZE;
		if (roomCells <= 1)
			m_DCO_CqbJob.m_iEntryCap = 1;
		else if (roomCells <= 3 || roomSpanX * roomSpanZ < DCO_CQB_SMALL_ROOM_M2)
			m_DCO_CqbJob.m_iEntryCap = 2;

		if (sector.m_iEntryPortal >= 0)
		{
			vector portalPos = plan.m_aCells[sector.m_iEntryPortal].m_vPos;
			vector inward = DCO_CqbClearUtil.SectorCentroid(plan, sectorIdx) - portalPos;
			inward[1] = 0;
			if (inward.LengthSq() < 0.01 && m_DCO_CqbJob.m_Building)
			{
				vector transform[4];
				m_DCO_CqbJob.m_Building.GetWorldTransform(transform);
				inward = transform[2];
				inward[1] = 0;
			}
			if (inward.LengthSq() > 0.01)
				inward.Normalize();
			vector side = Vector(-inward[2], 0, inward[0]);

			// Man 1 near-left, man 2 near-right, pairs alternating deeper: the grid approximation of doctrinal points of domination.
			DCO_CqbClearUtil.OrderCellsForBreach(plan, sector, portalPos, inward, side);

			vector stackBase = portalPos;
			float bestBehind = 0;
			float pairSq = DCO_CQB_STACK_PAIR_M * DCO_CQB_STACK_PAIR_M;
			for (int dcell = 0; dcell < plan.m_aCells.Count(); dcell++)
			{
				DCO_CqbCell doorCell = plan.m_aCells[dcell];
				if (!doorCell || !doorCell.m_bDoor)
					continue;
				if (vector.DistanceSq(doorCell.m_vPos, portalPos) > pairSq)
					continue;
				vector relDoor = doorCell.m_vPos - portalPos;
				float behind = -(relDoor[0] * inward[0] + relDoor[2] * inward[2]);
				if (behind > bestBehind)
				{
					bestBehind = behind;
					stackBase = doorCell.m_vPos;
				}
			}

			AIPathfindingComponent pf = DCO_CqbGetPathfinding(m_Owner.GetLeaderEntity());
			float sideSign = 1.0;
			for (int i = 0; i < m_DCO_CqbJob.m_aActiveEntry.Count(); i++)
			{
				float lateral = DCO_CQB_STACK_LATERAL_M + DCO_CQB_STACK_STEP_M * (i / 2);
				vector raw = stackBase - inward * DCO_CQB_STACK_BACK_M + side * (lateral * sideSign);
				sideSign = -sideSign;
				vector slot = raw;
				vector snapped;
				if (pf && pf.GetClosestPositionOnNavmesh(raw, Vector(1.5, 2.0, 1.5), snapped))
					slot = snapped;
				m_DCO_CqbJob.m_aStackSlots.Insert(slot);
				DCO_CqbMoveMember(m_DCO_CqbJob.m_aActiveEntry[i], slot);
			}

			sector.m_eState = EDCO_CqbSectorState.STACK;
			m_DCO_CqbJob.m_fBreachTime = -1;
			DCO_CqbLog(string.Format("[DCO-WPI] CQB: sector %1/%2 STACK - entry=%3 security=%4 cells=%5 (door closed until formation)",
				sectorIdx + 1, plan.m_aSectors.Count(), m_DCO_CqbJob.m_aActiveEntry.Count(),
				m_DCO_CqbJob.m_aSecurityTeam.Count(), sector.m_aCellIdx.Count()));
		}
		else
		{
			sector.m_eState = EDCO_CqbSectorState.FLOW;
			DCO_CqbReleaseNext(sector, now);
			DCO_CqbLog(string.Format("[DCO-WPI] CQB: sector %1/%2 FLOW direct (open transition) - entry=%3 cells=%4",
				sectorIdx + 1, plan.m_aSectors.Count(), m_DCO_CqbJob.m_aActiveEntry.Count(),
				sector.m_aCellIdx.Count()));
		}
		return true;
	}

	protected void DCO_CqbSkipSector(int sectorIdx, string reason)
	{
		DCO_CqbPlan plan = m_DCO_CqbJob.m_Plan;
		DCO_CqbSector sector = plan.m_aSectors[sectorIdx];
		if (!sector || sector.m_eState == EDCO_CqbSectorState.SKIPPED)
			return;
		foreach (int cellIdx : sector.m_aCellIdx)
		{
			DCO_CqbCell cell = plan.m_aCells[cellIdx];
			if (cell && cell.m_eState != EDCO_CqbCellState.CLEARED && cell.m_eState != EDCO_CqbCellState.SKIPPED)
			{
				cell.m_eState = EDCO_CqbCellState.SKIPPED;
				cell.m_Assignee = null;
				m_DCO_CqbJob.m_iSkipped++;
			}
		}
		sector.m_eState = EDCO_CqbSectorState.SKIPPED;
		if (m_DCO_CqbJob.m_iActiveSector == sectorIdx)
			m_DCO_CqbJob.m_iActiveSector = -1;
		Print(string.Format("[DCO-WPI] CQB: sector %1/%2 SKIPPED (%3); skipped cells=%4",
			sectorIdx + 1, plan.m_aSectors.Count(), reason, m_DCO_CqbJob.m_iSkipped), LogLevel.WARNING);
	}

	protected int DCO_CqbCoveragePercent()
	{
		if (!m_DCO_CqbJob || !m_DCO_CqbJob.m_Plan)
			return 0;
		int total = m_DCO_CqbJob.m_Plan.RoomCellCount();
		if (total <= 0)
			return 100;
		return (m_DCO_CqbJob.m_iCleared * 100) / total;
	}

	// One member's cell leg: progress, arrival, stall -> door assist -> one retry -> skip.
	protected void DCO_CqbUpdateCell(BaseWorld world, DCO_CqbSector sector, int cellIdx, DCO_CqbClearSettings cfg, float now)
	{
		DCO_CqbPlan plan = m_DCO_CqbJob.m_Plan;
		DCO_CqbCell cell = plan.m_aCells[cellIdx];
		if (!cell || cell.m_eState != EDCO_CqbCellState.MOVING)
			return;
		AIAgent member = cell.m_Assignee;
		if (!DCO_CqbCanUseAgent(member))
		{
			// Assignee died or left the group: put the cell back in the pool.
			cell.m_eState = EDCO_CqbCellState.PENDING;
			cell.m_Assignee = null;
			return;
		}
		IEntity entity = member.GetControlledEntity();
		DCO_CqbReadyMember(member);

		float distSq = vector.DistanceSq(entity.GetOrigin(), cell.m_vPos);
		AIBaseMovementComponent movement = member.GetMovementComponent();
		bool completed = movement && movement.HasCompletedRequest(true);
		if (distSq + DCO_CQB_PROGRESS_EPS_SQ < cell.m_fBestDistSq)
		{
			cell.m_fBestDistSq = distSq;
			cell.m_fLastProgressTime = now;
		}

		float arriveSq = DCO_CQB_CELL_ARRIVE_M * DCO_CQB_CELL_ARRIVE_M;
		float completedNearSq = DCO_CQB_COMPLETED_NEAR_M * DCO_CQB_COMPLETED_NEAR_M;
		if (distSq <= arriveSq || (completed && distSq <= completedNearSq))
		{
			cell.m_eState = EDCO_CqbCellState.CLEARED;
			cell.m_Assignee = null;
			m_DCO_CqbJob.m_iCleared++;
			DCO_CqbLog(string.Format("[DCO-WPI] CQB: s%1 cell CLEAR (%2/%3 room cells)",
				m_DCO_CqbJob.m_iActiveSector + 1, m_DCO_CqbJob.m_iCleared,
				plan.RoomCellCount()));
			int nextIdx = DCO_CqbNextFreeCell(sector);
			if (nextIdx >= 0)
				DCO_CqbAssignCellTo(member, nextIdx, now);
			return;
		}

		float stalledMs = now - cell.m_fLastProgressTime;
		if (stalledMs >= DCO_CQB_DOOR_STALL_MS
			&& (cell.m_fLastDoorAssistTime < 0 || (now - cell.m_fLastDoorAssistTime) >= DCO_CQB_DOOR_STALL_MS))
		{
			int doorsOpened = DCO_CqbClearUtil.OpenNearbyDoors(world, member, DCO_CQB_DOOR_ASSIST_M);
			cell.m_fLastDoorAssistTime = now;
			if (doorsOpened > 0)
				DCO_CqbLog(string.Format("[DCO-WPI] CQB: sector %1 door assist activated %2 nearby door action(s)",
					m_DCO_CqbJob.m_iActiveSector + 1, doorsOpened));
		}

		if (stalledMs >= DCO_CQB_RETRY_STALL_MS)
		{
			if (cell.m_iRetries < DCO_CQB_MAX_RETRIES)
			{
				cell.m_iRetries++;
				cell.m_fLastProgressTime = now;
				cell.m_fBestDistSq = 1000000000.0;
				DCO_CqbMoveMember(member, cell.m_vPos);
				Print(string.Format("[DCO-WPI] CQB: sector %1 cell made no path progress; Move retry %2/%3",
					m_DCO_CqbJob.m_iActiveSector + 1, cell.m_iRetries, DCO_CQB_MAX_RETRIES), LogLevel.WARNING);
			}
			else
			{
				cell.m_eState = EDCO_CqbCellState.SKIPPED;
				cell.m_Assignee = null;
				m_DCO_CqbJob.m_iSkipped++;
				Print(string.Format("[DCO-WPI] CQB: sector %1 cell classified unreachable; skipped=%2",
					m_DCO_CqbJob.m_iActiveSector + 1, m_DCO_CqbJob.m_iSkipped), LogLevel.WARNING);
				int nextIdx2 = DCO_CqbNextFreeCell(sector);
				if (nextIdx2 >= 0)
					DCO_CqbAssignCellTo(member, nextIdx2, now);
			}
		}
	}

	protected void DCO_CqbSmartLooks(DCO_CqbSector sector, DCO_CqbClearSettings cfg, float now)
	{
		DCO_CqbPlan plan = m_DCO_CqbJob.m_Plan;
		float duration = cfg.m_fCqbClearCheckSec + 0.35;

		array<vector> targets = {};
		// "Do not bypass": this room's OTHER doorways are watched first - nobody walks past an uncovered door.
		foreach (int otherPortal : sector.m_aPortalIdx)
		{
			if (otherPortal != sector.m_iEntryPortal)
				targets.Insert(plan.m_aCells[otherPortal].m_vPos);
		}
		int nextSector = DCO_CqbFindPendingSector();
		if (nextSector >= 0)
		{
			DCO_CqbSector next = plan.m_aSectors[nextSector];
			if (next.m_iEntryPortal >= 0)
				targets.Insert(plan.m_aCells[next.m_iEntryPortal].m_vPos);
			else
				targets.Insert(DCO_CqbClearUtil.SectorCentroid(plan, nextSector));
		}
		foreach (int cellIdx : sector.m_aCellIdx)
		{
			DCO_CqbCell cell = plan.m_aCells[cellIdx];
			if (cell && cell.m_eState == EDCO_CqbCellState.PENDING)
				targets.Insert(cell.m_vPos);
		}
		if (targets.IsEmpty() && m_DCO_CqbJob.m_Building)
		{
			// Everything owned/resolved: sweep building-aligned axes from the middle.
			vector transform[4];
			m_DCO_CqbJob.m_Building.GetWorldTransform(transform);
			vector centroid = DCO_CqbClearUtil.SectorCentroid(plan, m_DCO_CqbJob.m_iActiveSector);
			targets.Insert(centroid + transform[2] * 7.0);
			targets.Insert(centroid + transform[0] * 7.0);
			targets.Insert(centroid - transform[2] * 7.0);
			targets.Insert(centroid - transform[0] * 7.0);
		}
		if (targets.IsEmpty())
			return;

		vector portalFocus = DCO_CqbClearUtil.SectorCentroid(plan, m_DCO_CqbJob.m_iActiveSector);
		if (sector.m_iEntryPortal >= 0)
			portalFocus = plan.m_aCells[sector.m_iEntryPortal].m_vPos;

		int slot = 0;
		for (int m = 0; m < m_DCO_CqbJob.m_aActiveEntry.Count(); m++)
		{
			AIAgent member = m_DCO_CqbJob.m_aActiveEntry[m];
			if (!DCO_CqbCanUseAgent(member))
				continue;
			if (m >= m_DCO_CqbJob.m_iReleased)
			{
				// Still stacked: watch the threshold, not the far rooms.
				DCO_CqbLookAt(member, portalFocus, duration);
				continue;
			}
			if (DCO_CqbMemberCell(sector, member) >= 0)
				continue;	// busy moving to a cell; native gaze handles the path.
			DCO_CqbLookAt(member, targets[(sector.m_iScanStep + slot) % targets.Count()], duration);
			slot++;
		}
		sector.m_iScanStep++;
	}

	protected void DCO_CqbUpdateSector(BaseWorld world, DCO_CqbClearSettings cfg, float now)
	{
		DCO_CqbPlan plan = m_DCO_CqbJob.m_Plan;
		DCO_CqbSector sector = DCO_CqbActiveSector();
		if (!sector)
		{
			m_DCO_CqbJob.m_iActiveSector = -1;
			return;
		}
		int sectorIdx = m_DCO_CqbJob.m_iActiveSector;

		// Prune the active roster to members still usable; abandon the sector only when nobody is left even after a team rebuild.
		int usable = 0;
		foreach (AIAgent rosterMember : m_DCO_CqbJob.m_aActiveEntry)
		{
			if (DCO_CqbCanUseAgent(rosterMember))
				usable++;
		}
		if (usable == 0)
		{
			if (DCO_CqbBuildTeams() && DCO_CqbIssueSector(world, sectorIdx, cfg, now))
				return;
			DCO_CqbSkipSector(sectorIdx, "no living entry member");
			return;
		}

		vector securityFocus = DCO_CqbClearUtil.SectorCentroid(plan, sectorIdx);
		if (sector.m_iEntryPortal >= 0)
			securityFocus = plan.m_aCells[sector.m_iEntryPortal].m_vPos;
		DCO_CqbCoverSecurity(securityFocus, cfg.m_fCqbClearCheckSec + 0.35);

		vector localThreat;
		bool sectorThreat = DCO_CqbClearUtil.GetLiveThreatInSector(m_Perception, sector,
			DCO_CQB_THREAT_MARGIN_M, localThreat);

		if (sector.m_eState == EDCO_CqbSectorState.STACK)
		{
			DCO_CqbUpdateStack(world, cfg, now, plan, sector, sectorIdx);
			return;
		}
		if (sector.m_eState == EDCO_CqbSectorState.FLOW)
		{
			DCO_CqbUpdateFlow(world, cfg, now, plan, sector, sectorIdx, sectorThreat);
			return;
		}
		if (sector.m_eState == EDCO_CqbSectorState.DWELL)
			DCO_CqbUpdateDwell(cfg, now, plan, sector, sectorIdx, sectorThreat);
	}

	protected void DCO_CqbUpdateStack(BaseWorld world, DCO_CqbClearSettings cfg, float now,
		DCO_CqbPlan plan, DCO_CqbSector sector, int sectorIdx)
		{
			foreach (AIAgent stacked : m_DCO_CqbJob.m_aActiveEntry)
				DCO_CqbReadyMember(stacked);

			bool formed = now - m_DCO_CqbJob.m_fSectorIssueTime >= DCO_CQB_STACK_TIMEOUT_MS;
			if (!formed)
			{
				formed = true;
				float readySq = DCO_CQB_STACK_READY_M * DCO_CQB_STACK_READY_M;
				for (int slotIdx = 0; slotIdx < m_DCO_CqbJob.m_aActiveEntry.Count()
					&& slotIdx < m_DCO_CqbJob.m_aStackSlots.Count(); slotIdx++)
				{
					AIAgent stackMember = m_DCO_CqbJob.m_aActiveEntry[slotIdx];
					if (!DCO_CqbCanUseAgent(stackMember))
						continue;
					if (vector.DistanceSq(stackMember.GetControlledEntity().GetOrigin(),
						m_DCO_CqbJob.m_aStackSlots[slotIdx]) > readySq)
					{
						formed = false;
						break;
					}
				}
			}

			if (formed)
			{
				// Breach order: the point man opens THIS door only.
				AIAgent point = m_DCO_CqbJob.m_aActiveEntry[0];
				int doorsOpened = 0;
				if (sector.m_iEntryPortal >= 0 && DCO_CqbCanUseAgent(point))
					doorsOpened = DCO_CqbClearUtil.OpenDoorsAtPosition(world, point.GetControlledEntity(),
						plan.m_aCells[sector.m_iEntryPortal].m_vPos, DCO_CQB_BREACH_DOOR_M);
				m_DCO_CqbJob.m_fBreachTime = now;
				sector.m_eState = EDCO_CqbSectorState.FLOW;
				DCO_CqbLog(string.Format("[DCO-WPI] CQB: sector %1/%2 BREACH ordered - stack formed, door action(s)=%3",
					sectorIdx + 1, plan.m_aSectors.Count(), doorsOpened));
			}
			else
			{
				DCO_CqbSmartLooks(sector, cfg, now);
			}
			return;
		}

	protected void DCO_CqbUpdateFlow(BaseWorld world, DCO_CqbClearSettings cfg, float now,
		DCO_CqbPlan plan, DCO_CqbSector sector, int sectorIdx, bool sectorThreat)
		{
			vector portalPos = vector.Zero;
			bool hasPortal = sector.m_iEntryPortal >= 0;
			if (hasPortal)
				portalPos = plan.m_aCells[sector.m_iEntryPortal].m_vPos;

			if (m_DCO_CqbJob.m_iReleased == 0)
			{
				bool doorReady = !hasPortal
					|| !DCO_CqbClearUtil.AnyClosedDoorAtPosition(world, portalPos, DCO_CQB_BREACH_GATE_M)
					|| (m_DCO_CqbJob.m_fBreachTime >= 0 && (now - m_DCO_CqbJob.m_fBreachTime) >= DCO_CQB_BREACH_TIMEOUT_MS);
				if (doorReady)
				{
					DCO_CqbReleaseNext(sector, now);
					DCO_CqbLog(string.Format("[DCO-WPI] CQB: sector %1/%2 threshold OPEN - point released",
						sectorIdx + 1, plan.m_aSectors.Count()));
				}
			}
			else if (m_DCO_CqbJob.m_iReleased < m_DCO_CqbJob.m_aActiveEntry.Count()
				&& m_DCO_CqbJob.m_iReleased < m_DCO_CqbJob.m_iEntryCap
				&& DCO_CqbNextFreeCell(sector) >= 0)
			{
				AIAgent previous = m_DCO_CqbJob.m_aActiveEntry[m_DCO_CqbJob.m_iReleased - 1];
				bool releaseNext = !DCO_CqbCanUseAgent(previous);
				if (!releaseNext && sectorThreat)
					releaseNext = true;
				if (!releaseNext && (now - m_DCO_CqbJob.m_fLastReleaseTime) >= DCO_CQB_RELEASE_TIMEOUT_MS)
					releaseNext = true;
				if (!releaseNext && DCO_CqbMemberCell(sector, previous) < 0)
					releaseNext = true;
				if (!releaseNext)
				{
					if (hasPortal)
					{
						vector inward = DCO_CqbClearUtil.SectorCentroid(plan, sectorIdx) - portalPos;
						inward[1] = 0;
						if (inward.LengthSq() > 0.01)
							inward.Normalize();
						vector rel = previous.GetControlledEntity().GetOrigin() - portalPos;
						float depth = rel[0] * inward[0] + rel[2] * inward[2];
						releaseNext = depth >= DCO_CQB_FUNNEL_DEPTH_M;
					}
					else
					{
						float releaseSq = DCO_CQB_WINGMAN_RELEASE_M * DCO_CQB_WINGMAN_RELEASE_M;
						int previousCell = DCO_CqbMemberCell(sector, previous);
						releaseNext = vector.DistanceSq(previous.GetControlledEntity().GetOrigin(),
							plan.m_aCells[previousCell].m_vPos) <= releaseSq;
					}
				}
				if (releaseNext)
					DCO_CqbReleaseNext(sector, now);
			}

			// A live fight never burns the stall/retry budget: rooms are skipped for pathing failure, not for time spent shooting.
			if (sectorThreat || DCO_CqbHasLiveContact())
			{
				foreach (int pauseIdx : sector.m_aCellIdx)
				{
					DCO_CqbCell pauseCell = plan.m_aCells[pauseIdx];
					if (pauseCell && pauseCell.m_eState == EDCO_CqbCellState.MOVING)
						pauseCell.m_fLastProgressTime = now;
				}
			}

			foreach (int updateIdx : sector.m_aCellIdx)
				DCO_CqbUpdateCell(world, sector, updateIdx, cfg, now);

			bool anyMoving = false;
			bool anyPending = false;
			foreach (int checkIdx : sector.m_aCellIdx)
			{
				DCO_CqbCell check = plan.m_aCells[checkIdx];
				if (!check)
					continue;
				if (check.m_eState == EDCO_CqbCellState.MOVING)
					anyMoving = true;
				else if (check.m_eState == EDCO_CqbCellState.PENDING)
					anyPending = true;
			}

			if (!anyMoving && anyPending)
			{
				if (m_DCO_CqbJob.m_iReleased == 0)
				{
					DCO_CqbReleaseNext(sector, now);
				}
				else
				{
					AIAgent worker;
					for (int w = 0; w < m_DCO_CqbJob.m_iReleased && w < m_DCO_CqbJob.m_aActiveEntry.Count(); w++)
					{
						if (DCO_CqbCanUseAgent(m_DCO_CqbJob.m_aActiveEntry[w]))
						{
							worker = m_DCO_CqbJob.m_aActiveEntry[w];
							break;
						}
					}
					if (worker)
					{
						int freeIdx = DCO_CqbNextFreeCell(sector);
						if (freeIdx >= 0)
							DCO_CqbAssignCellTo(worker, freeIdx, now);
					}
					else if (m_DCO_CqbJob.m_iReleased >= m_DCO_CqbJob.m_aActiveEntry.Count())
					{
						DCO_CqbSkipSector(sectorIdx, "no usable released member for the remaining cells");
						return;
					}
					else
					{
						DCO_CqbReleaseNext(sector, now);
					}
				}
			}
			else if (!anyMoving && !anyPending)
			{
				sector.m_eState = EDCO_CqbSectorState.DWELL;
				sector.m_fDwellStart = now;
				sector.m_fDwellEnteredTime = now;
				DCO_CqbLog(string.Format("[DCO-WPI] CQB: sector %1/%2 DWELL - all cells resolved",
					sectorIdx + 1, plan.m_aSectors.Count()));
			}

			DCO_CqbSmartLooks(sector, cfg, now);
			return;
		}

	protected void DCO_CqbUpdateDwell(DCO_CqbClearSettings cfg, float now,
		DCO_CqbPlan plan, DCO_CqbSector sector, int sectorIdx, bool sectorThreat)
		{
			if (sectorThreat)
			{
				sector.m_fDwellStart = now;
				sector.m_fDwellEnteredTime = now;	// a live fight freezes the watchdog budget too.
			}

			// The quiet dwell only counts while a member is physically inside the sector box; a room is never certified from the outside.
			bool memberNear = false;
			AIAgent nearest;
			float nearestSq = 1000000000.0;
			vector center = (sector.m_vMins + sector.m_vMaxs) * 0.5;
			foreach (AIAgent dwellMember : m_DCO_CqbJob.m_aActiveEntry)
			{
				if (!DCO_CqbCanUseAgent(dwellMember))
					continue;
				vector pos = dwellMember.GetControlledEntity().GetOrigin();
				if (pos[0] >= sector.m_vMins[0] - DCO_CQB_DWELL_NEAR_M && pos[0] <= sector.m_vMaxs[0] + DCO_CQB_DWELL_NEAR_M
					&& pos[1] >= sector.m_vMins[1] - DCO_CQB_DWELL_NEAR_M && pos[1] <= sector.m_vMaxs[1] + DCO_CQB_DWELL_NEAR_M
					&& pos[2] >= sector.m_vMins[2] - DCO_CQB_DWELL_NEAR_M && pos[2] <= sector.m_vMaxs[2] + DCO_CQB_DWELL_NEAR_M)
					memberNear = true;
				float dSq = vector.DistanceSq(pos, center);
				if (dSq < nearestSq)
				{
					nearestSq = dSq;
					nearest = dwellMember;
				}
			}
			if (!memberNear)
			{
				sector.m_fDwellStart = now;
				if (nearest && nearestSq > DCO_CQB_DWELL_NEAR_M * DCO_CQB_DWELL_NEAR_M * 4)
					DCO_CqbMoveMember(nearest, center);
			}

			bool dwellForced = sector.m_fDwellEnteredTime >= 0
				&& (now - sector.m_fDwellEnteredTime) > DCO_CQB_DWELL_FORCE_MS;
			if (dwellForced)
				Print(string.Format("[DCO-WPI] CQB: sector %1/%2 DWELL watchdog fired (no member could reach the dwell band) - force-resolving",
					sectorIdx + 1, plan.m_aSectors.Count()), LogLevel.WARNING);

			DCO_CqbSmartLooks(sector, cfg, now);
			if (!dwellForced && (now - sector.m_fDwellStart) < cfg.m_fCqbNodeDwellSec * 1000.0)
				return;

			bool anyClearedCell = false;
			foreach (int doneIdx : sector.m_aCellIdx)
			{
				DCO_CqbCell done = plan.m_aCells[doneIdx];
				if (done && done.m_eState == EDCO_CqbCellState.CLEARED)
					anyClearedCell = true;
			}
			if (anyClearedCell)
			{
				sector.m_eState = EDCO_CqbSectorState.CLEARED;
				DCO_CqbReportClear();
				DCO_CqbLog(string.Format("[DCO-WPI] CQB: sector %1/%2 CLEAR - total coverage %3 pct",
					sectorIdx + 1, plan.m_aSectors.Count(), DCO_CqbCoveragePercent()));
			}
			else
			{
				sector.m_eState = EDCO_CqbSectorState.SKIPPED;
				Print(string.Format("[DCO-WPI] CQB: sector %1/%2 resolved with no reachable cell - marked SKIPPED",
					sectorIdx + 1, plan.m_aSectors.Count()), LogLevel.WARNING);
			}
			m_DCO_CqbJob.m_iActiveSector = -1;
		}

	// CLEAR: keep the boost asserted, drive the active sector, and issue the next one only when nothing is active.
	protected void DCO_CqbClearStep(BaseWorld world, IEntity leader, DCO_CqbClearSettings cfg, float now)
	{
		if (!m_DCO_CqbJob || !m_DCO_CqbJob.m_Building || !m_DCO_CqbJob.m_Plan
			|| m_DCO_CqbJob.m_Plan.m_aSectors.IsEmpty())
		{
			m_eDCO_CqbState = EDCO_CqbState.SELECT;
			return;
		}
		DCO_WaypointIntentUtil.ReleaseWaypointMovement(this);
		if (!world)
			return;

		DCO_CqbSetPerception(cfg.m_fCqbPerceptionBoost);

		if (m_DCO_CqbJob.m_iActiveSector >= 0)
		{
			DCO_CqbUpdateSector(world, cfg, now);
			return;
		}

		int nextIdx = DCO_CqbFindPendingSector();
		if (nextIdx < 0)
		{
			DCO_CqbFinishCoverage(leader, cfg, now);
			return;
		}
		if (!DCO_CqbIssueSector(world, nextIdx, cfg, now))
		{
			if (!DCO_CqbBuildTeams() || !DCO_CqbIssueSector(world, nextIdx, cfg, now))
				DCO_CqbSkipSector(nextIdx, "no non-player AI available");
		}
	}

	// All sectors are resolved.
	protected void DCO_CqbFinishCoverage(IEntity leader, DCO_CqbClearSettings cfg, float now)
	{
		DCO_CqbPlan plan = m_DCO_CqbJob.m_Plan;
		vector threatPos;
		bool liveThreat = DCO_CqbClearUtil.GetLiveThreatNearCells(m_Perception, plan.m_aCells,
			DCO_CQB_BUILDING_THREAT_M, threatPos);
		if (liveThreat && m_DCO_CqbJob.m_iRecoveryPasses >= DCO_CQB_MAX_RECOVERY_PASSES)
		{
			liveThreat = false;
			Print(string.Format("[DCO-WPI] CQB: recovery-pass cap (%1) reached with a live threat still near %2 - completing with residual threat",
				DCO_CQB_MAX_RECOVERY_PASSES, threatPos), LogLevel.WARNING);
		}
		if (liveThreat)
		{
			int bestCell = -1;
			float bestSq = 1000000000.0;
			for (int i = 0; i < plan.m_aCells.Count(); i++)
			{
				DCO_CqbCell cell = plan.m_aCells[i];
				if (!cell || cell.m_bDoor || cell.m_iSector < 0)
					continue;
				float distSq = vector.DistanceSq(threatPos, cell.m_vPos);
				if (bestCell < 0 || distSq < bestSq)
				{
					bestCell = i;
					bestSq = distSq;
				}
			}
			if (bestCell >= 0)
			{
				int sectorIdx = plan.m_aCells[bestCell].m_iSector;
				DCO_CqbSector recovery = plan.m_aSectors[sectorIdx];
				foreach (int cellIdx : recovery.m_aCellIdx)
				{
					DCO_CqbCell resetCell = plan.m_aCells[cellIdx];
					if (!resetCell)
						continue;
					if (resetCell.m_eState == EDCO_CqbCellState.CLEARED)
						m_DCO_CqbJob.m_iCleared--;
					else if (resetCell.m_eState == EDCO_CqbCellState.SKIPPED)
						m_DCO_CqbJob.m_iSkipped--;
					resetCell.m_eState = EDCO_CqbCellState.PENDING;
					resetCell.m_Assignee = null;
					resetCell.m_iRetries = 0;
				}
				recovery.m_eState = EDCO_CqbSectorState.PENDING;
				recovery.m_fDwellStart = -1;
				m_DCO_CqbJob.m_iActiveSector = -1;
				m_DCO_CqbJob.m_iRecoveryPasses++;
				DCO_CqbLog(string.Format("[DCO-WPI] CQB: fresh interior contact at %1 - reopening sector %2 (recovery pass %3)",
					threatPos, sectorIdx + 1, m_DCO_CqbJob.m_iRecoveryPasses));
				return;
			}
		}

		IEntity building = m_DCO_CqbJob.m_Building;
		Print(string.Format("[DCO-GM] CQB clear complete: building=%1 coverage=%2 pct unreachable=%3 recoveryPasses=%4",
			building.GetOrigin(), DCO_CqbCoveragePercent(), m_DCO_CqbJob.m_iSkipped, m_DCO_CqbJob.m_iRecoveryPasses), LogLevel.NORMAL);
		DCO_CqbReportClear();
		DCO_CqbSetPerception(1.0);
		DCO_CqbRegistry.Get().MarkCleared(building);
		DCO_WaypointIntentUtil.Complete(this);
		m_DCO_CqbJob = null;
		m_eDCO_CqbState = EDCO_CqbState.IDLE;
		m_bDCO_CqbOwnsMove = false;
	}

	protected void DCO_CqbAbort()
	{
		if (m_DCO_CqbJob)
		{
			DCO_CqbSetPerception(1.0);
			if (m_DCO_CqbJob.m_Building)
				DCO_CqbRegistry.Get().Release(m_DCO_CqbJob.m_Building);
		}
		m_DCO_CqbJob = null;
		m_eDCO_CqbState = EDCO_CqbState.IDLE;
		m_bDCO_CqbOwnsMove = false;
	}

}
