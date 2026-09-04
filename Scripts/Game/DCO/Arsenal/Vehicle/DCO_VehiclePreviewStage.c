[ComponentEditorProps(category: "Bifrost/Arsenal", description: "Authored floor contact, horizontal center, and yaw for the Vehicle Service preview vehicle.")]
class DCO_VehiclePreviewAnchorComponentClass : ScriptComponentClass
{
}

class DCO_VehiclePreviewAnchorComponent : ScriptComponent
{
}

[ComponentEditorProps(category: "Bifrost/Arsenal", description: "Preview-safe decorative workshop vehicle marker.")]
class DCO_VehicleStageVehicleMarkerComponentClass : ScriptComponentClass
{
}

class DCO_VehicleStageVehicleMarkerComponent : ScriptComponent
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Vehicle prefab", "et")]
	protected ResourceName m_VehiclePrefab;

	ResourceName GetVehiclePrefab()
	{
		return m_VehiclePrefab;
	}
}

[EntityEditorProps(category: "Bifrost/Arsenal", description: "Render-only vehicle preview with source mesh materials.", color: "0 0 0 0", dynamicBox: true)]
class DCO_VehicleMeshPreviewEntityClass : SCR_PrefabPreviewEntityClass
{
}

class DCO_VehicleMeshPreviewEntity : SCR_PrefabPreviewEntity
{
	override protected void SetPreviewObject(VObject mesh, ResourceName material)
	{
		if (!mesh)
			return;
		SetObject(mesh, string.Empty);
	}
}

class DCO_VehiclePreviewStage
{
	protected static const ResourceName STAGE_ENVIRONMENT = "{E8B3A9A26F4D1C70}Prefabs/UI/GRSA_VehicleStageEnvironment.et";
	protected static const ResourceName STAGE_PREVIEW_RIG = "{6A46F1C2D9A70001}Prefabs/UI/GRSA_VehicleStagePreviewRig.et";
	protected static const ResourceName VEHICLE_PREVIEW_MATERIAL = "{58F07022C12D0CF5}Assets/Editor/PlacingPreview/Preview.emat";
	protected static const ResourceName DAY_BIRD_1 = "Sounds/Environment/Ambients3D/Samples/Animals/Birds/ForestConiferous/Day/Environment_Birds_CU_Black-CappedChickadee_2.wav";
	protected static const ResourceName DAY_BIRD_2 = "Sounds/Environment/Ambients3D/Samples/Animals/Birds/ForestConiferous/Day/Environment_Birds_CU_CoalTit1_2.wav";
	protected static const ResourceName DAY_BIRD_3 = "Sounds/Environment/Ambients3D/Samples/Animals/Birds/ForestDeciduous/Day/Environment_Birds_MED_CommonNightingale1_3.wav";
	protected static const ResourceName NIGHT_WILDLIFE_1 = "Sounds/Environment/Ambients3D/Samples/Animals/Insect/Conifer/Conifer_Night_SL_Cricket_1.wav";
	protected static const ResourceName NIGHT_WILDLIFE_2 = "Sounds/Environment/Ambients3D/Samples/Animals/Insect/GrassShort/GrassShort_Night_SL_MarshCricket_2.wav";
	protected static const ResourceName NIGHT_WILDLIFE_3 = "Sounds/Environment/Ambients3D/Samples/Animals/Birds/ForestConiferous/Night/Environment_Birds_MED_ScreechOwl1_3.wav";
	protected static const ResourceName SUPPORT_ACP = "{9DD9C6279F4489B4}Sounds/SupportStations/SupportStations_Vehicles.acp";
	protected static const string SOUND_WORKSHOP_REPAIR = "SOUND_VEHICLE_REPAIR_PARTIAL";
	protected static const string SOUND_WORKSHOP_CLUNK = "SOUND_VEHICLE_REPAIR_DONE";
	protected static const vector FALLBACK_PLACEMENT = "3.18 0.023 6.241";
	protected static const float FALLBACK_YAW = 180;
	protected static const float FLOOR_CLEARANCE = 0.22;
	protected static const float CAMERA_BODY_CENTER_MAX_HEIGHT = 1.3;
	protected static const float HOME_PITCH = -4;
	protected static const float FRAME_DISTANCE_SCALE = 0.78;
	protected static const float FRAME_MIN_DISTANCE = 4.65;
	protected static const float FRAME_MAX_DISTANCE = 5.2;
	protected static const float ZOOM_MIN_DISTANCE = 2.2;
	protected static const float ZOOM_MIN_SCALE = 0.36;
	protected static const float ZOOM_MAX_DISTANCE = 5.3;
	protected static const float ZOOM_MAX_SCALE = 0.82;
	protected static const float EXTERIOR_YAW_HALF_RANGE = 72;
	protected static const float TRACK_HORIZONTAL_RANGE = 1.6;
	protected static const float TRACK_VERTICAL_RANGE = 1.15;

