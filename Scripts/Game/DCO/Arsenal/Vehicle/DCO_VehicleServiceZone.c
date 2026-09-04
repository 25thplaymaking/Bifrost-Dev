[ComponentEditorProps(category: "Bifrost/Arsenal", description: "Replicated movable interaction point for a Bifrost vehicle service area.")]
class DCO_VehicleServiceAccessComponentClass : ScriptComponentClass
{
}

class DCO_VehicleServiceAccessComponent : ScriptComponent
{
	static const float USE_RANGE = 4.5;
	protected static const float MAX_VERTICAL_OFFSET = 3.0;
	protected static ref array<DCO_VehicleServiceAccessComponent> s_aInstances;
	protected RplId m_ZoneId;
	protected vector m_vLocalAnchor;
	protected SCR_EditableEntityCore m_EditableEntityCore;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_aInstances)
			s_aInstances = {};
		if (s_aInstances.Find(this) < 0)
			s_aInstances.Insert(this);
		if (!Replication.IsServer())
			return;

		GetGame().GetCallqueue().CallLater(AuditZone, 1000, true);
		m_EditableEntityCore = SCR_EditableEntityCore.Cast(
			SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (m_EditableEntityCore)
			m_EditableEntityCore.Event_OnEntityTransformChangedServer.Insert(OnEditorTransformChanged);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(AuditZone);
		if (m_EditableEntityCore)
			m_EditableEntityCore.Event_OnEntityTransformChangedServer.Remove(OnEditorTransformChanged);
		Unregister();
		super.OnDelete(owner);
	}

	void ~DCO_VehicleServiceAccessComponent()
	{
		Unregister();
	}

	protected void Unregister()
	{
		if (!s_aInstances)
			return;
		int index = s_aInstances.Find(this);
		if (index >= 0)
			s_aInstances.Remove(index);
	}

	void Configure(RplId zoneId, vector localAnchor)
	{
		if (!Replication.IsServer())
			return;
		m_ZoneId = zoneId;
		m_vLocalAnchor = ClampAnchor(localAnchor);
		MoveToAnchor();
	}

	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteRplId(m_ZoneId);
		writer.WriteVector(m_vLocalAnchor);
		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadRplId(m_ZoneId);
		reader.ReadVector(m_vLocalAnchor);
		return true;
	}

	DCO_VehicleServiceZoneComponent GetZone()
	{
		if (!m_ZoneId.IsValid())
			return null;
		RplComponent replication = RplComponent.Cast(Replication.FindItem(m_ZoneId));
		if (!replication || !replication.GetEntity())
			return null;
		return DCO_VehicleServiceZoneComponent.Cast(
			replication.GetEntity().FindComponent(DCO_VehicleServiceZoneComponent));
	}

	RplId GetZoneId()
	{
		return m_ZoneId;
	}

	vector GetAnchorWorld()
	{
		IEntity owner = GetOwner();
		if (!Replication.IsServer() && owner)
			return owner.GetOrigin();
		DCO_VehicleServiceZoneComponent zone = GetZone();
		if (zone && zone.GetOwner())
			return zone.GetOwner().CoordToParent(m_vLocalAnchor);
		if (owner)
			return owner.GetOrigin();
		return vector.Zero;
	}

	bool SetAnchorWorld(vector worldPosition, out bool wasClamped)
	{
		wasClamped = false;
		if (!Replication.IsServer())
			return false;
		DCO_VehicleServiceZoneComponent zone = GetZone();
		IEntity zoneOwner;
		if (zone)
			zoneOwner = zone.GetOwner();
		if (!zoneOwner)
			return false;

		vector requestedLocal = zoneOwner.CoordToLocal(worldPosition);
		vector clampedLocal = ClampAnchor(requestedLocal);
		wasClamped = vector.DistanceSq(requestedLocal, clampedLocal) > 0.0001;
		m_vLocalAnchor = clampedLocal;
		MoveToAnchor();
		return true;
	}

	static DCO_VehicleServiceAccessComponent FindForZone(RplId zoneId)
	{
		if (!zoneId.IsValid() || !s_aInstances)
			return null;
		string key = zoneId.AsString();
		for (int i = s_aInstances.Count() - 1; i >= 0; --i)
		{
			DCO_VehicleServiceAccessComponent access = s_aInstances[i];
			if (!access || !access.GetOwner())
			{
				s_aInstances.Remove(i);
				continue;
			}
			if (access.GetZoneId().IsValid() && access.GetZoneId().AsString() == key)
				return access;
		}
		return null;
	}

	static DCO_VehicleServiceAccessComponent FindUsable(IEntity user)
	{
		if (!user || !s_aInstances || UserIsInVehicle(user))
			return null;

		DCO_VehicleServiceAccessComponent closest;
		float closestDistanceSq = float.MAX;
		for (int i = s_aInstances.Count() - 1; i >= 0; --i)
		{
			DCO_VehicleServiceAccessComponent access = s_aInstances[i];
			if (!access || !access.GetOwner())
			{
				s_aInstances.Remove(i);
				continue;
			}
			if (!IsUsableBy(access.GetOwner(), user))
				continue;
			float distanceSq = vector.DistanceSq(access.GetAnchorWorld(), user.GetOrigin());
			if (distanceSq >= closestDistanceSq)
				continue;
			closestDistanceSq = distanceSq;
			closest = access;
		}
		return closest;
	}

	static bool IsUsableBy(IEntity helper, IEntity user)
	{
		if (!helper || !user || UserIsInVehicle(user))
			return false;
		DCO_VehicleServiceAccessComponent access = DCO_VehicleServiceAccessComponent.Cast(
			helper.FindComponent(DCO_VehicleServiceAccessComponent));
		DCO_VehicleServiceZoneComponent zone;
		if (access)
			zone = access.GetZone();
		if (!access || !zone || !zone.ContainsUser(user))
			return false;
		return vector.DistanceSq(access.GetAnchorWorld(), user.GetOrigin()) <= USE_RANGE * USE_RANGE;
	}

	static bool UserIsInVehicle(IEntity user)
	{
		return user && CompartmentAccessComponent.GetVehicleIn(user) != null;
	}

	static void AppendActionOwner(DCO_VehicleServiceAccessComponent access, notnull array<IEntity> outEntities)
	{
		if (!access)
			return;
		IEntity owner = access.GetOwner();
		if (owner && owner.FindComponent(ActionsManagerComponent) && outEntities.Find(owner) < 0)
			outEntities.Insert(owner);
	}

	protected vector ClampAnchor(vector localAnchor)
	{
		float radius = DCO_VehicleServiceZoneComponent.SERVICE_RADIUS - 0.5;
		float horizontalSq = localAnchor[0] * localAnchor[0] + localAnchor[2] * localAnchor[2];
		if (horizontalSq > radius * radius)
		{
			float scale = radius / Math.Sqrt(horizontalSq);
			localAnchor[0] = localAnchor[0] * scale;
			localAnchor[2] = localAnchor[2] * scale;
		}
		localAnchor[1] = Math.Clamp(localAnchor[1], -1.0, MAX_VERTICAL_OFFSET);
		return localAnchor;
	}

	protected void AuditZone()
	{
		if (!Replication.IsServer() || !m_ZoneId.IsValid())
			return;
		if (!GetZone())
		{
			IEntity owner = GetOwner();
			if (owner)
				RplComponent.DeleteRplEntity(owner, true);
			return;
		}
		MoveToAnchor();
	}

	protected void OnEditorTransformChanged(SCR_EditableEntityComponent editableEntity, vector previousTransform[4])
	{
		if (!Replication.IsServer() || !editableEntity || editableEntity.GetOwner() != GetOwner())
			return;
		DCO_VehicleServiceZoneComponent zone = GetZone();
		IEntity owner = GetOwner();
		if (!zone || !zone.GetOwner() || !owner)
			return;

		m_vLocalAnchor = ClampAnchor(zone.GetOwner().CoordToLocal(owner.GetOrigin()));
		MoveToAnchor();
	}

	protected void MoveToAnchor()
	{
		DCO_VehicleServiceZoneComponent zone = GetZone();
		IEntity owner = GetOwner();
		if (!zone || !zone.GetOwner() || !owner)
			return;

		vector expected = zone.GetOwner().CoordToParent(m_vLocalAnchor);
		if (vector.DistanceSq(owner.GetOrigin(), expected) <= 0.0001)
			return;
		vector previous = owner.GetOrigin();
		owner.SetOrigin(expected);
		RplComponent replication = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (replication)
			replication.ForceNodeMovement(previous);
	}
}

