//! Client-side session backend for the armory editor: owns the draft loadout, kit persistence
//! wrappers, container routing, availability preview, and apply dispatch. Pure service layer —
//! holds no widgets, schedules no UI behavior. Screens subscribe to the change events; the
//! entry-point action opens the menu itself so this class never references UI types.
enum BIA_EExtraChangeResult
{
	INVALID,
	ADDED,
	REMOVED,
	EMPTY,
	NO_STORAGE,
	INCOMPATIBLE,
	WEIGHT_LIMIT,
	VOLUME_LIMIT
}

class BIA_DraftService
{
	protected static const int APPLY_DEBOUNCE_MS = 2100;
	protected static ref BIA_DraftService s_Instance;

	//! Character being edited. Bifrost GM sessions may target a replicated AI or another player;
	//! player Arsenal Access sessions target the locally controlled character.
	protected IEntity m_EditTarget;
	protected bool m_bBifrostSession;
	SCR_ArsenalComponent m_Arsenal;
	ref BIA_ArmoryConfig m_Config;
	ref BIA_Kit m_Draft;

	//! Full-fidelity kit backing the draft, kept while the draft is untouched so apply preserves
	//! nested contents.
	ref BIA_KitFile m_LoadedKitFile;

	//! True while the draft differs from the last applied or captured character state.
	bool m_bDraftDirty;

	//! Container prefab new inventory items are routed into, empty means automatic placement.
	protected ResourceName m_TargetContainer;

	protected float m_fLastApplySentMs;

	ref ScriptInvoker m_OnDraftChanged = new ScriptInvoker();
	ref ScriptInvoker m_OnKitListChanged = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	static BIA_DraftService Get()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Bifrost entry point shared by placed Arsenal Access and GM Edit Loadout. The target remains
	//! explicit through preview, draft capture, and the server request; no client-selected target is
	//! trusted by the server without the existing Bifrost access/GM authorization checks.
	static BIA_DraftService BeginForTarget(IEntity target, bool bifrostSession = true)
	{
		BIA_DraftService service = new BIA_DraftService();
		service.m_EditTarget = target;
		service.m_bBifrostSession = bifrostSession;
		service.m_Config = BIA_ConfigHolder.GetDefault();

		s_Instance = service;
		BIA_CatalogService.ClearSessionCache();
		BIA_ItemIntel.ClearSessionCache();
		service.RebuildDraftFromCharacter();
		if (bifrostSession && target)
			DCO_ArsenalServer.Route(DCO_ArsenalServer.VERB_HOLD, target, string.Empty);
		return service;
	}

	//------------------------------------------------------------------------------------------------
	static void Clear()
	{
		if (s_Instance && s_Instance.m_bBifrostSession && s_Instance.m_EditTarget)
			DCO_ArsenalServer.Route(DCO_ArsenalServer.VERB_RELEASE, s_Instance.m_EditTarget, string.Empty);
		s_Instance = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Rank locks are a game-mode-wide arsenal setting, surfaced here so screens never read the
	//! arsenal manager directly.
	static bool RanksActive()
	{
		bool requested = BIA_ArsenalScenarioSettings.Get().m_bUseRankLocks;
		BIA_DraftService service = Get();
		if (service && service.m_Config && service.m_Config.m_bUseRankLocks)
			requested = true;
		if (!requested)
			return false;

		SCR_ArsenalManagerComponent arsenalManager;
		if (SCR_ArsenalManagerComponent.GetArsenalManager(arsenalManager))
			return arsenalManager.AreItemsRankLocked();

		return false;
	}

	//------------------------------------------------------------------------------------------------
	GameEntity GetLocalCharacter()
	{
		if (m_EditTarget)
			return GameEntity.Cast(m_EditTarget);

		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return null;

		return GameEntity.Cast(controller.GetControlledEntity());
	}

	//------------------------------------------------------------------------------------------------
	SCR_Faction GetLocalFaction()
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(GetLocalCharacter());
		if (!character)
			return null;

		return SCR_Faction.Cast(character.GetFaction());
	}

