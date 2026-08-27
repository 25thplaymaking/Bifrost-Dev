// DCO GM Gameplay-panel PAUSE core.
enum EDCO_PauseScope
{
	SELECTED,
	ALL_AI
}

class EDCO_PauseAspect
{
	static const int AI      = 1;
	static const int PHYSICS = 2;
}

class DCO_GMPauseRecord
{
	IEntity m_Entity;
	int m_AspectMask;
	bool m_bAIWasOn;
	bool m_bSimWasOn;
}

class DCO_GMPauseCore
{
	protected static ref DCO_GMPauseCore s_Instance;
	static DCO_GMPauseCore Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMPauseCore();
		return s_Instance;
	}

	protected ref array<ref DCO_GMPauseRecord> m_Frozen = {};
	protected int m_iRequestOwnerPlayerId = -1;

	bool IsActive() { PruneDeadRecords(); return m_Frozen.Count() > 0; }
	int GetFrozenCount() { PruneDeadRecords(); return m_Frozen.Count(); }	// live feedback for the GAMEPLAY panel's status line.
	bool IsRequestOwner(int playerId) { return playerId > 0 && playerId == m_iRequestOwnerPlayerId; }
	void NoteRequestOwner(int playerId)
	{
		if (playerId > 0 && m_iRequestOwnerPlayerId <= 0)
			m_iRequestOwnerPlayerId = playerId;
	}

	// True while game time itself is stopped.
	static bool IsWorldPaused()
	{
		ChimeraWorld cw = ChimeraWorld.CastFrom(GetGame().GetWorld());
		return cw && cw.IsGameTimePaused();
	}

	void ReleaseAll()
	{
		Apply(EDCO_PauseScope.ALL_AI, 0, false);
	}

	// Keep remote selection pauses authoritative by freezing only the replicated entity requested by the GM.
	void ApplySelected(IEntity target, int aspectMask)
	{
		if (!target || DCO_PlayerUtil.IsPlayer(target))
			return;
		FreezeOne(target, aspectMask);
	}

	void Apply(int scope, int aspectMask, bool on)
	{
		if (!on)
		{
			ChimeraWorld cw = ChimeraWorld.CastFrom(GetGame().GetWorld());
			if (cw && cw.IsGameTimePaused())
			{
				cw.PauseGameTime(false);
				Print("[DCO-GM] pause: stale game-time pause RELEASED", LogLevel.NORMAL);
			}
		}

		if (on)
		{
			// FREEZE: sweep the requested scope and freeze each in-scope, non-player entity per the aspect mask.
			array<IEntity> targets = {};
			CollectScope(scope, targets);

			int count = 0;
			foreach (IEntity e : targets)
			{
				if (!e || DCO_PlayerUtil.IsPlayer(e))
					continue;
				if (FreezeOne(e, aspectMask))
					count++;
			}

			Print(string.Format("[DCO-GM] pause FREEZE: scope=%1 mask=%2 affected=%3 frozenSet=%4",
				scope, aspectMask, count, m_Frozen.Count()), LogLevel.NORMAL);
			return;
		}

		// LIFT: a resume lifts EXACTLY the remembered set, ignoring the current scope/selection.
		int lifted = 0;
		DCO_GMTools tools = DCO_GMTools.Get();
		foreach (DCO_GMPauseRecord record : m_Frozen)
		{
			if (!record)
				continue;
			IEntity e = record.m_Entity;
			if (!e || DCO_PlayerUtil.IsPlayer(e))
				continue;
			if (record.m_AspectMask & EDCO_PauseAspect.AI)
				tools.SetCharacterFrozen(e, !record.m_bAIWasOn);
			if (record.m_AspectMask & EDCO_PauseAspect.PHYSICS)
				tools.SetSimFrozen(e, !record.m_bSimWasOn);
			lifted++;
		}
		m_Frozen.Clear();
		m_iRequestOwnerPlayerId = -1;

		Print(string.Format("[DCO-GM] pause LIFT: released=%1 (scope arg %2 ignored on resume)",
			lifted, scope), LogLevel.NORMAL);
	}

	// Freeze one entity per the aspect mask while retaining its pre-pause state.
	protected bool FreezeOne(IEntity e, int aspectMask)
	{
		if (!e)
			return false;
		bool isChar = CharacterAnimationComponent.Cast(e.FindComponent(CharacterAnimationComponent)) != null;
		bool canAI = isChar && AIControlComponent.Cast(e.FindComponent(AIControlComponent));
		bool canSim = isChar || e.GetPhysics();
		int applicableMask = 0;
		if ((aspectMask & EDCO_PauseAspect.AI) && canAI)
			applicableMask |= EDCO_PauseAspect.AI;
		if ((aspectMask & EDCO_PauseAspect.PHYSICS) && canSim)
			applicableMask |= EDCO_PauseAspect.PHYSICS;
		if (applicableMask == 0)
			return false;

		DCO_GMPauseRecord record = FindRecord(e);
		if (!record)
		{
			record = new DCO_GMPauseRecord();
			record.m_Entity = e;
			m_Frozen.Insert(record);
		}

		DCO_GMTools tools = DCO_GMTools.Get();
		if ((applicableMask & EDCO_PauseAspect.AI)
			&& !(record.m_AspectMask & EDCO_PauseAspect.AI))
		{
			record.m_bAIWasOn = tools.IsAIOn(e);
			record.m_AspectMask |= EDCO_PauseAspect.AI;
			if (record.m_bAIWasOn)
				tools.SetCharacterFrozen(e, true);
		}

		if ((applicableMask & EDCO_PauseAspect.PHYSICS)
			&& !(record.m_AspectMask & EDCO_PauseAspect.PHYSICS))
		{
			record.m_bSimWasOn = tools.IsSimOn(e);
			record.m_AspectMask |= EDCO_PauseAspect.PHYSICS;
			if (record.m_bSimWasOn)
				tools.SetSimFrozen(e, true);
		}

		return true;
	}

	protected DCO_GMPauseRecord FindRecord(IEntity entity)
	{
		foreach (DCO_GMPauseRecord record : m_Frozen)
		{
			if (record && record.m_Entity == entity)
				return record;
		}
		return null;
	}

	protected void PruneDeadRecords()
	{
		for (int i = m_Frozen.Count() - 1; i >= 0; i--)
		{
			DCO_GMPauseRecord record = m_Frozen[i];
			if (!record || !record.m_Entity)
				m_Frozen.Remove(i);
		}
		if (m_Frozen.IsEmpty())
			m_iRequestOwnerPlayerId = -1;
	}

	protected void CollectScope(int scope, out array<IEntity> outList)
	{
		set<IEntity> acc = new set<IEntity>();

		if (scope == EDCO_PauseScope.SELECTED)
		{
			set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
			SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
			foreach (SCR_EditableEntityComponent ec : selected)
			{
				if (ec && ec.GetOwner())
					acc.Insert(ec.GetOwner());
			}
		}

		if (scope == EDCO_PauseScope.ALL_AI)
		{
			AIWorld aiw = GetGame().GetAIWorld();
			if (aiw)
			{
				array<AIAgent> agents = {};
				aiw.GetAIAgents(agents);
				foreach (AIAgent a : agents)
				{
					if (!a)
						continue;
					IEntity ent = a.GetControlledEntity();
					if (!ent)
						continue;
					acc.Insert(ent);
					SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(ent.FindComponent(SCR_CompartmentAccessComponent));
					if (access)
					{
						IEntity veh = access.GetVehicle();
						if (veh)
							acc.Insert(veh);
					}
				}
			}
		}
		foreach (IEntity e : acc)
			outList.Insert(e);
	}
}
