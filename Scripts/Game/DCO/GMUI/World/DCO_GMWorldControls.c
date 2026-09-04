enum EDCO_GMWorldControlAction
{
	OPEN_DOORS = 1,
	CLOSE_DOORS,
	LIGHTS_ON,
	LIGHTS_OFF,
	SURRENDER,
	RESTORE,
	GARRISON,
	RELEASE_GARRISON
}

class DCO_GMWorldStateGuard
{
	protected static BaseWorld s_World;
	protected static int s_iWorldSerial;

	static int Serial()
	{
		BaseWorld world = GetGame().GetWorld();
		if (world != s_World)
		{
			s_World = world;
			s_iWorldSerial++;
		}
		return s_iWorldSerial;
	}
}

class DCO_GMWorldDoorCollector
{
	protected IEntity m_Target;
	ref array<DoorComponent> m_aDoors = {};

	void DCO_GMWorldDoorCollector(IEntity target)
	{
		m_Target = target;
	}

	bool Collect(IEntity entity)
	{
		if (!entity || !DCO_CqbClearUtil.IsSelectedBuildingPart(entity, m_Target))
			return true;

		DoorComponent door = DoorComponent.Cast(entity.FindComponent(DoorComponent));
		if (door && m_aDoors.Find(door) < 0)
			m_aDoors.Insert(door);
		return true;
	}
}

class DCO_GMWorldDoorController
{
	static int FindDoors(IEntity target, out notnull array<DoorComponent> doors)
	{
		doors.Clear();
		if (!target)
			return 0;

		DoorComponent direct = DoorComponent.Cast(target.FindComponent(DoorComponent));
		if (direct)
			doors.Insert(direct);

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return doors.Count();

		vector mins;
		vector maxs;
		target.GetBounds(mins, maxs);
		vector size = maxs - mins;
		float radius = Math.Max(2.0, size.Length() * 0.5 + 2.0);
		vector center = target.CoordToParent((mins + maxs) * 0.5);
		DCO_GMWorldDoorCollector collector = new DCO_GMWorldDoorCollector(target);
		world.QueryEntitiesBySphere(center, radius, collector.Collect);
		foreach (DoorComponent found : collector.m_aDoors)
		{
			if (found && doors.Find(found) < 0)
				doors.Insert(found);
		}
		return doors.Count();
	}

	static int Apply(IEntity target, bool open)
	{
		if (!Replication.IsServer())
			return 0;

		array<DoorComponent> doors = {};
		FindDoors(target, doors);
		RplId instigator;
		RplComponent replication = RplComponent.Cast(target.FindComponent(RplComponent));
		if (replication)
			instigator = replication.Id();
		float control = 0;
		if (open)
			control = 1;
		foreach (DoorComponent door : doors)
		{
			if (door)
				door.SetControlValue(control, instigator);
		}
		return doors.Count();
	}
}

class DCO_GMWorldLightController
{
	protected static int SetCharacter(IEntity character, bool enabled)
	{
		if (!character || DCO_PlayerUtil.IsPlayer(character))
			return 0;

		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(character);
		if (!manager)
			return 0;

		array<SCR_GadgetComponent> gadgets = manager.GetGadgetsByType(EGadgetType.FLASHLIGHT);
		if (!gadgets)
			return 0;
		int changed;
		foreach (SCR_GadgetComponent gadget : gadgets)
		{
			if (!gadget || !gadget.GetOwner())
				continue;

			InventoryItemComponent item = InventoryItemComponent.Cast(gadget.GetOwner().FindComponent(InventoryItemComponent));
			EquipmentStorageSlot slot;
			if (item)
				slot = EquipmentStorageSlot.Cast(item.GetParentSlot());
			if (!slot)
				continue;

			bool wanted = enabled && !slot.IsOccluded();
			if (gadget.IsToggledOn() == wanted)
				continue;
			gadget.ToggleActive(wanted, SCR_EUseContext.FROM_ACTION);
			changed++;
		}
		return changed;
	}

	protected static int SetVehicle(IEntity vehicle, bool enabled)
	{
		if (!vehicle)
			return 0;
		BaseLightManagerComponent manager = BaseLightManagerComponent.Cast(vehicle.FindComponent(BaseLightManagerComponent));
		if (!manager)
			return 0;

		manager.SetLightsState(ELightType.Head, enabled);
		manager.SetLightsState(ELightType.Presence, enabled);
		if (!enabled)
			manager.SetLightsState(ELightType.HiBeam, false);
		return 1;
	}

