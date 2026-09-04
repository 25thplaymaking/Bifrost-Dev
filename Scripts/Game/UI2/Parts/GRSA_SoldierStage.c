//! Soldier host for the shared studio: an inventory-preview clone standing on the floor spot beside
//! the workstation set, dressed live from any kit. Card focus glides the camera onto the actual
//! equipped item, framed from its measured bounds — an empty slot falls back to the body region,
//! the region size still derived from the real character bounds, never from authored guesses.
//! The shared stage core owns the world and the camera; this class owns the character and the
//! soldier station's framing, and keeps the resident soldier current with the draft even while
//! another tab fronts the studio.
class GRSA_SoldierStage
{
	//! Standing spot on the open floor, far enough from the bench to keep the soldier camera's
	//! complete orbit clear of the workstation. The authored yaw turns the character toward home camera.
	//! The preview world does not pull the character down to the authored floor, so the spawn
	//! transform includes the floor height.
	protected static const vector CHAR_SPOT = "5 -0.549 -1.7";
	protected static const float CHAR_YAW = 75;
	protected static const vector HOME_ANGLES = "-120 -8 0";
	protected static const float HOME_LOOK_FRACTION = 0.55;
	//! A 38-degree vertical FOV needs ~1.45 character heights for an edge-to-edge fit. The
	//! additional margin keeps headwear and footwear comfortably inside the menu viewport.
	protected static const float HOME_DIST_PER_HEIGHT = 1.8;
	//! Frame distance per meter of target bounds diagonal; the vertical-FOV fit for the 38-degree
	//! stage camera is ~1.45 per meter, the margin keeps edges off the letterbox.
	protected static const float ITEM_FRAME_SCALE = 1.35;
	protected static const float ITEM_DIST_MIN = 0.45;
	protected static const float ITEM_ORBIT_MARGIN = 0.2;
	protected static const float REGION_FRAME_SCALE = 1.45;
	protected static const float ZOOM_MIN = 0.35;
	protected static const float ZOOM_MAX_PER_HEIGHT = 2.2;
	protected static const float PAN_RANGE = 1.2;
	protected static const float PAN_MIN_Y = -0.5;
	protected static const float PAN_MAX_Y = 2.0;
	protected static const float CHAR_HEIGHT_FALLBACK = 1.8;
	protected static const float BOUNDS_SLACK = 0.25;
	protected static const float ANIMATION_MAX_STEP = 0.05;
	protected static const ResourceName PREVIEW_GRAPH = "{7E131B0DEA83D762}anims/workspaces/player/player_inventory.agr";
	protected static const ResourceName PREVIEW_INSTANCE_UNARMED = "{F6304E0639C827E4}anims/workspaces/player/player_inventory_unarmed.asi";
	protected static const ResourceName PREVIEW_INSTANCE_RIFLE = "anims/workspaces/player/player_inventory_rifle.asi";
	protected static const ResourceName PREVIEW_INSTANCE_PISTOL = "anims/workspaces/player/player_inventory_pistol.asi";
	protected static const ResourceName PREVIEW_INSTANCE_LAUNCHER = "anims/workspaces/player/player_inventory_launcher.asi";
	protected static const ResourceName PREVIEW_INSTANCE_MACHINEGUN = "anims/workspaces/player/player_inventory_LMG.asi";
	protected static const ResourceName PREVIEW_INSTANCE_ONE_HAND = "anims/workspaces/player/player_inventory_1handed.asi";
	protected static const ResourceName PREVIEW_INSTANCE_HEAVY = "anims/workspaces/player/player_inventory_heavy.asi";

	//! Borrowed from the owning hub — a strong ref here could root the world through the draft
	//! service's static invoker if a subscription survives a hard teardown.
	protected GRSA_StageCore m_Core;
	protected RenderTargetWidget m_wTarget;
	protected IEntity m_PooledCharacter;
	protected IEntity m_Character;
	protected SCR_CharacterInventoryStorageComponent m_CharacterStorage;
	protected PreviewAnimationComponent m_PreviewAnimation;
	protected ResourceName m_CharacterPrefab;
	protected float m_fCharHeight;
	protected float m_fCharMinY;
	protected vector m_vCharCenter;
	protected bool m_bVisible;
	protected bool m_bHasPreviewWeapon;
	protected int m_iPreviewArmIK = -1;
	protected int m_iPreviewAimY = -1;
	protected int m_iPreviewAimX = -1;
	protected int m_iPreviewAimZ = -1;
	protected int m_iPreviewTransX = -1;
	protected int m_iPreviewTransY = -1;
	protected int m_iPreviewTransZ = -1;
	protected int m_iPreviewGearFB = -1;
	protected int m_iPreviewGearLR = -1;

