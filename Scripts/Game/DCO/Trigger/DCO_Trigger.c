enum EDCO_TriggerCondition
{
	ANY_CHARACTER,	// any AI- or player-controlled character inside.
	PLAYERS_ONLY,	// only player-controlled characters count.
	FACTION_US,
	FACTION_USSR,
	FACTION_FIA,
	FACTION_CIV,
}

enum EDCO_TriggerShape
{
	ELLIPSE,
	RECTANGLE,
}

enum EDCO_TriggerActivation
{
	PRESENT,
	NOT_PRESENT,
}

enum EDCO_TriggerOwnerMode
{
	AREA_FILTER,
	SYNCED_LEADER,
	SYNCED_ANY_MEMBER,
	SYNCED_ALL_MEMBERS,
}

enum EDCO_TriggerTimerMode
{
	IMMEDIATE,
	COUNTDOWN,
	TIMEOUT,
}

enum EDCO_TriggerRuntimeState
{
	ARMED,
	PENDING,
	ACTIVE,
}

enum EDCO_TriggerAction
{
	NOTIFY,	// on-screen warning notification to every player.
	SEND_QRF,	// dispatch hostile QRF-flagged AI groups to the trigger point.
	SPRING_AMBUSH,
	SPAWN_GROUP,	// spawn the selected engine AI group at the trigger center.
	FIRE_FX,	// start the nearest Bifrost FX emitter within the FX pair radius.
}

// One trigger-wide policy for every Ctrl-drag linked AI group. Per-group order
// queues remain available in the linked group's Bifrost Trigger properties.
enum EDCO_TriggerLinkedUnitMode
{
	LINK_ONLY,
	STAGE_AND_SPAWN,
	HOLD_FIRE_UNTIL_ACTIVATION,
}

// Spawnable group presets for the SPAWN_GROUP action.
enum EDCO_TriggerSpawnGroup
{
	US_FIRETEAM,
	US_RIFLE_SQUAD,
	US_MG_TEAM,
	USSR_FIREGROUP,
	USSR_RIFLE_SQUAD,
	USSR_MG_TEAM,
	FIA_FIRETEAM,
	FIA_RIFLE_SQUAD,
}

class DCO_TriggerStaticData
{
	ref array<string> m_ConditionNames = {
		"Anyone (AI or player)",
		"Players only",
	};

	ref array<string> m_ActionNames = {
		"Notify everyone",
		"Send QRF here",
		"Spring paired ambush",
		"Spawn group here",
		"Fire paired FX emitter",
	};

	ref array<string> m_LinkedUnitModeNames = {
		"Linked only",
		"Stage hidden, spawn on activation",
		"Ambush: hold fire until activation",
	};

	ref array<string> m_SpawnGroupNames = {
		"US Fire Team",
		"US Rifle Squad",
		"US MG Team",
		"Soviet Fire Group",
		"Soviet Rifle Squad",
		"Soviet MG Team",
		"FIA Fire Team",
		"FIA Rifle Squad",
	};

	ref array<ResourceName> m_GroupPrefabs = {
		"{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et",
		"{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et",
		"{958039B857396B7B}Prefabs/Groups/BLUFOR/Group_US_MachineGunTeam.et",
		"{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et",
		"{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et",
		"{A2F75E45C66B1C0A}Prefabs/Groups/OPFOR/Group_USSR_MachineGunTeam.et",
		"{5BEA04939D148B1D}Prefabs/Groups/INDFOR/Group_FIA_FireTeam.et",
		"{CE41AF625D05D0F0}Prefabs/Groups/INDFOR/Group_FIA_RifleSquad.et",
	};
}

class DCO_TriggerRegistry
{
	protected static ref array<DCO_TriggerComponent> s_aTriggers;

	static void Register(DCO_TriggerComponent t)
	{
		if (!s_aTriggers)
			s_aTriggers = {};
		if (s_aTriggers.Find(t) < 0)
			s_aTriggers.Insert(t);
	}

	static void Unregister(DCO_TriggerComponent t)
	{
		if (s_aTriggers)
		{
			int i = s_aTriggers.Find(t);
			if (i >= 0)
				s_aTriggers.Remove(i);
		}
	}

	static array<DCO_TriggerComponent> GetTriggers()
	{
		if (!s_aTriggers)
			s_aTriggers = {};
		return s_aTriggers;
	}
}

class DCO_TriggerPlacementServer
{
	protected static const ResourceName TRIGGER_PREFAB = "{EEFC7B09110761DE}Prefabs/E_DCO_Trigger.et";

	static bool Apply(vector position, out string result)
	{
		if (!Replication.IsServer())
		{
			result = "Trigger placement failed: server authority is unavailable.";
			return false;
		}
		Resource resource = Resource.Load(TRIGGER_PREFAB);
		if (!resource || !resource.IsValid())
		{
			result = "Trigger placement failed: the Bifrost trigger prefab is unavailable.";
			return false;
		}
		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		transform[3] = position;
		SCR_EditableEntityComponent editable = SCR_PlacingEditorComponent.SpawnEntityResource(TRIGGER_PREFAB, transform);
		if (!editable)
		{
			result = "Trigger placement failed: the server could not create an editable trigger.";
			return false;
		}
		result = "Placed Bifrost trigger. Open Properties to configure it.";
		return true;
	}
}

class DCO_TriggerFxRegistry
{
	protected static ref array<IEntity> s_aEmitters;

	static void Register(IEntity e)
	{
		if (!e)
			return;
		if (!s_aEmitters)
			s_aEmitters = {};
		if (s_aEmitters.Find(e) < 0)
			s_aEmitters.Insert(e);
	}

	static void Unregister(IEntity e)
	{
		if (s_aEmitters)
		{
			int i = s_aEmitters.Find(e);
			if (i >= 0)
				s_aEmitters.Remove(i);
		}
	}

	static array<IEntity> GetEmitters()
	{
		if (!s_aEmitters)
			s_aEmitters = {};
		for (int i = s_aEmitters.Count() - 1; i >= 0; i--)
		{
			if (!s_aEmitters[i])
				s_aEmitters.Remove(i);
		}
		return s_aEmitters;
	}
}

class DCO_TriggerComponentClass : ScriptComponentClass
{
}

class DCO_TriggerComponent : ScriptComponent
{
	protected static ref DCO_TriggerStaticData s_StaticData;

	protected static DCO_TriggerStaticData StaticData()
	{
		if (!s_StaticData)
			s_StaticData = new DCO_TriggerStaticData();
		return s_StaticData;
	}

	static array<string> DCO_GetActionNames() { return StaticData().m_ActionNames; }
	static array<string> DCO_GetLinkedUnitModeNames() { return StaticData().m_LinkedUnitModeNames; }

	[Attribute("0", UIWidgets.ComboBox, "Who trips this trigger: anyone, players only, or any character of a specific faction.", "", ParamEnumArray.FromEnum(EDCO_TriggerCondition), category: "Bifrost"), RplProp()]
	EDCO_TriggerCondition m_eCondition;

	[RplProp()]
	protected FactionKey m_sConditionFactionKey;

	[Attribute("0", UIWidgets.ComboBox, "Area shape.", "", ParamEnumArray.FromEnum(EDCO_TriggerShape), category: "Bifrost"), RplProp()]
	protected EDCO_TriggerShape m_eShape;