	static int Apply(IEntity target, bool enabled)
	{
		if (!Replication.IsServer() || !target)
			return 0;

		SCR_AIGroup group = SCR_AIGroup.Cast(target);
		if (group)
		{
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			int changed;
			foreach (AIAgent agent : agents)
			{
				if (agent)
					changed += SetCharacter(agent.GetControlledEntity(), enabled);
			}
			return changed;
		}

		if (ChimeraCharacter.Cast(target))
			return SetCharacter(target, enabled);
		if (Vehicle.Cast(target))
			return SetVehicle(target, enabled);
		return 0;
	}
}

modded class SCR_AIInfoComponent
{
	[RplProp()]
	protected bool m_bDCO_Surrendered;

	bool DCO_IsSurrendered()
	{
		return m_bDCO_Surrendered;
	}

	void DCO_SetSurrendered(bool surrendered)
	{
		if (!Replication.IsServer() || m_bDCO_Surrendered == surrendered)
			return;
		m_bDCO_Surrendered = surrendered;
		Replication.BumpMe();
	}
}

class DCO_GMSurrenderRecord
{
	IEntity m_Entity;
	int m_iWorldSerial;
	bool m_bAIWasActive;
	bool m_bWeaponWasRaised;
	int m_iPreviousPermanentLOD = -1;
	EUnitAIState m_ePreviousAIState;
}

class DCO_GMSurrenderService
{
	protected static ref array<ref DCO_GMSurrenderRecord> s_aRecords;

	protected static int PruneRecords()
	{
		int worldSerial = DCO_GMWorldStateGuard.Serial();
		if (!s_aRecords)
			return worldSerial;
		for (int i = s_aRecords.Count() - 1; i >= 0; i--)
		{
			DCO_GMSurrenderRecord record = s_aRecords[i];
			if (!record || record.m_iWorldSerial != worldSerial || !record.m_Entity || record.m_Entity.IsDeleted())
				s_aRecords.Remove(i);
		}
		return worldSerial;
	}

	protected static SCR_AIInfoComponent FindInfo(IEntity entity, out AIControlComponent control)
	{
		control = null;
		if (!entity)
			return null;
		control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		AIAgent agent;
		if (control)
			agent = control.GetAIAgent();
		if (!agent)
			return null;
		return SCR_AIInfoComponent.Cast(agent.FindComponent(SCR_AIInfoComponent));
	}

	protected static DCO_GMSurrenderRecord FindRecord(IEntity entity)
	{
		PruneRecords();
		if (!s_aRecords)
			return null;
		foreach (DCO_GMSurrenderRecord record : s_aRecords)
		{
			if (record.m_Entity == entity)
				return record;
		}
		return null;
	}

	static bool IsSurrendered(IEntity entity)
	{
		PruneRecords();
		AIControlComponent control;
		SCR_AIInfoComponent info = FindInfo(entity, control);
		return info && info.DCO_IsSurrendered();
	}