[ComponentEditorProps(category: "Bifrost/Arsenal", description: "Replicated vehicle service area used by the Bifrost vehicle Arsenal.")]
class DCO_VehicleServiceZoneComponentClass : ScriptComponentClass
{
}

class DCO_VehicleServiceZoneComponent : ScriptComponent
{
	protected static const ResourceName ACCESS_PREFAB = "{A4E68C3D90B17F25}Prefabs/E_DCO_VehicleServiceAccess.et";
	static const float SERVICE_RADIUS = 9.0;
	static const float USER_MARGIN = 2.0;
	static const float MAX_SERVICE_SPEED = 1.0;
	static const float MAX_SERVICE_ANGULAR_SPEED = 0.15;

	protected static ref array<DCO_VehicleServiceZoneComponent> s_aInstances;
	protected ref array<IEntity> m_aQueryVehicles = {};

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_aInstances)
			s_aInstances = {};
		if (s_aInstances.Find(this) < 0)
			s_aInstances.Insert(this);
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(EnsureAccessBinding, 250, true);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(EnsureAccessBinding);
		// The access component removes itself after the zone is gone; deleting it here re-enters editor teardown.
		Unregister();
		super.OnDelete(owner);
	}

	void ~DCO_VehicleServiceZoneComponent()
	{
		Unregister();
	}

	protected void Unregister()
	{
		if (!s_aInstances)
			return;
		int index = s_aInstances.Find(this);
		if (index >= 0)
			s_aInstances.Remove(index);
	}

	static DCO_VehicleServiceZoneComponent FindContaining(IEntity user)
	{
		if (!user || !s_aInstances)
			return null;
		DCO_VehicleServiceZoneComponent closest;
		float closestDistance = float.MAX;
		for (int i = s_aInstances.Count() - 1; i >= 0; --i)
		{
			DCO_VehicleServiceZoneComponent zone = s_aInstances[i];
			if (!zone || !zone.GetOwner())
			{
				s_aInstances.Remove(i);
				continue;
			}
			if (!zone.ContainsUser(user))
				continue;
			float distance = vector.Distance(zone.GetOwner().GetOrigin(), user.GetOrigin());
			if (distance >= closestDistance)
				continue;
			closest = zone;
			closestDistance = distance;
		}
		return closest;
	}

	bool ContainsUser(IEntity user)
	{
		IEntity owner = GetOwner();
		if (!owner || !user || owner.GetWorld() != user.GetWorld())
			return false;
		return vector.Distance(owner.GetOrigin(), user.GetOrigin()) <= SERVICE_RADIUS + USER_MARGIN;
	}

	bool ContainsVehicle(IEntity vehicle)
	{
		IEntity owner = GetOwner();
		vehicle = ResolveVehicle(vehicle);
		if (!owner || !vehicle || owner.GetWorld() != vehicle.GetWorld())
			return false;
		return vector.Distance(owner.GetOrigin(), vehicle.GetOrigin()) <= SERVICE_RADIUS;
	}

	bool IsVehicleStationary(IEntity vehicle)
	{
		vehicle = ResolveVehicle(vehicle);
		if (!vehicle)
			return false;
		Physics physics = vehicle.GetPhysics();
		if (!physics)
			return false;
		return physics.GetVelocity().Length() <= MAX_SERVICE_SPEED
			&& physics.GetAngularVelocity().Length() <= MAX_SERVICE_ANGULAR_SPEED;
	}

	RplId GetReplicationId()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return RplId.Invalid();
		RplComponent replication = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!replication)
			return RplId.Invalid();
		return replication.Id();
	}

	DCO_VehicleServiceAccessComponent GetAccess()
	{
		return DCO_VehicleServiceAccessComponent.FindForZone(GetReplicationId());
	}

	vector GetAccessPoint()
	{
		DCO_VehicleServiceAccessComponent access = GetAccess();
		if (access)
			return access.GetAnchorWorld();
		IEntity owner = GetOwner();
		if (owner)
			return owner.GetOrigin();
		return vector.Zero;
	}

	protected void EnsureAccessBinding()
	{
		if (!Replication.IsServer())
			return;
		if (GetAccess())
		{
			GetGame().GetCallqueue().Remove(EnsureAccessBinding);
			return;
		}
		IEntity owner = GetOwner();
		RplId zoneId = GetReplicationId();
		if (!owner || !zoneId.IsValid())
			return;

		Resource resource = Resource.Load(ACCESS_PREFAB);
		if (!resource || !resource.IsValid())
			return;
		vector transform[4];
		owner.GetWorldTransform(transform);
		vector accessPoint = owner.CoordToParent("0 0.15 0");
		transform[3] = accessPoint;
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = transform;
		IEntity helper = GetGame().SpawnEntityPrefab(resource, owner.GetWorld(), spawnParams);
		DCO_VehicleServiceAccessComponent access;
		if (helper)
			access = DCO_VehicleServiceAccessComponent.Cast(
				helper.FindComponent(DCO_VehicleServiceAccessComponent));
		if (!access)
		{
			if (helper)
				SCR_EntityHelper.DeleteEntityAndChildren(helper);
			return;
		}

		access.Configure(zoneId, owner.CoordToLocal(accessPoint));
		SCR_EditableEntityComponent zoneEditable = SCR_EditableEntityComponent.Cast(
			owner.FindComponent(SCR_EditableEntityComponent));
		SCR_EditableEntityComponent accessEditable = SCR_EditableEntityComponent.Cast(
			helper.FindComponent(SCR_EditableEntityComponent));
		if (zoneEditable && accessEditable)
			accessEditable.SetParentEntity(zoneEditable.GetParentEntity());
		SCR_GarbageSystem garbage = SCR_GarbageSystem.GetByEntityWorld(helper);
		if (garbage)
			garbage.UpdateBlacklist(helper, true);
	}

	int GetVehicles(notnull array<IEntity> outVehicles)
	{
		outVehicles.Clear();
		m_aQueryVehicles.Clear();
		IEntity owner = GetOwner();
		if (!owner || !owner.GetWorld())
			return 0;
		owner.GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), SERVICE_RADIUS, CollectVehicle);
		foreach (IEntity vehicle : m_aQueryVehicles)
			outVehicles.Insert(vehicle);
		return outVehicles.Count();
	}

	protected bool CollectVehicle(IEntity entity)
	{
		IEntity vehicle = ResolveVehicle(entity);
		if (!vehicle || !ContainsVehicle(vehicle) || m_aQueryVehicles.Find(vehicle) >= 0)
			return true;
		RplComponent replication = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		if (replication && replication.Id().IsValid())
			m_aQueryVehicles.Insert(vehicle);
		return true;
	}

	static IEntity ResolveVehicle(IEntity entity)
	{
		if (!entity)
			return null;
		IEntity root = entity.GetRootParent();
		if (!root)
			root = entity;
		return BaseVehicle.Cast(root);
	}

}

