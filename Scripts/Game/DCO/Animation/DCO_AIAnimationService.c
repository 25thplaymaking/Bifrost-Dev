enum EDCO_AIEditorAnimation
{
	NONE,
	SIT_GROUND,
	SIT,
	SIT_CHAIR,
	LEAN_LEFT,
	LEAN_RIGHT,
	SMOKE,
	PUSHUPS,
	LOITER,
	OFFICER_TABLE,
	OFFICER_WALK,
}

class DCO_AIAnimationSession
{
	IEntity m_Entity;
	int m_eAnimation;
	bool m_bExitOnThreat;
	ref SCR_AIAnimation_Base m_Animation;
	SCR_CharacterControllerComponent m_Controller;
	SCR_AIThreatSystem m_ThreatSystem;
	bool m_bDirectLoiter;

	bool Init(IEntity entity, int animation, bool exitOnThreat)
	{
		m_Entity = entity;
		m_eAnimation = animation;
		m_bExitOnThreat = exitOnThreat;
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_Entity);
		if (!character)
			return false;
		m_Controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!m_Controller)
			return false;

		ResolveThreatSystem();
		if (m_bExitOnThreat && m_ThreatSystem && m_ThreatSystem.GetState() >= EAIThreatState.ALERTED)
			return false;

		vector transform[4];
		m_Entity.GetWorldTransform(transform);
		bool started;
		if (DCO_AIAnimationService.IsNativeLoiter(animation))
		{
			ELoiteringType loiterType = animation - DCO_AIAnimationService.NATIVE_LOITER_BASE;
			started = StartDirectLoiter(loiterType, transform);
		}
		else switch (animation)
		{
			case EDCO_AIEditorAnimation.SIT_GROUND:
				m_Animation = new SCR_AIAnimation_Sitting();
				break;
			case EDCO_AIEditorAnimation.SIT:
				started = StartDirectLoiter(ELoiteringType.SIT, transform);
				break;
			case EDCO_AIEditorAnimation.SIT_CHAIR:
				started = StartDirectLoiter(ELoiteringType.SIT, transform, SCR_ELoiterItemID.CHAIR);
				break;
			case EDCO_AIEditorAnimation.LEAN_LEFT:
				started = StartDirectLoiter(ELoiteringType.LEAN_LEFT, transform);
				break;
			case EDCO_AIEditorAnimation.LEAN_RIGHT:
				started = StartDirectLoiter(ELoiteringType.LEAN_RIGHT, transform);
				break;
			case EDCO_AIEditorAnimation.SMOKE:
				started = StartDirectLoiter(ELoiteringType.SMOKING, transform, SCR_ELoiterItemID.CIGAR);
				break;
			case EDCO_AIEditorAnimation.PUSHUPS:
				m_Animation = new SCR_AIAnimation_Pushups();
				break;
			case EDCO_AIEditorAnimation.LOITER:
				m_Animation = new SCR_AIAnimation_Loitering();
				break;
			case EDCO_AIEditorAnimation.OFFICER_TABLE:
				m_Animation = new SCR_AIAnimation_OfficerMission_Table();
				break;
			case EDCO_AIEditorAnimation.OFFICER_WALK:
				m_Animation = new SCR_AIAnimation_OfficerMission_Walking();
				break;
		}

		if (m_Animation)
			started = m_Animation.StartAnimation(m_Entity, transform);
		if (!started)
			return false;
		if (m_bExitOnThreat && m_ThreatSystem)
			m_ThreatSystem.GetOnThreatStateChanged().Insert(OnThreatStateChanged);
		return true;
	}

	protected bool StartDirectLoiter(ELoiteringType loiterType, vector transform[4], SCR_ELoiterItemID itemPreset = SCR_ELoiterItemID.NONE)
	{
		if (!m_Controller.CanPlayLoiterAnimation(loiterType))
			return false;
		SCR_LoiterCustomAnimData customData;
		if (itemPreset != SCR_ELoiterItemID.NONE)
			customData = SCR_LoiterCustomAnimData.CreateInstance(itemPreset: itemPreset);
		m_Controller.StartLoitering(null, loiterType, true, true, true, transform, true, customData);
		m_bDirectLoiter = true;
		return true;
	}

	protected void ResolveThreatSystem()
	{
		AIControlComponent control = AIControlComponent.Cast(m_Entity.FindComponent(AIControlComponent));
		AIAgent agent;
		if (control)
			agent = control.GetAIAgent();
		SCR_AICombatComponent combat;
		if (agent)
			combat = SCR_AICombatComponent.Cast(agent.FindComponent(SCR_AICombatComponent));
		SCR_AIInfoComponent info;
		if (combat)
			info = combat.GetAIInfoComponent();
		if (info)
			m_ThreatSystem = info.GetThreatSystem();
	}

	void SetExitOnThreat(bool enabled)
	{
		m_bExitOnThreat = enabled;
		if (enabled && m_ThreatSystem && m_ThreatSystem.GetState() >= EAIThreatState.ALERTED)
			Stop(true);
	}

	protected void OnThreatStateChanged(EAIThreatState previousState, EAIThreatState newState)
	{
		if (m_bExitOnThreat && newState >= EAIThreatState.ALERTED)
			Stop(true);
	}

	void Stop(bool fast)
	{
		if (m_ThreatSystem)
			m_ThreatSystem.GetOnThreatStateChanged().Remove(OnThreatStateChanged);
		m_ThreatSystem = null;
		if (m_Animation && m_Entity)
			m_Animation.StopAnimation(m_Entity, fast);
		else if (m_bDirectLoiter && m_Controller)
			m_Controller.StopLoitering(fast);
		m_Animation = null;
		m_Controller = null;
		m_bDirectLoiter = false;
		DCO_AIAnimationService.Remove(this);
	}
}