	protected static bool SetCharacter(IEntity entity, bool surrendered)
	{
		int worldSerial = PruneRecords();
		if (!entity || DCO_PlayerUtil.IsPlayer(entity))
			return false;

		AIControlComponent control;
		SCR_AIInfoComponent info = FindInfo(entity, control);
		if (!info || !control)
			return false;
		if (info.DCO_IsSurrendered() == surrendered)
			return true;

		if (surrendered)
		{
			if (!DCO_AIAnimationService.Apply(entity, EDCO_AIEditorAnimation.SIT_GROUND, false))
				return false;

			DCO_GMSurrenderRecord record = new DCO_GMSurrenderRecord();
			record.m_Entity = entity;
			record.m_iWorldSerial = worldSerial;
			record.m_bAIWasActive = control.IsAIActivated();
			record.m_ePreviousAIState = info.GetAIState();
			AIAgent agent = control.GetAIAgent();
			if (agent)
				record.m_iPreviousPermanentLOD = agent.GetPermanentLOD();
			if (!s_aRecords)
				s_aRecords = {};
			s_aRecords.Insert(record);

			SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(
				entity.FindComponent(SCR_CharacterControllerComponent));
			if (controller)
			{
				record.m_bWeaponWasRaised = controller.IsWeaponRaised();
				controller.SetWeaponRaised(false);
			}
			info.SetAIState(EUnitAIState.UNRESPONSIVE);
			if (agent)
				agent.SetPermanentLOD(AIAgent.GetMaxLOD());
			if (control.IsAIActivated())
				control.DeactivateAI();
			info.DCO_SetSurrendered(true);
			return true;
		}

		DCO_AIAnimationService.Apply(entity, EDCO_AIEditorAnimation.NONE, false);
		DCO_GMSurrenderRecord oldRecord = FindRecord(entity);
		if (oldRecord)
		{
			info.SetAIState(oldRecord.m_ePreviousAIState);
			SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(
				entity.FindComponent(SCR_CharacterControllerComponent));
			if (controller)
				controller.SetWeaponRaised(oldRecord.m_bWeaponWasRaised);
			if (oldRecord.m_bAIWasActive && !control.IsAIActivated())
				control.ActivateAI();
			AIAgent restoredAgent = control.GetAIAgent();
			if (restoredAgent)
				restoredAgent.SetPermanentLOD(oldRecord.m_iPreviousPermanentLOD);
			s_aRecords.RemoveItem(oldRecord);
		}
		else
		{
			info.SetAIState(EUnitAIState.AVAILABLE);
			if (!control.IsAIActivated())
				control.ActivateAI();
			AIAgent restoredAgent = control.GetAIAgent();
			if (restoredAgent)
				restoredAgent.SetPermanentLOD(-1);
		}
		info.DCO_SetSurrendered(false);
		return true;
	}

	static int Apply(IEntity target, bool surrendered)
	{
		PruneRecords();
		if (!Replication.IsServer() || !target)
			return 0;
		SCR_AIGroup group = SCR_AIGroup.Cast(target);
		if (!group)
		{
			if (SetCharacter(target, surrendered))
				return 1;
			return 0;
		}

		array<AIAgent> agents = {};
		group.GetAgents(agents);
		int changed;
		foreach (AIAgent agent : agents)
		{
			if (agent && SetCharacter(agent.GetControlledEntity(), surrendered))
				changed++;
		}
		return changed;
	}
}

