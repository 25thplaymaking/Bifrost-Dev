class BIA_ArsenalRackProbe
{
	int passed;
	ref array<string> failures = {};
	ref array<string> details = {};
	void Expect(bool value, string label)
	{
		if (value) passed++;
		else failures.Insert(label);
	}
	void Run()
	{
		if (BIA_ShellMenu.IsArmoryOpen())
		{
			failures.Insert("Close Arsenal before running the isolated probe.");
			return;
		}
		CheckLayouts();
		if (!GetGame().InPlayMode() || !BIA_ItemIntel.GetPreviewManager() || !SCR_PlayerArsenalLoadout.ARSENALLOADOUT_COMPONENTS_TO_CHECK)
		{
			failures.Insert("Inventory checks require a fully initialized local play world; preview-world inventory operations do not establish transfer behavior.");
			return;
		}
		CheckInventory(GetGame().GetWorld());
		CheckXL(GetGame().GetWorld());
	}
	IEntity Spawn(BaseWorld world, ResourceName prefab)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			failures.Insert("Missing resource: " + prefab);
			return null;
		}
		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform[3] = "2736.82 77.53 1655.68";
		IEntity entity = GetGame().SpawnEntityPrefabLocal(resource, world, params);
		Expect(entity != null, "Spawn " + prefab);
		return entity;
	}
	void CheckInventory(BaseWorld world)
	{
		IEntity rackEntity = Spawn(world, "{D67E9B88BBDB4FE4}Prefabs/E_DCO_PlaceableArsenal.et");
		IEntity vest = Spawn(world, "{2835A0EA3B79E63E}Prefabs/Characters/Vests/Vest_ALICE/Variants/Vest_ALICE_rifleman.et");
		IEntity helmet = Spawn(world, "{B74A4FF0DD8BB116}Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01.et");
		IEntity mag = Spawn(world, "{D8F2CA92583B23D3}Prefabs/Weapons/Magazines/Magazine_556x45_STANAG_30rnd_M855_M856_Last_5Tracer.et");
		if (rackEntity && vest && helmet && mag)
		{
			rackEntity.SetOrigin("2736.82 77.53 1655.68");
			rackEntity.SetYawPitchRoll("65 0 0");
			rackEntity.Update();
			DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(rackEntity.FindComponent(DCO_GearRackComponent));
			DCO_GearRackStorageComponent storage = DCO_GearRackStorageComponent.Cast(rackEntity.FindComponent(DCO_GearRackStorageComponent));
			InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(rackEntity.FindComponent(InventoryStorageManagerComponent));
			Expect(rack && storage && manager, "Rack has access, native storage and inventory manager");
			Expect(DCO_PlacementCatalog.IsBifrostResource(SCR_ResourceNameUtils.GetPrefabName(rackEntity)), "Rack belongs to lightning-bolt Bifrost catalogue");
			if (rack && storage && manager)
			{
				Expect(rack.GetTarget() == rackEntity, "Physical rack is its own access target");
				vector position = rackEntity.GetOrigin();
				rack.GetAnchorWorld();
				Expect(rackEntity.GetOrigin() == position, "Reading interaction anchor never moves physical rack");
				Expect(DCO_GearRackComponent.GearKind(vest) == DCO_EGearRackSlot.VEST, "ALICE classified as vest");
				Expect(DCO_GearRackComponent.GearKind(helmet) == DCO_EGearRackSlot.HELMET, "PASGT classified as helmet");
				Expect(!storage.CanStoreItem(mag, -1), "Rack refuses loose magazines");
				Expect(!storage.CanStoreResource(SCR_ResourceNameUtils.GetPrefabName(vest), -1), "Rack cannot generate duplicate gear from a prefab");
				Expect(!rack.CanInteract(null) && !rack.CanHang(null, DCO_EGearRackSlot.VEST), "Rack rejects missing action user");
				Expect(manager.TryInsertItemInStorage(vest, storage), "Native inventory inserts original vest");
				Expect(rack.StoredGear(DCO_EGearRackSlot.VEST) == vest, "Rack contains original vest entity");
				Expect(!storage.CanStoreItem(vest, -1), "Occupied vest hook rejects another insert");
				Expect(manager.TryInsertItemInStorage(helmet, storage), "Helmet uses second independent hook");
				Expect(rack.StoredGear(DCO_EGearRackSlot.HELMET) == helmet, "Rack contains original helmet entity");
				CheckNestedCargo(vest, helmet, mag, manager);
				rack.BIA_TestDisplays(this);
				Expect(manager.TryRemoveItemFromStorage(vest, storage), "Original vest can be removed from rack");
				Expect(!rack.StoredGear(DCO_EGearRackSlot.VEST), "Removing vest frees only its hook");
				Expect(rack.StoredGear(DCO_EGearRackSlot.HELMET) == helmet, "Vest removal preserves helmet");
			}
		}
		// Items may still be parented to the rack; delete children before their container.
		BIA_PreviewDress.DeleteLocalHierarchy(mag);
		BIA_PreviewDress.DeleteLocalHierarchy(helmet);
		BIA_PreviewDress.DeleteLocalHierarchy(vest);
		BIA_PreviewDress.DeleteLocalHierarchy(rackEntity);
	}
	void CheckNestedCargo(IEntity vest, IEntity helmet, IEntity mag, InventoryStorageManagerComponent manager)
	{
		array<BaseInventoryStorageComponent> cargo = {};
		BIA_ItemIntel.CollectContainerStorages(vest, cargo);
		Expect(cargo.Count() >= 3, "ALICE exposes both magazine pouches and buttpack");
		set<BaseInventoryStorageComponent> unique = new set<BaseInventoryStorageComponent>();
		foreach (BaseInventoryStorageComponent candidate : cargo) unique.Insert(candidate);
		Expect(unique.Count() == cargo.Count(), "Nested storage traversal does not count a compartment twice");
		BaseInventoryStorageComponent pouch;
		foreach (BaseInventoryStorageComponent candidate : cargo)
		{
			if (SCR_ResourceNameUtils.GetPrefabName(candidate.GetOwner()).Contains("Pouch_ALICE_30rnd_STANAG"))
			{
				pouch = candidate;
				break;
			}
		}
		Expect(pouch != null, "Real native magazine pouch is discoverable inside vest");
		if (!pouch) return;
		Expect(manager.TryInsertItemInStorage(mag, pouch), "Native stocking places magazine inside mounted pouch");
		InventoryItemComponent magItem = InventoryItemComponent.Cast(mag.FindComponent(InventoryItemComponent));
		Expect(magItem && magItem.GetParentSlot() && magItem.GetParentSlot().GetStorage() == pouch, "Magazine retains exact nested storage identity");
		JsonSaveContext saved = new JsonSaveContext();
		Expect(BIA_LoadoutBridge.ReadEntity(vest, saved), "Nested vest capture succeeds");
		string json = saved.SaveToString();
		Expect(json.Contains("Pouch_ALICE_30rnd_STANAG") && json.Contains("Magazine_556x45_STANAG_30rnd"), "Saved vest includes pouch and magazine");
		InventoryItemComponent vestItem = InventoryItemComponent.Cast(vest.FindComponent(InventoryItemComponent));
		InventoryStorageSlot parent;
		if (vestItem) parent = vestItem.GetParentSlot();
		Expect(parent != null, "Captured vest has a parent storage before reapply");
		if (!parent) return;
		for (int pass = 0; pass < 2; pass++)
		{
			JsonLoadContext restore = new JsonLoadContext();
			restore.LoadFromString(json);
			BIA_ApplyStats restored = new BIA_ApplyStats();
			BIA_LoadoutBridge.ApplyNode(vest, restore, manager, parent.GetStorage(), parent.GetID(), null, restored);
			Expect(restored.m_iSkipped == 0 && magItem.GetParentSlot() && magItem.GetParentSlot().GetStorage() == pouch,
				"Repeated nested apply preserves original magazine: " + pass.ToString());
		}
		BaseInventoryStorageComponent mounts = BIA_ItemIntel.AttachmentStorage(vest);
		InventoryItemComponent pouchItem = InventoryItemComponent.Cast(pouch.GetOwner().FindComponent(InventoryItemComponent));
		InventoryStorageSlot mount;
		if (pouchItem) mount = pouchItem.GetParentSlot();
		Expect(mounts && mount && mount.GetStorage() == mounts, "Pouch belongs to a native clothing hardpoint");
		if (!mounts || !mount) return;
		Expect(!BIA_ItemIntel.MountAcceptsEntity(mount, helmet), "Helmet cannot occupy a pouch hardpoint");
		JsonLoadContext invalid = new JsonLoadContext();
		invalid.LoadFromString(string.Format("{\"attachments\":[{\"prefab\":\"%1\",\"slot\":%2}]}", SCR_ResourceNameUtils.GetPrefabName(helmet), mount.GetID()));
		BIA_ApplyStats stats = new BIA_ApplyStats();
		BIA_LoadoutBridge.ApplyAttachmentsArray(vest, invalid, manager, null, stats);
		Expect(stats.m_iSkipped == 1, "Server attachment path reports incompatible request");
		Expect(mount.GetAttachedEntity() == pouch.GetOwner(), "Rejected attachment keeps original occupied pouch");
		Expect(magItem.GetParentSlot() && magItem.GetParentSlot().GetStorage() == pouch, "Rejected attachment keeps original nested magazine");
	}
	void CheckXL(BaseWorld world)
	{
		Resource metal = Resource.Load("{CE9253778DD8FBDE}Common/Materials/Game/metal.gamemat");
		Expect(metal && metal.IsValid(), "Native metal game material resolves");
		if (metal && metal.IsValid()) details.Insert("Collision material: " + metal.GetResource().GetResourceName());
		IEntity xl = Spawn(world, "{762A1A6379E24BD4}Prefabs/E_DCO_XLGearCross.et");
		IEntity small = Spawn(world, "{D67E9B88BBDB4FE4}Prefabs/E_DCO_PlaceableArsenal.et");
		IEntity character = Spawn(world, "{37578B1666981FCE}Prefabs/Characters/Core/Character_Base.et");
		DCO_GearRackComponent rack;
		DCO_GearRackStorageComponent storage;
		EquipedLoadoutStorageComponent worn;
		InventoryStorageManagerComponent manager;
		if (xl)
		{
			rack = DCO_GearRackComponent.Cast(xl.FindComponent(DCO_GearRackComponent));
			storage = DCO_GearRackStorageComponent.Cast(xl.FindComponent(DCO_GearRackStorageComponent));
			Expect(xl.GetVObject() != null, "XL imported mesh loads natively");
			vector mins, maxs; xl.GetBounds(mins, maxs);
			details.Insert("XL native mesh bounds: " + mins.ToString() + " / " + maxs.ToString());
			Expect(Math.AbsFloat(maxs[1] - mins[1] - 1.3462) < 0.002, "XL native mesh retains intended height");
			Expect(rack && rack.SupportsBelt(), "XL supports waist belt hook");
			CheckCrossActions(xl, 4, "XL Gear Cross");
			xl.SetOrigin("2736.82 77.53 1655.68");
			xl.SetYawPitchRoll("65 0 0");
			xl.Update();
		}
		if (small)
		{
			DCO_GearRackComponent smallRack = DCO_GearRackComponent.Cast(small.FindComponent(DCO_GearRackComponent));
			Expect(smallRack && !smallRack.SupportsBelt(), "Small cross refuses belt hook");
			CheckCrossActions(small, 3, "Gear Cross");
		}
		if (character)
		{
			character.SetOrigin("2736.82 77.53 1656.68");
			worn = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
			manager = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		}
		Expect(worn && manager, "Native character has equipped storage and manager");
		int waist = BIA_KitConvert.FindClothingSlotForKey(worn, "ZEL_WaistArea");
		Expect(waist >= 0, "Loaded character exposes actual ZEL Waist slot");
		PlayerController controller = GetGame().GetPlayerController();
		SCR_ResourcePlayerControllerInventoryComponent requester;
		if (controller) requester = SCR_ResourcePlayerControllerInventoryComponent.Cast(controller.FindComponent(SCR_ResourcePlayerControllerInventoryComponent));
		array<ResourceName> belts = {
			"{3281A5FE11A4A7E3}Belts/MTAC_Belt/Prefab/Mtac_RPS_MC.et",
			"{6E5A5D2864C4CD2B}Belts/TYR_Gunfighter_MAB/Prefabs/TYR_MAB.et",
			"{94BF8E53A6E09DF7}Belts/Virtus_H_rig/Prefab/Virtus_H.et",
			"{7BD6C7BFB27170E1}Assets/Characters/Belts/Belt_Ronin/BisonBelt.et",
			"{38CD60392EA68DD7}Assets/Characters/Belts/Belt_Ronin/ShutoBelt.et",
			"{6ABC5B77A87C44F5}Assets/Characters/Belts/Belt_Ronin/TyrBelt.et"
		};
		if (!rack || !storage || !worn || !manager || waist < 0 || !requester)
		{
			failures.Insert("XL belt transfer fixtures or owned inventory requester unavailable");
			BIA_PreviewDress.DeleteLocalHierarchy(character);
			BIA_PreviewDress.DeleteLocalHierarchy(small);
			BIA_PreviewDress.DeleteLocalHierarchy(xl);
			return;
		}
		m_XL = xl;
		m_Small = small;
		m_Character = character;
		m_Worn = worn;
		m_Manager = manager;
		m_Requester = requester;
		m_Gear = {
			"{2835A0EA3B79E63E}Prefabs/Characters/Vests/Vest_ALICE/Variants/Vest_ALICE_rifleman.et",
			"{B74A4FF0DD8BB116}Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01.et",
			"{2835A0EA3B79E63E}Prefabs/Characters/Vests/Vest_ALICE/Variants/Vest_ALICE_rifleman.et",
			"{B74A4FF0DD8BB116}Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01.et"
		};
		foreach (ResourceName belt : belts) m_Gear.Insert(belt);
		small.SetOrigin(xl.GetOrigin());
		small.SetYawPitchRoll(xl.GetYawPitchRoll());
		small.Update();
		running = true;
		GetGame().GetCallqueue().CallLater(AdvanceTransfer, 100, true);
	}

	bool running;
	IEntity m_XL, m_Small, m_Character, m_Item, m_Mag;
	EquipedLoadoutStorageComponent m_Worn;
	InventoryStorageManagerComponent m_Manager;
	SCR_ResourcePlayerControllerInventoryComponent m_Requester;
	ref array<ResourceName> m_Gear;
	int m_Fixture, m_Phase, m_Waits, m_Slot;
	bool m_Stocked;
	string m_Before;
	void AdvanceTransfer()
	{
		if (m_Fixture >= m_Gear.Count())
		{
			Expect(m_Stocked, "At least one real nested belt pouch preserves a stocked magazine");
			details.Insert("XL belt fixture sweep completed");
			GetGame().GetCallqueue().Remove(AdvanceTransfer);
			BIA_PreviewDress.DeleteLocalHierarchy(m_Character);
			BIA_PreviewDress.DeleteLocalHierarchy(m_Small);
			BIA_PreviewDress.DeleteLocalHierarchy(m_XL);
			running = false;
			return;
		}
		IEntity owner = m_XL;
		if (m_Fixture < 2) owner = m_Small;
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(owner.FindComponent(DCO_GearRackComponent));
		ResourceName prefab = m_Gear[m_Fixture];
		if (m_Phase == 0)
		{
			m_Item = Spawn(owner.GetWorld(), prefab);
			m_Slot = BIA_KitConvert.FindClothingSlotForKey(m_Worn, BIA_ItemIntel.GetClothAreaType(prefab));
			if (!m_Item || m_Slot < 0 || !m_Manager.TryInsertItemInStorage(m_Item, m_Worn, m_Slot))
			{
				failures.Insert("Cannot equip fixture: " + prefab);
				NextFixture();
				return;
			}
			m_Phase = 1;
			return;
		}
		DCO_EGearRackSlot kind = DCO_GearRackComponent.GearKind(m_Item);
		if (m_Phase == 1)
		{
			if (WaitFor(m_Worn.Get(m_Slot) == m_Item, "Native equipment insertion: " + prefab)) return;
			if (!m_Item) return;
			Expect(DCO_GearRackComponent.WornGear(m_Character, kind) == m_Item, "Finds equipped original: " + prefab);
			if (kind == DCO_EGearRackSlot.BELT && !m_Stocked)
			{
				IEntity candidate = Spawn(owner.GetWorld(), "{D8F2CA92583B23D3}Prefabs/Weapons/Magazines/Magazine_556x45_STANAG_30rnd_M855_M856_Last_5Tracer.et");
				array<BaseInventoryStorageComponent> compartments = {};
				BIA_ItemIntel.CollectContainerStorages(m_Item, compartments);
				foreach (BaseInventoryStorageComponent compartment : compartments)
				{
					if (compartment.GetOwner() == m_Item || !candidate) continue;
					if (m_Manager.TryInsertItemInStorage(candidate, compartment))
					{
						m_Mag = candidate;
						details.Insert("Stocked nested belt pouch: " + SCR_ResourceNameUtils.GetPrefabName(compartment.GetOwner()));
						break;
					}
				}
				if (!m_Mag) BIA_PreviewDress.DeleteLocalHierarchy(candidate);
			}
			m_Phase = 2;
			return;
		}
		if (m_Phase == 2)
		{
			InventoryItemComponent magazine;
			if (m_Mag) magazine = InventoryItemComponent.Cast(m_Mag.FindComponent(InventoryItemComponent));
			if (m_Mag && WaitFor(magazine && magazine.GetParentSlot(), "Nested magazine insertion")) return;
			JsonSaveContext before = new JsonSaveContext();
			Expect(BIA_LoadoutBridge.ReadEntity(m_Item, before), "Capture before Hang");
			m_Before = before.SaveToString();
			CheckActionLabel(rack, kind, false);
			Expect(rack.CanHang(m_Character, kind), "World action enables Hang: " + prefab);
			if (!rack.TransferGear(m_Character, kind, false, m_Requester))
			{
				ChimeraCharacter character = ChimeraCharacter.Cast(m_Character);
				details.Insert(string.Format("Hang rejected: player=%1 rack=%2 dead=%3 unconscious=%4", m_Character.GetOrigin(), owner.GetOrigin(), character.GetCharacterController().IsDead(), character.GetCharacterController().IsUnconscious()));
				failures.Insert("Server Hang rejected: " + prefab);
				NextFixture(); return;
			}
			m_Phase = 3;
			return;
		}
		if (m_Phase == 3)
		{
			if (WaitFor(!rack.IsBusy() && rack.StoredGear(kind) == m_Item, "Native Hang completion: " + prefab)) return;
			if (!m_Item) return;
			Expect(!m_Worn.Get(m_Slot), "Hang removes original from body");
			CheckActionLabel(rack, kind, true);
			Expect(rack.CanTake(m_Character, kind), "World action enables Take");
			rack.BIA_TestCurrentDisplays(this);
			if (kind == DCO_EGearRackSlot.BELT) rack.BIA_TestBeltDisplay(this);
			if (!rack.TransferGear(m_Character, kind, true, m_Requester))
			{
				failures.Insert("Server Take rejected: " + prefab);
				NextFixture(); return;
			}
			m_Phase = 4;
			return;
		}
		if (WaitFor(!rack.IsBusy() && m_Worn.Get(m_Slot) == m_Item, "Native Take completion: " + prefab)) return;
		if (!m_Item) return;
		Expect(!rack.StoredGear(kind), "Take empties hook");
		CheckActionLabel(rack, kind, false);
		JsonSaveContext after = new JsonSaveContext();
		Expect(BIA_LoadoutBridge.ReadEntity(m_Item, after), "Capture after Take");
		Expect(m_Before == after.SaveToString(), "Original attachments and contents survive Hang/Take: " + prefab);
		if (m_Mag)
		{
			InventoryItemComponent magazine = InventoryItemComponent.Cast(m_Mag.FindComponent(InventoryItemComponent));
			bool preserved = magazine && magazine.GetParentSlot() && after.SaveToString().Contains("Magazine_556x45_STANAG_30rnd");
			Expect(preserved, "Original nested magazine survives belt round trip");
			m_Stocked = m_Stocked || preserved;
		}
		NextFixture();
	}
	bool WaitFor(bool ready, string failure)
	{
		if (ready) { m_Waits = 0; return false; }
		m_Waits++;
		if (m_Waits < 30) return true;
		failures.Insert(failure);
		NextFixture();
		return true;
	}
	void NextFixture()
	{
		BIA_PreviewDress.DeleteLocalHierarchy(m_Mag);
		BIA_PreviewDress.DeleteLocalHierarchy(m_Item);
		m_Mag = null; m_Item = null;
		m_Phase = 0; m_Waits = 0;
		m_Fixture++;
	}
	void CheckActionLabel(DCO_GearRackComponent rack, DCO_EGearRackSlot kind, bool take)
	{
		ActionsManagerComponent manager = ActionsManagerComponent.Cast(rack.GetOwner().FindComponent(ActionsManagerComponent));
		array<BaseUserAction> actions = {};
		manager.GetActionsList(actions);
		string gear = "Vest";
		typename actionType = DCO_HangVestAction;
		if (kind == DCO_EGearRackSlot.HELMET) { gear = "Helmet"; actionType = DCO_HangHelmetAction; }
		if (kind == DCO_EGearRackSlot.BELT) { gear = "Belt"; actionType = DCO_HangBeltAction; }
		string verb = "Hang ";
		if (take) verb = "Take ";
		foreach (BaseUserAction action : actions)
		{
			if (action.Type() != actionType) continue;
			string label;
			DCO_HangVestAction gearAction = DCO_HangVestAction.Cast(action);
			gearAction.GetActionNameScript(label);
			Expect(label == verb + gear, "World action label is " + verb + gear);
			return;
		}
		failures.Insert("World gear action missing");
	}
	void CheckCrossActions(IEntity entity, int count, string name)
	{
		ActionsManagerComponent actions = ActionsManagerComponent.Cast(entity.FindComponent(ActionsManagerComponent));
		array<BaseUserAction> list = {};
		if (actions) actions.GetActionsList(list);
		Expect(list.Count() == count, "Native action count: " + name);
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		Expect(editable && editable.GetInfo() && editable.GetInfo().GetName() == name, "Native display name: " + name);
	}

	void CheckLayouts()
	{
		BIA_Theme.BeginStationSession(Color.FromRGBA(217, 137, 43, 255), 0.2);
		array<ResourceName> layouts = {
			"{AB205AD4E2000001}UI/layouts/Menus/ArmoryV2/GRSA_Shell.layout",
			"{AB205AD4E2000004}UI/layouts/Menus/ArmoryV2/GRSA_ScreenGunsmith.layout"
		};
		foreach (ResourceName path : layouts)
		{
			Widget root = GetGame().GetWorkspace().CreateWidgets(path);
			Expect(root != null, "Native layout creates: " + path);
			if (!root) continue;
			BIA_Theme.Apply(root);
			CheckSurfaces(root);
			Expect(!root.FindAnyWidget("RackGearRow") && !root.FindAnyWidget("RackTakeVestButton")
				&& !root.FindAnyWidget("RackTakeHelmetButton") && !root.FindAnyWidget("RackTakeBeltButton"), "Arsenal has no equipment recovery controls");
			root.RemoveFromHierarchy();
		}
		BIA_Theme.EndSession();
	}
	void CheckSurfaces(Widget widget)
	{
		Widget cursor = widget;
		array<string> names = {"HeaderBg", "FooterBg", "LeftRailBg", "RightRailBg", "CandidatesBg", "HardpointCounterBg"};
		while (cursor)
		{
			if (names.Find(cursor.GetName()) >= 0)
			{
				Color color = cursor.GetColor();
				Expect(color.A() > 0.99 && cursor.GetOpacity() > 0.99 && color.R() < 0.05,
					"Text surface blocks stage at low opacity: " + cursor.GetName());
			}
			CheckSurfaces(cursor.GetChildren());
			cursor = cursor.GetSibling();
		}
	}
}

