// Unified tactical intent coordinator, opt-in and server-authoritative.
enum EDCO_TacticalIntent
{
	NONE,
	REACT_TO_CONTACT,
	QRF_REINFORCE,
}

modded class SCR_AIGroupUtilityComponent
{
	protected int m_eDCO_CoordIntent = EDCO_TacticalIntent.NONE;
	protected int m_iDCO_CoordPriority;
	protected float m_fDCO_CoordLeaseUntil = -1;
	protected string m_sDCO_CoordSource;

	void DCO_UpdateTacticalCoordinator()
	{
		if (!Replication.IsServer() || !DCO_TacticalMoveSettings.Get().m_bEnableTacticalCoordinator)
			return;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		DCO_CoordinatorExpire(world.GetWorldTime());
	}

	bool DCO_CoordinatorRequest(int intent, int priority, float leaseSec, string source)
	{
		DCO_TacticalMoveSettings cfg = DCO_TacticalMoveSettings.Get();
		if (!cfg || !cfg.m_bEnableTacticalCoordinator)
			return true;
		if (!m_Owner)
			return true;	// teardown edge: no group to coordinate, and the log calls below read the leader.
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;
		float now = world.GetWorldTime();
		DCO_CoordinatorExpire(now);
		if (m_eDCO_CoordIntent != EDCO_TacticalIntent.NONE && m_eDCO_CoordIntent != intent && m_iDCO_CoordPriority > priority)
		{
			DCO_Debug.LogGroup("COORD", m_Owner.GetLeaderEntity(), string.Format("reject %1 p%2 from %3; owned by %4 p%5 (%6)", intent, priority, source, m_eDCO_CoordIntent, m_iDCO_CoordPriority, m_sDCO_CoordSource));
			return false;
		}
		bool changed = m_eDCO_CoordIntent != intent || m_sDCO_CoordSource != source;
		m_eDCO_CoordIntent = intent;
		m_iDCO_CoordPriority = priority;
		m_sDCO_CoordSource = source;
		m_fDCO_CoordLeaseUntil = now + Math.Max(leaseSec, 0.25) * 1000.0;
		if (changed)
			DCO_Debug.LogGroup("COORD", m_Owner.GetLeaderEntity(), string.Format("accept %1 p%2 from %3 lease=%4s", intent, priority, source, leaseSec));
		return true;
	}

	void DCO_CoordinatorRelease(int intent)
	{
		if (!DCO_TacticalMoveSettings.Get().m_bEnableTacticalCoordinator || m_eDCO_CoordIntent != intent)
			return;
		if (!m_Owner)
		{
			DCO_CoordinatorClear();
			return;
		}
		DCO_Debug.LogGroup("COORD", m_Owner.GetLeaderEntity(), string.Format("release %1 (%2)", intent, m_sDCO_CoordSource));
		DCO_CoordinatorClear();
	}

	protected void DCO_CoordinatorExpire(float now)
	{
		if (m_eDCO_CoordIntent == EDCO_TacticalIntent.NONE || m_fDCO_CoordLeaseUntil < 0 || now <= m_fDCO_CoordLeaseUntil)
			return;
		if (!m_Owner)
		{
			DCO_CoordinatorClear();
			return;
		}
		DCO_Debug.LogGroup("COORD", m_Owner.GetLeaderEntity(), string.Format("lease expired %1 (%2)", m_eDCO_CoordIntent, m_sDCO_CoordSource));
		DCO_CoordinatorClear();
	}

	protected void DCO_CoordinatorClear()
	{
		m_eDCO_CoordIntent = EDCO_TacticalIntent.NONE;
		m_iDCO_CoordPriority = 0;
		m_fDCO_CoordLeaseUntil = -1;
		m_sDCO_CoordSource = string.Empty;
	}
}
