//! Weapon host for the shared studio: resolves the pooled weapon, clones it onto the bench in a
//! fixed hero pose, and keeps the draft's attachment set synced onto the clone â€” even while
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
	protected static ref map<ResourceName, vector> s_mShoulderContacts;
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

	//! Borrowed from the owning hub â€” a strong ref here could root the world through the draft
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
	//! Fronts the studio through this render node even before any weapon is staged â€” an empty
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

		//! Preview cloning supplies the materials needed for rendering in the private world.
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
	//! whose signature matches the staged build is a no-op â€” the screen and the draft listener
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
	//! Attachments the engine refused to mount during the last sync â€” the blocked-slot signal
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
	//! draft â€” attachments resync, and a weapon dropped from the draft clears the bench.
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
		PositionOnStand(item, m_ArmorStand, helmet, homeScale, ypr);
	}

	static void PositionOnStand(notnull IEntity item, notnull IEntity stand, bool helmet, float homeScale, vector ypr)
	{
		DCO_EGearRackSlot kind = DCO_EGearRackSlot.VEST;
		if (helmet) kind = DCO_EGearRackSlot.HELMET;
		PositionGearOnStand(item, stand, kind, homeScale, ypr);
	}

	static void PositionGearOnStand(notnull IEntity item, notnull IEntity stand, DCO_EGearRackSlot kind, float homeScale, vector ypr, bool xl = false)
	{
		if (kind == DCO_EGearRackSlot.PRIMARY)
		{
			PositionRifleOnStand(item, stand, homeScale);
			return;
		}
		vector mins, maxs;
		item.GetBounds(mins, maxs);
		vector itemSupport = Vector((mins[0] + maxs[0]) * 0.5, maxs[1], (mins[2] + maxs[2]) * 0.5);
		if (kind == DCO_EGearRackSlot.VEST)
			itemSupport = VestSupportPoint(item);
		float bottom = mins[1];
		if (kind == DCO_EGearRackSlot.VEST)
			bottom = LowestGearPoint(item, item);
		vector leftShoulder, rightShoulder;
		bool reverseMesh;
		if (ShoulderFrame(item, leftShoulder, rightShoulder))
		{
			vector across = rightShoulder - leftShoulder;
			// Imported worn meshes can face backwards relative to the stand's local axes.
			reverseMesh = across[0] < 0;
		}
		item.SetYawPitchRoll(ypr);
		item.SetScale(homeScale);
		bool helmet = kind == DCO_EGearRackSlot.HELMET;
		if (reverseMesh != (helmet && !xl))
		{
			// Turn around the stand's local up axis, including on a tilted stand.
			vector transform[4];
			item.GetTransform(transform);
			transform[0] = -transform[0];
			transform[2] = -transform[2];
			item.SetTransform(transform);
		}
		float supportHeight = STAND_SHOULDER_Y + REST_LIFT;
		if (helmet) supportHeight = STAND_CROWN_Y + HELMET_SHELL_CLEARANCE;
		if (xl)
		{
			supportHeight = 1.048 + REST_LIFT;
			if (helmet) supportHeight = 1.3462 + HELMET_SHELL_CLEARANCE;
			if (kind == DCO_EGearRackSlot.BELT)
			{
				supportHeight = 0.413;
				itemSupport[1] = (mins[1] + maxs[1]) * 0.5;
				// Full harnesses use the waist joint; compact belts seat by the band itself.
				Animation animation = item.GetAnimation();
				vector waist[4];
				if (maxs[1] > leftShoulder[1] && leftShoulder[1] > mins[1]
					&& animation && animation.GetBoneIndex("Spine2") != -1
					&& animation.GetBoneMatrix(animation.GetBoneIndex("Spine2"), waist)
					&& waist[3][1] >= mins[1] && waist[3][1] <= maxs[1])
					itemSupport = waist[3];
			}
		}
		if (!helmet && kind != DCO_EGearRackSlot.BELT && maxs[1] > mins[1])
		{
			float availableHeight = Math.Max(supportHeight * stand.GetScale() - REST_LIFT, 0.001);
			item.SetScale(Math.Min(homeScale, availableHeight / Math.Max(itemSupport[1] - bottom, 0.001)));
		}
		vector pos = stand.CoordToParent(Vector(0, supportHeight, 0));
		pos -= item.VectorToParent(itemSupport);
		item.SetOrigin(pos);
		item.Update();
	}

	static void PositionBackPanel(notnull IEntity panel, notnull IEntity vest)
	{
		vector pose[4];
		vest.GetTransform(pose);
		panel.SetTransform(pose);
		Animation panelAnimation = panel.GetAnimation();
		Animation vestAnimation = vest.GetAnimation();
		vector panelBone[4], vestBone[4];
		// Independent body slots share the wearer's spine frame, including imported mesh offsets.
		if (panelAnimation && vestAnimation
			&& panelAnimation.GetBoneIndex("Spine5") != -1 && vestAnimation.GetBoneIndex("Spine5") != -1
			&& panelAnimation.GetBoneMatrix(panelAnimation.GetBoneIndex("Spine5"), panelBone)
			&& vestAnimation.GetBoneMatrix(vestAnimation.GetBoneIndex("Spine5"), vestBone))
			panel.SetOrigin(vest.CoordToParent(vestBone[3]) - panel.VectorToParent(panelBone[3]));
		else
		{
			vector panelMin, panelMax, vestMin, vestMax;
			panel.GetBounds(panelMin, panelMax);
			vest.GetBounds(vestMin, vestMax);
			vector contact = Vector((panelMin[0] + panelMax[0]) * 0.5, panelMax[1], panelMax[2]);
			vector seat = Vector((vestMin[0] + vestMax[0]) * 0.5, VestSupportPoint(vest)[1] - 0.06, vestMin[2]);
			panel.SetOrigin(vest.CoordToParent(seat) - panel.VectorToParent(contact));
		}
		panel.Update();
	}

	protected static void PositionRifleOnStand(notnull IEntity rifle, notnull IEntity stand, float scale)
	{
		vector local[4], standPose[4], pose[4];
		local[2] = Vector(-0.20, 0.975, 0).Normalized();
		local[0] = Vector(local[2][1], -local[2][0], 0).Normalized();
		local[1] = "0 0 -1";
		vector minimum, maximum, standMin, standMax;
		rifle.GetBounds(minimum, maximum);
		stand.GetBounds(standMin, standMax);
		float bottom = 1e10;
		for (int corner = 0; corner < 8; corner++)
		{
			vector point = minimum;
			if (corner & 1) point[0] = maximum[0];
			if (corner & 2) point[1] = maximum[1];
			if (corner & 4) point[2] = maximum[2];
			float height = local[0][1] * point[0] + local[1][1] * point[1] + local[2][1] * point[2];
			bottom = Math.Min(bottom, height);
		}
		float relativeScale = scale / Math.Max(stand.GetScale(), 0.001);
		local[0] = local[0] * relativeScale;
		local[1] = local[1] * relativeScale;
		local[2] = local[2] * relativeScale;
		// The butt rests on the base and the barrel leans toward the cross's upright.
		local[3] = Vector(standMax[0] * 0.85, standMin[1] + 0.02 - bottom * relativeScale, 0.10);
		stand.GetTransform(standPose);
		Math3D.MatrixMultiply4(standPose, local, pose);
		rifle.SetTransform(pose);
		rifle.Update();
	}

	//------------------------------------------------------------------------------------------------
	//! Contact belongs to the shoulder mesh; pouches, collars and hanging tools cannot anchor the vest.
	protected static vector VestSupportPoint(notnull IEntity item)
	{
		IEntity shoulder = item;
		float highest = -1e10;
		FindShoulderMesh(item, item, shoulder, highest);
		if (!shoulder.GetVObject()) return vector.Zero;
		ResourceName mesh = shoulder.GetVObject().GetResourceName();
		if (!s_mShoulderContacts) s_mShoulderContacts = new map<ResourceName, vector>();
		vector contact;
		if (!s_mShoulderContacts.Find(mesh, contact))
		{
			contact = MeasureShoulderContact(shoulder);
			s_mShoulderContacts.Set(mesh, contact);
		}
		return item.CoordToLocal(shoulder.CoordToParent(contact));
	}

	protected static bool ShoulderFrame(IEntity part, out vector left, out vector right)
	{
		Animation animation = part.GetAnimation();
		if (!animation) return false;
		TNodeId leftBone = animation.GetBoneIndex("LeftShoulder");
		TNodeId rightBone = animation.GetBoneIndex("RightShoulder");
		vector frame[4];
		if (leftBone == -1 || rightBone == -1 || !animation.GetBoneMatrix(leftBone, frame)) return false;
		left = frame[3];
		if (!animation.GetBoneMatrix(rightBone, frame)) return false;
		right = frame[3];
		TNodeId arm = animation.GetBoneIndex("LeftArm");
		if (arm != -1 && animation.GetBoneMatrix(arm, frame)) left = vector.Lerp(left, frame[3], 0.6);
		arm = animation.GetBoneIndex("RightArm");
		if (arm != -1 && animation.GetBoneMatrix(arm, frame)) right = vector.Lerp(right, frame[3], 0.6);
		return true;
	}

	protected static void FindShoulderMesh(IEntity root, IEntity part, inout IEntity best, inout float highest)
	{
		vector left, right, mins, maxs;
		part.GetBounds(mins, maxs);
		if (HasGearVolume(part, mins, maxs) && ShoulderFrame(part, left, right)
			&& mins[0] <= Math.Min(left[0], right[0]) && maxs[0] >= Math.Max(left[0], right[0])
			&& mins[1] <= left[1] && maxs[1] > left[1])
		{
			vector top = root.CoordToLocal(part.CoordToParent(Vector(0, maxs[1], 0)));
			if (top[1] > highest)
			{
				highest = top[1];
				best = part;
			}
			// Complete carriers own their support; auxiliary armour must not replace it.
			if (part == root) return;
		}
		IEntity child = part.GetChildren();
		while (child)
		{
			FindShoulderMesh(root, child, best, highest);
			child = child.GetSibling();
		}
	}

	protected static vector MeasureShoulderContact(notnull IEntity part)
	{
		vector mins, maxs, left, right;
		part.GetBounds(mins, maxs);
		if (!ShoulderFrame(part, left, right))
		{
			left = vector.Lerp(mins, maxs, 0.75);
			right = left;
			left[0] = Math.Lerp(mins[0], maxs[0], 0.2);
			right[0] = Math.Lerp(mins[0], maxs[0], 0.8);
		}
		vector contact = (left + right) * 0.5;
		// Visual-only straps seat below their curved crown, relative to their own shoulder span.
		contact[1] = Math.Lerp(contact[1], maxs[1], 0.75);
		Animation animation = part.GetAnimation();
		vector neck[4];
		if (animation && animation.GetBoneIndex("Neck1") != -1
			&& animation.GetBoneMatrix(animation.GetBoneIndex("Neck1"), neck))
		{
			float shoulderY = (left[1] + right[1]) * 0.5;
			// A raised collar cannot move the shoulder seat above the anatomical shoulder region.
			if (neck[3][1] > shoulderY)
				contact[1] = Math.Min(contact[1], neck[3][1] * 2 - shoulderY);
		}
		Physics body = part.GetPhysics();
		bool ownedBody = !body;
		MeshObject mesh = part.GetVObject().ToMeshObject();
		if (ownedBody && mesh && mesh.GetNumGeoms() > 0)
			body = Physics.CreateStatic(part, 0xffffffff);
		if (!body) return contact;
		vector leftHit, rightHit;
		bool hitLeft = TraceShoulder(part, left, mins, maxs, leftHit);
		bool hitRight = TraceShoulder(part, right, mins, maxs, rightHit);
		if (ownedBody) body.Destroy();
		if (hitLeft && hitRight) contact = (leftHit + rightHit) * 0.5;
		return contact;
	}

	protected static bool TraceShoulder(IEntity part, vector shoulder, vector mins, vector maxs, out vector contact)
	{
		bool found;
		float highest = -1e10;
		for (int depth = -2; depth <= 2; depth++)
		{
			float z = shoulder[2] + depth * (maxs[2] - mins[2]) * 0.1;
			TraceParam trace = new TraceParam();
			trace.Start = part.CoordToParent(Vector(shoulder[0], maxs[1] + 0.1, z));
			trace.End = part.CoordToParent(Vector(shoulder[0], shoulder[1], z));
			trace.Flags = TraceFlags.ENTS;
			trace.TargetLayers = 0xffffffff;
			trace.Include = part;
			float fraction = part.GetWorld().TraceMove(trace, null);
			if (fraction <= 0 || fraction >= 1 || trace.TraceEnt != part) continue;
			vector hit = part.CoordToLocal(vector.Lerp(trace.Start, trace.End, fraction));
			if (hit[1] <= highest) continue;
			highest = hit[1];
			contact = hit;
			found = true;
		}
		return found;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasGearVolume(IEntity part, vector mins, vector maxs)
	{
		// Flat damage-plate helpers do not describe the worn item's visible extent.
		return part.GetVObject() && maxs[0] - mins[0] > 0.0001
			&& maxs[1] - mins[1] > 0.0001 && maxs[2] - mins[2] > 0.0001;
	}

	//! Hanging pouches and tools must clear the base without moving the shoulder support.
	protected static float LowestGearPoint(notnull IEntity item, notnull IEntity part)
	{
		vector mins, maxs;
		part.GetBounds(mins, maxs);
		float bottom = 1e10;
		if (HasGearVolume(part, mins, maxs))
		{
			for (int corner = 0; corner < 8; corner++)
			{
				vector point = mins;
				for (int axis = 0; axis < 3; axis++)
					if (corner & (1 << axis)) point[axis] = maxs[axis];
				point = item.CoordToLocal(part.CoordToParent(point));
				bottom = Math.Min(bottom, point[1]);
			}
		}
		IEntity child = part.GetChildren();
		while (child)
		{
			bottom = Math.Min(bottom, LowestGearPoint(item, child));
			child = child.GetSibling();
		}
		return bottom;
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
	//! The pooled entity is shared with every row thumbnail â€” draft parts are stripped back to
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
	//! Clears the bench (no draft weapon) â€” the studio and the resident soldier stay up.
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
