// Bifrost FX: Explosion / rocket strike emitter.
enum EDCO_FxExplosionSize
{
	GRENADE,
	DEMO_CHARGE,
	MORTAR_SHELL,
	TNT_MEDIUM,
	TNT_LARGE,
}

enum EDCO_FxExplosionDelivery
{
	GROUND_WARHEAD,
	HYDRA_M229,
	S5KO,
	HELICOPTER_FLYBY,
	HELICOPTER_GUNRUN,
	SOUND_EMITTER,
	// Append-only: persisted prefab values pin these ordinals.
	LOITER_FLYBY,
	LOITER_GUNRUN,
}

enum EDCO_FxFamily
{
	STRIKE,	// ground warhead + Hydra 70 M229 + S-5KO.
	AIRSUPPORT,
	AUDIO,	// custom .acp bank event.
	LOITER,
}

class DCO_FxExplosionStaticData
{
	ref array<ECompartmentType> m_AircraftCrewTypes = {ECompartmentType.PILOT, ECompartmentType.TURRET};

	ref array<string> m_SizeNames = {
		"Grenade - M67 frag (small)",
		"Demo Charge - M112 block",
		"Mortar Shell - 81mm HE",
		"Big Blast - TNT 27kg",
		"Massive Blast - TNT 45kg",
	};

	ref array<string> m_DeliveryNames = {
		"Ground Warhead",
		"Rocket - Hydra 70 M229 HE",
		"Rocket - Soviet S-5KO HEDP",
		"Aircraft - UH-1H Flyby",
		"Aircraft - UH-1H Gunrun",
		"Audio - Custom Bank Event",
		"Aircraft - UH-1H Observation Orbit",
		"Aircraft - UH-1H Armed Orbit",
	};

	ref array<string> m_TargetTypeNames = {"Infantry", "Vehicles", "Any"};
	ref array<float> m_NominalSoundRadius = {250, 350, 600, 1000, 1500};

	ref array<ResourceName> m_LiveWarheads = {
		"{9C7B7B7ECDC3A596}Prefabs/Weapons/Warheads/Warhead_Grenade_M67.et",
		"{A1195711333615DF}Prefabs/Weapons/Warheads/Warhead_ExplosiveCharge_M112.et",
		"{6B8EE808E69A69E0}Prefabs/Weapons/Warheads/Warhead_Shell_HE_M821.et",
		"{564D57EA34A75775}Prefabs/Weapons/Warheads/Explosions/Explosion_Tnt_Medium.et",
		"{72BEEF40AF179763}Prefabs/Weapons/Warheads/Explosions/Explosion_Tnt_Large.et",
	};

	ref array<ResourceName> m_RocketPrefabs = {
		"{072A755D5CB85D47}Prefabs/Weapons/Ammo/Ammo_Rocket_Hydra70_HE_M229.et",
		"{EE65544BA845C458}Prefabs/Weapons/Ammo/Ammo_Rocket_S5_HEDP_S5KO.et",
	};

	ref array<ResourceName> m_CosmeticParticles = {
		"{5592BC9B67C60D16}Particles/Weapon/Explosion_RGD5.ptc",
		"{9D8D204CCCC44A38}Particles/Weapon/Explosion_M112.ptc",
		"{318510C6FB1633D9}Particles/Weapon/Explosion_Mortar_Base.ptc",
		"{001CC9410BE16BE3}Particles/Logistics/Explosion/TNT/Explosion_TNT_Medium.ptc",
		"{79ED2EDBC38185AB}Particles/Logistics/Explosion/TNT/Explosion_TNT_Large.ptc",
	};

	ref array<ResourceName> m_CosmeticAcps = {
		"{DB39A785FAA82E94}Sounds/Weapons/Grenades/Weapons_Grenade_Generic.acp",
		"{15572A81FA5612BB}Sounds/Weapons/Explosives/DemoBlocks/_SharedData/Weapons_Explosives_DemoBlock_Generic.acp",
		"{D8320D0247C27BE9}Sounds/Weapons/Ammo/MortarShells/Weapons_Ammo_MortarShell_8xmm_HE.acp",
		"{B9347A5498317F65}Sounds/Particles/Logistics/Explosion/TNT/Particles_Explosions_TNT_Medium.acp",
		"{E4EF3755472EC669}Sounds/Particles/Logistics/Explosion/TNT/Particles_Explosions_TNT_Large.acp",
	};
}

class DCO_FxAircraftPass
{
	IEntity m_Aircraft;
	vector m_Direction;
	vector m_Start;
	vector m_Target;
	bool m_bGunrun;
	bool m_bGunrunStarted;
	bool m_bContinuousFire;	// snapshotted per pass: once on station, fire until the station clock expires.
	bool m_bCrewReady;	// mission movement starts only after pilot/turret occupants finish seating.
	float m_fCrewWaitSec = 5.0;	// malformed third-party prefab fail-safe; the live-sim airframe is held meanwhile.
	float m_fCruiseSpeed;	// snapshotted ingress/egress speed.
	int m_iRoundsLeft;
	int m_iGunrunShotsFired;
	float m_fShotClockMs;
	float m_fLifeSec;
	bool m_bLoiter;	// orbit at the target instead of exiting after the overfly.
	bool m_bOnStation;	// loiter phase: established on the orbit ring and circling.
	bool m_bDeparting;	// loiter phase: station time spent, flying out for cleanup.
	float m_fStationSec;
	vector m_vOrbitEntry;	// tangent entry point: inbound loiters fly here, never through the target.
	vector m_vHeading;
	float m_fOrbitSign = 1.0;	// fixed turn direction; entry geometry is built to merge tangentially into it.
	float m_fOrbitY;	// fixed orbit altitude, sampled over the complete ring before spawn.
	float m_fOrbitSpeed;	// stable station speed; flyby speed remains the ingress/egress speed.
	ref array<IEntity> m_aCrew = {};	// default occupants spawned for this pass; deleted with the airframe.
}

class DCO_FxExplosionComponentClass : ScriptComponentClass
{
}

class DCO_FxExplosionComponent : ScriptComponent
{
	protected static ref DCO_FxExplosionStaticData s_StaticData;

	protected static DCO_FxExplosionStaticData StaticData()
	{
		if (!s_StaticData)
			s_StaticData = new DCO_FxExplosionStaticData();
		return s_StaticData;
	}

	static array<string> DCO_GetSizeNames() { return StaticData().m_SizeNames; }
	static array<string> DCO_GetTargetTypeNames() { return StaticData().m_TargetTypeNames; }
	static string DCO_GetDeliveryName(int ordinal) { return StaticData().m_DeliveryNames[ordinal]; }

	[Attribute("0", UIWidgets.ComboBox, "Blast size for ground impacts and cosmetic visuals.", "", ParamEnumArray.FromEnum(EDCO_FxExplosionSize), category: "Bifrost"), RplProp()]
	EDCO_FxExplosionSize m_eSize;

	[Attribute("0", UIWidgets.ComboBox, "Delivery: ground warhead, Hydra 70 M229, or S-5KO rocket. Rockets are spawned only when LIVE.", "", ParamEnumArray.FromEnum(EDCO_FxExplosionDelivery), category: "Bifrost"), RplProp()]
	EDCO_FxExplosionDelivery m_eDelivery;

	[Attribute("0", UIWidgets.ComboBox, "Effect family this emitter offers. Set on the PLACEABLE PREFAB, not by the GM - it decides which deliveries the GM can pick and which attribute rows are shown.", "", ParamEnumArray.FromEnum(EDCO_FxFamily), category: "Bifrost"), RplProp()]
	EDCO_FxFamily m_eFamily;

	[Attribute("1", UIWidgets.Slider, "Explosions per barrage.", "1 20 1", category: "Bifrost"), RplProp()]
	int m_iBarrageCount;

