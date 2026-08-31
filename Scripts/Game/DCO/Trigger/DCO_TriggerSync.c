enum EDCO_TriggerSequenceAction
{
	NONE,
	MOVE,
	DEFEND,
}

class DCO_TriggerSequenceStep
{
	int m_eAction;
	vector m_vTarget;
	int m_iTargetChoice;

	void DCO_TriggerSequenceStep()
	{
		m_eAction = EDCO_TriggerSequenceAction.NONE;
		m_iTargetChoice = 0;
	}
}

class DCO_TriggerBinding
{
	int m_iId;
	RplId m_GroupEntityId;
	ResourceName m_rGroupPrefab;
	string m_sGroupLabel;
	vector m_aTransform[4];
	bool m_bSpawnOnTrigger;
	bool m_bStaged;
	bool m_bConsumed;
	bool m_bTriggerHoldActive;
	bool m_bManualHoldCaptured;
	bool m_bManualHoldBeforeTrigger;
	ref array<ref DCO_TriggerSequenceStep> m_aSteps = {};

	void DCO_TriggerBinding()
	{
		for (int i = 0; i < 3; i++)
			m_aSteps.Insert(new DCO_TriggerSequenceStep());
	}

	DCO_TriggerSequenceStep GetStep(int index)
	{
		if (!m_aSteps.IsIndexValid(index))
			return null;
		return m_aSteps[index];
	}
}

class DCO_TriggerBindingLocator
{
	RplId m_GroupEntityId;
	DCO_TriggerComponent m_Trigger;
	DCO_TriggerBinding m_Binding;
}

// Server-side lookup used by the native per-group Properties attributes.
class DCO_TriggerSyncRegistry
{
	protected static ref array<ref DCO_TriggerBindingLocator> s_aBindings;

	protected static void Ensure()
	{
		if (!s_aBindings)
			s_aBindings = {};
	}

	static void Register(RplId groupId, DCO_TriggerComponent trigger, DCO_TriggerBinding binding)
	{
		Ensure();
		UnregisterGroup(groupId);
		DCO_TriggerBindingLocator locator = new DCO_TriggerBindingLocator();
		locator.m_GroupEntityId = groupId;
		locator.m_Trigger = trigger;
		locator.m_Binding = binding;
		s_aBindings.Insert(locator);
	}

	static bool Find(RplId groupId, out DCO_TriggerComponent trigger, out DCO_TriggerBinding binding)
	{
		if (!s_aBindings)
			return false;
		foreach (DCO_TriggerBindingLocator locator : s_aBindings)
		{
			if (!locator || locator.m_GroupEntityId != groupId)
				continue;
			trigger = locator.m_Trigger;
			binding = locator.m_Binding;
			return trigger != null && binding != null;
		}
		return false;
	}

	static void UnregisterGroup(RplId groupId)
	{
		if (!s_aBindings)
			return;
		for (int i = s_aBindings.Count() - 1; i >= 0; i--)
		{
			DCO_TriggerBindingLocator locator = s_aBindings[i];
			if (!locator || locator.m_GroupEntityId == groupId)
				s_aBindings.Remove(i);
		}
	}

	static void UnregisterTrigger(DCO_TriggerComponent trigger)
	{
		if (!s_aBindings)
			return;
		for (int i = s_aBindings.Count() - 1; i >= 0; i--)
		{
			DCO_TriggerBindingLocator locator = s_aBindings[i];
			if (!locator || locator.m_Trigger == trigger)
				s_aBindings.Remove(i);
		}
	}
}

class DCO_TriggerSyncServer
{
	static bool Apply(RplId groupId, RplId triggerId, out string result)
	{
		if (!Replication.IsServer())
		{
			result = "Sync failed: server authority is unavailable.";
			return false;
		}
		RplComponent groupRpl = RplComponent.Cast(Replication.FindItem(groupId));
		RplComponent triggerRpl = RplComponent.Cast(Replication.FindItem(triggerId));
		if (!groupRpl || !triggerRpl)
		{
			result = "Sync failed: group or trigger is no longer available.";
			return false;
		}

		IEntity groupEntity = groupRpl.GetEntity();
		IEntity triggerEntity = triggerRpl.GetEntity();
		SCR_EditableGroupComponent groupEditable;
		DCO_TriggerComponent trigger;
		if (groupEntity)
			groupEditable = SCR_EditableGroupComponent.Cast(groupEntity.FindComponent(SCR_EditableGroupComponent));
		if (triggerEntity)
			trigger = DCO_TriggerComponent.Cast(triggerEntity.FindComponent(DCO_TriggerComponent));
		if (!groupEditable || !trigger)
		{
			result = "Sync failed: drag an AI group onto a Bifrost trigger.";
			return false;
		}

		return trigger.DCO_AddSyncedGroup(groupEditable, groupId, result);
	}
}