modded class SCR_AIGroupUtilityComponent
{
	[RplProp()]
	protected bool m_bDCO_GMGarrisoned;
	protected IEntity m_DCO_GarrisonBuilding;
	protected ref array<AIAgent> m_aDCO_GarrisonAgents;
	protected ref array<vector> m_aDCO_GarrisonPositions;
	protected ref array<vector> m_aDCO_GarrisonCells;
	protected ref array<AIWaypoint> m_aDCO_GarrisonPreviousWaypoints;
	protected float m_fDCO_GarrisonLastUpdate = -1;
	protected float m_fDCO_GarrisonPreviousAutonomousDistance;
	protected EAIGroupCombatMode m_eDCO_GarrisonPreviousCombatMode;

	bool DCO_IsGMGarrisoned()
	{
		return m_bDCO_GMGarrisoned;
	}

	protected void DCO_GarrisonIssueMove(AIAgent agent, vector position)
	{
		if (!agent || !m_Mailbox)
			return;
		IEntity character = agent.GetControlledEntity();
		if (!character || DCO_PlayerUtil.IsPlayer(character))
			return;
		SCR_AIMessage_Move move = SCR_AIMessage_Move.Create(null, position, EMovementType.WALK, false, null);
		if (move)
			m_Mailbox.RequestBroadcast(move, agent);
	}

	protected void DCO_RebuildGarrisonAssignments()
	{
		if (!m_aDCO_GarrisonCells || m_aDCO_GarrisonCells.IsEmpty())
			return;
		if (!m_aDCO_GarrisonAgents)
			m_aDCO_GarrisonAgents = {};
		if (!m_aDCO_GarrisonPositions)
			m_aDCO_GarrisonPositions = {};
		m_aDCO_GarrisonAgents.Clear();
		m_aDCO_GarrisonPositions.Clear();

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		int cellIndex;
		foreach (AIAgent agent : agents)
		{
			if (!agent || !agent.GetControlledEntity() || DCO_PlayerUtil.IsPlayer(agent.GetControlledEntity()))
				continue;
			vector position = m_aDCO_GarrisonCells[cellIndex % m_aDCO_GarrisonCells.Count()];
			m_aDCO_GarrisonAgents.Insert(agent);
			m_aDCO_GarrisonPositions.Insert(position);
			DCO_GarrisonIssueMove(agent, position);
			cellIndex++;
		}
	}

	protected void DCO_RestoreGarrisonWaypoints()
	{
		if (!m_Owner || !m_aDCO_GarrisonPreviousWaypoints)
			return;
		array<AIWaypoint> current = {};
		m_Owner.GetWaypoints(current);
		foreach (AIWaypoint waypoint : m_aDCO_GarrisonPreviousWaypoints)
		{
			if (!waypoint || waypoint.IsDeleted() || current.Contains(waypoint))
				continue;
			m_Owner.AddWaypoint(waypoint);
			current.Insert(waypoint);
		}
		m_aDCO_GarrisonPreviousWaypoints.Clear();
	}

	bool DCO_ApplyGMGarrison(IEntity building, notnull array<vector> cells)
	{
		if (!Replication.IsServer() || !m_Owner || !building || cells.IsEmpty())
			return false;

		DCO_ClearGMGarrison();
		m_DCO_GarrisonBuilding = building;
		m_aDCO_GarrisonCells = {};
		foreach (vector cell : cells)
			m_aDCO_GarrisonCells.Insert(cell);
		m_fDCO_GarrisonPreviousAutonomousDistance = GetMaxAutonomousDistance();
		m_eDCO_GarrisonPreviousCombatMode = GetCombatModeExternal();
		m_aDCO_GarrisonPreviousWaypoints = {};
		m_Owner.GetWaypoints(m_aDCO_GarrisonPreviousWaypoints);
		m_bDCO_GMGarrisoned = true;
		SetMaxAutonomousDistance(8.0);
		SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);

		for (int waypointIndex = m_aDCO_GarrisonPreviousWaypoints.Count() - 1; waypointIndex >= 0; waypointIndex--)
			m_Owner.RemoveWaypointAt(0);

		DCO_RebuildGarrisonAssignments();
		if (!m_aDCO_GarrisonAgents || m_aDCO_GarrisonAgents.IsEmpty())
		{
			DCO_ClearGMGarrison();
			return false;
		}
		m_fDCO_GarrisonLastUpdate = -1;
		Replication.BumpMe();
		return true;
	}

	void DCO_ClearGMGarrison()
	{
		if (!Replication.IsServer())
			return;
		bool wasActive = m_bDCO_GMGarrisoned;
		m_bDCO_GMGarrisoned = false;
		m_DCO_GarrisonBuilding = null;
		if (m_aDCO_GarrisonAgents)
			m_aDCO_GarrisonAgents.Clear();
		if (m_aDCO_GarrisonPositions)
			m_aDCO_GarrisonPositions.Clear();
		if (m_aDCO_GarrisonCells)
			m_aDCO_GarrisonCells.Clear();
		if (wasActive)
		{
			SetMaxAutonomousDistance(m_fDCO_GarrisonPreviousAutonomousDistance);
			SetCombatMode(m_eDCO_GarrisonPreviousCombatMode);
			DCO_RestoreGarrisonWaypoints();
			Replication.BumpMe();
		}
	}

	void DCO_UpdateGMGarrison()
	{
		if (!Replication.IsServer() || !m_bDCO_GMGarrisoned || !m_Owner)
			return;
		if (!m_DCO_GarrisonBuilding || m_DCO_GarrisonBuilding.IsDeleted())
		{
			DCO_ClearGMGarrison();
			return;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_GarrisonLastUpdate >= 0 && now - m_fDCO_GarrisonLastUpdate < 1200)
			return;
		m_fDCO_GarrisonLastUpdate = now;

		array<AIAgent> current = {};
		m_Owner.GetAgents(current);
		int usable;
		foreach (AIAgent candidate : current)
		{
			if (candidate && candidate.GetControlledEntity() && !DCO_PlayerUtil.IsPlayer(candidate.GetControlledEntity()))
				usable++;
		}
		if (!m_aDCO_GarrisonAgents || usable != m_aDCO_GarrisonAgents.Count())
			DCO_RebuildGarrisonAssignments();
		if (!m_aDCO_GarrisonAgents || !m_aDCO_GarrisonPositions)
			return;

		int assignmentCount = Math.Min(m_aDCO_GarrisonAgents.Count(), m_aDCO_GarrisonPositions.Count());
		for (int i = 0; i < assignmentCount; i++)
		{
			AIAgent agent = m_aDCO_GarrisonAgents[i];
			IEntity character;
			if (agent)
				character = agent.GetControlledEntity();
			if (!character || character.IsDeleted() || DCO_PlayerUtil.IsPlayer(character))
				continue;
			if (vector.DistanceSq(character.GetOrigin(), m_aDCO_GarrisonPositions[i]) > 9.0)
				DCO_GarrisonIssueMove(agent, m_aDCO_GarrisonPositions[i]);
		}
	}
}