	//------------------------------------------------------------------------------------------------
	void GRSA_SoldierStage(notnull GRSA_StageCore core)
	{
		m_Core = core;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Insert(OnDraftChanged);
	}

	//------------------------------------------------------------------------------------------------
	//! The render node of whichever tab is fronting the studio right now; rebinding is how a tab
	//! takes over the shared world's output.
	void Bind(notnull RenderTargetWidget target)
	{
		m_wTarget = target;
		if (m_Core.IsAlive())
			m_Core.BindTarget(target);
	}

	//------------------------------------------------------------------------------------------------
	//! Hidden tabs do not spend animation work until the soldier station fronts the studio again.
	void SetVisible(bool visible)
	{
		m_bVisible = visible;
	}

	//------------------------------------------------------------------------------------------------
	void Tick(float tDelta)
	{
		if (!m_bVisible || !m_Character || m_Character.IsDeleted() || !m_PreviewAnimation)
			return;

		float step = Math.Clamp(tDelta, 0, ANIMATION_MAX_STEP);
		ApplyPreviewGraphInputs();
		m_PreviewAnimation.UpdateFrameStep(m_Character, step);
	}

	//------------------------------------------------------------------------------------------------
	void RefreshFromDraft(notnull GRSA_DraftService service)
	{
		if (service.m_Draft)
			RefreshFromKit(service.m_Draft, service);
	}