class DCO_VehicleServiceAccessPlacement
{
	protected static const float INTERACTION_VERTICAL_LIFT = 0.15;
	protected static ref DCO_VehicleServiceAccessPlacement s_Instance;
	protected RplId m_ZoneId;
	protected bool m_bTargeting;

	static DCO_VehicleServiceAccessPlacement Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_VehicleServiceAccessPlacement();
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

	void BeginTargeting(DCO_VehicleServiceZoneComponent zone)
	{
		if (!zone || !zone.GetReplicationId().IsValid())
		{
			OnAuthorityResult(false, "Vehicle Service access cannot be moved: select a replicated service bay.");
			return;
		}
		m_ZoneId = zone.GetReplicationId();
		m_bTargeting = true;
		ShowMessage("Click inside the service circle to place its access point. Escape cancels.");
	}

	bool IsTargeting()
	{
		return m_bTargeting;
	}

	bool SelectAtCursor(vector cursorWorldPosition, bool hasCursorWorldPosition)
	{
		if (!m_bTargeting)
			return false;
		if (!hasCursorWorldPosition)
		{
			OnAuthorityResult(false, "Vehicle Service access needs a valid world position.");
			return true;
		}

		RplId zoneId = m_ZoneId;
		Cancel();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
		{
			OnAuthorityResult(false, "Vehicle Service access failed: no local player controller.");
			return true;
		}

		if (Replication.IsServer())
		{
			if (!DCO_GMRights.Allow(playerController.GetPlayerId(), "vehicle service access placement"))
			{
				OnAuthorityResult(false, "Vehicle Service access refused: Game Master rights required.");
				return true;
			}
			string result;
			bool success = ApplyRelayed(zoneId, cursorWorldPosition, result);
			OnAuthorityResult(success, result);
			return true;
		}

		playerController.DCO_SendGMVehicleServiceAccessMove(zoneId, cursorWorldPosition);
		return true;
	}

