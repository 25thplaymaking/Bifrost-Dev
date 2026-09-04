[ComponentEditorProps(category: "Bifrost/Arsenal", description: "Replicated binding that exposes the Bifrost arsenal on a GM-selected entity.")]
class DCO_ArsenalAccessComponentClass : ScriptComponentClass
{
}

// The helper is deliberately non-physical and is replicated as its own top-level node. A helper
// spawned beneath an already-registered target hierarchy receives a server RplId but does not
// reliably stream to clients. The target id and local anchor are therefore part of the helper's
// JIP snapshot, while the local interaction handler supplies its action through manual collection.
class DCO_ArsenalAccessComponent : ScriptComponent
{
	static const float USE_RANGE = 4.5;
	protected static const float MIN_AIM_RADIUS = 0.65;
	protected static ref array<DCO_ArsenalAccessComponent> s_aInstances;
	protected RplId m_TargetId;
	protected vector m_vLocalAnchor;
	protected vector m_vAccentRgb = "0.851 0.537 0.169";
	protected float m_fPanelOpacity = 1.0;
	protected SCR_EditableEntityCore m_EditableEntityCore;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_aInstances)
			s_aInstances = {};
		if (s_aInstances.Find(this) < 0)
			s_aInstances.Insert(this);
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(AuditTarget, 1000, true);
			m_EditableEntityCore = SCR_EditableEntityCore.Cast(
				SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
			if (m_EditableEntityCore)
				m_EditableEntityCore.Event_OnEntityTransformChangedServer.Insert(OnEditorTransformChanged);
		}
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(AuditTarget);
		if (m_EditableEntityCore)
			m_EditableEntityCore.Event_OnEntityTransformChangedServer.Remove(OnEditorTransformChanged);
		UnregisterInstance();
		super.OnDelete(owner);
	}

	void ~DCO_ArsenalAccessComponent()
	{
		UnregisterInstance();
	}

	protected void UnregisterInstance()
	{
		if (!s_aInstances)
			return;
		int index = s_aInstances.Find(this);
		if (index >= 0)
			s_aInstances.Remove(index);
	}

	void Configure(RplId targetId, vector localAnchor, vector accentRgb, float panelOpacity)
	{
		if (!Replication.IsServer())
			return;
		m_TargetId = targetId;
		m_vLocalAnchor = localAnchor;
		m_vAccentRgb = accentRgb;
		m_fPanelOpacity = panelOpacity;
	}

	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteRplId(m_TargetId);
		writer.WriteVector(m_vLocalAnchor);
		writer.WriteVector(m_vAccentRgb);
		writer.WriteFloat01(m_fPanelOpacity);
		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadRplId(m_TargetId);
		reader.ReadVector(m_vLocalAnchor);
		reader.ReadVector(m_vAccentRgb);
		reader.ReadFloat01(m_fPanelOpacity);
		return true;
	}

	RplId GetTargetId()
	{
		return m_TargetId;
	}

	Color GetAccentColor()
	{
		return Color.FromRGBA(
			Math.Round(Math.Clamp(m_vAccentRgb[0], 0.0, 1.0) * 255),
			Math.Round(Math.Clamp(m_vAccentRgb[1], 0.0, 1.0) * 255),
			Math.Round(Math.Clamp(m_vAccentRgb[2], 0.0, 1.0) * 255), 255);
	}

	float GetPanelOpacity()
	{
		return Math.Clamp(m_fPanelOpacity, DCO_GMTheme.OPACITY_MIN, 1.0);
	}

	IEntity GetTarget()
	{
		if (!m_TargetId.IsValid())
			return null;
		RplComponent targetRpl = RplComponent.Cast(Replication.FindItem(m_TargetId));
		if (!targetRpl)
			return null;
		return targetRpl.GetEntity();
	}

	vector GetAnchorWorld()
	{
		IEntity owner = GetOwner();
		if (!Replication.IsServer() && owner)
			return owner.GetOrigin();	// Existing proxies receive marker movement through the helper's replicated transform.
		IEntity target = GetTarget();
		if (target)
			return target.CoordToParent(m_vLocalAnchor);
		if (owner)
			return owner.GetOrigin();
		return vector.Zero;
	}

	static DCO_ArsenalAccessComponent FindForTarget(RplId targetId)
	{
		if (!targetId.IsValid() || !s_aInstances)
			return null;
		string targetKey = targetId.AsString();
		for (int i = s_aInstances.Count() - 1; i >= 0; --i)
		{
			DCO_ArsenalAccessComponent access = s_aInstances[i];
			if (!access || !access.GetOwner())
			{
				s_aInstances.Remove(i);
				continue;
			}
			if (access.GetTarget() && access.GetTargetId().IsValid() && access.GetTargetId().AsString() == targetKey)
				return access;
		}
		return null;
	}

	static bool IsUsableBy(IEntity helper, IEntity user)
	{
		if (!helper || !user)
			return false;
		DCO_ArsenalAccessComponent access = DCO_ArsenalAccessComponent.Cast(
			helper.FindComponent(DCO_ArsenalAccessComponent));
		if (!access || !access.GetTarget())
			return false;
		return vector.DistanceSq(access.GetAnchorWorld(), user.GetOrigin()) <= USE_RANGE * USE_RANGE;
	}

	// Server-side authorization for non-GM loadout verbs. A player may only edit their own
	// controlled body while it remains beside a replicated Arsenal Access binding.
	static bool CanUseNearby(IEntity user)
	{
		if (!user || !s_aInstances)
			return false;

		foreach (DCO_ArsenalAccessComponent access : s_aInstances)
		{
			if (access && IsUsableBy(access.GetOwner(), user))
				return true;
		}
		return false;
	}

	static DCO_ArsenalAccessComponent FindAimed(IEntity user, float traceRange)
	{
		if (!user || !s_aInstances || s_aInstances.IsEmpty())
			return null;

		CameraManager cameraManager = GetGame().GetCameraManager();
		World world = GetGame().GetWorld();
		if (!cameraManager || !world)
			return null;
		CameraBase camera = cameraManager.CurrentCamera();
		if (!camera)
			return null;

		vector rayStart = camera.GetOrigin();
		vector rayDirection = camera.GetWorldTransformAxis(2);
		if (rayDirection.LengthSq() < 0.001)
			return null;
		rayDirection.Normalize();
		float rayLength = Math.Max(USE_RANGE, traceRange + 0.5);

		TraceParam trace = new TraceParam();
		trace.Start = rayStart;
		trace.End = rayStart + rayDirection * rayLength;
		trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		trace.TargetLayers = EPhysicsLayerDefs.FireGeometry;
		trace.Exclude = user;
		world.TraceMove(trace, null);

		DCO_ArsenalAccessComponent best;
		float bestScore = float.MAX;
		bool bestIsDirectHit;
		foreach (DCO_ArsenalAccessComponent access : s_aInstances)
		{
			if (!access)
				continue;
			IEntity helper = access.GetOwner();
			IEntity target = access.GetTarget();
			if (!helper || !target || !IsUsableBy(helper, user))
				continue;

			vector toAnchor = access.GetAnchorWorld() - rayStart;
			float forward = vector.Dot(toAnchor, rayDirection);
			if (forward <= 0.05 || forward > rayLength)
				continue;
			float lateralSq = Math.Max(0, toAnchor.LengthSq() - forward * forward);
			bool targetHit = trace.TraceEnt && SharesHierarchy(trace.TraceEnt, target);
			float aimRadius = Math.Max(MIN_AIM_RADIUS, forward * 0.12);
			if (lateralSq > aimRadius * aimRadius)
				continue;

			if (!targetHit)
			{
				if (bestIsDirectHit)
					continue;
				TraceParam anchorTrace = new TraceParam();
				anchorTrace.Start = rayStart;
				anchorTrace.End = access.GetAnchorWorld();
				anchorTrace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
				anchorTrace.TargetLayers = EPhysicsLayerDefs.FireGeometry;
				anchorTrace.Exclude = user;
				float anchorFraction = world.TraceMove(anchorTrace, null);
				bool reachesAnchor = anchorFraction >= 0.999;
				bool reachesTarget = anchorTrace.TraceEnt && SharesHierarchy(anchorTrace.TraceEnt, target);
				if (!reachesAnchor && !reachesTarget)
					continue;
			}
			else if (!bestIsDirectHit)
			{
				best = null;
				bestScore = float.MAX;
				bestIsDirectHit = true;
			}

			float score = lateralSq;
			if (score >= bestScore)
				continue;
			bestScore = score;
			best = access;
		}
		return best;
	}

	protected void AuditTarget()
	{
		if (!Replication.IsServer() || !m_TargetId.IsValid())
			return;
		IEntity target = GetTarget();
		IEntity owner = GetOwner();
		if (!target)
		{
			if (owner)
			{
				UnregisterInstance();
				RplComponent.DeleteRplEntity(owner, true);
			}
			return;
		}

		if (owner)
		{
			vector expectedPosition = target.CoordToParent(m_vLocalAnchor);
			if (vector.DistanceSq(owner.GetOrigin(), expectedPosition) > 0.0001)
			{
				vector previousPosition = owner.GetOrigin();
				owner.SetOrigin(expectedPosition);
				RplComponent replication = RplComponent.Cast(owner.FindComponent(RplComponent));
				if (replication)
					replication.ForceNodeMovement(previousPosition);
			}
		}
	}

	protected void OnEditorTransformChanged(SCR_EditableEntityComponent editableEntity, vector previousTransform[4])
	{
		if (!Replication.IsServer() || !editableEntity || editableEntity.GetOwner() != GetOwner())
			return;
		IEntity target = GetTarget();
		IEntity owner = GetOwner();
		if (!target || !owner)
			return;

		m_vLocalAnchor = target.CoordToLocal(owner.GetOrigin());
	}

	protected static bool SharesHierarchy(IEntity first, IEntity second)
	{
		if (!first || !second)
			return false;
		IEntity cursor = first;
		while (cursor)
		{
			if (cursor == second)
				return true;
			cursor = cursor.GetParent();
		}
		cursor = second;
		while (cursor)
		{
			if (cursor == first)
				return true;
			cursor = cursor.GetParent();
		}
		return false;
	}

	static void AppendActionOwners(IEntity root, notnull array<IEntity> outEntities)
	{
		if (!root)
			return;
		if (root.FindComponent(ActionsManagerComponent) && outEntities.Find(root) < 0)
			outEntities.Insert(root);
		IEntity child = root.GetChildren();
		while (child)
		{
			AppendActionOwners(child, outEntities);
			child = child.GetSibling();
		}
	}
}

