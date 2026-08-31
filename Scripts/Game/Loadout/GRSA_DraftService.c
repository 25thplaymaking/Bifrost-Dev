//! Client-side session backend for the armory editor: owns the draft loadout, kit persistence
//! wrappers, container routing, availability preview, and apply dispatch. Pure service layer —
//! holds no widgets, schedules no UI behavior. Screens subscribe to the change events; the
//! entry-point action opens the menu itself so this class never references UI types.
class GRSA_DraftService
{
	protected static const int APPLY_DEBOUNCE_MS = 2100;
	protected static ref GRSA_DraftService s_Instance;

	//! Character being edited. Bifrost GM sessions may target a replicated AI or another player;
	//! player Arsenal Access sessions target the locally controlled character.
	protected IEntity m_EditTarget;
	protected bool m_bBifrostSession;
	SCR_ArsenalComponent m_Arsenal;
	ref GRSA_ArmoryConfig m_Config;
	ref GRSA_Kit m_Draft;

	//! Full-fidelity kit backing the draft, kept while the draft is untouched so apply preserves
	//! nested contents.
	ref GRSA_KitFile m_LoadedKitFile;

	//! True while the draft differs from the last applied or captured character state.
	bool m_bDraftDirty;

	//! Container prefab new inventory items are routed into, empty means automatic placement.
	protected ResourceName m_TargetContainer;

	protected float m_fLastApplySentMs;

	ref ScriptInvoker m_OnDraftChanged = new ScriptInvoker();
	ref ScriptInvoker m_OnKitListChanged = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	static GRSA_DraftService Get()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Bifrost entry point shared by placed Arsenal Access and GM Edit Loadout. The target remains
	//! explicit through preview, draft capture, and the server request; no client-selected target is
	//! trusted by the server without the existing Bifrost access/GM authorization checks.
	static GRSA_DraftService BeginForTarget(IEntity target, bool bifrostSession = true)
	{
		GRSA_DraftService service = new GRSA_DraftService();
		service.m_EditTarget = target;
		service.m_bBifrostSession = bifrostSession;
		service.m_Config = GRSA_ConfigHolder.GetDefault();

		s_Instance = service;
		GRSA_CatalogService.ClearSessionCache();
		GRSA_ItemIntel.ClearSessionCache();
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
		bool requested = GRSA_ArsenalScenarioSettings.Get().m_bUseRankLocks;
		GRSA_DraftService service = Get();
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
		if (GRSA_ArsenalScenarioSettings.Get().m_eCatalogScope == GRSA_ECatalogScope.STATION_ARSENAL && m_Arsenal)
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
		bool enabled = GRSA_ArsenalScenarioSettings.Get().m_bUseSupplies;
		if (m_Config && m_Config.m_bUseSupplies)
			enabled = true;
		return enabled && m_Arsenal && m_Arsenal.IsArsenalUsingSupplies();
	}

	//------------------------------------------------------------------------------------------------
	bool CanChangeSavedKits()
	{
		return GRSA_ArsenalScenarioSettings.Get().m_bAllowKitChanges;
	}

