enum EDCO_TracerRound
{
	US_556_RIFLE,	// 5.56x45 M856   — red, M16 rifle.
	US_762_MG,	// 7.62x51 M62    — red, M60 MG.
	US_50CAL,	// 12.7x99 M10    — red, M2 heavy.
	SOV_545_RIFLE,	// 5.45x39 7T3    — green, AK-74 rifle.
	SOV_762X39,	// 7.62x39 57T231P — green, AKM/Vz58.
	SOV_762X54R_MG,	// 7.62x54R 7T2   — green, PKM MG.
}

class DCO_TracerEmitterStaticData
{
	ref array<string> m_RoundNames = {
		"Rifle - US 5.56 (red)",
		"Machine Gun - US 7.62 (red)",
		"Heavy MG - US .50cal (red)",
		"Rifle - Soviet 5.45 (green)",
		"Rifle - Soviet 7.62x39 (green)",
		"Machine Gun - Soviet 7.62x54R (green)",
	};

	ref array<float> m_NominalSoundRadius = {350, 700, 1200, 350, 450, 750};

	ref array<ResourceName> m_RoundPrefabs = {
		"{A0CEFA7FA41091F7}Prefabs/Weapons/Ammo/Ammo_556x45_Tracer_M856.et",
		"{9CCBDD2ACB73FFA9}Prefabs/Weapons/Ammo/Ammo_762x51_Tracer_M62.et",
		"{678188455B777E61}Prefabs/Weapons/Ammo/Ammo_127x99_Tracer_M10.et",
		"{2BC4C9C3D171A53B}Prefabs/Weapons/Ammo/Ammo_545x39_Tracer_7T3.et",
		"{B9AB11A51D795EC5}Prefabs/Weapons/Ammo/Ammo_762x39_Tracer_57T231P.et",
		"{279B4FEECCA799BD}Prefabs/Weapons/Ammo/Ammo_762x54r_Tracer_7T2.et",
	};

	ref array<ResourceName> m_BallPrefabs = {
		"{AC26AB660097633D}Prefabs/Weapons/Ammo/Ammo_556x45_Ball_M855.et",
		"{C14C5DE6F97F06F0}Prefabs/Weapons/Ammo/Ammo_762x51_Ball_M80.et",
		"{A4E070505C87AD42}Prefabs/Weapons/Ammo/Ammo_127x99_Ball_M33.et",
		"{1D9DDE1632F33A9E}Prefabs/Weapons/Ammo/Ammo_545x39_Ball_7N6.et",
		"{2FBCA0A7CEDDE7B0}Prefabs/Weapons/Ammo/Ammo_762x39_Ball_57N231.et",
		"{AC29AE3D5ECD6390}Prefabs/Weapons/Ammo/Ammo_762x54r_Ball_57N323S.et",
	};

	ref array<ResourceName> m_ShotAcps = {
		"{DB92C647E05B2512}Sounds/Weapons/Rifles/M16A2/Weapons_Rifles_M16A2_Shot.acp",
		"{FE8279CA292FC6EE}Sounds/Weapons/Machineguns/M60/Weapons_Machineguns_M60_Shot.acp",
		"{EE8892C4180091FD}Sounds/Weapons/HeavyWeapons/M2/Weapons_HeavyWeapons_M2_Shot.acp",
		"{2C3081D1EE023D68}Sounds/Weapons/Rifles/AK-74/Weapons_Rifles_AK-74_Shot.acp",
		"{2C3081D1EE023D68}Sounds/Weapons/Rifles/AK-74/Weapons_Rifles_AK-74_Shot.acp",
		"{DD1D885601C14070}Sounds/Weapons/Machineguns/PKM/Weapons_Machineguns_PKM_Shot.acp",
	};
}

class DCO_TracerEmitterComponentClass : ScriptComponentClass
{
}

class DCO_TracerEmitterComponent : ScriptComponent
{
	protected static ref DCO_TracerEmitterStaticData s_StaticData;

	protected static DCO_TracerEmitterStaticData StaticData()
	{
		if (!s_StaticData)
			s_StaticData = new DCO_TracerEmitterStaticData();
		return s_StaticData;
	}

	static array<string> DCO_GetRoundNames() { return StaticData().m_RoundNames; }

	[Attribute("0", UIWidgets.ComboBox, "Round type fired by this stream (tracer variants of the vanilla calibers).", "", ParamEnumArray.FromEnum(EDCO_TracerRound), category: "Bifrost"), RplProp()]
	EDCO_TracerRound m_eRound;

	[Attribute("600", UIWidgets.Slider, "Rate of fire (rounds per minute).", "60 1200 20", category: "Bifrost"), RplProp()]
	float m_fRpm;

	[Attribute("5", UIWidgets.Slider, "Rounds per burst.", "1 30 1", category: "Bifrost"), RplProp()]
	int m_iBurstLen;