// Target choices stay deliberately small: the two locations already involved in
// the sync plus any task zones the GM explicitly placed as objectives.
class DCO_TriggerObjectiveCatalog
{
	protected static array<DCO_TaskZoneComponent> SortedZones()
	{
		array<DCO_TaskZoneComponent> result = {};
		array<string> keys = {};
		foreach (DCO_TaskZoneComponent zone : DCO_TaskZoneRegistry.GetZones())
		{
			if (!zone || !zone.GetOwner())
				continue;
			RplComponent rpl = RplComponent.Cast(zone.GetOwner().FindComponent(RplComponent));
			if (!rpl || !rpl.Id().IsValid())
				continue;
			string key = rpl.Id().ToString();
			int insertAt = keys.Count();
			for (int i = 0; i < keys.Count(); i++)
			{
				if (key.Compare(keys[i]) < 0)
				{
					insertAt = i;
					break;
				}
			}
			keys.InsertAt(key, insertAt);
			result.InsertAt(zone, insertAt);
		}
		return result;
	}

	static int Count()
	{
		return 3 + SortedZones().Count();
	}

	static string LabelAt(int index)
	{
		if (index == 0)
			return "Trigger position";
		if (index == 1)
			return "Group staging position";
		if (index == 2)
			return "Recorded position";

		array<DCO_TaskZoneComponent> zones = SortedZones();
		int zoneIndex = index - 3;
		if (!zones.IsIndexValid(zoneIndex) || !zones[zoneIndex])
			return "Unavailable objective";
		DCO_TaskZoneComponent zone = zones[zoneIndex];
		string role = "Objective";
		switch (zone.DCO_GetRole())
		{
			case EDCO_ZoneRole.QRF: role = "QRF Stage"; break;
			case EDCO_ZoneRole.DEFEND: role = "Defend Area"; break;
			case EDCO_ZoneRole.AMBUSH: role = "Ambush Position"; break;
			case EDCO_ZoneRole.AMBUSH_TRIGGER: role = "Kill-Zone"; break;
			case EDCO_ZoneRole.CLEAR: role = "Clear Area"; break;
			case EDCO_ZoneRole.REINFORCE: role = "Reinforce Stage"; break;
		}
		if (zone.DCO_GetPairId() > 0)
			return string.Format("%1 · Pair %2", role, zone.DCO_GetPairId());
		return role;
	}

	static bool Resolve(int index, DCO_TriggerComponent trigger, DCO_TriggerBinding binding, int stepIndex, out vector target)
	{
		if (!trigger || !binding)
			return false;
		if (index == 0)
		{
			target = trigger.DCO_GetCenter();
			return true;
		}
		if (index == 1)
		{
			target = binding.m_aTransform[3];
			return true;
		}
		if (index == 2)
		{
			DCO_TriggerSequenceStep step = binding.GetStep(stepIndex);
			if (!step)
				return false;
			target = step.m_vTarget;
			return target != vector.Zero;
		}

		array<DCO_TaskZoneComponent> zones = SortedZones();
		int zoneIndex = index - 3;
		if (!zones.IsIndexValid(zoneIndex) || !zones[zoneIndex])
			return false;
		target = zones[zoneIndex].DCO_GetCenter();
		return true;
	}
}

class DCO_TriggerSequenceRunner
{
	protected SCR_AIGroup m_Group;
	protected ref array<ref DCO_TriggerSequenceStep> m_aSteps = {};
	protected int m_iStep = -1;
	protected bool m_bIssued;
	protected float m_fStepStarted;