	//------------------------------------------------------------------------------------------------
	void RebuildDraftFromCharacter()
	{
		GameEntity character = GetLocalCharacter();
		if (character)
			m_Draft = GRSA_KitCapture.CaptureFromCharacter(character, GetCostType());
		else
			m_Draft = new GRSA_Kit();

		m_LoadedKitFile = null;
		m_bDraftDirty = false;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	void NotifyDraftChanged()
	{
		if (m_Draft)
		{
			GRSA_KitCapture.BuildSummaries(m_Draft, GetLocalFaction());
			GRSA_KitCapture.ComputeCost(m_Draft, GetLocalFaction(), GetCostType());
		}
		m_OnDraftChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	//! Draft edits detach the loaded kit file, the flat model is then authoritative on apply.
	void SetDraftClothing(int slotIdx, ResourceName prefab)
	{
		if (!m_Draft)
			return;

		m_Draft.SetClothing(slotIdx, prefab);
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

		GRSA_KitWeapon existing = m_Draft.FindWeapon(slotIdx);
		bool prefabChanged = !existing || existing.m_Prefab != prefab;

		m_Draft.SetWeapon(slotIdx, prefab);

		GRSA_KitWeapon weapon = m_Draft.FindWeapon(slotIdx);
		if (weapon)
		{
			if (prefabChanged && !prefab.IsEmpty())
			{
				GRSA_ItemIntel.GetDefaultAttachments(prefab, weapon.m_aAttachments);
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

		GRSA_KitWeapon weapon = m_Draft.FindWeapon(weaponSlotIdx);
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
	protected void EvictSameTypeAttachments(notnull GRSA_KitWeapon weapon, ResourceName attachmentPrefab)
	{
		string typeName = GRSA_ItemIntel.GetAttachmentTypeName(attachmentPrefab);
		if (typeName.IsEmpty())
			return;

		weapon.EnsurePins();
		for (int i = weapon.m_aAttachments.Count() - 1; i >= 0; --i)
		{
			if (GRSA_ItemIntel.GetAttachmentTypeName(weapon.m_aAttachments[i]) != typeName)
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

		GRSA_KitWeapon weapon = m_Draft.FindWeapon(weaponSlotIdx);
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

		GRSA_KitWeapon weapon = m_Draft.FindWeapon(weaponSlotIdx);
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
	//! Quick-add: routes into the session target container.
	void AddDraftExtra(ResourceName prefab, int delta)
	{
		AddDraftExtraTo(prefab, delta, m_TargetContainer);
	}

	//------------------------------------------------------------------------------------------------
	//! Adds go into the given container's bucket. Removals drain that bucket first, then any other
	//! bucket still holding the item, so "remove one" always works regardless of the current target.
	void AddDraftExtraTo(ResourceName prefab, int delta, ResourceName container)
	{
		if (!m_Draft)
			return;

		if (delta >= 0)
		{
			m_Draft.SetExtra(prefab, m_Draft.GetExtraCount(prefab, container) + delta, container);
		}
		else
		{
			int remove = -delta;
			int inBucket = m_Draft.GetExtraCount(prefab, container);
			int fromBucket = Math.Min(inBucket, remove);
			if (fromBucket > 0)
				m_Draft.SetExtra(prefab, inBucket - fromBucket, container);
			remove -= fromBucket;

			if (remove > 0)
			{
				array<ResourceName> otherBuckets = {};
				array<int> otherCounts = {};
				foreach (GRSA_KitExtra extra : m_Draft.m_aExtras)
				{
					if (extra.m_Prefab == prefab && extra.m_Container != container)
					{
						otherBuckets.Insert(extra.m_Container);
						otherCounts.Insert(extra.m_iCount);
					}
				}

				for (int i = 0; i < otherBuckets.Count() && remove > 0; ++i)
				{
					int fromOther = Math.Min(otherCounts[i], remove);
					m_Draft.SetExtra(prefab, otherCounts[i] - fromOther, otherBuckets[i]);
					remove -= fromOther;
				}
			}
		}

		m_LoadedKitFile = null;
		m_bDraftDirty = true;
		NotifyDraftChanged();
	}

	//------------------------------------------------------------------------------------------------
	void LoadKitIntoDraft(notnull GRSA_KitFile kitFile)
	{
		if (!m_Draft)
			m_Draft = new GRSA_Kit();

		GRSA_KitConvert.FlattenToDraft(kitFile, GetLocalCharacter(), m_Draft);
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

		GRSA_KitFile kitFile = GRSA_KitConvert.FromDraft(m_Draft, character, m_Draft.m_sName);
		RequestApplyKit(kitFile);
	}

	//------------------------------------------------------------------------------------------------
	void RequestApplyKit(notnull GRSA_KitFile kitFile)
	{
		float now = System.GetTickCount();
		if (m_fLastApplySentMs > 0 && now - m_fLastApplySentMs < APPLY_DEBOUNCE_MS)
			return;

		SCR_ResourcePlayerControllerInventoryComponent rpcComponent = GetRpcComponent();
		if (!rpcComponent)
		{
			GRSA_Log.Warn("No SCR_ResourcePlayerControllerInventoryComponent on the player controller");
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
			GRSA_Log.Warn("Kit apply rejected locally: edit target has no replication id");
			return;
		}

		m_fLastApplySentMs = now;
		rpcComponent.GRSA_RequestApplyKit(kitFile, arsenalRplId, targetRplId);
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

		GRSA_KitFile kitFile;
		if (m_LoadedKitFile)
			kitFile = m_LoadedKitFile;
		else
			kitFile = GRSA_KitConvert.FromDraft(m_Draft, character, kitName);

		kitFile.m_sName = kitName;
		GRSA_KitConvert.ComputeSidecars(kitFile, GetLocalFaction(), GetCostType());

		bool saved = GRSA_KitStore.SaveKit(kitFile, slot);
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

		GRSA_KitFile kitFile;
		if (m_LoadedKitFile)
			kitFile = m_LoadedKitFile;
		else
			kitFile = GRSA_KitConvert.FromDraft(m_Draft, character, kitName);

		kitFile.m_sName = kitName;
		GRSA_KitConvert.ComputeSidecars(kitFile, GetLocalFaction(), GetCostType());

		GRSA_KitFile previous = GRSA_KitStore.LoadKit(slot);
		if (previous && previous.m_iCreatedAtUnix > 0)
			kitFile.m_iCreatedAtUnix = previous.m_iCreatedAtUnix;

		bool saved = GRSA_KitStore.SaveKit(kitFile, slot);
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

		GRSA_KitFile kit = GRSA_KitStore.LoadKit(slot);
		if (!kit)
			return false;

		kit.m_sName = newName;
		return GRSA_KitStore.SaveKit(kit, slot);
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

		int imported = GRSA_KitStore.ImportLegacyKits(factionKey, m_Config.m_iKitSlotCount);
		if (imported > 0)
			m_OnKitListChanged.Invoke();
		return imported;
	}

	//------------------------------------------------------------------------------------------------
	bool DeleteKitSlot(int slot)
	{
		bool deleted = GRSA_KitStore.DeleteKit(slot);
		if (deleted)
			m_OnKitListChanged.Invoke();
		return deleted;
	}

	//------------------------------------------------------------------------------------------------
	void GetKitSlots(notnull out array<ref GRSA_KitFile> kits)
	{
		kits.Clear();
		if (!m_Config)
			return;

		GRSA_KitStore.GetKits(m_Config.m_iKitSlotCount, kits);
	}

	//------------------------------------------------------------------------------------------------
	ResourceName GetTargetContainer()
	{
		return m_TargetContainer;
	}

	//------------------------------------------------------------------------------------------------
	void SetTargetContainer(ResourceName container)
	{
		m_TargetContainer = container;
	}

	//------------------------------------------------------------------------------------------------
	//! Drafted clothing with a weight-capped storage, the candidates for targeted item placement.
	void GetDraftContainers(notnull array<ResourceName> outContainers)
	{
		outContainers.Clear();
		if (!m_Draft)
			return;

		foreach (GRSA_KitClothing clothing : m_Draft.m_aClothings)
		{
			if (!clothing.m_Prefab.IsEmpty() && GRSA_ItemIntel.GetStorageMaxLoad(clothing.m_Prefab) > 0)
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
			foreach (GRSA_KitClothing clothing : m_Draft.m_aClothings)
			{
				if (!clothing.m_Prefab.IsEmpty() && GRSA_ItemIntel.GetStorageMaxLoad(clothing.m_Prefab) > 0)
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
		if (!m_Draft || container.IsEmpty())
			return 0;

		float fill;
		foreach (GRSA_KitExtra extra : m_Draft.m_aExtras)
		{
			if (extra.m_Container == container)
				fill += GRSA_ItemIntel.GetWeight(extra.m_Prefab) * extra.m_iCount;
		}
		return fill;
	}

	//------------------------------------------------------------------------------------------------
	//! Total drafted mass for the header readout: worn clothing, weapons with their attachments,
	//! and routed extras.
	float GetDraftWeight()
	{
		if (!m_Draft)
			return 0;

		float total;
		foreach (GRSA_KitClothing clothing : m_Draft.m_aClothings)
			total += GRSA_ItemIntel.GetWeight(clothing.m_Prefab);

		foreach (GRSA_KitWeapon weapon : m_Draft.m_aWeapons)
		{
			total += GRSA_ItemIntel.GetWeight(weapon.m_Prefab);
			foreach (ResourceName attachment : weapon.m_aAttachments)
				total += GRSA_ItemIntel.GetWeight(attachment);
		}

		foreach (GRSA_KitExtra extra : m_Draft.m_aExtras)
			total += GRSA_ItemIntel.GetWeight(extra.m_Prefab) * extra.m_iCount;

		return total;
	}

	//------------------------------------------------------------------------------------------------
	//! Client-side availability preview against the same scenario policy the server applies.
	int CountUnavailableItems(notnull GRSA_KitFile kitFile)
	{
		if (!m_Config)
			return 0;

		GameEntity character = GetLocalCharacter();
		if (!character)
			return 0;

		GRSA_ApplyGate gate = GRSA_ApplyGate.Build(m_Arsenal, character, m_Config);
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