	[Attribute("0", UIWidgets.ComboBox, "Fire when subjects are present or absent.", "", ParamEnumArray.FromEnum(EDCO_TriggerActivation), category: "Bifrost"), RplProp()]
	protected EDCO_TriggerActivation m_eActivation;

	[Attribute("0", UIWidgets.ComboBox, "Use the area filter or a Ctrl-drag synced group's leader or members as the owner condition.", "", ParamEnumArray.FromEnum(EDCO_TriggerOwnerMode), category: "Bifrost"), RplProp()]
	protected EDCO_TriggerOwnerMode m_eOwnerMode;

	[Attribute("25", UIWidgets.Slider, "Trigger radius (m).", "5 500 5", category: "Bifrost"), RplProp()]
	float m_fRadius;

	[Attribute("25", UIWidgets.Slider, "Trigger half-depth (m).", "5 500 5", category: "Bifrost"), RplProp()]
	protected float m_fRadiusZ;

	[Attribute("0", UIWidgets.Slider, "Vertical trigger height (m). 0 means unlimited.", "0 500 5", category: "Bifrost"), RplProp()]
	protected float m_fHeight;

	[Attribute("1", UIWidgets.Slider, "How many matching characters must be inside at once before the trigger fires.", "1 30 1", category: "Bifrost"), RplProp()]
	int m_iCountThreshold;

	[Attribute("0", UIWidgets.CheckBox, "Repeat mode re-arms after the condition becomes false. Off activates once and remains active until Armed is toggled off.", category: "Bifrost"), RplProp()]
	bool m_bRepeat;

	[Attribute("10", UIWidgets.Slider, "Repeat re-arm delay (s) after the condition becomes false.", "1 300 1", category: "Bifrost"), RplProp()]
	float m_fCooldownSec;

	[Attribute("0", UIWidgets.ComboBox, "Immediate, Countdown, or Timeout activation timing.", "", ParamEnumArray.FromEnum(EDCO_TriggerTimerMode), category: "Bifrost"), RplProp()]
	protected EDCO_TriggerTimerMode m_eTimerMode;

	[Attribute("0", UIWidgets.Slider, "Minimum activation delay (s).", "0 600 1", category: "Bifrost"), RplProp()]
	protected float m_fTimerMin;

	[Attribute("0", UIWidgets.Slider, "Most likely activation delay (s).", "0 600 1", category: "Bifrost"), RplProp()]
	protected float m_fTimerMid;

	[Attribute("0", UIWidgets.Slider, "Maximum activation delay (s).", "0 600 1", category: "Bifrost"), RplProp()]
	protected float m_fTimerMax;

	[Attribute("0", UIWidgets.ComboBox, "What firing this trigger does.", "", ParamEnumArray.FromEnum(EDCO_TriggerAction), category: "Bifrost"), RplProp()]
	EDCO_TriggerAction m_eAction;

	[Attribute("0", UIWidgets.ComboBox, "Which AI group the SPAWN_GROUP action spawns at the trigger center.", "", ParamEnumArray.FromEnum(EDCO_TriggerSpawnGroup), category: "Bifrost"), RplProp()]
	EDCO_TriggerSpawnGroup m_eSpawnGroup;

	[RplProp()]
	protected FactionKey m_sSpawnFactionKey;

	[RplProp()]
	protected ResourceName m_rSpawnGroupPrefab;

	[Attribute("0", UIWidgets.Slider, "SPRING_AMBUSH pairing: the Pair ID of the DCO Ambush Position(s) to spring. 0 = spring the NEAREST ambush position.", "0 50 1", category: "Bifrost"), RplProp()]
	int m_iPairId;

	[Attribute("50", UIWidgets.Slider, "FIRE_FX pairing: the nearest Bifrost FX emitter within this range (m) of the trigger is the one toggled.", "5 200 5", category: "Bifrost"), RplProp()]
	float m_fFxPairRadius;

	[Attribute("0", UIWidgets.CheckBox, "Armed. Off = the trigger never fires. Turning it back ON also re-arms a tripped Once trigger.", category: "Bifrost"), RplProp()]
	bool m_bEnabled;

	[RplProp()]
	protected EDCO_TriggerLinkedUnitMode m_eLinkedUnitMode;

	[Attribute("2", UIWidgets.Slider, "How often (s) the trigger re-evaluates its condition. Keep at 2+ with many triggers placed.", "0.5 15 0.5", category: "Bifrost"), RplProp()]
	float m_fCheckSec;

	protected static const int COLOR_ARMED = 0xFFFF3030;
	protected static const int COLOR_DISABLED = 0xFF707070;	// grey - not armed.
	protected static const int COLOR_PENDING = 0xFFFFB340;
	protected static const int COLOR_ACTIVE = 0xFF44D47A;

