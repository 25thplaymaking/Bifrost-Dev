// Bifrost FX: Mortar Strike Emitter — GM-placeable incoming mortar fire scattered around the emitter point.
class DCO_FxMortarComponentClass : ScriptComponentClass
{
}

class DCO_FxMortarComponent : ScriptComponent
{
	[Attribute("40", UIWidgets.Slider, "Impact scatter radius around the emitter (meters).", "5 150 5", category: "Bifrost"), RplProp()]
	float m_fSpreadRadius;

	[Attribute("8", UIWidgets.Slider, "Impacts per salvo.", "1 30 1", category: "Bifrost"), RplProp()]
	int m_iSalvoCount;

	[Attribute("2", UIWidgets.Slider, "Seconds between impacts within a salvo.", "0.5 10 0.5", category: "Bifrost"), RplProp()]
	float m_fImpactIntervalSec;

	[Attribute("0", UIWidgets.Slider, "Pause before the next salvo repeats (seconds). 0 = single salvo per FIRE press.", "0 120 5", category: "Bifrost"), RplProp()]
	float m_fSalvoPauseSec;

	[Attribute("0", UIWidgets.CheckBox, "LIVE shells: real 81mm HE detonations that wound, kill and destroy across the spread. OFF = real falling shells + whistle, cosmetic impacts only.", category: "Bifrost"), RplProp()]
	bool m_bLive;

	[Attribute("1", UIWidgets.CheckBox, "Play the impact explosion sound in COSMETIC mode. Whistle and LIVE detonations always carry their own vanilla sound.", category: "Bifrost"), RplProp()]
	bool m_bSound;

	[Attribute("0", UIWidgets.CheckBox, "Start the strike as soon as the emitter is placed.", category: "Bifrost"), RplProp()]
	bool m_bFiring;

	static const float SPAWN_HEIGHT = 200.0;
	static const float RING_HEIGHT = 2.0;
	static const float NOMINAL_SOUND_RADIUS = 800.0;	// documented visualization, not an engine attenuation query.
	static const int VISUAL_MS = 1000;
	static const int DUD_POLL_MS = 200;	// cosmetic dud impact watch.
	static const float DUD_IMPACT_HEIGHT = 1.0;	// height above terrain that counts as ground contact.
	static const int DUD_TIMEOUT_MS = 60000;	// safety: a dud that never lands is detonated cosmetically.
	static const string EXPLOSION_EVENT = "SOUND_EXPLOSION";	// event name confirmed in the shell's ExplosionEffect block.

	static const ResourceName LIVE_SHELL = "{8AEB3E3846780747}Prefabs/Weapons/Ammo/Ammo_Shell_81mm_HE_M821_EffectModule.et";

	static const ResourceName DUD_SHELL = "{38BAE094333E31BF}Prefabs/Weapons/Ammo/Ammo_Shell_81mm_HE_M821.et";

	static const ResourceName IMPACT_PTC = "{318510C6FB1633D9}Particles/Weapon/Explosion_Mortar_Base.ptc";
	static const ResourceName IMPACT_ACP = "{D8320D0247C27BE9}Sounds/Weapons/Ammo/MortarShells/Weapons_Ammo_MortarShell_8xmm_HE.acp";

