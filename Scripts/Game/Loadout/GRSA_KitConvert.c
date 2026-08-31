//! Character capture, draft conversion and slot-key mapping. Slot keys are format-compatible
//! with the established save-mod kits: weapon slots by name, clothing by loadout area typename,
//! equipment slots as Character.<source>.
class GRSA_KitConvert
{
	//------------------------------------------------------------------------------------------------
	static string WeaponSlotKey(int idx)
	{
		switch (idx)
		{
			case 0: return "Primary";
			case 1: return "Secondary";
			case 2: return "Sidearm";
			case 3: return "Grenade";
			case 4: return "Throwable";
		}
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static int WeaponIdxFromKey(string slotKey)
	{
		if (slotKey == "Primary")
			return 0;
		if (slotKey == "Secondary")
			return 1;
		if (slotKey == "Sidearm")
			return 2;
		if (slotKey == "Grenade")
			return 3;
		if (slotKey == "Throwable")
			return 4;
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	static string ClothingKeyForSlot(notnull EquipedLoadoutStorageComponent loadoutStorage, int slotIdx)
	{
		LoadoutSlotInfo slotInfo = LoadoutSlotInfo.Cast(loadoutStorage.GetSlot(slotIdx));
		if (!slotInfo || !slotInfo.GetAreaType())
			return string.Empty;

		return slotInfo.GetAreaType().Type().ToString();
	}

	//------------------------------------------------------------------------------------------------
	//! Full-fidelity snapshot of the worn character, nested storages included.
	static GRSA_KitFile CaptureFromCharacter(notnull GameEntity character, string kitName)
	{
		GRSA_KitFile kit = new GRSA_KitFile();
		kit.m_sName = kitName;
		kit.m_sKit = "kits";

		EquipedWeaponStorageComponent weaponStorage = EquipedWeaponStorageComponent.Cast(character.FindComponent(EquipedWeaponStorageComponent));
		if (weaponStorage)
		{
			int weaponSlotCount = weaponStorage.GetSlotsCount();
			for (int wIdx = 0; wIdx < weaponSlotCount; wIdx++)
			{
				string wKey = WeaponSlotKey(wIdx);
				if (wKey.IsEmpty())
					continue;

				InventoryStorageSlot wSlot = weaponStorage.GetSlot(wIdx);
				if (!wSlot)
					continue;

				IEntity weapon = wSlot.GetAttachedEntity();
				if (weapon)
					kit.QueueCaptureSlot(wKey, weapon);
			}
		}

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
		if (loadoutStorage)
		{
			array<InventoryItemComponent> wornItems = {};
			loadoutStorage.GetOwnedItems(wornItems, false);
			foreach (InventoryItemComponent itemComponent : wornItems)
			{
				if (!itemComponent)
					continue;

				IEntity attached = itemComponent.GetOwner();
				LoadoutSlotInfo slotInfo = LoadoutSlotInfo.Cast(itemComponent.GetParentSlot());
				if (!attached || !slotInfo || !slotInfo.GetAreaType())
					continue;

				kit.QueueCaptureSlot(slotInfo.GetAreaType().Type().ToString(), attached);
			}
		}

		CaptureEquipmentSlots(character, loadoutStorage, kit);

		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (inventoryManager)
			kit.m_fTotalWeight = Math.Round(inventoryManager.GetTotalWeightOfAllStorages() * 100) * 0.01;

		SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(character);
		if (chimeraCharacter && chimeraCharacter.GetFaction())
			kit.StampFaction(chimeraCharacter.GetFaction().GetFactionKey());

		kit.StampSavedNow();
		return kit;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CaptureEquipmentSlots(notnull GameEntity character, EquipedLoadoutStorageComponent loadoutStorage, notnull GRSA_KitFile kit)
	{
		array<BaseInventoryStorageComponent> storages = {};
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!manager)
			return;

		manager.GetStorages(storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage || storage == loadoutStorage)
				continue;
			if (EquipedWeaponStorageComponent.Cast(storage))
				continue;
			if (storage.Type() != SCR_EquipmentStorageComponent)
				continue;

			array<InventoryItemComponent> equipItems = {};
			storage.GetOwnedItems(equipItems, false);
			foreach (InventoryItemComponent itemComponent : equipItems)
			{
				if (!itemComponent)
					continue;

				IEntity attached = itemComponent.GetOwner();
				InventoryStorageSlot parentSlot = itemComponent.GetParentSlot();
				if (!attached || !parentSlot)
					continue;

				SCR_ItemAttributeCollection attributes = SCR_ItemAttributeCollection.Cast(itemComponent.GetAttributes());
				if (attributes && attributes.GetCommonType() == ECommonItemType.IDENTITY_ITEM)
					continue;

				string sourceName = parentSlot.GetSourceName();
				string charKey;
				if (!sourceName.IsEmpty())
					charKey = string.Format("Character.%1", sourceName);
				else
					charKey = string.Format("Character.%1", parentSlot.GetID());
				kit.QueueCaptureSlot(charKey, attached);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Draft picks become bare prefab nodes with attachment lists, defaults fill the rest on spawn.
	static GRSA_KitFile FromDraft(notnull GRSA_Kit draft, notnull GameEntity character, string kitName)
	{
		GRSA_KitFile kit = new GRSA_KitFile();
		kit.m_sName = kitName;
		kit.m_sKit = "kits";

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
		foreach (GRSA_KitClothing clothing : draft.m_aClothings)
		{
			if (!loadoutStorage)
				break;

			string key = ClothingKeyForSlot(loadoutStorage, clothing.m_iSlotIdx);
			if (!key.IsEmpty())
				kit.SetOverrideSlot(key, clothing.m_Prefab);
		}

		foreach (GRSA_KitWeapon weapon : draft.m_aWeapons)
		{
			string key = WeaponSlotKey(weapon.m_iSlotIdx);
			if (key.IsEmpty())
				continue;

			weapon.EnsurePins();
			kit.SetOverrideSlot(key, weapon.m_Prefab, weapon.m_aAttachments, weapon.m_bForceReplace, weapon.m_aAttachmentSlots);
		}

		foreach (GRSA_KitExtra extra : draft.m_aExtras)
		{
			GRSA_KitExtra copy = new GRSA_KitExtra();
			copy.m_Prefab = extra.m_Prefab;
			copy.m_iCount = extra.m_iCount;
			copy.m_Container = extra.m_Container;
			kit.m_aExtras.Insert(copy);
		}

		SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(character);
		if (chimeraCharacter && chimeraCharacter.GetFaction())
			kit.StampFaction(chimeraCharacter.GetFaction().GetFactionKey());

		kit.m_iRequiredRank = draft.m_iRequiredRank;
		kit.m_fSupplyCost = draft.m_fSuppliesCost;
		kit.StampSavedNow();
		return kit;
	}

	//------------------------------------------------------------------------------------------------
	//! Top-level flatten for draft editing. Nested pouch contents stay in the kit file's raw tree.
	static void FlattenToDraft(notnull GRSA_KitFile kit, GameEntity character, notnull GRSA_Kit outDraft)
	{
		outDraft.m_aClothings.Clear();
		outDraft.m_aWeapons.Clear();
		outDraft.m_aExtras.Clear();
		outDraft.m_sName = kit.m_sName;
		outDraft.m_sFactionKey = kit.m_sFactionKey;

		EquipedLoadoutStorageComponent loadoutStorage;
		if (character)
			loadoutStorage = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));

		int slotCount = kit.GetSlotCount();
		for (int i = 0; i < slotCount; ++i)
		{
			string key = kit.GetSlotKeyAt(i);
			string prefab = kit.GetSlotPrefabAt(i);
			if (prefab.IsEmpty())
				continue;

			int weaponIdx = WeaponIdxFromKey(key);
			if (weaponIdx >= 0)
			{
				outDraft.SetWeapon(weaponIdx, prefab);
				GRSA_KitWeapon draftWeapon = outDraft.FindWeapon(weaponIdx);
				if (draftWeapon)
				{
					kit.GetSlotAttachments(key, draftWeapon.m_aAttachments, draftWeapon.m_aAttachmentSlots);
					if (draftWeapon.m_aAttachments.IsEmpty())
						GRSA_ItemIntel.GetDefaultAttachments(prefab, draftWeapon.m_aAttachments);
					draftWeapon.EnsurePins();
				}
				continue;
			}

			if (key.IndexOf("Character.") == 0)
				continue;

			int clothingSlot = FindClothingSlotForKey(loadoutStorage, key);
			if (clothingSlot >= 0)
				outDraft.SetClothing(clothingSlot, prefab);
		}

		foreach (GRSA_KitExtra extra : kit.m_aExtras)
		{
			outDraft.SetExtra(extra.m_Prefab, extra.m_iCount, extra.m_Container);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Display sidecars, recomputed authoritatively server-side at apply time.
	static void ComputeSidecars(notnull GRSA_KitFile kit, SCR_Faction faction, SCR_EArsenalSupplyCostType costType)
	{
		float cost = 0;
		int requiredRank = 0;

		array<ResourceName> prefabs = {};
		kit.CollectItemPrefabs(prefabs);
		foreach (ResourceName prefab : prefabs)
		{
			SCR_ArsenalItem itemData = GRSA_CatalogService.FindArsenalItemData(prefab, faction);
			if (!itemData)
				continue;

			cost += itemData.GetSupplyCost(costType);
			requiredRank = Math.Max(requiredRank, itemData.GetRequiredRank());
		}

		kit.m_fSupplyCost = cost;
		kit.m_iRequiredRank = requiredRank;
	}

	//------------------------------------------------------------------------------------------------
	static int FindClothingSlotForKey(EquipedLoadoutStorageComponent loadoutStorage, string areaKey)
	{
		if (!loadoutStorage || areaKey.IsEmpty())
			return -1;

		typename area = areaKey.ToType();
		if (!area)
			return -1;

		int slotsCount = loadoutStorage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			LoadoutSlotInfo slotInfo = LoadoutSlotInfo.Cast(loadoutStorage.GetSlot(i));
			if (!slotInfo || !slotInfo.GetAreaType())
				continue;

			if (slotInfo.GetAreaType().Type() == area)
				return i;
		}
		return -1;
	}
}
