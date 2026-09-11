//! Weapon host for the shared studio: resolves the pooled weapon, clones it onto the bench in a
//! fixed hero pose, and keeps the draft's attachment set synced onto the clone — even while
//! another tab fronts the studio, so the bench in the backdrop never shows a stale build. The
//! shared stage core owns the world, environment rig, dust and the glide camera; this class
//! owns what stands on the bench and the bench station's framing.
class BIA_WeaponStage
{
	protected static const float HOME_DISTANCE_SCALE = 1.15;
	protected static const float FOCUS_DISTANCE_SCALE = 0.42;
	protected static const float STAND_FOCUS_DISTANCE_SCALE = 0.82;
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
	//! Shoulder support height in the metre-scale stand mesh.
	protected static const float STAND_SHOULDER_Y = 0.514;
	protected static const float STAND_VEST_LIFT = 0.035;
	protected static const float STAND_CROWN_Y = 0.7493;
	//! Outer helmet shell sits just above the supporting dome.
	protected static const float HELMET_SHELL_CLEARANCE = 0.015;
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
	protected BIA_StageCore m_Core;
	protected ref array<ResourceName> m_aUnplaced = {};
	protected RenderTargetWidget m_wTarget;
	protected IEntity m_Weapon;
	protected IEntity m_ArmorStand;
	protected bool m_bItemOnStand;
	protected bool m_bVestOnStand;
	protected bool m_bHelmetOnStand;
	protected IEntity m_CompanionHelmet;
	protected string m_sCompanionHelmetSignature;
	protected float m_fCompanionHelmetHomeScale = 1;
	protected vector m_vStandHomeTransform[4];
	protected vector m_vStandHomeAngles;
	protected IEntity m_Pooled;
	protected IEntity m_SlotSource;
	protected ResourceName m_WeaponPrefab;
	protected int m_iDraftClothingSlot = -1;
	protected string m_sSyncedSignature;
	protected vector m_vRestCenter;
	protected vector m_vRestCenterLocal;
	protected float m_fBoundsDiag;
	protected float m_fItemHomeScale = 1;
	protected float m_fSpinYaw;
	protected float m_fFocusYaw;
	protected float m_fFocusYawTarget;
	protected float m_fFocusPitch;
	protected float m_fFocusPitchTarget;
	protected vector m_vFocusLocal;
	protected float m_fFocusDist;
	protected bool m_bHasFocus;

