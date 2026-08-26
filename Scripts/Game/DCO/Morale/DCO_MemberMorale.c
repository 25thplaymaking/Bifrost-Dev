// Per-member morale modifier.
modded class SCR_AIGroupUtilityComponent
{
	protected float	m_fDCO_LastMemberMoraleTime	= -1;

	void DCO_UpdateMemberMorale()
	{
		if (!Replication.IsServer())
			return;

		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		if (!cfg.m_bEnableMemberMorale || !m_Owner)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_LastMemberMoraleTime >= 0 && (now - m_fDCO_LastMemberMoraleTime) < cfg.m_fMemberMoraleCheckSec * 1000.0)
			return;
		m_fDCO_LastMemberMoraleTime = now;

		// Below the throttle: IsGroupInVehicle is a per-member FindComponent scan.
		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
			return;

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity ent = a.GetControlledEntity();
			if (!ent)
				continue;
			if (DCO_PlayerUtil.IsPlayer(ent))
				continue;	// never hold a player's fire.

			SCR_ChimeraAIAgent chim = SCR_ChimeraAIAgent.Cast(a);
			if (!chim || !chim.m_UtilityComponent || !chim.m_UtilityComponent.m_ThreatSystem)
				continue;

			float supp = chim.m_UtilityComponent.m_ThreatSystem.GetSuppressionMeasure();
			// Personal morale = group baseline minus this soldier's own current stress.
			float personal = m_fDCO_Morale - supp * cfg.m_fMemberMoraleSuppressionWeight;

			if (personal > cfg.m_fMemberMoraleHesitateThreshold)
				continue;	// steady enough - leave to the normal combat AI.

			SCR_CharacterControllerComponent cc = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
			if (!cc)
				continue;

			// Shaken: hesitate to fire / keep head down.
			cc.SetWeaponNoFireTime(cfg.m_fMemberMoraleHesitateSec);

			if (cfg.m_bMemberMoraleCower && personal <= cfg.m_fMemberMoraleCowerThreshold)
				DCO_StanceUtil.TrySetStance(ent, ECharacterStance.PRONE, cfg.m_fStanceCooldownSec * 1000.0);

			if (cfg.m_bDebug)
				DCO_Debug.LogGroup("MBRMORALE", ent, string.Format("shaken: personal=%1 (group=%2 supp=%3)", personal, m_fDCO_Morale, supp));
		}
	}
}