class DCO_AIAnimationService
{
	// Runtime loiter IDs are encoded outside the stable curated range. Both the
	// server and clients enumerate the same engine enum, so future vanilla
	// loiter additions become available without creating new network types.
	static const int NATIVE_LOITER_BASE = 1000;
	protected static ref array<int> s_aCatalogIds;
	protected static ref array<string> s_aCatalogLabels;
	protected static ref array<ref DCO_AIAnimationSession> s_aSessions;
	protected static bool s_bPolling;

	static bool IsValid(int animation)
	{
		BuildCatalog();
		return s_aCatalogIds.Contains(animation);
	}

	static bool IsNativeLoiter(int animation)
	{
		return animation >= NATIVE_LOITER_BASE;
	}

	static int CatalogCount()
	{
		BuildCatalog();
		return s_aCatalogIds.Count();
	}

	static int CatalogIdAt(int index)
	{
		BuildCatalog();
		if (!s_aCatalogIds.IsIndexValid(index))
			return EDCO_AIEditorAnimation.NONE;
		return s_aCatalogIds[index];
	}

	static string CatalogLabelAt(int index)
	{
		BuildCatalog();
		if (!s_aCatalogLabels.IsIndexValid(index))
			return "Unknown animation";
		return s_aCatalogLabels[index];
	}

	protected static void BuildCatalog()
	{
		if (s_aCatalogIds)
			return;
		s_aCatalogIds = {};
		s_aCatalogLabels = {};
		for (int animation = EDCO_AIEditorAnimation.NONE; animation <= EDCO_AIEditorAnimation.OFFICER_WALK; animation++)
		{
			s_aCatalogIds.Insert(animation);
			s_aCatalogLabels.Insert(Label(animation));
		}

		typename enumType = ELoiteringType;
		int variableCount = enumType.GetVariableCount();
		for (int i = 0; i < variableCount; i++)
		{
			int loiterValue;
			if (!enumType.GetVariableValue(null, i, loiterValue))
				continue;
			if (loiterValue == ELoiteringType.NONE || loiterValue == ELoiteringType.CUSTOM || IsCuratedLoiter(loiterValue))
				continue;
			s_aCatalogIds.Insert(NATIVE_LOITER_BASE + loiterValue);
			s_aCatalogLabels.Insert("Native · " + enumType.GetVariableName(i));
		}
	}

	protected static bool IsCuratedLoiter(int loiterValue)
	{
		return loiterValue == ELoiteringType.SIT
			|| loiterValue == ELoiteringType.LEAN_LEFT
			|| loiterValue == ELoiteringType.LEAN_RIGHT
			|| loiterValue == ELoiteringType.SMOKING
			|| loiterValue == ELoiteringType.PUSHUPS
			|| loiterValue == ELoiteringType.LOITERING;
	}

