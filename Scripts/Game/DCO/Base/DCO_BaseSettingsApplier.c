// Base-settings applier.
modded class SCR_AIGroupUtilityComponent
{
	protected float m_fDCO_LastBaseTime         = -1;
	protected int   m_iDCO_BaseFormationApplied = -1;
	protected bool  m_bDCO_ReactWasInContact    = false;	// reaction-time edge state: was this group in contact last check.
	protected bool  m_bDCO_BaseApplied          = false;	// restore edge: did we push the base baseline onto this group.

	protected bool  m_bDCO_GrpOverride   = false;	// true: this group uses its own levers below, else the global store.
	protected int   m_iDCO_GrpSkill      = 50;
	protected int   m_iDCO_GrpPerception = 50;
	protected int   m_iDCO_GrpReaction   = 50;
	protected int   m_iDCO_GrpFireRate   = 50;

	int DCO_ResolveGrp(int grpVal, int globVal)
	{
		if (m_bDCO_GrpOverride)
			return grpVal;
		return globVal;
	}

	bool DCO_GetGrpOverride()        { return m_bDCO_GrpOverride; }
	void DCO_SetGrpOverride(bool v)  { m_bDCO_GrpOverride = v; }
	int  DCO_GetGrpSkill()           { return m_iDCO_GrpSkill; }
	void DCO_SetGrpSkill(int v)      { m_iDCO_GrpSkill = v; }
	int  DCO_GetGrpPerception()      { return m_iDCO_GrpPerception; }
	void DCO_SetGrpPerception(int v) { m_iDCO_GrpPerception = v; }
	int  DCO_GetGrpReaction()        { return m_iDCO_GrpReaction; }
	void DCO_SetGrpReaction(int v)   { m_iDCO_GrpReaction = v; }
	int  DCO_GetGrpFireRate()        { return m_iDCO_GrpFireRate; }
	void DCO_SetGrpFireRate(int v)   { m_iDCO_GrpFireRate = v; }

	void DCO_UpdateReaction()
	{
		if (!Replication.IsServer())
			return;

		DCO_BaseSettings bs = DCO_BaseSettings.Get();
		if (!bs.m_bEnableBaseSettings || !m_Owner || !m_Perception)
		{
			m_bDCO_ReactWasInContact = false;	// reset edge so re-enabling re-arms cleanly.
			return;
		}

		array<IEntity> targets = m_Perception.m_aTargetEntities;
		bool inContact = targets && !targets.IsEmpty();

		// Rising edge only: just acquired contact this check.
		bool ambushOwnsHold = (m_bDCO_IsAmbusher && !m_bDCO_AmbushSprung) || m_bDCO_ManualHold;
		if (inContact && !m_bDCO_ReactWasInContact && !ambushOwnsHold)
		{
			int grpScore = DCO_ResolveGrp(m_iDCO_GrpReaction, bs.m_iReactionTime);
			array<AIAgent> agents = {};
			m_Owner.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				if (!a)
					continue;
				IEntity ent = a.GetControlledEntity();
				if (!ent)
					continue;
				if (DCO_BaseSettingsApplierUtil.IsPlayerControlled(ent))
					continue;	// AI only.

				int score = grpScore;
				SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
				if (combat && combat.DCO_GetUnitOverride())
					score = combat.DCO_GetUnitReaction();

				float delay = DCO_BaseSettingsUtil.ReactionDelayFromScore(score);
				if (delay <= 0)
					continue;
				SCR_CharacterControllerComponent cc = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
				if (cc)
					cc.SetWeaponNoFireTime(delay);
			}

			if (bs.m_bDebugBaseSettings)
				DCO_Debug.LogGroup("BASE", m_Owner.GetLeaderEntity(), string.Format("reaction hold on contact (group score %1)", grpScore));
		}

		m_bDCO_ReactWasInContact = inContact;
	}

	// Durable formation apply.
	protected bool DCO_ApplyFormation(int formationOrdinal)
	{
		IEntity groupEnt = GetOwner();
		if (!groupEnt)
			return false;

		bool applied = false;
		SCR_EAIGroupFormation desiredF = formationOrdinal;
		string formName = SCR_Enum.GetEnumName(SCR_EAIGroupFormation, formationOrdinal);

		SCR_AIGroupSettingsComponent setComp = SCR_AIGroupSettingsComponent.Cast(groupEnt.FindComponent(SCR_AIGroupSettingsComponent));
		if (setComp)
		{
			SCR_AIGroupFormationSetting newSetting = SCR_AIGroupFormationSetting.Create(SCR_EAISettingOrigin.EDITOR, desiredF);
			if (newSetting)
			{
				setComp.AddSetting(newSetting, true, true);	// createCopy, removeSameTypeAndOrigin.
				applied = true;
			}
		}

		// Immediate effect + fallback: the native formation set.
		AIFormationComponent formComp = AIFormationComponent.Cast(groupEnt.FindComponent(AIFormationComponent));
		if (formComp && formName != string.Empty)
		{
			formComp.SetFormation(formName);
			applied = true;
		}

		if (DCO_BaseSettings.Get().m_bDebugBaseSettings)
		{
			IEntity leaderDbg = null;
			if (m_Owner)
				leaderDbg = m_Owner.GetLeaderEntity();
			bool hasSet = setComp != null;
			bool hasForm = formComp != null;
			DCO_Debug.LogGroup("BASEFORM", leaderDbg, string.Format("ord=%1 name='%2' setComp=%3 formComp=%4 applied=%5", formationOrdinal, formName, hasSet, hasForm, applied));
		}

		return applied;
	}

	protected void DCO_RestoreBaseSettings()
	{
		if (!m_bDCO_BaseApplied || !m_Owner)
			return;
		m_bDCO_BaseApplied = false;
		m_iDCO_BaseFormationApplied = -1;	// re-apply formation cleanly if re-enabled.

		bool moraleOwnsSkill = DCO_MoraleSettings.Get().m_bEnableMoraleAccuracy;

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			IEntity ent = agent.GetControlledEntity();
			if (!ent)
				continue;
			if (DCO_BaseSettingsApplierUtil.IsPlayerControlled(ent))
				continue;	// AI only.

			SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
			if (!combat)
				continue;
			if (combat.DCO_GetUnitOverride())
				continue;	// per-UNIT override owns this member's levers - the restore must not clear them.
			if (!moraleOwnsSkill)
			{
				combat.ResetAISkill();
				combat.SetFireRateCoef(1.0, false);
			}
			combat.SetPerceptionFactor(1.0);
		}
	}

	void DCO_UpdateBaseSettings()
	{
		if (!Replication.IsServer())
			return;

		DCO_BaseSettings cfg = DCO_BaseSettings.Get();
		if (!cfg.m_bEnableBaseSettings)
		{
			DCO_RestoreBaseSettings();	// one-shot restore on the OFF edge.
			return;
		}
		if (!m_Owner)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_LastBaseTime >= 0 && (now - m_fDCO_LastBaseTime) < cfg.m_fApplyIntervalSec * 1000.0)
			return;
		m_fDCO_LastBaseTime = now;

		bool moraleOwnsSkill = DCO_MoraleSettings.Get().m_bEnableMoraleAccuracy;
		bool engaged = m_Perception && m_Perception.m_aTargetEntities && !m_Perception.m_aTargetEntities.IsEmpty();

		EAISkill grpSkill   = DCO_BaseSettingsUtil.SkillFromScore(DCO_ResolveGrp(m_iDCO_GrpSkill, cfg.m_iAiSkill));
		float    grpFireCo  = DCO_BaseSettingsUtil.FireRateFromScore(DCO_ResolveGrp(m_iDCO_GrpFireRate, cfg.m_iFireRate));
		float    grpPerc    = DCO_BaseSettingsUtil.PerceptionFromScore(DCO_ResolveGrp(m_iDCO_GrpPerception, cfg.m_iPerception));

		// Group formation.
		if (cfg.m_eDefaultSpawnFormation != m_iDCO_BaseFormationApplied)
		{
			if (DCO_ApplyFormation(cfg.m_eDefaultSpawnFormation))
				m_iDCO_BaseFormationApplied = cfg.m_eDefaultSpawnFormation;
		}

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			IEntity ent = agent.GetControlledEntity();
			if (!ent)
				continue;

			// AI only: never touch a player-controlled character.
			if (DCO_BaseSettingsApplierUtil.IsPlayerControlled(ent))
				continue;

			SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
			if (combat)
			{
				if (combat.DCO_GetUnitOverride())
				{
				}
				else
				{
					if (!moraleOwnsSkill)
					{
						combat.SetAISkill(grpSkill);
						combat.SetFireRateCoef(grpFireCo, false);
					}
					if (!engaged)
						combat.SetPerceptionFactor(grpPerc);
				}
			}

			if (cfg.m_eGlobalStance != DCO_EBaseStance.NONE)
				DCO_BaseSettingsApplierUtil.ForceStance(ent, cfg.m_eGlobalStance);
		}

		m_bDCO_BaseApplied = true;	// arm the restore edge: we have pushed the baseline onto this group.

		if (cfg.m_bDebugBaseSettings)
			DCO_Debug.LogGroup("BASE", m_Owner.GetLeaderEntity(),
				string.Format("skill=%1 perc=%2 form=%3 stance=%4 grpOverride=%5",
					cfg.m_iAiSkill, cfg.m_iPerception, cfg.m_eDefaultSpawnFormation, cfg.m_eGlobalStance, m_bDCO_GrpOverride));
	}
}

// Helper holder: AI-only check + stance bridge in one place.
class DCO_BaseSettingsApplierUtil
{
	static bool IsPlayerControlled(IEntity ent)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return false;
		return pm.GetPlayerIdFromControlledEntity(ent) != 0;
	}

	static void ForceStance(IEntity ent, DCO_EBaseStance stance)
	{
		if (DCO_StanceUtil.IsMidMove(ent))
			return;

		ECharacterStance s;
		switch (stance)
		{
			case DCO_EBaseStance.STAND:  s = ECharacterStance.STAND;  break;
			case DCO_EBaseStance.CROUCH: s = ECharacterStance.CROUCH; break;
			case DCO_EBaseStance.PRONE:  s = ECharacterStance.PRONE;  break;
			default: return;
		}
		DCO_StanceUtil.TrySetStance(ent, s, 1500);
	}
}