	protected int m_iShotsLeft;
	protected ref Resource m_LoadedLive;
	protected ref Resource m_LoadedDud;
	protected ref array<IEntity> m_aDuds = {};
	protected ref array<vector> m_aDudLastPos = {};
	protected ref array<int> m_aDudAgeMs = {};	// parallel to m_aDuds: airborne time for the never-lands safety valve.
	protected bool m_bDudPollRunning;
	protected ref Shape m_RingShape;
	protected ref Shape m_SoundRadiusShape;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame() || !GetGame().InPlayMode())
			return;

		GetGame().GetCallqueue().CallLater(DCO_DrawRing, 500, false);
		GetGame().GetCallqueue().CallLater(DCO_DrawRing, VISUAL_MS, true);

		DCO_TriggerFxRegistry.Register(owner);

		if (!Replication.IsServer())
			return;

		m_iShotsLeft = m_iSalvoCount;
		if (m_bFiring)
			DCO_ScheduleShell(1000);	// placement grace before the first round is in the air.
	}

	void ~DCO_FxMortarComponent()
	{
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(DCO_SalvoTick);
			GetGame().GetCallqueue().Remove(DCO_NextSalvo);
			GetGame().GetCallqueue().Remove(DCO_DudTick);
			GetGame().GetCallqueue().Remove(DCO_DrawRing);
		}

		// Emitter deleted mid-strike: cosmetic duds still in the air would otherwise litter the field as unarmed pickup shells.
		if (GetGame() && Replication.IsServer() && m_aDuds)
		{
			foreach (IEntity dud : m_aDuds)
			{
				if (dud)
					RplComponent.DeleteRplEntity(dud, false);
			}
		}
		m_aDuds = null;
		m_aDudLastPos = null;
		m_aDudAgeMs = null;
		m_RingShape = null;
		m_SoundRadiusShape = null;

		DCO_TriggerFxRegistry.Unregister(GetOwner());
	}

	bool DCO_IsFiring()
	{
		return m_bFiring;
	}

	void DCO_SetFiring(bool fire)
	{
		m_bFiring = fire;
		DCO_ReplicateState();
		GetGame().GetCallqueue().Remove(DCO_SalvoTick);
		GetGame().GetCallqueue().Remove(DCO_NextSalvo);
		if (fire)
		{
			m_iShotsLeft = m_iSalvoCount;
			DCO_ScheduleShell(1);	// first round immediately.
		}
	}

	float DCO_GetSpread()			{ return m_fSpreadRadius; }
	void DCO_SetSpread(float v)		{ m_fSpreadRadius = Math.Clamp(v, 5, 150); DCO_ReplicateState(); }
	int DCO_GetSalvoCount()			{ return m_iSalvoCount; }

	void DCO_SetSalvoCount(int v)
	{
		m_iSalvoCount = Math.Clamp(v, 1, 30);
		DCO_ReplicateState();
		if (m_iShotsLeft > m_iSalvoCount)
			m_iShotsLeft = m_iSalvoCount;	// a mid-salvo shrink applies immediately, never over-fires.
	}

	float DCO_GetInterval()			{ return m_fImpactIntervalSec; }
	void DCO_SetInterval(float v)	{ m_fImpactIntervalSec = Math.Clamp(v, 0.5, 10); DCO_ReplicateState(); }
	float DCO_GetSalvoPause()		{ return m_fSalvoPauseSec; }
	void DCO_SetSalvoPause(float v)	{ m_fSalvoPauseSec = Math.Clamp(v, 0, 120); DCO_ReplicateState(); }
	bool DCO_GetLive()				{ return m_bLive; }
	void DCO_SetLive(bool v)		{ m_bLive = v; DCO_ReplicateState(); }
	bool DCO_GetSound()				{ return m_bSound; }
	void DCO_SetSound(bool v)		{ m_bSound = v; DCO_ReplicateState(); }

	protected void DCO_ReplicateState()
	{
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	protected void DCO_ScheduleShell(int delayMs)
	{
		GetGame().GetCallqueue().Remove(DCO_SalvoTick);
		GetGame().GetCallqueue().CallLater(DCO_SalvoTick, delayMs, false);
	}

	// One shell per tick; self-schedules the next at the impact interval.
	protected void DCO_SalvoTick()
	{
		if (!m_bFiring || !Replication.IsServer())
			return;

		DCO_FireShell();

		m_iShotsLeft--;
		if (m_iShotsLeft > 0)
		{
			int intervalMs = Math.Round(m_fImpactIntervalSec * 1000.0);
			DCO_ScheduleShell(intervalMs);
			return;
		}

		if (m_fSalvoPauseSec > 0)
		{
			int pauseMs = Math.Round(m_fSalvoPauseSec * 1000.0);
			GetGame().GetCallqueue().CallLater(DCO_NextSalvo, pauseMs, false);
		}
		else
		{
			m_bFiring = false;
			DCO_ReplicateState();
			m_iShotsLeft = m_iSalvoCount;
		}
	}

	protected void DCO_NextSalvo()
	{
		if (!m_bFiring || !Replication.IsServer())
			return;
		m_iShotsLeft = m_iSalvoCount;
		DCO_ScheduleShell(1);
	}

	protected void DCO_FireShell()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		if (!m_LoadedLive)
			m_LoadedLive = Resource.Load(LIVE_SHELL);
		if (!m_LoadedDud)
			m_LoadedDud = Resource.Load(DUD_SHELL);

		Resource shellRes = m_LoadedLive;
		if (!m_bLive)
			shellRes = m_LoadedDud;
		if (!shellRes)
		{
			Print("[DCO-FX] mortar emitter: shell prefab failed to load - strike stopped", LogLevel.WARNING);
			m_bFiring = false;
			DCO_ReplicateState();
			return;
		}

		// Scatter: engine helper, uniform disc around the emitter; impact Y snapped to terrain.
		vector center = owner.GetOrigin();
		vector target = SCR_Math2D.GenerateRandomPointInRadius(0, m_fSpreadRadius, Vector(center[0], 0, center[2]), false);
		target[1] = owner.GetWorld().GetSurfaceY(target[0], target[2]);

		vector spawnPos = center + Vector(0, SPAWN_HEIGHT, 0);
		vector dir = target - spawnPos;
		dir.Normalize();

		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixFromForwardVec(dir, sp.Transform);
		sp.Transform[3] = spawnPos;

		IEntity shell = GetGame().SpawnEntityPrefab(shellRes, owner.GetWorld(), sp);
		if (!shell)
		{
			Print("[DCO-FX] mortar emitter: shell spawn FAILED", LogLevel.WARNING);
			return;
		}

		Physics phys = shell.GetPhysics();
		if (phys)
			phys.ChangeSimulationState(SimulationState.SIMULATION);

		ProjectileMoveComponent move = ProjectileMoveComponent.Cast(shell.FindComponent(ProjectileMoveComponent));
		if (!move)
		{
			Print("[DCO-FX] mortar emitter: shell has no ProjectileMoveComponent", LogLevel.WARNING);
			RplComponent.DeleteRplEntity(shell, false);
			return;
		}
		move.Launch(dir, vector.Zero, 1.0, shell, null, owner, null, null);

		// Cosmetic rounds are duds - watch them down and stage the impact ourselves.
		if (!m_bLive)
		{
			m_aDuds.Insert(shell);
			m_aDudLastPos.Insert(spawnPos);
			m_aDudAgeMs.Insert(0);
			if (!m_bDudPollRunning)
			{
				m_bDudPollRunning = true;
				GetGame().GetCallqueue().CallLater(DCO_DudTick, DUD_POLL_MS, true);
			}
		}
	}

	protected void DCO_DudTick()
	{
		if (!Replication.IsServer())
			return;

		for (int i = m_aDuds.Count() - 1; i >= 0; i--)
		{
			IEntity dud = m_aDuds[i];
			if (!dud)
			{
				// The engine removed the round on contact before we saw it land - impact at last seen spot.
				DCO_CosmeticImpact(m_aDudLastPos[i]);
				DCO_RemoveDud(i);
				continue;
			}

			vector prevPos = m_aDudLastPos[i];
			vector pos = dud.GetOrigin();
			m_aDudLastPos[i] = pos;
			m_aDudAgeMs[i] = m_aDudAgeMs[i] + DUD_POLL_MS;

			bool grounded = (pos[1] - dud.GetWorld().GetSurfaceY(pos[0], pos[2])) <= DUD_IMPACT_HEIGHT;
			bool atRest = m_aDudAgeMs[i] > 3000 && vector.DistanceSq(prevPos, pos) < 0.01;
			if (grounded || atRest || m_aDudAgeMs[i] >= DUD_TIMEOUT_MS)
			{
				DCO_CosmeticImpact(pos);
				RplComponent.DeleteRplEntity(dud, false);
				DCO_RemoveDud(i);
			}
		}

		if (m_aDuds.IsEmpty())
		{
			m_bDudPollRunning = false;
			GetGame().GetCallqueue().Remove(DCO_DudTick);
		}
	}

	protected void DCO_RemoveDud(int i)
	{
		m_aDuds.Remove(i);
		m_aDudLastPos.Remove(i);
		m_aDudAgeMs.Remove(i);
	}

	protected void DCO_CosmeticImpact(vector pos)
	{
		int soundFlag = 0;
		if (m_bSound)
			soundFlag = 1;
		RpcDo_DCO_MortarImpact(pos, soundFlag);
		Rpc(RpcDo_DCO_MortarImpact, pos, soundFlag);
	}

	// Cosmetic impact on THIS machine: the shell's own mortar explosion particle + the 81mm HE bank played positionally.
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_DCO_MortarImpact(vector pos, int soundFlag)
	{
		if (System.IsConsoleApp())
			return;

		ParticleEffectEntitySpawnParams pp = new ParticleEffectEntitySpawnParams();
		pp.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(pp.Transform);
		pp.Transform[3] = pos;
		ParticleEffectEntity.SpawnParticleEffect(IMPACT_PTC, pp);

		if (soundFlag > 0)
		{
			vector mat[4];
			Math3D.MatrixIdentity4(mat);
			mat[3] = pos;
			AudioSystem.PlayEvent(IMPACT_ACP, EXPLOSION_EVENT, mat);	// positional, engine attenuation.
		}
	}

	protected void DCO_DrawRing()
	{
		// GM-only: players must not see the strike ring, and a dedicated server has no renderer.
		if (!DCO_GMRights.IsLocalGameMaster())
		{
			m_RingShape = null;
			m_SoundRadiusShape = null;
			return;
		}

		IEntity owner = GetOwner();
		if (!owner)
			return;

		int color = 0xFFD9892B;	// amber - cosmetic.
		if (m_bLive)
			color = 0xFFFF3030;	// red - live shells.

		m_RingShape = DCO_ZoneShape.FlatCircle(owner.GetOrigin(), m_fSpreadRadius, color);
		m_SoundRadiusShape = null;
		if (m_bLive || m_bSound)
			m_SoundRadiusShape = DCO_ZoneShape.FlatCircle(owner.GetOrigin(), NOMINAL_SOUND_RADIUS, 0x6680D8FF);
	}
}
