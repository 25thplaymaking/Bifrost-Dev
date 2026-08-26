// Straggler / one-man-team merge.
modded class SCR_AIGroupUtilityComponent
{
	protected float		m_fDCO_LastMergeTime		= -1;
	protected SCR_AIGroup	m_DCO_PendingMergeTarget;

	void DCO_UpdateMerge()
	{
		if (!Replication.IsServer())
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		if (!cfg.m_bEnableStragglerMerge || !m_Owner || !m_Perception)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		if (m_fDCO_LastMergeTime >= 0 && (now - m_fDCO_LastMergeTime) < cfg.m_fMergeCheckSec * 1000.0)
			return;
		m_fDCO_LastMergeTime = now;

		if (m_DCO_PendingMergeTarget)
			return;	// a merge is already queued for this group.

		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
			return;

		// Pure-AI stragglers only, and only while in contact.
		if (m_Owner.GetPlayerCount() > 0)
			return;
		int count = m_Owner.GetPlayerAndAgentCount();
		if (count <= 0 || count > cfg.m_iStragglerSize)
			return;

		array<IEntity> targets = m_Perception.m_aTargetEntities;
		if (!targets || targets.IsEmpty())
			return;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return;

		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return;

		vector selfPos = leader.GetOrigin();
		Faction selfFaction = m_Owner.GetFaction();
		float bestSq = cfg.m_fMergeRadius * cfg.m_fMergeRadius;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);

		SCR_AIGroup best;
		set<SCR_AIGroup> seen = new set<SCR_AIGroup>();
		seen.Insert(m_Owner);

		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
			if (!grp || seen.Contains(grp))
				continue;
			seen.Insert(grp);

			if (grp.GetFaction() != selfFaction || grp.GetPlayerCount() > 0)
				continue;	// same faction, AI-only target.
			if (DCO_VehicleUtil.IsGroupInVehicle(grp))
				continue;	// don't fold survivors into a vehicle crew.
			if (grp.GetPlayerAndAgentCount() <= count)
				continue;	// must be strictly larger.

			SCR_AIGroupUtilityComponent candGu = grp.GetGroupUtilityComponent();
			if (candGu && candGu.m_DCO_PendingMergeTarget)
				continue;

			IEntity grpLeader = grp.GetLeaderEntity();
			if (!grpLeader)
				continue;

			float dSq = vector.DistanceSq(grpLeader.GetOrigin(), selfPos);
			if (dSq > bestSq)
				continue;

			bestSq = dSq;
			best = grp;
		}

		if (!best)
			return;

		// Defer the actual transfer one frame so we don't mutate the group hierarchy mid-tick.
		m_DCO_PendingMergeTarget = best;
		GetGame().GetCallqueue().CallLater(DCO_ExecuteMerge, 100, false);
	}

	// Deferred: move all of this group's surviving members into the queued target group.
	protected void DCO_ExecuteMerge()
	{
		SCR_AIGroup target = m_DCO_PendingMergeTarget;
		m_DCO_PendingMergeTarget = null;

		if (!target || !m_Owner)
			return;

		array<AIAgent> myAgents = {};
		m_Owner.GetAgents(myAgents);
		foreach (AIAgent a : myAgents)
		{
			if (!a)
				continue;
			IEntity ent = a.GetControlledEntity();
			if (!ent)
				continue;

			m_Owner.RemoveAIEntityFromGroup(ent);	// leave the straggler group.
			target.AddAIEntityToGroup(ent);	// join the larger group.
		}
	}
}
