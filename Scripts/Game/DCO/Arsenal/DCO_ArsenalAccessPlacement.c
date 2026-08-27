[ComponentEditorProps(category: "Bifrost/Arsenal", description: "Marks the replicated interaction proxy used by the GM-placeable Arsenal Access system.")]
class DCO_ArsenalAccessComponentClass : ScriptComponentClass
{
}

class DCO_ArsenalAccessComponent : ScriptComponent
{
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!owner)
			return;

		// Keep model geometry available to the interaction collector without rendering or blocking movement.
		owner.ClearFlags(EntityFlags.VISIBLE);
		Physics physics = owner.GetPhysics();
		if (physics)
		{
			physics.SetInteractionLayer(EPhysicsLayerDefs.Interaction);
			physics.SetActive(ActiveState.INACTIVE);
		}
	}
}

class DCO_ArsenalAccessPlacement
{
	protected static const ResourceName ACCESS_PREFAB = "{B7D1A94F63C84E20}Prefabs/E_DCO_ArsenalAccess.et";
	protected static ref DCO_ArsenalAccessPlacement s_Instance;

	protected SCR_PlacingEditorComponent m_Placing;
	protected bool m_bSubscribed;
	protected IEntity m_PendingAccess;
	protected IEntity m_PendingTarget;
	protected int m_iRouteAttempts;

	static DCO_ArsenalAccessPlacement Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_ArsenalAccessPlacement();
		return s_Instance;
	}

	void Init()
	{
		TrySubscribe();
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(TryRoutePending);
		if (m_Placing && m_bSubscribed)
			m_Placing.GetOnPlaceEntity().Remove(OnPlaced);
		m_bSubscribed = false;
		m_Placing = null;
		m_PendingAccess = null;
		m_PendingTarget = null;
		m_iRouteAttempts = 0;
	}

	protected bool TrySubscribe()
	{
		if (!m_Placing)
			m_Placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (!m_Placing)
			return false;
		if (!m_bSubscribed)
		{
			m_Placing.GetOnPlaceEntity().Insert(OnPlaced);
			m_bSubscribed = true;
		}
		return true;
	}

	protected void OnPlaced(int prefabID, SCR_EditableEntityComponent editable)
	{
		if (!editable)
			return;
		IEntity access = editable.GetOwner();
		if (!access || !access.FindComponent(DCO_ArsenalAccessComponent))
			return;

		vector rayOrigin;
		vector rayDirection;
		IEntity target;
		if (CursorRay(rayOrigin, rayDirection))
			target = ResolveEditableRoot(RayPick(access, rayOrigin, rayDirection));

		m_PendingAccess = access;
		m_PendingTarget = target;
		m_iRouteAttempts = 0;
		GetGame().GetCallqueue().Remove(TryRoutePending);
		GetGame().GetCallqueue().CallLater(TryRoutePending, 0, false);
	}

	protected void TryRoutePending()
	{
		if (!m_PendingAccess)
			return;
		if (Replication.IsServer())
		{
			AttachOnAuthority(m_PendingAccess, m_PendingTarget);
			FinishRoute(m_PendingTarget != null);
			return;
		}

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		RplComponent accessRpl = RplComponent.Cast(m_PendingAccess.FindComponent(RplComponent));
		RplComponent targetRpl;
		if (m_PendingTarget)
			targetRpl = RplComponent.Cast(m_PendingTarget.FindComponent(RplComponent));
		bool accessReady = accessRpl && accessRpl.Id().IsValid();
		bool targetReady = !m_PendingTarget || (targetRpl && targetRpl.Id().IsValid());
		if (playerController && accessReady && targetReady)
		{
			RplId targetId = RplId.Invalid();
			if (targetRpl)
				targetId = targetRpl.Id();
			playerController.DCO_SendGMArsenalAccessAttach(accessRpl.Id(), targetId);
			FinishRoute(m_PendingTarget != null);
			return;
		}

		m_iRouteAttempts++;
		if (m_iRouteAttempts < 20)
		{
			GetGame().GetCallqueue().CallLater(TryRoutePending, 100, false);
			return;
		}
		Notify("ARSENAL ATTACH FAILED");
		Print("[DCO-ARS] placed access proxy never received valid replication ids", LogLevel.WARNING);
		m_PendingAccess = null;
		m_PendingTarget = null;
	}

	protected void FinishRoute(bool attached)
	{
		if (attached)
			Notify("ARSENAL ACCESS ATTACHED");
		else
			Notify("PLACE ARSENAL ACCESS ON AN ENTITY");
		m_PendingAccess = null;
		m_PendingTarget = null;
		m_iRouteAttempts = 0;
	}

	static void ApplyRelayed(RplId accessId, RplId targetId)
	{
		RplComponent accessRpl = RplComponent.Cast(Replication.FindItem(accessId));
		if (!accessRpl)
			return;
		IEntity target;
		if (targetId.IsValid())
		{
			RplComponent targetRpl = RplComponent.Cast(Replication.FindItem(targetId));
			if (targetRpl)
				target = targetRpl.GetEntity();
		}
		AttachOnAuthority(accessRpl.GetEntity(), target);
	}

	protected static void AttachOnAuthority(IEntity access, IEntity target)
	{
		if (!access || !access.FindComponent(DCO_ArsenalAccessComponent))
			return;
		if (!target || target == access)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(access);
			return;
		}

		IEntity child = target.GetChildren();
		while (child)
		{
			if (child != access && child.FindComponent(DCO_ArsenalAccessComponent))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(access);
				Print("[DCO-ARS] target already has arsenal access - duplicate placement removed", LogLevel.NORMAL);
				return;
			}
			child = child.GetSibling();
		}

		target.AddChild(access, -1, EAddChildFlags.AUTO_TRANSFORM);
		Physics physics = access.GetPhysics();
		if (physics)
		{
			physics.SetInteractionLayer(EPhysicsLayerDefs.Interaction);
			physics.SetActive(ActiveState.INACTIVE);
		}
		SCR_GarbageSystem garbage = SCR_GarbageSystem.GetByEntityWorld(access);
		if (garbage)
			garbage.UpdateBlacklist(access, true);
		Print("[DCO-ARS] arsenal access attached to GM-placeable target", LogLevel.NORMAL);
	}

	protected bool CursorRay(out vector origin, out vector direction)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		World world = GetGame().GetWorld();
		if (!workspace || !world)
			return false;
		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		vector projectionDirection;
		origin = workspace.ProjScreenToWorld(workspace.DPIUnscale(mouseX), workspace.DPIUnscale(mouseY), projectionDirection, world);
		direction = projectionDirection;
		direction.Normalize();
		return true;
	}

	protected IEntity RayPick(IEntity exclude, vector origin, vector direction)
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		TraceParam trace = new TraceParam();
		trace.Start = origin;
		trace.End = origin + direction * 2000;
		trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		trace.TargetLayers = EPhysicsLayerDefs.FireGeometry;
		trace.Exclude = exclude;
		world.TraceMove(trace, null);
		return trace.TraceEnt;
	}

	protected IEntity ResolveEditableRoot(IEntity hit)
	{
		IEntity candidate = hit;
		while (candidate)
		{
			if (candidate.FindComponent(SCR_EditableEntityComponent) && candidate.FindComponent(RplComponent))
				return candidate;
			candidate = candidate.GetParent();
		}
		return null;
	}

	protected void Notify(string message)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
			popup.PopupMsg(message, duration: 2);
	}
}