	void Init(SCR_AIGroup group, array<ref DCO_TriggerSequenceStep> source)
	{
		m_Group = group;
		foreach (DCO_TriggerSequenceStep sourceStep : source)
		{
			if (!sourceStep)
				continue;
			DCO_TriggerSequenceStep copy = new DCO_TriggerSequenceStep();
			copy.m_eAction = sourceStep.m_eAction;
			copy.m_vTarget = sourceStep.m_vTarget;
			copy.m_iTargetChoice = sourceStep.m_iTargetChoice;
			m_aSteps.Insert(copy);
		}
		GetGame().GetCallqueue().CallLater(Advance, 500, false);
	}

	protected void Advance()
	{
		m_iStep++;
		while (m_aSteps.IsIndexValid(m_iStep) && m_aSteps[m_iStep].m_eAction == EDCO_TriggerSequenceAction.NONE)
			m_iStep++;
		if (!m_aSteps.IsIndexValid(m_iStep))
		{
			Stop();
			return;
		}
		m_bIssued = false;
		BaseWorld world = GetGame().GetWorld();
		m_fStepStarted = 0;
		if (world)
			m_fStepStarted = world.GetWorldTime();
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		Tick();
	}

	protected void Tick()
	{
		if (!Replication.IsServer() || !m_Group)
		{
			Stop();
			return;
		}
		DCO_TriggerSequenceStep step = m_aSteps[m_iStep];
		IEntity leader = m_Group.GetLeaderEntity();
		if (!leader)
		{
			Stop();
			return;
		}

		if (!m_bIssued)
		{
			AICommunicationComponent comms = AICommunicationComponent.Cast(m_Group.FindComponent(AICommunicationComponent));
			if (!comms)
			{
				Print("[DCO-TRIGGER] queued action failed: spawned group has no AI communication component", LogLevel.WARNING);
				Stop();
				return;
			}
			DCO_VehicleUtil.OrderGroupMoveToPosition(m_Group, step.m_vTarget, comms);
			m_bIssued = true;
		}

		float dx = leader.GetOrigin()[0] - step.m_vTarget[0];
		float dz = leader.GetOrigin()[2] - step.m_vTarget[2];
		if ((dx * dx + dz * dz) <= 144.0)
		{
			GetGame().GetCallqueue().Remove(Tick);
			if (step.m_eAction == EDCO_TriggerSequenceAction.DEFEND)
			{
				SCR_AIGroupUtilityComponent util = m_Group.GetGroupUtilityComponent();
				if (util)
				{
					util.DCO_SetManualHold(true);
					util.DCO_SetDefender(true);
				}
				Stop();
				return;
			}
			Advance();
			return;
		}

		BaseWorld world = GetGame().GetWorld();
		if (world && world.GetWorldTime() - m_fStepStarted > 600000)
		{
			Print(string.Format("[DCO-TRIGGER] queued action %1 timed out after 10 minutes", m_iStep + 1), LogLevel.WARNING);
			GetGame().GetCallqueue().Remove(Tick);
			Advance();
		}
	}

	void Stop()
	{
		GetGame().GetCallqueue().Remove(Tick);
		GetGame().GetCallqueue().Remove(Advance);
		DCO_TriggerSequenceService.Remove(this);
	}
}

class DCO_TriggerSequenceService
{
	protected static ref array<ref DCO_TriggerSequenceRunner> s_aRunners;

	static void Start(SCR_AIGroup group, array<ref DCO_TriggerSequenceStep> steps)
	{
		if (!Replication.IsServer() || !group || !steps)
			return;
		if (!s_aRunners)
			s_aRunners = {};
		DCO_TriggerSequenceRunner runner = new DCO_TriggerSequenceRunner();
		s_aRunners.Insert(runner);
		runner.Init(group, steps);
	}

	static void Remove(DCO_TriggerSequenceRunner runner)
	{
		if (!s_aRunners)
			return;
		int index = s_aRunners.Find(runner);
		if (index >= 0)
			s_aRunners.Remove(index);
	}
}

// Client interaction state. The stock selection component delegates only the
// Ctrl-drag gesture that begins on a group; all other selection paths stay native.
class DCO_TriggerSyncDrag
{
	protected static ref DCO_TriggerSyncDrag s_Instance;
	protected DCO_GMRenderManager m_Render;
	protected SCR_EditableGroupComponent m_Group;
	protected vector m_vStartScreen;
	protected bool m_bDragging;

