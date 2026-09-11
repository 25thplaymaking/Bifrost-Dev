//! Preview-entity dressing shared by each mannequin stage. It only changes local preview
//! entities; live inventory mutations stay in the server-side apply pipeline.
class BIA_PreviewDress
{
	//------------------------------------------------------------------------------------------------
	//! Preview entities are local and must not be removed through replicated-entity deletion.
	static void DeleteLocalHierarchy(IEntity entity)
	{
		if (!entity || entity.IsDeleted())
			return;

		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity sibling = child.GetSibling();
			DeleteLocalHierarchy(child);
			child = sibling;
		}

		if (!entity.IsDeleted())
			delete entity;
	}

	//------------------------------------------------------------------------------------------------
	//! Re-dresses in place: slots already holding the wanted prefab are untouched, everything else
	//! swaps, weapons get their full attachment subtree synced, the primary lands in hand.
	static void DiffDress(notnull IEntity preview, notnull BIA_Kit kit)
	{
		BaseWorld previewWorld = preview.GetWorld();

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(preview.FindComponent(EquipedLoadoutStorageComponent));
		if (loadoutStorage)
		{
			int slotsCount = loadoutStorage.GetSlotsCount();
			for (int i = 0; i < slotsCount; ++i)
			{
				InventoryStorageSlot slot = loadoutStorage.GetSlot(i);
				if (!slot)
					continue;

				ResourceName wantedPrefab;
				BIA_KitClothing kitClothing = kit.FindClothing(i);
				if (kitClothing)
					wantedPrefab = kitClothing.m_Prefab;

				IEntity clothing = SyncSlot(slot, wantedPrefab, previewWorld);
				if (clothing && kitClothing)
				{
					kitClothing.EnsurePins();
					SyncWeaponAttachmentList(clothing, kitClothing.m_aAttachments, previewWorld, kitClothing.m_aAttachmentSlots);
				}
			}
		}

		EquipedWeaponStorageComponent weaponStorage = EquipedWeaponStorageComponent.Cast(preview.FindComponent(EquipedWeaponStorageComponent));
		IEntity primaryWeapon;
		if (weaponStorage)
		{
			int slotsCount = weaponStorage.GetSlotsCount();
			for (int i = 0; i < slotsCount; ++i)
			{
				InventoryStorageSlot slot = weaponStorage.GetSlot(i);
				if (!slot)
					continue;

				ResourceName wantedPrefab;
				BIA_KitWeapon kitWeapon = kit.FindWeapon(i);
				if (kitWeapon)
					wantedPrefab = kitWeapon.m_Prefab;

				IEntity weapon = SyncSlot(slot, wantedPrefab, previewWorld);
				if (weapon && kitWeapon)
				{
					kitWeapon.EnsurePins();
					SyncWeaponAttachmentList(weapon, kitWeapon.m_aAttachments, previewWorld, kitWeapon.m_aAttachmentSlots);
					if (!primaryWeapon || kitWeapon.m_iSlotIdx == 0)
						primaryWeapon = weapon;
				}
			}
		}

		if (primaryWeapon)
			SelectWeapon(preview, primaryWeapon);
	}

	//------------------------------------------------------------------------------------------------
	static IEntity SyncSlot(notnull InventoryStorageSlot slot, ResourceName wantedPrefab, BaseWorld previewWorld)
	{
		IEntity current = slot.GetAttachedEntity();
		ResourceName currentPrefab;
		if (current)
			currentPrefab = SCR_ResourceNameUtils.GetPrefabName(current);

		if (currentPrefab == wantedPrefab)
			return current;

		if (current)
			DeleteLocalHierarchy(current);

		if (wantedPrefab.IsEmpty())
			return null;

		IEntity spawned = SpawnLocal(wantedPrefab, previewWorld);
		if (spawned)
			slot.AttachEntity(spawned);
		return spawned;
	}

	//------------------------------------------------------------------------------------------------
	//! The kit attachment list is the complete authoritative set for the WHOLE subtree: prune
	//! anything not wanted, then place the rest over several passes so items that only fit on
	//! another attachment (a light on a rail) mount once their parent exists. pins runs parallel
	//! to attachments (-1 = automatic): pinned entries claim their exact top-level hardpoint before
	//! first-fit runs, and a pinned prefab found resting anywhere else is displaced to its pin.
	//! outUnplaced (optional) receives everything the engine finally refused.
	static void SyncWeaponAttachmentList(notnull IEntity weapon, notnull array<ResourceName> attachments, BaseWorld previewWorld, array<int> pins = null, array<ResourceName> outUnplaced = null)
	{
		map<int, ResourceName> pinBySlot = new map<int, ResourceName>();
		map<ResourceName, int> pinByPrefab = new map<ResourceName, int>();
		BaseInventoryStorageComponent topStorage = BIA_ItemIntel.AttachmentStorage(weapon);
		if (pins && topStorage && pins.Count() == attachments.Count())
		{
			foreach (int pinIdx, ResourceName pinPrefab : attachments)
			{
				int pin = pins[pinIdx];
				if (pin == -1) continue;
				if (pin < -1 || pin >= topStorage.GetSlotsCount() || pinBySlot.Contains(pin))
				{
					if (outUnplaced) outUnplaced.Copy(attachments);
					return;
				}

				pinBySlot.Insert(pin, pinPrefab);
				if (!pinByPrefab.Contains(pinPrefab))
					pinByPrefab.Insert(pinPrefab, pin);
			}
		}

		//! Displacement before the prune: clear pin-target slots of foreign items and clear pinned
		//! prefabs off wrong slots, so keep-by-count can never strand a pin.
		if (topStorage && !pinBySlot.IsEmpty())
		{
			int topCount = topStorage.GetSlotsCount();
			for (int i = 0; i < topCount; ++i)
			{
				InventoryStorageSlot slot = topStorage.GetSlot(i);
				if (!slot)
					continue;

				IEntity current = slot.GetAttachedEntity();
				if (!current)
					continue;

				ResourceName currentPrefab = SCR_ResourceNameUtils.GetPrefabName(current);

				ResourceName pinWant;
				bool slotIsPinTarget = pinBySlot.Find(i, pinWant);
				if (slotIsPinTarget && currentPrefab == pinWant)
					continue;

				int prefabPin;
				bool prefabIsPinned = pinByPrefab.Find(currentPrefab, prefabPin);

				if (slotIsPinTarget || (prefabIsPinned && prefabPin != i))
					DeleteLocalHierarchy(current);
			}
		}

		map<ResourceName, int> wantedCounts = new map<ResourceName, int>();
		foreach (ResourceName attachment : attachments)
		{
			int count = 0;
			wantedCounts.Find(attachment, count);
			wantedCounts.Set(attachment, count + 1);
		}

		PruneSubtree(weapon, wantedCounts, 0);

		array<ResourceName> pending = {};
		foreach (ResourceName prefab, int remaining : wantedCounts)
		{
			for (int n = 0; n < remaining; ++n)
				pending.Insert(prefab);
		}

		//! Pinned placement first, so first-fit can never steal a reserved hardpoint.
		if (topStorage && !pinBySlot.IsEmpty())
		{
			foreach (int pinSlot, ResourceName pinnedPrefab : pinBySlot)
			{
				InventoryStorageSlot slot = topStorage.GetSlot(pinSlot);
				if (!slot || slot.GetAttachedEntity())
					continue;

				int pendingIdx = pending.Find(pinnedPrefab);
				if (pendingIdx < 0)
					continue;

				if (!BIA_ItemIntel.MountAccepts(slot, pinnedPrefab))
					continue;

				IEntity attachment = SpawnLocal(pinnedPrefab, previewWorld);
				if (!attachment)
					continue;

				if (!topStorage.CanStoreItem(attachment, pinSlot))
				{
					DeleteLocalHierarchy(attachment);
					continue;
				}

				slot.AttachEntity(attachment);
				pending.Remove(pendingIdx);
			}
		}

		array<ResourceName> rejectedPins = {};
		foreach (int requestedSlot, ResourceName requestedPrefab : pinBySlot)
		{
			IEntity mounted = topStorage.Get(requestedSlot);
			if (mounted && SCR_ResourceNameUtils.GetPrefabName(mounted) == requestedPrefab) continue;
			int pendingPin = pending.Find(requestedPrefab);
			if (pendingPin >= 0)
			{
				pending.Remove(pendingPin);
				rejectedPins.Insert(requestedPrefab);
			}
		}

		for (int pass = 0; pass < 4 && !pending.IsEmpty(); ++pass)
		{
			array<ResourceName> stillPending = {};
			foreach (ResourceName prefab : pending)
			{
				IEntity attachment = SpawnLocal(prefab, previewWorld);
				if (!attachment)
				{
					stillPending.Insert(prefab);
					continue;
				}

				if (!AttachIntoSubtree(weapon, attachment, prefab, 0))
				{
					DeleteLocalHierarchy(attachment);
					stillPending.Insert(prefab);
				}
			}
			pending = stillPending;
		}

		if (outUnplaced)
		{
			outUnplaced.Copy(pending);
			foreach (ResourceName rejected : rejectedPins) outUnplaced.Insert(rejected);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Wanted attached items stay and recurse, everything else is deleted with its children.
	protected static void PruneSubtree(notnull IEntity owner, notnull map<ResourceName, int> wantedCounts, int depth)
	{
		if (depth > 4)
			return;

		BaseInventoryStorageComponent storage = BIA_ItemIntel.AttachmentStorage(owner);
		if (!storage)
			return;

		int slotsCount = storage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			if (!slot)
				continue;

			IEntity current = slot.GetAttachedEntity();
			if (!current)
				continue;

			ResourceName currentPrefab = SCR_ResourceNameUtils.GetPrefabName(current);
			int wanted = 0;
			wantedCounts.Find(currentPrefab, wanted);
			if (wanted > 0)
			{
				wantedCounts.Set(currentPrefab, wanted - 1);
				PruneSubtree(current, wantedCounts, depth + 1);
				continue;
			}

			DeleteLocalHierarchy(current);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool AttachIntoSubtree(notnull IEntity owner, notnull IEntity attachment, ResourceName attachmentPrefab, int depth)
	{
		if (depth > 4)
			return false;

		BaseInventoryStorageComponent storage = BIA_ItemIntel.AttachmentStorage(owner);
		if (!storage)
			return false;

		int slotsCount = storage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			if (!slot || slot.GetAttachedEntity())
				continue;

			//! CanStoreItem alone lets parts land on any accepting rail; the slot's authored
			//! attachment type is the real contract, the same predicate the browsers filter with.
			if (!BIA_ItemIntel.MountAccepts(slot, attachmentPrefab))
				continue;

			if (!storage.CanStoreItem(attachment, i))
				continue;

			slot.AttachEntity(attachment);
			return true;
		}

		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			if (!slot)
				continue;

			IEntity attached = slot.GetAttachedEntity();
			if (attached && AttachIntoSubtree(attached, attachment, attachmentPrefab, depth + 1))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void SelectWeapon(notnull IEntity preview, notnull IEntity weapon)
	{
		//! Inventory preview clones derive their held-weapon state from the selected weapon slot.
		BaseWeaponManagerComponent weaponManager = BaseWeaponManagerComponent.Cast(preview.FindComponent(BaseWeaponManagerComponent));
		if (!weaponManager)
			return;

		array<WeaponSlotComponent> outSlots = {};
		weaponManager.GetWeaponsSlots(outSlots);
		foreach (WeaponSlotComponent weaponSlot : outSlots)
		{
			if (weaponSlot.GetWeaponEntity() == weapon)
			{
				weaponManager.SelectWeapon(weaponSlot);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity SpawnLocal(ResourceName prefab, BaseWorld world)
	{
		if (prefab.IsEmpty())
			return null;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;

		return GetGame().SpawnEntityPrefabLocal(resource, world);
	}
}
