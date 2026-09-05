//! Native-style mouse boundary for direct manipulation of the active stage render.
class GRSA_StageDragHandler : ScriptedWidgetEventHandler
{
	protected WorkspaceWidget m_Workspace;
	protected RenderTargetWidget m_Target;
	protected GRSA_StageCore m_Core;
	protected bool m_bDragging;

	//------------------------------------------------------------------------------------------------
	void GRSA_StageDragHandler(notnull RenderTargetWidget target, notnull GRSA_StageCore core)
	{
		m_Target = target;
		m_Core = core;
		m_Workspace = GetGame().GetWorkspace();
		if (m_Workspace)
			m_Workspace.AddHandler(this);
		m_Target.AddHandler(this);
	}

	//------------------------------------------------------------------------------------------------
	void SetTarget(notnull RenderTargetWidget target)
	{
		if (target == m_Target)
			return;

		m_bDragging = false;
		m_Core.SetPointerRotate(false);
		if (m_Target)
			m_Target.RemoveHandler(this);
		m_Target = target;
		m_Target.AddHandler(this);
	}

	//------------------------------------------------------------------------------------------------
	protected bool CanStartDrag(Widget source, int x, int y)
	{
		return m_Core && m_Core.CanStartPointerRotate(source, x, y);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 0)
			return false;

		if (!CanStartDrag(w, x, y))
			return false;

		m_bDragging = true;
		m_Core.SetPointerRotate(true);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (button != 0)
			return false;

		if (!m_bDragging)
			return false;

		m_bDragging = false;
		m_Core.SetPointerRotate(false);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!m_Core || !m_Core.CanStartPointerRotate(w, x, y))
			return false;

		m_Core.AddPointerZoom(wheel);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!m_bDragging)
			return false;

		m_bDragging = false;
		m_Core.SetPointerRotate(false);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (m_Core)
			m_Core.SetPointerRotate(false);
		if (m_Target)
			m_Target.RemoveHandler(this);
		if (m_Workspace)
			m_Workspace.RemoveHandler(this);

		m_bDragging = false;
		m_Target = null;
		m_Core = null;
		m_Workspace = null;
	}
}

//! Shared preview-world stage with the GRS environment, one camera, orbit, pan, zoom, and reset.
//! Hosts own the subject and framing target; the core owns camera movement and input.
class GRSA_StageCore
{
	protected static const ResourceName STAGE_ENVIRONMENT = "{AB205AD4E2000007}Prefabs/UI/GRSA_StageEnvironment.et";
	//! GRS-owned continuous motes (bigger box, denser, slower, hotter alpha than the base
	//! three-second one-shot) — the slow drifting studio air across the whole set.
	protected static const ResourceName DUST_EFFECT = "{83D4D0AFC6BC38F3}Particles/GRSA_StageDust.ptc";
	static const int CAMERA = 0;
	protected static const float CAMERA_FAR_PLANE = 50;
	protected static const float CAMERA_NEAR_PLANE = 0.001;
	protected static const float CAMERA_FOV = 38;
	//! Glides close at a speed proportional to the remaining distance (plus a floor so they
	//! finish): big moves respond instantly, landings stay soft.
	protected static const float EASE_FLOOR = 3;
	protected static const float EASE_ANGLE = 0.2;
	protected static const float EASE_MOVE = 8;
	//! Cinematic pacing for station-to-station moves: a home glide whose look-point jump passes
	//! the engage distance slows the ease until arrival, so a tab change reads as a camera pan
	//! across the studio instead of a snap. Close-range glides keep the normal response.
	protected static const float TRAVEL_EASE_SCALE = 0.35;
	protected static const float TRAVEL_ENGAGE_DIST = 0.8;
	protected static const float ARRIVE_ANGLE_EPS = 1;
	protected static const float ARRIVE_LOOK_EPS = 0.03;
	protected static const float ARRIVE_DIST_EPS = 0.03;
	//! Whole-set establishing pose a fresh studio opens on (bench left, soldier spot right); the
	//! first station glide is the menu's opening camera move.
	protected static const vector WIDE_ANGLES = "-120 -12 0";
	protected static const vector WIDE_LOOK = "0.95 0.35 -0.35";
	protected static const float WIDE_DIST = 4.4;
	protected static const float STUDIO_KEY_LV = 3.4;
	protected static const float STUDIO_FILL_LV = 5.2;

