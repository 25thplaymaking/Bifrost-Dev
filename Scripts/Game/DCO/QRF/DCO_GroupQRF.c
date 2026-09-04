modded class SCR_AIGroupUtilityComponent
{
	[RplProp()]
	protected bool	m_bDCO_IsQRFResponder	= false;
	[RplProp()]
	protected float	m_fDCO_QRFRange			= 1500.0;
	protected float	m_fDCO_LastQRFTime		= -1;
	[RplProp()]
	protected bool	m_bDCO_QRFDeployed		= false;
	protected IEntity m_DCO_QRFSupportTarget;

	[RplProp()]
	protected vector	m_vDCO_QRFHoldPos		= vector.Zero;
	[RplProp()]
	protected bool		m_bDCO_HasQRFHold		= false;

	protected static const float DCO_QRF_CHECK_INTERVAL_MS = 5000.0;

	// Shared DCO tick.
	override SCR_AIActionBase EvaluateActivity(out bool restartActivity)
	{
		SCR_AIActionBase action = super.EvaluateActivity(restartActivity);
		DCO_UpdateAmbush();
		DCO_UpdateDefend();
		DCO_UpdateGMFormation();
		DCO_UpdateCqbClear();
		DCO_UpdateQRF();
		DCO_UpdateGMGarrison();
		return action;
	}

	bool DCO_IsQRFResponder()
	{
		return m_bDCO_IsQRFResponder;
	}

	void DCO_SetQRFResponder(bool enable)
	{
		if (!Replication.IsServer() || m_bDCO_IsQRFResponder == enable)
			return;
		m_bDCO_IsQRFResponder = enable;
		if (!enable)
		{
			m_bDCO_QRFDeployed = false;
			m_DCO_QRFSupportTarget = null;
		}
		Replication.BumpMe();
		Print(string.Format("[DCO-GM] QRF responder %1: group=%2 range=%3m", enable, m_Owner, m_fDCO_QRFRange), LogLevel.NORMAL);
	}

	float DCO_GetQRFRange()
	{
		return m_fDCO_QRFRange;
	}

	void DCO_SetQRFHoldPosition(vector pos)
	{
		if (!Replication.IsServer())
			return;
		m_vDCO_QRFHoldPos = pos;
		m_bDCO_HasQRFHold = true;
		Replication.BumpMe();
		Print(string.Format("[DCO-GM] QRF rally updated: group=%1 position=%2", m_Owner, pos), LogLevel.NORMAL);
	}

	void DCO_ClearQRFHoldPosition()
	{
		if (!Replication.IsServer())
			return;
		m_bDCO_HasQRFHold = false;
		Replication.BumpMe();
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
		if (!Replication.IsServer() || m_fDCO_QRFRange == range)
			return;
		m_fDCO_QRFRange = range;
		Replication.BumpMe();
		Print(string.Format("[DCO-GM] QRF range updated: group=%1 range=%2m", m_Owner, range), LogLevel.NORMAL);
	}

	bool DCO_NeedsQRFSupport()
	{
		if (m_Perception)
		{
			array<IEntity> targets = m_Perception.m_aTargetEntities;
			if (targets && !targets.IsEmpty())
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
			if (m_bDCO_QRFDeployed)
				Print(string.Format("[DCO-GM] QRF standing down: group=%1 returning to rally", m_Owner), LogLevel.NORMAL);
			m_bDCO_QRFDeployed = false;
			m_DCO_QRFSupportTarget = null;
			Replication.BumpMe();

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
				float leash = 50.0;
				if (vector.DistanceSq(selfPos, holdPos) > leash * leash)
				{
					AICommunicationComponent rcomms = m_Mailbox;
					if (rcomms)
						DCO_VehicleUtil.OrderGroupMoveToPosition(m_Owner, holdPos, rcomms);
				}
			}
			return;
		}

		if (m_bDCO_QRFDeployed && m_DCO_QRFSupportTarget == supportTarget)
			return;	// already responding to this call.

		AIAgent selfLeaderAgent = m_Owner.GetLeaderAgent();
		AICommunicationComponent comms = m_Mailbox;
		if (!selfLeaderAgent || !comms)
			return;

		// A QRF intent waypoint remains as the rally point; ordinary responder flags override their external route.
		if (qrfDirected)
			DCO_WaypointIntentUtil.ReleaseWaypointMovement(this);
		else
			DCO_ClearExternalWaypoints(m_Owner);

		DCO_VehicleUtil.OrderGroupMoveToEntity(m_Owner, supportTarget, comms);
		m_bDCO_QRFDeployed = true;
		m_DCO_QRFSupportTarget = supportTarget;
		Replication.BumpMe();
		Print(string.Format("[DCO-GM] QRF deployed: group=%1 target=%2 distance=%3m", m_Owner, supportTarget, Math.Round(Math.Sqrt(bestDistSq))), LogLevel.NORMAL);
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