	bool Cancel()
	{
		bool wasTargeting = m_bTargeting;
		m_bTargeting = false;
		m_ZoneId = RplId.Invalid();
		return wasTargeting;
	}

	void OnAuthorityResult(bool success, string result)
	{
		LogLevel level = LogLevel.WARNING;
		if (success)
			level = LogLevel.NORMAL;
		Print("[DCO-VEH-SVC] " + result, level);
		ShowMessage(result);
	}

	static bool ApplyRelayed(RplId zoneId, vector worldPosition, out string result)
	{
		if (!Replication.IsServer())
		{
			result = "Vehicle Service access failed: server authority is unavailable.";
			return false;
		}

		RplComponent zoneReplication = RplComponent.Cast(Replication.FindItem(zoneId));
		IEntity zoneOwner;
		if (zoneReplication)
			zoneOwner = zoneReplication.GetEntity();
		DCO_VehicleServiceZoneComponent zone;
		if (zoneOwner)
			zone = DCO_VehicleServiceZoneComponent.Cast(
				zoneOwner.FindComponent(DCO_VehicleServiceZoneComponent));
		if (!zone)
		{
			result = "Vehicle Service access failed: the service bay is no longer available.";
			return false;
		}

		DCO_VehicleServiceAccessComponent access = zone.GetAccess();
		if (!access)
		{
			result = "Vehicle Service access failed: the bay access marker is not ready yet.";
			return false;
		}

		worldPosition[1] = worldPosition[1] + INTERACTION_VERTICAL_LIFT;
		bool wasClamped;
		if (!access.SetAnchorWorld(worldPosition, wasClamped))
		{
			result = "Vehicle Service access failed: the marker could not be moved.";
			return false;
		}
		if (wasClamped)
			result = "Vehicle Service access moved to the nearest valid point inside the bay.";
		else
			result = "Vehicle Service access moved.";
		return true;
	}

	protected static void ShowMessage(string message)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
			popup.PopupMsg(message, duration: 4);
	}
}

class DCO_OpenVehicleServiceAction : ScriptedUserAction
{
	protected static const int OPEN_DEBOUNCE_MS = 750;
	protected static int s_iLastOpenAt;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		DCO_VehicleServiceAccessComponent access = DCO_VehicleServiceAccessComponent.Cast(
			pOwnerEntity.FindComponent(DCO_VehicleServiceAccessComponent));
		DCO_VehicleServiceZoneComponent zone;
		if (access)
			zone = access.GetZone();
		if (!zone || !DCO_VehicleServiceAccessComponent.IsUsableBy(pOwnerEntity, pUserEntity))
			return;
		int now = System.GetTickCount();
		if (s_iLastOpenAt && now - s_iLastOpenAt < OPEN_DEBOUNCE_MS)
			return;
		s_iLastOpenAt = now;
		DCO_VehicleServiceMenu.Open(zone, pUserEntity);
	}

	override bool CanBeShownScript(IEntity user)
	{
		if (DCO_VehicleServiceMenu.IsServiceOpen())
			return false;
		return DCO_VehicleServiceAccessComponent.IsUsableBy(GetOwner(), user);
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
		outName = "Open Vehicle Service";
		return true;
	}
}

class DCO_VehicleServiceServer
{
	static const int REPAIR_DURATION_MS = 25000;
	static const int REFUEL_DURATION_MS = 20000;
	static const int REARM_DURATION_MS = 15000;
	static const int CAPABILITY_REPAIR = 1;
	static const int CAPABILITY_REFUEL = 2;
	static const int CAPABILITY_REARM = 4;
	static const int VERB_REPAIR = 1;
	static const int VERB_REFUEL = 2;
	static const int VERB_REARM = 3;
	static const int VERB_FULL_SERVICE = 4;
	static const int VERB_ADD_CARGO = 5;
	static const int VERB_REMOVE_CARGO = 6;
	static const int MAX_PAYLOAD_LENGTH = 512;

	static bool IsTimedServiceVerb(int verb)
	{
		return verb >= VERB_REPAIR && verb <= VERB_FULL_SERVICE;
	}

	static int GetServiceDurationMs(int verb, int capabilities)
	{
		switch (verb)
		{
			case VERB_REPAIR:
				if (capabilities & CAPABILITY_REPAIR)
					return REPAIR_DURATION_MS;
				break;
			case VERB_REFUEL:
				if (capabilities & CAPABILITY_REFUEL)
					return REFUEL_DURATION_MS;
				break;
			case VERB_REARM:
				if (capabilities & CAPABILITY_REARM)
					return REARM_DURATION_MS;
				break;
			case VERB_FULL_SERVICE:
				int duration;
				if (capabilities & CAPABILITY_REPAIR)
					duration += REPAIR_DURATION_MS;
				if (capabilities & CAPABILITY_REFUEL)
					duration += REFUEL_DURATION_MS;
				if (capabilities & CAPABILITY_REARM)
					duration += REARM_DURATION_MS;
				return duration;
		}
		return 0;
	}

	static int GetServiceCapabilities(IEntity vehicle)
	{
		if (!vehicle)
			return 0;

		int capabilities;
		SCR_DamageManagerComponent damage = GetVehicleDamageManager(vehicle);
		if (damage && damage.GetState() != EDamageState.DESTROYED)
			capabilities |= CAPABILITY_REPAIR;

		array<SCR_FuelManagerComponent> managers = {};
		if (SCR_FuelManagerComponent.GetAllFuelManagers(vehicle, managers) > 0)
			capabilities |= CAPABILITY_REFUEL;

		if (HasSupportedArmament(vehicle))
			capabilities |= CAPABILITY_REARM;
		return capabilities;
	}