modded class DCO_GearRackComponent
{
	void BIA_TestCurrentDisplays(BIA_ArsenalRackProbe probe)
	{
		RefreshDisplays();
		BIA_TestDisplay(probe, m_VestDisplay);
		BIA_TestDisplay(probe, m_HelmetDisplay);
		BIA_TestDisplay(probe, m_BeltDisplay);
	}
	void BIA_TestDisplay(BIA_ArsenalRackProbe probe, IEntity display)
	{
		if (!display) return;
		vector mins, maxs;
		display.GetBounds(mins, maxs);
		probe.details.Insert(string.Format("Runtime display %1: origin=%2, rack=%3, scale=%4, flags=%5, bounds=%6 / %7", display.Type(), display.GetOrigin(), GetOwner().GetOrigin(), display.GetScale(), display.GetFlags(), mins, maxs));
		probe.Expect((display.GetFlags() & EntityFlags.VISIBLE) != 0, "Runtime gear has visible flag");
		probe.Expect(vector.Distance(display.GetOrigin(), GetOwner().GetOrigin()) < 2.0, "Runtime gear remains beside non-origin rack");
	}
	void BIA_TestBeltDisplay(BIA_ArsenalRackProbe probe)
	{
		RefreshDisplays();
		probe.Expect(m_BeltDisplay && m_BeltDisplay.GetVObject(), "XL produces native belt preview mesh");
		if (!m_BeltDisplay) return;
		vector mins, maxs; m_BeltDisplay.GetBounds(mins, maxs);
		probe.details.Insert("Belt preview " + SCR_ResourceNameUtils.GetPrefabName(StoredGear(DCO_EGearRackSlot.BELT)) + " bounds=" + mins.ToString() + " / " + maxs.ToString());
		vector forward = m_BeltDisplay.GetWorldTransformAxis(2).Normalized();
		probe.Expect(vector.Distance(forward, GetOwner().GetWorldTransformAxis(2).Normalized()) < 0.001, "Belt faces forward with vest");
		probe.Expect(m_BeltDisplay.GetParent() == GetOwner(), "Belt display follows cross hierarchy");
		BIA_TestDisplay(probe, m_BeltDisplay);
	}
	void BIA_TestDisplays(BIA_ArsenalRackProbe probe)
	{
		RefreshDisplays();
		probe.Expect(m_VestDisplay && m_VestDisplay.GetVObject(), "Rack has local worn vest display mesh");
		probe.Expect(m_HelmetDisplay && m_HelmetDisplay.GetVObject(), "Rack has local helmet display mesh");
		probe.Expect(m_VestDisplay && m_VestDisplay != StoredGear(DCO_EGearRackSlot.VEST), "Display is distinct from real stored vest");
		probe.Expect(m_HelmetDisplay && m_HelmetDisplay.GetParent() == GetOwner(), "Display follows placed rack transform");
		BIA_TestDisplay(probe, m_VestDisplay);
		BIA_TestDisplay(probe, m_HelmetDisplay);
	}
}