	protected static int s_iWorldCounter;

	//! (float yawDeltaDeg) manual drag/stick rotation goes to the SUBJECT, not the camera: the
	//! fixed camera keeps the set composed while the piece turns in place, the gunsmith way.
	ref ScriptInvoker m_OnSubjectSpin = new ScriptInvoker();

	protected ref SharedItemRef m_WorldRef;
	protected ResourceName m_EnvironmentPrefab = STAGE_ENVIRONMENT;
	protected string m_sWorldType;
	protected ref GRSA_StageDragHandler m_DragHandler;
	protected BaseWorld m_World;
	protected IEntity m_EnvironmentRig;
	protected LightEntity m_StudioKey;
	protected LightEntity m_StudioFillLeft;
	protected LightEntity m_StudioFillRight;
	protected bool m_bStudioLightingAllowed = true;
	protected bool m_bAutomaticHDR;
	protected float m_fCameraEVAdjustment;
	protected ParticleEffectEntity m_Dust;
	protected RenderTargetWidget m_wTarget;
	protected InputManager m_InputManager;

	//! <yaw, pitch, roll> like the base inspection default pose.
	protected vector m_vAngles;
	protected vector m_vAnglesTarget;
	protected vector m_vLook;
	protected vector m_vLookTarget;
	protected float m_fDist;
	protected float m_fDistTarget;
	protected float m_fEaseScale = 1;

	protected vector m_vHomeAngles;
	protected vector m_vHomeLook;
	protected float m_fHomeDist;
	protected float m_fZoomMin = 0.3;
	protected float m_fZoomMax = 5;
	protected bool m_bYawLimited;
	protected float m_fYawCenter;
	protected float m_fYawHalfRange = 180;
	protected vector m_vPanCenter;
	protected float m_fPanRange = 1.5;
	protected float m_fPanMinY = 0.05;
	protected float m_fPanMaxY = 1.6;

	protected bool m_bSubjectSpinYaw;
	protected bool m_bPointerRotate;
	protected bool m_bDragHeld;
	protected bool m_bMouseRotate;
	protected bool m_bPanHeld;
	protected bool m_bMousePan;
	protected bool m_bPanModeHeld;
	protected bool m_bPanArmed;
	protected bool m_bResetHeld;
	protected bool m_bMouseSampled;
	protected bool m_bTranslationTrack;
	protected int m_iMouseX;
	protected int m_iMouseY;
	protected float m_fPointerZoom;

	//------------------------------------------------------------------------------------------------
	bool IsAlive()
	{
		return m_World != null;
	}

	//------------------------------------------------------------------------------------------------
	BaseWorld GetWorld()
	{
		return m_World;
	}

	//------------------------------------------------------------------------------------------------
	IEntity GetEnvironmentRig()
	{
		return m_EnvironmentRig;
	}

