
class DCO_GMTrigger
{
	vector m_Pos;
	float  m_Radius;
	bool   m_Fired;
}

class DCO_GMFlyby
{
	IEntity m_Entity;
	vector  m_StartPos;
	vector  m_Fwd;
}

class DCO_GMTools
{
	protected static ref DCO_GMTools s_Inst;
	static DCO_GMTools Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_GMTools();
		return s_Inst;
	}

	protected ref array<IEntity> m_HiddenTerrain = {};	// currently-hidden entities, for restore.
	protected ref array<IEntity> m_QueryBuf = {};	// QueryEntitiesBySphere collect buffer.
	protected ref array<ref Shape> m_Tracers = {};
	protected ref array<ref DCO_GMTrigger> m_Triggers = {};	// placed trigger zones.
	protected int m_TriggerCharCount;	// scratch for the trigger character-count query.
	protected bool m_TriggerTickOn;	// true while the trigger evaluation tick is running.
	protected int m_FpsFrames;	// frame counter for the 1-second FPS measurement.
	protected ref array<IEntity> m_HiddenUnits = {};
	protected IEntity m_TeleportMark;
	protected ref array<ref DCO_GMFlyby> m_Flybys = {};	// vehicles currently on a flyby.

	// SIM OPTIONS panel state.
	protected ref map<IEntity, bool> m_SimOff = new map<IEntity, bool>();	// entities with simulation disabled.
	protected ref map<IEntity, bool> m_AIOnConfirmed = new map<IEntity, bool>();
	protected ref map<IEntity, int> m_SavedLayers = new map<IEntity, int>();	// original physics interaction masks.

	// Live mannequin-freeze commands, keyed by the character they are pinning.
	protected ref map<IEntity, ref DCO_CharacterCommandFreeze> m_FreezeCmds = new map<IEntity, ref DCO_CharacterCommandFreeze>();

	protected ref map<IEntity, bool> m_MannequinFrozen = new map<IEntity, bool>();

	// Commands that have been asked to finish but may not have ticked yet.
	protected ref array<ref DCO_CharacterCommandFreeze> m_RetiringFreeze = {};

	// Per-entity scheduling sequence for the two deferred stance calls.
	protected ref map<IEntity, int> m_RefreezeSeq = new map<IEntity, int>();
	protected ref map<IEntity, int> m_StanceSeq = new map<IEntity, int>();

	protected void PruneDeadEntries()
	{
		for (int i = m_HiddenUnits.Count() - 1; i >= 0; i--)
		{
			if (!m_HiddenUnits[i])
				m_HiddenUnits.Remove(i);
		}
		for (int i = m_SimOff.Count() - 1; i >= 0; i--)
		{
			if (!m_SimOff.GetKey(i))
				m_SimOff.RemoveElement(i);
		}
		for (int i = m_AIOnConfirmed.Count() - 1; i >= 0; i--)
		{
			if (!m_AIOnConfirmed.GetKey(i))
				m_AIOnConfirmed.RemoveElement(i);
		}
		for (int i = m_SavedLayers.Count() - 1; i >= 0; i--)
		{
			if (!m_SavedLayers.GetKey(i))
				m_SavedLayers.RemoveElement(i);
		}
		for (int i = m_RefreezeSeq.Count() - 1; i >= 0; i--)
		{
			if (!m_RefreezeSeq.GetKey(i))
				m_RefreezeSeq.RemoveElement(i);
		}
		for (int i = m_StanceSeq.Count() - 1; i >= 0; i--)
		{
			if (!m_StanceSeq.GetKey(i))
				m_StanceSeq.RemoveElement(i);
		}
	}

	static const int   STANCE_SETTLE_MS = 1400;	// how long a stance transition is given before the freeze re-applies.
	static const int   STANCE_RETRY_MS   = 120;	// re-assert interval while waiting for a stance order to take.
	static const int   STANCE_RETRY_TRIES = 12;	// ~1.4 s of retries, then give up rather than spin.
	static const int   FREEZE_RETIRE_MS = 3000;	// how long a finishing command is kept alive before release.
	protected bool m_FlybyTickOn;	// true while the flyby mover tick is running.

	static const float HIDE_RADIUS = 35.0;
	static const float TRIGGER_RADIUS = 20.0;

	void HideTerrainAt(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		m_QueryBuf.Clear();
		world.QueryEntitiesBySphere(pos, HIDE_RADIUS, CollectHideable);
		int n = 0;
		foreach (IEntity e : m_QueryBuf)
		{
			if (!e)
				continue;
			e.ClearFlags(EntityFlags.VISIBLE, true);	// recursive to child meshes.
			if (!m_HiddenTerrain.Contains(e))
			{
				m_HiddenTerrain.Insert(e);
				n++;
			}
		}
		Print(string.Format("[DCO-GM] Hide Terrain: hid %1 entities within %2 m (total hidden=%3)", n, HIDE_RADIUS, m_HiddenTerrain.Count()), LogLevel.NORMAL);
	}

	// QueryEntitiesBySphere callback: collect building/destructible-structure entities.
	protected bool CollectHideable(IEntity e)
	{
		if (!e)
			return true;
		if (Building.Cast(e) || e.FindComponent(SCR_DestructibleBuildingComponent))
			m_QueryBuf.Insert(e);
		return true;	// keep iterating.
	}

	void RestoreTerrain()
	{
		int n = 0;
		foreach (IEntity e : m_HiddenTerrain)
		{
			if (e)
			{
				e.SetFlags(EntityFlags.VISIBLE, true);
				n++;
			}
		}
		m_HiddenTerrain.Clear();
		Print(string.Format("[DCO-GM] Restore Terrain: re-shown %1 entities", n), LogLevel.NORMAL);
	}

	bool HasHiddenTerrain()
	{
		return !m_HiddenTerrain.IsEmpty();
	}

	// FX: TRACER EMITTER helpers.
	bool IsTracerEmitter(IEntity owner)
	{
		return owner && owner.FindComponent(DCO_TracerEmitterComponent);
	}

	bool IsTracerFiring(IEntity owner)
	{
		if (!owner)
			return false;
		DCO_TracerEmitterComponent fx = DCO_TracerEmitterComponent.Cast(owner.FindComponent(DCO_TracerEmitterComponent));
		return fx && fx.DCO_IsFiring();
	}

	void ToggleTracerFireEntity(IEntity owner)
	{
		if (!owner)
			return;
		DCO_TracerEmitterComponent fx = DCO_TracerEmitterComponent.Cast(owner.FindComponent(DCO_TracerEmitterComponent));
		if (!fx)
			return;
		fx.DCO_SetFiring(!fx.DCO_IsFiring());
	}

	protected void ClearTracers()
	{
		m_Tracers.Clear();	// dropping the ref Shapes removes them from the render.
	}

	void PlaceMarkerAt(vector pos)
	{
		SCR_MapMarkerManagerComponent mgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!mgr)
		{
			Print("[DCO-GM] marker: no SCR_MapMarkerManagerComponent instance", LogLevel.WARNING);
			return;
		}
		SCR_MapMarkerBase marker = mgr.PrepareMilitaryMarker(EMilitarySymbolIdentity.BLUFOR, EMilitarySymbolDimension.LAND, EMilitarySymbolIcon.INFANTRY);
		if (!marker)
		{
			Print("[DCO-GM] marker: PrepareMilitaryMarker returned null (military config missing?)", LogLevel.WARNING);
			return;
		}
		int wx = Math.Round(pos[0]);
		int wy = Math.Round(pos[2]);
		marker.SetWorldPos(wx, wy);
		mgr.InsertStaticMarker(marker, true);	// isLocal=true -> the GM's own client marker.
		Print(string.Format("[DCO-GM] placed map marker at world (%1, %2)", wx, wy), LogLevel.NORMAL);
	}

	// TRIGGERS: place a trigger zone at the cursor.
	void PlaceTriggerAt(vector pos)
	{
		DCO_GMTrigger t = new DCO_GMTrigger();
		t.m_Pos = pos;
		t.m_Radius = TRIGGER_RADIUS;
		t.m_Fired = false;
		m_Triggers.Insert(t);
		if (!m_TriggerTickOn)
		{
			m_TriggerTickOn = true;
			GetGame().GetCallqueue().CallLater(TriggerTick, 500, true);	// 2 Hz evaluation.
		}
		Print(string.Format("[DCO-GM] trigger placed at %1 (r=%2 m); %3 active", pos, TRIGGER_RADIUS, m_Triggers.Count()), LogLevel.NORMAL);
	}

	protected void TriggerTick()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		int live = 0;
		foreach (DCO_GMTrigger t : m_Triggers)
		{
			if (!t || t.m_Fired)
				continue;
			live++;
			m_TriggerCharCount = 0;
			world.QueryEntitiesBySphere(t.m_Pos, t.m_Radius, CountChars);
			if (m_TriggerCharCount > 0)
			{
				t.m_Fired = true;
				FireTrigger(t);
			}
		}
		if (live == 0)	// nothing left to evaluate -> stop the tick.
		{
			GetGame().GetCallqueue().Remove(TriggerTick);
			m_TriggerTickOn = false;
		}
	}

	protected bool CountChars(IEntity e)
	{
		if (e && ChimeraCharacter.Cast(e))
			m_TriggerCharCount++;
		return true;
	}

	protected void FireTrigger(DCO_GMTrigger t)
	{
		PlaceMarkerAt(t.m_Pos);	// mark it on the map so the GM sees which trigger tripped.
		vector p[2];
		p[0] = t.m_Pos + Vector(0, 0.5, 0);
		p[1] = t.m_Pos + Vector(0, 12.0, 0);	// a vertical beam pulse.
		Shape pulse = Shape.CreateLines(0xFF40E020, ShapeFlags.NOZBUFFER, p, 2);	// green.
		if (pulse)
		{
			m_Tracers.Insert(pulse);
			GetGame().GetCallqueue().Remove(ClearTracers);
			GetGame().GetCallqueue().CallLater(ClearTracers, 3000, false);
		}
		Print(string.Format("[DCO-GM] TRIGGER FIRED at %1 (character entered)", t.m_Pos), LogLevel.NORMAL);
	}

	void ToggleInvuln(SCR_EditableEntityComponent e)
	{
		if (!e)
			return;
		DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_INVULN, e.GetOwner(), vector.Zero);
	}

	void ToggleInvulnEntity(IEntity owner)
	{
		if (!owner)
			return;
		DamageManagerComponent dmg = DamageManagerComponent.Cast(owner.FindComponent(DamageManagerComponent));
		if (!dmg)
		{
			Print("[DCO-GM] invuln: entity has no DamageManagerComponent", LogLevel.WARNING);
			return;
		}
		bool makeVulnerable = !dmg.IsDamageHandlingEnabled();	// flip current state.
		dmg.EnableDamageHandling(makeVulnerable);
		string state = "OFF (god mode)";
		if (makeVulnerable)
			state = "ON (vulnerable)";
		Print("[DCO-GM] invulnerability toggled: damage handling " + state, LogLevel.NORMAL);
	}


	// Physics simulation on/off.
	void ToggleSimEntity(IEntity owner)
	{
		if (!owner)
			return;
		SetSimFrozen(owner, !m_SimOff.Contains(owner));
	}

	void SetSimFrozen(IEntity owner, bool frozen)
	{
		if (!owner)
			return;
		PruneDeadEntries();
		if (DCO_PlayerUtil.IsPlayer(owner))
		{
			Print("[DCO-GM] sim set: target is a PLAYER - skipped (never act on players)", LogLevel.WARNING);
			return;
		}
		if (m_SimOff.Contains(owner) == frozen)
			return;	// already in the requested state — idempotent.

		// Vehicle simulation APIs do not control character gravity.
		CharacterAnimationComponent anim = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
		if (anim)
		{
			anim.PhysicsEnableGravity(!frozen && IsAIOn(owner));
			if (frozen)
			{
				Physics characterPhysics = owner.GetPhysics();
				if (characterPhysics)
				{
					characterPhysics.SetVelocity(vector.Zero);
					characterPhysics.SetAngularVelocity(vector.Zero);
				}
			}
			string cstate = "ON (restored)";
			if (frozen)
			{
				m_SimOff.Insert(owner, true);
				cstate = "OFF (free positioning)";
			}
			else
				m_SimOff.Remove(owner);
			Print("[DCO-GM] sim set: character gravity " + cstate, LogLevel.NORMAL);
			return;
		}

		Physics ph = owner.GetPhysics();
		if (!ph)
		{
			Print("[DCO-GM] sim: entity has no Physics", LogLevel.WARNING);
			return;
		}
		string state = "ON";
		if (frozen)
		{
			SCR_PhysicsHelper.ChangeSimulationState(owner, SimulationState.NONE, true);
			m_SimOff.Insert(owner, true);
			state = "OFF";
		}
		else
		{
			SCR_PhysicsHelper.ChangeSimulationState(owner, SimulationState.SIMULATION, true);
			m_SimOff.Remove(owner);
		}
		Print("[DCO-GM] sim set: " + state, LogLevel.NORMAL);
	}

	void ToggleAIEntity(IEntity owner)
	{
		if (!owner)
			return;
		if (DCO_PlayerUtil.IsPlayer(owner))
		{
			Print("[DCO-GM] ai toggle: target is a PLAYER - skipped (never act on players)", LogLevel.WARNING);
			return;
		}
		AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!ctrl)
		{
			Print("[DCO-GM] ai toggle: entity has no AIControlComponent (nothing to freeze)", LogLevel.WARNING);
			return;
		}
		SetCharacterFrozen(owner, IsAIOn(owner));	// read the ENGINE, then flip it.
	}

	// Applies or releases the complete character freeze.
	void SetCharacterFrozen(IEntity owner, bool frozen)
	{
		if (!owner)
			return;
		AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!ctrl)
			return;

		if (frozen)
		{
			// Parks the agent before disabling its AI.
			AIAgent agent = ctrl.GetAIAgent();
			if (agent)
				agent.SetPermanentLOD(AIAgent.GetMaxLOD());
			if (ctrl.IsAIActivated())
				ctrl.DeactivateAI();
			CharacterAnimationComponent frozenAnim = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
			if (frozenAnim)
				frozenAnim.PhysicsEnableGravity(false);

			InstallFreezeCommand(owner);	// layer 3 - the one that actually holds the BODY still.
			Print("[DCO-GM] ai toggled: OFF (MAX LOD + brain off + body pinned by freeze command)", LogLevel.NORMAL);
			return;
		}

		ReleaseFreezeCommand(owner);
		if (!ctrl.IsAIActivated())
			ctrl.ActivateAI();
		AIAgent back = ctrl.GetAIAgent();
		if (back)
			back.SetPermanentLOD(-1);
		CharacterAnimationComponent restoredAnim = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
		if (restoredAnim)
			restoredAnim.PhysicsEnableGravity(!m_SimOff.Contains(owner));

		// Returns stance control to the AI.
		DCO_StanceUtil.SetGMStanceLock(owner, -1);	// <0 releases.
		BumpStanceSeq(owner);	// stales THIS unit's pending retries only - other units keep theirs.

		Print("[DCO-GM] ai toggled: ON (freeze released + brain reactivated + LOD released + stance lock cleared)", LogLevel.NORMAL);
	}

	protected void InstallFreezeCommand(IEntity owner)
	{
		if (!owner || m_FreezeCmds.Contains(owner) || m_MannequinFrozen.Contains(owner))
			return;

		CharacterAnimationComponent anim = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
		if (!anim)
			return;	// not a character - the brain+LOD half above is the whole freeze for a non-character agent.

		// Stops movement without client-only simulation controls.
		CharacterControllerComponent mcc = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		if (mcc)
			mcc.SetMovement(0, vector.Zero);
		Physics ph = owner.GetPhysics();
		if (ph)
		{
			ph.SetVelocity(vector.Zero);
			ph.SetAngularVelocity(vector.Zero);
		}

		DCO_CharacterCommandFreeze cmd = new DCO_CharacterCommandFreeze(anim, owner);
		anim.SetCurrentCommand(cmd);
		m_FreezeCmds.Insert(owner, cmd);
	}

	protected void ReleaseFreezeCommand(IEntity owner)
	{
		if (!owner)
			return;

		// Clears any remaining animation-level freeze.
		CharacterAnimationComponent anim = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
		if (anim)
		{
			CharacterCommandHandlerComponent handler = anim.GetCommandHandler();
			if (handler)
				handler.SetSimulationDisabled(false);
		}
		m_MannequinFrozen.Remove(owner);

		DCO_CharacterCommandFreeze cmd = m_FreezeCmds.Get(owner);
		if (cmd)
		{
			cmd.RequestFinish();
			// Retains the command until its finish request is processed.
			m_RetiringFreeze.Insert(cmd);
			GetGame().GetCallqueue().Remove(ClearRetiredFreezes);
			GetGame().GetCallqueue().CallLater(ClearRetiredFreezes, FREEZE_RETIRE_MS, false);
		}
		m_FreezeCmds.Remove(owner);

		RestoreUsable(owner);
	}

	// Put a character back into a usable state.
	void RestoreUsable(IEntity owner)
	{
		if (!owner)
			return;
		CharacterAnimationComponent anim = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
		if (!anim)
			return;
		CharacterCommandHandlerComponent handler = anim.GetCommandHandler();
		if (!handler)
			return;

		handler.CancelItemUse();	// drops a frozen-open map / gadget.

		// Guarantee a LIVE full-body command.
		if (!handler.GetCommandVehicle())
			handler.StartCommand_Move();
	}

	protected void ClearRetiredFreezes()
	{
		m_RetiringFreeze.Clear();
	}

	// Bump an entity's re-freeze sequence: any RefreezeAfterStance already queued for THIS entity is now stale.
	protected int BumpRefreezeSeq(IEntity owner)
	{
		int seq = 0;
		m_RefreezeSeq.Find(owner, seq);
		seq++;
		m_RefreezeSeq.Set(owner, seq);
		return seq;
	}

	// The same per-entity supersede for the stance-retry tick.
	protected int BumpStanceSeq(IEntity owner)
	{
		int seq = 0;
		m_StanceSeq.Find(owner, seq);
		seq++;
		m_StanceSeq.Set(owner, seq);
		return seq;
	}

	// Re-install the freeze after a GM stance change has been allowed to play out.
	protected void RefreezeAfterStance(IEntity owner, int seq)
	{
		if (!owner)
			return;
		int cur = 0;
		m_RefreezeSeq.Find(owner, cur);
		if (cur != seq)
			return;
		AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!ctrl || ctrl.IsAIActivated())
			return;
		InstallFreezeCommand(owner);
		Print("[DCO-GM] freeze re-applied after stance transition", LogLevel.NORMAL);
	}

	// Tell a frozen unit's pin to follow the body to its new transform.
	void ReanchorFrozen(IEntity owner)
	{
		if (!owner)
			return;
		DCO_CharacterCommandFreeze cmd = m_FreezeCmds.Get(owner);
		if (cmd)
			cmd.Reanchor();
	}

	bool IsFrozenByCommand(IEntity owner)
	{
		return owner && (m_FreezeCmds.Contains(owner) || m_MannequinFrozen.Contains(owner));
	}

	bool IsCharacter(IEntity owner)
	{
		return owner && CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent)) != null;
	}


	// Pose a unit into a stance.
	void SetStanceEntity(IEntity owner, int stanceOrd)
	{
		if (!owner)
			return;
		if (DCO_PlayerUtil.IsPlayer(owner))
		{
			Print("[DCO-GM] stance: target is a PLAYER - skipped (never act on players)", LogLevel.WARNING);
			return;
		}
		if (stanceOrd < 0 || stanceOrd > 2)
			return;

		CharacterControllerComponent cc = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		if (!cc)
		{
			Print("[DCO-GM] stance: entity is not a character", LogLevel.WARNING);
			return;
		}

		bool wasFrozen = IsFrozenByCommand(owner);
		if (wasFrozen)
		{
			ReleaseFreezeCommand(owner);
			int refreezeSeq = BumpRefreezeSeq(owner);	// supersedes only THIS entity's pending re-freeze.
			GetGame().GetCallqueue().CallLater(RefreezeAfterStance, STANCE_SETTLE_MS, false, owner, refreezeSeq);
		}

		ECharacterStance st = stanceOrd;
		DCO_StanceUtil.SetGMStanceLock(owner, stanceOrd);	// GM order beats the autonomous DCO stance systems.
		bool issued = DCO_StanceUtil.TrySetStance(owner, st, 0);	// 0 = no throttle; a direct order, not a nudge.

		// Supersedes retries from earlier stance requests.
		int stanceSeq = BumpStanceSeq(owner);	// supersedes only THIS entity's pending retries.
		GetGame().GetCallqueue().CallLater(EnforceStanceTick, STANCE_RETRY_MS, false, owner, stanceOrd, STANCE_RETRY_TRIES, stanceSeq);

		string frozenNote = "";
		if (wasFrozen)
			frozenNote = " [was frozen: released to transition, re-freezing in " + STANCE_SETTLE_MS.ToString() + "ms]";
		Print(string.Format("[DCO-GM] stance set to %1 (0=stand 1=crouch 2=prone) issued=%2%3",
			stanceOrd, issued, frozenNote), LogLevel.NORMAL);
	}

	// Re-assert a GM stance order until the engine confirms it took.
	protected void EnforceStanceTick(IEntity owner, int stanceOrd, int triesLeft, int seq)
	{
		if (!owner || triesLeft <= 0)
			return;
		int cur = 0;
		m_StanceSeq.Find(owner, cur);
		if (cur != seq)
			return;
		if (DCO_PlayerUtil.IsPlayer(owner))
			return;	// never act on players.

		CharacterControllerComponent cc = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		if (!cc)
			return;
		if (cc.GetStance() == stanceOrd)
			return;	// the engine confirms it took - done.

		ECharacterStance st = stanceOrd;
		DCO_StanceUtil.TrySetStance(owner, st, 0);
		GetGame().GetCallqueue().CallLater(EnforceStanceTick, STANCE_RETRY_MS, false, owner, stanceOrd, triesLeft - 1, seq);
	}

	// Weapon raised / lowered — the last pose lever the engine exposes for a character.
	void ToggleWeaponRaisedEntity(IEntity owner)
	{
		if (!owner || DCO_PlayerUtil.IsPlayer(owner))
			return;
		CharacterControllerComponent cc = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		if (!cc)
			return;
		bool raised = !cc.IsWeaponRaised();
		cc.SetWeaponRaised(raised);
		Print(string.Format("[DCO-GM] weapon raised: %1", raised), LogLevel.NORMAL);
	}

	// Collision on/off.
	void ToggleCollisionEntity(IEntity owner)
	{
		if (!owner)
			return;
		PruneDeadEntries();
		Physics ph = owner.GetPhysics();
		if (!ph)
		{
			Print("[DCO-GM] collision: entity has no Physics", LogLevel.WARNING);
			return;
		}
		int saved;
		if (m_SavedLayers.Find(owner, saved))
		{
			ph.SetInteractionLayer(saved);	// restore exactly what the prefab shipped with.
			m_SavedLayers.Remove(owner);
			Print("[DCO-GM] collision toggled: ON", LogLevel.NORMAL);
			return;
		}
		m_SavedLayers.Insert(owner, ph.GetInteractionLayer());
		ph.SetInteractionLayer(0);
		Print("[DCO-GM] collision toggled: OFF", LogLevel.NORMAL);
	}

	bool IsSimOn(IEntity owner)
	{
		if (!owner)
			return false;
		return !m_SimOff.Contains(owner);
	}

	// Apply server acknowledgements to the requesting GM's presentation cache.
	void MirrorAuthorityState(IEntity owner, int toolId, bool on)
	{
		if (!owner)
			return;
		if (toolId == DCO_GMToolsServer.TOOL_SIM)
		{
			if (on)
				m_SimOff.Remove(owner);
			else
				m_SimOff.Set(owner, true);
		}
		else if (toolId == DCO_GMToolsServer.TOOL_COLLISION)
		{
			if (on)
				m_SavedLayers.Remove(owner);
			else
				m_SavedLayers.Set(owner, 0);
		}
		else if (toolId == DCO_GMToolsServer.TOOL_AI)
			m_AIOnConfirmed.Set(owner, on);
	}

	bool IsDamageOn(IEntity owner)
	{
		if (!owner)
			return false;
		DamageManagerComponent dmg = DamageManagerComponent.Cast(owner.FindComponent(DamageManagerComponent));
		return dmg && dmg.IsDamageHandlingEnabled();
	}

	// Reads authoritative AI state when no client confirmation exists.
	bool IsAIOn(IEntity owner)
	{
		if (!owner)
			return false;
		bool confirmed;
		if (!Replication.IsServer() && m_AIOnConfirmed.Find(owner, confirmed))
			return confirmed;
		AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!ctrl)
			return false;
		if (!ctrl.IsAIActivated())
			return false;
		AIAgent agent = ctrl.GetAIAgent();
		if (agent && agent.GetPermanentLOD() == AIAgent.GetMaxLOD())
			return false;
		return true;
	}

	int GetStanceOrd(IEntity owner)
	{
		if (!owner)
			return -1;
		CharacterControllerComponent cc = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		if (!cc)
			return -1;
		return cc.GetStance();
	}

	bool IsWeaponRaisedOn(IEntity owner)
	{
		if (!owner)
			return false;
		CharacterControllerComponent cc = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		return cc && cc.IsWeaponRaised();
	}

	// The Simulation row applies to ANYTHING with physics, characters included.
	bool CanToggleSim(IEntity owner)
	{
		return HasPhysics(owner) || IsCharacter(owner);
	}

	// Character collision is not safely toggleable.
	bool CanToggleCollision(IEntity owner)
	{
		return HasPhysics(owner) && !IsCharacter(owner);
	}

	bool IsCollisionOn(IEntity owner)
	{
		if (!owner)
			return false;
		return !m_SavedLayers.Contains(owner);
	}

	bool HasAI(IEntity owner)
	{
		return owner && AIControlComponent.Cast(owner.FindComponent(AIControlComponent)) != null;
	}

	bool HasPhysics(IEntity owner)
	{
		return owner && owner.GetPhysics() != null;
	}

	bool HasDamage(IEntity owner)
	{
		return owner && DamageManagerComponent.Cast(owner.FindComponent(DamageManagerComponent)) != null;
	}

	void MarkForTeleport(SCR_EditableEntityComponent e)
	{
		if (!e)
			return;
		m_TeleportMark = e.GetOwner();
		Print("[DCO-GM] marked for teleport -> right-click empty ground -> 'Teleport Marked Unit Here'", LogLevel.NORMAL);
	}

	bool HasTeleportMark()
	{
		return m_TeleportMark != null;
	}

	void TeleportMarkedTo(vector pos)
	{
		if (!m_TeleportMark)
		{
			Print("[DCO-GM] teleport: nothing marked", LogLevel.WARNING);
			return;
		}
		// Server-authoritative transform -> routed.
		DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_TELEPORT, m_TeleportMark, pos);
		m_TeleportMark = null;
	}

	void TeleportEntityTo(IEntity target, vector pos)
	{
		if (!target)
			return;
		vector mat[4];
		target.GetWorldTransform(mat);
		mat[3] = pos;	// keep facing, change position.
		float sc = target.GetScale();
		BaseGameEntity bge = BaseGameEntity.Cast(target);
		if (bge)
			bge.Teleport(mat);
		else
			target.SetWorldTransform(mat);
		target.SetScale(sc);
		Physics ph = target.GetPhysics();
		if (ph)
			ph.SetVelocity(vector.Zero);	// kill momentum so it doesn't carry through / ragdoll.
		target.Update();
		Print(string.Format("[DCO-GM] teleported marked unit to %1", pos), LogLevel.NORMAL);
	}

	static const float FLYBY_SPEED = 60.0;	// m/s.
	static const float FLYBY_RANGE = 4000.0;	// despawn after this distance.

	void SendOnFlyby(SCR_EditableEntityComponent e)
	{
		if (!e)
			return;
		DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_FLYBY, e.GetOwner(), vector.Zero);
	}

	void FlybyEntity(IEntity v)
	{
		if (!v)
			return;
		vector mat[4];
		v.GetWorldTransform(mat);
		Physics ph = v.GetPhysics();
		if (ph)
			ph.SetActive(ActiveState.INACTIVE);	// kinematic - we drive the transform, no gravity/tumble.
		DCO_GMFlyby fb = new DCO_GMFlyby();
		fb.m_Entity = v;
		fb.m_StartPos = mat[3];
		fb.m_Fwd = mat[2];	// the vehicle's forward axis.
		m_Flybys.Insert(fb);
		if (!m_FlybyTickOn)
		{
			m_FlybyTickOn = true;
			GetGame().GetCallqueue().CallLater(FlybyTick, 33, true);	// ~30 Hz.
		}
		Print("[DCO-GM] vehicle sent on flyby (faces its current heading; despawns at range)", LogLevel.NORMAL);
	}

	protected void FlybyTick()
	{
		float step = FLYBY_SPEED * 0.033;
		for (int i = m_Flybys.Count() - 1; i >= 0; i--)
		{
			DCO_GMFlyby fb = m_Flybys[i];
			if (!fb || !fb.m_Entity)
			{
				m_Flybys.Remove(i);
				continue;
			}
			vector mat[4];
			fb.m_Entity.GetWorldTransform(mat);
			mat[3] = mat[3] + fb.m_Fwd * step;	// advance along forward.
			fb.m_Entity.SetWorldTransform(mat);
			fb.m_Entity.Update();
			if (vector.Distance(fb.m_StartPos, mat[3]) > FLYBY_RANGE)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(fb.m_Entity);	// it has flown off - despawn.
				m_Flybys.Remove(i);
			}
		}
		if (m_Flybys.IsEmpty())
		{
			GetGame().GetCallqueue().Remove(FlybyTick);
			m_FlybyTickOn = false;
		}
	}

	void ToggleVisibility(SCR_EditableEntityComponent e)
	{
		if (!e)
			return;
		IEntity owner = e.GetOwner();
		if (!owner)
			return;
		PruneDeadEntries();
		if (m_HiddenUnits.Find(owner) != -1)
		{
			owner.SetFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true);
			m_HiddenUnits.RemoveItem(owner);
			Print("[DCO-GM] unit visibility: SHOWN", LogLevel.NORMAL);
		}
		else
		{
			owner.ClearFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true);
			m_HiddenUnits.Insert(owner);
			Print("[DCO-GM] unit visibility: HIDDEN (your view)", LogLevel.NORMAL);
		}
	}

	// FPS MONITOR.
	void ShowFps()
	{
		m_FpsFrames = 0;
		GetGame().GetCallqueue().Remove(FpsCountFrame);
		GetGame().GetCallqueue().Remove(FpsReport);
		GetGame().GetCallqueue().CallLater(FpsCountFrame, 0, true);	// runs every frame.
		GetGame().GetCallqueue().CallLater(FpsReport, 1000, false);	// stop + report after 1 s.
	}

	protected void FpsCountFrame()
	{
		m_FpsFrames++;
	}

	protected void FpsReport()
	{
		GetGame().GetCallqueue().Remove(FpsCountFrame);
		Print(string.Format("[DCO-GM] FPS (this client, measured over 1 s): %1", m_FpsFrames), LogLevel.NORMAL);
	}
}
