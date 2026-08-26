// Per-character Base-Settings override.
modded class SCR_AICombatComponent
{
	protected bool m_bDCO_UnitOverride  = false;	// true: this character uses its own levers below.
	protected int  m_iDCO_UnitSkill      = 50;
	protected int  m_iDCO_UnitPerception = 50;
	protected int  m_iDCO_UnitReaction   = 50;
	protected int  m_iDCO_UnitFireRate   = 50;

	bool DCO_GetUnitOverride() { return m_bDCO_UnitOverride; }
	int  DCO_GetUnitSkill()      { return m_iDCO_UnitSkill; }
	int  DCO_GetUnitPerception() { return m_iDCO_UnitPerception; }
	int  DCO_GetUnitReaction()   { return m_iDCO_UnitReaction; }
	int  DCO_GetUnitFireRate()   { return m_iDCO_UnitFireRate; }

	void DCO_SetUnitOverride(bool on)
	{
		m_bDCO_UnitOverride = on;
		DCO_ApplyUnitOverride();	// apply the levers / restore engine defaults immediately.
	}

	void DCO_SetUnitSkill(int v)      { m_iDCO_UnitSkill = v;      DCO_ApplyUnitOverride(); }
	void DCO_SetUnitPerception(int v) { m_iDCO_UnitPerception = v; DCO_ApplyUnitOverride(); }
	void DCO_SetUnitReaction(int v)   { m_iDCO_UnitReaction = v; }
	void DCO_SetUnitFireRate(int v)   { m_iDCO_UnitFireRate = v;   DCO_ApplyUnitOverride(); }

	void DCO_ApplyUnitOverride()
	{
		if (!Replication.IsServer())
			return;
		if (m_bDCO_UnitOverride)
		{
			SetAISkill(DCO_BaseSettingsUtil.SkillFromScore(m_iDCO_UnitSkill));
			SetFireRateCoef(DCO_BaseSettingsUtil.FireRateFromScore(m_iDCO_UnitFireRate), false);
			SetPerceptionFactor(DCO_BaseSettingsUtil.PerceptionFromScore(m_iDCO_UnitPerception));
		}
		else
		{
			ResetAISkill();
			SetFireRateCoef(1.0, false);
			SetPerceptionFactor(1.0);
		}
	}
}