	//------------------------------------------------------------------------------------------------
	//! Faction context for names and scoped catalogs. All Factions and Player Faction use the edited
	//! character; Station Inventory uses its assigned faction when one exists.
	SCR_Faction GetBrowseFaction()
	{
		if (BIA_ArsenalScenarioSettings.Get().m_eCatalogScope == BIA_ECatalogScope.STATION_ARSENAL && m_Arsenal)
			return m_Arsenal.GetAssignedFaction();

		return GetLocalFaction();
	}

	//------------------------------------------------------------------------------------------------
	SCR_EArsenalSupplyCostType GetCostType()
	{
		if (m_Arsenal)
			return m_Arsenal.GetSupplyCostType();

		return SCR_EArsenalSupplyCostType.DEFAULT;
	}

	//------------------------------------------------------------------------------------------------
	bool UsesSupplies()
	{
		bool enabled = BIA_ArsenalScenarioSettings.Get().m_bUseSupplies;
		if (m_Config && m_Config.m_bUseSupplies)
			enabled = true;
		return enabled && m_Arsenal && m_Arsenal.IsArsenalUsingSupplies();
	}

	//------------------------------------------------------------------------------------------------
	bool CanChangeSavedKits()
	{
		return BIA_ArsenalScenarioSettings.Get().m_bAllowKitChanges;
	}