	[Attribute("1", UIWidgets.Slider, "Tracer density: 1 = every round is a tracer; N = one tracer per N rounds, the rest fire as ball (a real MG belt is 4-5).", "1 10 1", category: "Bifrost"), RplProp()]
	int m_iTracerEvery;

	[Attribute("2", UIWidgets.Slider, "Pause between bursts (seconds). 0 = continuous fire.", "0 15 0.5", category: "Bifrost"), RplProp()]
	float m_fPauseSec;

	[Attribute("0", UIWidgets.CheckBox, "LIVE rounds: the impact fuse is armed and the stream can wound/kill. OFF = cosmetic tracer visuals only.", category: "Bifrost"), RplProp()]
	bool m_bLive;

	[Attribute("1", UIWidgets.CheckBox, "Play a gunshot sound per round while firing.", category: "Bifrost"), RplProp()]
	bool m_bSound;

	[Attribute("0", UIWidgets.CheckBox, "Start firing as soon as the emitter is placed.", category: "Bifrost"), RplProp()]
	bool m_bFiring;

	static const float MUZZLE_HEIGHT = 1.4;
	static const float AIM_LINE_LEN = 12.0;
	static const int VISUAL_MS = 1000;

	static const string SHOT_EVENT = "SOUND_SHOT";
	protected int m_iBurstLeft;
	protected int m_iShotCounter;	// running shot index driving the every-Nth tracer pick.
	protected int m_iSpawnFails;
	protected ref Resource m_LoadedTracer;	// cached per round type; re-loaded when m_eRound changes.
	protected ref Resource m_LoadedBall;
	protected EDCO_TracerRound m_eLoadedFor;
	protected ref Shape m_AimShape;
	protected ref Shape m_SoundRadiusShape;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame() || !GetGame().InPlayMode())
			return;

		GetGame().GetCallqueue().CallLater(DCO_DrawAim, 500, false);
		GetGame().GetCallqueue().CallLater(DCO_DrawAim, VISUAL_MS, true);

		if (!Replication.IsServer())
			return;

		m_iBurstLeft = m_iBurstLen;
		if (m_bFiring)
			DCO_ScheduleNext(DCO_RoundIntervalMs());
	}

	void ~DCO_TracerEmitterComponent()
	{
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(DCO_FireTick);
			GetGame().GetCallqueue().Remove(DCO_DrawAim);
		}
		m_AimShape = null;
		m_SoundRadiusShape = null;
	}

	bool DCO_IsFiring()
	{
		return m_bFiring;
	}

	void DCO_SetFiring(bool fire)
	{
		m_bFiring = fire;
		DCO_ReplicateState();
		GetGame().GetCallqueue().Remove(DCO_FireTick);
		if (fire)
		{
			m_iBurstLeft = m_iBurstLen;
			m_iShotCounter = 0;	// each FIRE press starts the belt on a tracer.
			m_iSpawnFails = 0;
			DCO_ScheduleNext(1);	// first round immediately.
		}
	}

	int DCO_GetRound()					{ return m_eRound; }
	void DCO_SetRound(int r)			{ m_eRound = Math.Clamp(r, 0, StaticData().m_RoundPrefabs.Count() - 1); DCO_ReplicateState(); }
	float DCO_GetRpm()					{ return m_fRpm; }
	void DCO_SetRpm(float v)			{ m_fRpm = Math.Clamp(v, 60, 1200); DCO_ReplicateState(); }
	int DCO_GetBurstLen()				{ return m_iBurstLen; }

	void DCO_SetBurstLen(int v)
	{
		m_iBurstLen = Math.Clamp(v, 1, 30);
		DCO_ReplicateState();
		if (m_iBurstLeft > m_iBurstLen)
			m_iBurstLeft = m_iBurstLen;	// a mid-burst shrink applies immediately, never over-fires.
	}

	int DCO_GetTracerEvery()			{ return m_iTracerEvery; }
	void DCO_SetTracerEvery(int v)		{ m_iTracerEvery = Math.Clamp(v, 1, 10); DCO_ReplicateState(); }
	float DCO_GetPause()				{ return m_fPauseSec; }
	void DCO_SetPause(float v)			{ m_fPauseSec = Math.Clamp(v, 0, 15); DCO_ReplicateState(); }
	bool DCO_GetLive()					{ return m_bLive; }
	void DCO_SetLive(bool v)			{ m_bLive = v; DCO_ReplicateState(); }
	bool DCO_GetSound()					{ return m_bSound; }
	void DCO_SetSound(bool v)			{ m_bSound = v; DCO_ReplicateState(); }

	protected void DCO_ReplicateState()
	{
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	protected int DCO_RoundIntervalMs()
	{
		float rpm = Math.Clamp(m_fRpm, 60, 1200);
		return Math.Round(60000.0 / rpm);
	}

	protected void DCO_ScheduleNext(int delayMs)
	{
		GetGame().GetCallqueue().Remove(DCO_FireTick);
		GetGame().GetCallqueue().CallLater(DCO_FireTick, delayMs, false);
	}

	// One shot per tick; self-schedules the next at the round interval, inserting the burst pause when a burst completes.
	protected void DCO_FireTick()
	{
		if (!m_bFiring || !Replication.IsServer())
			return;

		DCO_FireOne();

		m_iBurstLeft--;
		if (m_iBurstLeft > 0)
		{
			DCO_ScheduleNext(DCO_RoundIntervalMs());
		}
		else
		{
			if (m_iSpawnFails > 0)
			{
				Print(string.Format("[DCO-FX] tracer emitter: %1 of %2 rounds FAILED to spawn this burst",
					m_iSpawnFails, m_iBurstLen), LogLevel.WARNING);
				m_iSpawnFails = 0;
			}
			m_iBurstLeft = m_iBurstLen;
			int pauseMs = Math.Round(m_fPauseSec * 1000.0);
			if (pauseMs < DCO_RoundIntervalMs())
				pauseMs = DCO_RoundIntervalMs();	// pause 0 = continuous at the rate of fire.
			DCO_ScheduleNext(pauseMs);
		}
	}

	protected void DCO_FireOne()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		int roundIdx = Math.Clamp(m_eRound, 0, StaticData().m_RoundPrefabs.Count() - 1);
		if (!m_LoadedTracer || m_eLoadedFor != m_eRound)
		{
			m_LoadedTracer = Resource.Load(StaticData().m_RoundPrefabs[roundIdx]);
			m_LoadedBall = Resource.Load(StaticData().m_BallPrefabs[roundIdx]);
			m_eLoadedFor = m_eRound;
		}
		if (!m_LoadedTracer)
		{
			Print("[DCO-FX] tracer emitter: round prefab failed to load - stream stopped", LogLevel.WARNING);
			m_bFiring = false;
			DCO_ReplicateState();
			return;
		}

		// Every-Nth belt mix: shot 0 (and every m_iTracerEvery-th after) is the tracer; the rest are ball.
		Resource roundRes = m_LoadedTracer;
		int density = Math.Clamp(m_iTracerEvery, 1, 10);
		if (density > 1 && (m_iShotCounter % density) != 0 && m_LoadedBall)
			roundRes = m_LoadedBall;
		m_iShotCounter++;

		vector mat[4];
		owner.GetWorldTransform(mat);
		vector dir = mat[2];
		dir.Normalize();
		vector muzzle = mat[3] + Vector(0, MUZZLE_HEIGHT, 0);

		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixFromForwardVec(dir, sp.Transform);	// BI's own orthonormal-from-forward helper.
		sp.Transform[3] = muzzle;

		IEntity proj = GetGame().SpawnEntityPrefab(roundRes, owner.GetWorld(), sp);
		if (!proj)
		{
			m_iSpawnFails++;
			return;
		}

		Physics phys = proj.GetPhysics();
		if (phys)
			phys.ChangeSimulationState(SimulationState.SIMULATION);

		if (m_bLive)
		{
			BaseTriggerComponent trigger = BaseTriggerComponent.Cast(proj.FindComponent(BaseTriggerComponent));
			if (trigger)
				trigger.SetLive();
		}

		ProjectileMoveComponent move = ProjectileMoveComponent.Cast(proj.FindComponent(ProjectileMoveComponent));
		if (move)
			move.Launch(dir, vector.Zero, 1.0, proj, null, owner, null, null);

		if (m_bSound)
			AudioSystem.PlayEvent(StaticData().m_ShotAcps[roundIdx], SHOT_EVENT, sp.Transform);	// positional, engine attenuation.
	}

	// Aim line: muzzle -> forward.
	protected void DCO_DrawAim()
	{
		// GM-only: players must not see the aim line or sound ring, and a dedicated server has no renderer.
		if (!DCO_GMRights.IsLocalGameMaster())
		{
			m_AimShape = null;
			m_SoundRadiusShape = null;
			return;
		}

		IEntity owner = GetOwner();
		if (!owner)
			return;

		vector mat[4];
		owner.GetWorldTransform(mat);
		vector dir = mat[2];
		dir.Normalize();
		vector muzzle = mat[3] + Vector(0, MUZZLE_HEIGHT, 0);

		int color = 0xFFD9892B;	// amber - cosmetic.
		if (m_bLive)
			color = 0xFFFF3030;	// red - live rounds.

		vector pts[2];
		pts[0] = muzzle;
		pts[1] = muzzle + dir * AIM_LINE_LEN;
		m_AimShape = Shape.CreateLines(color, ShapeFlags.NOZBUFFER, pts, 2);
		m_SoundRadiusShape = null;
		if (m_bSound)
		{
			int roundIdx = Math.Clamp(m_eRound, 0, StaticData().m_NominalSoundRadius.Count() - 1);
			m_SoundRadiusShape = DCO_ZoneShape.FlatCircle(owner.GetOrigin(), StaticData().m_NominalSoundRadius[roundIdx], 0x6680D8FF);
		}
	}
}