// Native F action. It opens a proper MenuManager menu and leaves every gear mutation on the
// existing server-authoritative Bifrost arsenal route.
class DCO_OpenArsenalAction : ScriptedUserAction
{
	protected static const int OPEN_DEBOUNCE_MS = 750;
	protected static int s_iLastOpenAt;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!DCO_ArsenalAccessComponent.IsUsableBy(pOwnerEntity, pUserEntity))
			return;
		int now = System.GetTickCount();
		if (s_iLastOpenAt && now - s_iLastOpenAt < OPEN_DEBOUNCE_MS)
			return;
		s_iLastOpenAt = now;
		DCO_ArsenalAccessComponent access = DCO_ArsenalAccessComponent.Cast(
			pOwnerEntity.FindComponent(DCO_ArsenalAccessComponent));
		DCO_ArsenalMenu.OpenForLocalPlayer(pUserEntity, access);
	}

	override bool CanBeShownScript(IEntity user)
	{
		if (DCO_ArsenalMenu.IsOpen())
			return false;
		return DCO_ArsenalAccessComponent.IsUsableBy(GetOwner(), user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Open Arsenal Access";
		return true;
	}
}

// Physical collection cannot discover a component added to an arbitrary prefab at runtime.
// Manual collection is the engine-supported path used by native inspected items and vehicles.
modded class SCR_InteractionHandlerComponent
{
	protected DCO_ArsenalAccessComponent m_DCOArsenalAccess;
	protected DCO_VehicleServiceAccessComponent m_DCOVehicleServiceAccess;

	override protected void HandleOverride(notnull ChimeraCharacter character)
	{
		m_DCOArsenalAccess = null;
		m_DCOVehicleServiceAccess = null;
		super.HandleOverride(character);

		DCO_ArsenalAccessComponent access = DCO_ArsenalAccessComponent.FindAimed(character, GetVisibilityRange());
		if (access)
		{
			IEntity target = access.GetTarget();
			IEntity helper = access.GetOwner();
			if (target && helper)
			{
				helper.SetOrigin(access.GetAnchorWorld());
				DCO_ArsenalAccessComponent.AppendActionOwners(helper, m_aCollectedEntities);
				DCO_ArsenalAccessComponent.AppendActionOwners(helper, m_aCollectedNearbyEntities);
				DCO_ArsenalAccessComponent.AppendActionOwners(target, m_aCollectedEntities);
				DCO_ArsenalAccessComponent.AppendActionOwners(target, m_aCollectedNearbyEntities);
				m_DCOArsenalAccess = access;
			}
		}

		DCO_VehicleServiceAccessComponent vehicleAccess = DCO_VehicleServiceAccessComponent.FindUsable(character);
		if (vehicleAccess)
		{
			DCO_VehicleServiceAccessComponent.AppendActionOwner(vehicleAccess, m_aCollectedEntities);
			DCO_VehicleServiceAccessComponent.AppendActionOwner(vehicleAccess, m_aCollectedNearbyEntities);
			m_DCOVehicleServiceAccess = vehicleAccess;
		}
		if (!m_DCOArsenalAccess && !m_DCOVehicleServiceAccess)
			return;
		// Service is additive to aimed vehicle actions; only Arsenal inspection replaces them.
		SetManualCollectionOverride(m_DCOArsenalAccess != null);
		SetManualNearbyCollectionOverride(true);
	}

	override array<IEntity> GetManualOverrideList(IEntity owner, out vector referencePoint)
	{
		if (m_DCOArsenalAccess && m_DCOArsenalAccess.GetOwner())
		{
			referencePoint = m_DCOArsenalAccess.GetAnchorWorld();
			return m_aCollectedEntities;
		}
		if (!m_DCOVehicleServiceAccess || !m_DCOVehicleServiceAccess.GetOwner())
			return super.GetManualOverrideList(owner, referencePoint);

		referencePoint = m_DCOVehicleServiceAccess.GetAnchorWorld();
		return m_aCollectedEntities;
	}

	override array<IEntity> GetManualNearbyOverrideList(IEntity owner, out vector referencePoint)
	{
		if (m_DCOArsenalAccess && m_DCOArsenalAccess.GetOwner())
		{
			referencePoint = m_DCOArsenalAccess.GetAnchorWorld();
			return m_aCollectedNearbyEntities;
		}
		if (!m_DCOVehicleServiceAccess || !m_DCOVehicleServiceAccess.GetOwner())
			return super.GetManualNearbyOverrideList(owner, referencePoint);

		referencePoint = m_DCOVehicleServiceAccess.GetAnchorWorld();
		return m_aCollectedNearbyEntities;
	}
}

