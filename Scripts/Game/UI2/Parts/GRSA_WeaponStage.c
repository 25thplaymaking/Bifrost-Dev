//! Weapon host for the shared studio: resolves the pooled weapon, clones it onto the bench in a
//! fixed hero pose, and keeps the draft's attachment set synced onto the clone — even while
//! another tab fronts the studio, so the bench in the backdrop never shows a stale build. The
//! shared stage core owns the world, environment rig, dust and the glide camera; this class
//! owns what stands on the bench and the bench station's framing.
class GRSA_WeaponStage
{
	protected static const float HOME_DISTANCE_SCALE = 1.15;
	protected static const float FOCUS_DISTANCE_SCALE = 0.42;
	protected static const float ZOOM_MIN_SCALE = 0.15;
	protected static const float ZOOM_MAX_SCALE = 2.5;
	protected static const float FOCUS_YAW_SCALE = 50;
	protected static const float FOCUS_YAW_MAX = 24;
	protected static const float FOCUS_PITCH_SCALE = -50;
	protected static const float FOCUS_PITCH_MAX = 12;
	protected static const float FOCUS_SPIN_EASE = 8;
	protected static const float FOCUS_SPIN_EPSILON = 0.05;
	//! World Y of the workbench top the weapon rests on; the lift keeps the lowest point of the
	//! bounds from z-fighting the tabletop.
	protected static const float REST_SURFACE_Y = 0.378;
	protected static const float REST_LIFT = 0.01;
	protected static const float PAN_RANGE = 1.5;
	protected static const float PAN_MIN_Y = 0.05;
	protected static const float PAN_MAX_Y = 1.6;
	//! Display yaw of the resting weapon: muzzle screen-right toward the open side of the room
	//! (not into the gunwall), angled 12 toward the camera for depth. 0 = flat muzzle-right
	//! profile, 180 = flat muzzle-left.
	protected static const float WEAPON_POSE_YAW = 348;
	//! Camera yaw matching the set's -120 rotation faces the bench front and gunwall squarely
	//! (the wall centroid sits at azimuth ~-120 from the bench). Retune with the set.
	protected static const vector HOME_ANGLES = "-120 -18 0";
	//! Framing for a bare bench while no draft weapon is staged.
	protected static const vector BENCH_LOOK = "0 0.55 0";
	protected static const float BENCH_EMPTY_DIST = 1.6;
	protected static const float EMPTY_ZOOM_MIN = 0.4;
	protected static const float EMPTY_ZOOM_MAX = 4;

	//! Borrowed from the owning hub — a strong ref here could root the world through the draft
	//! service's static invoker if a subscription survives a hard teardown.
	protected GRSA_StageCore m_Core;
	protected ref array<ResourceName> m_aUnplaced = {};
	protected RenderTargetWidget m_wTarget;
	protected IEntity m_Weapon;
	protected IEntity m_Pooled;
	protected IEntity m_SlotSource;
	protected ResourceName m_WeaponPrefab;
	protected string m_sSyncedSignature;
	protected vector m_vRestCenter;
	protected vector m_vRestCenterLocal;
	protected float m_fBoundsDiag;
	protected float m_fSpinYaw;
	protected float m_fFocusYaw;
	protected float m_fFocusYawTarget;
	protected float m_fFocusPitch;
	protected float m_fFocusPitchTarget;
	protected vector m_vFocusLocal;
	protected float m_fFocusDist;
	protected bool m_bHasFocus;

	//------------------------------------------------------------------------------------------------
	void GRSA_WeaponStage(notnull GRSA_StageCore core)
	{
		m_Core = core;
		core.m_OnSubjectSpin.Insert(OnSubjectSpin);

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Insert(OnDraftChanged);
	}

