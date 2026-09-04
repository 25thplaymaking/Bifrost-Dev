class GRSA_ApplyContext
{
	SCR_ArsenalComponent m_Arsenal;
	ref GRSA_ArmoryConfig m_Config;
	int m_iPlayerId;
}

//! Server-side kit application. Validates faction policy and supplies up front, then walks the
//! kit through the bridge with the per-item arsenal gate: items the station's ACTIVE arsenal
//! config does not offer (or the player's rank cannot buy) are skipped and reported, never
//! spawned and never able to fail the whole kit.
class GRSA_ApplyService
{
	//------------------------------------------------------------------------------------------------
	static GRSA_ApplyReport ApplyKitFile(notnull GameEntity character, notnull GRSA_KitFile kit, notnull GRSA_ApplyContext ctx)
	{
		GRSA_ApplyReport report = new GRSA_ApplyReport();
		report.m_eStatus = GRSA_EApplyStatus.FAILED_INVALID;

		if (!Replication.IsServer())
		{
			report.m_eStatus = GRSA_EApplyStatus.FAILED_SERVER;
			return report;
		}

		if (kit.IsEmpty())
			return report;

		SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(character);
		if (!chimeraCharacter)
		{
			report.m_eStatus = GRSA_EApplyStatus.FAILED_NO_CHARACTER;
			return report;
		}

		string playerFactionKey;
		SCR_Faction faction;
		if (chimeraCharacter.GetFaction())
		{
			faction = SCR_Faction.Cast(chimeraCharacter.GetFaction());
			playerFactionKey = chimeraCharacter.GetFaction().GetFactionKey();
		}

		if (!PassesFactionPolicy(kit, playerFactionKey, ctx))
		{
			report.m_eStatus = GRSA_EApplyStatus.FAILED_FACTION;
			return report;
		}

		InventoryStorageManagerComponent storageManager = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!storageManager)
		{
			report.m_eStatus = GRSA_EApplyStatus.FAILED_NO_CHARACTER;
			return report;
		}

		string rawJson = kit.GetRawJson();
		if (rawJson.IsEmpty())
			rawJson = kit.ExportToString();
		if (rawJson.IsEmpty())
			return report;

		GRSA_ApplyGate gate = GRSA_ApplyGate.Build(ctx.m_Arsenal, character, ctx.m_Config);

		SCR_EArsenalSupplyCostType costType = SCR_EArsenalSupplyCostType.DEFAULT;
		if (ctx.m_Arsenal)
			costType = ctx.m_Arsenal.GetSupplyCostType();

		float addedCost;
		float removedRefund;
		ComputeDeltaCost(kit, character, faction, costType, gate, addedCost, removedRefund);

		SCR_ResourceConsumer consumer;
		SCR_ResourceGenerator generator;
		if (UsesSupplies(ctx))
		{
			SCR_ResourceComponent resourceComponent = SCR_ResourceComponent.Cast(ctx.m_Arsenal.GetOwner().FindComponent(SCR_ResourceComponent));
			if (resourceComponent)
			{
				consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
				generator = resourceComponent.GetGenerator(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
			}

			float netCost = addedCost;
			if (ctx.m_Config.m_bRefundRemovedItems)
				netCost = addedCost - removedRefund;

			if (consumer && netCost > 0 && consumer.GetAggregatedResourceValue() < netCost)
			{
				report.m_eStatus = GRSA_EApplyStatus.FAILED_SUPPLIES;
				return report;
			}
		}

		GRSA_ApplyStats stats = new GRSA_ApplyStats();
		bool sawNestedStorage = false;
		ApplySlotData(character, rawJson, storageManager, gate, stats, sawNestedStorage);
		ApplyExtras(character, kit, storageManager, gate, stats);

		if (kit.m_aExtras.IsEmpty() && !sawNestedStorage && ctx.m_Config.m_iDefaultMagazineCount > 0)
			GrantDefaultMagazines(character, storageManager, ctx.m_Config.m_iDefaultMagazineCount, gate, stats);

		report.m_iApplied = stats.m_iApplied;
		report.m_iSkipped = stats.m_iSkipped;
		foreach (string sample : stats.m_aSamples)
		{
			report.m_aSkippedNames.Insert(GRSA_CatalogService.GetDisplayName(sample, faction));
		}

		if (report.m_iApplied == 0 && report.m_iSkipped > 0)
		{
			report.m_eStatus = GRSA_EApplyStatus.FAILED_INVALID;
			return report;
		}

		ChargeSupplies(consumer, generator, addedCost, removedRefund, ctx, report);

		if (report.m_iSkipped > 0)
			report.m_eStatus = GRSA_EApplyStatus.PARTIAL;
		else
			report.m_eStatus = GRSA_EApplyStatus.SUCCESS;

		return report;
	}