class DCO_GMGarrisonSurvey
{
	SCR_AIGroup m_Group;
	IEntity m_Building;
	SCR_PlayerController m_Requester;
	int m_iWorldSerial;
	int m_iAttempts;
}

class DCO_GMGarrisonService
{
	protected static const int MAX_SURVEY_ATTEMPTS = 25;
	protected static ref array<ref DCO_GMGarrisonSurvey> s_aSurveys;

	protected static int PruneSurveys()
	{
		int worldSerial = DCO_GMWorldStateGuard.Serial();
		if (!s_aSurveys)
			return worldSerial;
		for (int i = s_aSurveys.Count() - 1; i >= 0; i--)
		{
			DCO_GMGarrisonSurvey survey = s_aSurveys[i];
			if (!survey || survey.m_iWorldSerial != worldSerial || !survey.m_Group || survey.m_Group.IsDeleted())
				s_aSurveys.Remove(i);
		}
		return worldSerial;
	}

	static bool IsBuilding(IEntity entity)
	{
		return entity && (Building.Cast(entity) || entity.FindComponent(SCR_DestructibleBuildingComponent));
	}

	protected static AIPathfindingComponent FindPathfinding(SCR_AIGroup group)
	{
		if (!group)
			return null;
		AIPathfindingComponent pathfinding = AIPathfindingComponent.Cast(group.FindComponent(AIPathfindingComponent));
		if (!pathfinding && group.GetLeaderEntity())
			pathfinding = AIPathfindingComponent.Cast(group.GetLeaderEntity().FindComponent(AIPathfindingComponent));
		return pathfinding;
	}

	static bool Begin(SCR_AIGroup group, IEntity building, SCR_PlayerController requester, out string result)
	{
		result = "Garrison failed";
		if (!Replication.IsServer() || !group || !IsBuilding(building))
			return false;
		if (!FindPathfinding(group))
		{
			result = "Garrison failed: selected group has no pathfinding component.";
			return false;
		}

		DCO_GMGarrisonSurvey survey = new DCO_GMGarrisonSurvey();
		survey.m_Group = group;
		survey.m_Building = building;
		survey.m_Requester = requester;
		survey.m_iWorldSerial = PruneSurveys();
		if (!s_aSurveys)
			s_aSurveys = {};
		for (int i = s_aSurveys.Count() - 1; i >= 0; i--)
		{
			DCO_GMGarrisonSurvey pending = s_aSurveys[i];
			if (!pending || pending.m_Group == group)
				s_aSurveys.Remove(i);
		}
		s_aSurveys.Insert(survey);
		bool successful;
		if (Process(survey, result, successful))
		{
			s_aSurveys.RemoveItem(survey);
			return successful;
		}

		result = "Garrison survey queued while interior navmesh streams.";
		GetGame().GetCallqueue().CallLater(Retry, 400, false, survey);
		return true;
	}