	[Attribute("0", UIWidgets.Slider, "Random scatter radius around the selected center (meters).", "0 200 5", category: "Bifrost"), RplProp()]
	float m_fScatterRadius;

	[Attribute("0.5", UIWidgets.Slider, "Seconds between explosions within a barrage.", "0.1 10 0.1", category: "Bifrost"), RplProp()]
	float m_fShotSpacingSec;

	[Attribute("0", UIWidgets.Slider, "Pause before the next barrage. 0 = one barrage per FIRE press.", "0 120 1", category: "Bifrost"), RplProp()]
	float m_fRepeatSec;

	[Attribute("0", UIWidgets.CheckBox, "Center each shot on a random connected player inside the tracking ring, then apply scatter.", category: "Bifrost"), RplProp()]
	bool m_bTrackPlayers;

	[Attribute("250", UIWidgets.Slider, "Player tracking radius around the emitter (meters).", "25 1000 25", category: "Bifrost"), RplProp()]
	float m_fTrackingRadius;

	[Attribute("0", UIWidgets.CheckBox, "LIVE: a damaging base-game warhead or projectile. OFF: cosmetic effects only.", category: "Bifrost"), RplProp()]
	bool m_bLive;

	[Attribute("1", UIWidgets.CheckBox, "Play the positional explosion bank in COSMETIC mode.", category: "Bifrost"), RplProp()]
	bool m_bSound;

	[Attribute("1", UIWidgets.Slider, "COSMETIC particle scale. Does not alter LIVE damage.", "0.25 4 0.25", category: "Bifrost"), RplProp()]
	float m_fParticleScale;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Optional custom cosmetic particle (.ptc). Empty uses the selected blast tier.", "ptc", category: "Bifrost"), RplProp()]
	ResourceName m_CustomParticle;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Optional positional audio bank (.acp) for Audio delivery.", "acp", category: "Bifrost"), RplProp()]
	ResourceName m_CustomSoundBank;

	[Attribute("SOUND_SHOT", UIWidgets.EditBox, "Event name inside the custom .acp bank.", category: "Bifrost"), RplProp()]
	string m_sCustomSoundEvent;

	[Attribute("500", UIWidgets.Slider, "Documented nominal audible-radius ring for custom audio; the bank owns real attenuation.", "25 3000 25", category: "Bifrost"), RplProp()]
	float m_fCustomSoundRadius;

	[Attribute("55", UIWidgets.Slider, "Aircraft pass speed in meters per second.", "25 100 5", category: "Bifrost"), RplProp()]
	float m_fFlybySpeed;

	[Attribute("80", UIWidgets.Slider, "Aircraft clearance above the highest sampled terrain along the pass.", "30 300 10", category: "Bifrost"), RplProp()]
	float m_fFlybyHeight;

	[Attribute("1500", UIWidgets.Slider, "Aircraft start/despawn distance on each side of the emitter.", "500 4000 100", category: "Bifrost"), RplProp()]
	float m_fFlybyDistance;

	[Attribute("40", UIWidgets.Slider, "Rounds in a helicopter gunrun. Cosmetic passes draw tracers without creating ammunition.", "5 100 5", category: "Bifrost"), RplProp()]
	int m_iGunrunRounds;

	[Attribute("900", UIWidgets.Slider, "Gunrun cyclic rate.", "300 1200 50", category: "Bifrost"), RplProp()]
	float m_fGunrunRpm;

	[Attribute("", UIWidgets.EditBox, "Aircraft prefab for this emitter's passes. Empty = stock UH-1H. Picked from the GM 'FX Aircraft' row (runtime helicopter catalog - modded helis included).", category: "Bifrost"), RplProp()]
	ResourceName m_sAircraftPrefab;

	[Attribute("60", UIWidgets.Slider, "LOITER: time on station in seconds (the fuel budget) before the aircraft departs.", "10 600 10", category: "Bifrost"), RplProp()]
	float m_fLoiterSec;

	[Attribute("0", UIWidgets.Slider, "LOITER target faction. Bifrost stores the stable faction key selected by the GM.", "0 3 1", category: "Bifrost"), RplProp()]
	int m_iTargetFaction;

	[RplProp()]
	protected FactionKey m_sTargetFactionKey;

	[Attribute("0", UIWidgets.Slider, "LOITER: which target inside the target faction the orbit/gunrun prioritizes (0 = Infantry, 1 = Vehicles, 2 = Any). Requires a Target Faction.", "0 2 1", category: "Bifrost"), RplProp()]
	int m_iTargetType;

	[Attribute("0", UIWidgets.CheckBox, "LOITER: gunrun fires for the WHOLE time on station, ignoring the Rounds budget.", category: "Bifrost"), RplProp()]
	bool m_bContinuousFire;

	[Attribute("0", UIWidgets.CheckBox, "Start firing after placement.", category: "Bifrost"), RplProp()]
	bool m_bFiring;
	protected bool m_bPendingAttributeFire;

	static const float GROUND_LIFT = 0.1;
	static const float ROCKET_SPAWN_HEIGHT = 150.0;
	static const float MARKER_HEIGHT = 1.5;
	static const int VISUAL_MS = 1000;
	static const float GUNRUN_START_DISTANCE = 350.0;
	static const float LOITER_ORBIT_RADIUS = 250.0;	// orbit circle around the target point.
	static const float LOITER_MAX_ORBIT_SPEED = 37.5;
	static const float LOITER_ORBIT_GAIN = 1.0;	// radial correction toward the holding ring.
	static const float LOITER_TURN_RESPONSE = 1.5;
	static const float LOITER_VERTICAL_RATE = 5.0;	// maximum climb/descent correction while holding the sampled safe altitude.
	static const float LOITER_ENTRY_CAPTURE = 15.0;	// max tangent-entry correction; normal frame crossing is much smaller.
	static const int LOITER_RING_SAMPLES = 24;	// terrain samples around the ring for the fixed orbit altitude.
	static const ResourceName FLYBY_AIRCRAFT = "{70BAEEFC2D3FEE64}Prefabs/Vehicles/Helicopters/UH1H/UH1H.et";
	static const ResourceName GUNRUN_ROUND = "{9CCBDD2ACB73FFA9}Prefabs/Weapons/Ammo/Ammo_762x51_Tracer_M62.et";
	static const ResourceName GUNRUN_ACP = "{FE8279CA292FC6EE}Sounds/Weapons/Machineguns/M60/Weapons_Machineguns_M60_Shot.acp";
	static const string GUNRUN_EVENT = "SOUND_SHOT";
	static const int GUNRUN_DANGER_EVERY = 8;
	static const float GUNRUN_DANGER_RADIUS = 20.0;
	static const float GUNRUN_MUZZLE_CLEARANCE = 4.0;
	static const float GUNRUN_COSMETIC_TRACER_LENGTH = 90.0;
	static const string EXPLOSION_EVENT = "SOUND_EXPLOSION";

	static const int TARGET_TYPE_INFANTRY = 0;
	static const int TARGET_TYPE_VEHICLES = 1;
	static const int TARGET_TYPE_ANY      = 2;