	//------------------------------------------------------------------------------------------------
	//! Mirrors the base arsenal save-type policy: the station's SCR_EArsenalSaveType decides whether
	//! faction filtering applies, legacy unstamped kits pass, friendly factions pass when allowed.
	protected static bool PassesFactionPolicy(notnull GRSA_KitFile kit, string playerFactionKey, notnull GRSA_ApplyContext ctx)
	{
		if (!GRSA_ArsenalScenarioSettings.Get().m_bRestrictKitFaction)
			return true;

		if (!SCR_GameModeCampaign.GetInstance())
			return true;

		SCR_EArsenalSaveType saveType = SCR_EArsenalSaveType.NO_RESTRICTIONS;
		if (ctx.m_Arsenal)
			saveType = ctx.m_Arsenal.GetArsenalSaveType();

		if (saveType == SCR_EArsenalSaveType.NO_RESTRICTIONS || saveType == SCR_EArsenalSaveType.SAVING_DISABLED)
			return true;

		if (kit.m_iFactionStampVersion < GRSA_KitFile.FACTION_STAMP_PLAYER || kit.m_sFactionKey.IsEmpty())
			return true;

		if (playerFactionKey.IsEmpty())
			return true;

		if (kit.m_sFactionKey == playerFactionKey)
			return true;

		if (saveType == SCR_EArsenalSaveType.FRIENDLY_AND_FACTION_ITEMS_ONLY && IsFriendlyFactionKey(playerFactionKey, kit.m_sFactionKey))
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsFriendlyFactionKey(string playerKey, string kitKey)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return false;

		SCR_Faction playerFaction = SCR_Faction.Cast(factionManager.GetFactionByKey(playerKey));
		if (!playerFaction)
			return false;

		array<Faction> friendlies = {};
		playerFaction.GetFriendlyFactions(friendlies);
		foreach (Faction friendly : friendlies)
		{
			if (friendly && friendly.GetFactionKey() == kitKey)
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplySlotData(notnull GameEntity character, string rawJson, notnull InventoryStorageManagerComponent storageManager, GRSA_ApplyGate gate, notnull GRSA_ApplyStats stats, out bool sawNestedStorage)
	{
		sawNestedStorage = rawJson.IndexOf("\"storages\"") >= 0;

		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromString(rawJson))
			return;

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
		EquipedWeaponStorageComponent weaponStorage = EquipedWeaponStorageComponent.Cast(character.FindComponent(EquipedWeaponStorageComponent));

		set<string> kitKeys = new set<string>();

		int slotCount = 0;
		if (context.StartMap("slotData", slotCount))
		{
			for (int i = 0; i < slotCount; i++)
			{
				string slotKey;
				if (!context.ReadMapKey(i, slotKey))
					continue;
				if (!context.StartObject(slotKey))
					continue;

				kitKeys.Insert(slotKey);

				BaseInventoryStorageComponent parentStorage;
				int slotId = -1;
				if (ResolveTopLevelTarget(character, loadoutStorage, weaponStorage, slotKey, parentStorage, slotId))
				{
					IEntity existing;
					InventoryStorageSlot slot = parentStorage.GetSlot(slotId);
					if (slot)
						existing = slot.GetAttachedEntity();

					GRSA_LoadoutBridge.ApplyNode(existing, context, storageManager, parentStorage, slotId, gate, stats);
				}
				else
				{
					ResourceName prefab;
					context.ReadValue("prefab", prefab);
					stats.RecordSkip(prefab);
					GRSA_LoadoutBridge.SkipStoragesArray(context);
				}

				context.EndObject();
			}
			context.EndMap();
		}

		StripSlotsNotInKit(loadoutStorage, weaponStorage, kitKeys, storageManager);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ResolveTopLevelTarget(notnull GameEntity character, EquipedLoadoutStorageComponent loadoutStorage, EquipedWeaponStorageComponent weaponStorage, string slotKey, out BaseInventoryStorageComponent parentStorage, out int slotId)
	{
		int weaponIdx = GRSA_KitConvert.WeaponIdxFromKey(slotKey);
		if (weaponIdx >= 0)
		{
			if (!weaponStorage || weaponIdx >= weaponStorage.GetSlotsCount())
				return false;

			parentStorage = weaponStorage;
			slotId = weaponIdx;
			return true;
		}

		if (slotKey.IndexOf("Character.") == 0)
			return ResolveEquipmentTarget(character, slotKey, parentStorage, slotId);

		typename area = slotKey.ToType();
		if (!area || !loadoutStorage)
			return false;

		LoadoutSlotInfo slotInfo = loadoutStorage.GetSlotFromArea(area);
		if (!slotInfo)
			return false;

		parentStorage = loadoutStorage;
		slotId = slotInfo.GetID();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ResolveEquipmentTarget(notnull GameEntity character, string compoundKey, out BaseInventoryStorageComponent parentStorage, out int slotId)
	{
		array<string> parts = {};
		compoundKey.Split(".", parts, true);
		if (parts.Count() < 2)
			return false;

		string sourceKey = parts[1];

		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!manager)
			return false;

		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage || storage.Type() != SCR_EquipmentStorageComponent)
				continue;

			int slotsCount = storage.GetSlotsCount();
			for (int i = 0; i < slotsCount; ++i)
			{
				InventoryStorageSlot slot = storage.GetSlot(i);
				if (!slot)
					continue;

				if (slot.GetSourceName() == sourceKey || slot.GetID().ToString() == sourceKey)
				{
					parentStorage = storage;
					slotId = slot.GetID();
					return true;
				}
			}
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Worn clothing and weapons whose slot key is absent from the kit are removed, the kit is the
	//! complete target state.
	protected static void StripSlotsNotInKit(EquipedLoadoutStorageComponent loadoutStorage, EquipedWeaponStorageComponent weaponStorage, notnull set<string> kitKeys, notnull InventoryStorageManagerComponent storageManager)
	{
		if (loadoutStorage)
		{
			int slotsCount = loadoutStorage.GetSlotsCount();
			for (int i = 0; i < slotsCount; ++i)
			{
				LoadoutSlotInfo slotInfo = LoadoutSlotInfo.Cast(loadoutStorage.GetSlot(i));
				if (!slotInfo || !slotInfo.GetAreaType())
					continue;

				IEntity attached = slotInfo.GetAttachedEntity();
				if (!attached)
					continue;

				if (!kitKeys.Contains(slotInfo.GetAreaType().Type().ToString()))
					storageManager.TryDeleteItem(attached);
			}
		}

		if (weaponStorage)
		{
			int slotsCount = weaponStorage.GetSlotsCount();
			for (int i = 0; i < slotsCount; ++i)
			{
				string key = GRSA_KitConvert.WeaponSlotKey(i);
				if (key.IsEmpty() || kitKeys.Contains(key))
					continue;

				InventoryStorageSlot slot = weaponStorage.GetSlot(i);
				if (!slot)
					continue;

				IEntity attached = slot.GetAttachedEntity();
				if (attached)
					storageManager.TryDeleteItem(attached);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Pair-aware top-up: container-routed entries top up against what that container already holds,
	//! automatic entries net against whatever existing stock the routed entries did not claim, so a
	//! re-applied kit never duplicates loose items. Surplus loose cargo is removed before top-up.
	protected static void ApplyExtras(notnull GameEntity character, notnull GRSA_KitFile kit, notnull InventoryStorageManagerComponent storageManager, GRSA_ApplyGate gate, notnull GRSA_ApplyStats stats)
	{
		PruneExtras(character, kit, storageManager, stats);
		if (kit.m_aExtras.IsEmpty())
			return;

		GRSA_Kit current = GRSA_KitCapture.CaptureFromCharacter(character);
		map<string, int> pairCounts = new map<string, int>();
		map<ResourceName, int> totalCounts = new map<ResourceName, int>();
		foreach (GRSA_KitExtra existing : current.m_aExtras)
		{
			pairCounts.Set(existing.m_Prefab + "|" + existing.m_Container, existing.m_iCount);
			int total = 0;
			totalCounts.Find(existing.m_Prefab, total);
			totalCounts.Set(existing.m_Prefab, total + existing.m_iCount);
		}

		map<ResourceName, int> claimed = new map<ResourceName, int>();
		foreach (GRSA_KitExtra extra : kit.m_aExtras)
		{
			if (extra.m_Container.IsEmpty())
				continue;

			int existingHere = 0;
			pairCounts.Find(extra.m_Prefab + "|" + extra.m_Container, existingHere);
			int matched = Math.Min(existingHere, extra.m_iCount);

			int alreadyClaimed = 0;
			claimed.Find(extra.m_Prefab, alreadyClaimed);
			claimed.Set(extra.m_Prefab, alreadyClaimed + matched);

			SpawnExtraCount(character, storageManager, gate, stats, extra, extra.m_iCount - matched);
		}

		foreach (GRSA_KitExtra extra : kit.m_aExtras)
		{
			if (!extra.m_Container.IsEmpty())
				continue;

			int total = 0;
			totalCounts.Find(extra.m_Prefab, total);
			int claimedCount = 0;
			claimed.Find(extra.m_Prefab, claimedCount);

			int matched = Math.Min(Math.Max(0, total - claimedCount), extra.m_iCount);
			claimed.Set(extra.m_Prefab, claimedCount + matched);

			SpawnExtraCount(character, storageManager, gate, stats, extra, extra.m_iCount - matched);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void PruneExtras(notnull GameEntity character, notnull GRSA_KitFile kit, notnull InventoryStorageManagerComponent storageManager, notnull GRSA_ApplyStats stats)
	{
		map<string, int> routedRemaining = new map<string, int>();
		map<ResourceName, int> automaticRemaining = new map<ResourceName, int>();
		foreach (GRSA_KitExtra extra : kit.m_aExtras)
		{
			if (extra.m_iCount <= 0)
				continue;

			if (extra.m_Container.IsEmpty())
			{
				int automaticCount = 0;
				automaticRemaining.Find(extra.m_Prefab, automaticCount);
				automaticRemaining.Set(extra.m_Prefab, automaticCount + extra.m_iCount);
				continue;
			}

			string pairKey = extra.m_Prefab + "|" + extra.m_Container;
			int routedCount = 0;
			routedRemaining.Find(pairKey, routedCount);
			routedRemaining.Set(pairKey, routedCount + extra.m_iCount);
		}

		array<IEntity> items = {};
		array<ResourceName> prefabs = {};
		array<ResourceName> containers = {};
		GRSA_KitCapture.CollectExtraEntities(character, items, prefabs, containers);
		foreach (int i, IEntity item : items)
		{
			ResourceName prefabName = prefabs[i];
			ResourceName containerPrefab = containers[i];
			bool keep = false;
			if (!containerPrefab.IsEmpty())
			{
				string pairKey = prefabName + "|" + containerPrefab;
				int routedCount = 0;
				routedRemaining.Find(pairKey, routedCount);
				if (routedCount > 0)
				{
					routedRemaining.Set(pairKey, routedCount - 1);
					keep = true;
				}
			}

			if (!keep)
			{
				int automaticCount = 0;
				automaticRemaining.Find(prefabName, automaticCount);
				if (automaticCount > 0)
				{
					automaticRemaining.Set(prefabName, automaticCount - 1);
					keep = true;
				}
			}

			if (keep)
				continue;

			if (storageManager.TryDeleteItem(item))
				stats.m_iApplied++;
			else
				stats.RecordSkip(prefabName);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void SpawnExtraCount(notnull GameEntity character, notnull InventoryStorageManagerComponent storageManager, GRSA_ApplyGate gate, notnull GRSA_ApplyStats stats, notnull GRSA_KitExtra extra, int spawnCount)
	{
		for (int n = 0; n < spawnCount; ++n)
		{
			if (gate && !gate.Allows(extra.m_Prefab))
			{
				stats.RecordSkip(extra.m_Prefab);
				continue;
			}

			Resource resource = Resource.Load(extra.m_Prefab);
			if (!resource || !resource.IsValid())
			{
				stats.RecordSkip(extra.m_Prefab);
				continue;
			}

			bool spawned = false;
			array<BaseInventoryStorageComponent> targetStorages = {};
			FindContainerStorages(character, extra.m_Container, targetStorages);
			foreach (BaseInventoryStorageComponent targetStorage : targetStorages)
			{
				if (storageManager.TrySpawnPrefabToStorage(extra.m_Prefab, targetStorage, -1, EStoragePurpose.PURPOSE_DEPOSIT))
				{
					spawned = true;
					break;
				}
			}
			if (!spawned && extra.m_Container.IsEmpty())
				spawned = storageManager.TrySpawnPrefabToStorage(extra.m_Prefab, purpose: EStoragePurpose.PURPOSE_DEPOSIT);

			if (spawned)
				stats.m_iApplied++;
			else
				stats.RecordSkip(extra.m_Prefab);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Cargo stores beneath the requested worn prefab, in their authored order.
	protected static void FindContainerStorages(notnull GameEntity character, ResourceName containerPrefab, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		outStorages.Clear();
		if (containerPrefab.IsEmpty())
			return;

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
		if (!loadoutStorage)
			return;

		int slotsCount = loadoutStorage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = loadoutStorage.GetSlot(i);
			if (!slot)
				continue;

			IEntity worn = slot.GetAttachedEntity();
			if (!worn || SCR_ResourceNameUtils.GetPrefabName(worn) != containerPrefab)
				continue;

			GRSA_ItemIntel.CollectContainerStorages(worn, outStorages);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void GrantDefaultMagazines(notnull GameEntity character, notnull InventoryStorageManagerComponent storageManager, int magazineCount, GRSA_ApplyGate gate, notnull GRSA_ApplyStats stats)
	{
		EquipedWeaponStorageComponent weaponStorage = EquipedWeaponStorageComponent.Cast(character.FindComponent(EquipedWeaponStorageComponent));
		if (!weaponStorage)
			return;

		int slotsCount = weaponStorage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			IEntity weaponEntity = weaponStorage.Get(i);
			if (!weaponEntity)
				continue;

			BaseWeaponComponent weaponComponent = BaseWeaponComponent.Cast(weaponEntity.FindComponent(BaseWeaponComponent));
			if (!weaponComponent)
				continue;

			BaseMagazineComponent magazineComponent = weaponComponent.GetCurrentMagazine();
			if (!magazineComponent)
				continue;

			ResourceName magazinePrefab = SCR_ResourceNameUtils.GetPrefabName(magazineComponent.GetOwner());
			if (magazinePrefab.IsEmpty())
				continue;

			if (gate && !gate.Allows(magazinePrefab))
				continue;

			for (int n = 0; n < magazineCount; ++n)
			{
				if (storageManager.TrySpawnPrefabToStorage(magazinePrefab, purpose: EStoragePurpose.PURPOSE_DEPOSIT))
					stats.m_iApplied++;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool UsesSupplies(notnull GRSA_ApplyContext ctx)
	{
		bool enabled = GRSA_ArsenalScenarioSettings.Get().m_bUseSupplies || ctx.m_Config.m_bUseSupplies;
		return enabled && ctx.m_Arsenal && ctx.m_Arsenal.IsArsenalUsingSupplies();
	}

	//------------------------------------------------------------------------------------------------
	protected static void ComputeDeltaCost(notnull GRSA_KitFile kit, notnull GameEntity character, SCR_Faction faction, SCR_EArsenalSupplyCostType costType, GRSA_ApplyGate gate, out float addedCost, out float removedRefund)
	{
		addedCost = 0;
		removedRefund = 0;

		array<ResourceName> kitPrefabs = {};
		kit.CollectItemPrefabs(kitPrefabs);

		map<ResourceName, int> wanted = new map<ResourceName, int>();
		foreach (ResourceName prefab : kitPrefabs)
		{
			if (gate && !gate.Allows(prefab))
				continue;
			Increment(wanted, prefab, 1);
		}

		GRSA_Kit currentKit = GRSA_KitCapture.CaptureFromCharacter(character);
		map<ResourceName, int> current = new map<ResourceName, int>();
		foreach (GRSA_KitClothing clothing : currentKit.m_aClothings)
			Increment(current, clothing.m_Prefab, 1);
		foreach (GRSA_KitWeapon weapon : currentKit.m_aWeapons)
		{
			Increment(current, weapon.m_Prefab, 1);
			foreach (ResourceName attachment : weapon.m_aAttachments)
				Increment(current, attachment, 1);
		}
		foreach (GRSA_KitExtra extra : currentKit.m_aExtras)
			Increment(current, extra.m_Prefab, extra.m_iCount);

		foreach (ResourceName prefab, int wantedCount : wanted)
		{
			int currentCount = 0;
			current.Find(prefab, currentCount);
			int addedCount = wantedCount - currentCount;
			if (addedCount <= 0)
				continue;

			SCR_ArsenalItem itemData = GRSA_CatalogService.FindArsenalItemData(prefab, faction);
			if (itemData)
				addedCost += itemData.GetSupplyCost(costType) * addedCount;
		}

		foreach (ResourceName prefab, int currentCount : current)
		{
			int wantedCount = 0;
			wanted.Find(prefab, wantedCount);
			int removedCount = currentCount - wantedCount;
			if (removedCount <= 0)
				continue;

			SCR_ArsenalItem itemData = GRSA_CatalogService.FindArsenalItemData(prefab, faction);
			if (itemData)
				removedRefund += itemData.GetSupplyRefundAmount(costType) * removedCount;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void Increment(notnull map<ResourceName, int> counts, ResourceName prefab, int amount)
	{
		if (prefab.IsEmpty())
			return;

		int current = 0;
		counts.Find(prefab, current);
		counts.Set(prefab, current + amount);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ChargeSupplies(SCR_ResourceConsumer consumer, SCR_ResourceGenerator generator, float addedCost, float removedRefund, notnull GRSA_ApplyContext ctx, notnull GRSA_ApplyReport report)
	{
		if (!consumer)
			return;

		if (ctx.m_Config.m_bRefundRemovedItems && generator && removedRefund > 0)
			generator.RequestGeneration(removedRefund);

		if (addedCost > 0)
		{
			consumer.RequestConsumtion(addedCost);
			report.m_fSuppliesCharged = addedCost - removedRefund;
		}
	}
}
