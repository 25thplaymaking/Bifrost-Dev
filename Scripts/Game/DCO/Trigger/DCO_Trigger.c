// GM Trigger - an configurable placeable trigger area.
enum EDCO_TriggerCondition
{
	ANY_CHARACTER,	// any AI- or player-controlled character inside.
	PLAYERS_ONLY,	// only player-controlled characters count.
	FACTION_US,
	FACTION_USSR,
	FACTION_FIA,
	FACTION_CIV,
}

enum EDCO_TriggerAction
{
	NOTIFY,	// on-screen warning notification to every player.
	SEND_QRF,	// dispatch hostile QRF-flagged AI groups to the trigger point.
	SPRING_AMBUSH,
	SPAWN_GROUP,	// spawn the selected engine AI group at the trigger center.
	FIRE_FX,	// start the nearest Bifrost FX emitter within the FX pair radius.
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
	[Attribute("0", UIWidgets.ComboBox, "Who trips this trigger: anyone, players only, or any character of a specific faction.", "", ParamEnumArray.FromEnum(EDCO_TriggerCondition), category: "Bifrost"), RplProp()]
	EDCO_TriggerCondition m_eCondition;

	[RplProp()]
	protected FactionKey m_sConditionFactionKey;

	[Attribute("25", UIWidgets.Slider, "Trigger radius (m).", "5 500 5", category: "Bifrost"), RplProp()]
	float m_fRadius;

	[Attribute("1", UIWidgets.Slider, "How many matching characters must be inside at once before the trigger fires.", "1 30 1", category: "Bifrost"), RplProp()]
	int m_iCountThreshold;

	[Attribute("0", UIWidgets.CheckBox, "Repeat mode: fires again (at most once per cooldown) while the condition holds. Off = fires ONCE then latches; toggle Enabled off/on to re-arm.", category: "Bifrost"), RplProp()]
	bool m_bRepeat;

	[Attribute("10", UIWidgets.Slider, "Repeat cooldown (s): minimum time between fires in Repeat mode.", "1 300 1", category: "Bifrost"), RplProp()]
	float m_fCooldownSec;

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

	[Attribute("1", UIWidgets.CheckBox, "Armed. Off = the trigger never fires. Turning it back ON also re-arms a tripped Once trigger.", category: "Bifrost"), RplProp()]
	bool m_bEnabled;

	[Attribute("2", UIWidgets.Slider, "How often (s) the trigger re-evaluates its condition. Keep at 2+ with many triggers placed.", "0.5 15 0.5", category: "Bifrost")]
	float m_fCheckSec;

	protected static const float DCO_VISUAL_HEIGHT = 3.0;
	protected static const int COLOR_ARMED = 0xFFFF3030;
	protected static const int COLOR_DISABLED = 0xFF707070;	// grey - not armed.
	protected static const int COLOR_SPENT = 0xFF454545;	// dark grey - Once trigger already fired.

	static const ref array<string> COND_NAMES = {
		"Anyone (AI or player)",
		"Players only",
	};

	static const ref array<string> ACTION_NAMES = {
		"Notify everyone",
		"Send QRF here",
		"Spring paired ambush",
		"Spawn group here",
		"Fire paired FX emitter",
	};

	static const ref array<string> SPAWN_GROUP_NAMES = {
		"US Fire Team",
		"US Rifle Squad",
		"US MG Team",
		"Soviet Fire Group",
		"Soviet Rifle Squad",
		"Soviet MG Team",
		"FIA Fire Team",
		"FIA Rifle Squad",
	};

	protected static const ref array<ResourceName> GROUP_PREFABS = {
		"{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et",
		"{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et",
		"{958039B857396B7B}Prefabs/Groups/BLUFOR/Group_US_MachineGunTeam.et",
		"{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et",
		"{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et",
		"{A2F75E45C66B1C0A}Prefabs/Groups/OPFOR/Group_USSR_MachineGunTeam.et",
		"{5BEA04939D148B1D}Prefabs/Groups/INDFOR/Group_FIA_FireTeam.et",
		"{CE41AF625D05D0F0}Prefabs/Groups/INDFOR/Group_FIA_RifleSquad.et",
	};

	[RplProp()]
	protected bool m_bTripped = false;	// Once-mode latch.
	protected float m_fLastFireMs = -1;
	protected bool m_bCondActive = false;
	protected IEntity m_FxTarget;	// emitter this trigger switched ON in Repeat mode.
	protected ref Shape m_DCO_VisualShape;