	static bool SupportsVerb(int capabilities, int verb)
	{
		switch (verb)
		{
			case VERB_REPAIR: return (capabilities & CAPABILITY_REPAIR) != 0;
			case VERB_REFUEL: return (capabilities & CAPABILITY_REFUEL) != 0;
			case VERB_REARM: return (capabilities & CAPABILITY_REARM) != 0;
			case VERB_FULL_SERVICE: return capabilities != 0;
		}
		return false;
	}

	static int GetServicePhase(int verb, int elapsedMs, int capabilities)
	{
		if (verb != VERB_FULL_SERVICE)
			return verb;
		if (capabilities & CAPABILITY_REPAIR)
		{
			if (elapsedMs < REPAIR_DURATION_MS)
				return VERB_REPAIR;
			elapsedMs -= REPAIR_DURATION_MS;
		}
		if (capabilities & CAPABILITY_REFUEL)
		{
			if (elapsedMs < REFUEL_DURATION_MS)
				return VERB_REFUEL;
			elapsedMs -= REFUEL_DURATION_MS;
		}
		if (capabilities & CAPABILITY_REARM)
			return VERB_REARM;
		return 0;
	}

	static bool CanStart(SCR_PlayerController caller, int verb, RplId zoneId, RplId vehicleId,
		string payload, out int durationMs, out int capabilities, out string result)
	{
		durationMs = 0;
		capabilities = 0;
		if (!IsTimedServiceVerb(verb))
		{
			result = "Vehicle service rejected an invalid timed request.";
			return false;
		}

		DCO_VehicleServiceZoneComponent zone;
		IEntity vehicle;
		if (!ValidateTarget(caller, zoneId, vehicleId, zone, vehicle, result))
			return false;
		capabilities = GetServiceCapabilities(vehicle);
		if (!SupportsVerb(capabilities, verb))
		{
			if (verb == VERB_REARM)
				result = "This vehicle has no supported armament to rearm.";
			else
				result = "This vehicle does not expose that service capability.";
			return false;
		}
		durationMs = GetServiceDurationMs(verb, capabilities);
		if (durationMs <= 0)
		{
			result = "Vehicle service could not calculate a valid service duration.";
			return false;
		}
		return true;
	}

	static bool Apply(SCR_PlayerController caller, int verb, RplId zoneId, RplId vehicleId,
		string payload, int authorizedCapabilities, out string result)
	{
		result = "Vehicle service request failed.";
		if (!Replication.IsServer() || !caller)
			return false;
		if (verb < VERB_REPAIR || verb > VERB_REMOVE_CARGO || payload.Length() > MAX_PAYLOAD_LENGTH)
		{
			result = "Vehicle service rejected an invalid request.";
			return false;
		}

		DCO_VehicleServiceZoneComponent zone;
		IEntity vehicle;
		if (!ValidateTarget(caller, zoneId, vehicleId, zone, vehicle, result))
			return false;
		int capabilities = GetServiceCapabilities(vehicle);
		if (IsTimedServiceVerb(verb) && authorizedCapabilities != 0)
			capabilities &= authorizedCapabilities;
		if (IsTimedServiceVerb(verb) && !SupportsVerb(capabilities, verb))
		{
			result = "The requested service capability is no longer available.";
			return false;
		}

		switch (verb)
		{
			case VERB_REPAIR:
				return Repair(vehicle, result);
			case VERB_REFUEL:
				return Refuel(vehicle, result);
			case VERB_REARM:
				return Rearm(vehicle, result);
			case VERB_FULL_SERVICE:
				return FullService(vehicle, capabilities, result);
			case VERB_ADD_CARGO:
				return AddCargo(vehicle, payload, result);
			case VERB_REMOVE_CARGO:
				return RemoveCargo(vehicle, payload, result);
		}
		return false;
	}

	protected static bool ValidateTarget(SCR_PlayerController caller, RplId zoneId, RplId vehicleId,
		out DCO_VehicleServiceZoneComponent zone, out IEntity vehicle, out string result)
	{
		if (!Replication.IsServer() || !caller)
		{
			result = "Vehicle service is not available on this machine.";
			return false;
		}

		RplComponent zoneReplication = RplComponent.Cast(Replication.FindItem(zoneId));
		RplComponent vehicleReplication = RplComponent.Cast(Replication.FindItem(vehicleId));
		if (!zoneReplication || !vehicleReplication || !zoneReplication.GetEntity() || !vehicleReplication.GetEntity())
		{
			result = "The service zone or vehicle is no longer available.";
			return false;
		}

		zone = DCO_VehicleServiceZoneComponent.Cast(
			zoneReplication.GetEntity().FindComponent(DCO_VehicleServiceZoneComponent));
		vehicle = DCO_VehicleServiceZoneComponent.ResolveVehicle(vehicleReplication.GetEntity());
		IEntity user = caller.GetControlledEntity();
		DCO_VehicleServiceAccessComponent access;
		if (zone)
			access = zone.GetAccess();
		if (!zone || !access || !vehicle || !user
			|| !DCO_VehicleServiceAccessComponent.IsUsableBy(access.GetOwner(), user)
			|| !zone.ContainsVehicle(vehicle))
		{
			result = "Exit the vehicle and remain beside the service access point.";
			return false;
		}
		if (!zone.IsVehicleStationary(vehicle))
		{
			result = "Stop the vehicle before servicing it.";
			return false;
		}

		SCR_DamageManagerComponent damage = GetVehicleDamageManager(vehicle);
		if (damage && damage.GetState() == EDamageState.DESTROYED)
		{
			result = "Destroyed vehicles cannot be restored by the service bay.";
			return false;
		}
		return true;
	}

	protected static bool Repair(IEntity vehicle, out string result)
	{
		SCR_DamageManagerComponent damage = GetVehicleDamageManager(vehicle);
		if (!damage)
		{
			result = "This vehicle has no supported repair system.";
			return false;
		}
		if (damage.GetState() == EDamageState.DESTROYED)
		{
			result = "Destroyed vehicles cannot be restored by the service bay.";
			return false;
		}
		if (damage.CanBeHealed())
			damage.FullHeal();
		result = "Vehicle repair complete.";
		return true;
	}