	protected static bool Process(DCO_GMGarrisonSurvey survey, out string result, out bool successful)
	{
		result = "Garrison survey pending";
		successful = false;
		if (!survey || survey.m_iWorldSerial != DCO_GMWorldStateGuard.Serial() || !survey.m_Group
			|| survey.m_Group.IsDeleted() || !survey.m_Building || survey.m_Building.IsDeleted())
		{
			result = "Garrison failed: selection is no longer valid.";
			return true;
		}

		AIPathfindingComponent pathfinding = FindPathfinding(survey.m_Group);
		array<vector> cells = {};
		array<bool> doors = {};
		EDCO_CqbSurveyResult surveyResult = DCO_CqbClearUtil.CollectInteriorCells(
			pathfinding, survey.m_Building, cells, doors);
		if (surveyResult == EDCO_CqbSurveyResult.WAITING)
			return false;
		if (surveyResult == EDCO_CqbSurveyResult.INVALID)
		{
			result = "Garrison failed: interior navmesh is unavailable.";
			return true;
		}

		array<vector> roomCells = {};
		for (int i = 0; i < cells.Count(); i++)
		{
			if (!doors[i])
				roomCells.Insert(cells[i]);
		}
		if (roomCells.IsEmpty())
		{
			result = "Garrison failed: building has no reachable interior positions.";
			return true;
		}

		SCR_AIGroupUtilityComponent utility = survey.m_Group.GetGroupUtilityComponent();
		if (!utility || !utility.DCO_ApplyGMGarrison(survey.m_Building, roomCells))
		{
			result = "Garrison failed: group has no usable AI members.";
			return true;
		}
		result = string.Format("Garrison assigned: %1 interior positions available.", roomCells.Count());
		successful = true;
		return true;
	}

	protected static void Retry(DCO_GMGarrisonSurvey survey)
	{
		PruneSurveys();
		if (!survey || !s_aSurveys || s_aSurveys.Find(survey) < 0)
			return;
		survey.m_iAttempts++;
		string result;
		bool successful;
		bool complete = Process(survey, result, successful);
		if (!complete && survey.m_iAttempts < MAX_SURVEY_ATTEMPTS)
		{
			GetGame().GetCallqueue().CallLater(Retry, 400, false, survey);
			return;
		}
		if (!complete)
		{
			result = "Garrison failed: interior navmesh streaming timed out.";
			successful = false;
		}
		s_aSurveys.RemoveItem(survey);
		LogLevel level = LogLevel.WARNING;
		if (successful)
			level = LogLevel.NORMAL;
		Print("[DCO-GM] " + result, level);
		if (survey.m_Requester)
			survey.m_Requester.DCO_SendGMWorldControlResult(successful, result);
	}
}

class DCO_GMWorldControlServer
{
	static bool Apply(int action, RplId targetId, RplId auxiliaryId, SCR_PlayerController requester, out string result)
	{
		result = "World control failed";
		if (!Replication.IsServer())
			return false;

		SCR_EditableEntityComponent editable;
		int categories;
		int allowed = EDCO_GMBatchTargetCategory.ENTITY;
		if (action == EDCO_GMWorldControlAction.LIGHTS_ON || action == EDCO_GMWorldControlAction.LIGHTS_OFF)
			allowed = EDCO_GMBatchTargetCategory.CHARACTER | EDCO_GMBatchTargetCategory.GROUP | EDCO_GMBatchTargetCategory.VEHICLE;
		else if (action == EDCO_GMWorldControlAction.SURRENDER || action == EDCO_GMWorldControlAction.RESTORE)
			allowed = EDCO_GMBatchTargetCategory.CHARACTER | EDCO_GMBatchTargetCategory.GROUP;
		else if (action == EDCO_GMWorldControlAction.GARRISON || action == EDCO_GMWorldControlAction.RELEASE_GARRISON)
			allowed = EDCO_GMBatchTargetCategory.GROUP;
		if (!DCO_GMBatchAuthority.Resolve(targetId, allowed, editable, categories))
		{
			result = "World control skipped: target is no longer valid.";
			return false;
		}

		IEntity target = editable.GetOwner();
		int changed;
		switch (action)
		{
			case EDCO_GMWorldControlAction.OPEN_DOORS:
				changed = DCO_GMWorldDoorController.Apply(target, true);
				result = string.Format("Opened %1 selected door(s).", changed);
				return changed > 0;
			case EDCO_GMWorldControlAction.CLOSE_DOORS:
				changed = DCO_GMWorldDoorController.Apply(target, false);
				result = string.Format("Closed %1 selected door(s).", changed);
				return changed > 0;
			case EDCO_GMWorldControlAction.LIGHTS_ON:
				changed = DCO_GMWorldLightController.Apply(target, true);
				result = string.Format("Enabled %1 selected light target(s).", changed);
				return changed > 0;
			case EDCO_GMWorldControlAction.LIGHTS_OFF:
				changed = DCO_GMWorldLightController.Apply(target, false);
				result = string.Format("Disabled %1 selected light target(s).", changed);
				return changed > 0;
			case EDCO_GMWorldControlAction.SURRENDER:
				changed = DCO_GMSurrenderService.Apply(target, true);
				result = string.Format("Surrendered %1 selected AI unit(s).", changed);
				return changed > 0;
			case EDCO_GMWorldControlAction.RESTORE:
				changed = DCO_GMSurrenderService.Apply(target, false);
				result = string.Format("Restored %1 selected AI unit(s).", changed);
				return changed > 0;
			case EDCO_GMWorldControlAction.RELEASE_GARRISON:
			{
				SCR_AIGroup groupToRelease = SCR_AIGroup.Cast(target);
				SCR_AIGroupUtilityComponent releaseUtility;
				if (groupToRelease)
					releaseUtility = groupToRelease.GetGroupUtilityComponent();
				if (!releaseUtility || !releaseUtility.DCO_IsGMGarrisoned())
				{
					result = "Selected group is not garrisoned.";
					return false;
				}
				releaseUtility.DCO_ClearGMGarrison();
				result = "Selected garrison released.";
				return true;
			}
			case EDCO_GMWorldControlAction.GARRISON:
			{
				SCR_EditableEntityComponent buildingEditable;
				int buildingCategories;
				if (!DCO_GMBatchAuthority.Resolve(auxiliaryId, EDCO_GMBatchTargetCategory.ENTITY,
					buildingEditable, buildingCategories) || !DCO_GMGarrisonService.IsBuilding(buildingEditable.GetOwner()))
				{
					result = "Garrison failed: select an editable building with the group.";
					return false;
				}
				return DCO_GMGarrisonService.Begin(SCR_AIGroup.Cast(target), buildingEditable.GetOwner(), requester, result);
			}
		}
		result = "Unknown world control.";
		return false;
	}
}