	static string Label(int animation)
	{
		if (IsNativeLoiter(animation))
			return "Native · " + typename.EnumToString(ELoiteringType, animation - NATIVE_LOITER_BASE);
		switch (animation)
		{
			case EDCO_AIEditorAnimation.NONE: return "Return to normal";
			case EDCO_AIEditorAnimation.SIT_GROUND: return "Sit on ground";
			case EDCO_AIEditorAnimation.SIT: return "Sit";
			case EDCO_AIEditorAnimation.SIT_CHAIR: return "Sit on chair";
			case EDCO_AIEditorAnimation.LEAN_LEFT: return "Lean left";
			case EDCO_AIEditorAnimation.LEAN_RIGHT: return "Lean right";
			case EDCO_AIEditorAnimation.SMOKE: return "Smoke cigar";
			case EDCO_AIEditorAnimation.PUSHUPS: return "Push-ups";
			case EDCO_AIEditorAnimation.LOITER: return "Loiter";
			case EDCO_AIEditorAnimation.OFFICER_TABLE: return "Officer briefing at table";
			case EDCO_AIEditorAnimation.OFFICER_WALK: return "Officer briefing walk";
		}
		return "Unknown animation";
	}

	protected static DCO_AIAnimationSession Find(IEntity entity)
	{
		if (!s_aSessions)
			return null;
		foreach (DCO_AIAnimationSession session : s_aSessions)
		{
			if (session && session.m_Entity == entity)
				return session;
		}
		return null;
	}

	static bool Apply(IEntity entity, int animation, bool exitOnThreat)
	{
		if (!Replication.IsServer() || !entity || !IsValid(animation))
			return false;
		DCO_AIAnimationSession oldSession = Find(entity);
		if (oldSession)
			oldSession.Stop(true);
		if (animation == EDCO_AIEditorAnimation.NONE)
			return true;
		if (!s_aSessions)
			s_aSessions = {};
		DCO_AIAnimationSession session = new DCO_AIAnimationSession();
		if (!session.Init(entity, animation, exitOnThreat))
		{
			Print("[DCO-ANIMATION] AI animation could not start on the selected unit", LogLevel.WARNING);
			return false;
		}
		s_aSessions.Insert(session);
		if (!s_bPolling)
		{
			s_bPolling = true;
			GetGame().GetCallqueue().CallLater(Poll, 1000, true);
		}
		return true;
	}

	static void Remove(DCO_AIAnimationSession session)
	{
		if (!s_aSessions)
			return;
		int index = s_aSessions.Find(session);
		if (index >= 0)
			s_aSessions.Remove(index);
		if (s_aSessions.IsEmpty() && s_bPolling)
		{
			s_bPolling = false;
			GetGame().GetCallqueue().Remove(Poll);
		}
	}

	protected static void Poll()
	{
		if (!s_aSessions)
			return;
		for (int i = s_aSessions.Count() - 1; i >= 0; i--)
		{
			DCO_AIAnimationSession session = s_aSessions[i];
			if (!session || !session.m_Entity || session.m_Entity.IsDeleted() || DCO_PlayerUtil.IsPlayer(session.m_Entity))
			{
				if (session)
					session.Stop(true);
				else
					s_aSessions.Remove(i);
			}
		}
	}
}

class DCO_AIAnimationServer
{
	static bool Apply(RplId targetId, int animation, bool exitOnThreat, out string result)
	{
		result = "Animation request failed";
		if (!Replication.IsServer())
			return false;
		if (!DCO_AIAnimationService.IsValid(animation))
		{
			result = "Unknown animation";
			return false;
		}
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(targetId));
		if (!rpl || !rpl.GetEntity())
		{
			result = "The selected unit is not replicated";
			return false;
		}
		IEntity target = rpl.GetEntity();
		SCR_EditableCharacterComponent editable = SCR_EditableCharacterComponent.Cast(target.FindComponent(SCR_EditableCharacterComponent));
		AIControlComponent control = AIControlComponent.Cast(target.FindComponent(AIControlComponent));
		if (!editable || !control || !control.GetAIAgent())
		{
			result = "Select an AI-controlled unit";
			return false;
		}
		if (DCO_PlayerUtil.IsPlayer(target))
		{
			result = "Player-controlled units cannot be animated";
			return false;
		}
		if (!DCO_AIAnimationService.Apply(target, animation, exitOnThreat))
		{
			result = "The unit cannot start that animation right now";
			return false;
		}
		result = DCO_AIAnimationService.Label(animation);
		return true;
	}
}