	protected static bool Refuel(IEntity vehicle, out string result)
	{
		array<SCR_FuelManagerComponent> managers = {};
		if (SCR_FuelManagerComponent.GetAllFuelManagers(vehicle, managers) <= 0)
		{
			result = "This vehicle has no supported fuel system.";
			return false;
		}
		SCR_FuelManagerComponent.SetTotalFuelPercentageOfFuelManagers(managers, 1.0);
		result = "Vehicle refuel complete.";
		return true;
	}

	protected static bool Rearm(IEntity vehicle, out string result)
	{
		int magazines;
		int rockets;
		int supportedSystems;
		RearmMountedWeapons(vehicle, magazines, rockets, supportedSystems);
		if (supportedSystems <= 0)
		{
			result = "This vehicle has no supported ammunition to refill.";
			return false;
		}
		if (magazines + rockets <= 0)
		{
			result = "Vehicle ammunition is already full.";
			return true;
		}
		result = string.Format("Vehicle rearmed: %1 magazines and %2 rockets refilled.", magazines, rockets);
		return true;
	}

	protected static void RearmMountedWeapons(IEntity vehicle, inout int magazines, inout int rockets, inout int supportedSystems)
	{
		array<WeaponSlotComponent> weaponSlots = {};
		GetMountedWeaponSlots(vehicle, weaponSlots);
		array<IEntity> visitedWeapons = {};
		foreach (WeaponSlotComponent weaponSlot : weaponSlots)
		{
			IEntity weapon;
			if (weaponSlot)
				weapon = weaponSlot.GetWeaponEntity();
			if (!weapon || visitedWeapons.Find(weapon) >= 0)
				continue;
			visitedWeapons.Insert(weapon);
			RearmWeaponSlot(weaponSlot, magazines, rockets, supportedSystems);
		}
	}

	protected static bool HasSupportedArmament(IEntity vehicle)
	{
		array<WeaponSlotComponent> weaponSlots = {};
		GetMountedWeaponSlots(vehicle, weaponSlots);
		array<IEntity> visitedWeapons = {};
		foreach (WeaponSlotComponent weaponSlot : weaponSlots)
		{
			IEntity weapon;
			if (weaponSlot)
				weapon = weaponSlot.GetWeaponEntity();
			if (!weapon || visitedWeapons.Find(weapon) >= 0)
				continue;
			visitedWeapons.Insert(weapon);

			array<BaseMuzzleComponent> muzzles = {};
			weaponSlot.GetMuzzlesList(muzzles);
			foreach (BaseMuzzleComponent muzzle : muzzles)
			{
				BaseMagazineComponent magazine = muzzle.GetMagazine();
				if (magazine && magazine.GetMaxAmmoCount() > 0)
					return true;
			}

			array<Managed> rocketComponents = {};
			weapon.FindComponents(SCR_RocketEjectorMuzzleComponent, rocketComponents);
			foreach (Managed managedRocket : rocketComponents)
			{
				SCR_RocketEjectorMuzzleComponent rocketMuzzle = SCR_RocketEjectorMuzzleComponent.Cast(managedRocket);
				if (rocketMuzzle && rocketMuzzle.GetBarrelsCount() > 0
					&& !rocketMuzzle.GetDefaultRocketPrefab().IsEmpty())
					return true;
			}
		}
		return false;
	}

	static int GetMountedWeaponSlots(IEntity vehicle, notnull array<WeaponSlotComponent> outSlots)
	{
		outSlots.Clear();
		if (!vehicle)
			return 0;

		array<IEntity> visitedHolders = {};
		AppendWeaponSlotsHierarchy(vehicle, outSlots, visitedHolders);
		return outSlots.Count();
	}

	protected static void AppendWeaponSlots(IEntity entity, notnull array<WeaponSlotComponent> outSlots)
	{
		array<Managed> components = {};
		entity.FindComponents(WeaponSlotComponent, components);
		foreach (Managed component : components)
		{
			WeaponSlotComponent weaponSlot = WeaponSlotComponent.Cast(component);
			if (weaponSlot && outSlots.Find(weaponSlot) < 0)
				outSlots.Insert(weaponSlot);
		}
	}

