
// Course of action for a group in contact.
enum EDCO_COA
{
	AUTO,
	DEFEND,
	SUPPORT_BY_FIRE,
	ASSAULT_FLANK,
	BREAK_CONTACT,
	ADVANCE
}

modded class SCR_AIGroupUtilityComponent
{
	protected int	m_eDCO_COA				= EDCO_COA.AUTO;	// chosen COA; read by the contact drill.
	protected float	m_fDCO_LastBrainTime	= -1;

	// Current course of action.
	int DCO_GetCOA()
	{
		return m_eDCO_COA;
	}

	void DCO_UpdateTacticalBrain()
	{
		if (!Replication.IsServer())
			return;

		DCO_TacticalMoveSettings cfg = DCO_TacticalMoveSettings.Get();
		if (!cfg || !cfg.m_bEnableTacticalBrain || !m_Owner || !m_Perception)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_LastBrainTime >= 0 && (now - m_fDCO_LastBrainTime) < cfg.m_fBrainCheckSec * 1000.0)
			return;
		m_fDCO_LastBrainTime = now;

		// No contact: clear the COA and let default/idle behaviour run.
		array<IEntity> targets = m_Perception.m_aTargetEntities;
		bool inContact = targets && !targets.IsEmpty();
		if (!inContact)
		{
			m_eDCO_COA = EDCO_COA.AUTO;
			return;
		}

		// Own strength = living members.
		int own = 0;
		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (a && a.GetControlledEntity())
				own++;
		}

		// Enemy strength = perceived targets.
		int enemy = 0;
		foreach (IEntity t : targets)
		{
			if (t)
				enemy++;
		}
		if (enemy < 1)
			enemy = 1;

		float ratio = own / (float)enemy;

		int coa;
		if (ratio <= cfg.m_fBrainBreakPowerRatio)
			coa = EDCO_COA.BREAK_CONTACT;
		else if (ratio >= cfg.m_fBrainAssaultPowerRatio)
			coa = EDCO_COA.ASSAULT_FLANK;
		else
			coa = EDCO_COA.SUPPORT_BY_FIRE;

		m_eDCO_COA = coa;

		DCO_Debug.LogGroup("BRAIN", m_Owner.GetLeaderEntity(), string.Format("own=%1 enemy=%2 ratio=%3 -> COA=%4", own, enemy, ratio, coa));
	}
}