	static DCO_TriggerSyncDrag Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_TriggerSyncDrag();
		return s_Instance;
	}

	void Start(DCO_GMRenderManager render)
	{
		Stop();
		m_Render = render;
		if (m_Render)
			m_Render.GetOnRender().Insert(OnRender);
	}

	void Stop()
	{
		Cancel();
		if (m_Render)
			m_Render.GetOnRender().Remove(OnRender);
		m_Render = null;
	}

	bool BeginFromFocused(SCR_BaseEditableEntityFilter focusedFilter)
	{
		if (!DCO_GMUIController.IsActive() || DCO_GMUIController.IsNativePropertiesOpen() || DCO_GMGizmo.IsPreciseModeActive() || !focusedFilter)
			return false;
		set<SCR_EditableEntityComponent> focused = new set<SCR_EditableEntityComponent>();
		focusedFilter.GetEntities(focused);
		if (focused.Count() != 1)
			return false;
		SCR_EditableGroupComponent group = SCR_EditableGroupComponent.Cast(focused[0]);
		if (!group || !group.GetOwner())
			return false;
		m_Group = group;
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		m_vStartScreen = Vector(mx, my, 0);
		m_bDragging = false;
		return true;
	}

	bool Update()
	{
		if (!m_Group)
			return false;
		if (DCO_GMUIController.IsNativePropertiesOpen())
		{
			Cancel();
			return true;
		}
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		if (vector.Distance(m_vStartScreen, Vector(mx, my, 0)) >= 10.0)
			m_bDragging = true;
		return m_bDragging;
	}

	bool Finish()
	{
		if (!m_Group)
			return false;
		SCR_EditableGroupComponent group = m_Group;
		bool wasDragging = m_bDragging;
		m_Group = null;
		m_bDragging = false;
		if (!wasDragging)
			return false;

		SCR_EditableEntityComponent hovered;
		SCR_ContextActionsEditorComponent context = SCR_ContextActionsEditorComponent.Cast(
			SCR_ContextActionsEditorComponent.GetInstance(SCR_ContextActionsEditorComponent, false));
		if (context)
			hovered = context.GetHoveredEntity();
		if (!hovered || !hovered.GetOwner() || !hovered.GetOwner().FindComponent(DCO_TriggerComponent))
		{
			OnAuthorityResult(false, "Sync cancelled: drop the line on a Bifrost trigger.");
			return true;
		}

		RplComponent groupRpl = RplComponent.Cast(group.GetOwner().FindComponent(RplComponent));
		RplComponent triggerRpl = RplComponent.Cast(hovered.GetOwner().FindComponent(RplComponent));
		if (!groupRpl || !triggerRpl || !groupRpl.Id().IsValid() || !triggerRpl.Id().IsValid())
		{
			OnAuthorityResult(false, "Sync failed: group or trigger is not replicated.");
			return true;
		}

		if (Replication.IsServer())
		{
			SCR_PlayerController localController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (!localController || !DCO_GMRights.Allow(localController.GetPlayerId(), "trigger sync"))
			{
				OnAuthorityResult(false, "Sync refused: Game Master rights required.");
				return true;
			}
			string result;
			bool ok = DCO_TriggerSyncServer.Apply(groupRpl.Id(), triggerRpl.Id(), result);
			OnAuthorityResult(ok, result);
			return true;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (pc)
			pc.DCO_SendTriggerSync(groupRpl.Id(), triggerRpl.Id());
		else
			OnAuthorityResult(false, "Sync failed: no local player controller.");
		return true;
	}

	bool Cancel()
	{
		bool hadGroup;
		if (m_Group)
			hadGroup = true;
		m_Group = null;
		m_bDragging = false;
		return hadGroup;
	}

	void OnAuthorityResult(bool success, string result)
	{
		LogLevel level = LogLevel.WARNING;
		if (success)
			level = LogLevel.NORMAL;
		Print("[DCO-TRIGGER] " + result, level);
	}

	protected void OnRender(DCO_GMRenderManager render)
	{
		if (!m_bDragging || !m_Group || !m_Group.GetOwner() || DCO_GMUIController.IsNativePropertiesOpen())
			return;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		if (!workspace || !world)
			return;
		vector from = workspace.ProjWorldToScreenNative(m_Group.GetOwner().GetOrigin(), world);
		if (from[2] < 0)
			return;
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		render.DrawScreenLine(from, Vector(mx, my, 0), 0xFFFFFFFF, 2.0);
	}
}