	protected static void AppendWeaponSlotsHierarchy(IEntity entity, notnull array<WeaponSlotComponent> outSlots,
		notnull array<IEntity> visitedHolders)
	{
		if (!entity || visitedHolders.Find(entity) >= 0)
			return;
		visitedHolders.Insert(entity);
		AppendWeaponSlots(entity, outSlots);

		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity next = child.GetSibling();
			bool isInventoryItem = child.FindComponent(InventoryItemComponent) != null;
			if (!isInventoryItem || Turret.Cast(child) != null)
				AppendWeaponSlotsHierarchy(child, outSlots, visitedHolders);
			child = next;
		}
	}

	protected static void RearmWeaponSlot(WeaponSlotComponent weaponSlot, inout int magazines, inout int rockets, inout int supportedSystems)
	{
		if (!weaponSlot || !weaponSlot.GetWeaponEntity())
			return;
		array<BaseMuzzleComponent> muzzles = {};
		weaponSlot.GetMuzzlesList(muzzles);
		foreach (BaseMuzzleComponent muzzle : muzzles)
		{
			BaseMagazineComponent magazine = muzzle.GetMagazine();
			if (!magazine || magazine.GetMaxAmmoCount() <= 0)
				continue;
			supportedSystems++;
			if (magazine.GetAmmoCount() >= magazine.GetMaxAmmoCount())
				continue;
			magazine.SetAmmoCount(magazine.GetMaxAmmoCount());
			magazines++;
		}

		RearmRocketMuzzles(weaponSlot.GetWeaponEntity(), rockets, supportedSystems);
	}

	protected static void RearmRocketMuzzles(IEntity weapon, inout int rockets, inout int supportedSystems)
	{
		array<Managed> rocketComponents = {};
		weapon.FindComponents(SCR_RocketEjectorMuzzleComponent, rocketComponents);
		foreach (Managed managedRocket : rocketComponents)
		{
			SCR_RocketEjectorMuzzleComponent rocketMuzzle = SCR_RocketEjectorMuzzleComponent.Cast(managedRocket);
			if (!rocketMuzzle)
				continue;
			ResourceName rocketPrefab = rocketMuzzle.GetDefaultRocketPrefab();
			if (rocketPrefab.IsEmpty())
				continue;
			Resource rocketResource = Resource.Load(rocketPrefab);
			if (!rocketResource.IsValid())
				continue;
			supportedSystems++;
			for (int barrel = 0; barrel < rocketMuzzle.GetBarrelsCount(); barrel++)
			{
				if (!rocketMuzzle.CanReloadBarrel(barrel))
					continue;
				IEntity rocket = GetGame().SpawnEntityPrefab(rocketResource, weapon.GetWorld());
				if (!rocket)
					continue;
				rocketMuzzle.ReloadBarrel(barrel, rocket);
				rockets++;
			}
		}
	}

	protected static bool FullService(IEntity vehicle, int capabilities, out string result)
	{
		string ignored;
		bool repaired;
		bool refueled;
		bool rearmed;
		if (capabilities & CAPABILITY_REPAIR)
			repaired = Repair(vehicle, ignored);
		if (capabilities & CAPABILITY_REFUEL)
			refueled = Refuel(vehicle, ignored);
		if (capabilities & CAPABILITY_REARM)
			rearmed = Rearm(vehicle, ignored);
		if (!repaired && !refueled && !rearmed)
		{
			result = "This vehicle exposes no supported service capability.";
			return false;
		}
		result = string.Format("Full service complete: repair %1, fuel %2, ammunition %3.",
			YesNo(repaired), YesNo(refueled), YesNo(rearmed));
		return true;
	}

	protected static string YesNo(bool value)
	{
		if (value)
			return "done";
		return "not supported";
	}

	protected static SCR_DamageManagerComponent GetVehicleDamageManager(IEntity vehicle)
	{
		if (!vehicle)
			return null;
		return SCR_DamageManagerComponent.Cast(vehicle.FindComponent(SCR_DamageManagerComponent));
	}

	static InventoryStorageManagerComponent GetVehicleInventory(IEntity vehicle)
	{
		if (!vehicle)
			return null;
		SCR_VehicleInventoryStorageManagerComponent vehicleInventory = SCR_VehicleInventoryStorageManagerComponent.Cast(
			vehicle.FindComponent(SCR_VehicleInventoryStorageManagerComponent));
		if (vehicleInventory)
			return vehicleInventory;
		return InventoryStorageManagerComponent.Cast(vehicle.FindComponent(InventoryStorageManagerComponent));
	}

	static bool IsCargoItem(IEntity item)
	{
		if (!item)
			return false;
		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!itemComponent)
			return false;
		InventoryStorageSlot parentSlot = itemComponent.GetParentSlot();
		BaseInventoryStorageComponent storage;
		if (parentSlot)
			storage = parentSlot.GetStorage();
		return storage && storage.GetPurpose() == EStoragePurpose.PURPOSE_DEPOSIT;
	}

	protected static bool AddCargo(IEntity vehicle, ResourceName prefab, out string result)
	{
		ResourceName containerPrefab;
		ResourceName itemPrefab;
		ParseCargoPayload(prefab, containerPrefab, itemPrefab);
		DCO_ArsenalCatalog catalog = DCO_ArsenalCatalog.Get();
		catalog.Build();
		DCO_ArsenalEntry entry = catalog.FindByPrefab(itemPrefab);
		if (!entry)
		{
			result = "The requested cargo item is not in the active Arsenal catalog.";
			return false;
		}
		InventoryStorageManagerComponent inventory = GetCargoTargetInventory(vehicle, containerPrefab);
		if (!inventory)
		{
			result = "This vehicle has no supported cargo storage.";
			return false;
		}
		if (!inventory.CanInsertResource(itemPrefab, EStoragePurpose.PURPOSE_DEPOSIT))
		{
			result = "The selected item does not fit in this vehicle's cargo storage.";
			return false;
		}
		if (!inventory.TrySpawnPrefabToStorage(itemPrefab, purpose: EStoragePurpose.PURPOSE_DEPOSIT))
		{
			result = "The vehicle rejected the cargo insertion.";
			return false;
		}
		result = "Added " + entry.m_sName + " to vehicle cargo.";
		return true;
	}

	protected static bool RemoveCargo(IEntity vehicle, ResourceName prefab, out string result)
	{
		ResourceName containerPrefab;
		ResourceName itemPrefab;
		ParseCargoPayload(prefab, containerPrefab, itemPrefab);
		InventoryStorageManagerComponent inventory = GetCargoTargetInventory(vehicle, containerPrefab);
		if (!inventory)
		{
			result = "This vehicle has no supported cargo storage.";
			return false;
		}
		array<IEntity> items = {};
		inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!IsCargoItem(item) || !item.GetPrefabData() || item.GetPrefabData().GetPrefabName() != itemPrefab)
				continue;
			if (!inventory.TryDeleteItem(item))
			{
				result = "The vehicle rejected the cargo removal.";
				return false;
			}
			DCO_ArsenalEntry entry = DCO_ArsenalCatalog.Get().FindByPrefab(itemPrefab);
			if (entry)
				result = "Removed " + entry.m_sName + " from vehicle cargo.";
			else
				result = "Removed one cargo item from the vehicle.";
			return true;
		}
		result = "That item is no longer present in vehicle cargo.";
		return false;
	}

	protected static void ParseCargoPayload(string payload, out ResourceName containerPrefab, out ResourceName itemPrefab)
	{
		containerPrefab = ResourceName.Empty;
		itemPrefab = payload;
		int separator = payload.IndexOf("^");
		if (separator < 0)
			return;
		containerPrefab = payload.Substring(0, separator);
		itemPrefab = payload.Substring(separator + 1, payload.Length() - separator - 1);
	}

	protected static InventoryStorageManagerComponent GetCargoTargetInventory(IEntity vehicle, ResourceName containerPrefab)
	{
		InventoryStorageManagerComponent vehicleInventory = GetVehicleInventory(vehicle);
		if (!vehicleInventory || containerPrefab.IsEmpty())
			return vehicleInventory;
		array<IEntity> items = {};
		vehicleInventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!IsCargoItem(item) || !item.GetPrefabData()
				|| item.GetPrefabData().GetPrefabName() != containerPrefab)
				continue;
			InventoryStorageManagerComponent nested = InventoryStorageManagerComponent.Cast(
				item.FindComponent(InventoryStorageManagerComponent));
			if (nested)
				return nested;
		}
		return null;
	}
}

