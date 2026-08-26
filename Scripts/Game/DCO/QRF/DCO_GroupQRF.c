modded class SCR_AIGroupUtilityComponent
{
	protected bool	m_bDCO_IsQRFResponder	= false;
	protected float	m_fDCO_QRFRange			= 1500.0;
	protected float	m_fDCO_LastQRFTime		= -1;
	protected bool	m_bDCO_QRFDeployed		= false;

	protected vector	m_vDCO_QRFHoldPos		= vector.Zero;
	protected bool		m_bDCO_HasQRFHold		= false;

	protected static const float DCO_QRF_CHECK_INTERVAL_MS = 5000.0;

	// Shared DCO tick.
	override SCR_AIActionBase EvaluateActivity(out bool restartActivity)
	{
		SCR_AIActionBase action = super.EvaluateActivity(restartActivity);
		DCO_UpdateMorale();
		DCO_UpdateMemberMorale();
		DCO_UpdateEmergencyRearm();
		DCO_UpdateSuppression();
		DCO_UpdateArmorAvoid();
		DCO_UpdateFriendlyFire();
		DCO_UpdateMerge();
		DCO_UpdateAmbush();
		DCO_UpdateDefend();
		DCO_UpdateTacticalCoordinator();
		DCO_UpdateTacticalBrain();
		DCO_UpdateReactToContact();
		DCO_UpdateFormation();
		DCO_UpdateCoverPlacement();
		DCO_UpdateFormationShape();
		DCO_UpdateTacticalPath();
		DCO_UpdateLauncherDiscipline();
		DCO_UpdateVisionLimit();
		DCO_UpdateBaseSettings();
		DCO_UpdateReaction();
		DCO_UpdateIdle();
		DCO_UpdateGMFormation();
		DCO_UpdateGMStance();
		DCO_UpdateCoverStance();
		DCO_UpdateCQB();
		DCO_UpdateCqbClear();
		DCO_UpdateMachineGunner();
		DCO_UpdateQRF();
		return action;
	}

	bool DCO_IsQRFResponder()
	{
		return m_bDCO_IsQRFResponder;
	}

	void DCO_SetQRFResponder(bool enable)
	{
		m_bDCO_IsQRFResponder = enable;
		if (!enable)
			m_bDCO_QRFDeployed = false;
	}

	float DCO_GetQRFRange()
	{
		return m_fDCO_QRFRange;
	}

	void DCO_SetQRFHoldPosition(vector pos)
	{
		m_vDCO_QRFHoldPos = pos;
		m_bDCO_HasQRFHold = true;
	}

	void DCO_ClearQRFHoldPosition()
	{
		m_bDCO_HasQRFHold = false;
	}

	bool DCO_HasQRFHoldPosition()
	{
		return m_bDCO_HasQRFHold;
	}

	vector DCO_GetQRFHoldPosition()
	{
		return m_vDCO_QRFHoldPos;
	}

	void DCO_SetQRFRange(float range)
	{
		m_fDCO_QRFRange = range;
	}

	bool DCO_NeedsQRFSupport()
	{
		if (m_bDCO_Broken)
			return true;

		if (m_Perception)
		{
			array<IEntity> targets = m_Perception.m_aTargetEntities;
			if (targets && !targets.IsEmpty() && m_fDCO_Morale <= DCO_MoraleSettings.Get().m_fQRFCriticalMorale)
				return true;
		}
		return false;
	}

	bool DCO_GroupHasActiveWaypoint(SCR_AIGroup grp)
	{
		if (!grp)
			return false;
		return grp.GetCurrentWaypoint() != null;
	}

	void DCO_ClearExternalWaypoints(SCR_AIGroup grp)
	{
		if (!grp)
			return;
		array<AIWaypoint> wps = {};
		grp.GetWaypoints(wps);
		foreach (AIWaypoint wp : wps)
		{
			if (wp)
				grp.RemoveWaypoint(wp);
		}
	}

	void DCO_UpdateQRF()
	{
		if (!Replication.IsServer())
			return;

		bool qrfDirected = DCO_WaypointIntentUtil.IsQrfDirected(this);
		if ((!m_bDCO_IsQRFResponder && !qrfDirected) || !m_Owner)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		if (m_fDCO_LastQRFTime >= 0 && (now - m_fDCO_LastQRFTime) < DCO_QRF_CHECK_INTERVAL_MS)
			return;
		m_fDCO_LastQRFTime = now;

		IEntity selfLeader = m_Owner.GetLeaderEntity();
		if (!selfLeader)
			return;

		vector selfPos = selfLeader.GetOrigin();
		Faction selfFaction = m_Owner.GetFaction();
		float rangeSq = m_fDCO_QRFRange * m_fDCO_QRFRange;

		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);

		IEntity supportTarget;
		float bestDistSq = rangeSq;
		set<SCR_AIGroup> seen = new set<SCR_AIGroup>();

		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
			if (!grp || grp == m_Owner || seen.Contains(grp))
				continue;
			seen.Insert(grp);

			if (grp.GetFaction() != selfFaction)
				continue;

			IEntity grpLeader = grp.GetLeaderEntity();
			if (!grpLeader)
				continue;

			float dSq = vector.DistanceSq(grpLeader.GetOrigin(), selfPos);
			if (dSq > bestDistSq)
				continue;

			SCR_AIGroupUtilityComponent grpUtil = grp.GetGroupUtilityComponent();
			if (!grpUtil || !grpUtil.DCO_NeedsQRFSupport())
				continue;

			bestDistSq = dSq;
			supportTarget = DCO_PickQRFMoveTarget(grpUtil, grpLeader);
		}

		if (!supportTarget)
		{
			m_bDCO_QRFDeployed = false;
			DCO_CoordinatorRelease(EDCO_TacticalIntent.QRF_REINFORCE);	// nobody needs help - re-arm.

			// Fall-back behaviour: walk back to the staging/rally point so it waits there until the next call.
			vector holdPos = m_vDCO_QRFHoldPos;
			bool hasHold = m_bDCO_HasQRFHold;
			if (qrfDirected)
			{
				holdPos = DCO_WaypointIntentUtil.GetIntentPos(this);
				hasHold = true;
			}
			if (hasHold)
			{
				float leash = DCO_MoraleSettings.Get().m_fQRFHoldLeash;
				if (vector.DistanceSq(selfPos, holdPos) > leash * leash)
				{
					if (!DCO_CoordinatorRequest(EDCO_TacticalIntent.QRF_REINFORCE, 60, 10.0, "QRF return"))
						return;
					AICommunicationComponent rcomms = m_Mailbox;
					if (rcomms)
						DCO_VehicleUtil.OrderGroupMoveToPosition(m_Owner, holdPos, rcomms);
				}
			}
			return;
		}

		if (m_bDCO_QRFDeployed)
			return;	// already responding to the current call.

		AIAgent selfLeaderAgent = m_Owner.GetLeaderAgent();
		AICommunicationComponent comms = m_Mailbox;
		if (!selfLeaderAgent || !comms)
			return;

		// QRF intentionally overrides the responder's patrol/waypoint to go support.
		if (DCO_MoraleSettings.Get().m_bClearWaypointOnOverride)
			DCO_ClearExternalWaypoints(m_Owner);

		if (!DCO_CoordinatorRequest(EDCO_TacticalIntent.QRF_REINFORCE, 60, 15.0, "QRF deploy"))
			return;

		DCO_VehicleUtil.OrderGroupMoveToEntity(m_Owner, supportTarget, comms);
		m_bDCO_QRFDeployed = true;
	}

	// Prefer moving onto the distressed group's enemy; fall back to the friendly group's position.
	protected IEntity DCO_PickQRFMoveTarget(SCR_AIGroupUtilityComponent grpUtil, IEntity grpLeader)
	{
		if (grpUtil.m_Perception)
		{
			array<IEntity> tgts = grpUtil.m_Perception.m_aTargetEntities;
			if (tgts)
			{
				foreach (IEntity t : tgts)
				{
					if (t)
						return t;
				}
			}
		}
		return grpLeader;
	}
}
