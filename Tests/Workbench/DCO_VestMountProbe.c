class DCO_VestMountProbe
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
		if (!BIA_ItemIntel.GetPreviewManager())
		{
			failures.Insert("A fully initialized local game with its item preview manager is required.");
			return;
		}
		BIA_KitFile file = new BIA_KitFile();
		file.DCO_TestMountReadback(this);
		BIA_StageCore stage = new BIA_StageCore();
		if (!stage.EnsureWorld("BifrostVestTest"))
		{
			failures.Insert("Private preview world");
			return;
		}
		if ("GRS_ArmorVestStorageComponent".ToType()) Check(stage.GetWorld(), "{BE491CE7D9FF74CA}Prefabs/Vests/MFCR_Chestrigs/GRS_V2_MFCR_Black.et", "{22850DBB9FF3D973}Prefabs/Pouches/Black/GRS_M67_Filbe_Pouch_Black.et", true);
		else details.Insert("SKIP: GRS is not loaded; duplicate-pouch mount checks were not run.");
		Check(stage.GetWorld(), "{A0A27FF148640F4E}Plate Carriers/FCPC/Prefabs/FCPC.et", "{7EC9A9DF1B770AF2}BackPanels/Prefabs/Ferro_BackPanel_Banger_MC.et", false);
		stage.Release();
	}
	void Check(BaseWorld world, ResourceName vestPath, ResourceName partPath, bool duplicates)
	{
		Resource vestResource = Resource.Load(vestPath);
		Resource partResource = Resource.Load(partPath);
		if (!vestResource || !vestResource.IsValid() || !partResource || !partResource.IsValid())
		{
			failures.Insert("Resources: " + vestPath);
			return;
		}
		IEntity vest = GetGame().SpawnEntityPrefabLocal(vestResource, world);
		IEntity part = GetGame().SpawnEntityPrefabLocal(partResource, world);
		if (!vest || !part)
		{
			failures.Insert("Spawn: " + vestPath);
			if (vest) BIA_PreviewDress.DeleteLocalHierarchy(vest);
			if (part) BIA_PreviewDress.DeleteLocalHierarchy(part);
			return;
		}
		ResourceName partPrefab = SCR_ResourceNameUtils.GetPrefabName(part);
		details.Insert(SCR_ResourceNameUtils.GetPrefabName(vest));
		details.Insert(partPrefab);
		BaseInventoryStorageComponent storage = BIA_ItemIntel.AttachmentStorage(vest);
		if (!storage)
		{
			failures.Insert("Native mounts: " + vestPath);
			BIA_PreviewDress.DeleteLocalHierarchy(vest);
			return;
		}
		array<ref BIA_SlotNode> nodes = {};
		BIA_ItemIntel.BuildSlotTree(vest, nodes);
		Expect(nodes.Count() > 1, "Visible mount tree: " + vestPath);
		details.Insert(string.Format("%1: %2 slots, %3 nodes", vestPath, storage.GetSlotsCount(), nodes.Count()));
		array<int> compatible = {};
		int incompatible = -1;
		for (int i = 0; i < storage.GetSlotsCount(); i++)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			if (BIA_ItemIntel.MountAccepts(slot, partPrefab)) compatible.Insert(i);
			else incompatible = i;
		}
		BIA_PreviewDress.DeleteLocalHierarchy(part);
		Expect(!compatible.IsEmpty(), "Compatible mount: " + vestPath);
		details.Insert(string.Format("Compatible mounts: %1", compatible.Count()));
		if (compatible.IsEmpty())
		{
			BIA_PreviewDress.DeleteLocalHierarchy(vest);
			return;
		}
		array<ResourceName> wanted = {partPrefab};
		array<int> pins = {compatible[0]};
		array<ResourceName> rejected = {};
		float baseCapacity = BIA_ItemIntel.GetLiveStorageMaxVolume(vest);
		BIA_PreviewDress.SyncWeaponAttachmentList(vest, wanted, world, pins, rejected);
		Expect(rejected.IsEmpty() && storage.Get(pins[0]), "Pinned mount: " + vestPath);
		if (duplicates)
			Expect(BIA_ItemIntel.GetLiveStorageMaxVolume(vest) > baseCapacity, "Mounted pouch increases live contents capacity");
		if (duplicates && compatible.Count() > 2)
		{
			wanted.Insert(partPrefab);
			pins.Insert(compatible[1]);
			BIA_PreviewDress.SyncWeaponAttachmentList(vest, wanted, world, pins, rejected);
			Expect(rejected.IsEmpty() && storage.Get(pins[0]) && storage.Get(pins[1]), "Duplicate pouches retain distinct mounts");
			wanted.Remove(1);
			pins.Remove(1);
			pins[0] = compatible[2];
			BIA_PreviewDress.SyncWeaponAttachmentList(vest, wanted, world, pins, rejected);
			Expect(rejected.IsEmpty() && !storage.Get(compatible[0]) && !storage.Get(compatible[1]) && storage.Get(compatible[2]), "Pouch moves to exact chosen mount");
		}
		{
			BIA_DraftService contents = new BIA_DraftService();
			contents.m_Draft = new BIA_Kit();
			ResourceName grenade = "Prefabs/Weapons/Grenades/Grenade_M67.et";
			Expect(contents.GetAddDraftExtraResult(grenade, 1, vestPath, vest) == BIA_EExtraChangeResult.ADDED, "Mounted pouch accepts contents from live vest storage");
			Expect(contents.GetAddDraftExtraResult(grenade, 10000, vestPath, vest) != BIA_EExtraChangeResult.ADDED, "Live vest storage rejects over-capacity contents");
			Expect(contents.GetAddDraftExtraResult(grenade, 1, partPath, vest) == BIA_EExtraChangeResult.INVALID, "Mismatched preview cannot bypass container validation");
		}
		int validPin = pins[0];
		pins[0] = storage.GetSlotsCount();
		BIA_PreviewDress.SyncWeaponAttachmentList(vest, wanted, world, pins, rejected);
		Expect(rejected.Count() == 1 && storage.Get(validPin), "Invalid pin preserves existing mounts: " + vestPath);
		pins[0] = validPin;
		if (incompatible >= 0)
		{
			pins[0] = incompatible;
			BIA_PreviewDress.SyncWeaponAttachmentList(vest, wanted, world, pins, rejected);
			Expect(rejected.Count() == 1 && !storage.Get(incompatible), "Incompatible pin is refused: " + vestPath);
		}
		wanted.Clear();
		pins.Clear();
		BIA_PreviewDress.SyncWeaponAttachmentList(vest, wanted, world, pins, rejected);
		array<ResourceName> remaining = {};
		BIA_ItemIntel.CollectSubtreeAttachments(vest, remaining);
		Expect(remaining.IsEmpty(), "Remove all mounted parts: " + vestPath);
		BIA_PreviewDress.DeleteLocalHierarchy(vest);
	}
}

