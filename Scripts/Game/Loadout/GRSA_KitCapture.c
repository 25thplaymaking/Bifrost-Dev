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

			WeaponAttachmentsStorageComponent attachmentsStorage = WeaponAttachmentsStorageComponent.Cast(attachedEntity.FindComponent(WeaponAttachmentsStorageComponent));
			if (!attachmentsStorage)
				continue;

			int attachmentSlots = attachmentsStorage.GetSlotsCount();
			for (int nSlot = 0; nSlot < attachmentSlots; ++nSlot)
			{
				IEntity attachment = attachmentsStorage.Get(nSlot);
				if (!attachment)
					continue;

				ResourceName attachmentPrefab = SCR_ResourceNameUtils.GetPrefabName(attachment);
				if (!attachmentPrefab.IsEmpty())
					kitWeapon.m_aAttachments.Insert(attachmentPrefab);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Loose carried items: everything that is not equipped clothing, an equipped weapon, or a weapon
	//! attachment. Each item is attributed to the worn container it physically sits in, items in the
	//! character's own pockets land in the automatic bucket (empty container).
	protected static void CaptureExtras(notnull GameEntity character, notnull GRSA_Kit kit)
	{
		SCR_InventoryStorageManagerComponent storageManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!storageManager)
			return;

		set<IEntity> wornEntities = new set<IEntity>();
		CollectSlotEntities(EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent)), wornEntities);

		set<IEntity> equippedEntities = new set<IEntity>();
		foreach (IEntity worn : wornEntities)
		{
			equippedEntities.Insert(worn);
		}
		CollectSlotEntities(EquipedWeaponStorageComponent.Cast(character.FindComponent(EquipedWeaponStorageComponent)), equippedEntities);

		array<IEntity> items = {};
		storageManager.GetItems(items);

		map<string, int> counts = new map<string, int>();
		map<string, ResourceName> pairPrefabs = new map<string, ResourceName>();
		map<string, ResourceName> pairContainers = new map<string, ResourceName>();
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

			ResourceName containerPrefab = ResolveRootContainerPrefab(itemComponent, wornEntities);

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
	//! Walks the item's parent chain outward until it reaches a worn container (its prefab is the
	//! attribution) or the character's own storage (empty = automatic bucket).
	protected static ResourceName ResolveRootContainerPrefab(notnull InventoryItemComponent itemComponent, notnull set<IEntity> wornEntities)
	{
		InventoryStorageSlot slot = itemComponent.GetParentSlot();
		int depth = 0;
		while (slot && depth < 6)
		{
			BaseInventoryStorageComponent storage = slot.GetStorage();
			if (!storage)
				break;

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
			prefabs.Insert(clothing.m_Prefab);
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