// CREATE arms this tool; the stock GM focus filter supplies the exact clicked entity. Authority
// then spawns one top-level replicated action binding at the selected surface position.
class DCO_ArsenalAccessPlacement
{
	protected static const ResourceName ACCESS_PREFAB = "{B7D1A94F63C84E20}Prefabs/E_DCO_ArsenalAccess.et";
	protected static const float INTERACTION_VERTICAL_LIFT = 0.15;
	protected static ref DCO_ArsenalAccessPlacement s_Instance;
	protected bool m_bTargeting;

	static DCO_ArsenalAccessPlacement Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_ArsenalAccessPlacement();
		return s_Instance;
	}

	void Init()
	{
		Cancel();
	}

	void Shutdown()
	{
		Cancel();
	}

	void BeginTargeting()
	{
		m_bTargeting = true;
		DCO_GMUIController.RefreshArsenalAccessIndicator();
	}

	bool IsTargeting()
	{
		return m_bTargeting;
	}

	bool SelectFromFocused(SCR_BaseEditableEntityFilter focusedFilter, vector cursorWorldPosition, bool hasCursorWorldPosition)
	{
		if (!m_bTargeting)
			return false;
		if (!focusedFilter || DCO_GMUIController.IsNativePropertiesOpen())
		{
			Cancel();
			return true;
		}

		set<SCR_EditableEntityComponent> focused = new set<SCR_EditableEntityComponent>();
		focusedFilter.GetEntities(focused);
		if (focused.Count() != 1)
		{
			OnAuthorityResult(false, "Select one GM object or vehicle for Arsenal Access.");
			return true;
		}

		IEntity target = focused[0].GetOwner();
		if (!target || target.FindComponent(DCO_ArsenalAccessComponent))
		{
			OnAuthorityResult(false, "Select the object receiving Arsenal Access, not its access binding.");
			return true;
		}
		RplComponent targetRpl = RplComponent.Cast(target.FindComponent(RplComponent));
		if (!targetRpl || !targetRpl.Id().IsValid())
		{
			OnAuthorityResult(false, "Arsenal Access requires a replicated GM object or vehicle.");
			return true;
		}

		vector interactionPosition = target.GetOrigin();
		if (hasCursorWorldPosition)
			interactionPosition = cursorWorldPosition;
		DCO_GMTheme theme = DCO_GMTheme.Get();
		vector accentRgb = Vector(theme.m_AccentColor.R(), theme.m_AccentColor.G(), theme.m_AccentColor.B());
		float panelOpacity = theme.m_PanelOpacity;
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
		{
			OnAuthorityResult(false, "Arsenal Access failed: no local player controller.");
			return true;
		}
		if (Replication.IsServer())
		{
			if (!DCO_GMRights.Allow(playerController.GetPlayerId(), "arsenal access placement"))
			{
				OnAuthorityResult(false, "Arsenal Access refused: Game Master rights required.");
				return true;
			}
			string result;
			bool success = ApplyRelayed(targetRpl.Id(), interactionPosition, accentRgb, panelOpacity, result);
			OnAuthorityResult(success, result);
			return true;
		}

		playerController.DCO_SendGMArsenalAccessCreate(targetRpl.Id(), interactionPosition, accentRgb, panelOpacity);
		return true;
	}

	bool Cancel()
	{
		bool wasTargeting = m_bTargeting;
		m_bTargeting = false;
		if (wasTargeting)
			DCO_GMUIController.RefreshArsenalAccessIndicator();
		return wasTargeting;
	}

	void OnAuthorityResult(bool success, string result)
	{
		LogLevel level = LogLevel.WARNING;
		if (success)
			level = LogLevel.NORMAL;
		Print("[DCO-ARS] " + result, level);
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
			popup.PopupMsg(result, duration: 3);
	}

	static bool ApplyRelayed(RplId targetId, vector interactionPosition, vector accentRgb, float panelOpacity, out string result)
	{
		if (!Replication.IsServer())
		{
			result = "Arsenal Access failed: server authority is unavailable.";
			return false;
		}

		RplComponent targetRpl = RplComponent.Cast(Replication.FindItem(targetId));
		IEntity target;
		if (targetRpl)
			target = targetRpl.GetEntity();
		if (!target || !target.FindComponent(SCR_EditableEntityComponent))
		{
			result = "Arsenal Access failed: the selected GM entity is no longer available.";
			return false;
		}
		accentRgb[0] = Math.Clamp(accentRgb[0], 0.0, 1.0);
		accentRgb[1] = Math.Clamp(accentRgb[1], 0.0, 1.0);
		accentRgb[2] = Math.Clamp(accentRgb[2], 0.0, 1.0);
		panelOpacity = Math.Clamp(panelOpacity, DCO_GMTheme.OPACITY_MIN, 1.0);
		// Keep the prompt above the selected surface so clients do not have to aim beneath props.
		interactionPosition[1] = interactionPosition[1] + INTERACTION_VERTICAL_LIFT;
		Resource resource = Resource.Load(ACCESS_PREFAB);
		if (!resource || !resource.IsValid())
		{
			result = "Arsenal Access failed: the Bifrost action binding is unavailable.";
			return false;
		}

		DCO_ArsenalAccessComponent existingComponent = DCO_ArsenalAccessComponent.FindForTarget(targetId);
		IEntity existingAccess;
		if (existingComponent)
			existingAccess = existingComponent.GetOwner();
		vector transform[4];
		target.GetWorldTransform(transform);
		transform[3] = interactionPosition;
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = transform;
		IEntity access = GetGame().SpawnEntityPrefab(resource, target.GetWorld(), spawnParams);
		DCO_ArsenalAccessComponent accessComponent;
		if (access)
			accessComponent = DCO_ArsenalAccessComponent.Cast(access.FindComponent(DCO_ArsenalAccessComponent));
		if (accessComponent)
			accessComponent.Configure(targetId, target.CoordToLocal(interactionPosition), accentRgb, panelOpacity);
		if (!ValidateBinding(access, target))
		{
			if (access)
				DeleteBinding(access);
			result = "Arsenal Access failed: the replicated action binding did not initialize.";
			return false;
		}

		if (existingAccess && existingAccess != access)
			DeleteBinding(existingAccess);
		SCR_EditableEntityComponent targetEditable = SCR_EditableEntityComponent.Cast(
			target.FindComponent(SCR_EditableEntityComponent));
		SCR_EditableEntityComponent accessEditable = SCR_EditableEntityComponent.Cast(
			access.FindComponent(SCR_EditableEntityComponent));
		if (targetEditable && accessEditable)
			accessEditable.SetParentEntity(targetEditable.GetParentEntity());
		SCR_GarbageSystem garbage = SCR_GarbageSystem.GetByEntityWorld(access);
		if (garbage)
			garbage.UpdateBlacklist(access, true);

		if (existingAccess)
			result = "Arsenal Access refreshed. Select another object, or press Escape to finish.";
		else
			result = "Arsenal Access attached. Select another object, or press Escape to finish.";
		return true;
	}

	protected static bool ValidateBinding(IEntity access, IEntity target)
	{
		if (!access || access.GetParent())
			return false;
		DCO_ArsenalAccessComponent accessComponent = DCO_ArsenalAccessComponent.Cast(
			access.FindComponent(DCO_ArsenalAccessComponent));
		if (!accessComponent || accessComponent.GetTarget() != target)
			return false;
		RplComponent replication = RplComponent.Cast(access.FindComponent(RplComponent));
		ActionsManagerComponent actions = ActionsManagerComponent.Cast(access.FindComponent(ActionsManagerComponent));
		if (!replication || !replication.Id().IsValid() || !actions || !actions.IsEnabled())
			return false;
		array<BaseUserAction> actionList = {};
		actions.GetActionsList(actionList);
		foreach (BaseUserAction action : actionList)
		{
			if (action && action.Type() == DCO_OpenArsenalAction)
				return true;
		}
		return false;
	}

	protected static void DeleteBinding(IEntity access)
	{
		if (!access)
			return;
		RplComponent accessRpl = RplComponent.Cast(access.FindComponent(RplComponent));
		if (accessRpl && accessRpl.Id().IsValid())
			RplComponent.DeleteRplEntity(access, true);
		else
			SCR_EntityHelper.DeleteEntityAndChildren(access);
	}
}