class DCO_GMWorldControlClient
{
	static const int MENU_GARRISON = 460;
	static const int MENU_RELEASE_GARRISON = 461;

	protected static void GetSelection(SCR_EditableEntityComponent fallback,
		out notnull set<SCR_EditableEntityComponent> selected)
	{
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
		if (selected.IsEmpty() && fallback)
			selected.Insert(fallback);
	}

	static SCR_EditableEntityComponent FindSelectedBuilding()
	{
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		GetSelection(null, selected);
		foreach (SCR_EditableEntityComponent editable : selected)
		{
			if (editable && DCO_GMGarrisonService.IsBuilding(editable.GetOwner()))
				return editable;
		}
		return null;
	}

	static bool HasDoorTarget(SCR_EditableEntityComponent fallback)
	{
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		GetSelection(fallback, selected);
		array<DoorComponent> doors = {};
		foreach (SCR_EditableEntityComponent editable : selected)
		{
			if (editable && DCO_GMWorldDoorController.FindDoors(editable.GetOwner(), doors) > 0)
				return true;
		}
		return false;
	}

	static bool HasLightTarget(SCR_EditableEntityComponent fallback)
	{
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		GetSelection(fallback, selected);
		foreach (SCR_EditableEntityComponent editable : selected)
		{
			if (!editable)
				continue;
			IEntity target = editable.GetOwner();
			SCR_AIGroup group = SCR_AIGroup.Cast(target);
			if (group)
			{
				array<AIAgent> agents = {};
				group.GetAgents(agents);
				foreach (AIAgent agent : agents)
				{
					if (agent && HasCharacterFlashlight(agent.GetControlledEntity()))
						return true;
				}
			}
			else if (ChimeraCharacter.Cast(target) && HasCharacterFlashlight(target))
				return true;
			else if (Vehicle.Cast(target) && target.FindComponent(BaseLightManagerComponent))
				return true;
		}
		return false;
	}

	protected static bool HasCharacterFlashlight(IEntity character)
	{
		if (!character || DCO_PlayerUtil.IsPlayer(character))
			return false;
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(character);
		if (!manager)
			return false;
		array<SCR_GadgetComponent> gadgets = manager.GetGadgetsByType(EGadgetType.FLASHLIGHT);
		return gadgets && !gadgets.IsEmpty();
	}