modded class SCR_PlayerController
{
	protected int m_iDCO_VehicleServiceRequestId;
	protected int m_iDCO_VehicleServiceGeneration;
	protected int m_iDCO_VehicleServiceVerb;
	protected RplId m_DCO_VehicleServiceZoneId;
	protected RplId m_DCO_VehicleServiceVehicleId;
	protected string m_sDCO_VehicleServicePayload;
	protected int m_iDCO_VehicleServiceCapabilities;

	void DCO_SendVehicleService(int requestId, int verb, RplId zoneId, RplId vehicleId, string payload)
	{
		Rpc(DCO_RpcVehicleService, requestId, verb, zoneId, vehicleId, payload);
	}

	void DCO_CancelVehicleService(int requestId)
	{
		Rpc(DCO_RpcCancelVehicleService, requestId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcVehicleService(int requestId, int verb, RplId zoneId, RplId vehicleId, string payload)
	{
		if (requestId <= 0 || payload.Length() > DCO_VehicleServiceServer.MAX_PAYLOAD_LENGTH)
		{
			Rpc(DCO_RpcVehicleServiceResult, requestId, false, "Vehicle service rejected an invalid request identifier.");
			return;
		}

		if (DCO_VehicleServiceServer.IsTimedServiceVerb(verb))
		{
			string startResult;
			int durationMs;
			int capabilities;
			if (m_iDCO_VehicleServiceRequestId != 0)
			{
				Rpc(DCO_RpcVehicleServiceResult, requestId, false, "Finish or cancel the active vehicle service first.");
				return;
			}
			if (!DCO_VehicleServiceServer.CanStart(this, verb, zoneId, vehicleId, payload,
				durationMs, capabilities, startResult))
			{
				Rpc(DCO_RpcVehicleServiceResult, requestId, false, startResult);
				return;
			}

			m_iDCO_VehicleServiceRequestId = requestId;
			m_iDCO_VehicleServiceVerb = verb;
			m_DCO_VehicleServiceZoneId = zoneId;
			m_DCO_VehicleServiceVehicleId = vehicleId;
			m_sDCO_VehicleServicePayload = payload;
			m_iDCO_VehicleServiceCapabilities = capabilities;
			m_iDCO_VehicleServiceGeneration++;
			Rpc(DCO_RpcVehicleServiceStarted, requestId, verb, durationMs, capabilities);
			GetGame().GetCallqueue().CallLater(DCO_CompleteVehicleService,
				durationMs, false,
				requestId, m_iDCO_VehicleServiceGeneration);
			return;
		}

		string result;
		bool success = DCO_VehicleServiceServer.Apply(this, verb, zoneId, vehicleId, payload, 0, result);
		Rpc(DCO_RpcVehicleServiceResult, requestId, success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcCancelVehicleService(int requestId)
	{
		if (requestId == 0 || requestId != m_iDCO_VehicleServiceRequestId)
			return;
		ClearPendingVehicleService();
	}

	protected void DCO_CompleteVehicleService(int requestId, int generation)
	{
		if (requestId == 0 || requestId != m_iDCO_VehicleServiceRequestId
			|| generation != m_iDCO_VehicleServiceGeneration)
			return;

		int verb = m_iDCO_VehicleServiceVerb;
		RplId zoneId = m_DCO_VehicleServiceZoneId;
		RplId vehicleId = m_DCO_VehicleServiceVehicleId;
		string payload = m_sDCO_VehicleServicePayload;
		int capabilities = m_iDCO_VehicleServiceCapabilities;
		ClearPendingVehicleService();

		string result;
		bool success = DCO_VehicleServiceServer.Apply(this, verb, zoneId, vehicleId,
			payload, capabilities, result);
		Rpc(DCO_RpcVehicleServiceResult, requestId, success, result);
	}

	protected void ClearPendingVehicleService()
	{
		m_iDCO_VehicleServiceRequestId = 0;
		m_iDCO_VehicleServiceVerb = 0;
		m_DCO_VehicleServiceZoneId = RplId.Invalid();
		m_DCO_VehicleServiceVehicleId = RplId.Invalid();
		m_sDCO_VehicleServicePayload = string.Empty;
		m_iDCO_VehicleServiceCapabilities = 0;
		m_iDCO_VehicleServiceGeneration++;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcVehicleServiceStarted(int requestId, int verb, int durationMs, int capabilities)
	{
		DCO_VehicleServiceMenu.OnAuthorityStarted(requestId, verb, durationMs, capabilities);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcVehicleServiceResult(int requestId, bool success, string result)
	{
		DCO_VehicleServiceMenu.OnAuthorityResult(requestId, success, result);
	}
}