	//------------------------------------------------------------------------------------------------
	//! Selects a stage environment before the private world is created.
	bool SetEnvironment(ResourceName environmentPrefab)
	{
		if (m_World || environmentPrefab.IsEmpty())
			return false;

		m_EnvironmentPrefab = environmentPrefab;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool SetWorldType(string worldType)
	{
		if (m_World || worldType.IsEmpty())
			return false;

		m_sWorldType = worldType;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Enables atmosphere-driven exposure before the private world is created.
	bool SetAutomaticHDR(bool automaticHDR)
	{
		if (m_World)
			return false;

		m_bAutomaticHDR = automaticHDR;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool SetCameraEVAdjustment(float adjustment)
	{
		if (m_World)
			return false;

		m_fCameraEVAdjustment = adjustment;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool EnsureWorld(string namePrefix)
	{
		if (m_World)
			return true;

		s_iWorldCounter++;
		string worldName = string.Format("%1_%2", namePrefix, s_iWorldCounter);
		string worldType = worldName;
		if (!m_sWorldType.IsEmpty())
			worldType = m_sWorldType;
		m_WorldRef = BaseWorld.CreateWorld(worldType, worldName);
		if (!m_WorldRef)
		{
			GRSA_Log.Error("StageCore: CreateWorld failed");
			return false;
		}

		m_World = m_WorldRef.GetRef();
		if (!m_World)
		{
			GRSA_Log.Error("StageCore: world reference is empty");
			m_WorldRef = null;
			return false;
		}

		m_World.SetCameraType(CAMERA, CameraType.PERSPECTIVE);
		m_World.SetCameraFarPlane(CAMERA, CAMERA_FAR_PLANE);
		m_World.SetCameraNearPlane(CAMERA, CAMERA_NEAR_PLANE);
		m_World.SetCameraVerticalFOV(CAMERA, CAMERA_FOV);
		if (m_bAutomaticHDR)
			m_World.SetCameraHDRBrightness(CAMERA, -1);
		else
			m_World.SetCameraHDRBrightness(CAMERA, 1.0);
		m_World.AdjustCameraEV(CAMERA, m_fCameraEVAdjustment);

		//! A non-preview world relies on its selected environment for world properties as well as scenery.
		Resource lighting = Resource.Load(m_EnvironmentPrefab);
		if (lighting && lighting.IsValid())
			m_EnvironmentRig = GetGame().SpawnEntityPrefab(lighting, m_World);
		if (m_EnvironmentRig)
		{
			BindStudioLights();
			if (m_bStudioLightingAllowed && (!m_StudioKey || !m_StudioFillLeft || !m_StudioFillRight))
				GRSA_Log.Warn("StageCore: camera-relative studio lights are missing from the environment rig");
		}
		else
			GRSA_Log.Warn("StageCore: environment failed to load, stage will be dark");

		//! The motes are air-decoupled (.ptc ParentVelRelToAir 0): a created world has no
		//! weather state, and the base dust value of 2 multiplied that undefined air velocity
		//! into room-sized stretched sheets.
		EnsureDust();

		//! Every fresh studio opens on the wide establishing pose; the first station glide that
		//! follows is the menu's opening camera move.
		SnapTo(WIDE_ANGLES, WIDE_LOOK, WIDE_DIST);

		m_InputManager = GetGame().GetInputManager();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void BindTarget(notnull RenderTargetWidget target)
	{
		if (!m_World)
			return;

		if (m_DragHandler)
			m_DragHandler.SetTarget(target);
		else
			m_DragHandler = new GRSA_StageDragHandler(target, this);

		m_wTarget = target;
		target.SetWorld(m_World, CAMERA);
		//! A render target's refresh period defaults to never — without this the widget shows
		//! nothing but its clear color while the world renders to nobody.
		target.SetRefresh(1, 0);
		target.Update();
		if (m_bAutomaticHDR)
			m_World.SetCameraHDRBrightness(CAMERA, -1);
		else
			m_World.SetCameraHDRBrightness(CAMERA, 1.0);
		m_World.AdjustCameraEV(CAMERA, m_fCameraEVAdjustment);
	}

	//------------------------------------------------------------------------------------------------
	void SetHome(vector angles, vector look, float dist)
	{
		angles[0] = ClampYaw(angles[0]);
		m_vHomeAngles = angles;
		m_vHomeLook = look;
		m_fHomeDist = Math.Clamp(dist, m_fZoomMin, m_fZoomMax);
	}

	//------------------------------------------------------------------------------------------------
	void SetZoomRange(float minDist, float maxDist)
	{
		m_fZoomMin = Math.Max(0.05, minDist);
		m_fZoomMax = Math.Max(m_fZoomMin, maxDist);
		m_fDistTarget = Math.Clamp(m_fDistTarget, m_fZoomMin, m_fZoomMax);
		m_fDist = Math.Clamp(m_fDist, m_fZoomMin, m_fZoomMax);
	}

	//------------------------------------------------------------------------------------------------
	void SetYawLimit(float center, float halfRange)
	{
		m_bYawLimited = true;
		m_fYawCenter = center;
		m_fYawHalfRange = Math.Clamp(halfRange, 0, 180);
		m_vAnglesTarget[0] = ClampYaw(m_vAnglesTarget[0]);
		m_vAngles[0] = ClampYaw(m_vAngles[0]);
	}

	//------------------------------------------------------------------------------------------------
	void ClearYawLimit()
	{
		m_bYawLimited = false;
	}

	//------------------------------------------------------------------------------------------------
	void SetStudioLightingAllowed(bool allowed)
	{
		m_bStudioLightingAllowed = allowed;
	}

	//------------------------------------------------------------------------------------------------
	void SetTranslationTrack(bool enabled)
	{
		m_bTranslationTrack = enabled;
	}

	//------------------------------------------------------------------------------------------------
	//! Weapon stations spin the piece under a fixed camera; the soldier station orbits the
	//! camera instead — skinned garments pin to the spawn orientation in a created world, so
	//! a live-rotated character walks out of its own vest.
	void SetSubjectSpin(bool subjectSpin)
	{
		m_bSubjectSpinYaw = subjectSpin;
	}

	//------------------------------------------------------------------------------------------------
	//! The adjustable key and two rear fills are authored last in the rig's light group.
	protected void BindStudioLights()
	{
		m_StudioKey = null;
		m_StudioFillLeft = null;
		m_StudioFillRight = null;
		CollectStudioLights(m_EnvironmentRig);
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectStudioLights(IEntity entity)
	{
		if (!entity)
			return;

		LightEntity light = LightEntity.Cast(entity);
		if (light)
		{
			m_StudioKey = m_StudioFillLeft;
			m_StudioFillLeft = m_StudioFillRight;
			m_StudioFillRight = light;
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			CollectStudioLights(child);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Widget mouse handlers own the press boundary; relative movement still comes from the
	//! active armory input context.
	void SetPointerRotate(bool active)
	{
		m_bPointerRotate = active;
		m_bMouseSampled = false;
		if (!active)
			m_bMouseRotate = false;
	}

	//------------------------------------------------------------------------------------------------
	void AddPointerZoom(float wheel)
	{
		m_fPointerZoom = wheel;
	}

	//------------------------------------------------------------------------------------------------
	//! Open stage pixels rotate; buttons, lists, sliders and scroll regions retain their input.
	bool CanStartPointerRotate(Widget source, int x, int y)
	{
		if (!m_wTarget || !m_wTarget.IsVisibleInHierarchy() || !IsOverStage(x, y))
			return false;

		// Workspace callbacks can name the workspace instead of the clicked control.
		Widget hit = WidgetManager.GetWidgetUnderCursor();
		return !IsStageControl(hit) && !IsStageControl(source);
	}

	protected bool IsStageControl(Widget cursor)
	{
		Widget stageRoot = m_wTarget.GetParent();
		while (cursor && cursor != stageRoot)
		{
			if (ButtonWidget.Cast(cursor) || EditBoxWidget.Cast(cursor) || MultilineEditBoxWidget.Cast(cursor)
				|| CheckBoxWidget.Cast(cursor) || SliderWidget.Cast(cursor) || BaseListboxWidget.Cast(cursor)
				|| ScrollLayoutWidget.Cast(cursor) || cursor.FindHandler(SCR_ButtonBaseComponent))
				return true;

			cursor = cursor.GetParent();
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	void SetPanBounds(vector center, float range, float minY, float maxY)
	{
		m_vPanCenter = center;
		m_fPanRange = range;
		m_fPanMinY = minY;
		m_fPanMaxY = maxY;
	}

	//------------------------------------------------------------------------------------------------
	void GoHome(bool snap = false)
	{
		if (snap)
		{
			SnapTo(m_vHomeAngles, m_vHomeLook, m_fHomeDist);
			return;
		}

		if (vector.Distance(m_vLook, m_vHomeLook) > TRAVEL_ENGAGE_DIST)
			m_fEaseScale = TRAVEL_EASE_SCALE;

		m_vAnglesTarget = m_vHomeAngles;
		m_vLookTarget = m_vHomeLook;
		m_fDistTarget = m_fHomeDist;
	}

	//------------------------------------------------------------------------------------------------
	//! Hard-set the camera, targets included — no glide, no leftover travel pacing.
	void SnapTo(vector angles, vector look, float dist)
	{
		m_vAnglesTarget = angles;
		m_vLookTarget = look;
		m_fDistTarget = dist;
		m_vAngles = angles;
		m_vLook = look;
		m_fDist = dist;
		m_fEaseScale = 1;
		ApplyCamera();
	}

	//------------------------------------------------------------------------------------------------
	//! Glide in on a world point at the given distance, keeping the current viewing angle.
	void FocusOn(vector worldPos, float dist)
	{
		m_vLookTarget = worldPos;
		m_fDistTarget = Math.Clamp(dist, m_fZoomMin, m_fZoomMax);
	}

	//------------------------------------------------------------------------------------------------
	void FocusOnAtYaw(vector worldPos, float dist, float yaw)
	{
		m_vAnglesTarget[0] = ClampYaw(yaw);
		FocusOn(worldPos, dist);
	}

	//------------------------------------------------------------------------------------------------
	//! Widget-local reference position of a world point, projected through the stage's own camera
	//! and the render widget's real aspect — the workspace projection assumes a full-screen camera
	//! and lands offset and aspect-skewed inside a smaller widget.
	bool ProjectWorldPoint(vector worldPos, out vector screenPos)
	{
		if (!m_World || !m_wTarget)
			return false;

		vector camMat[4];
		Math3D.AnglesToMatrix(m_vAngles, camMat);
		vector camPos = m_vLook - camMat[2] * m_fDist;

		vector rel = worldPos - camPos;
		float cx = vector.Dot(rel, camMat[0]);
		float cy = vector.Dot(rel, camMat[1]);
		float cz = vector.Dot(rel, camMat[2]);
		if (cz <= 0.001)
			return false;

		float pxW, pxH;
		m_wTarget.GetScreenSize(pxW, pxH);
		if (pxW <= 0 || pxH <= 0)
			return false;

		float halfV = Math.Tan(CAMERA_FOV * 0.5 * Math.DEG2RAD);
		float halfH = halfV * (pxW / pxH);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float refW = workspace.DPIUnscale(pxW);
		float refH = workspace.DPIUnscale(pxH);

		float ndcX = cx / (cz * halfH);
		float ndcY = cy / (cz * halfV);
		screenPos[0] = (ndcX * 0.5 + 0.5) * refW;
		screenPos[1] = (0.5 - ndcY * 0.5) * refH;
		screenPos[2] = cz;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Per-frame: polled manual navigation on top of the glide targets, then one camera write.
	void Tick(float tDelta)
	{
		if (!m_World)
			return;

		PollInput(tDelta);

		//! Recover if the effect is stopped externally so the studio air remains visible.
		if (m_Dust && m_Dust.GetState() == EParticleEffectState.STOPPED)
			m_Dust.Play();

		vector angleDelta = m_vAnglesTarget - m_vAngles;
		float lookDelta = vector.Distance(m_vLook, m_vLookTarget);
		float distDelta = Math.AbsFloat(m_fDistTarget - m_fDist);

		float alpha = Math.Clamp((angleDelta.Length() * EASE_ANGLE + EASE_FLOOR) * tDelta * m_fEaseScale, 0, 1);
		m_vAngles[0] = Math.Lerp(m_vAngles[0], m_vAnglesTarget[0], alpha);
		m_vAngles[1] = Math.Lerp(m_vAngles[1], m_vAnglesTarget[1], alpha);

		alpha = Math.Clamp((lookDelta * EASE_MOVE + EASE_FLOOR) * tDelta * m_fEaseScale, 0, 1);
		m_vLook = vector.Lerp(m_vLook, m_vLookTarget, alpha);

		alpha = Math.Clamp((distDelta * EASE_MOVE + EASE_FLOOR) * tDelta * m_fEaseScale, 0, 1);
		m_fDist = Math.Lerp(m_fDist, m_fDistTarget, alpha);

		//! Travel pacing releases on arrival so manual orbit gets its normal response back.
		if (m_fEaseScale < 1 && angleDelta.Length() < ARRIVE_ANGLE_EPS && lookDelta < ARRIVE_LOOK_EPS && distDelta < ARRIVE_DIST_EPS)
			m_fEaseScale = 1;

		ApplyCamera();
	}

	//------------------------------------------------------------------------------------------------
	protected void PollInput(float tDelta)
	{
		if (!m_InputManager || !m_wTarget)
			return;

		//! A hidden render target (another tab fronting the shell) must not steer the shared
		//! camera — the gamepad path has no cursor hit-test that would otherwise stop it.
		if (!m_wTarget.IsVisibleInHierarchy())
			return;

		GRSA_ClientPrefs prefs = GRSA_ClientPrefs.Get();

		if (m_InputManager.IsUsingMouseAndKeyboard())
		{
			int mouseX, mouseY;
			WidgetManager.GetMousePos(mouseX, mouseY);

			//! The widget event preserves UI hit-testing; the action is the fallback when another
			//! menu component consumes that event before the workspace handler receives it.
			bool dragHeld = m_bPointerRotate || m_InputManager.GetActionValue("GRSA_ArmoryDrag") > 0;
			int mouseDeltaX;
			int mouseDeltaY;
			if (dragHeld)
			{
				if (m_bMouseSampled)
				{
					mouseDeltaX = mouseX - m_iMouseX;
					mouseDeltaY = mouseY - m_iMouseY;
				}
				m_iMouseX = mouseX;
				m_iMouseY = mouseY;
				m_bMouseSampled = true;
			}
			else
			{
				m_bMouseSampled = false;
			}

			if (dragHeld && !m_bDragHeld)
				m_bMouseRotate = CanStartPointerRotate(WidgetManager.GetWidgetUnderCursor(), mouseX, mouseY);
			else if (!dragHeld)
				m_bMouseRotate = false;
			m_bDragHeld = dragHeld;

			bool panHeld = m_InputManager.GetActionValue("GRSA_ArmoryPan") > 0;
			if (panHeld && !m_bPanHeld)
				m_bMousePan = IsOverStage(mouseX, mouseY);
			else if (!panHeld)
				m_bMousePan = false;
			m_bPanHeld = panHeld;

			if (m_bMousePan)
			{
				//! Speed rides the zoom distance with a floor, so panning never goes dead at
				//! close inspection range.
				Pan(m_InputManager.GetActionValue("GRSA_ArmoryYaw"), m_InputManager.GetActionValue("GRSA_ArmoryPitch"), Math.Max(m_fDist, 0.8) * 0.004);
			}
			else if (m_bMouseRotate)
			{
				if (m_bTranslationTrack)
				{
					float trackX = m_InputManager.GetActionValue("GRSA_ArmoryYaw");
					float trackY = m_InputManager.GetActionValue("GRSA_ArmoryPitch");
					if (trackX == 0)
						trackX = mouseDeltaX;
					if (trackY == 0)
						trackY = mouseDeltaY;
					Pan(trackX, trackY, Math.Max(m_fDist, 0.8) * 0.004);
				}
				else
				{
					//! Drag is yaw only and the camera never tilts — a drag-pitched camera
					//! ratchets into a birds-eye nothing on the station resets.
					float yawInput = m_InputManager.GetActionValue("GRSA_ArmoryYaw");
					if (yawInput == 0 && mouseDeltaX != 0)
						yawInput = mouseDeltaX;

					float yawDelta = yawInput * prefs.GetOrbitScale() * 0.75;
					if (m_bSubjectSpinYaw)
						m_OnSubjectSpin.Invoke(yawDelta);
					else
						m_vAnglesTarget[0] = ClampYaw(m_vAnglesTarget[0] + yawDelta);
				}
			}

			//! The full-bleed render target sits under every side panel — scroll regions own the
			//! wheel there, the camera only zooms over open stage.
			if (IsOverStage(mouseX, mouseY)
				&& !GRSA_SmoothScrollComponent.IsCursorOverAny(mouseX, mouseY)
				&& !GRSA_CarouselComponent.IsCursorOverAny(mouseX, mouseY))
			{
				float wheel = m_InputManager.GetActionValue("GRSA_ArmoryZoomWheel");
				if (wheel == 0)
					wheel = m_fPointerZoom;
				if (wheel != 0)
					m_fDistTarget = Math.Clamp(m_fDistTarget * (1 - wheel * 0.001), m_fZoomMin, m_fZoomMax);
			}
			m_fPointerZoom = 0;
		}
		else
		{
			//! Left-stick click toggles the right stick between orbit and pan; right-stick click
			//! glides everything back to the hero pose.
			bool panPressed = m_InputManager.GetActionValue("GRSA_ArmoryPanMode") > 0;
			if (panPressed && !m_bPanModeHeld)
				m_bPanArmed = !m_bPanArmed;
			m_bPanModeHeld = panPressed;

			bool resetPressed = m_InputManager.GetActionValue("GRSA_ArmoryReset") > 0;
			if (resetPressed && !m_bResetHeld)
			{
				m_bPanArmed = false;
				GoHome();
			}
			m_bResetHeld = resetPressed;

			//! 100 deg/s at full stick matches the base inspection screen's gamepad rate.
			float scale = prefs.GetOrbitScale() * 100 * tDelta;
			if (m_bPanArmed || m_bTranslationTrack)
			{
				Pan(-m_InputManager.GetActionValue("GRSA_ArmoryYaw"), -m_InputManager.GetActionValue("GRSA_ArmoryPitch"), Math.Max(m_fDist, 0.8) * 2 * tDelta);
			}
			else
			{
				float yawDelta = m_InputManager.GetActionValue("GRSA_ArmoryYaw") * scale;
				if (m_bSubjectSpinYaw)
					m_OnSubjectSpin.Invoke(yawDelta);
				else
					m_vAnglesTarget[0] = ClampYaw(m_vAnglesTarget[0] + yawDelta);
			}

			float zoomDelta = m_InputManager.GetActionValue("GRSA_ArmoryZoomIn") - m_InputManager.GetActionValue("GRSA_ArmoryZoomOut");
			if (zoomDelta != 0)
				m_fDistTarget = Math.Clamp(m_fDistTarget * (1 - zoomDelta * tDelta * 1.5), m_fZoomMin, m_fZoomMax);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Grab-the-scene pan in the camera plane, clamped to a bubble around the host's subject so
	//! the view can never wander off the set. Any focus glide or GoHome retargets the look point.
	protected void Pan(float inputX, float inputY, float scale)
	{
		vector camMat[4];
		Math3D.AnglesToMatrix(m_vAngles, camMat);
		vector target = m_vLookTarget - camMat[0] * (inputX * scale) + camMat[1] * (inputY * scale);
		target[0] = Math.Clamp(target[0], m_vPanCenter[0] - m_fPanRange, m_vPanCenter[0] + m_fPanRange);
		target[1] = Math.Clamp(target[1], m_fPanMinY, m_fPanMaxY);
		target[2] = Math.Clamp(target[2], m_vPanCenter[2] - m_fPanRange, m_vPanCenter[2] + m_fPanRange);
		m_vLookTarget = target;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsOverStage(int x, int y)
	{
		if (!m_wTarget)
			return false;

		float sizeX, sizeY, posX, posY;
		m_wTarget.GetScreenSize(sizeX, sizeY);
		m_wTarget.GetScreenPos(posX, posY);
		return x >= posX && x <= posX + sizeX && y >= posY && y <= posY + sizeY;
	}

	//------------------------------------------------------------------------------------------------
	//! Angles-plus-distance placement around the look target, the base inspection pattern.
	protected void ApplyCamera()
	{
		vector camMat[4];
		Math3D.AnglesToMatrix(m_vAngles, camMat);
		camMat[3] = m_vLook - camMat[2] * m_fDist;
		m_World.SetCameraEx(CAMERA, camMat);
		ApplyStudioLighting(camMat);
	}

	//------------------------------------------------------------------------------------------------
	protected float ClampYaw(float yaw)
	{
		if (!m_bYawLimited)
			return yaw;

		float delta = yaw - m_fYawCenter;
		while (delta > 180)
			delta -= 360;
		while (delta < -180)
			delta += 360;
		return m_fYawCenter + Math.Clamp(delta, -m_fYawHalfRange, m_fYawHalfRange);
	}

	//------------------------------------------------------------------------------------------------
	//! The key follows just above the camera while paired fills sit behind the subject, so every
	//! rotated side remains readable without putting visible fixtures in the stage.
	protected void ApplyStudioLighting(vector camMat[4])
	{
		if (!m_StudioKey || !m_StudioFillLeft || !m_StudioFillRight)
			return;

		GRSA_ClientPrefs prefs = GRSA_ClientPrefs.Get();
		bool enabled = m_bStudioLightingAllowed && prefs.m_bStudioLighting;
		m_StudioKey.SetEnabled(enabled);
		m_StudioFillLeft.SetEnabled(enabled);
		m_StudioFillRight.SetEnabled(enabled);
		if (!enabled)
			return;

		float brightness = prefs.m_iStudioBrightness * 0.01;
		float rearFill = prefs.m_iStudioRearFill * 0.01;
		m_StudioKey.SetColor(new Color(1, 0.97, 0.92, 1), STUDIO_KEY_LV * brightness);
		m_StudioFillLeft.SetColor(new Color(0.88, 0.93, 1, 1), STUDIO_FILL_LV * brightness * rearFill);
		m_StudioFillRight.SetColor(new Color(0.88, 0.93, 1, 1), STUDIO_FILL_LV * brightness * rearFill);

		vector keyMat[4];
		keyMat[0] = camMat[0];
		keyMat[1] = camMat[1];
		keyMat[2] = camMat[2];
		keyMat[3] = camMat[3] + camMat[0] * 0.65 + camMat[1] * 0.55 + camMat[2] * 0.45;
		m_StudioKey.SetTransform(keyMat);

		vector fillLeftMat[4];
		fillLeftMat[0] = camMat[0];
		fillLeftMat[1] = camMat[1];
		fillLeftMat[2] = camMat[2];
		fillLeftMat[3] = m_vLook + camMat[2] * 0.6 + camMat[1] * 0.55 - camMat[0] * 0.65;
		m_StudioFillLeft.SetTransform(fillLeftMat);

		vector fillRightMat[4];
		fillRightMat[0] = camMat[0];
		fillRightMat[1] = camMat[1];
		fillRightMat[2] = camMat[2];
		fillRightMat[3] = m_vLook + camMat[2] * 0.6 + camMat[1] * 0.55 + camMat[0] * 0.65;
		m_StudioFillRight.SetTransform(fillRightMat);
	}

	//------------------------------------------------------------------------------------------------
	protected void EnsureDust()
	{
		if (m_Dust || !m_World)
			return;

		ParticleEffectEntitySpawnParams spawnParams = new ParticleEffectEntitySpawnParams();
		spawnParams.TargetWorld = m_World;
		spawnParams.UseFrameEvent = true;
		spawnParams.DeleteWhenStopped = false;
		spawnParams.PlayOnSpawn = true;
		spawnParams.Transform[3] = "0 0.6 0";
		m_Dust = ParticleEffectEntity.SpawnParticleEffect(DUST_EFFECT, spawnParams);
		if (!m_Dust)
			GRSA_Log.Warn("StageCore: dust effect failed to spawn, stage stays clean");
	}

	//------------------------------------------------------------------------------------------------
	void UnbindTarget()
	{
		if (m_DragHandler)
			m_DragHandler.Destroy();

		if (m_wTarget)
			m_wTarget.SetWorld(null, 0);

		m_DragHandler = null;
		m_wTarget = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the widget to its previous path; dropping the owning reference deletes the world
	//! and everything spawned in it.
	void Release()
	{
		UnbindTarget();
		m_Dust = null;
		m_StudioKey = null;
		m_StudioFillLeft = null;
		m_StudioFillRight = null;
		m_EnvironmentRig = null;
		m_World = null;
		m_WorldRef = null;
	}
}