modded class BIA_KitFile
{
	void DCO_TestMountReadback(DCO_VestMountProbe probe)
	{
		JsonLoadContext context = new JsonLoadContext();
		context.LoadFromString("{\"attachments\":[{\"prefab\":\"same.et\",\"slot\":2},{\"prefab\":\"same.et\",\"slot\":7}]}");
		array<ResourceName> parts = {};
		array<int> pins = {};
		ReadNodeAttachments(context, parts, pins);
		probe.Expect(parts.Count() == 2 && pins.Count() == 2 && pins[0] == 2 && pins[1] == 7, "Saved duplicate parts retain exact pins");
		context = new JsonLoadContext();
		context.LoadFromString("{\"storages\":[{\"id\":\"DCO_TestClothStorage:0\",\"slots\":{\"3\":{\"prefab\":\"pouch.et\",\"storages\":[{\"id\":\"SCR_EquipmentStorageComponent:0\",\"slots\":{\"0\":{\"prefab\":\"nested.et\"}}}]}}}]}");
		parts.Clear();
		pins.Clear();
		ReadNodeAttachments(context, parts, pins);
		probe.Expect(parts.Count() == 2 && pins.Count() == 2 && pins[0] == 3 && pins[1] == -1, "Native subclass mount tree retains top-level pins and nested items");
		context = new JsonLoadContext();
		context.LoadFromString("{\"attachments\":[],\"storages\":[{\"id\":\"ClothNodeStorageComponent:0\",\"slots\":{\"1\":{\"prefab\":\"old.et\"}}}]}");
		parts.Clear();
		pins.Clear();
		ReadNodeAttachments(context, parts, pins);
		probe.Expect(parts.IsEmpty() && pins.IsEmpty(), "Explicitly empty draft does not restore older saved mounts");
		context = new JsonLoadContext();
		context.LoadFromString("{\"storages\":[{\"id\":\"SCR_EquipmentStorageComponent:0\",\"slots\":{\"3\":{\"prefab\":\"mag.et\"}}},{\"id\":\"ClothNodeStorageComponent:0\",\"slots\":{\"3\":{\"prefab\":\"pouch.et\"}}}]}");
		parts.Clear();
		pins.Clear();
		ReadNodeAttachments(context, parts, pins);
		probe.Expect(parts.Count() == 1 && parts[0] == "pouch.et" && pins.Count() == 1 && pins[0] == 3, "Secondary equipment slot cannot replace a clothing mount");
	}
}

class DCO_TestClothStorageClass : ClothNodeStorageComponentClass {}
class DCO_TestClothStorage : ClothNodeStorageComponent {}