	protected ref GRSA_StageCore m_Core;
	protected SCR_BasePreviewEntity m_WorkshopPreview;
	protected SCR_BasePreviewEntity m_Vehicle;
	protected ResourceName m_VehiclePrefab;
	protected RenderTargetWidget m_Target;
	protected vector m_vCenter;
	protected vector m_vPlacement = FALLBACK_PLACEMENT;
	protected float m_fBoundsDiagonal = 4;
	protected float m_fPlacementYaw = FALLBACK_YAW;
	protected ref array<SCR_BasePreviewEntity> m_Decorations = {};
	protected ref array<float> m_DecorationFloorHeights = {};
	protected bool m_bDecorationBoundsPending;
	protected bool m_bEnvironmentReady;
	protected bool m_bEnvironmentPending;
	protected bool m_bBoundsPending;
	protected bool m_bNight;
	protected float m_fAmbienceDelay;
	protected AudioHandle m_AmbienceAudio = AudioHandle.Invalid;
	protected float m_fWorkshopDelay;
	protected float m_fWorkshopBurstRemaining;
	protected AudioHandle m_WorkshopAudio = AudioHandle.Invalid;

	void DCO_VehiclePreviewStage()
	{
		m_bNight = IsScenarioNight();
		m_Core = new GRSA_StageCore();
		m_Core.SetAutomaticHDR(false);
		m_Core.SetCameraEVAdjustment(0);
		m_Core.SetStudioLightingAllowed(false);
		m_Core.SetTranslationTrack(false);
		m_Core.SetEnvironment(STAGE_PREVIEW_RIG);
		m_fAmbienceDelay = Math.RandomFloat(1.5, 4);
		m_fWorkshopDelay = Math.RandomFloat(3.5, 7);
	}

	bool ShowVehicle(ResourceName prefab, RenderTargetWidget target)
	{
		if (prefab.IsEmpty() || !target)
			return false;
		if (!m_Core.EnsureWorld("DCO_VehicleService"))
			return false;
		if (!EnsureWorkshopPreview())
			return false;
		if (!m_bEnvironmentReady)
			m_bEnvironmentPending = true;
		ReadAuthoredPlacement();

		m_Target = target;
		m_Core.BindTarget(target);
		if (m_Vehicle && !m_Vehicle.IsDeleted() && prefab == m_VehiclePrefab)
			return true;

		ClearVehicle();
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return false;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(Vector(m_fPlacementYaw, 0, 0), spawnParams.Transform);
		spawnParams.Transform[3] = m_vPlacement;
		m_Vehicle = SCR_PrefabPreviewEntity.SpawnPreviewFromPrefab(resource,
			"DCO_VehicleMeshPreviewEntity", m_Core.GetWorld(), spawnParams, VEHICLE_PREVIEW_MATERIAL,
			EPreviewEntityFlag.IGNORE_TERRAIN);
		if (!m_Vehicle)
			return false;

		m_VehiclePrefab = prefab;
		m_Vehicle.Update();
		m_bBoundsPending = !ReadPreviewBounds();
		if (!m_bBoundsPending)
			FrameVehicle(true);
		return true;
	}

	void Tick(float timeSlice)
	{
		if (!m_Core)
			return;

		m_Core.Tick(timeSlice);
		if (m_bBoundsPending && ReadPreviewBounds())
		{
			m_bBoundsPending = false;
			FrameVehicle(true);
		}
		if (m_bEnvironmentPending)
		{
			m_bEnvironmentPending = false;
			EnsureEnvironmentDecorations();
			return;
		}
		if (m_bDecorationBoundsPending)
			m_bDecorationBoundsPending = !AlignEnvironmentDecorationsToFloor();
		TickAmbience(timeSlice);
		TickWorkshopAmbience(timeSlice);
	}