	protected vector m_vFxSearchCenter;
	protected float m_fFxBestSq;
	protected IEntity m_FxBest;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame() || !GetGame().InPlayMode())
			return;

		GetGame().GetCallqueue().CallLater(DCO_DrawVisual, 500, false);
		GetGame().GetCallqueue().CallLater(DCO_DrawVisual, (int)(m_fCheckSec * 1000.0), true);

		if (!Replication.IsServer())
			return;

		DCO_TriggerRegistry.Register(this);
		GetGame().GetCallqueue().CallLater(DCO_Tick, (int)(m_fCheckSec * 1000.0), true);
	}

	void ~DCO_TriggerComponent()
	{
		DCO_TriggerRegistry.Unregister(this);
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(DCO_Tick);
			GetGame().GetCallqueue().Remove(DCO_DrawVisual);
			DCO_FxStop();
		}
		m_DCO_VisualShape = null;
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
			return Math.Clamp(m_eCondition, 0, COND_NAMES.Count() - 1);
		int index = DCO_FactionCatalog.IndexOf(m_sConditionFactionKey);
		if (index < 0)
			return DCO_GetConditionCount();
		return COND_NAMES.Count() + index;
	}
	void DCO_SetCondition(int c)
	{
		if (c < COND_NAMES.Count())
		{
			m_eCondition = Math.Clamp(c, 0, COND_NAMES.Count() - 1);
			m_sConditionFactionKey = "";
		}
		else
		{
			FactionKey key = DCO_FactionCatalog.KeyAt(c - COND_NAMES.Count());
			if (!key.IsEmpty())
			{
				m_sConditionFactionKey = key;
				m_eCondition = EDCO_TriggerCondition.FACTION_US;
			}
		}
		DCO_ReplicateState();
	}
	static int DCO_GetConditionCount()	{ return COND_NAMES.Count() + DCO_FactionCatalog.Count(); }
	static string DCO_GetConditionName(int index)
	{
		if (index >= 0 && index < COND_NAMES.Count())
			return COND_NAMES[index];
		return "Faction: " + DCO_FactionCatalog.NameAt(index - COND_NAMES.Count());
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
	int DCO_GetCountThreshold()			{ return m_iCountThreshold; }
	void DCO_SetCountThreshold(int c)	{ m_iCountThreshold = Math.Clamp(c, 1, 30); DCO_ReplicateState(); }
	bool DCO_GetRepeat()				{ return m_bRepeat; }
	float DCO_GetCooldown()				{ return m_fCooldownSec; }
	void DCO_SetCooldown(float s)		{ m_fCooldownSec = Math.Clamp(s, 1, 300); DCO_ReplicateState(); }
	int DCO_GetAction()					{ return m_eAction; }
	void DCO_SetAction(int a)			{ m_eAction = Math.Clamp(a, 0, ACTION_NAMES.Count() - 1); DCO_ReplicateState(); }
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

	protected void DCO_MigrateLegacySelections()
	{
		if (m_sConditionFactionKey.IsEmpty() && m_eCondition >= EDCO_TriggerCondition.FACTION_US)
		{
			int legacyFaction = m_eCondition - EDCO_TriggerCondition.FACTION_US;
			array<FactionKey> legacyKeys = {"US", "USSR", "FIA", "CIV"};
			if (legacyKeys.IsIndexValid(legacyFaction))
				m_sConditionFactionKey = legacyKeys[legacyFaction];
		}
		if (m_rSpawnGroupPrefab.IsEmpty())
		{
			int legacyGroup = Math.Clamp(m_eSpawnGroup, 0, GROUP_PREFABS.Count() - 1);
			m_rSpawnGroupPrefab = GROUP_PREFABS[legacyGroup];
			m_sSpawnFactionKey = DCO_TriggerGroupCatalog.FactionForPrefab(m_rSpawnGroupPrefab);
		}
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
			DCO_FxStop();
			m_bCondActive = false;
			return;
		}
		if (was)
			return;
		m_bTripped = false;
		m_fLastFireMs = -1;
		DCO_ReplicateState();
	}

	void DCO_SetRepeat(bool on)
	{
		if (!on)
			DCO_FxStop();
		m_bRepeat = on;
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
		int count = DCO_CountMatchesInside(tripper);
		bool active = count >= Math.Clamp(m_iCountThreshold, 1, 30);

		if (active)
		{
			if (!m_bRepeat)
			{
				m_bTripped = true;
				DCO_ReplicateState();
				DCO_Fire(tripper, count);
			}
			else
			{
				BaseWorld world = GetGame().GetWorld();
				float now = 0;
				if (world)
					now = world.GetWorldTime();
				if (m_fLastFireMs < 0 || (now - m_fLastFireMs) >= (m_fCooldownSec * 1000.0))
				{
					m_fLastFireMs = now;
					DCO_Fire(tripper, count);
				}
			}
		}
		else
		{
			// Falling edge in Repeat mode: the circle emptied - release a held FX emitter.
			if (m_bRepeat && m_bCondActive)
				DCO_FxStop();
		}

		m_bCondActive = active;
	}

	// Count characters inside the circle that match the condition.
	protected int DCO_CountMatchesInside(out IEntity firstMatch)
	{
		vector center = DCO_GetCenter();
		float radiusSq = m_fRadius * m_fRadius;
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
				if (!ent)
					continue;
				if (DCO_FlatDistSq(ent.GetOrigin(), center) > radiusSq)
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
				if (!ent)
					continue;
				if (DCO_PlayerUtil.IsPlayer(ent))
					continue;	// already counted in the player loop.
				if (DCO_FlatDistSq(ent.GetOrigin(), center) > radiusSq)
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

	protected void DCO_Fire(IEntity tripper, int count)
	{
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
		}

		Print(string.Format("[DCO-TRIGGER] fired: action=%1 condition=%2 matches=%3 repeat=%4",
			ACTION_NAMES[Math.Clamp(m_eAction, 0, ACTION_NAMES.Count() - 1)],
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

	// The GM-visible circle.
	protected void DCO_DrawVisual()
	{
		// GM-only: players must not see trigger circles, and a dedicated server has no renderer.
		if (!DCO_GMRights.IsLocalGameMaster())
		{
			m_DCO_VisualShape = null;
			return;
		}

		if (m_fRadius < 1.0)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		int color = COLOR_ARMED;
		if (!m_bEnabled)
			color = COLOR_DISABLED;
		else if (m_bTripped && !m_bRepeat)
			color = COLOR_SPENT;

		m_DCO_VisualShape = DCO_ZoneShape.FlatCircle(owner.GetOrigin(), m_fRadius, color);
	}
}