	[RplProp()]
	protected bool m_bTripped = false;	// Once-mode latch.
	[RplProp()]
	protected EDCO_TriggerRuntimeState m_eRuntimeState = EDCO_TriggerRuntimeState.ARMED;
	protected float m_fLastFireMs = -1;
	protected float m_fRearmAtMs;
	protected float m_fActivationDeadlineMs;
	protected bool m_bCondActive = false;
	protected bool m_bActivated;
	protected bool m_bActivationPending;
	protected IEntity m_PendingTripper;
	protected int m_iPendingCount;
	protected IEntity m_FxTarget;	// emitter this trigger switched ON in Repeat mode.
	protected vector m_vFxSearchCenter;
	protected float m_fFxBestSq;
	protected IEntity m_FxBest;
	protected ref array<ref DCO_TriggerBinding> m_aSyncedGroups = {};
	protected int m_iNextSyncedGroupId = 1;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame() || !GetGame().InPlayMode())
			return;

		DCO_TriggerRegistry.Register(this);

		if (!Replication.IsServer())
			return;

		DCO_MigrateLegacySelections();
		GetGame().GetCallqueue().CallLater(DCO_Tick, (int)(m_fCheckSec * 1000.0), true);
	}

	void ~DCO_TriggerComponent()
	{
		// A GM deleting an unfired trigger must not strand hidden AI or leave a
		// trigger-owned fire hold behind. World shutdown does not recreate entities.
		if (Replication.IsServer() && GetGame() && GetGame().InPlayMode())
			DCO_CancelPendingSyncedGroups();
		DCO_TriggerRegistry.Unregister(this);
		DCO_TriggerSyncRegistry.UnregisterTrigger(this);
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(DCO_Tick);
			DCO_FxStop();
		}
	}

	vector DCO_GetCenter()
	{
		IEntity owner = GetOwner();
		if (owner)
			return owner.GetOrigin();
		return vector.Zero;
	}

	int DCO_GetCondition()
	{
		DCO_MigrateLegacySelections();
		if (m_sConditionFactionKey.IsEmpty())
			return Math.Clamp(m_eCondition, 0, StaticData().m_ConditionNames.Count() - 1);
		int index = DCO_FactionCatalog.IndexOf(m_sConditionFactionKey);
		if (index < 0)
			return DCO_GetConditionCount();
		return StaticData().m_ConditionNames.Count() + index;
	}
	void DCO_SetCondition(int c)
	{
		if (c < StaticData().m_ConditionNames.Count())
		{
			m_eCondition = Math.Clamp(c, 0, StaticData().m_ConditionNames.Count() - 1);
			m_sConditionFactionKey = "";
		}
		else
		{
			FactionKey key = DCO_FactionCatalog.KeyAt(c - StaticData().m_ConditionNames.Count());
			if (!key.IsEmpty())
			{
				m_sConditionFactionKey = key;
				m_eCondition = EDCO_TriggerCondition.FACTION_US;
			}
		}
		DCO_ReplicateState();
	}
	static int DCO_GetConditionCount()	{ return StaticData().m_ConditionNames.Count() + DCO_FactionCatalog.Count(); }
	static string DCO_GetConditionName(int index)
	{
		if (index >= 0 && index < StaticData().m_ConditionNames.Count())
			return StaticData().m_ConditionNames[index];
		return "Faction: " + DCO_FactionCatalog.NameAt(index - StaticData().m_ConditionNames.Count());
	}
	FactionKey DCO_GetConditionFactionKey() { DCO_MigrateLegacySelections(); return m_sConditionFactionKey; }
	string DCO_GetConditionDisplayName()
	{
		if (!m_sConditionFactionKey.IsEmpty())
			return "Faction: " + DCO_FactionCatalog.NameFor(m_sConditionFactionKey);
		return DCO_GetConditionName(DCO_GetCondition());
	}
	float DCO_GetRadius()				{ return m_fRadius; }
	void DCO_SetRadius(float r)			{ m_fRadius = Math.Clamp(r, 5, 500); DCO_ReplicateState(); }
	int DCO_GetShape()					{ return m_eShape; }
	void DCO_SetShape(int shape)		{ m_eShape = Math.ClampInt(shape, EDCO_TriggerShape.ELLIPSE, EDCO_TriggerShape.RECTANGLE); DCO_ReplicateState(); }
	int DCO_GetActivation()			{ return m_eActivation; }
	void DCO_SetActivation(int mode)	{ m_eActivation = Math.ClampInt(mode, EDCO_TriggerActivation.PRESENT, EDCO_TriggerActivation.NOT_PRESENT); DCO_ResetRuntimeState(); DCO_ReplicateState(); }
	int DCO_GetOwnerMode()				{ return m_eOwnerMode; }
	void DCO_SetOwnerMode(int mode)	{ m_eOwnerMode = Math.ClampInt(mode, EDCO_TriggerOwnerMode.AREA_FILTER, EDCO_TriggerOwnerMode.SYNCED_ALL_MEMBERS); DCO_ResetRuntimeState(); DCO_ReplicateState(); }
	float DCO_GetRadiusZ()
	{
		if (m_fRadiusZ < 5)
			return m_fRadius;
		else
			return m_fRadiusZ;
	}
	void DCO_SetRadiusZ(float radius)	{ m_fRadiusZ = Math.Clamp(radius, 5, 500); DCO_ReplicateState(); }
	float DCO_GetHeight()				{ return m_fHeight; }
	void DCO_SetHeight(float height)	{ m_fHeight = Math.Clamp(height, 0, 500); DCO_ReplicateState(); }
	int DCO_GetCountThreshold()			{ return m_iCountThreshold; }
	void DCO_SetCountThreshold(int c)	{ m_iCountThreshold = Math.Clamp(c, 1, 30); DCO_ReplicateState(); }
	bool DCO_GetRepeat()				{ return m_bRepeat; }
	float DCO_GetCooldown()				{ return m_fCooldownSec; }
	void DCO_SetCooldown(float s)		{ m_fCooldownSec = Math.Clamp(s, 1, 300); DCO_ReplicateState(); }
	int DCO_GetTimerMode()				{ return m_eTimerMode; }
	void DCO_SetTimerMode(int mode)		{ m_eTimerMode = Math.ClampInt(mode, EDCO_TriggerTimerMode.IMMEDIATE, EDCO_TriggerTimerMode.TIMEOUT); DCO_ResetRuntimeState(); DCO_ReplicateState(); }
	float DCO_GetTimerMin()				{ return m_fTimerMin; }
	float DCO_GetTimerMid()				{ return m_fTimerMid; }
	float DCO_GetTimerMax()				{ return m_fTimerMax; }
	void DCO_SetTimerMin(float value)	{ m_fTimerMin = Math.Clamp(value, 0, 600); DCO_NormalizeTimer(); DCO_ReplicateState(); }
	void DCO_SetTimerMid(float value)	{ m_fTimerMid = Math.Clamp(value, 0, 600); DCO_NormalizeTimer(); DCO_ReplicateState(); }
	void DCO_SetTimerMax(float value)	{ m_fTimerMax = Math.Clamp(value, 0, 600); DCO_NormalizeTimer(); DCO_ReplicateState(); }
	float DCO_GetCheckInterval()		{ return m_fCheckSec; }
	void DCO_SetCheckInterval(float s)
	{
		m_fCheckSec = Math.Clamp(s, 0.5, 15);
		if (Replication.IsServer() && GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(DCO_Tick);
			GetGame().GetCallqueue().CallLater(DCO_Tick, (int)(m_fCheckSec * 1000.0), true);
		}
		DCO_ReplicateState();
	}
	int DCO_GetAction()					{ return m_eAction; }
	void DCO_SetAction(int a)			{ m_eAction = Math.Clamp(a, 0, StaticData().m_ActionNames.Count() - 1); DCO_ReplicateState(); }
	int DCO_GetSpawnFaction()
	{
		DCO_MigrateLegacySelections();
		if (m_sSpawnFactionKey.IsEmpty())
			return 0;
		int index = DCO_TriggerGroupCatalog.FactionIndexOf(m_sSpawnFactionKey);
		if (index < 0)
			return DCO_TriggerGroupCatalog.FactionCount() + 1;
		return index + 1;
	}
	void DCO_SetSpawnFaction(int index)
	{
		FactionKey selectedKey;
		if (index > 0)
			selectedKey = DCO_TriggerGroupCatalog.FactionKeyAt(index - 1);
		if (index > 0 && selectedKey.IsEmpty())
			return;
		m_sSpawnFactionKey = selectedKey;
		if (!selectedKey.IsEmpty() && DCO_TriggerGroupCatalog.FactionForPrefab(m_rSpawnGroupPrefab) != selectedKey)
		{
			DCO_TriggerGroupEntry first = DCO_TriggerGroupCatalog.GroupAt(selectedKey, 0);
			if (first)
				m_rSpawnGroupPrefab = first.m_Prefab;
		}
		DCO_ReplicateState();
	}
	FactionKey DCO_GetSpawnFactionKey() { DCO_MigrateLegacySelections(); return m_sSpawnFactionKey; }
	int DCO_GetSpawnGroup()
	{
		DCO_MigrateLegacySelections();
		int index = DCO_TriggerGroupCatalog.GroupIndexOf(m_sSpawnFactionKey, m_rSpawnGroupPrefab);
		if (index < 0)
			return DCO_TriggerGroupCatalog.GroupCount(m_sSpawnFactionKey);
		return index;
	}
	void DCO_SetSpawnGroup(int index)
	{
		DCO_TriggerGroupEntry selected = DCO_TriggerGroupCatalog.GroupAt(m_sSpawnFactionKey, index);
		if (!selected)
			return;
		m_rSpawnGroupPrefab = selected.m_Prefab;
		m_eSpawnGroup = 0;
		DCO_ReplicateState();
	}
	ResourceName DCO_GetSpawnGroupPrefab() { DCO_MigrateLegacySelections(); return m_rSpawnGroupPrefab; }
	int DCO_GetPairId()					{ return m_iPairId; }
	void DCO_SetPairId(int id)			{ m_iPairId = Math.Clamp(id, 0, 50); DCO_ReplicateState(); }
	float DCO_GetFxPairRadius()			{ return m_fFxPairRadius; }
	void DCO_SetFxPairRadius(float r)	{ m_fFxPairRadius = Math.Clamp(r, 5, 200); DCO_ReplicateState(); }
	bool DCO_GetEnabled()				{ return m_bEnabled; }
	bool DCO_IsTripped()				{ return m_bTripped; }
	int DCO_GetRuntimeState()			{ return m_eRuntimeState; }
	int DCO_GetVisualColor()
	{
		if (!m_bEnabled)
			return COLOR_DISABLED;
		if (m_eRuntimeState == EDCO_TriggerRuntimeState.PENDING)
			return COLOR_PENDING;
		if (m_eRuntimeState == EDCO_TriggerRuntimeState.ACTIVE)
			return COLOR_ACTIVE;
		return COLOR_ARMED;
	}
	int DCO_GetLinkedUnitMode()		{ return m_eLinkedUnitMode; }
	int DCO_GetSyncedGroupCount()
	{
		int count;
		foreach (DCO_TriggerBinding binding : m_aSyncedGroups)
		{
			if (binding && !binding.m_bConsumed)
				count++;
		}
		return count;
	}

	void DCO_SetLinkedUnitMode(int mode)
	{
		if (!Replication.IsServer())
			return;
		m_eLinkedUnitMode = Math.ClampInt(mode, EDCO_TriggerLinkedUnitMode.LINK_ONLY, EDCO_TriggerLinkedUnitMode.HOLD_FIRE_UNTIL_ACTIVATION);
		foreach (DCO_TriggerBinding binding : m_aSyncedGroups)
			DCO_ApplyLinkedUnitMode(binding);
		DCO_ReplicateState();
	}

	protected void DCO_NormalizeTimer()
	{
		m_fTimerMin = Math.Clamp(m_fTimerMin, 0, 600);
		m_fTimerMax = Math.Clamp(m_fTimerMax, m_fTimerMin, 600);
		m_fTimerMid = Math.Clamp(m_fTimerMid, m_fTimerMin, m_fTimerMax);
	}

	protected void DCO_ResetRuntimeState()
	{
		if (m_bActivated)
			DCO_Deactivate();
		m_bActivationPending = false;
		m_bCondActive = false;
		m_PendingTripper = null;
		m_iPendingCount = 0;
		m_eRuntimeState = EDCO_TriggerRuntimeState.ARMED;
	}

	protected void DCO_MigrateLegacySelections()
	{
		if (!Replication.IsServer())
			return;

		bool changed;
		if (m_fRadiusZ < 5)
		{
			m_fRadiusZ = Math.Clamp(m_fRadius, 5, 500);
			changed = true;
		}
		float oldTimerMin = m_fTimerMin;
		float oldTimerMid = m_fTimerMid;
		float oldTimerMax = m_fTimerMax;
		DCO_NormalizeTimer();
		if (m_fTimerMin != oldTimerMin || m_fTimerMid != oldTimerMid || m_fTimerMax != oldTimerMax)
			changed = true;
		if (m_sConditionFactionKey.IsEmpty() && m_eCondition >= EDCO_TriggerCondition.FACTION_US)
		{
			int legacyFaction = m_eCondition - EDCO_TriggerCondition.FACTION_US;
			array<FactionKey> legacyKeys = {"US", "USSR", "FIA", "CIV"};
			if (legacyKeys.IsIndexValid(legacyFaction))
			{
				m_sConditionFactionKey = legacyKeys[legacyFaction];
				changed = true;
			}
		}
		if (m_rSpawnGroupPrefab.IsEmpty() && !StaticData().m_GroupPrefabs.IsEmpty())
		{
			int legacyGroup = Math.Clamp(m_eSpawnGroup, 0, StaticData().m_GroupPrefabs.Count() - 1);
			m_rSpawnGroupPrefab = StaticData().m_GroupPrefabs[legacyGroup];
			changed = true;
		}
		if (m_sSpawnFactionKey.IsEmpty() && !m_rSpawnGroupPrefab.IsEmpty())
		{
			m_sSpawnFactionKey = DCO_TriggerGroupCatalog.FactionForPrefab(m_rSpawnGroupPrefab);
			if (!m_sSpawnFactionKey.IsEmpty())
				changed = true;
		}
		if (changed)
			DCO_ReplicateState();
	}

	protected void DCO_ReplicateState()
	{
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	void DCO_SetEnabled(bool on)
	{
		bool was = m_bEnabled;
		m_bEnabled = on;
		DCO_ReplicateState();
		if (!on)
		{
			DCO_ResetRuntimeState();
			return;
		}
		if (was)
			return;
		m_bTripped = false;
		m_fLastFireMs = -1;
		m_fRearmAtMs = 0;
		m_eRuntimeState = EDCO_TriggerRuntimeState.ARMED;
		DCO_ReplicateState();
	}

	void DCO_SetRepeat(bool on)
	{
		m_bRepeat = on;
		DCO_ResetRuntimeState();
		DCO_ReplicateState();
	}

	protected float DCO_FlatDistSq(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}

	protected void DCO_Tick()
	{
		if (!Replication.IsServer())
			return;
		if (!m_bEnabled)
			return;
		if (!m_bRepeat && m_bTripped)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		IEntity tripper;
		int count;
		bool active = DCO_EvaluateCondition(tripper, count);
		float now = DCO_WorldTime();
		bool wasActive = m_bCondActive;
		m_bCondActive = active;

		if (active && !wasActive && !m_bActivationPending && !m_bActivated)
			DCO_BeginActivation(tripper, count, now);
		else if (!active && wasActive)
		{
			if (m_bActivationPending && m_eTimerMode != EDCO_TriggerTimerMode.COUNTDOWN)
				DCO_CancelPendingActivation();
			if (m_bActivated && m_bRepeat)
				DCO_Deactivate();
		}

		if (m_bActivationPending && now >= m_fActivationDeadlineMs)
		{
			if (m_eTimerMode != EDCO_TriggerTimerMode.COUNTDOWN && !m_bCondActive)
				DCO_CancelPendingActivation();
			else
				DCO_Activate(m_PendingTripper, m_iPendingCount, now);
		}
	}

	protected float DCO_WorldTime()
	{
		BaseWorld world = GetGame().GetWorld();
		if (world)
			return world.GetWorldTime();
		return 0;
	}

	protected float DCO_SampleTimerDelay()
	{
		DCO_NormalizeTimer();
		if (m_eTimerMode == EDCO_TriggerTimerMode.IMMEDIATE || m_fTimerMax <= 0)
			return 0;
		if (m_fTimerMax <= m_fTimerMin)
			return m_fTimerMin;
		float span = m_fTimerMax - m_fTimerMin;
		float pivot = (m_fTimerMid - m_fTimerMin) / span;
		float sample = Math.RandomFloat01();
		if (sample < pivot)
			return m_fTimerMin + Math.Sqrt(sample * span * (m_fTimerMid - m_fTimerMin));
		return m_fTimerMax - Math.Sqrt((1.0 - sample) * span * (m_fTimerMax - m_fTimerMid));
	}

	protected void DCO_BeginActivation(IEntity tripper, int count, float now)
	{
		float readyAt = now + DCO_SampleTimerDelay() * 1000.0;
		readyAt = Math.Max(readyAt, m_fRearmAtMs);
		if (readyAt <= now)
		{
			DCO_Activate(tripper, count, now);
			return;
		}
		m_bActivationPending = true;
		m_fActivationDeadlineMs = readyAt;
		m_PendingTripper = tripper;
		m_iPendingCount = count;
		m_eRuntimeState = EDCO_TriggerRuntimeState.PENDING;
		DCO_ReplicateState();
	}

	protected void DCO_CancelPendingActivation()
	{
		m_bActivationPending = false;
		m_PendingTripper = null;
		m_iPendingCount = 0;
		m_eRuntimeState = EDCO_TriggerRuntimeState.ARMED;
		DCO_ReplicateState();
	}

	protected void DCO_Activate(IEntity tripper, int count, float now)
	{
		m_bActivationPending = false;
		m_PendingTripper = null;
		m_iPendingCount = 0;
		m_bActivated = true;
		m_fLastFireMs = now;
		if (!m_bRepeat)
			m_bTripped = true;
		m_eRuntimeState = EDCO_TriggerRuntimeState.ACTIVE;
		DCO_ReplicateState();
		DCO_Fire(tripper, count);
		if (!m_bCondActive && m_bRepeat)
			DCO_Deactivate();
	}

	protected void DCO_Deactivate()
	{
		if (!m_bActivated)
			return;
		m_bActivated = false;
		DCO_FxStop();
		if (m_bRepeat)
		{
			m_fRearmAtMs = DCO_WorldTime() + m_fCooldownSec * 1000.0;
			DCO_RearmSyncedTriggerHolds();
		}
		m_eRuntimeState = EDCO_TriggerRuntimeState.ARMED;
		DCO_ReplicateState();
	}

	protected bool DCO_EvaluateCondition(out IEntity firstMatch, out int count)
	{
		firstMatch = null;
		count = 0;
		int total = 0;
		bool valid = true;
		if (m_eOwnerMode == EDCO_TriggerOwnerMode.AREA_FILTER)
			count = DCO_CountAreaMatches(firstMatch);
		else
			valid = DCO_CountSyncedMatches(firstMatch, count, total);
		if (!valid)
			return false;

		bool present;
		if (m_eOwnerMode == EDCO_TriggerOwnerMode.AREA_FILTER)
			present = count >= Math.Clamp(m_iCountThreshold, 1, 30);
		else if (m_eOwnerMode == EDCO_TriggerOwnerMode.SYNCED_ALL_MEMBERS)
			present = total > 0 && count == total;
		else
			present = count > 0;
		if (m_eActivation == EDCO_TriggerActivation.NOT_PRESENT)
			return !present;
		return present;
	}

	protected bool DCO_IsLivingCharacter(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return false;
		CharacterControllerComponent controller = character.GetCharacterController();
		return controller && !controller.IsDead();
	}

	protected bool DCO_IsInside(IEntity entity)
	{
		if (!entity || !GetOwner())
			return false;
		DCO_MigrateLegacySelections();
		vector transform[4];
		GetOwner().GetWorldTransform(transform);
		vector axisX = transform[0];
		vector axisZ = transform[2];
		axisX[1] = 0;
		axisZ[1] = 0;
		axisX.Normalize();
		axisZ.Normalize();
		vector delta = entity.GetOrigin() - transform[3];
		if (m_fHeight > 0 && Math.AbsFloat(delta[1]) > m_fHeight * 0.5)
			return false;
		float localX = Math.AbsFloat(vector.Dot(delta, axisX));
		float localZ = Math.AbsFloat(vector.Dot(delta, axisZ));
		float radiusX = Math.Max(m_fRadius, 1);
		float radiusZ = Math.Max(m_fRadiusZ, 1);
		if (m_eShape == EDCO_TriggerShape.RECTANGLE)
			return localX <= radiusX && localZ <= radiusZ;
		float normalizedX = localX / radiusX;
		float normalizedZ = localZ / radiusZ;
		return normalizedX * normalizedX + normalizedZ * normalizedZ <= 1.0;
	}

	protected int DCO_CountAreaMatches(out IEntity firstMatch)
	{
		firstMatch = null;
		SCR_Faction condFaction = DCO_GetConditionFaction();
		int count = 0;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			array<int> ids = {};
			pm.GetPlayers(ids);
			foreach (int id : ids)
			{
				IEntity ent = pm.GetPlayerControlledEntity(id);
				if (!DCO_IsLivingCharacter(ent))
					continue;
				if (!DCO_IsInside(ent))
					continue;
				if (!DCO_ConditionMatches(ent, true, condFaction))
					continue;
				count++;
				if (!firstMatch)
					firstMatch = ent;
			}
		}

		DCO_MigrateLegacySelections();
		if (m_sConditionFactionKey.IsEmpty() && m_eCondition == EDCO_TriggerCondition.PLAYERS_ONLY)
			return count;

		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (aiWorld)
		{
			array<AIAgent> agents = {};
			aiWorld.GetAIAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;
				IEntity ent = agent.GetControlledEntity();
				if (!DCO_IsLivingCharacter(ent))
					continue;
				if (DCO_PlayerUtil.IsPlayer(ent))
					continue;	// already counted in the player loop.
				if (!DCO_IsInside(ent))
					continue;
				if (!DCO_ConditionMatches(ent, false, condFaction))
					continue;
				count++;
				if (!firstMatch)
					firstMatch = ent;
			}
		}

		return count;
	}

	protected bool DCO_CountSyncedMatches(out IEntity firstMatch, out int count, out int total)
	{
		firstMatch = null;
		count = 0;
		total = 0;
		foreach (DCO_TriggerBinding binding : m_aSyncedGroups)
		{
			if (!binding || binding.m_bStaged || binding.m_bConsumed)
				continue;
			RplComponent groupRpl = RplComponent.Cast(Replication.FindItem(binding.m_GroupEntityId));
			IEntity groupEntity;
			if (groupRpl)
				groupEntity = groupRpl.GetEntity();
			SCR_EditableGroupComponent editable;
			if (groupEntity)
				editable = SCR_EditableGroupComponent.Cast(groupEntity.FindComponent(SCR_EditableGroupComponent));
			SCR_AIGroup group;
			if (editable)
				group = editable.GetAIGroupComponent();
			if (!group)
				continue;

			if (m_eOwnerMode == EDCO_TriggerOwnerMode.SYNCED_LEADER)
			{
				IEntity leader = group.GetLeaderEntity();
				if (!DCO_IsLivingCharacter(leader))
					continue;
				total++;
				if (DCO_IsInside(leader))
				{
					count++;
					if (!firstMatch)
						firstMatch = leader;
				}
				continue;
			}

			array<AIAgent> agents = {};
			group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;
				IEntity member = agent.GetControlledEntity();
				if (!DCO_IsLivingCharacter(member))
					continue;
				total++;
				if (!DCO_IsInside(member))
					continue;
				count++;
				if (!firstMatch)
					firstMatch = member;
			}
		}
		return total > 0;
	}

	protected SCR_Faction DCO_GetConditionFaction()
	{
		DCO_MigrateLegacySelections();
		string key = m_sConditionFactionKey;
		if (key.IsEmpty())
			return null;
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return null;
		return SCR_Faction.Cast(fm.GetFactionByKey(key));
	}

	protected bool DCO_ConditionMatches(IEntity ent, bool isPlayer, SCR_Faction condFaction)
	{
		DCO_MigrateLegacySelections();
		if (m_sConditionFactionKey.IsEmpty())
		{
			if (m_eCondition == EDCO_TriggerCondition.ANY_CHARACTER)
				return true;
			if (m_eCondition == EDCO_TriggerCondition.PLAYERS_ONLY)
				return isPlayer;
			return false;
		}

		if (!condFaction)
			return false;
		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (!fac)
			return false;
		return fac.GetAffiliatedFaction() == condFaction;
	}

	protected Faction DCO_FactionOf(IEntity ent)
	{
		if (!ent)
			return null;
		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (!fac)
			return null;
		return fac.GetAffiliatedFaction();
	}

	bool DCO_AddSyncedGroup(SCR_EditableGroupComponent editable, RplId groupId, out string result)
	{
		if (!Replication.IsServer() || !editable || !editable.GetOwner())
		{
			result = "Sync failed: server could not resolve the AI group.";
			return false;
		}
		DCO_TriggerComponent existingTrigger;
		DCO_TriggerBinding existingBinding;
		if (DCO_TriggerSyncRegistry.Find(groupId, existingTrigger, existingBinding))
		{
			result = "This group is already synced to a trigger.";
			return false;
		}

		SCR_AIGroup aiGroup = editable.GetAIGroupComponent();
		if (!aiGroup || aiGroup.GetPlayerCount(true) > 0)
		{
			result = "Sync refused: player-controlled groups cannot be staged or replaced.";
			return false;
		}
		ResourceName prefab = editable.GetPrefab();
		Resource resource = Resource.Load(prefab);
		if (prefab.IsEmpty() || !resource || !resource.IsValid())
		{
			result = "Sync failed: the group's source prefab is unavailable.";
			return false;
		}

		DCO_TriggerBinding binding = new DCO_TriggerBinding();
		binding.m_iId = m_iNextSyncedGroupId++;
		binding.m_GroupEntityId = groupId;
		binding.m_rGroupPrefab = prefab;
		binding.m_sGroupLabel = editable.GetDisplayName();
		if (binding.m_sGroupLabel.IsEmpty())
			binding.m_sGroupLabel = "AI Group";
		vector transform[4];
		editable.GetOwner().GetWorldTransform(transform);
		binding.m_aTransform = transform;
		foreach (DCO_TriggerSequenceStep step : binding.m_aSteps)
			step.m_vTarget = DCO_GetCenter();
		m_aSyncedGroups.Insert(binding);
		DCO_TriggerSyncRegistry.Register(groupId, this, binding);
		DCO_ApplyLinkedUnitMode(binding);
		result = "Synced " + binding.m_sGroupLabel + ". Configure all linked groups in Trigger Properties > Units, or edit this group's Bifrost Trigger queue.";
		return true;
	}

	DCO_TriggerBinding DCO_GetSyncedBinding(int bindingId)
	{
		foreach (DCO_TriggerBinding binding : m_aSyncedGroups)
		{
			if (binding && binding.m_iId == bindingId)
				return binding;
		}
		return null;
	}

	void DCO_SetSyncedSpawn(int bindingId, bool enabled)
	{
		if (!Replication.IsServer())
			return;
		DCO_TriggerBinding binding = DCO_GetSyncedBinding(bindingId);
		if (!binding || binding.m_bConsumed)
			return;
		if (enabled)
			DCO_ReleaseSyncedTriggerHold(binding, true);
		binding.m_bSpawnOnTrigger = enabled;
		if (enabled && !binding.m_bStaged)
			GetGame().GetCallqueue().Call(DCO_StageSyncedGroup, bindingId);
		else if (!enabled && binding.m_bStaged)
			DCO_RestoreStagedGroup(binding);
	}

	protected SCR_EditableGroupComponent DCO_GetSyncedEditable(DCO_TriggerBinding binding)
	{
		if (!binding || binding.m_bStaged || binding.m_bConsumed)
			return null;
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(binding.m_GroupEntityId));
		IEntity groupEntity;
		if (rpl)
			groupEntity = rpl.GetEntity();
		if (!groupEntity)
			return null;
		return SCR_EditableGroupComponent.Cast(groupEntity.FindComponent(SCR_EditableGroupComponent));
	}

	protected SCR_AIGroupUtilityComponent DCO_GetSyncedUtility(DCO_TriggerBinding binding)
	{
		SCR_EditableGroupComponent editable = DCO_GetSyncedEditable(binding);
		SCR_AIGroup group;
		if (editable)
			group = editable.GetAIGroupComponent();
		if (!group)
			return null;
		return group.GetGroupUtilityComponent();
	}

	protected void DCO_ApplySyncedTriggerHold(DCO_TriggerBinding binding)
	{
		if (!binding || binding.m_bConsumed)
			return;
		SCR_AIGroupUtilityComponent utility = DCO_GetSyncedUtility(binding);
		if (!utility)
			return;
		if (!binding.m_bManualHoldCaptured)
		{
			binding.m_bManualHoldBeforeTrigger = utility.DCO_GetManualHold();
			binding.m_bManualHoldCaptured = true;
		}
		utility.DCO_SetManualHold(true);
		binding.m_bTriggerHoldActive = true;
	}

	protected void DCO_ReleaseSyncedTriggerHold(DCO_TriggerBinding binding, bool restorePrevious)
	{
		if (!binding)
			return;
		SCR_AIGroupUtilityComponent utility = DCO_GetSyncedUtility(binding);
		if (utility && binding.m_bTriggerHoldActive)
		{
			if (restorePrevious && binding.m_bManualHoldCaptured)
				utility.DCO_SetManualHold(binding.m_bManualHoldBeforeTrigger);
			else
				utility.DCO_SetManualHold(false);
		}
		binding.m_bTriggerHoldActive = false;
		if (restorePrevious)
			binding.m_bManualHoldCaptured = false;
	}

	protected bool DCO_RestoreStagedGroup(DCO_TriggerBinding binding)
	{
		if (!Replication.IsServer() || !binding || !binding.m_bStaged || binding.m_bConsumed)
			return false;
		SCR_EditableEntityComponent spawned = SCR_PlacingEditorComponent.SpawnEntityResource(binding.m_rGroupPrefab, binding.m_aTransform);
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(spawned);
		if (!editableGroup || !editableGroup.GetAIGroupComponent())
		{
			Print("[DCO-TRIGGER] failed to restore staged group while cancelling trigger: " + binding.m_rGroupPrefab, LogLevel.WARNING);
			return false;
		}

		binding.m_bStaged = false;
		RplComponent rpl = RplComponent.Cast(editableGroup.GetOwner().FindComponent(RplComponent));
		if (rpl && rpl.Id().IsValid())
		{
			binding.m_GroupEntityId = rpl.Id();
			DCO_TriggerSyncRegistry.Register(binding.m_GroupEntityId, this, binding);
		}
		return true;
	}

	protected void DCO_ApplyLinkedUnitMode(DCO_TriggerBinding binding)
	{
		if (!binding || binding.m_bConsumed)
			return;
		switch (m_eLinkedUnitMode)
		{
			case EDCO_TriggerLinkedUnitMode.STAGE_AND_SPAWN:
			{
				DCO_ReleaseSyncedTriggerHold(binding, true);
				binding.m_bSpawnOnTrigger = true;
				if (!binding.m_bStaged)
					GetGame().GetCallqueue().Call(DCO_StageSyncedGroup, binding.m_iId);
				break;
			}
			case EDCO_TriggerLinkedUnitMode.HOLD_FIRE_UNTIL_ACTIVATION:
			{
				binding.m_bSpawnOnTrigger = false;
				if (binding.m_bStaged && !DCO_RestoreStagedGroup(binding))
					break;
				DCO_ApplySyncedTriggerHold(binding);
				break;
			}
			default:
			{
				binding.m_bSpawnOnTrigger = false;
				if (binding.m_bStaged)
					DCO_RestoreStagedGroup(binding);
				DCO_ReleaseSyncedTriggerHold(binding, true);
				break;
			}
		}
	}

	protected void DCO_RearmSyncedTriggerHolds()
	{
		if (m_eLinkedUnitMode != EDCO_TriggerLinkedUnitMode.HOLD_FIRE_UNTIL_ACTIVATION)
			return;
		foreach (DCO_TriggerBinding binding : m_aSyncedGroups)
			DCO_ApplySyncedTriggerHold(binding);
	}

	protected void DCO_CancelPendingSyncedGroups()
	{
		foreach (DCO_TriggerBinding binding : m_aSyncedGroups)
		{
			if (!binding || binding.m_bConsumed)
				continue;
			if (binding.m_bStaged)
				DCO_RestoreStagedGroup(binding);
			DCO_ReleaseSyncedTriggerHold(binding, true);
			binding.m_bSpawnOnTrigger = false;
		}
	}

	void DCO_SetSyncedAction(int bindingId, int stepIndex, int action)
	{
		if (!Replication.IsServer())
			return;
		DCO_TriggerBinding binding = DCO_GetSyncedBinding(bindingId);
		DCO_TriggerSequenceStep step;
		if (binding)
			step = binding.GetStep(stepIndex);
		if (step)
			step.m_eAction = Math.ClampInt(action, EDCO_TriggerSequenceAction.NONE, EDCO_TriggerSequenceAction.DEFEND);
	}

	void DCO_SetSyncedTarget(int bindingId, int stepIndex, int choice)
	{
		if (!Replication.IsServer())
			return;
		DCO_TriggerBinding binding = DCO_GetSyncedBinding(bindingId);
		DCO_TriggerSequenceStep step;
		if (binding)
			step = binding.GetStep(stepIndex);
		if (!step)
			return;
		vector target;
		if (!DCO_TriggerObjectiveCatalog.Resolve(choice, this, binding, stepIndex, target))
			return;
		step.m_vTarget = target;
		// Store dynamic objectives as coordinates so the queue survives source removal.
		if (choice >= 3)
			step.m_iTargetChoice = 2;
		else
			step.m_iTargetChoice = choice;
	}

	protected void DCO_StageSyncedGroup(int bindingId)
	{
		if (!Replication.IsServer())
			return;
		DCO_TriggerBinding binding = DCO_GetSyncedBinding(bindingId);
		if (!binding || binding.m_bStaged || !binding.m_bSpawnOnTrigger)
			return;
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(binding.m_GroupEntityId));
		IEntity groupEntity;
		if (rpl)
			groupEntity = rpl.GetEntity();
		SCR_EditableGroupComponent editable;
		if (groupEntity)
			editable = SCR_EditableGroupComponent.Cast(groupEntity.FindComponent(SCR_EditableGroupComponent));
		SCR_AIGroup group;
		if (editable)
			group = editable.GetAIGroupComponent();
		if (!editable || !group || group.GetPlayerCount(true) > 0)
		{
			binding.m_bSpawnOnTrigger = false;
			Print("[DCO-TRIGGER] staging refused: synced group disappeared or gained a player", LogLevel.WARNING);
			return;
		}
		binding.m_bStaged = true;
		DCO_TriggerSyncRegistry.UnregisterGroup(binding.m_GroupEntityId);
		editable.Delete(true, true);
	}

	protected void DCO_FireSyncedGroups()
	{
		if (!Replication.IsServer())
			return;
		foreach (DCO_TriggerBinding binding : m_aSyncedGroups)
		{
			if (binding && binding.m_bTriggerHoldActive)
				DCO_ReleaseSyncedTriggerHold(binding, false);
			if (!binding || !binding.m_bSpawnOnTrigger || !binding.m_bStaged || binding.m_bConsumed)
				continue;
			SCR_EditableEntityComponent spawned = SCR_PlacingEditorComponent.SpawnEntityResource(binding.m_rGroupPrefab, binding.m_aTransform);
			SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(spawned);
			SCR_AIGroup group;
			if (editableGroup)
				group = editableGroup.GetAIGroupComponent();
			if (!group)
			{
				Print("[DCO-TRIGGER] synced group failed to spawn: " + binding.m_rGroupPrefab, LogLevel.WARNING);
				continue;
			}
			binding.m_bConsumed = true;
			DCO_TriggerSequenceService.Start(group, binding.m_aSteps);
		}
	}

	protected void DCO_Fire(IEntity tripper, int count)
	{
		DCO_FireSyncedGroups();
		switch (m_eAction)
		{
			case EDCO_TriggerAction.NOTIFY:
			{
				DCO_ActionNotify();
				break;
			}
			case EDCO_TriggerAction.SEND_QRF:
			{
				DCO_ActionSendQrf(tripper);
				break;
			}
			case EDCO_TriggerAction.SPRING_AMBUSH:
			{
				DCO_ActionSpringAmbush();
				break;
			}
			case EDCO_TriggerAction.SPAWN_GROUP:
			{
				DCO_ActionSpawnGroup();
				break;
			}
			case EDCO_TriggerAction.FIRE_FX:
			{
				DCO_ActionFireFx();
				break;
			}
			default:
			{
				break;
			}
		}

		Print(string.Format("[DCO-TRIGGER] fired: action=%1 condition=%2 matches=%3 repeat=%4",
			StaticData().m_ActionNames[Math.Clamp(m_eAction, 0, StaticData().m_ActionNames.Count() - 1)],
			DCO_GetConditionDisplayName(),
			count, m_bRepeat), LogLevel.NORMAL);
	}

	// Warning toast on every player's screen.
	protected void DCO_ActionNotify()
	{
		SCR_NotificationsComponent.SendToEveryone(ENotification.EDITOR_ENEMY_IN_AREA);
	}

	protected void DCO_ActionSendQrf(IEntity tripper)
	{
		vector center = DCO_GetCenter();
		SCR_Faction tripperFaction = SCR_Faction.Cast(DCO_FactionOf(tripper));

		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);

		array<SCR_AIGroup> seen = {};
		int dispatched = 0;
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
			if (!grp || seen.Find(grp) >= 0)
				continue;
			seen.Insert(grp);

			IEntity leader = grp.GetLeaderEntity();
			if (!leader)
				continue;
			if (DCO_PlayerUtil.IsPlayer(leader))
				continue;	// never issue orders to a player-led group.

			SCR_AIGroupUtilityComponent util = grp.GetGroupUtilityComponent();
			if (!util || !util.DCO_IsQRFResponder())
				continue;

			if (tripperFaction)
			{
				SCR_Faction grpFaction = SCR_Faction.Cast(grp.GetFaction());
				if (!grpFaction || grpFaction == tripperFaction)
					continue;
				if (!grpFaction.IsFactionEnemy(tripperFaction))
					continue;
			}

			// The responder's own QRF Range attribute is its response radius to this alarm.
			float range = util.DCO_GetQRFRange();
			if (DCO_FlatDistSq(leader.GetOrigin(), center) > range * range)
				continue;

			AICommunicationComponent comms = AICommunicationComponent.Cast(grp.FindComponent(AICommunicationComponent));
			if (!comms)
				continue;

			util.DCO_SetQRFHoldPosition(center);
			DCO_VehicleUtil.OrderGroupMoveToPosition(grp, center, comms);
			dispatched++;
		}

		if (dispatched == 0)
			Print("[DCO-TRIGGER] SEND_QRF fired but no eligible QRF responder was in range", LogLevel.WARNING);
	}

	protected void DCO_ActionSpringAmbush()
	{
		array<DCO_TaskZoneComponent> zones = DCO_TaskZoneRegistry.GetZones();
		vector center = DCO_GetCenter();

		if (m_iPairId != 0)
		{
			int sprung = 0;
			foreach (DCO_TaskZoneComponent z : zones)
			{
				if (!z)
					continue;
				if (z.DCO_GetRole() == EDCO_ZoneRole.AMBUSH && z.DCO_GetPairId() == m_iPairId)
				{
					z.DCO_SpringManagedAmbushes();
					sprung++;
				}
			}
			if (sprung == 0)
				Print(string.Format("[DCO-TRIGGER] SPRING_AMBUSH: no ambush position with Pair ID %1", m_iPairId), LogLevel.WARNING);
			return;
		}

		DCO_TaskZoneComponent nearest;
		float bestSq = -1;
		foreach (DCO_TaskZoneComponent z : zones)
		{
			if (!z || z.DCO_GetRole() != EDCO_ZoneRole.AMBUSH)
				continue;
			float dSq = DCO_FlatDistSq(z.DCO_GetCenter(), center);
			if (bestSq < 0 || dSq < bestSq)
			{
				bestSq = dSq;
				nearest = z;
			}
		}
		if (nearest)
			nearest.DCO_SpringManagedAmbushes();
		else
			Print("[DCO-TRIGGER] SPRING_AMBUSH: no ambush position placed", LogLevel.WARNING);
	}

	// Server-side replicated spawn of the selected engine group prefab at the trigger center.
	protected void DCO_ActionSpawnGroup()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		DCO_MigrateLegacySelections();
		ResourceName prefab = m_rSpawnGroupPrefab;
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print("[DCO-TRIGGER] SPAWN_GROUP: selected group resource is unavailable: " + prefab, LogLevel.WARNING);
			return;
		}

		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(sp.Transform);
		sp.Transform[3] = DCO_GetCenter();

		IEntity grpEnt = GetGame().SpawnEntityPrefabEx(prefab, false, owner.GetWorld(), sp);
		if (!grpEnt)
		{
			Print("[DCO-TRIGGER] SPAWN_GROUP: prefab failed to spawn: " + prefab, LogLevel.WARNING);
			return;
		}

		Print("[DCO-TRIGGER] spawned group at trigger: " + prefab, LogLevel.NORMAL);
	}

	// Start the nearest pairable Bifrost FX emitter within the FX pair radius.
	protected void DCO_ActionFireFx()
	{
		// Repeat mode holds ONE emitter latched from rising edge to falling edge.
		if (m_bRepeat && m_FxTarget)
		{
			if (!DCO_FxIsFiring(m_FxTarget))
				DCO_FxToggle(m_FxTarget, true);
			return;
		}

		IEntity best = DCO_FindNearestFxEmitter();
		if (!best)
		{
			Print(string.Format("[DCO-TRIGGER] FIRE_FX: no FX emitter within %1 m of the trigger", m_fFxPairRadius), LogLevel.WARNING);
			return;
		}

		if (!DCO_FxToggle(best, true))
		{
			Print("[DCO-TRIGGER] FIRE_FX: paired entity has no toggleable FX component", LogLevel.WARNING);
			return;
		}

		if (m_bRepeat)
			m_FxTarget = best;
	}

	protected IEntity DCO_FindNearestFxEmitter()
	{
		m_vFxSearchCenter = DCO_GetCenter();
		m_fFxBestSq = m_fFxPairRadius * m_fFxPairRadius;
		m_FxBest = null;

		array<IEntity> reg = DCO_TriggerFxRegistry.GetEmitters();
		foreach (IEntity e : reg)
			DCO_FxConsiderCandidate(e);

		IEntity owner = GetOwner();
		if (owner && owner.GetWorld())
			owner.GetWorld().QueryEntitiesBySphere(m_vFxSearchCenter, m_fFxPairRadius, DCO_FxQueryCallback);

		return m_FxBest;
	}

	protected bool DCO_FxQueryCallback(IEntity e)
	{
		DCO_FxConsiderCandidate(e);
		return true;
	}

	protected void DCO_FxConsiderCandidate(IEntity e)
	{
		if (!e || e == m_FxBest)
			return;
		if (!DCO_FxIsToggleable(e))
			return;
		float dSq = DCO_FlatDistSq(e.GetOrigin(), m_vFxSearchCenter);
		if (dSq > m_fFxBestSq)
			return;
		m_fFxBestSq = dSq;
		m_FxBest = e;
	}

	protected bool DCO_FxIsToggleable(IEntity e)
	{
		if (DCO_TracerEmitterComponent.Cast(e.FindComponent(DCO_TracerEmitterComponent)))
			return true;
		if (DCO_FxExplosionComponent.Cast(e.FindComponent(DCO_FxExplosionComponent)))
			return true;
		if (DCO_FxMortarComponent.Cast(e.FindComponent(DCO_FxMortarComponent)))
			return true;
		return false;
	}

	// Resolve a paired entity to its concrete FX component and switch it.
	protected bool DCO_FxToggle(IEntity e, bool on)
	{
		if (!e)
			return false;

		DCO_TracerEmitterComponent tracer = DCO_TracerEmitterComponent.Cast(e.FindComponent(DCO_TracerEmitterComponent));
		if (tracer)
		{
			tracer.DCO_SetFiring(on);
			return true;
		}

		DCO_FxExplosionComponent boom = DCO_FxExplosionComponent.Cast(e.FindComponent(DCO_FxExplosionComponent));
		if (boom)
		{
			boom.DCO_SetFiring(on);
			return true;
		}

		DCO_FxMortarComponent mortar = DCO_FxMortarComponent.Cast(e.FindComponent(DCO_FxMortarComponent));
		if (mortar)
		{
			mortar.DCO_SetFiring(on);
			return true;
		}

		return false;
	}

	protected bool DCO_FxIsFiring(IEntity e)
	{
		if (!e)
			return false;

		DCO_TracerEmitterComponent tracer = DCO_TracerEmitterComponent.Cast(e.FindComponent(DCO_TracerEmitterComponent));
		if (tracer)
			return tracer.DCO_IsFiring();

		DCO_FxExplosionComponent boom = DCO_FxExplosionComponent.Cast(e.FindComponent(DCO_FxExplosionComponent));
		if (boom)
			return boom.DCO_IsFiring();

		DCO_FxMortarComponent mortar = DCO_FxMortarComponent.Cast(e.FindComponent(DCO_FxMortarComponent));
		if (mortar)
			return mortar.DCO_IsFiring();

		return false;
	}

	protected void DCO_FxStop()
	{
		if (!m_FxTarget)
			return;
		DCO_FxToggle(m_FxTarget, false);
		m_FxTarget = null;
	}

}