	//------------------------------------------------------------------------------------------------
	void BIA_WeaponStage(notnull BIA_StageCore core)
	{
		m_Core = core;
		core.m_OnSubjectSpin.Insert(OnSubjectSpin);

		BIA_DraftService service = BIA_DraftService.Get();
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
			BIA_Log.Error("WeaponStage: no render widget available, stage cannot draw");
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
		if (!m_Core.EnsureWorld("BIA_Stage"))
			return;

		m_wTarget = target;
		m_Core.BindTarget(target);
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns (or reuses) the weapon resting on the bench and binds the stage world to the render
	//! widget. Returns the live weapon entity for slot enumeration.
	IEntity ShowWeapon(ResourceName prefab, notnull RenderTargetWidget target)
	{
		if (prefab.IsEmpty() || !m_Core.EnsureWorld("BIA_Stage"))
			return null;

		m_wTarget = target;
		m_Core.BindTarget(target);

		if (m_Weapon && !m_Weapon.IsDeleted() && m_WeaponPrefab == prefab)
			return SlotSource();

		ReleaseWeapon();

		//! Both working own-world previews (the base inspect screen and the vanilla-derived
		//! editors) put the item in through the preview pipeline — a raw prefab spawn renders
		//! black in a runtime world. Resolve the pooled entity, clone it into the stage.
		ItemPreviewManagerEntity manager = BIA_ItemIntel.GetPreviewManager();
		if (!manager)
			return null;

		IEntity pooled = manager.ResolvePreviewEntityForPrefab(prefab);
		if (!pooled)
		{
			BIA_Log.Warn("WeaponStage: pooled weapon failed to resolve");
			return null;
		}

		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(pooled.FindComponent(InventoryItemComponent));
		if (!itemComponent)
		{
			BIA_Log.Warn("WeaponStage: weapon has no InventoryItemComponent, cannot stage");
			return null;
		}

		m_Pooled = pooled;
		m_WeaponPrefab = prefab;
		BindArmorStand();
		if (!RecloneWeapon())
		{
			ReleaseWeapon();
			return null;
		}
		RefreshCompanionHelmet();

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
	//! Tracks a clothing draft slot while that item is staged on the bench.
	void SetDraftClothingSlot(int clothingSlot)
	{
		if (m_iDraftClothingSlot == clothingSlot)
			return;

		ReleaseWeapon();
		m_iDraftClothingSlot = clothingSlot;
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
		BIA_PreviewDress.SyncWeaponAttachmentList(m_Pooled, attachments, m_Pooled.GetWorld(), pins, m_aUnplaced);
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
			if (m_bItemOnStand) m_fFocusDist = m_fBoundsDiag * STAND_FOCUS_DISTANCE_SCALE;
			RefreshFocusedCamera();
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

		BIA_DraftService service = BIA_DraftService.Get();
		if (!service || !service.m_Draft)
			return;

		if (m_iDraftClothingSlot >= 0)
		{
			BIA_KitClothing clothing = service.m_Draft.FindClothing(m_iDraftClothingSlot);
			if (!clothing || clothing.m_Prefab != m_WeaponPrefab)
			{
				ClearStage();
				return;
			}

			clothing.EnsurePins();
			SyncAttachments(clothing.m_aAttachments, clothing.m_aAttachmentSlots);
			if (RefreshCompanionHelmet())
			{
				ApplyRestPose(true);
				RefreshStationContract();
				RefreshFocusedCamera();
			}
			return;
		}

		BIA_KitWeapon match;
		foreach (BIA_KitWeapon weapon : service.m_Draft.m_aWeapons)
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

		if (m_bItemOnStand && m_ArmorStand)
		{
			ApplyStandRestPose(captureHomeBounds);
			return;
		}

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
	protected void BindArmorStand()
	{
		string area = BIA_ItemIntel.GetClothAreaType(m_WeaponPrefab);
		m_bHelmetOnStand = m_iDraftClothingSlot >= 0 && area.Contains("HeadCover");
		m_bVestOnStand = m_iDraftClothingSlot >= 0 && area.Contains("Vest");
		m_bItemOnStand = m_bVestOnStand || m_bHelmetOnStand;
		if (!m_bItemOnStand)
			return;

		m_ArmorStand = m_Core.GetWorld().FindEntityByName("BIA_ArmorStand");
		if (!m_ArmorStand)
		{
			m_bItemOnStand = false;
			m_bVestOnStand = false;
			m_bHelmetOnStand = false;
			BIA_Log.Warn("WeaponStage: BIA_ArmorStand is missing from the environment");
			return;
		}

		m_ArmorStand.GetTransform(m_vStandHomeTransform);
		m_vStandHomeAngles = m_ArmorStand.GetYawPitchRoll();
		m_ArmorStand.SetFlags(EntityFlags.VISIBLE, true);
	}

	//------------------------------------------------------------------------------------------------
	//! The body's own bounds keep attachments from changing its support point.
	protected void ApplyStandRestPose(bool captureHomeBounds)
	{
		vector ypr = m_vStandHomeAngles;
		ypr[0] = ypr[0] + m_fSpinYaw + m_fFocusYaw;
		m_ArmorStand.SetTransform(m_vStandHomeTransform);
		m_ArmorStand.SetYawPitchRoll(ypr);
		m_ArmorStand.Update();
		ApplySupportedItemPose(m_Weapon, m_bHelmetOnStand, m_fItemHomeScale, ypr);
		if (m_CompanionHelmet)
			ApplySupportedItemPose(m_CompanionHelmet, true, m_fCompanionHelmetHomeScale, ypr);

		if (captureHomeBounds)
		{
			vector mins, maxs;
			SCR_Global.GetWorldBoundsWithChildren(m_Weapon, mins, maxs);
			vector standMins, standMaxs;
			SCR_Global.GetWorldBoundsWithChildren(m_ArmorStand, standMins, standMaxs);
			for (int axis = 0; axis < 3; axis++)
			{
				mins[axis] = Math.Min(mins[axis], standMins[axis]);
				maxs[axis] = Math.Max(maxs[axis], standMaxs[axis]);
			}
			if (m_CompanionHelmet)
			{
				vector helmetMins, helmetMaxs;
				SCR_Global.GetWorldBoundsWithChildren(m_CompanionHelmet, helmetMins, helmetMaxs);
				for (int axis = 0; axis < 3; axis++)
				{
					mins[axis] = Math.Min(mins[axis], helmetMins[axis]);
					maxs[axis] = Math.Max(maxs[axis], helmetMaxs[axis]);
				}
			}
			m_vRestCenter = (mins + maxs) * 0.5;
			m_vRestCenterLocal = m_Weapon.CoordToLocal(m_vRestCenter);
			m_fBoundsDiag = Math.Max(vector.Distance(mins, maxs), 0.1);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplySupportedItemPose(notnull IEntity item, bool helmet, float homeScale, vector ypr)
	{
		item.SetYawPitchRoll(ypr);
		item.SetScale(homeScale);
		vector mins, maxs;
		item.GetBounds(mins, maxs);
		vector itemSupport = Vector((mins[0] + maxs[0]) * 0.5, maxs[1], (mins[2] + maxs[2]) * 0.5);
		float supportHeight = STAND_SHOULDER_Y + STAND_VEST_LIFT;
		if (helmet)
			supportHeight = STAND_CROWN_Y + HELMET_SHELL_CLEARANCE;
		else if (maxs[1] > mins[1])
		{
			float availableHeight = Math.Max(supportHeight * m_ArmorStand.GetScale() - REST_LIFT, 0.001);
			item.SetScale(Math.Min(homeScale, availableHeight / (maxs[1] - mins[1])));
		}

		vector pos = m_ArmorStand.CoordToParent(Vector(0, supportHeight, 0));
		pos -= item.VectorToParent(itemSupport);
		item.SetOrigin(pos);
		item.Update();
	}

	//------------------------------------------------------------------------------------------------
	//! A vest edit keeps the draft helmet on the same private stand without involving live inventory.
	protected bool RefreshCompanionHelmet()
	{
		BIA_KitClothing helmet;
		BIA_DraftService service = BIA_DraftService.Get();
		if (m_bVestOnStand && service && service.m_Draft)
		{
			foreach (BIA_KitClothing clothing : service.m_Draft.m_aClothings)
			{
				if (clothing && !clothing.m_Prefab.IsEmpty() && IsHeadCoverClothing(clothing.m_Prefab))
				{
					helmet = clothing;
					break;
				}
			}
		}

		string signature;
		if (helmet)
		{
			helmet.EnsurePins();
			signature = helmet.m_Prefab;
			foreach (int i, ResourceName attachment : helmet.m_aAttachments)
			{
				int pin = -1;
				if (i < helmet.m_aAttachmentSlots.Count()) pin = helmet.m_aAttachmentSlots[i];
				signature += attachment + ":" + pin.ToString() + ";";
			}
		}

		if (signature == m_sCompanionHelmetSignature && (!helmet || m_CompanionHelmet))
			return false;

		DeleteCompanionHelmet();
		m_sCompanionHelmetSignature = signature;
		if (!helmet || !m_Core || !m_Core.IsAlive())
			return true;

		Resource resource = Resource.Load(helmet.m_Prefab);
		if (!resource || !resource.IsValid())
			return true;

		IEntity source = GetGame().SpawnEntityPrefabLocal(resource, m_Core.GetWorld());
		if (!source)
			return true;

		BIA_PreviewDress.SyncWeaponAttachmentList(source, helmet.m_aAttachments, m_Core.GetWorld(), helmet.m_aAttachmentSlots);
		InventoryItemComponent item = InventoryItemComponent.Cast(source.FindComponent(InventoryItemComponent));
		if (item)
			m_CompanionHelmet = item.CreatePreviewEntity(m_Core.GetWorld(), BIA_StageCore.CAMERA);
		BIA_PreviewDress.DeleteLocalHierarchy(source);
		if (!m_CompanionHelmet)
		{
			BIA_Log.Warn("WeaponStage: companion helmet preview failed");
			return true;
		}

		m_fCompanionHelmetHomeScale = m_CompanionHelmet.GetScale();
		if (m_ArmorStand)
			ApplySupportedItemPose(m_CompanionHelmet, true, m_fCompanionHelmetHomeScale, m_ArmorStand.GetYawPitchRoll());
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The preview-manager cache can be cold while a private stage is being built.
	protected bool IsHeadCoverClothing(ResourceName prefab)
	{
		if (BIA_ItemIntel.GetClothAreaType(prefab).Contains("HeadCover"))
			return true;
		if (!m_Core || !m_Core.IsAlive())
			return false;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return false;
		IEntity source = GetGame().SpawnEntityPrefabLocal(resource, m_Core.GetWorld());
		if (!source)
			return false;
		BaseLoadoutClothComponent cloth = BaseLoadoutClothComponent.Cast(source.FindComponent(BaseLoadoutClothComponent));
		string area;
		if (cloth && cloth.GetAreaType()) area = cloth.GetAreaType().Type().ToString();
		BIA_PreviewDress.DeleteLocalHierarchy(source);
		return area.Contains("HeadCover");
	}

	//! Manual rotation turns the active item and its support together.
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

		RefreshFocusedCamera();
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

		m_Weapon = itemComponent.CreatePreviewEntity(m_Core.GetWorld(), BIA_StageCore.CAMERA);
		if (!m_Weapon)
		{
			BIA_Log.Warn("WeaponStage: preview clone failed");
			return false;
		}
		m_fItemHomeScale = m_Weapon.GetScale();

		ApplyRestPose();

		//! Preview clones can strip storages; hardpoint enumeration falls back to the pooled
		//! entity, whose local slot offsets are identical.
		m_SlotSource = m_Weapon;
		if (!BIA_ItemIntel.AttachmentStorage(m_Weapon))
			m_SlotSource = m_Pooled;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The pooled entity is shared with every row thumbnail — draft parts are stripped back to
	//! factory state on release or they leak into later renders.
	protected void ReleaseWeapon()
	{
		DeleteCompanionHelmet();
		if (m_ArmorStand && !m_ArmorStand.IsDeleted())
		{
			m_ArmorStand.SetTransform(m_vStandHomeTransform);
			m_ArmorStand.Update();
			m_ArmorStand.ClearFlags(EntityFlags.VISIBLE, true);
		}
		m_ArmorStand = null;
		m_bItemOnStand = false;
		m_bVestOnStand = false;
		m_bHelmetOnStand = false;
		DeleteRenderedWeapon();

		if (m_Pooled && !m_Pooled.IsDeleted())
		{
			array<ResourceName> factory = {};
			array<int> factoryPins = {};
			BIA_ItemIntel.GetDefaultAttachments(m_WeaponPrefab, factory, factoryPins);
			BIA_PreviewDress.SyncWeaponAttachmentList(m_Pooled, factory, m_Pooled.GetWorld(), factoryPins);
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
	protected void DeleteCompanionHelmet()
	{
		IEntity helmet = m_CompanionHelmet;
		m_CompanionHelmet = null;
		m_sCompanionHelmetSignature = string.Empty;
		m_fCompanionHelmetHomeScale = 1;
		BIA_PreviewDress.DeleteLocalHierarchy(helmet);
	}

	//------------------------------------------------------------------------------------------------
	protected void DeleteRenderedWeapon()
	{
		IEntity weapon = m_Weapon;
		m_Weapon = null;
		m_SlotSource = null;
		m_fItemHomeScale = 1;
		BIA_PreviewDress.DeleteLocalHierarchy(weapon);
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

		localPos = BoundedInspectionPoint(localPos);
		vector center = m_vRestCenterLocal;
		float span = Math.Max(m_fBoundsDiag, 0.1);
		m_fFocusYawTarget = Math.Clamp((localPos[2] - center[2]) / span * FOCUS_YAW_SCALE, -FOCUS_YAW_MAX, FOCUS_YAW_MAX);
		m_fFocusPitchTarget = Math.Clamp((localPos[1] - center[1]) / span * FOCUS_PITCH_SCALE, -FOCUS_PITCH_MAX, FOCUS_PITCH_MAX);
		if (m_bItemOnStand)
		{
			m_fFocusYawTarget = 0;
			m_fFocusPitchTarget = 0;
		}
		m_vFocusLocal = localPos;
		m_fFocusDist = m_fBoundsDiag * FOCUS_DISTANCE_SCALE;
		if (m_bItemOnStand) m_fFocusDist = m_fBoundsDiag * STAND_FOCUS_DISTANCE_SCALE;
		m_bHasFocus = true;
		RefreshFocusedCamera();
	}

	//------------------------------------------------------------------------------------------------
	//! Clothing mods can author inspection offsets outside their visible mesh; keep stand focus usable.
	protected vector BoundedInspectionPoint(vector localPos)
	{
		if (!m_bItemOnStand || !m_Weapon)
			return localPos;

		vector mins, maxs;
		m_Weapon.GetBounds(mins, maxs);
		for (int axis = 0; axis < 3; axis++)
			localPos[axis] = Math.Clamp(localPos[axis], mins[axis], maxs[axis]);
		return localPos;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshFocusedCamera()
	{
		if (m_bHasFocus && m_Weapon)
			m_Core.FocusOn(m_Weapon.CoordToParent(m_vFocusLocal), m_fFocusDist);
	}

	//! Widget-local reference position of a weapon-local point through the stage camera.
	bool ProjectPoint(vector localPos, out vector screenPos)
	{
		if (!m_Weapon)
			return false;

		localPos = BoundedInspectionPoint(localPos);
		return m_Core.ProjectWorldPoint(m_Weapon.CoordToParent(localPos), screenPos);
	}

	//------------------------------------------------------------------------------------------------
	//! The hub owns the world; this only detaches the host from the session.
	void Destroy()
	{
		BIA_DraftService service = BIA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Remove(OnDraftChanged);

		if (m_Core)
			m_Core.m_OnSubjectSpin.Remove(OnSubjectSpin);

		ReleaseWeapon();
		m_iDraftClothingSlot = -1;
		m_Core = null;
	}
}
