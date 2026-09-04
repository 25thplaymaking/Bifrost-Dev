// Defensive hold.
modded class SCR_AIGroupUtilityComponent
{
	[RplProp()]
	protected bool	m_bDCO_IsDefender		= false;
	protected float	m_fDCO_LastDefendTime	= -1;
	protected bool	m_bDCO_DefendIssued		= false;
	protected vector m_vDCO_DefendDirection	= vector.Zero;

	protected static const float DCO_DEFEND_CHECK_INTERVAL_MS	= 5000.0;
	protected static const float DCO_DEFEND_ARC_RAD				= 3.14159;
	protected static const float DCO_DEFEND_PRIORITY			= 1.0;

	bool DCO_IsDefender()
	{
		return m_bDCO_IsDefender;
	}

	void DCO_SetDefender(bool enable)
	{
		if (!Replication.IsServer() || m_bDCO_IsDefender == enable)
			return;
		m_bDCO_IsDefender = enable;
		Replication.BumpMe();
		if (!enable)
		{
			m_bDCO_DefendIssued = false;
			m_vDCO_DefendDirection = vector.Zero;
		}
		Print(string.Format("[DCO-GM] defensive hold %1: group=%2", enable, m_Owner), LogLevel.NORMAL);
	}

	void DCO_UpdateDefend()
	{
		if (!Replication.IsServer())
			return;

		if (!m_bDCO_IsDefender || !m_Owner || !m_Mailbox || !m_Perception)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		if (m_fDCO_LastDefendTime >= 0 && (now - m_fDCO_LastDefendTime) < DCO_DEFEND_CHECK_INTERVAL_MS)
			return;
		m_fDCO_LastDefendTime = now;

		// A mounted crew defends by holding in its vehicle; a hold/orient order would make it disembark to take foot positions.
		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
			return;

		// Nearest perceived enemy = the direction to defend toward.
		array<IEntity> targets = m_Perception.m_aTargetEntities;
		if (!targets || targets.IsEmpty())
		{
			if (m_bDCO_DefendIssued)
				Print(string.Format("[DCO-GM] defensive hold: contact lost, waiting at group=%1", m_Owner), LogLevel.NORMAL);
			m_bDCO_DefendIssued = false;
			m_vDCO_DefendDirection = vector.Zero;
			return;
		}

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return;
		vector leaderPos = leader.GetOrigin();

		IEntity nearest;
		float bestSq = 1000000000.0;
		foreach (IEntity t : targets)
		{
			if (!t)
				continue;
			float dSq = vector.DistanceSq(t.GetOrigin(), leaderPos);
			if (dSq < bestSq)
			{
				bestSq = dSq;
				nearest = t;
			}
		}
		if (!nearest)
			return;

		vector dir = nearest.GetOrigin() - leaderPos;
		dir[1] = 0;
		if (dir.LengthSq() < 0.01)
			return;
		dir.Normalize();
		if (m_bDCO_DefendIssued && vector.Dot(m_vDCO_DefendDirection, dir) >= 0.94)
			return;

		SCR_AIMessage_Defend msg = SCR_AIMessage_Defend.Create(dir, DCO_DEFEND_ARC_RAD, false, DCO_DEFEND_PRIORITY, null, null);
		if (!msg)
			return;

		m_Mailbox.RequestBroadcast(msg);
		m_bDCO_DefendIssued = true;
		m_vDCO_DefendDirection = dir;
		Print(string.Format("[DCO-GM] defensive hold oriented: group=%1 target=%2 direction=%3", m_Owner, nearest, dir), LogLevel.NORMAL);
	}
}
