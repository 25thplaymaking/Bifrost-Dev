modded class SCR_AIGroupUtilityComponent
{
	protected int   m_iDCO_GMFormationOrd   = -1;
	protected float m_fDCO_GMFormationLast  = -1;	// throttle for the standing-hold reposition.
	protected float m_fDCO_GMFormHandlerLast = -1;

	protected int  m_iDCO_GMStance        = -1;
	protected bool m_bDCO_GMStanceLocked  = false;

	// GM formation order.
	void DCO_OrderFormation(int ord)
	{
		IEntity groupEnt = GetOwner();
		if (!groupEnt)
			return;
		SCR_AIGroup grp = SCR_AIGroup.Cast(groupEnt);
		if (!grp)
			return;
		string formName = SCR_Enum.GetEnumName(SCR_EAIGroupFormation, ord);
		if (formName == string.Empty)
			return;

		AIGroupMovementComponent moveComp = AIGroupMovementComponent.Cast(groupEnt.FindComponent(AIGroupMovementComponent));
		if (moveComp)
			moveComp.SetFormationDefinition(0, formName);

		AIFormationComponent formComp = grp.GetFormationComponent();
		if (formComp)
			formComp.SetFormation(formName);

		m_iDCO_GMFormationOrd    = ord;
		m_fDCO_GMFormationLast   = -1;	// reform standing immediately.
		m_fDCO_GMFormHandlerLast = -1;	// re-assert travel formation immediately.

		Print(string.Format("[DCO-GM] Formation '%1' ordered (travel+held; moveComp=%2 formComp=%3)", formName, moveComp != null, formComp != null), LogLevel.NORMAL);
	}

	// GM stance order.
	void DCO_SetGMStance(int stanceOrd)
	{
		m_iDCO_GMStance = stanceOrd;
		Print(string.Format("[DCO-GM] GM stance ordered (ord=%1; persists via DCO_UpdateGMStance)", stanceOrd), LogLevel.NORMAL);
	}

	int DCO_GetGMStance()
	{
		return m_iDCO_GMStance;
	}


	void DCO_SendGMOrder(int actionId)
	{
		if (Replication.IsServer())
		{
			DCO_GMGroupOrders.ApplyToUtil(this, actionId);
			return;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
		{
			Print("[DCO-GM] order relay: no local player controller (cannot reach the server)", LogLevel.WARNING);
			return;
		}
		IEntity groupEnt = GetOwner();
		RplComponent rpl;
		if (groupEnt)
			rpl = RplComponent.Cast(groupEnt.FindComponent(RplComponent));
		if (!rpl || !rpl.Id().IsValid())
		{
			Print("[DCO-GM] order relay: group has no valid replication id - order skipped", LogLevel.WARNING);
			return;
		}
		pc.DCO_SendGMOrderFor(rpl.Id(), actionId);
	}


	void DCO_UpdateGMStance()
	{
		if (!Replication.IsServer())
			return;

		IEntity groupEnt = GetOwner();
		SCR_AIGroup grp = SCR_AIGroup.Cast(groupEnt);
		if (!grp)
			return;

		if (m_iDCO_GMStance < 0)
		{
			if (m_bDCO_GMStanceLocked)
			{
				DCO_GMStanceForMembers(grp, -1);
				m_bDCO_GMStanceLocked = false;
			}
			return;
		}

		if (DCO_VehicleUtil.IsGroupInVehicle(grp))
			return;	// seated members can't meaningfully change stance.

		DCO_GMStanceForMembers(grp, m_iDCO_GMStance);	// lock every member to the GM stance + apply it.
		m_bDCO_GMStanceLocked = true;
	}

	protected void DCO_GMStanceForMembers(SCR_AIGroup grp, int ord)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity ent = a.GetControlledEntity();
			if (!ent)
				continue;
			DCO_StanceUtil.SetGMStanceLock(ent, ord);	// ord<0 releases; supersedes the autonomous stance systems.
			if (ord >= 0)
			{
				ECharacterStance st = ord;
				DCO_StanceUtil.TrySetStance(ent, st, 1500.0);	// guards players + anti-jitter throttle.
			}
		}
	}

	// Keep the GM formation followed both moving and standing.
	void DCO_UpdateGMFormation()
	{
		if (!Replication.IsServer())
			return;
		if (m_iDCO_GMFormationOrd < 0)
			return;

		IEntity groupEnt = GetOwner();
		SCR_AIGroup grp = SCR_AIGroup.Cast(groupEnt);
		if (!grp)
			return;
		if (DCO_VehicleUtil.IsGroupInVehicle(grp))
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();

		string formName = SCR_Enum.GetEnumName(SCR_EAIGroupFormation, m_iDCO_GMFormationOrd);

		// (A) Travel formation: re-assert on the move handler every ~3s.
		AIGroupMovementComponent moveComp = AIGroupMovementComponent.Cast(groupEnt.FindComponent(AIGroupMovementComponent));
		if (moveComp && (m_fDCO_GMFormHandlerLast < 0 || (now - m_fDCO_GMFormHandlerLast) >= 3000.0))
		{
			m_fDCO_GMFormHandlerLast = now;
			moveComp.SetFormationDefinition(0, formName);
		}

		// (B) Standing hold only when idle + calm.
		if (GetThreatMeasure() > 0.1)
			return;
		if (grp.GetCurrentWaypoint())
			return;

		if (m_fDCO_GMFormationLast >= 0 && (now - m_fDCO_GMFormationLast) < 2000.0)
			return;
		m_fDCO_GMFormationLast = now;

		IEntity leader = grp.GetLeaderEntity();
		if (!leader)
			return;
		AICommunicationComponent comms = AICommunicationComponent.Cast(groupEnt.FindComponent(AICommunicationComponent));
		if (!comms)
			return;

		vector leaderPos = leader.GetOrigin();
		vector fwd = leader.GetTransformAxis(2);
		fwd[1] = 0;
		if (fwd.LengthSq() < 0.01)
			fwd = Vector(0, 0, 1);
		fwd.Normalize();
		vector right = Vector(fwd[2], 0, -fwd[0]);

		AIPathfindingComponent pf = AIPathfindingComponent.Cast(leader.FindComponent(AIPathfindingComponent));

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int idx = 0;
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity ent = a.GetControlledEntity();
			if (!ent || ent == leader)
				continue;	// the leader anchors the formation.
			if (DCO_PlayerUtil.IsPlayer(ent))
				continue;	// never order a player-controlled member.
			idx++;

			vector slot = leaderPos + DCO_GMFormationOffset(m_iDCO_GMFormationOrd, idx, 4.0, fwd, right);
			DCO_GMFormSnapNav(pf, slot, slot);

			if (vector.DistanceSq(ent.GetOrigin(), slot) < 4.0)
				continue;

			SCR_AIMessage_Move msg = SCR_AIMessage_Move.Create(null, slot, EMovementType.WALK, false, null);
			if (msg)
			{
				msg.SetReceiver(a);
				comms.RequestBroadcast(msg, a);
			}
		}
	}

	protected vector DCO_GMFormationOffset(int ord, int i, float s, vector fwd, vector right)
	{
		int rank = (i + 1) / 2;	// 1,1,2,2,3,3...
		float side = 1.0;
		if (i % 2 == 0)
			side = -1.0;

		switch (ord)
		{
			case 1:	// Line: spread left/right of the leader, roughly abreast.
				return right * (side * rank * s) + fwd * (-s * 0.15);
			case 2:	// Column: single file directly behind the leader.
				return fwd * (-i * s);
			case 3:	// Staggered Column: file behind the leader, alternating slightly left/right.
				return fwd * (-i * s) + right * (side * s * 0.5);
		}
		return fwd * (-rank * s) + right * (side * rank * s * 0.7);
	}

	protected void DCO_GMFormSnapNav(AIPathfindingComponent pf, vector inPos, out vector outPos)
	{
		outPos = inPos;
		if (!pf)
			return;
		vector corrected;
		if (pf.GetClosestPositionOnNavmesh(inPos, Vector(8, 2, 8), corrected))
			outPos = corrected;
	}
}
