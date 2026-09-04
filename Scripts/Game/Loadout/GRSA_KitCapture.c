class GRSA_KitCapture
{
	//------------------------------------------------------------------------------------------------
	static GRSA_Kit CaptureFromCharacter(notnull GameEntity character, SCR_EArsenalSupplyCostType costType = SCR_EArsenalSupplyCostType.DEFAULT)
	{
		GRSA_Kit kit = new GRSA_Kit();

		SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(character);
		SCR_Faction faction;
		if (chimeraCharacter && chimeraCharacter.GetFaction())
		{
			faction = SCR_Faction.Cast(chimeraCharacter.GetFaction());
			kit.m_sFactionKey = chimeraCharacter.GetFaction().GetFactionKey();
		}

		CaptureClothings(character, kit);
		CaptureWeapons(character, kit);
		CaptureExtras(character, kit);

		BuildSummaries(kit, faction);
		ComputeCost(kit, faction, costType);
		kit.StampSavedNow();
		return kit;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CaptureClothings(notnull GameEntity character, notnull GRSA_Kit kit)
	{
		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
		if (!loadoutStorage)
			return;

		int slotsCount = loadoutStorage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = loadoutStorage.GetSlot(i);
			if (!slot)
				continue;

			IEntity attachedEntity = slot.GetAttachedEntity();
			if (!attachedEntity)
				continue;

			ResourceName prefabName = SCR_ResourceNameUtils.GetPrefabName(attachedEntity);
			if (prefabName.IsEmpty())
				continue;

			kit.SetClothing(i, prefabName);
			GRSA_KitClothing kitClothing = kit.FindClothing(i);
			if (kitClothing)
				CaptureAttachments(attachedEntity, kitClothing.m_aAttachments, kitClothing.m_aAttachmentSlots);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CaptureWeapons(notnull GameEntity character, notnull GRSA_Kit kit)
	{
		EquipedWeaponStorageComponent weaponStorage = EquipedWeaponStorageComponent.Cast(character.FindComponent(EquipedWeaponStorageComponent));
		if (!weaponStorage)
			return;

		int slotsCount = weaponStorage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = weaponStorage.GetSlot(i);
			if (!slot)
				continue;

			IEntity attachedEntity = slot.GetAttachedEntity();
			if (!attachedEntity)
				continue;

			ResourceName prefabName = SCR_ResourceNameUtils.GetPrefabName(attachedEntity);
			if (prefabName.IsEmpty())
				continue;

			kit.SetWeapon(i, prefabName);
			GRSA_KitWeapon kitWeapon = kit.FindWeapon(i);
			if (!kitWeapon)
				continue;

			CaptureAttachments(attachedEntity, kitWeapon.m_aAttachments, kitWeapon.m_aAttachmentSlots);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Captures the complete attachment subtree and preserves exact top-level hardpoints.
	protected static void CaptureAttachments(notnull IEntity owner, notnull array<ResourceName> outAttachments, notnull array<int> outPins)
	{
		outAttachments.Clear();
		outPins.Clear();

		WeaponAttachmentsStorageComponent storage = WeaponAttachmentsStorageComponent.Cast(owner.FindComponent(WeaponAttachmentsStorageComponent));
		if (!storage)
		{
			GRSA_ItemIntel.CollectSubtreeAttachments(owner, outAttachments);
			for (int i = 0; i < outAttachments.Count(); ++i)
				outPins.Insert(-1);
			return;
		}

		int slotsCount = storage.GetSlotsCount();
		for (int slotIdx = 0; slotIdx < slotsCount; ++slotIdx)
		{
			IEntity attachment = storage.Get(slotIdx);
			if (!attachment)
				continue;

			ResourceName prefab = SCR_ResourceNameUtils.GetPrefabName(attachment);
			if (prefab.IsEmpty())
				continue;

			outAttachments.Insert(prefab);
			outPins.Insert(slotIdx);
		}

		for (int slotIdx = 0; slotIdx < slotsCount; ++slotIdx)
		{
			IEntity attachment = storage.Get(slotIdx);
			if (!attachment)
				continue;

			int nestedStart = outAttachments.Count();
			GRSA_ItemIntel.CollectSubtreeAttachments(attachment, outAttachments);
			for (int nestedIdx = nestedStart; nestedIdx < outAttachments.Count(); ++nestedIdx)
				outPins.Insert(-1);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Loose carried items: everything that is not equipped clothing, an equipped weapon, or a weapon
	//! attachment. Each item is attributed to the worn container it physically sits in, items in the
	//! character's own pockets land in the automatic bucket (empty container).
	protected static void CaptureExtras(notnull GameEntity character, notnull GRSA_Kit kit)
	{
		map<string, int> counts = new map<string, int>();
		map<string, ResourceName> pairPrefabs = new map<string, ResourceName>();
		map<string, ResourceName> pairContainers = new map<string, ResourceName>();
		array<IEntity> items = {};
		array<ResourceName> prefabs = {};
		array<ResourceName> containers = {};
		CollectExtraEntities(character, items, prefabs, containers);
		for (int i = 0; i < items.Count(); ++i)
		{
			ResourceName prefabName = prefabs[i];
			ResourceName containerPrefab = containers[i];
			string pairKey = prefabName + "|" + containerPrefab;
			int current = 0;
			counts.Find(pairKey, current);
			counts.Set(pairKey, current + 1);
			pairPrefabs.Set(pairKey, prefabName);
			pairContainers.Set(pairKey, containerPrefab);
		}

		foreach (string pairKey, int count : counts)
		{
			kit.SetExtra(pairPrefabs.Get(pairKey), count, pairContainers.Get(pairKey));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Enumerates only loose cargo that the kit can add or remove.
	static void CollectExtraEntities(
		notnull GameEntity character,
		notnull array<IEntity> outItems,
		notnull array<ResourceName> outPrefabs,
		notnull array<ResourceName> outContainers)
	{
		outItems.Clear();
		outPrefabs.Clear();
		outContainers.Clear();

		SCR_InventoryStorageManagerComponent storageManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!storageManager)
			return;

		set<IEntity> wornEntities = new set<IEntity>();
		CollectSlotEntities(EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent)), wornEntities);
		set<BaseInventoryStorageComponent> wornCargoStorages = new set<BaseInventoryStorageComponent>();
		foreach (IEntity worn : wornEntities)
		{
			array<BaseInventoryStorageComponent> cargoStorages = {};
			GRSA_ItemIntel.CollectContainerStorages(worn, cargoStorages);
			foreach (BaseInventoryStorageComponent cargoStorage : cargoStorages)
				wornCargoStorages.Insert(cargoStorage);
		}

		set<IEntity> equippedEntities = new set<IEntity>();
		foreach (IEntity worn : wornEntities)
		{
			equippedEntities.Insert(worn);
		}
		CollectSlotEntities(EquipedWeaponStorageComponent.Cast(character.FindComponent(EquipedWeaponStorageComponent)), equippedEntities);

		array<IEntity> items = {};
		storageManager.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!item || equippedEntities.Contains(item))
				continue;

			InventoryItemComponent itemComponent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (!itemComponent)
				continue;

			InventoryStorageSlot parentSlot = itemComponent.GetParentSlot();
			if (parentSlot && WeaponAttachmentsStorageComponent.Cast(parentSlot.GetStorage()))
				continue;

			ResourceName prefabName = SCR_ResourceNameUtils.GetPrefabName(item);
			if (prefabName.IsEmpty())
				continue;

			bool insideCargo;
			ResourceName containerPrefab = ResolveRootContainerPrefab(itemComponent, wornEntities, wornCargoStorages, insideCargo);
			if (!containerPrefab.IsEmpty() && !insideCargo)
				continue;

			outItems.Insert(item);
			outPrefabs.Insert(prefabName);
			outContainers.Insert(containerPrefab);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Walks the item's parent chain outward until it reaches a worn container (its prefab is the
	//! attribution) or the character's own storage (empty = automatic bucket).
	protected static ResourceName ResolveRootContainerPrefab(
		notnull InventoryItemComponent itemComponent,
		notnull set<IEntity> wornEntities,
		notnull set<BaseInventoryStorageComponent> wornCargoStorages,
		out bool insideCargo)
	{
		insideCargo = false;
		InventoryStorageSlot slot = itemComponent.GetParentSlot();
		int depth = 0;
		while (slot && depth < 6)
		{
			BaseInventoryStorageComponent storage = slot.GetStorage();
			if (!storage)
				break;
			if (wornCargoStorages.Contains(storage))
				insideCargo = true;

			IEntity owner = storage.GetOwner();
			if (!owner)
				break;

			if (wornEntities.Contains(owner))
				return SCR_ResourceNameUtils.GetPrefabName(owner);

			InventoryItemComponent ownerItem = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
			if (!ownerItem)
				break;

			slot = ownerItem.GetParentSlot();
			depth++;
		}
		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectSlotEntities(BaseInventoryStorageComponent storage, notnull set<IEntity> outEntities)
	{
		if (!storage)
			return;

		int slotsCount = storage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			if (!slot)
				continue;

			IEntity attachedEntity = slot.GetAttachedEntity();
			if (attachedEntity)
				outEntities.Insert(attachedEntity);
		}
	}

	//------------------------------------------------------------------------------------------------
	static void BuildSummaries(notnull GRSA_Kit kit, SCR_Faction faction)
	{
		string weaponsSummary;
		foreach (GRSA_KitWeapon weapon : kit.m_aWeapons)
		{
			if (!weaponsSummary.IsEmpty())
				weaponsSummary += ", ";
			weaponsSummary += GRSA_CatalogService.GetDisplayName(weapon.m_Prefab, faction);
		}
		kit.m_sSummaryWeapons = weaponsSummary;

		string clothingSummary;
		int clothingListed = 0;
		foreach (GRSA_KitClothing clothing : kit.m_aClothings)
		{
			if (clothingListed >= 3)
				break;

			if (!clothingSummary.IsEmpty())
				clothingSummary += ", ";
			clothingSummary += GRSA_CatalogService.GetDisplayName(clothing.m_Prefab, faction);
			clothingListed++;
		}
		kit.m_sSummaryClothing = clothingSummary;
	}

	//------------------------------------------------------------------------------------------------
	static void ComputeCost(notnull GRSA_Kit kit, SCR_Faction faction, SCR_EArsenalSupplyCostType costType)
	{
		float cost = 0;
		int requiredRank = 0;

		array<ResourceName> prefabs = {};
		foreach (GRSA_KitClothing clothing : kit.m_aClothings)
		{
			prefabs.Insert(clothing.m_Prefab);
			foreach (ResourceName attachment : clothing.m_aAttachments)
				prefabs.Insert(attachment);
		}
		foreach (GRSA_KitWeapon weapon : kit.m_aWeapons)
		{
			prefabs.Insert(weapon.m_Prefab);
			foreach (ResourceName attachment : weapon.m_aAttachments)
				prefabs.Insert(attachment);
		}

		foreach (ResourceName prefabName : prefabs)
		{
			SCR_ArsenalItem itemData = GRSA_CatalogService.FindArsenalItemData(prefabName, faction);
			if (!itemData)
				continue;

			cost += itemData.GetSupplyCost(costType);
			requiredRank = Math.Max(requiredRank, itemData.GetRequiredRank());
		}

		foreach (GRSA_KitExtra extra : kit.m_aExtras)
		{
			SCR_ArsenalItem itemData = GRSA_CatalogService.FindArsenalItemData(extra.m_Prefab, faction);
			if (!itemData)
				continue;

			cost += itemData.GetSupplyCost(costType) * extra.m_iCount;
			requiredRank = Math.Max(requiredRank, itemData.GetRequiredRank());
		}

		kit.m_fSuppliesCost = cost;
		kit.m_iRequiredRank = requiredRank;
	}
}