	bool ProjectDamagePoint(int hitZoneIndex, out vector screenPos)
	{
		return false;
	}

	void FocusDamagePoint(int hitZoneIndex)
	{
	}

	void GoHome()
	{
		if (!m_Vehicle)
			return;
		m_bBoundsPending = !ReadPreviewBounds();
		if (!m_bBoundsPending)
			FrameVehicle();
	}

	void Clear()
	{
		ClearVehicle();
	}

	protected bool ReadPreviewBounds()
	{
		if (!m_Vehicle)
			return false;

		vector mins, maxs;
		m_Vehicle.GetPreviewBounds(mins, maxs);
		if (!ArePreviewBoundsValid(mins, maxs))
			return false;

		vector vehicleTransform[4];
		m_Vehicle.GetWorldTransform(vehicleTransform);
		vehicleTransform[3] = m_vPlacement;
		vehicleTransform[3][1] = m_vPlacement[1] - mins[1] + FLOOR_CLEARANCE;
		m_Vehicle.SetWorldTransform(vehicleTransform);
		m_Vehicle.Update();

		m_vCenter = vehicleTransform[3];
		float boundsHeight = maxs[1] - mins[1];
		m_vCenter[1] = m_vPlacement[1] + FLOOR_CLEARANCE
			+ Math.Min(boundsHeight * 0.5, CAMERA_BODY_CENTER_MAX_HEIGHT);
		m_fBoundsDiagonal = vector.Distance(mins, maxs);
		if (m_fBoundsDiagonal < 1)
			m_fBoundsDiagonal = 1;
		return true;
	}

	protected bool ArePreviewBoundsValid(vector mins, vector maxs)
	{
		for (int axis = 0; axis < 3; axis++)
		{
			if (mins[axis] > maxs[axis]
				|| Math.AbsFloat(mins[axis]) > 100000
				|| Math.AbsFloat(maxs[axis]) > 100000)
				return false;
		}
		return vector.Distance(mins, maxs) > 0.01;
	}

	protected void FrameVehicle(bool snap = false)
	{
		if (!m_Core || !m_Core.IsAlive())
			return;

		float distance = Math.Clamp(m_fBoundsDiagonal * FRAME_DISTANCE_SCALE, FRAME_MIN_DISTANCE, FRAME_MAX_DISTANCE);
		float minDistance = Math.Max(ZOOM_MIN_DISTANCE, m_fBoundsDiagonal * ZOOM_MIN_SCALE);
		minDistance = Math.Min(minDistance, distance * 0.9);
		float maxDistance = Math.Clamp(m_fBoundsDiagonal * ZOOM_MAX_SCALE, distance, ZOOM_MAX_DISTANCE);
		m_Core.SetSubjectSpin(false);
		m_Core.SetZoomRange(minDistance, maxDistance);
		m_Core.SetPanBounds(m_vCenter, TRACK_HORIZONTAL_RANGE, m_vCenter[1] - 1.0, m_vCenter[1] + TRACK_VERTICAL_RANGE);
		vector homeAngles = Vector(m_fPlacementYaw + 180, HOME_PITCH, 0);
		m_Core.SetYawLimit(homeAngles[0], EXTERIOR_YAW_HALF_RANGE);
		m_Core.SetHome(homeAngles, m_vCenter, distance);
		m_Core.GoHome(snap);
	}

	protected void ReadAuthoredPlacement()
	{
		m_vPlacement = FALLBACK_PLACEMENT;
		m_fPlacementYaw = FALLBACK_YAW;
		if (!m_Core)
			return;

		IEntity anchor = FindAnchor(m_Core.GetEnvironmentRig());
		if (!anchor)
			return;

		m_vPlacement = anchor.GetOrigin();
		vector anchorAngles = anchor.GetYawPitchRoll();
		m_fPlacementYaw = anchorAngles[0];
	}

