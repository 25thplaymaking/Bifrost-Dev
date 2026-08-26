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

class DCO_GMPauseCore
{
	protected static ref DCO_GMPauseCore s_Instance;
	static DCO_GMPauseCore Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMPauseCore();
		return s_Instance;
	}

	protected ref set<IEntity> m_Frozen = new set<IEntity>();

	bool IsActive() { return m_Frozen.Count() > 0; }
	int GetFrozenCount() { return m_Frozen.Count(); }	// live feedback for the GAMEPLAY panel's status line.

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
		array<IEntity> frozen = {};
		foreach (IEntity fe : m_Frozen)
			frozen.Insert(fe);

		int lifted = 0;
		foreach (IEntity e : frozen)
		{
			if (!e || DCO_PlayerUtil.IsPlayer(e))
				continue;
			UnfreezeOne(e);
			lifted++;
		}
		m_Frozen.Clear();

		Print(string.Format("[DCO-GM] pause LIFT: released=%1 (scope arg %2 ignored on resume)",
			lifted, scope), LogLevel.NORMAL);
	}

	// Freeze one entity per the aspect mask; record it so the lift can reverse exactly this.
	protected bool FreezeOne(IEntity e, int aspectMask)
	{
		bool did = false;
		bool isChar = CharacterAnimationComponent.Cast(e.FindComponent(CharacterAnimationComponent)) != null;

		if ((aspectMask & EDCO_PauseAspect.AI) && isChar
			&& AIControlComponent.Cast(e.FindComponent(AIControlComponent)))
		{
			DCO_GMTools.Get().SetCharacterFrozen(e, true);
			did = true;
		}
		if ((aspectMask & EDCO_PauseAspect.PHYSICS) && !isChar && e.GetPhysics())
		{
			DCO_GMTools.Get().SetSimFrozen(e, true);
			did = true;
		}

		if (did)
			m_Frozen.Insert(e);
		return did;
	}

	protected void UnfreezeOne(IEntity e)
	{
		bool isChar = CharacterAnimationComponent.Cast(e.FindComponent(CharacterAnimationComponent)) != null;
		if (isChar)
			DCO_GMTools.Get().SetCharacterFrozen(e, false);
		else
			DCO_GMTools.Get().SetSimFrozen(e, false);
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