	//------------------------------------------------------------------------------------------------
	//! Dresses the stage character in place from any kit; camera framing is FrameStation's job, so
	//! draft edits never yank the view.
	void RefreshFromKit(notnull GRSA_Kit kit, notnull GRSA_DraftService service)
	{
		GameEntity character = service.GetLocalCharacter();
		if (!character)
			return;

		ResourceName characterPrefab = SCR_ResourceNameUtils.GetPrefabName(character);
		if (characterPrefab.IsEmpty())
			return;

		bool createdWorld = !m_Core.IsAlive();
		if (!m_Core.EnsureWorld("GRSA_Stage"))
			return;
		if (createdWorld && m_wTarget)
			m_Core.BindTarget(m_wTarget);

		if (!EnsureCharacterSource(characterPrefab))
			return;

		DeleteRenderedCharacter();
		//! The preview manager tracks hierarchy changes; its pooled source must not be advanced directly.
		GRSA_PreviewDress.DiffDress(m_PooledCharacter, kit);
		if (!RecloneCharacter())
			return;

		RefreshCharacterBounds();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshCharacterBounds()
	{
		if (!m_Character)
			return;

		vector mins, maxs;
		m_Character.GetWorldBounds(mins, maxs);
		m_fCharHeight = maxs[1] - mins[1];
		if (m_fCharHeight <= 0)
		{
			m_fCharHeight = CHAR_HEIGHT_FALLBACK;
			m_fCharMinY = CHAR_SPOT[1];
			m_vCharCenter = CHAR_SPOT;
			m_vCharCenter[1] = m_fCharMinY + m_fCharHeight * 0.5;
		}
		else
		{
			m_fCharMinY = mins[1];
			m_vCharCenter = (mins + maxs) * 0.5;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Soldier-station camera contract: home pose, zoom range and pan bubble sized from the real
	//! character height, then a glide (or snap) home.
	void FrameStation(bool snap = false)
	{
		if (!m_Core.IsAlive())
			return;

		m_Core.SetSubjectSpin(false);

		float height = m_fCharHeight;
		if (height <= 0)
			height = CHAR_HEIGHT_FALLBACK;

		vector homeLook = m_vCharCenter;
		if (homeLook == vector.Zero)
		{
			homeLook = CHAR_SPOT;
			homeLook[1] = homeLook[1] + height * 0.5;
		}
		m_Core.SetHome(HOME_ANGLES, homeLook, height * HOME_DIST_PER_HEIGHT);
		m_Core.SetZoomRange(ZOOM_MIN, height * ZOOM_MAX_PER_HEIGHT);
		m_Core.SetPanBounds(homeLook, PAN_RANGE, PAN_MIN_Y, PAN_MAX_Y);
		m_Core.GoHome(snap);
	}

	//------------------------------------------------------------------------------------------------
	//! The resident soldier stays current with the draft even while another tab fronts the studio.
	protected void OnDraftChanged()
	{
		if (!m_Core.IsAlive())
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			RefreshFromDraft(service);
	}

	//------------------------------------------------------------------------------------------------
	protected bool EnsureCharacterSource(ResourceName characterPrefab)
	{
		if (m_PooledCharacter && !m_PooledCharacter.IsDeleted() && m_CharacterStorage && m_CharacterPrefab == characterPrefab)
			return true;

		ReleaseCharacter();

		ItemPreviewManagerEntity manager = GRSA_ItemIntel.GetPreviewManager();
		if (!manager)
			return false;

		m_PooledCharacter = manager.ResolvePreviewEntityForPrefab(characterPrefab);
		if (!m_PooledCharacter)
		{
			GRSA_Log.Warn("Soldier stage: pooled character failed to resolve");
			return false;
		}

		m_CharacterStorage = SCR_CharacterInventoryStorageComponent.Cast(m_PooledCharacter.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!m_CharacterStorage)
		{
			GRSA_Log.Warn("Soldier stage: character has no preview-capable inventory storage");
			m_PooledCharacter = null;
			return false;
		}

		m_CharacterPrefab = characterPrefab;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Character inventory storage owns the engine's animated preview prefab. Re-clone after every
	//! dress change because preview clones deliberately do not retain mutable inventory storages.
	protected bool RecloneCharacter()
	{
		DeleteRenderedCharacter();

		if (!m_CharacterStorage || !m_Core.IsAlive())
			return false;

		m_Character = m_CharacterStorage.CreatePreviewEntity(m_Core.GetWorld(), GRSA_StageCore.CAMERA);
		if (!m_Character)
		{
			GRSA_Log.Warn("Soldier stage: animated preview clone failed");
			return false;
		}

		m_Character.SetYawPitchRoll(Vector(CHAR_YAW, 0, 0));
		m_Character.SetOrigin(CHAR_SPOT);
		//! Propagate the placed clone transform through its worn and attached preview hierarchy.
		m_Character.Update();
		m_PreviewAnimation = PreviewAnimationComponent.Cast(m_Character.FindComponent(PreviewAnimationComponent));
		if (!m_PreviewAnimation)
		{
			GRSA_Log.Warn("Soldier stage: preview clone has no animation component");
			DeleteRenderedCharacter();
			return false;
		}

		ConfigurePreviewAnimationGraph();
		m_PreviewAnimation.UpdateFrameStep(m_Character, 0.001);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The engine's inventory graph is purpose-built for manually stepped preview clones and avoids
	//! the runtime movement, speech and facial state expected by the live-player graph.
	protected void ConfigurePreviewAnimationGraph()
	{
		if (!m_PreviewAnimation || !m_Character)
			return;

		IEntity weaponEntity = FindPreviewWeapon(m_PooledCharacter);
		if (!weaponEntity)
			weaponEntity = FindPreviewWeapon(m_Character);

		EWeaponType weaponType;
		BaseWeaponComponent weapon;
		if (weaponEntity)
			weapon = BaseWeaponComponent.Cast(weaponEntity.FindComponent(BaseWeaponComponent));
		m_bHasPreviewWeapon = weapon != null;
		if (weapon)
			weaponType = weapon.GetWeaponType();

		ResourceName instance = ResolvePreviewAnimationInstance(weaponType, m_bHasPreviewWeapon);
		m_PreviewAnimation.SetGraphResource(m_Character, PREVIEW_GRAPH, instance, "MasterControl");
		m_PreviewAnimation.UpdateFrameStep(m_Character, 0.001);

		m_iPreviewArmIK = m_PreviewAnimation.BindIntVariable("ArmIK");
		m_iPreviewAimY = m_PreviewAnimation.BindFloatVariable("AimY");
		m_iPreviewAimX = m_PreviewAnimation.BindFloatVariable("AimIKX");
		m_iPreviewAimZ = m_PreviewAnimation.BindFloatVariable("AimIKZ");
		m_iPreviewTransX = m_PreviewAnimation.BindFloatVariable("AimTransX");
		m_iPreviewTransY = m_PreviewAnimation.BindFloatVariable("AimTransY");
		m_iPreviewTransZ = m_PreviewAnimation.BindFloatVariable("AimTransZ");
		m_iPreviewGearFB = m_PreviewAnimation.BindFloatVariable("GearFB");
		m_iPreviewGearLR = m_PreviewAnimation.BindFloatVariable("GearLR");

		ResourceName weaponIK = ResolveWeaponIKPose(weaponEntity);
		if (!weaponIK.IsEmpty())
			m_PreviewAnimation.SetHandsIKPose(m_Character, weaponIK);
		m_PreviewAnimation.SetIkState(m_bHasPreviewWeapon, m_bHasPreviewWeapon);
		ApplyPreviewGraphInputs();
		m_PreviewAnimation.UpdateFrameStep(m_Character, 0.001);
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName ResolvePreviewAnimationInstance(EWeaponType weaponType, bool hasWeapon)
	{
		if (!hasWeapon)
			return PREVIEW_INSTANCE_UNARMED;

		switch (weaponType)
		{
			case EWeaponType.WT_HANDGUN:
				return PREVIEW_INSTANCE_PISTOL;
			case EWeaponType.WT_MACHINEGUN:
				return PREVIEW_INSTANCE_MACHINEGUN;
			case EWeaponType.WT_ROCKETLAUNCHER:
			case EWeaponType.WT_GRENADELAUNCHER:
				return PREVIEW_INSTANCE_LAUNCHER;
			case EWeaponType.WT_FRAGGRENADE:
			case EWeaponType.WT_SMOKEGRENADE:
				return PREVIEW_INSTANCE_ONE_HAND;
			case EWeaponType.WT_AUTOCANNON:
				return PREVIEW_INSTANCE_HEAVY;
		}

		return PREVIEW_INSTANCE_RIFLE;
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity FindPreviewWeapon(IEntity source)
	{
		if (!source)
			return null;

		BaseWeaponManagerComponent weaponManager = BaseWeaponManagerComponent.Cast(source.FindComponent(BaseWeaponManagerComponent));
		if (weaponManager)
		{
			BaseWeaponComponent currentWeapon = weaponManager.GetCurrentWeapon();
			if (currentWeapon)
				return currentWeapon.GetOwner();
		}

		EquipedWeaponStorageComponent weaponStorage = EquipedWeaponStorageComponent.Cast(source.FindComponent(EquipedWeaponStorageComponent));
		if (!weaponStorage)
			return null;

		int slotsCount = weaponStorage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = weaponStorage.GetSlot(i);
			if (!slot)
				continue;

			IEntity weaponEntity = slot.GetAttachedEntity();
			if (!weaponEntity)
				continue;

			if (weaponEntity.FindComponent(BaseWeaponComponent))
				return weaponEntity;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName ResolveWeaponIKPose(IEntity weaponEntity)
	{
		if (!weaponEntity)
			return ResourceName.Empty;

		ResourceName weaponPrefab = SCR_ResourceNameUtils.GetPrefabName(weaponEntity);
		if (weaponPrefab.IsEmpty())
			return ResourceName.Empty;

		Resource resource = Resource.Load(weaponPrefab);
		if (!resource || !resource.IsValid())
			return ResourceName.Empty;

		IEntitySource weaponSource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!weaponSource)
			return ResourceName.Empty;

		IEntityComponentSource itemSource = SCR_ComponentHelper.GetInventoryItemComponentSource(weaponSource);
		if (!itemSource)
			return ResourceName.Empty;

		BaseContainer attributes = itemSource.GetObject("Attributes");
		if (!attributes)
			return ResourceName.Empty;

		BaseContainer animationAttributes = attributes.GetObject("ItemAnimationAttributes");
		if (!animationAttributes)
			return ResourceName.Empty;

		ResourceName weaponIK;
		animationAttributes.Get("AnimationIKPose", weaponIK);
		return weaponIK;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyPreviewGraphInputs()
	{
		if (!m_PreviewAnimation)
			return;

		int armIK;
		if (m_bHasPreviewWeapon)
			armIK = 1;
		if (m_iPreviewArmIK >= 0)
			m_PreviewAnimation.SetIntVariable(m_iPreviewArmIK, armIK);

		if (m_iPreviewAimY >= 0)
			m_PreviewAnimation.SetFloatVariable(m_iPreviewAimY, 0);
		if (m_iPreviewAimX >= 0)
			m_PreviewAnimation.SetFloatVariable(m_iPreviewAimX, 0);
		if (m_iPreviewAimZ >= 0)
			m_PreviewAnimation.SetFloatVariable(m_iPreviewAimZ, 0);
		if (m_iPreviewTransX >= 0)
			m_PreviewAnimation.SetFloatVariable(m_iPreviewTransX, 0);
		if (m_iPreviewTransY >= 0)
			m_PreviewAnimation.SetFloatVariable(m_iPreviewTransY, 0);
		if (m_iPreviewTransZ >= 0)
			m_PreviewAnimation.SetFloatVariable(m_iPreviewTransZ, 0);
		if (m_iPreviewGearFB >= 0)
			m_PreviewAnimation.SetFloatVariable(m_iPreviewGearFB, 0);
		if (m_iPreviewGearLR >= 0)
			m_PreviewAnimation.SetFloatVariable(m_iPreviewGearLR, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! The auto-zoom: glide onto the focused card's target. An equipped item is framed from its own
	//! measured bounds; an empty slot frames the matching body region sized from the character.
	void FocusCard(bool isWeapon, int weaponSlot, int clothingSlot, string area)
	{
		if (!m_Character)
			return;

		IEntity item = ResolveSlotEntity(isWeapon, weaponSlot, clothingSlot);
		if (item)
		{
			vector worldMins, worldMaxs;
			item.GetWorldBounds(worldMins, worldMaxs);
			float diag = vector.Distance(worldMins, worldMaxs);
			if (diag > 0.01)
			{
				//! A slot-attached item's CoordToParent lands in the character's space, not the
				//! world — frame from the world AABB instead, boxed to the character so bogus modded
				//! bounds cannot drag the look point off the soldier or the distance past home.
				vector center = (worldMins + worldMaxs) * 0.5;
				if (item.GetWorld() != m_Character.GetWorld() && m_PooledCharacter)
					center = m_Character.CoordToParent(m_PooledCharacter.CoordToLocal(center));

				vector charMins, charMaxs;
				m_Character.GetWorldBounds(charMins, charMaxs);
				center[0] = Math.Clamp(center[0], charMins[0] - BOUNDS_SLACK, charMaxs[0] + BOUNDS_SLACK);
				center[1] = Math.Clamp(center[1], charMins[1] - BOUNDS_SLACK, charMaxs[1] + BOUNDS_SLACK);
				center[2] = Math.Clamp(center[2], charMins[2] - BOUNDS_SLACK, charMaxs[2] + BOUNDS_SLACK);

				float height = m_fCharHeight;
				if (height <= 0)
					height = CHAR_HEIGHT_FALLBACK;

				float clearX = Math.Max(Math.AbsFloat(center[0] - charMins[0]), Math.AbsFloat(charMaxs[0] - center[0]));
				float clearZ = Math.Max(Math.AbsFloat(center[2] - charMins[2]), Math.AbsFloat(charMaxs[2] - center[2]));
				float orbitClearance = Math.Sqrt(clearX * clearX + clearZ * clearZ) + ITEM_ORBIT_MARGIN;
				float dist = Math.Clamp(Math.Max(diag * ITEM_FRAME_SCALE, orbitClearance), ITEM_DIST_MIN, height * HOME_DIST_PER_HEIGHT);
				m_Core.FocusOn(center, dist);
				return;
			}
		}

		FocusRegionFallback(isWeapon, area);
	}

	//------------------------------------------------------------------------------------------------
	void ClearFocus()
	{
		m_Core.GoHome();
	}

	//------------------------------------------------------------------------------------------------
	//! Mutable preview source for inspecting the runtime hardpoints on one worn item.
	IEntity GetClothingSource(int clothingSlot)
	{
		if (clothingSlot < 0)
			return null;

		IEntity source = m_PooledCharacter;
		if (!source)
			source = m_Character;
		if (!source)
			return null;

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(source.FindComponent(EquipedLoadoutStorageComponent));
		if (!loadoutStorage)
			return null;

		InventoryStorageSlot slot = loadoutStorage.GetSlot(clothingSlot);
		if (!slot)
			return null;

		return slot.GetAttachedEntity();
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity ResolveSlotEntity(bool isWeapon, int weaponSlot, int clothingSlot)
	{
		IEntity slotSource = m_Character;
		if (!slotSource)
			return null;

		if (isWeapon)
		{
			if (weaponSlot < 0)
				return null;

			EquipedWeaponStorageComponent weaponStorage = EquipedWeaponStorageComponent.Cast(slotSource.FindComponent(EquipedWeaponStorageComponent));
			if (!weaponStorage && m_PooledCharacter)
			{
				slotSource = m_PooledCharacter;
				weaponStorage = EquipedWeaponStorageComponent.Cast(slotSource.FindComponent(EquipedWeaponStorageComponent));
			}
			if (!weaponStorage)
				return null;

			InventoryStorageSlot slot = weaponStorage.GetSlot(weaponSlot);
			if (!slot)
				return null;

			return slot.GetAttachedEntity();
		}

		if (clothingSlot < 0)
			return null;

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(slotSource.FindComponent(EquipedLoadoutStorageComponent));
		if (!loadoutStorage && m_PooledCharacter)
		{
			slotSource = m_PooledCharacter;
			loadoutStorage = EquipedLoadoutStorageComponent.Cast(slotSource.FindComponent(EquipedLoadoutStorageComponent));
		}
		if (!loadoutStorage)
			return null;

		InventoryStorageSlot clothSlot = loadoutStorage.GetSlot(clothingSlot);
		if (!clothSlot)
			return null;

		return clothSlot.GetAttachedEntity();
	}

	//------------------------------------------------------------------------------------------------
	//! Body-region framing from the live character bounds: look height and framing span as
	//! fractions of the real height, so any character size frames correctly.
	protected void FocusRegionFallback(bool isWeapon, string area)
	{
		float lookFraction = HOME_LOOK_FRACTION;
		float spanFraction = 1.0;

		if (isWeapon || area.Contains("Jacket") || area.Contains("Vest") || area.Contains("Backpack") || area.Contains("Handwear") || area.Contains("Watch"))
		{
			lookFraction = 0.6;
			spanFraction = 0.55;
		}
		else if (area.Contains("HeadCover") || area.Contains("Googles"))
		{
			lookFraction = 0.86;
			spanFraction = 0.3;
		}
		else if (area.Contains("Pants"))
		{
			lookFraction = 0.28;
			spanFraction = 0.55;
		}
		else if (area.Contains("Boots"))
		{
			lookFraction = 0.05;
			spanFraction = 0.3;
		}
		else
		{
			ClearFocus();
			return;
		}

		vector look = m_vCharCenter;
		look[1] = m_fCharMinY + m_fCharHeight * lookFraction;
		m_Core.FocusOn(look, Math.Max(m_fCharHeight * spanFraction * REGION_FRAME_SCALE, ITEM_DIST_MIN));
	}

	//------------------------------------------------------------------------------------------------
	//! The hub owns the world; this only detaches the host from the session.
	void Destroy()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Remove(OnDraftChanged);

		ReleaseCharacter();
		m_fCharHeight = 0;
		m_fCharMinY = 0;
		m_vCharCenter = vector.Zero;
		m_wTarget = null;
		m_Core = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void ReleaseCharacter()
	{
		DeleteRenderedCharacter();

		m_PooledCharacter = null;
		m_CharacterStorage = null;
		m_CharacterPrefab = ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void DeleteRenderedCharacter()
	{
		m_PreviewAnimation = null;
		m_bHasPreviewWeapon = false;
		m_iPreviewArmIK = -1;
		m_iPreviewAimY = -1;
		m_iPreviewAimX = -1;
		m_iPreviewAimZ = -1;
		m_iPreviewTransX = -1;
		m_iPreviewTransY = -1;
		m_iPreviewTransZ = -1;
		m_iPreviewGearFB = -1;
		m_iPreviewGearLR = -1;
		IEntity character = m_Character;
		m_Character = null;
		GRSA_PreviewDress.DeleteLocalHierarchy(character);
	}
}