	static bool HasSurrenderTarget(SCR_EditableEntityComponent fallback, bool surrendered)
	{
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		GetSelection(fallback, selected);
		foreach (SCR_EditableEntityComponent editable : selected)
		{
			if (!editable)
				continue;
			IEntity target = editable.GetOwner();
			SCR_AIGroup group = SCR_AIGroup.Cast(target);
			if (group)
			{
				array<AIAgent> agents = {};
				group.GetAgents(agents);
				foreach (AIAgent agent : agents)
				{
					IEntity character;
					if (agent)
						character = agent.GetControlledEntity();
					if (character && !DCO_PlayerUtil.IsPlayer(character)
						&& DCO_GMSurrenderService.IsSurrendered(character) == surrendered)
						return true;
				}
			}
			else if (ChimeraCharacter.Cast(target) && !DCO_PlayerUtil.IsPlayer(target)
				&& DCO_GMSurrenderService.IsSurrendered(target) == surrendered)
				return true;
		}
		return false;
	}

	static void RouteSelected(int action, SCR_EditableEntityComponent fallback)
	{
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		GetSelection(fallback, selected);
		int allowed = EDCO_GMBatchTargetCategory.ENTITY;
		if (action == EDCO_GMWorldControlAction.LIGHTS_ON || action == EDCO_GMWorldControlAction.LIGHTS_OFF)
			allowed = EDCO_GMBatchTargetCategory.CHARACTER | EDCO_GMBatchTargetCategory.GROUP | EDCO_GMBatchTargetCategory.VEHICLE;
		else if (action == EDCO_GMWorldControlAction.SURRENDER || action == EDCO_GMWorldControlAction.RESTORE)
			allowed = EDCO_GMBatchTargetCategory.CHARACTER | EDCO_GMBatchTargetCategory.GROUP;

		DCO_GMBatchTargetSet targets = DCO_GMBatchTargets.Normalize(selected, allowed);
		for (int i = 0; i < targets.GetTargetCount(); i++)
		{
			DCO_GMBatchTarget target = targets.GetTarget(i);
			if (target)
				Route(action, target.m_RplId, RplId.Invalid());
		}
	}

	static void Garrison(notnull array<SCR_EditableEntityComponent> groups, SCR_EditableEntityComponent building)
	{
		if (!building)
			return;
		RplId buildingId;
		if (!building.IsReplicated(buildingId) || !buildingId.IsValid())
			return;
		foreach (SCR_EditableEntityComponent group : groups)
		{
			RplId groupId;
			if (group && group.IsReplicated(groupId) && groupId.IsValid())
				Route(EDCO_GMWorldControlAction.GARRISON, groupId, buildingId);
		}
	}

	static void ReleaseGarrison(notnull array<SCR_EditableEntityComponent> groups)
	{
		foreach (SCR_EditableEntityComponent group : groups)
		{
			RplId groupId;
			if (group && group.IsReplicated(groupId) && groupId.IsValid())
				Route(EDCO_GMWorldControlAction.RELEASE_GARRISON, groupId, RplId.Invalid());
		}
	}

	protected static void Route(int action, RplId targetId, RplId auxiliaryId)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;
		if (Replication.IsServer())
		{
			if (!DCO_GMRights.Allow(controller.GetPlayerId(), "GM world control"))
				return;
			string result;
			bool success = DCO_GMWorldControlServer.Apply(action, targetId, auxiliaryId, controller, result);
			OnResult(success, result);
			return;
		}
		controller.DCO_SendGMWorldControl(action, targetId, auxiliaryId);
	}

	static void OnResult(bool success, string result)
	{
		LogLevel level = LogLevel.WARNING;
		if (success)
			level = LogLevel.NORMAL;
		Print("[DCO-GM] " + result, level);
	}
}

modded class SCR_PlayerController
{
	void DCO_SendGMWorldControl(int action, RplId targetId, RplId auxiliaryId)
	{
		Rpc(DCO_RpcGMWorldControl, action, targetId, auxiliaryId);
	}

	void DCO_SendGMWorldControlResult(bool success, string result)
	{
		if (Replication.IsServer())
			Rpc(DCO_RpcGMWorldControlResult, success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMWorldControl(int action, RplId targetId, RplId auxiliaryId)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM world control"))
		{
			DCO_SendGMWorldControlResult(false, "World control refused: Game Master rights required.");
			return;
		}
		string result;
		bool success = DCO_GMWorldControlServer.Apply(action, targetId, auxiliaryId, this, result);
		DCO_SendGMWorldControlResult(success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMWorldControlResult(bool success, string result)
	{
		DCO_GMWorldControlClient.OnResult(success, result);
	}
}