	protected IEntity FindAnchor(IEntity entity)
	{
		if (!entity)
			return null;
		if (entity.FindComponent(DCO_VehiclePreviewAnchorComponent))
			return entity;

		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity found = FindAnchor(child);
			if (found)
				return found;
			child = child.GetSibling();
		}
		return null;
	}

	protected void EnsureEnvironmentDecorations()
	{
		if (m_bEnvironmentReady || !m_Core || !m_Core.IsAlive())
			return;
		m_bEnvironmentReady = true;
		CollectEnvironmentDecorations(m_Core.GetEnvironmentRig());
		SetArtificialLights(m_Core.GetEnvironmentRig(), true);
	}

	protected bool EnsureWorkshopPreview()
	{
		if (m_WorkshopPreview && !m_WorkshopPreview.IsDeleted())
			return true;

		Resource resource = Resource.Load(STAGE_ENVIRONMENT);
		if (!resource || !resource.IsValid())
			return false;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		m_WorkshopPreview = SCR_PrefabPreviewEntity.SpawnPreviewFromPrefab(resource,
			"DCO_VehicleMeshPreviewEntity", m_Core.GetWorld(), params, VEHICLE_PREVIEW_MATERIAL,
			EPreviewEntityFlag.IGNORE_TERRAIN);
		if (!m_WorkshopPreview)
			return false;

		m_WorkshopPreview.Update();
		return true;
	}

	protected bool IsScenarioNight()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return false;
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		return manager && manager.IsSunSet();
	}

	protected void SetArtificialLights(IEntity entity, bool enabled)
	{
		if (!entity)
			return;
		LightEntity light = LightEntity.Cast(entity);
		if (light)
			light.SetEnabled(enabled);

		IEntity child = entity.GetChildren();
		while (child)
		{
			SetArtificialLights(child, enabled);
			child = child.GetSibling();
		}
	}

	protected void TickAmbience(float timeSlice)
	{
		if (!m_bEnvironmentReady || !m_Core || !m_Core.IsAlive())
			return;

		m_fAmbienceDelay -= timeSlice;
		if (m_fAmbienceDelay > 0)
			return;

		if (m_AmbienceAudio != AudioHandle.Invalid && AudioSystem.IsSoundPlayed(m_AmbienceAudio))
		{
			m_fAmbienceDelay = 1;
			return;
		}

		m_AmbienceAudio = AudioSystem.PlaySound(GetAmbientSample());
		if (m_bNight)
			m_fAmbienceDelay = Math.RandomFloat(9, 17);
		else
			m_fAmbienceDelay = Math.RandomFloat(7, 14);
	}

	protected ResourceName GetAmbientSample()
	{
		int sample = Math.RandomInt(0, 3);
		if (m_bNight)
		{
			if (sample == 0)
				return NIGHT_WILDLIFE_1;
			if (sample == 1)
				return NIGHT_WILDLIFE_2;
			return NIGHT_WILDLIFE_3;
		}

		if (sample == 0)
			return DAY_BIRD_1;
		if (sample == 1)
			return DAY_BIRD_2;
		return DAY_BIRD_3;
	}

	protected void StopAmbience()
	{
		if (m_AmbienceAudio == AudioHandle.Invalid)
			return;
		AudioSystem.TerminateSoundFadeOut(m_AmbienceAudio, true, 0.3);
		m_AmbienceAudio = AudioHandle.Invalid;
	}

	protected void TickWorkshopAmbience(float timeSlice)
	{
		if (!m_bEnvironmentReady || !m_Core || !m_Core.IsAlive())
			return;
		if (m_WorkshopAudio != AudioHandle.Invalid)
		{
			m_fWorkshopBurstRemaining -= timeSlice;
			if (m_fWorkshopBurstRemaining > 0 && AudioSystem.IsSoundPlayed(m_WorkshopAudio))
				return;
			StopWorkshopAmbience();
			m_fWorkshopDelay = Math.RandomFloat(4.5, 9);
			return;
		}

		m_fWorkshopDelay -= timeSlice;
		if (m_fWorkshopDelay > 0)
			return;

		IEntity listener;
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			listener = controller.GetControlledEntity();
		if (!listener)
		{
			m_fWorkshopDelay = 2;
			return;
		}

		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		transform[3] = listener.GetOrigin() + Vector(Math.RandomFloat(9, 14), 0,
			Math.RandomFloat(-9, 9));
		m_WorkshopAudio = AudioSystem.PlayEvent(SUPPORT_ACP, SOUND_WORKSHOP_REPAIR, transform);
		if (m_WorkshopAudio == AudioHandle.Invalid)
			m_WorkshopAudio = AudioSystem.PlayEvent(SUPPORT_ACP, SOUND_WORKSHOP_CLUNK, transform);
		m_fWorkshopBurstRemaining = Math.RandomFloat(0.8, 1.7);
		if (m_WorkshopAudio == AudioHandle.Invalid)
			m_fWorkshopDelay = Math.RandomFloat(4.5, 9);
	}

	protected void StopWorkshopAmbience()
	{
		if (m_WorkshopAudio == AudioHandle.Invalid)
			return;
		AudioSystem.TerminateSoundFadeOut(m_WorkshopAudio, true, 0.25);
		m_WorkshopAudio = AudioHandle.Invalid;
		m_fWorkshopBurstRemaining = 0;
	}

	protected void CollectEnvironmentDecorations(IEntity entity)
	{
		if (!entity)
			return;

		DCO_VehicleStageVehicleMarkerComponent vehicleMarker = DCO_VehicleStageVehicleMarkerComponent.Cast(
			entity.FindComponent(DCO_VehicleStageVehicleMarkerComponent));
		if (vehicleMarker)
			SpawnDecoration(vehicleMarker.GetVehiclePrefab(), entity);

		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity next = child.GetSibling();
			CollectEnvironmentDecorations(child);
			child = next;
		}
	}

	protected void SpawnDecoration(ResourceName prefab, IEntity marker)
	{
		if (prefab.IsEmpty() || !marker)
			return;
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return;
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		marker.GetWorldTransform(params.Transform);
		SCR_BasePreviewEntity decoration = SCR_PrefabPreviewEntity.SpawnPreviewFromPrefab(resource,
			"DCO_VehicleMeshPreviewEntity", m_Core.GetWorld(), params, VEHICLE_PREVIEW_MATERIAL,
			EPreviewEntityFlag.IGNORE_TERRAIN);
		if (!decoration)
			return;
		decoration.Update();
		m_Decorations.Insert(decoration);
		m_DecorationFloorHeights.Insert(params.Transform[3][1]);
		m_bDecorationBoundsPending = !AlignEnvironmentDecorationsToFloor();
	}

	protected bool AlignEnvironmentDecorationsToFloor()
	{
		bool aligned = true;
		for (int i = 0; i < m_Decorations.Count(); i++)
		{
			SCR_BasePreviewEntity decoration = m_Decorations[i];
			if (!decoration || decoration.IsDeleted())
				continue;

			vector mins, maxs;
			decoration.GetPreviewBounds(mins, maxs);
			if (!ArePreviewBoundsValid(mins, maxs))
			{
				aligned = false;
				continue;
			}

			vector transform[4];
			decoration.GetWorldTransform(transform);
			transform[3][1] = m_DecorationFloorHeights[i] - mins[1] + FLOOR_CLEARANCE;
			decoration.SetWorldTransform(transform);
			decoration.Update();
		}
		return aligned;
	}

	protected void ClearEnvironmentDecorations()
	{
		foreach (SCR_BasePreviewEntity decoration : m_Decorations)
		{
			if (decoration)
				SCR_EntityHelper.DeleteEntityAndChildren(decoration);
		}
		m_Decorations.Clear();
		m_DecorationFloorHeights.Clear();
		m_bDecorationBoundsPending = false;
		m_bEnvironmentReady = false;
		m_bEnvironmentPending = false;
	}

	protected void ClearVehicle()
	{
		IEntity vehicle = m_Vehicle;
		m_Vehicle = null;
		m_VehiclePrefab = ResourceName.Empty;
		m_bBoundsPending = false;
		if (vehicle)
			SCR_EntityHelper.DeleteEntityAndChildren(vehicle);
	}

	void Destroy()
	{
		StopAmbience();
		StopWorkshopAmbience();
		if (m_Core)
			m_Core.UnbindTarget();

		// Releasing the private world owns deletion of its complete entity hierarchy.
		m_Vehicle = null;
		m_WorkshopPreview = null;
		m_VehiclePrefab = ResourceName.Empty;
		m_bBoundsPending = false;
		m_Decorations.Clear();
		m_DecorationFloorHeights.Clear();
		m_bDecorationBoundsPending = false;
		m_bEnvironmentReady = false;
		m_bEnvironmentPending = false;
		if (m_Core)
			m_Core.Release();
		m_Core = null;
		m_Target = null;
	}
}