	//------------------------------------------------------------------------------------------------
	//! Creates a runtime render widget when the authored node is unavailable.
	static RenderTargetWidget CreateFallbackRender(Widget parent)
	{
		if (!parent)
			return null;

		RenderTargetWidget render = RenderTargetWidget.Cast(GetGame().GetWorkspace().CreateWidget(WidgetType.RenderTargetWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NOFOCUS, new Color(), 0, parent));
		if (!render)
		{
			GRSA_Log.Error("WeaponStage: no render widget available, stage cannot draw");
			return null;
		}

		AlignableSlot.SetHorizontalAlign(render, LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetVerticalAlign(render, LayoutVerticalAlign.Stretch);
		render.SetClearColor(false, 0);
		render.SetZOrder(-100);
		return render;
	}

	//------------------------------------------------------------------------------------------------
	bool IsAlive()
	{
		return m_Weapon != null;
	}

	//------------------------------------------------------------------------------------------------
	IEntity GetWeapon()
	{
		return m_Weapon;
	}

	//------------------------------------------------------------------------------------------------
	//! Fronts the studio through this render node even before any weapon is staged — an empty
	//! bench is a valid scene.
	void ShowOn(notnull RenderTargetWidget target)
	{
		if (!m_Core.EnsureWorld("GRSA_Stage"))
			return;

		m_wTarget = target;
		m_Core.BindTarget(target);
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns (or reuses) the weapon resting on the bench and binds the stage world to the render
	//! widget. Returns the live weapon entity for slot enumeration.
	IEntity ShowWeapon(ResourceName prefab, notnull RenderTargetWidget target)
	{
		if (prefab.IsEmpty() || !m_Core.EnsureWorld("GRSA_Stage"))
			return null;

		m_wTarget = target;
		m_Core.BindTarget(target);

		if (m_Weapon && !m_Weapon.IsDeleted() && m_WeaponPrefab == prefab)
			return SlotSource();

		ReleaseWeapon();

		//! Both working own-world previews (the base inspect screen and the vanilla-derived
		//! editors) put the item in through the preview pipeline — a raw prefab spawn renders
		//! black in a runtime world. Resolve the pooled entity, clone it into the stage.
		ItemPreviewManagerEntity manager = GRSA_ItemIntel.GetPreviewManager();
		if (!manager)
			return null;

		IEntity pooled = manager.ResolvePreviewEntityForPrefab(prefab);
		if (!pooled)
		{
			GRSA_Log.Warn("WeaponStage: pooled weapon failed to resolve");
			return null;
		}

		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(pooled.FindComponent(InventoryItemComponent));
		if (!itemComponent)
		{
			GRSA_Log.Warn("WeaponStage: weapon has no InventoryItemComponent, cannot stage");
			return null;
		}

		m_Pooled = pooled;
		m_WeaponPrefab = prefab;
		if (!RecloneWeapon())
			return null;

		FrameStation();
		return SlotSource();
	}

	//------------------------------------------------------------------------------------------------
	//! The entity hardpoint enumeration should read slots from.
	IEntity SlotSource()
	{
		return m_SlotSource;
	}

	//------------------------------------------------------------------------------------------------
	//! Applies the draft's attachment set: mutate the pooled source (the render clone strips its
	//! storages), then re-clone so the stage shows the new build. Returns the fresh slot source
	//! for the callout rebuild, null when nothing is staged. pins runs parallel to attachments
	//! (-1 = automatic placement); the shared dressing walker owns the placement rules. A sync
	//! whose signature matches the staged build is a no-op — the screen and the draft listener
	//! both feed this path, and only one of them should pay for a re-clone.
	IEntity SyncAttachments(notnull array<ResourceName> attachments, array<int> pins = null)
	{
		if (!m_Pooled || !m_Core.IsAlive())
			return null;

		string signature = BuildSignature(attachments, pins);
		if (signature == m_sSyncedSignature)
			return SlotSource();

		DeleteRenderedWeapon();
		GRSA_PreviewDress.SyncWeaponAttachmentList(m_Pooled, attachments, m_Pooled.GetWorld(), pins, m_aUnplaced);
		if (!RecloneWeapon())
			return null;

		m_sSyncedSignature = signature;
		float focusYaw = m_fFocusYaw;
		float focusPitch = m_fFocusPitch;
		m_fFocusYaw = 0;
		m_fFocusPitch = 0;
		ApplyRestPose(true);
		RefreshStationContract();
		m_fFocusYaw = focusYaw;
		m_fFocusPitch = focusPitch;
		ApplyRestPose();
		if (m_bHasFocus)
		{
			m_fFocusDist = m_fBoundsDiag * FOCUS_DISTANCE_SCALE;
			m_Core.FocusOn(m_Weapon.CoordToParent(m_vFocusLocal), m_fFocusDist);
		}
		return SlotSource();
	}

	//------------------------------------------------------------------------------------------------
	//! Attachments the engine refused to mount during the last sync — the blocked-slot signal
	//! (a mounted bayonet refuses a suppressor the way worn overalls refuse pants).
	array<ResourceName> GetUnplaced()
	{
		return m_aUnplaced;
	}

	//------------------------------------------------------------------------------------------------
	protected string BuildSignature(notnull array<ResourceName> attachments, array<int> pins)
	{
		string signature = m_WeaponPrefab;
		foreach (int i, ResourceName attachment : attachments)
		{
			int pin = -1;
			if (pins && i < pins.Count())
				pin = pins[i];
			signature += attachment + ":" + pin.ToString() + ";";
		}
		return signature;
	}

	//------------------------------------------------------------------------------------------------
	//! Keeps the bench honest while another tab fronts the studio: the staged weapon follows the
	//! draft — attachments resync, and a weapon dropped from the draft clears the bench.
	protected void OnDraftChanged()
	{
		if (!m_Weapon || m_WeaponPrefab.IsEmpty())
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.m_Draft)
			return;

		GRSA_KitWeapon match;
		foreach (GRSA_KitWeapon weapon : service.m_Draft.m_aWeapons)
		{
			if (weapon && weapon.m_Prefab == m_WeaponPrefab)
			{
				match = weapon;
				break;
			}
		}

		if (!match)
		{
			ClearStage();
			return;
		}

		match.EnsurePins();
		SyncAttachments(match.m_aAttachments, match.m_aAttachmentSlots);
	}

	//------------------------------------------------------------------------------------------------
	//! Yaw-only pose (factory yaw plus the manual spin); the origin compensates for the rotated
	//! bounds center so the weapon stays centered over the mat at any yaw.
	protected void ApplyRestPose(bool captureHomeBounds = false)
	{
		if (!m_Weapon)
			return;

		vector ypr = Vector(WEAPON_POSE_YAW + m_fSpinYaw + m_fFocusYaw, m_fFocusPitch, 0);
		m_Weapon.SetYawPitchRoll(ypr);

		vector mins, maxs;
		m_Weapon.GetBounds(mins, maxs);
		vector center = (mins + maxs) * 0.5;
		vector rot[4];
		Math3D.AnglesToMatrix(ypr, rot);
		vector pos = -(rot[0] * center[0] + rot[1] * center[1] + rot[2] * center[2]);
		m_Weapon.SetOrigin(pos);
		m_Weapon.Update();

		//! The engine helper unions the receiver and every mounted visual in world space. Using
		//! child GetTransform as a local matrix double-transforms magazines on some weapon trees.
		SCR_Global.GetWorldBoundsWithChildren(m_Weapon, mins, maxs);
		vector worldCenter = (mins + maxs) * 0.5;
		pos = m_Weapon.GetOrigin();
		pos[0] = pos[0] - worldCenter[0];
		pos[2] = pos[2] - worldCenter[2];
		pos[1] = pos[1] + REST_SURFACE_Y + REST_LIFT - mins[1];
		m_Weapon.SetOrigin(pos);
		m_Weapon.Update();

		if (captureHomeBounds)
		{
			SCR_Global.GetWorldBoundsWithChildren(m_Weapon, mins, maxs);
			m_vRestCenter = (mins + maxs) * 0.5;
			m_vRestCenterLocal = m_Weapon.CoordToLocal(m_vRestCenter);
			m_fBoundsDiag = vector.Distance(mins, maxs);
			if (m_fBoundsDiag <= 0)
				m_fBoundsDiag = 1;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Manual rotation turns the resting weapon in place; only the fronting tab's subject listens.
	protected void OnSubjectSpin(float yawDelta)
	{
		if (!m_Weapon || !m_wTarget || !m_wTarget.IsVisibleInHierarchy())
			return;

		m_fSpinYaw += yawDelta;
		m_fFocusYawTarget = 0;
		m_fFocusPitchTarget = 0;
		ApplyRestPose();
	}

	//------------------------------------------------------------------------------------------------
	//! Eases the contextual hardpoint presentation and keeps the camera locked to the transformed
	//! mount while the weapon turns beneath it.
	void Tick(float tDelta)
	{
		if (!m_Weapon)
			return;

		float alpha = Math.Clamp(tDelta * FOCUS_SPIN_EASE, 0, 1);
		bool changed;
		if (Math.AbsFloat(m_fFocusYawTarget - m_fFocusYaw) > FOCUS_SPIN_EPSILON)
		{
			m_fFocusYaw = Math.Lerp(m_fFocusYaw, m_fFocusYawTarget, alpha);
			changed = true;
		}
		else
		{
			m_fFocusYaw = m_fFocusYawTarget;
		}

		if (Math.AbsFloat(m_fFocusPitchTarget - m_fFocusPitch) > FOCUS_SPIN_EPSILON)
		{
			m_fFocusPitch = Math.Lerp(m_fFocusPitch, m_fFocusPitchTarget, alpha);
			changed = true;
		}
		else
		{
			m_fFocusPitch = m_fFocusPitchTarget;
		}

		if (changed)
			ApplyRestPose();

		if (m_bHasFocus)
			m_Core.FocusOn(m_Weapon.CoordToParent(m_vFocusLocal), m_fFocusDist);
	}

	//------------------------------------------------------------------------------------------------
	protected bool RecloneWeapon()
	{
		DeleteRenderedWeapon();

		if (!m_Pooled || m_Pooled.IsDeleted())
			return false;

		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(m_Pooled.FindComponent(InventoryItemComponent));
		if (!itemComponent)
			return false;

		m_Weapon = itemComponent.CreatePreviewEntity(m_Core.GetWorld(), GRSA_StageCore.CAMERA);
		if (!m_Weapon)
		{
			GRSA_Log.Warn("WeaponStage: preview clone failed");
			return false;
		}

		ApplyRestPose();

		//! Preview clones can strip storages; hardpoint enumeration falls back to the pooled
		//! entity, whose local slot offsets are identical.
		m_SlotSource = m_Weapon;
		if (!m_Weapon.FindComponent(WeaponAttachmentsStorageComponent))
			m_SlotSource = m_Pooled;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The pooled entity is shared with every row thumbnail — draft parts are stripped back to
	//! factory state on release or they leak into later renders.
	protected void ReleaseWeapon()
	{
		DeleteRenderedWeapon();

		if (m_Pooled && !m_Pooled.IsDeleted())
		{
			array<ResourceName> factory = {};
			GRSA_ItemIntel.GetDefaultAttachments(m_WeaponPrefab, factory);
			GRSA_PreviewDress.SyncWeaponAttachmentList(m_Pooled, factory, m_Pooled.GetWorld());
		}

		m_Pooled = null;
		m_SlotSource = null;
		m_WeaponPrefab = ResourceName.Empty;
		m_sSyncedSignature = string.Empty;
		m_fBoundsDiag = 0;
		m_fFocusYaw = 0;
		m_fFocusYawTarget = 0;
		m_fFocusPitch = 0;
		m_fFocusPitchTarget = 0;
		m_bHasFocus = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void DeleteRenderedWeapon()
	{
		IEntity weapon = m_Weapon;
		m_Weapon = null;
		m_SlotSource = null;
		GRSA_PreviewDress.DeleteLocalHierarchy(weapon);
	}

	//------------------------------------------------------------------------------------------------
	//! Clears the bench (no draft weapon) — the studio and the resident soldier stay up.
	void ClearStage()
	{
		ReleaseWeapon();
	}

	//------------------------------------------------------------------------------------------------
	//! Bench-station camera contract: home pose, zoom range and pan bubble sized from the staged
	//! weapon's bounds (bare-bench framing when nothing is staged), then a glide (or snap) home.
	void FrameStation(bool snap = false)
	{
		if (!m_Core.IsAlive())
			return;

		m_Core.SetSubjectSpin(true);
		m_fSpinYaw = 0;
		m_fFocusYaw = 0;
		m_fFocusYawTarget = 0;
		m_fFocusPitch = 0;
		m_fFocusPitchTarget = 0;
		m_bHasFocus = false;
		ApplyRestPose(true);
		RefreshStationContract();
		m_Core.GoHome(snap);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshStationContract()
	{
		if (!m_Core.IsAlive())
			return;

		vector look = m_vRestCenter;
		float dist = m_fBoundsDiag * HOME_DISTANCE_SCALE;
		if (m_fBoundsDiag <= 0)
		{
			look = BENCH_LOOK;
			dist = BENCH_EMPTY_DIST;
			m_Core.SetZoomRange(EMPTY_ZOOM_MIN, EMPTY_ZOOM_MAX);
		}
		else
		{
			m_Core.SetZoomRange(m_fBoundsDiag * ZOOM_MIN_SCALE, m_fBoundsDiag * ZOOM_MAX_SCALE);
		}

		m_Core.SetPanBounds(look, PAN_RANGE, PAN_MIN_Y, PAN_MAX_Y);
		m_Core.SetHome(HOME_ANGLES, look, dist);
	}

	//------------------------------------------------------------------------------------------------
	//! Hero pose framing the whole weapon; the pose every deselection glides back to.
	void GoHome(bool snap = false)
	{
		m_bHasFocus = false;
		m_fFocusYawTarget = 0;
		m_fFocusPitchTarget = 0;
		m_Core.GoHome(snap);
	}

	//------------------------------------------------------------------------------------------------
	//! Glides in on a hardpoint while keeping the current viewing angle.
	void FocusPoint(vector localPos)
	{
		if (!m_Weapon)
			return;

		vector center = m_vRestCenterLocal;
		float span = Math.Max(m_fBoundsDiag, 0.1);
		m_fFocusYawTarget = Math.Clamp((localPos[2] - center[2]) / span * FOCUS_YAW_SCALE, -FOCUS_YAW_MAX, FOCUS_YAW_MAX);
		m_fFocusPitchTarget = Math.Clamp((localPos[1] - center[1]) / span * FOCUS_PITCH_SCALE, -FOCUS_PITCH_MAX, FOCUS_PITCH_MAX);
		m_vFocusLocal = localPos;
		m_fFocusDist = m_fBoundsDiag * FOCUS_DISTANCE_SCALE;
		m_bHasFocus = true;
		m_Core.FocusOn(m_Weapon.CoordToParent(localPos), m_fFocusDist);
	}

	//! Widget-local reference position of a weapon-local point through the stage camera.
	bool ProjectPoint(vector localPos, out vector screenPos)
	{
		if (!m_Weapon)
			return false;

		return m_Core.ProjectWorldPoint(m_Weapon.CoordToParent(localPos), screenPos);
	}

	//------------------------------------------------------------------------------------------------
	//! The hub owns the world; this only detaches the host from the session.
	void Destroy()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Remove(OnDraftChanged);

		if (m_Core)
			m_Core.m_OnSubjectSpin.Remove(OnSubjectSpin);

		ReleaseWeapon();
		m_Core = null;
	}
}
