class GRSA_ApplyStats
{
	int m_iApplied;
	int m_iSkipped;
	ref array<string> m_aSamples = {};

	//------------------------------------------------------------------------------------------------
	void RecordSkip(string item)
	{
		m_iSkipped++;
		if (m_aSamples.Count() < 5 && !item.IsEmpty())
			m_aSamples.Insert(item);
	}
}

//! Nested storage capture and apply walker. The per-item gate skips restricted arsenal items
//! instead of spawning them.
class GRSA_LoadoutBridge
{
	//------------------------------------------------------------------------------------------------
	static string ComponentId(IEntity owner, GenericComponent component)
	{
		if (!owner || !component)
			return string.Empty;

		BaseContainer source = component.GetComponentSource(owner);
		if (!source)
			return string.Empty;

		return component.ClassName() + ":" + SCR_ResourceNameUtils.GetPrefabGUID(source.GetResourceName());
	}

	//------------------------------------------------------------------------------------------------
	static bool ShouldSaveStorage(BaseInventoryStorageComponent candidate)
	{
		if (!candidate)
			return false;

		typename candidateType = candidate.Type();
		foreach (typename toCheck : SCR_PlayerArsenalLoadout.ARSENALLOADOUT_COMPONENTS_TO_CHECK)
		{
			if (candidateType.IsInherited(toCheck))
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Attachments sort by their own required-attachment count so dependents mount after prerequisites.
	protected static array<InventoryItemComponent> GetSlotItems(BaseInventoryStorageComponent storage)
	{
		array<InventoryItemComponent> items = {};
		if (!storage)
			return items;

		storage.GetOwnedItems(items, false);

		SCR_WeaponAttachmentsStorageComponent weaponAttachments = SCR_WeaponAttachmentsStorageComponent.Cast(storage);
		if (!weaponAttachments)
			return items;

		array<ref SCR_SortableItem<InventoryItemComponent>> sortSlots = {};
		foreach (InventoryItemComponent item : items)
		{
			int numRequirements = 0;
			SCR_WeaponAttachmentObstructionAttributes obstruction = SCR_WeaponAttachmentObstructionAttributes.Cast(item.FindAttribute(SCR_WeaponAttachmentObstructionAttributes));
			if (obstruction)
				numRequirements = obstruction.GetRequiredAttachmentTypes().Count();
			SCR_SortableItem<InventoryItemComponent> sortable(item, numRequirements);
			sortSlots.Insert(sortable);
		}
		sortSlots.Sort();

		items.Clear();
		foreach (SCR_SortableItem<InventoryItemComponent> sortedSlot : sortSlots)
		{
			items.Insert(sortedSlot.m_Item);
		}
		return items;
	}

	//------------------------------------------------------------------------------------------------
	static bool ReadEntity(IEntity owner, SaveContext context)
	{
		if (!owner || !context)
			return false;

		context.WriteValue("prefab", SCR_ResourceNameUtils.GetPrefabName(owner));
		return ReadEntityStorages(owner, context);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ReadEntityStorages(IEntity owner, SaveContext context)
	{
		set<BaseInventoryStorageComponent> candidates = new set<BaseInventoryStorageComponent>();
		SCR_PlayerArsenalLoadout.FindStorageComponents(owner, candidates);

		array<BaseInventoryStorageComponent> storages = {};
		foreach (BaseInventoryStorageComponent candidate : candidates)
		{
			if (ShouldSaveStorage(candidate))
				storages.Insert(candidate);
		}

		int storageCount = storages.Count();
		if (storageCount == 0)
			return true;

		context.StartArray("storages", storageCount);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			context.StartObject();
			context.WriteValue("id", ComponentId(owner, storage));

			array<InventoryItemComponent> slotItems = GetSlotItems(storage);
			int slotCount = slotItems.Count();
			context.StartMap("slots", slotCount);
			foreach (InventoryItemComponent item : slotItems)
			{
				if (!item)
					continue;

				InventoryStorageSlot parentSlot = item.GetParentSlot();
				if (!parentSlot)
					continue;

				context.WriteMapKey(parentSlot.GetID().ToString());
				context.StartObject();
				if (!ReadEntity(item.GetOwner(), context))
					return false;
				context.EndObject();
			}
			context.EndMap();
			context.EndObject();
		}
		return context.EndArray();
	}

	//------------------------------------------------------------------------------------------------
	static bool ApplyStoragesOnto(IEntity entity, LoadContext context, InventoryStorageManagerComponent manager, GRSA_ApplyGate gate, notnull GRSA_ApplyStats stats)
	{
		if (!entity || !manager)
			return SkipStoragesArray(context);

		return ApplyStoragesArray(entity, context, manager, gate, stats);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ApplyStoragesArray(IEntity owner, LoadContext context, InventoryStorageManagerComponent manager, GRSA_ApplyGate gate, notnull GRSA_ApplyStats stats)
	{
		int storageCount = 0;
		if (!context.StartArray("storages", storageCount))
			return true;

		if (storageCount == 0)
			return context.EndArray();

		array<BaseInventoryStorageComponent> ownerStorages = {};
		CollectStorages(owner, ownerStorages);

		for (int s = 0; s < storageCount; s++)
		{
			if (!context.StartObject())
				return false;

			string savedId;
			if (!context.ReadValue("id", savedId))
			{
				context.EndObject();
				continue;
			}

			BaseInventoryStorageComponent matchedStorage = MatchStorage(owner, ownerStorages, savedId);
			if (!matchedStorage)
			{
				int dropped = SkipSlotsMap(context);
				if (dropped > 0)
					stats.RecordSkip(savedId);
				stats.m_iSkipped += dropped;
				context.EndObject();
				continue;
			}

			ApplySlotsMap(matchedStorage, context, manager, gate, stats);
			context.EndObject();
		}

		return context.EndArray();
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplySlotsMap(BaseInventoryStorageComponent storage, LoadContext context, InventoryStorageManagerComponent manager, GRSA_ApplyGate gate, notnull GRSA_ApplyStats stats)
	{
		int slotCount = 0;
		if (!context.StartMap("slots", slotCount))
			return;

		map<int, IEntity> existingBySlot = new map<int, IEntity>();
		array<InventoryItemComponent> ownedItems = {};
		storage.GetOwnedItems(ownedItems, false);
		foreach (InventoryItemComponent itemComponent : ownedItems)
		{
			if (!itemComponent)
				continue;

			InventoryStorageSlot parentSlot = itemComponent.GetParentSlot();
			if (!parentSlot)
				continue;

			existingBySlot.Insert(parentSlot.GetID(), itemComponent.GetOwner());
		}

		for (int slotIdx = 0; slotIdx < slotCount; slotIdx++)
		{
			string idxStr;
			if (!context.ReadMapKey(slotIdx, idxStr))
			{
				stats.m_iSkipped++;
				continue;
			}

			int childSlotId = idxStr.ToInt(-1);
			if (childSlotId < 0)
			{
				stats.m_iSkipped++;
				continue;
			}

			IEntity existing;
			existingBySlot.Take(childSlotId, existing);

			if (!context.StartObject(idxStr))
			{
				stats.m_iSkipped++;
				continue;
			}

			ApplyNode(existing, context, manager, storage, childSlotId, gate, stats);
			context.EndObject();
		}
		context.EndMap();

		foreach (int leftoverSlotId, IEntity leftover : existingBySlot)
		{
			if (leftover)
				manager.TryDeleteItem(leftover);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Applies one serialized node (prefab + nested storages + bare attachments) into a target slot.
	static void ApplyNode(IEntity owner, LoadContext context, InventoryStorageManagerComponent manager, BaseInventoryStorageComponent parentStorage, int slotId, GRSA_ApplyGate gate, notnull GRSA_ApplyStats stats)
	{
		ResourceName savedPrefab;
		if (!context.ReadValue("prefab", savedPrefab))
		{
			stats.m_iSkipped++;
			return;
		}

		int forceFresh = 0;
		context.ReadValue("fresh", forceFresh);

		ResourceName currentPrefab;
		if (owner)
			currentPrefab = SCR_ResourceNameUtils.GetPrefabName(owner);

		if (savedPrefab == currentPrefab && owner && forceFresh == 0)
		{
			stats.m_iApplied++;
			ApplyStoragesArray(owner, context, manager, gate, stats);
			ApplyAttachmentsArray(owner, context, manager, gate, stats);
			return;
		}

		if (savedPrefab.IsEmpty())
		{
			if (owner)
				manager.TryDeleteItem(owner);
			SkipStoragesArray(context);
			return;
		}

		if (gate && !gate.Allows(savedPrefab))
		{
			stats.RecordSkip(savedPrefab);
			SkipStoragesArray(context);
			return;
		}

		Resource resource = Resource.Load(savedPrefab);
		if (!resource || !resource.IsValid())
		{
			stats.RecordSkip(savedPrefab);
			SkipStoragesArray(context);
			return;
		}

		if (owner && !manager.TryDeleteItem(owner))
		{
			stats.RecordSkip(savedPrefab);
			SkipStoragesArray(context);
			return;
		}

		if (!manager.TrySpawnPrefabToStorage(savedPrefab, parentStorage, slotId))
		{
			stats.RecordSkip(savedPrefab);
			SkipStoragesArray(context);
			return;
		}

		IEntity spawned = parentStorage.Get(slotId);
		if (!spawned)
		{
			stats.RecordSkip(savedPrefab);
			SkipStoragesArray(context);
			return;
		}

		stats.m_iApplied++;
		ApplyStoragesArray(spawned, context, manager, gate, stats);
		ApplyAttachmentsArray(spawned, context, manager, gate, stats);
	}

	//------------------------------------------------------------------------------------------------
	//! Armory extension: draft-sourced nodes carry a bare attachments list instead of a storage tree.
	static void ApplyAttachmentsArray(IEntity owner, LoadContext context, InventoryStorageManagerComponent manager, GRSA_ApplyGate gate, notnull GRSA_ApplyStats stats)
	{
		int attachmentCount = 0;
		if (!context.StartArray("attachments", attachmentCount))
			return;

		array<ResourceName> wanted = {};
		array<int> wantedPins = {};
		for (int i = 0; i < attachmentCount; i++)
		{
			if (!context.StartObject())
				continue;

			ResourceName prefab;
			context.ReadValue("prefab", prefab);
			int pinnedSlot = -1;
			context.ReadValue("slot", pinnedSlot);
			context.EndObject();

			if (!prefab.IsEmpty())
			{
				wanted.Insert(prefab);
				wantedPins.Insert(pinnedSlot);
			}
		}
		context.EndArray();

		if (!owner || !manager)
			return;

		WeaponAttachmentsStorageComponent topStorage = WeaponAttachmentsStorageComponent.Cast(owner.FindComponent(WeaponAttachmentsStorageComponent));
		if (!topStorage)
			return;

		//! Pin bookkeeping mirrors the client preview walker: pinned entries claim their exact
		//! top-level hardpoint, and a pinned prefab resting anywhere else is displaced to its pin.
		map<int, ResourceName> pinBySlot = new map<int, ResourceName>();
		map<ResourceName, int> pinByPrefab = new map<ResourceName, int>();
		foreach (int pinIdx, ResourceName pinPrefab : wanted)
		{
			int pin = wantedPins[pinIdx];
			if (pin < 0 || pin >= topStorage.GetSlotsCount() || pinBySlot.Contains(pin))
				continue;

			pinBySlot.Insert(pin, pinPrefab);
			if (!pinByPrefab.Contains(pinPrefab))
				pinByPrefab.Insert(pinPrefab, pin);
		}

		if (!pinBySlot.IsEmpty())
		{
			int topCount = topStorage.GetSlotsCount();
			for (int i = 0; i < topCount; ++i)
			{
				IEntity current = topStorage.Get(i);
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
					manager.TryDeleteItem(current);
			}
		}

		map<ResourceName, int> wantedCounts = new map<ResourceName, int>();
		foreach (ResourceName prefab : wanted)
		{
			int current = 0;
			wantedCounts.Find(prefab, current);
			wantedCounts.Set(prefab, current + 1);
		}

		PruneAttachmentsSubtree(owner, wantedCounts, manager, 0);

		array<ResourceName> pending = {};
		foreach (ResourceName prefab, int count : wantedCounts)
		{
			for (int n = 0; n < count; ++n)
				pending.Insert(prefab);
		}

		//! Pinned placement first, so first-fit can never steal a reserved hardpoint. Failures fall
		//! through to the generic passes below — the item still applies, just auto-placed.
		if (!pinBySlot.IsEmpty())
		{
			foreach (int pinSlot, ResourceName pinnedPrefab : pinBySlot)
			{
				if (topStorage.Get(pinSlot))
					continue;

				int pendingIdx = pending.Find(pinnedPrefab);
				if (pendingIdx < 0)
					continue;

				if (gate && !gate.Allows(pinnedPrefab))
					continue;

				Resource pinResource = Resource.Load(pinnedPrefab);
				if (!pinResource || !pinResource.IsValid())
					continue;

				if (!manager.TrySpawnPrefabToStorage(pinnedPrefab, topStorage, pinSlot))
					continue;

				stats.m_iApplied++;
				pending.Remove(pendingIdx);
			}
		}

		for (int pass = 0; pass < 4 && !pending.IsEmpty(); ++pass)
		{
			array<ResourceName> stillPending = {};
			foreach (ResourceName prefab : pending)
			{
				if (gate && !gate.Allows(prefab))
				{
					stats.RecordSkip(prefab);
					continue;
				}

				Resource resource = Resource.Load(prefab);
				if (!resource || !resource.IsValid())
				{
					stats.RecordSkip(prefab);
					continue;
				}

				if (SpawnAttachmentIntoSubtree(owner, prefab, manager, 0))
					stats.m_iApplied++;
				else
					stillPending.Insert(prefab);
			}
			pending = stillPending;
		}

		foreach (ResourceName prefab : pending)
		{
			stats.RecordSkip(prefab);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Wanted attached items stay and recurse, everything else is deleted, any depth.
	protected static void PruneAttachmentsSubtree(IEntity owner, notnull map<ResourceName, int> wantedCounts, InventoryStorageManagerComponent manager, int depth)
	{
		if (!owner || depth > 4)
			return;

		WeaponAttachmentsStorageComponent storage = WeaponAttachmentsStorageComponent.Cast(owner.FindComponent(WeaponAttachmentsStorageComponent));
		if (!storage)
			return;

		int slotsCount = storage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			IEntity currentAttachment = storage.Get(i);
			if (!currentAttachment)
				continue;

			ResourceName currentPrefab = SCR_ResourceNameUtils.GetPrefabName(currentAttachment);
			int wantedCount = 0;
			wantedCounts.Find(currentPrefab, wantedCount);
			if (wantedCount > 0)
			{
				wantedCounts.Set(currentPrefab, wantedCount - 1);
				PruneAttachmentsSubtree(currentAttachment, wantedCounts, manager, depth + 1);
				continue;
			}

			manager.TryDeleteItem(currentAttachment);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Tries the owner's own hardpoints first, then every attached item's hardpoints beneath it,
	//! so a light mounts onto its rail once the rail exists.
	protected static bool SpawnAttachmentIntoSubtree(IEntity owner, ResourceName prefab, InventoryStorageManagerComponent manager, int depth)
	{
		if (!owner || depth > 4)
			return false;

		WeaponAttachmentsStorageComponent storage = WeaponAttachmentsStorageComponent.Cast(owner.FindComponent(WeaponAttachmentsStorageComponent));
		if (!storage)
			return false;

		if (manager.TrySpawnPrefabToStorage(prefab, storage, -1))
			return true;

		int slotsCount = storage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			IEntity attached = storage.Get(i);
			if (attached && SpawnAttachmentIntoSubtree(attached, prefab, manager, depth + 1))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool SkipStoragesArray(LoadContext context)
	{
		int storageCount = 0;
		if (!context.StartArray("storages", storageCount))
			return true;

		for (int s = 0; s < storageCount; s++)
		{
			if (!context.StartObject())
				continue;

			string id;
			context.ReadValue("id", id);
			SkipSlotsMap(context);
			context.EndObject();
		}
		return context.EndArray();
	}

	//------------------------------------------------------------------------------------------------
	protected static int SkipSlotsMap(LoadContext context)
	{
		int slotCount = 0;
		if (!context.StartMap("slots", slotCount))
			return 0;

		int dropped = 0;
		for (int i = 0; i < slotCount; i++)
		{
			string idxStr;
			if (!context.ReadMapKey(i, idxStr))
				continue;
			if (!context.StartObject(idxStr))
				continue;

			ResourceName prefab;
			context.ReadValue("prefab", prefab);
			SkipStoragesArray(context);
			context.EndObject();
			dropped++;
		}
		context.EndMap();
		return dropped;
	}

	//------------------------------------------------------------------------------------------------
	protected static BaseInventoryStorageComponent MatchStorage(IEntity owner, notnull array<BaseInventoryStorageComponent> ownerStorages, string savedId)
	{
		if (!owner || savedId.IsEmpty())
			return null;

		foreach (BaseInventoryStorageComponent candidate : ownerStorages)
		{
			if (!candidate)
				continue;

			if (ComponentId(owner, candidate) == savedId)
				return candidate;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectStorages(IEntity owner, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		if (!owner)
			return;

		set<BaseInventoryStorageComponent> seen = new set<BaseInventoryStorageComponent>();
		SCR_PlayerArsenalLoadout.FindStorageComponents(owner, seen);
		foreach (BaseInventoryStorageComponent storage : seen)
		{
			if (storage)
				outStorages.Insert(storage);
		}
	}
}