	//------------------------------------------------------------------------------------------------
	void RebuildDraftFromCharacter()
	{
		GameEntity character = GetLocalCharacter();
		if (character)
			m_Draft = BIA_KitCapture.CaptureFromCharacter(character, GetCostType());
		else
			m_Draft = new BIA_Kit();

		m_LoadedKitFile = null;
		m_bDraftDirty = false;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	void NotifyDraftChanged()
	{
		if (m_Draft)
		{
			BIA_KitCapture.BuildSummaries(m_Draft, GetLocalFaction());
			BIA_KitCapture.ComputeCost(m_Draft, GetLocalFaction(), GetCostType());
		}
		m_OnDraftChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	//! Draft edits detach the loaded kit file, the flat model is then authoritative on apply.
	void SetDraftClothing(int slotIdx, ResourceName prefab)
	{
		if (!m_Draft)
			return;

		BIA_KitClothing existing = m_Draft.FindClothing(slotIdx);
		bool prefabChanged = !existing || existing.m_Prefab != prefab;
		m_Draft.SetClothing(slotIdx, prefab);

		BIA_KitClothing clothing = m_Draft.FindClothing(slotIdx);
		if (clothing && prefabChanged && !prefab.IsEmpty())
		{
			BIA_ItemIntel.GetDefaultAttachments(prefab, clothing.m_aAttachments, clothing.m_aAttachmentSlots);
			clothing.EnsurePins();
		}

		if (!m_TargetContainer.IsEmpty())
		{
			array<ResourceName> containers = {};
			GetDraftContainers(containers);
			if (containers.Find(m_TargetContainer) < 0)
				m_TargetContainer = ResourceName.Empty;
		}
		m_LoadedKitFile = null;
		m_bDraftDirty = true;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Replaces exactly one runtime-supported hardpoint on an equipped clothing item.
	void SwapDraftClothingAttachment(int clothingSlotIdx, ResourceName oldPrefab, ResourceName newPrefab, int hardpointSlot)
	{
		if (!m_Draft)
			return;

		BIA_KitClothing clothing = m_Draft.FindClothing(clothingSlotIdx);
		if (!clothing)
			return;

		clothing.EnsurePins();
		if (!oldPrefab.IsEmpty())
		{
			int existing = -1;
			foreach (int i, ResourceName attachment : clothing.m_aAttachments)
			{
				if (attachment == oldPrefab && clothing.m_aAttachmentSlots[i] == hardpointSlot)
				{
					existing = i;
					break;
				}
			}
			if (existing < 0)
				existing = clothing.m_aAttachments.Find(oldPrefab);
			if (existing >= 0)
			{
				clothing.m_aAttachments.Remove(existing);
				clothing.m_aAttachmentSlots.Remove(existing);
			}
		}

		if (!newPrefab.IsEmpty())
		{
			clothing.m_aAttachments.Insert(newPrefab);
			clothing.m_aAttachmentSlots.Insert(hardpointSlot);
		}

		m_LoadedKitFile = null;
		m_bDraftDirty = true;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Equipping seeds the draft with the prefab's factory attachments, variant weapons keep their
	//! parts and the attachment list becomes the complete authoritative set for sync and apply.
	void SetDraftWeapon(int slotIdx, ResourceName prefab)
	{
		if (!m_Draft)
			return;

		BIA_KitWeapon existing = m_Draft.FindWeapon(slotIdx);
		bool prefabChanged = !existing || existing.m_Prefab != prefab;

		m_Draft.SetWeapon(slotIdx, prefab);

		BIA_KitWeapon weapon = m_Draft.FindWeapon(slotIdx);
		if (weapon)
		{
			if (prefabChanged && !prefab.IsEmpty())
			{
				BIA_ItemIntel.GetDefaultAttachments(prefab, weapon.m_aAttachments);
				weapon.m_bForceReplace = false;
			}
			else if (!prefabChanged)
			{
				weapon.m_bForceReplace = true;
			}
			weapon.EnsurePins();
		}

		m_LoadedKitFile = null;
		m_bDraftDirty = true;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	void ToggleDraftAttachment(int weaponSlotIdx, ResourceName attachmentPrefab)
	{
		if (!m_Draft)
			return;

		BIA_KitWeapon weapon = m_Draft.FindWeapon(weaponSlotIdx);
		if (!weapon)
			return;

		weapon.EnsurePins();
		int existing = weapon.m_aAttachments.Find(attachmentPrefab);
		if (existing >= 0)
		{
			weapon.m_aAttachments.Remove(existing);
			weapon.m_aAttachmentSlots.Remove(existing);
		}
		else
		{
			EvictSameTypeAttachments(weapon, attachmentPrefab);
			weapon.m_aAttachments.Insert(attachmentPrefab);
			weapon.m_aAttachmentSlots.Insert(-1);
		}

		m_LoadedKitFile = null;
		m_bDraftDirty = true;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! One attachment per mount type: picking a second optic swaps it in place of the first, the
	//! physical slot can only hold one. Same-prefab toggling is handled by the caller.
	protected void EvictSameTypeAttachments(notnull BIA_KitWeapon weapon, ResourceName attachmentPrefab)
	{
		string typeName = BIA_ItemIntel.GetAttachmentTypeName(attachmentPrefab);
		if (typeName.IsEmpty())
			return;

		weapon.EnsurePins();
		for (int i = weapon.m_aAttachments.Count() - 1; i >= 0; --i)
		{
			if (BIA_ItemIntel.GetAttachmentTypeName(weapon.m_aAttachments[i]) != typeName)
				continue;

			weapon.m_aAttachments.Remove(i);
			weapon.m_aAttachmentSlots.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Slot-targeted swap: replaces exactly the hardpoint's previous attachment, no type eviction,
	//! so two same-type hardpoints can hold two items.
	void SwapDraftAttachment(int weaponSlotIdx, ResourceName oldPrefab, ResourceName newPrefab)
	{
		if (!m_Draft)
			return;

		BIA_KitWeapon weapon = m_Draft.FindWeapon(weaponSlotIdx);
		if (!weapon)
			return;

		weapon.EnsurePins();
		if (!oldPrefab.IsEmpty())
		{
			int existing = weapon.m_aAttachments.Find(oldPrefab);
			if (existing >= 0)
			{
				weapon.m_aAttachments.Remove(existing);
				weapon.m_aAttachmentSlots.Remove(existing);
			}
		}

		if (!newPrefab.IsEmpty())
		{
			weapon.m_aAttachments.Insert(newPrefab);
			weapon.m_aAttachmentSlots.Insert(-1);
		}

		m_LoadedKitFile = null;
		m_bDraftDirty = true;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Pins one mounted attachment to an authored hardpoint slot on its weapon's own attachment
	//! storage; -1 returns it to automatic placement. The stage preview and the server apply honor
	//! the pin identically, so the slider's position is what the character actually wears.
	void SetDraftAttachmentPin(int weaponSlotIdx, ResourceName attachmentPrefab, int hardpointSlot)
	{
		if (!m_Draft || attachmentPrefab.IsEmpty())
			return;

		BIA_KitWeapon weapon = m_Draft.FindWeapon(weaponSlotIdx);
		if (!weapon)
			return;

		weapon.EnsurePins();
		int idx = weapon.m_aAttachments.Find(attachmentPrefab);
		if (idx < 0 || weapon.m_aAttachmentSlots[idx] == hardpointSlot)
			return;

		weapon.m_aAttachmentSlots[idx] = hardpointSlot;
		m_LoadedKitFile = null;
		m_bDraftDirty = true;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Moves one worn-item attachment to another authored hardpoint on the same clothing item.
	void SetDraftClothingAttachmentPin(int clothingSlotIdx, ResourceName attachmentPrefab, int hardpointSlot, int currentHardpointSlot = -1)
	{
		if (!m_Draft || attachmentPrefab.IsEmpty())
			return;

		BIA_KitClothing clothing = m_Draft.FindClothing(clothingSlotIdx);
		if (!clothing)
			return;

		clothing.EnsurePins();
		int idx = -1;
		foreach (int i, ResourceName attachment : clothing.m_aAttachments)
		{
			if (attachment == attachmentPrefab && (currentHardpointSlot < 0 || clothing.m_aAttachmentSlots[i] == currentHardpointSlot))
			{
				idx = i;
				break;
			}
		}
		if (idx < 0 || clothing.m_aAttachmentSlots[idx] == hardpointSlot)
			return;

		clothing.m_aAttachmentSlots[idx] = hardpointSlot;
		m_LoadedKitFile = null;
		m_bDraftDirty = true;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Quick-add: routes into the session target container.
	bool AddDraftExtra(ResourceName prefab, int delta)
	{
		return AddDraftExtraTo(prefab, delta, m_TargetContainer);
	}

	//------------------------------------------------------------------------------------------------
	//! Changes only the selected container bucket so contents never move between worn items implicitly.
	bool AddDraftExtraTo(ResourceName prefab, int delta, ResourceName container)
	{
		BIA_EExtraChangeResult result = ChangeDraftExtraTo(prefab, delta, container);
		return result == BIA_EExtraChangeResult.ADDED || result == BIA_EExtraChangeResult.REMOVED;
	}

	//------------------------------------------------------------------------------------------------
	//! Reports the exact outcome while keeping the draft unchanged on every rejected addition.
	BIA_EExtraChangeResult ChangeDraftExtraTo(ResourceName prefab, int delta, ResourceName container, IEntity containerPreview = null)
	{
		if (!m_Draft || prefab.IsEmpty() || delta == 0)
			return BIA_EExtraChangeResult.INVALID;

		BIA_EExtraChangeResult result;
		if (delta > 0)
		{
			result = GetAddDraftExtraResult(prefab, delta, container, containerPreview);
			if (result != BIA_EExtraChangeResult.ADDED)
				return result;
		}

		if (delta >= 0)
		{
			m_Draft.SetExtra(prefab, m_Draft.GetExtraCount(prefab, container) + delta, container);
		}
		else
		{
			int inBucket = m_Draft.GetExtraCount(prefab, container);
			int wanted = Math.Max(0, inBucket + delta);
			if (wanted == inBucket)
				return BIA_EExtraChangeResult.EMPTY;
			m_Draft.SetExtra(prefab, wanted, container);
			result = BIA_EExtraChangeResult.REMOVED;
		}

		m_LoadedKitFile = null;
		m_bDraftDirty = true;
		NotifyDraftChanged();
		return result;
	}

	//------------------------------------------------------------------------------------------------
	bool CanAddDraftExtraTo(ResourceName prefab, int count, ResourceName container)
	{
		return GetAddDraftExtraResult(prefab, count, container) == BIA_EExtraChangeResult.ADDED;
	}

	//------------------------------------------------------------------------------------------------
	BIA_EExtraChangeResult GetAddDraftExtraResult(ResourceName prefab, int count, ResourceName container, IEntity containerPreview = null)
	{
		if (!m_Draft || prefab.IsEmpty() || count <= 0)
			return BIA_EExtraChangeResult.INVALID;
		if (container.IsEmpty())
			return BIA_EExtraChangeResult.ADDED;

		if (containerPreview)
		{
			if (SCR_ResourceNameUtils.GetPrefabName(containerPreview) != container)
				return BIA_EExtraChangeResult.INVALID;
			return CanFitDraftContainerContents(container, prefab, count, containerPreview);
		}

		if (!BIA_ItemIntel.HasContainerStorage(container))
			return BIA_EExtraChangeResult.NO_STORAGE;
		if (!BIA_ItemIntel.CanStoreInContainer(container, prefab))
			return BIA_EExtraChangeResult.INCOMPATIBLE;

		return CanFitDraftContainerContents(container, prefab, count);
	}

	//------------------------------------------------------------------------------------------------
	//! Replays the drafted bucket through the same ordered child stores used by targeted Wear.
	protected BIA_EExtraChangeResult CanFitDraftContainerContents(ResourceName container, ResourceName addedPrefab, int addedCount, IEntity containerPreview = null)
	{
		array<BaseInventoryStorageComponent> storages = {};
		if (containerPreview)
			BIA_ItemIntel.CollectContainerStorages(containerPreview, storages);
		else
			BIA_ItemIntel.GetContainerStorages(container, storages);
		if (storages.IsEmpty())
			return BIA_EExtraChangeResult.NO_STORAGE;

		array<float> usedWeights = {};
		array<float> usedVolumes = {};
		for (int i = 0; i < storages.Count(); ++i)
		{
			usedWeights.Insert(0);
			usedVolumes.Insert(0);
		}

		bool proposalPlaced;
		foreach (BIA_KitExtra extra : m_Draft.m_aExtras)
		{
			if (extra.m_Container != container)
				continue;

			int wanted = extra.m_iCount;
			if (!proposalPlaced && extra.m_Prefab == addedPrefab)
			{
				wanted += addedCount;
				proposalPlaced = true;
			}

			for (int n = 0; n < wanted; ++n)
			{
				BIA_EExtraChangeResult result = PlaceDraftItem(extra.m_Prefab, storages, usedWeights, usedVolumes);
				if (result != BIA_EExtraChangeResult.ADDED)
					return result;
			}
		}

		if (!proposalPlaced)
		{
			for (int n = 0; n < addedCount; ++n)
			{
				BIA_EExtraChangeResult result = PlaceDraftItem(addedPrefab, storages, usedWeights, usedVolumes);
				if (result != BIA_EExtraChangeResult.ADDED)
					return result;
			}
		}

		return BIA_EExtraChangeResult.ADDED;
	}

	//------------------------------------------------------------------------------------------------
	protected BIA_EExtraChangeResult PlaceDraftItem(
		ResourceName prefab,
		notnull array<BaseInventoryStorageComponent> storages,
		notnull array<float> usedWeights,
		notnull array<float> usedVolumes)
	{
		float itemWeight = BIA_ItemIntel.GetWeight(prefab);
		float itemVolume = BIA_ItemIntel.GetVolume(prefab);
		bool compatible;
		bool weightBlocked;
		bool volumeBlocked;

		foreach (int i, BaseInventoryStorageComponent storage : storages)
		{
			if (!BIA_ItemIntel.CanStoreInStorage(storage, prefab))
				continue;

			compatible = true;
			float maxLoad;
			SCR_UniversalInventoryStorageComponent universal = SCR_UniversalInventoryStorageComponent.Cast(storage);
			if (universal)
				maxLoad = universal.GetMaxLoad();
			float maxVolume = storage.GetMaxVolumeCapacity();

			bool weightFits = maxLoad <= 0 || usedWeights[i] + itemWeight <= maxLoad + 0.001;
			bool volumeFits = maxVolume <= 0 || itemVolume <= 0 || usedVolumes[i] + itemVolume <= maxVolume + 0.001;
			if (weightFits && volumeFits)
			{
				usedWeights[i] = usedWeights[i] + itemWeight;
				usedVolumes[i] = usedVolumes[i] + itemVolume;
				return BIA_EExtraChangeResult.ADDED;
			}

			if (!weightFits)
				weightBlocked = true;
			if (!volumeFits)
				volumeBlocked = true;
		}

		if (!compatible)
			return BIA_EExtraChangeResult.INCOMPATIBLE;
		if (volumeBlocked)
			return BIA_EExtraChangeResult.VOLUME_LIMIT;
		if (weightBlocked)
			return BIA_EExtraChangeResult.WEIGHT_LIMIT;
		return BIA_EExtraChangeResult.INCOMPATIBLE;
	}

	//------------------------------------------------------------------------------------------------
	void LoadKitIntoDraft(notnull BIA_KitFile kitFile)
	{
		if (!m_Draft)
			m_Draft = new BIA_Kit();

		BIA_KitConvert.FlattenToDraft(kitFile, GetLocalCharacter(), m_Draft);
		m_LoadedKitFile = kitFile;
		m_bDraftDirty = true;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! An untouched loaded kit applies with full nested fidelity, an edited draft applies flat.
	void RequestApplyDraft()
	{
		if (m_LoadedKitFile)
		{
			RequestApplyKit(m_LoadedKitFile);
			return;
		}

		if (!m_Draft)
			return;

		GameEntity character = GetLocalCharacter();
		if (!character)
			return;

		BIA_KitFile kitFile = BIA_KitConvert.FromDraft(m_Draft, character, m_Draft.m_sName);
		RequestApplyKit(kitFile);
	}

	//------------------------------------------------------------------------------------------------
	void RequestApplyKit(notnull BIA_KitFile kitFile)
	{
		float now = System.GetTickCount();
		if (m_fLastApplySentMs > 0 && now - m_fLastApplySentMs < APPLY_DEBOUNCE_MS)
			return;

		SCR_ResourcePlayerControllerInventoryComponent rpcComponent = GetRpcComponent();
		if (!rpcComponent)
		{
			BIA_Log.Warn("No SCR_ResourcePlayerControllerInventoryComponent on the player controller");
			return;
		}

		RplId arsenalRplId = RplId.Invalid();
		if (m_Arsenal)
			arsenalRplId = Replication.FindItemId(m_Arsenal);
		RplId targetRplId = RplId.Invalid();
		GameEntity target = GetLocalCharacter();
		if (target)
		{
			RplComponent targetRpl = RplComponent.Cast(target.FindComponent(RplComponent));
			if (targetRpl)
				targetRplId = targetRpl.Id();
		}
		if (!targetRplId.IsValid())
		{
			BIA_Log.Warn("Kit apply rejected locally: edit target has no replication id");
			return;
		}

		m_fLastApplySentMs = now;
		rpcComponent.BIA_RequestApplyKit(kitFile, arsenalRplId, targetRplId);
	}

	//------------------------------------------------------------------------------------------------
	//! Screens use this to hold the apply verb while a request is likely still inside the server
	//! cooldown window.
	float GetMsSinceApplySent()
	{
		if (m_fLastApplySentMs <= 0)
			return -1;

		return System.GetTickCount() - m_fLastApplySentMs;
	}

	//------------------------------------------------------------------------------------------------
	//! Saves the draft as it stands, edits included — what the player sees on the stage is what
	//! lands in the kit slot.
	bool SaveDraftToSlot(int slot, string kitName)
	{
		if (!CanChangeSavedKits())
			return false;

		GameEntity character = GetLocalCharacter();
		if (!m_Draft || !character)
			return false;

		BIA_KitFile kitFile;
		if (m_LoadedKitFile)
			kitFile = m_LoadedKitFile;
		else
			kitFile = BIA_KitConvert.FromDraft(m_Draft, character, kitName);

		kitFile.m_sName = kitName;
		BIA_KitConvert.ComputeSidecars(kitFile, GetLocalFaction(), GetCostType());

		bool saved = BIA_KitStore.SaveKit(kitFile, slot);
		if (saved)
			m_OnKitListChanged.Invoke();
		return saved;
	}

	//------------------------------------------------------------------------------------------------
	//! Overwrites keep the original creation stamp so the kit holds its list position.
	bool OverwriteSlotWithDraft(int slot, string kitName, bool notify = true)
	{
		if (!CanChangeSavedKits())
			return false;

		GameEntity character = GetLocalCharacter();
		if (!m_Draft || !character)
			return false;

		BIA_KitFile kitFile;
		if (m_LoadedKitFile)
			kitFile = m_LoadedKitFile;
		else
			kitFile = BIA_KitConvert.FromDraft(m_Draft, character, kitName);

		kitFile.m_sName = kitName;
		BIA_KitConvert.ComputeSidecars(kitFile, GetLocalFaction(), GetCostType());

		BIA_KitFile previous = BIA_KitStore.LoadKit(slot);
		if (previous && previous.m_iCreatedAtUnix > 0)
			kitFile.m_iCreatedAtUnix = previous.m_iCreatedAtUnix;

		bool saved = BIA_KitStore.SaveKit(kitFile, slot);
		if (saved && notify)
			m_OnKitListChanged.Invoke();
		return saved;
	}

	//------------------------------------------------------------------------------------------------
	//! Load-modify-save keeps the raw slotData passthrough intact, only the name field changes.
	bool RenameKitSlot(int slot, string newName)
	{
		if (!CanChangeSavedKits() || newName.IsEmpty())
			return false;

		BIA_KitFile kit = BIA_KitStore.LoadKit(slot);
		if (!kit)
			return false;

		kit.m_sName = newName;
		return BIA_KitStore.SaveKit(kit, slot);
	}

	//------------------------------------------------------------------------------------------------
	int ImportLegacyKits()
	{
		if (!CanChangeSavedKits() || !m_Config)
			return 0;

		string factionKey;
		SCR_Faction faction = GetLocalFaction();
		if (faction)
			factionKey = faction.GetFactionKey();

		int imported = BIA_KitStore.ImportLegacyKits(factionKey, m_Config.m_iKitSlotCount);
		if (imported > 0)
			m_OnKitListChanged.Invoke();
		return imported;
	}

	//------------------------------------------------------------------------------------------------
	bool DeleteKitSlot(int slot)
	{
		bool deleted = BIA_KitStore.DeleteKit(slot);
		if (deleted)
			m_OnKitListChanged.Invoke();
		return deleted;
	}

	//------------------------------------------------------------------------------------------------
	void GetKitSlots(notnull out array<ref BIA_KitFile> kits)
	{
		kits.Clear();
		if (!m_Config)
			return;

		BIA_KitStore.GetKits(m_Config.m_iKitSlotCount, kits);
	}

	//------------------------------------------------------------------------------------------------
	ResourceName GetTargetContainer()
	{
		return m_TargetContainer;
	}

	//------------------------------------------------------------------------------------------------
	void SetTargetContainer(ResourceName container)
	{
		if (!container.IsEmpty() && !BIA_ItemIntel.HasContainerStorage(container))
			container = ResourceName.Empty;
		m_TargetContainer = container;
	}

	//------------------------------------------------------------------------------------------------
	string GetTargetContainerDisplayName()
	{
		return GetContainerDisplayName(m_TargetContainer);
	}

	//------------------------------------------------------------------------------------------------
	//! Short worn-area label keeps storage selectors readable regardless of an item's catalog name.
	string GetContainerDisplayName(ResourceName container)
	{
		if (container.IsEmpty())
			return "AUTO";

		string area = BIA_ItemIntel.GetClothAreaType(container);
		if (!area.IsEmpty())
		{
			string label = BIA_CatalogService.PrettyAreaName(area);
			if (!label.IsEmpty())
				return label;
		}

		return BIA_CatalogService.GetDisplayName(container, GetBrowseFaction());
	}

	//------------------------------------------------------------------------------------------------
	//! Drafted clothing with player-facing cargo storage, the candidates for targeted item placement.
	void GetDraftContainers(notnull array<ResourceName> outContainers)
	{
		outContainers.Clear();
		if (!m_Draft)
			return;

		foreach (BIA_KitClothing clothing : m_Draft.m_aClothings)
		{
			if (!clothing.m_Prefab.IsEmpty() && BIA_ItemIntel.HasContainerStorage(clothing.m_Prefab))
				outContainers.Insert(clothing.m_Prefab);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Cycles Auto, then each storage-capable worn container.
	void CycleTargetContainer()
	{
		array<ResourceName> cycle = {};
		cycle.Insert(ResourceName.Empty);
		if (m_Draft)
		{
			foreach (BIA_KitClothing clothing : m_Draft.m_aClothings)
			{
				if (!clothing.m_Prefab.IsEmpty() && BIA_ItemIntel.HasContainerStorage(clothing.m_Prefab))
					cycle.Insert(clothing.m_Prefab);
			}
		}

		int idx = cycle.Find(m_TargetContainer);
		if (idx < 0)
			idx = 0;
		m_TargetContainer = cycle[(idx + 1) % cycle.Count()];
	}

	//------------------------------------------------------------------------------------------------
	//! Drafted weight already routed into this container.
	float GetContainerFill(ResourceName container)
	{
		float weight;
		float volume;
		GetContainerUsage(container, weight, volume);
		return weight;
	}

	//------------------------------------------------------------------------------------------------
	//! Drafted volume already routed into this container.
	float GetContainerVolumeFill(ResourceName container)
	{
		float weight;
		float volume;
		GetContainerUsage(container, weight, volume);
		return volume;
	}

	//------------------------------------------------------------------------------------------------
	//! One canonical draft pass supplies both capacity dimensions for a worn container.
	void GetContainerUsage(ResourceName container, out float weight, out float volume)
	{
		weight = 0;
		volume = 0;
		if (!m_Draft || container.IsEmpty())
			return;

		foreach (BIA_KitExtra extra : m_Draft.m_aExtras)
		{
			if (extra.m_Container == container)
			{
				weight += BIA_ItemIntel.GetWeight(extra.m_Prefab) * extra.m_iCount;
				volume += BIA_ItemIntel.GetVolume(extra.m_Prefab) * extra.m_iCount;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Total drafted mass for the header readout: worn clothing, weapons with their attachments,
	//! and routed extras.
	float GetDraftWeight()
	{
		if (!m_Draft)
			return 0;

		float total;
		foreach (BIA_KitClothing clothing : m_Draft.m_aClothings)
		{
			total += BIA_ItemIntel.GetWeight(clothing.m_Prefab);
			foreach (ResourceName attachment : clothing.m_aAttachments)
				total += BIA_ItemIntel.GetWeight(attachment);
		}

		foreach (BIA_KitWeapon weapon : m_Draft.m_aWeapons)
		{
			total += BIA_ItemIntel.GetWeight(weapon.m_Prefab);
			foreach (ResourceName attachment : weapon.m_aAttachments)
				total += BIA_ItemIntel.GetWeight(attachment);
		}

		foreach (BIA_KitExtra extra : m_Draft.m_aExtras)
			total += BIA_ItemIntel.GetWeight(extra.m_Prefab) * extra.m_iCount;

		return total;
	}

	//------------------------------------------------------------------------------------------------
	//! Client-side availability preview against the same scenario policy the server applies.
	int CountUnavailableItems(notnull BIA_KitFile kitFile)
	{
		if (!m_Config)
			return 0;

		GameEntity character = GetLocalCharacter();
		if (!character)
			return 0;

		BIA_ApplyGate gate = BIA_ApplyGate.Build(m_Arsenal, character, m_Config);
		if (!gate.IsRestricting())
			return 0;

		array<ResourceName> prefabs = {};
		kitFile.CollectItemPrefabs(prefabs);

		int unavailable = 0;
		foreach (ResourceName prefab : prefabs)
		{
			if (!gate.Allows(prefab))
				unavailable++;
		}
		return unavailable;
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_ResourcePlayerControllerInventoryComponent GetRpcComponent()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return null;

		return SCR_ResourcePlayerControllerInventoryComponent.Cast(controller.FindComponent(SCR_ResourcePlayerControllerInventoryComponent));
	}
}