	protected int m_iShotsLeft;
	protected ref Resource m_LoadedWarhead;
	protected EDCO_FxExplosionSize m_eLoadedFor;
	protected ref Resource m_LoadedRocket;
	protected EDCO_FxExplosionDelivery m_eLoadedRocketFor;
	protected ref Shape m_MarkerShape;
	protected ref Shape m_ScatterShape;
	protected ref Shape m_TrackingShape;
	protected ref Shape m_SoundRadiusShape;
	protected ref Resource m_LoadedAircraft;
	protected ResourceName m_LoadedAircraftFor;
	protected ref Resource m_LoadedGunrunRound;
	protected ref array<ref Shape> m_CosmeticTracers = {};
	protected Faction m_FacQueryFaction;
	protected vector m_vFacQueryCenter;
	protected float m_fFacBestSq;
	protected IEntity m_FacBest;
	protected ref array<ref DCO_FxAircraftPass> m_AircraftPasses = {};
	// Passes awaiting teardown.
	protected ref array<ref DCO_FxAircraftPass> m_PendingDeletePasses = {};
	protected bool m_bDeletionFlushQueued;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame() || !GetGame().InPlayMode())
			return;

		if (Replication.IsServer())
		{
			DCO_MigrateTargetFaction();
			if (DCO_ClampDeliveryToFamily())
				DCO_ReplicateState();
		}
		GetGame().GetCallqueue().CallLater(DCO_DrawMarker, 500, false);
		GetGame().GetCallqueue().CallLater(DCO_DrawMarker, VISUAL_MS, true);
		DCO_TriggerFxRegistry.Register(owner);

		if (!Replication.IsServer())
			return;

		m_iShotsLeft = Math.Clamp(m_iBarrageCount, 1, 20);
		if (m_bFiring)
			DCO_ScheduleShot(1000);
	}

	void ~DCO_FxExplosionComponent()
	{
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(DCO_BarrageTick);
			GetGame().GetCallqueue().Remove(DCO_NextBarrage);
			GetGame().GetCallqueue().Remove(DCO_DrawMarker);
			GetGame().GetCallqueue().Remove(DCO_FlushPassDeletions);
			GetGame().GetCallqueue().Remove(DCO_ApplyFiringAfterAttributes);
		}
		if (GetGame() && Replication.IsServer())
		{
			if (m_AircraftPasses)
			{
				foreach (DCO_FxAircraftPass pass : m_AircraftPasses)
					DCO_DeletePass(pass);
			}
			if (m_PendingDeletePasses)
			{
				foreach (DCO_FxAircraftPass dead : m_PendingDeletePasses)
					DCO_DeletePass(dead);
			}
		}
		m_AircraftPasses = null;
		m_PendingDeletePasses = null;
		DCO_TriggerFxRegistry.Unregister(GetOwner());
		m_MarkerShape = null;
		m_ScatterShape = null;
		m_TrackingShape = null;
		m_SoundRadiusShape = null;
	}

	bool DCO_IsFiring() { return m_bFiring; }

	void DCO_SetFiring(bool fire)
	{
		if (!Replication.IsServer())
			return;

		GetGame().GetCallqueue().Remove(DCO_ApplyFiringAfterAttributes);
		m_bPendingAttributeFire = false;
		m_bFiring = fire;
		DCO_ReplicateState();
		GetGame().GetCallqueue().Remove(DCO_BarrageTick);
		GetGame().GetCallqueue().Remove(DCO_NextBarrage);
		if (fire)
		{
			m_iShotsLeft = Math.Clamp(m_iBarrageCount, 1, 20);
			DCO_ScheduleShot(1);
		}
	}

	void DCO_CommitFiringAfterAttributes(bool fire)
	{
		if (!Replication.IsServer())
			return;

		GetGame().GetCallqueue().Remove(DCO_ApplyFiringAfterAttributes);
		m_bPendingAttributeFire = fire;
		if (!fire)
		{
			DCO_SetFiring(false);
			return;
		}
		GetGame().GetCallqueue().CallLater(DCO_ApplyFiringAfterAttributes, 0, false);
	}

	protected void DCO_ApplyFiringAfterAttributes()
	{
		if (!m_bPendingAttributeFire || !Replication.IsServer())
			return;
		m_bPendingAttributeFire = false;
		DCO_SetFiring(true);
	}

	int DCO_GetSize() { return m_eSize; }
	void DCO_SetSize(int s) { m_eSize = Math.Clamp(s, 0, StaticData().m_LiveWarheads.Count() - 1); DCO_ReplicateState(); }
	int DCO_GetDelivery() { return m_eDelivery; }
	void DCO_SetDelivery(int v) { m_eDelivery = Math.Clamp(v, 0, StaticData().m_DeliveryNames.Count() - 1); DCO_ReplicateState(); }

	// FAMILY / delivery scoping.
	int DCO_GetFamily() { return m_eFamily; }
	int DCO_GetFamilyBit() { return 1 << m_eFamily; }

	static void DCO_FamilyDeliveries(int family, out array<int> outOrdinals)
	{
		outOrdinals.Clear();
		if (family == EDCO_FxFamily.AIRSUPPORT)
		{
			outOrdinals.Insert(EDCO_FxExplosionDelivery.HELICOPTER_FLYBY);
			outOrdinals.Insert(EDCO_FxExplosionDelivery.HELICOPTER_GUNRUN);
			return;
		}
		if (family == EDCO_FxFamily.AUDIO)
		{
			outOrdinals.Insert(EDCO_FxExplosionDelivery.SOUND_EMITTER);
			return;
		}
		if (family == EDCO_FxFamily.LOITER)
		{
			outOrdinals.Insert(EDCO_FxExplosionDelivery.LOITER_FLYBY);
			outOrdinals.Insert(EDCO_FxExplosionDelivery.LOITER_GUNRUN);
			return;
		}
		outOrdinals.Insert(EDCO_FxExplosionDelivery.GROUND_WARHEAD);
		outOrdinals.Insert(EDCO_FxExplosionDelivery.HYDRA_M229);
		outOrdinals.Insert(EDCO_FxExplosionDelivery.S5KO);
	}

	int DCO_GetDeliveryIndex()
	{
		array<int> allowed = {};
		DCO_FamilyDeliveries(m_eFamily, allowed);
		int current = m_eDelivery;
		int at = allowed.Find(current);
		if (at < 0)
			return 0;
		return at;
	}

	void DCO_SetDeliveryIndex(int idx)
	{
		array<int> allowed = {};
		DCO_FamilyDeliveries(m_eFamily, allowed);
		if (allowed.IsEmpty())
			return;
		m_eDelivery = allowed[Math.Clamp(idx, 0, allowed.Count() - 1)];
		DCO_ReplicateState();
	}

	protected bool DCO_ClampDeliveryToFamily()
	{
		array<int> allowed = {};
		DCO_FamilyDeliveries(m_eFamily, allowed);
		int current = m_eDelivery;
		if (allowed.IsEmpty() || allowed.Contains(current))
			return false;
		m_eDelivery = allowed[0];
		return true;
	}
	int DCO_GetBarrageCount() { return m_iBarrageCount; }
	void DCO_SetBarrageCount(int v)
	{
		m_iBarrageCount = Math.Clamp(v, 1, 20);
		DCO_ReplicateState();
		if (m_iShotsLeft > m_iBarrageCount)
			m_iShotsLeft = m_iBarrageCount;
	}
	float DCO_GetScatter() { return m_fScatterRadius; }
	void DCO_SetScatter(float v) { m_fScatterRadius = Math.Clamp(v, 0, 200); DCO_ReplicateState(); }
	float DCO_GetShotSpacing() { return m_fShotSpacingSec; }
	void DCO_SetShotSpacing(float v) { m_fShotSpacingSec = Math.Clamp(v, 0.1, 10); DCO_ReplicateState(); }
	float DCO_GetRepeat() { return m_fRepeatSec; }
	void DCO_SetRepeat(float v) { m_fRepeatSec = Math.Clamp(v, 0, 120); DCO_ReplicateState(); }
	bool DCO_GetTrackPlayers() { return m_bTrackPlayers; }
	void DCO_SetTrackPlayers(bool v) { m_bTrackPlayers = v; DCO_ReplicateState(); }
	float DCO_GetTrackingRadius() { return m_fTrackingRadius; }
	void DCO_SetTrackingRadius(float v) { m_fTrackingRadius = Math.Clamp(v, 25, 1000); DCO_ReplicateState(); }
	bool DCO_GetLive() { return m_bLive; }
	void DCO_SetLive(bool v) { m_bLive = v; DCO_ReplicateState(); }
	bool DCO_GetSound() { return m_bSound; }
	void DCO_SetSound(bool v) { m_bSound = v; DCO_ReplicateState(); }
	float DCO_GetParticleScale() { return m_fParticleScale; }
	void DCO_SetParticleScale(float v) { m_fParticleScale = Math.Clamp(v, 0.25, 4); DCO_ReplicateState(); }
	float DCO_GetCustomSoundRadius() { return m_fCustomSoundRadius; }
	void DCO_SetCustomSoundRadius(float v) { m_fCustomSoundRadius = Math.Clamp(v, 25, 3000); DCO_ReplicateState(); }
	float DCO_GetFlybySpeed() { return m_fFlybySpeed; }
	void DCO_SetFlybySpeed(float v) { m_fFlybySpeed = Math.Clamp(v, 25, 100); DCO_ReplicateState(); }
	float DCO_GetFlybyHeight() { return m_fFlybyHeight; }
	void DCO_SetFlybyHeight(float v) { m_fFlybyHeight = Math.Clamp(v, 30, 300); DCO_ReplicateState(); }
	float DCO_GetFlybyDistance() { return m_fFlybyDistance; }
	void DCO_SetFlybyDistance(float v) { m_fFlybyDistance = Math.Clamp(v, 500, 4000); DCO_ReplicateState(); }
	int DCO_GetGunrunRounds() { return m_iGunrunRounds; }
	void DCO_SetGunrunRounds(int v) { m_iGunrunRounds = Math.Clamp(v, 5, 100); DCO_ReplicateState(); }
	float DCO_GetGunrunRpm() { return m_fGunrunRpm; }
	void DCO_SetGunrunRpm(float v) { m_fGunrunRpm = Math.Clamp(v, 300, 1200); DCO_ReplicateState(); }
	float DCO_GetLoiterSec() { return m_fLoiterSec; }
	void DCO_SetLoiterSec(float v) { m_fLoiterSec = Math.Clamp(v, 10, 600); DCO_ReplicateState(); }
	int DCO_GetAircraftIndex() { return DCO_FxAircraftCatalog.IndexOf(m_sAircraftPrefab); }
	void DCO_SetAircraftIndex(int i) { m_sAircraftPrefab = DCO_FxAircraftCatalog.PrefabAt(i); DCO_ReplicateState(); }
	bool DCO_GetContinuousFire() { return m_bContinuousFire; }
	void DCO_SetContinuousFire(bool v) { m_bContinuousFire = v; DCO_ReplicateState(); }
	int DCO_GetTargetFaction()
	{
		DCO_MigrateTargetFaction();
		if (m_sTargetFactionKey.IsEmpty())
			return 0;
		int index = DCO_FactionCatalog.IndexOf(m_sTargetFactionKey);
		if (index < 0)
			return DCO_FactionCatalog.TargetCount();
		return index + 1;
	}
	void DCO_SetTargetFaction(int v)
	{
		FactionKey key = DCO_FactionCatalog.TargetKeyAt(v);
		if (v > 0 && key.IsEmpty())
			return;
		m_sTargetFactionKey = key;
		m_iTargetFaction = 0;
		DCO_ReplicateState();
	}
	FactionKey DCO_GetTargetFactionKey() { DCO_MigrateTargetFaction(); return m_sTargetFactionKey; }
	int DCO_GetTargetType() { return m_iTargetType; }
	void DCO_SetTargetType(int v) { m_iTargetType = Math.Clamp(v, 0, StaticData().m_TargetTypeNames.Count() - 1); DCO_ReplicateState(); }

	protected void DCO_MigrateTargetFaction()
	{
		if (!Replication.IsServer() || !m_sTargetFactionKey.IsEmpty() || m_iTargetFaction <= 0)
			return;
		array<FactionKey> legacyKeys = {"", "US", "USSR", "FIA"};
		if (legacyKeys.IsIndexValid(m_iTargetFaction))
		{
			m_sTargetFactionKey = legacyKeys[m_iTargetFaction];
			DCO_ReplicateState();
		}
	}

	protected void DCO_ReplicateState()
	{
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	protected void DCO_ScheduleShot(int delayMs)
	{
		GetGame().GetCallqueue().Remove(DCO_BarrageTick);
		GetGame().GetCallqueue().CallLater(DCO_BarrageTick, delayMs, false);
	}

	protected void DCO_BarrageTick()
	{
		if (!m_bFiring || !Replication.IsServer())
			return;

		DCO_DetonateAt(DCO_ResolveTarget());
		m_iShotsLeft--;
		if (m_iShotsLeft > 0)
		{
			DCO_ScheduleShot(Math.Round(Math.Clamp(m_fShotSpacingSec, 0.1, 10) * 1000.0));
			return;
		}

		if (m_fRepeatSec > 0)
		{
			GetGame().GetCallqueue().CallLater(DCO_NextBarrage, Math.Round(m_fRepeatSec * 1000.0), false);
		}
		else
		{
			m_bFiring = false;
			DCO_ReplicateState();
			m_iShotsLeft = m_iBarrageCount;
		}
	}

	protected void DCO_NextBarrage()
	{
		if (!m_bFiring || !Replication.IsServer())
			return;
		m_iShotsLeft = Math.Clamp(m_iBarrageCount, 1, 20);
		DCO_ScheduleShot(1);
	}

	protected vector DCO_ResolveTarget()
	{
		IEntity owner = GetOwner();
		vector emitterCenter = owner.GetOrigin();
		vector center = emitterCenter;

		if (m_bTrackPlayers)
		{
			PlayerManager playerManager = GetGame().GetPlayerManager();
			if (playerManager)
			{
				array<int> playerIds = {};
				array<IEntity> candidates = {};
				playerManager.GetPlayers(playerIds);
				foreach (int playerId : playerIds)
				{
					IEntity controlled = playerManager.GetPlayerControlledEntity(playerId);
					if (controlled && vector.Distance(center, controlled.GetOrigin()) <= m_fTrackingRadius)
						candidates.Insert(controlled);
				}
				if (!candidates.IsEmpty())
					center = candidates[Math.RandomInt(0, candidates.Count())].GetOrigin();
			}
		}

		DCO_MigrateTargetFaction();
		if (!m_sTargetFactionKey.IsEmpty())
		{
			vector facPos;
			if (DCO_FindNearestFactionMember(emitterCenter, facPos))
				center = facPos;
			else
				center = emitterCenter;
		}

		vector target = center;
		if (m_fScatterRadius > 0)
			target = SCR_Math2D.GenerateRandomPointInRadius(0, m_fScatterRadius, Vector(center[0], 0, center[2]), false);
		target[1] = owner.GetWorld().GetSurfaceY(target[0], target[2]) + GROUND_LIFT;
		return target;
	}

	protected bool DCO_FindNearestFactionMember(vector center, out vector outPos)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return false;
		m_FacQueryFaction = fm.GetFactionByKey(m_sTargetFactionKey);
		if (!m_FacQueryFaction)
			return false;
		m_vFacQueryCenter = center;
		m_fFacBestSq = 0;
		m_FacBest = null;
		GetOwner().GetWorld().QueryEntitiesBySphere(center, Math.Clamp(m_fTrackingRadius, 25, 1000), DCO_FactionQueryCallback);
		if (!m_FacBest)
			return false;
		outPos = m_FacBest.GetOrigin();
		return true;
	}

	protected bool DCO_FactionQueryCallback(IEntity e)
	{
		bool wantInfantry = (m_iTargetType != TARGET_TYPE_VEHICLES);
		bool wantVehicles = (m_iTargetType != TARGET_TYPE_INFANTRY);
		vector entPos;
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (ch)
		{
			if (!wantInfantry)
				return true;
			CharacterControllerComponent cc = ch.GetCharacterController();
			if (cc && cc.IsDead())
				return true;
			entPos = ch.GetOrigin();
		}
		else
		{
			if (!wantVehicles || !Vehicle.Cast(e))
				return true;
			entPos = e.GetOrigin();
		}
		// Faction match.
		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
		if (!fac || fac.GetAffiliatedFaction() != m_FacQueryFaction)
			return true;
		float dSq = vector.DistanceSq(entPos, m_vFacQueryCenter);
		if (!m_FacBest || dSq < m_fFacBestSq)
		{
			m_fFacBestSq = dSq;
			m_FacBest = e;
		}
		return true;
	}

	protected void DCO_DetonateAt(vector pos)
	{
		int sizeIdx = Math.Clamp(m_eSize, 0, StaticData().m_LiveWarheads.Count() - 1);
		int deliveryIdx = Math.Clamp(m_eDelivery, 0, StaticData().m_DeliveryNames.Count() - 1);

		if (deliveryIdx == EDCO_FxExplosionDelivery.SOUND_EMITTER)
		{
			string bank = m_CustomSoundBank;
			string eventName = m_sCustomSoundEvent;
			if (!bank.IsEmpty() && !eventName.IsEmpty())
			{
				RpcDo_DCO_CustomSound(pos, bank, eventName);
				Rpc(RpcDo_DCO_CustomSound, pos, bank, eventName);
			}
			return;
		}

		bool isFlyby = deliveryIdx == EDCO_FxExplosionDelivery.HELICOPTER_FLYBY
			|| deliveryIdx == EDCO_FxExplosionDelivery.HELICOPTER_GUNRUN;
		bool isOrbit = deliveryIdx == EDCO_FxExplosionDelivery.LOITER_FLYBY
			|| deliveryIdx == EDCO_FxExplosionDelivery.LOITER_GUNRUN;
		if (isFlyby || isOrbit)
		{
			bool gunrun = deliveryIdx == EDCO_FxExplosionDelivery.HELICOPTER_GUNRUN
				|| deliveryIdx == EDCO_FxExplosionDelivery.LOITER_GUNRUN;
			DCO_StartAircraftPass(gunrun, pos, isOrbit);
			return;
		}

		if (m_bLive)
		{
			if (deliveryIdx == EDCO_FxExplosionDelivery.GROUND_WARHEAD)
				DCO_DetonateLiveGround(sizeIdx, pos);
			else
				DCO_LaunchLiveRocket(deliveryIdx, pos);
			return;
		}

		int soundFlag = 0;
		if (m_bSound)
			soundFlag = 1;
		string customPtc = m_CustomParticle;
		RpcDo_DCO_CosmeticBoom(pos, sizeIdx, soundFlag, m_fParticleScale, customPtc);
		Rpc(RpcDo_DCO_CosmeticBoom, pos, sizeIdx, soundFlag, m_fParticleScale, customPtc);
	}

	protected void DCO_DetonateLiveGround(int sizeIdx, vector pos)
	{
		IEntity owner = GetOwner();
		if (!m_LoadedWarhead || m_eLoadedFor != m_eSize)
		{
			m_LoadedWarhead = Resource.Load(StaticData().m_LiveWarheads[sizeIdx]);
			m_eLoadedFor = m_eSize;
		}
		if (!m_LoadedWarhead)
		{
			Print("[DCO-FX] explosion emitter: warhead prefab failed to load - strike stopped", LogLevel.WARNING);
			m_bFiring = false;
			DCO_ReplicateState();
			return;
		}

		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(sp.Transform);
		sp.Transform[3] = pos;
		IEntity boom = GetGame().SpawnEntityPrefab(m_LoadedWarhead, owner.GetWorld(), sp);
		if (!boom)
		{
			Print("[DCO-FX] explosion emitter: warhead spawn FAILED", LogLevel.WARNING);
			return;
		}
		BaseTriggerComponent trigger = BaseTriggerComponent.Cast(boom.FindComponent(BaseTriggerComponent));
		if (trigger)
			trigger.OnUserTrigger(boom);
		else
			Print("[DCO-FX] explosion emitter: spawned warhead has no trigger component", LogLevel.WARNING);
	}


	protected void DCO_StartAircraftPass(bool gunrun, vector target, bool loiter = false)
	{
		IEntity owner = GetOwner();
		if (!owner || m_AircraftPasses.Count() >= 4)
		{
			Print("[DCO-FX] aircraft pass rejected: active-pass safety limit reached", LogLevel.WARNING);
			return;
		}
		ResourceName aircraftPrefab = DCO_FxAircraftCatalog.PrefabAt(DCO_GetAircraftIndex());
		if (!m_LoadedAircraft || m_LoadedAircraftFor != aircraftPrefab)
		{
			m_LoadedAircraft = Resource.Load(aircraftPrefab);
			m_LoadedAircraftFor = aircraftPrefab;
		}
		if (!m_LoadedAircraft)
		{
			Print("[DCO-FX] aircraft prefab failed to load", LogLevel.WARNING);
			return;
		}

		vector ownerMat[4];
		owner.GetWorldTransform(ownerMat);
		vector direction = ownerMat[2];
		direction[1] = 0;
		direction.Normalize();
		float distance = Math.Clamp(m_fFlybyDistance, 500, 4000);
		float cruiseSpeed = Math.Clamp(m_fFlybySpeed, 25, 100);

		// A flyby is aimed THROUGH the effect point.
		vector orbitEntry = target;
		if (loiter)
		{
			vector entryRadial = Vector(-direction[2], 0, direction[0]);
			orbitEntry = target + entryRadial * LOITER_ORBIT_RADIUS;
		}
		vector start = orbitEntry - direction * distance;
		float maxTerrainY = target[1];
		float approachLength = distance;
		if (!loiter)
			approachLength = distance * 2.0;
		for (int sample = 0; sample <= 20; sample++)
		{
			vector samplePos = start + direction * (approachLength * sample / 20.0);
			maxTerrainY = Math.Max(maxTerrainY, owner.GetWorld().GetSurfaceY(samplePos[0], samplePos[2]));
		}
		if (loiter)
		{
			// One fixed safe altitude for the whole ring.
			for (int ringSample = 0; ringSample < LOITER_RING_SAMPLES; ringSample++)
			{
				float ringAngle = (ringSample / (float)LOITER_RING_SAMPLES) * 6.2831853;
				vector ringPos = target + Vector(Math.Cos(ringAngle) * LOITER_ORBIT_RADIUS, 0, Math.Sin(ringAngle) * LOITER_ORBIT_RADIUS);
				maxTerrainY = Math.Max(maxTerrainY, owner.GetWorld().GetSurfaceY(ringPos[0], ringPos[2]));
			}
		}
		float flightY = maxTerrainY + Math.Clamp(m_fFlybyHeight, 30, 300);
		start[1] = flightY;
		orbitEntry[1] = flightY;

		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixFromForwardVec(direction, sp.Transform);
		sp.Transform[3] = start;
		IEntity aircraft = GetGame().SpawnEntityPrefab(m_LoadedAircraft, owner.GetWorld(), sp);
		if (!aircraft)
		{
			Print("[DCO-FX] aircraft pass spawn FAILED", LogLevel.WARNING);
			return;
		}
		if (!DCO_FxAircraftCatalog.IsSupportedHelicopter(aircraft))
		{
			Print("[DCO-FX] aircraft pass rejected: selected prefab is not a supported helicopter", LogLevel.WARNING);
			SCR_EntityHelper.DeleteEntityAndChildren(aircraft);
			return;
		}
		Physics physics = aircraft.GetPhysics();
		// Never disable a vehicle's simulation while the compartment manager is spawning/moving occupants.
		physics.ChangeSimulationState(SimulationState.SIMULATION);
		physics.SetVelocity(Vector(0, 0, 0));

		DCO_FxAircraftPass pass = new DCO_FxAircraftPass();
		pass.m_Aircraft = aircraft;
		pass.m_Direction = direction;
		pass.m_Start = start;
		pass.m_Target = target;
		pass.m_vOrbitEntry = orbitEntry;
		pass.m_vHeading = direction;
		pass.m_fCruiseSpeed = cruiseSpeed;
		pass.m_fOrbitY = flightY;
		pass.m_fOrbitSpeed = Math.Min(cruiseSpeed, LOITER_MAX_ORBIT_SPEED);
		pass.m_bGunrun = gunrun;
		pass.m_bLoiter = loiter;
		pass.m_bContinuousFire = loiter && gunrun && m_bContinuousFire;
		if (pass.m_bContinuousFire)
			pass.m_iRoundsLeft = 0;	// deliberately no ammo budget; Time on Station is the sole stop condition.
		else
			pass.m_iRoundsLeft = Math.Clamp(m_iGunrunRounds, 5, 100);
		// Hard watchdog: full ingress + egress with 50% margin, plus the complete station budget.
		pass.m_fLifeSec = (distance * 2.0 / cruiseSpeed) * 1.5;
		if (loiter)
		{
			pass.m_fStationSec = Math.Clamp(m_fLoiterSec, 10, 600);
			pass.m_fLifeSec += pass.m_fStationSec;
		}
		m_AircraftPasses.Insert(pass);

		// Crew is mission-ready at launch: ask engine to fill every PILOT and TURRET slot configured by the selected helicopter.
		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(aircraft.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (compartments)
		{
			compartments.GetOnDoneSpawningDefaultOccupants().Insert(DCO_OnPassCrewSpawned);
			bool crewAccepted = compartments.SpawnDefaultOccupants(StaticData().m_AircraftCrewTypes);
			if (!crewAccepted)
			{
				compartments.GetOnDoneSpawningDefaultOccupants().Remove(DCO_OnPassCrewSpawned);
				pass.m_bCrewReady = true;
			}
		}
		else
		{
			Print("[DCO-FX] aircraft has no compartment manager - cannot pre-seat flight crew", LogLevel.WARNING);
			pass.m_bCrewReady = true;
		}

		HelicopterControllerComponent controller = HelicopterControllerComponent.Cast(aircraft.FindComponent(HelicopterControllerComponent));
		if (controller)
		{
			if (!controller.IsEngineOn())
			{
				ForceStartEngineParams startParams = new ForceStartEngineParams();
				startParams.m_bAirborne = true;
				controller.ForceStartEngine(startParams);
			}
		}
		else
		{
			VehicleHelicopterSimulation sim = VehicleHelicopterSimulation.Cast(aircraft.FindComponent(VehicleHelicopterSimulation));
			if (sim)
				sim.EngineStart();
		}

		DCO_EnableAircraftFrame();
	}

	protected void DCO_OnPassCrewSpawned(SCR_BaseCompartmentManagerComponent manager, array<IEntity> spawned, bool wasCanceled)
	{
		if (!manager)
			return;
		manager.GetOnDoneSpawningDefaultOccupants().Remove(DCO_OnPassCrewSpawned);
		if (wasCanceled || !spawned || !m_AircraftPasses)
		{
			Print("[DCO-FX] aircraft crew spawn canceled", LogLevel.WARNING);
			return;
		}
		IEntity airframe = manager.GetOwner();
		foreach (DCO_FxAircraftPass pass : m_AircraftPasses)
		{
			if (!pass || pass.m_Aircraft != airframe)
				continue;
			foreach (IEntity crew : spawned)
			{
				if (!crew)
					continue;
				pass.m_aCrew.Insert(crew);
				DCO_SilenceTurretGunner(crew);
			}
			pass.m_bCrewReady = true;
			return;
		}

		Print("[DCO-FX] aircraft crew finished spawning after its pass was gone - deleting orphaned occupants", LogLevel.WARNING);
		foreach (IEntity crew : spawned)
		{
			if (crew)
				RplComponent.DeleteRplEntity(crew, false);
		}
	}

	protected bool DCO_SilenceTurretGunner(IEntity crew)
	{
		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(crew.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
			return false;
		BaseCompartmentSlot slot = access.GetCompartment();
		if (!slot || slot.GetType() != ECompartmentType.TURRET)
			return false;
		AIControlComponent ai = AIControlComponent.Cast(crew.FindComponent(AIControlComponent));
		if (!ai)
			return false;
		ai.DeactivateAI();
		return true;
	}

	protected void DCO_ReassertGunnerSilence(DCO_FxAircraftPass pass)
	{
		if (!pass || !pass.m_aCrew)
			return;
		foreach (IEntity crew : pass.m_aCrew)
		{
			if (crew)
				DCO_SilenceTurretGunner(crew);
		}
	}

	// Delete a pass's airframe AND its spawned crew.
	protected void DCO_DeletePass(DCO_FxAircraftPass pass)
	{
		if (!pass)
			return;
		if (pass.m_aCrew)
		{
			foreach (IEntity crew : pass.m_aCrew)
			{
				if (crew)
					RplComponent.DeleteRplEntity(crew, false);
			}
			pass.m_aCrew.Clear();
		}
		if (pass.m_Aircraft)
			RplComponent.DeleteRplEntity(pass.m_Aircraft, false);
	}

	// Move a finished/invalid pass out of the live list and schedule its teardown for AFTER the pump loop.
	protected void DCO_QueuePassDeletion(int index)
	{
		if (!m_AircraftPasses || index < 0 || index >= m_AircraftPasses.Count())
			return;
		DCO_FxAircraftPass pass = m_AircraftPasses[index];
		m_AircraftPasses.Remove(index);
		if (!pass || !m_PendingDeletePasses)
			return;
		m_PendingDeletePasses.Insert(pass);
		if (!m_bDeletionFlushQueued && GetGame() && GetGame().GetCallqueue())
		{
			m_bDeletionFlushQueued = true;
			GetGame().GetCallqueue().CallLater(DCO_FlushPassDeletions, 0, false);
		}
	}

	protected void DCO_FlushPassDeletions()
	{
		m_bDeletionFlushQueued = false;
		if (!m_PendingDeletePasses)
			return;
		foreach (DCO_FxAircraftPass pass : m_PendingDeletePasses)
			DCO_DeletePass(pass);
		m_PendingDeletePasses.Clear();
	}

	// Render-frame aircraft pump.
	protected bool m_bAircraftFrameOn;

	protected void DCO_EnableAircraftFrame()
	{
		if (m_bAircraftFrameOn)
			return;
		IEntity owner = GetOwner();
		if (!owner)
			return;
		SetEventMask(owner, GetEventMask() | EntityEvent.FRAME);
		m_bAircraftFrameOn = true;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!m_AircraftPasses || m_AircraftPasses.IsEmpty())
			return;
		DCO_AircraftFrame(Math.Clamp(timeSlice, 0.001, 0.1));
	}

	protected void DCO_AircraftFrame(float dt)
	{
		if (!Replication.IsServer() || !m_AircraftPasses)
			return;
		for (int i = m_AircraftPasses.Count() - 1; i >= 0; i--)
		{
			DCO_FxAircraftPass pass = m_AircraftPasses[i];
			if (!pass || !pass.m_Aircraft || !pass.m_Aircraft.GetPhysics())
			{
				DCO_QueuePassDeletion(i);
				continue;
			}
			if (!pass.m_bCrewReady)
			{
				// Hold the airframe under normal simulation.
				Physics holdPhysics = pass.m_Aircraft.GetPhysics();
				if (holdPhysics)
				{
					vector holdVelocity = Vector(0, 0, 0);
					holdVelocity[1] = Math.Clamp(pass.m_fOrbitY - pass.m_Aircraft.GetOrigin()[1], -LOITER_VERTICAL_RATE, LOITER_VERTICAL_RATE);
					holdPhysics.SetVelocity(holdVelocity);
				}
				pass.m_fCrewWaitSec -= dt;
				if (pass.m_fCrewWaitSec > 0)
					continue;
				pass.m_bCrewReady = true;
				Print("[DCO-FX] aircraft crew spawn timed out - launching with available occupants", LogLevel.WARNING);
			}
			pass.m_fLifeSec -= dt;
			if (pass.m_fLifeSec <= 0)
			{
				Print("[DCO-FX] aircraft pass timed out (stalled airframe) - force-cleaned", LogLevel.WARNING);
				DCO_QueuePassDeletion(i);
				continue;
			}
			if (pass.m_bLoiter)
			{
				if (DCO_LoiterFrame(pass, dt))
					DCO_QueuePassDeletion(i);
				continue;
			}
			IEntity aircraft = pass.m_Aircraft;
			Physics physics = aircraft.GetPhysics();
			vector velocity = pass.m_Direction * pass.m_fCruiseSpeed;
			if (physics)
				physics.SetVelocity(velocity);

			float travelled = vector.DistanceXZ(pass.m_Start, aircraft.GetOrigin());
			float remaining = vector.DistanceXZ(aircraft.GetOrigin(), pass.m_Target);
			if (pass.m_bGunrun && !pass.m_bGunrunStarted && remaining <= GUNRUN_START_DISTANCE)
				pass.m_bGunrunStarted = true;
			if (pass.m_bGunrunStarted && pass.m_iRoundsLeft > 0)
			{
				pass.m_fShotClockMs -= dt * 1000.0;
				if (pass.m_fShotClockMs <= 0)
				{
					DCO_FireGunrunRound(pass, velocity);
					pass.m_iRoundsLeft--;
					pass.m_fShotClockMs = 60000.0 / Math.Clamp(m_fGunrunRpm, 300, 1200);
				}
			}
			if (travelled >= Math.Clamp(m_fFlybyDistance, 500, 4000) * 2.0)
				DCO_QueuePassDeletion(i);
		}
	}

	// LOITER mover: tangent ingress -> constant-radius/constant-altitude orbit -> tangent egress.
	protected bool DCO_LoiterFrame(DCO_FxAircraftPass pass, float dt)
	{
		IEntity aircraft = pass.m_Aircraft;
		float speed = pass.m_fCruiseSpeed;
		Physics physics = aircraft.GetPhysics();

		if (!pass.m_bOnStation && !pass.m_bDeparting)
		{
			// Straight ingress to the authored tangent point, not to the target center.
			vector cur = aircraft.GetOrigin();
			vector ingressVelocity = pass.m_Direction * speed;
			ingressVelocity[1] = Math.Clamp(pass.m_fOrbitY - cur[1], -LOITER_VERTICAL_RATE, LOITER_VERTICAL_RATE);
			if (physics)
				physics.SetVelocity(ingressVelocity);
			float entryRemaining = vector.DistanceXZ(cur, pass.m_vOrbitEntry);
			float crossedEntry = vector.Dot(cur - pass.m_vOrbitEntry, pass.m_Direction);
			float captureDistance = Math.Max(LOITER_ENTRY_CAPTURE, speed * dt * 2.0);
			if (entryRemaining <= captureDistance || crossedEntry >= 0)
			{
				pass.m_bOnStation = true;
				pass.m_vHeading = pass.m_Direction;
				pass.m_bGunrunStarted = pass.m_bGunrun;
			}
			return false;
		}

		if (pass.m_bOnStation)
		{
			vector cur = aircraft.GetOrigin();
			vector toCenter = pass.m_Target - cur;
			toCenter[1] = 0;
			float orbitDistance = toCenter.Length();
			vector inward;
			if (orbitDistance > 0.001)
				inward = toCenter / orbitDistance;
			else
				inward = Vector(1, 0, 0);
			vector tangent = Vector(-inward[2], 0, inward[0]) * pass.m_fOrbitSign;
			float radialError = Math.Clamp((orbitDistance - LOITER_ORBIT_RADIUS) / LOITER_ORBIT_RADIUS, -1.0, 1.0);
			vector desiredHeading = tangent + inward * (radialError * LOITER_ORBIT_GAIN);
			if (desiredHeading.LengthSq() > 0.0001)
				desiredHeading.Normalize();
			else
				desiredHeading = tangent;
			// Ease the flight direction toward the tangent instead of snapping to it each frame.
			vector heading = pass.m_vHeading;
			heading[1] = 0;
			if (heading.LengthSq() < 0.0001)
				heading = desiredHeading;
			else
				heading.Normalize();
			float turn = Math.Clamp(dt * LOITER_TURN_RESPONSE, 0.0, 1.0);
			heading = heading + (desiredHeading - heading) * turn;
			if (heading.LengthSq() > 0.0001)
				heading.Normalize();
			else
				heading = desiredHeading;
			pass.m_vHeading = heading;
			float orbitSpeed = pass.m_fOrbitSpeed;
			vector orbitVelocity = heading * orbitSpeed;
			orbitVelocity[1] = Math.Clamp(pass.m_fOrbitY - cur[1], -LOITER_VERTICAL_RATE, LOITER_VERTICAL_RATE);
			if (physics)
				physics.SetVelocity(orbitVelocity);

			if (pass.m_bGunrunStarted && (pass.m_bContinuousFire || pass.m_iRoundsLeft > 0))
			{
				pass.m_fShotClockMs -= dt * 1000.0;
				if (pass.m_fShotClockMs <= 0)
				{
					DCO_FireGunrunRound(pass, orbitVelocity);
					if (!pass.m_bContinuousFire)
						pass.m_iRoundsLeft--;
					pass.m_fShotClockMs = 60000.0 / Math.Clamp(m_fGunrunRpm, 300, 1200);
				}
			}

			pass.m_fStationSec -= dt;
			if (pass.m_fStationSec <= 0)
			{
				pass.m_bOnStation = false;
				pass.m_bDeparting = true;
				pass.m_Direction = pass.m_vHeading;	// tangent egress: continue the current flight direction.
				pass.m_Start = cur;
			}
			return false;
		}

		// Tangent egress under the same live simulation.
		vector exitPos = aircraft.GetOrigin();
		vector exitVelocity = pass.m_Direction * speed;
		exitVelocity[1] = Math.Clamp(pass.m_fOrbitY - exitPos[1], -LOITER_VERTICAL_RATE, LOITER_VERTICAL_RATE);
		if (physics)
			physics.SetVelocity(exitVelocity);
		return vector.DistanceXZ(exitPos, pass.m_Start) >= Math.Clamp(m_fFlybyDistance, 500, 4000);
	}

	protected void DCO_FireGunrunRound(DCO_FxAircraftPass pass, vector aircraftVelocity)
	{
		vector target = SCR_Math2D.GenerateRandomPointInRadius(0, Math.Max(5, m_fScatterRadius), Vector(pass.m_Target[0], 0, pass.m_Target[2]), false);
		target[1] = GetOwner().GetWorld().GetSurfaceY(target[0], target[2]);
		// Keep live ammunition clear of the aircraft collision volume.
		vector origin = pass.m_Aircraft.GetOrigin() - Vector(0, 1.5, 0);
		vector direction = target - origin;
		direction.Normalize();
		vector muzzle = origin + direction * GUNRUN_MUZZLE_CLEARANCE;

		if (m_bSound)
		{
			RpcDo_DCO_GunrunSound(muzzle);
			Rpc(RpcDo_DCO_GunrunSound, muzzle);
		}
		pass.m_iGunrunShotsFired++;
		// Broadcast danger periodically so nearby AI reacts to the pass.
		if ((pass.m_iGunrunShotsFired % GUNRUN_DANGER_EVERY) == 0)
		{
			DCO_BroadcastGunrunDanger(target);
			DCO_ReassertGunnerSilence(pass);	// keep door gunners deactivated - LOD can re-wake them near the enemy.
		}

		if (!m_bLive)
		{
			RpcDo_DCO_CosmeticTracer(muzzle, direction);
			Rpc(RpcDo_DCO_CosmeticTracer, muzzle, direction);
			return;
		}

		if (!m_LoadedGunrunRound)
			m_LoadedGunrunRound = Resource.Load(GUNRUN_ROUND);
		if (!m_LoadedGunrunRound)
			return;

		IEntity projectile = DCO_SpawnProjectile(m_LoadedGunrunRound, muzzle, direction);
		if (!projectile)
			return;
		DCO_LaunchProjectile(projectile, direction, aircraftVelocity, pass.m_Aircraft);
	}

	protected void DCO_BroadcastGunrunDanger(vector center)
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;
		SCR_AIDangerEvent_UnsafeArea danger = new SCR_AIDangerEvent_UnsafeArea();
		danger.SetDangerType(EAIDangerEventType.Danger_UnsafeArea);
		danger.SetPosition(center);
		danger.SetRadius(Math.Max(GUNRUN_DANGER_RADIUS, m_fScatterRadius + 10.0));
		aiWorld.RequestBroadcastDangerEvent(danger);
	}


	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_DCO_CustomSound(vector pos, string bankName, string eventName)
	{
		if (System.IsConsoleApp() || bankName.IsEmpty() || eventName.IsEmpty())
			return;
		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		mat[3] = pos;
		ResourceName bank = bankName;
		AudioSystem.PlayEvent(bank, eventName, mat);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	protected void RpcDo_DCO_GunrunSound(vector pos)
	{
		if (System.IsConsoleApp())
			return;
		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		mat[3] = pos;
		AudioSystem.PlayEvent(GUNRUN_ACP, GUNRUN_EVENT, mat);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	protected void RpcDo_DCO_CosmeticTracer(vector muzzle, vector direction)
	{
		if (System.IsConsoleApp())
			return;
		vector points[2];
		points[0] = muzzle;
		points[1] = muzzle + direction * GUNRUN_COSMETIC_TRACER_LENGTH;
		Shape tracer = Shape.CreateLines(0xFFFFC66D, ShapeFlags.NOZBUFFER, points, 2);
		if (tracer)
		{
			m_CosmeticTracers.Insert(tracer);
			GetGame().GetCallqueue().CallLater(DCO_ClearCosmeticTracers, 80, false);
		}
	}

	protected void DCO_ClearCosmeticTracers()
	{
		m_CosmeticTracers.Clear();
	}

	protected IEntity DCO_SpawnProjectile(Resource projectileResource, vector position, vector direction)
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixFromForwardVec(direction, spawnParams.Transform);
		spawnParams.Transform[3] = position;
		return GetGame().SpawnEntityPrefab(projectileResource, GetOwner().GetWorld(), spawnParams);
	}

	protected bool DCO_LaunchProjectile(IEntity projectile, vector direction, vector inheritedVelocity, IEntity parent)
	{
		BaseTriggerComponent trigger = BaseTriggerComponent.Cast(projectile.FindComponent(BaseTriggerComponent));
		if (trigger)
			trigger.SetLive();

		ProjectileMoveComponent move = ProjectileMoveComponent.Cast(projectile.FindComponent(ProjectileMoveComponent));
		if (!move)
		{
			RplComponent.DeleteRplEntity(projectile, false);
			return false;
		}

		move.Launch(direction, inheritedVelocity, 1.0, projectile, null, parent, null, null);
		return true;
	}

	protected void DCO_LaunchLiveRocket(int deliveryIdx, vector target)
	{
		IEntity owner = GetOwner();
		int rocketIdx = deliveryIdx - 1;
		if (rocketIdx < 0 || rocketIdx >= StaticData().m_RocketPrefabs.Count())
			return;

		if (!m_LoadedRocket || m_eLoadedRocketFor != m_eDelivery)
		{
			m_LoadedRocket = Resource.Load(StaticData().m_RocketPrefabs[rocketIdx]);
			m_eLoadedRocketFor = m_eDelivery;
		}
		if (!m_LoadedRocket)
		{
			Print("[DCO-FX] explosion emitter: rocket prefab failed to load - strike stopped", LogLevel.WARNING);
			m_bFiring = false;
			DCO_ReplicateState();
			return;
		}

		vector spawnPos = target + Vector(0, ROCKET_SPAWN_HEIGHT, 0);
		vector direction = target - spawnPos;
		direction.Normalize();
		IEntity rocket = DCO_SpawnProjectile(m_LoadedRocket, spawnPos, direction);
		if (!rocket)
		{
			Print("[DCO-FX] explosion emitter: rocket spawn FAILED", LogLevel.WARNING);
			return;
		}

		if (!DCO_LaunchProjectile(rocket, direction, vector.Zero, owner))
		{
			Print("[DCO-FX] explosion emitter: rocket has no ProjectileMoveComponent", LogLevel.WARNING);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_DCO_CosmeticBoom(vector pos, int sizeIdx, int soundFlag, float particleScale, string customPtc)
	{
		if (System.IsConsoleApp())
			return;
		sizeIdx = Math.Clamp(sizeIdx, 0, StaticData().m_CosmeticParticles.Count() - 1);
		ResourceName particle = StaticData().m_CosmeticParticles[sizeIdx];
		if (!customPtc.IsEmpty())
			particle = customPtc;

		ParticleEffectEntitySpawnParams pp = new ParticleEffectEntitySpawnParams();
		pp.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(pp.Transform);
		pp.Transform[3] = pos;
		ParticleEffectEntity effect = ParticleEffectEntity.SpawnParticleEffect(particle, pp);
		if (effect)
			effect.SetScale(Math.Clamp(particleScale, 0.25, 4));

		if (soundFlag > 0)
		{
			vector mat[4];
			Math3D.MatrixIdentity4(mat);
			mat[3] = pos;
			AudioSystem.PlayEvent(StaticData().m_CosmeticAcps[sizeIdx], EXPLOSION_EVENT, mat);
		}
	}

	protected void DCO_DrawMarker()
	{
		// GM-only: players must not see strike markers/rings, and a dedicated server has no renderer.
		if (!DCO_GMRights.IsLocalGameMaster())
		{
			m_MarkerShape = null;
			m_ScatterShape = null;
			m_TrackingShape = null;
			m_SoundRadiusShape = null;
			return;
		}

		IEntity owner = GetOwner();
		if (!owner)
			return;
		int color = 0xFFD9892B;
		if (m_bLive)
			color = 0xFFFF3030;
		vector base = owner.GetOrigin();
		vector pts[2];
		pts[0] = base;
		pts[1] = base + Vector(0, MARKER_HEIGHT, 0);
		m_MarkerShape = Shape.CreateLines(color, ShapeFlags.NOZBUFFER, pts, 2);
		m_ScatterShape = null;
		m_TrackingShape = null;
		m_SoundRadiusShape = null;
		if (m_fScatterRadius > 0)
			m_ScatterShape = DCO_ZoneShape.FlatCircle(base, m_fScatterRadius, color);
		if (m_bTrackPlayers)
			m_TrackingShape = DCO_ZoneShape.FlatCircle(base, m_fTrackingRadius, 0xFF3FBFD9);
		if (m_eDelivery == EDCO_FxExplosionDelivery.SOUND_EMITTER && !m_CustomSoundBank.IsEmpty())
			m_SoundRadiusShape = DCO_ZoneShape.FlatCircle(base, Math.Clamp(m_fCustomSoundRadius, 25, 3000), 0x6680D8FF);
		else if (m_bLive || m_bSound)
		{
			int sizeIdx = Math.Clamp(m_eSize, 0, StaticData().m_NominalSoundRadius.Count() - 1);
			m_SoundRadiusShape = DCO_ZoneShape.FlatCircle(base, StaticData().m_NominalSoundRadius[sizeIdx], 0x6680D8FF);
		}
	}
}
