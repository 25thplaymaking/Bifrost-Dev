//! One hardpoint in the recursive weapon slot tree: the weapon's own slots, then the slots of
//! every attached item, any depth.
class BIA_SlotNode
{
	string m_sLabel;
	string m_sTypePretty;
	typename m_SlotTypename;
	ResourceName m_AttachedPrefab;
	int m_iDepth;
	int m_iStorageSlot = -1;
	bool m_bVisible;
}

class BIA_ItemIntel
{
	//! Base game preview manager prefab, spawned locally so no custom preview world resource is ever shipped.
	protected static const ResourceName PREVIEW_MANAGER_PREFAB = "{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et";

	// Built on first use - eager static initializers charge the module-init budget shared by every loaded mod.
	protected static ref map<ResourceName, string> s_mAreaTypeCache;
	protected static ref map<ResourceName, float> s_mWeightCache;
	protected static ref map<ResourceName, float> s_mVolumeCache;
	protected static ref map<ResourceName, float> s_mStorageLoadCache;
	protected static ref map<ResourceName, float> s_mStorageVolumeCache;
	protected static ref map<ResourceName, int> s_mContainerStorageCache;
	protected static ref map<string, int> s_mStorageCompatCache;
	protected static ref map<ResourceName, int> s_mVisibleAttachmentSlotCache;
	protected static ref map<string, int> s_mAttachCompatCache;
	protected static ref map<ResourceName, string> s_mAttachTypeCache;
	protected static ref map<ResourceName, string> s_mAttachClassCache;
	protected static ref map<string, int> s_mSlotVerdictCache;
	protected static ref map<ResourceName, ref array<ResourceName>> s_mDefaultAttachmentCache;
	protected static ref map<ResourceName, ref array<int>> s_mDefaultAttachmentPins;

	protected static void EnsureCaches()
	{
		if (s_mAreaTypeCache)
			return;

		s_mAreaTypeCache = new map<ResourceName, string>();
		s_mWeightCache = new map<ResourceName, float>();
		s_mVolumeCache = new map<ResourceName, float>();
		s_mStorageLoadCache = new map<ResourceName, float>();
		s_mStorageVolumeCache = new map<ResourceName, float>();
		s_mContainerStorageCache = new map<ResourceName, int>();
		s_mStorageCompatCache = new map<string, int>();
		s_mVisibleAttachmentSlotCache = new map<ResourceName, int>();
		s_mAttachCompatCache = new map<string, int>();
		s_mAttachTypeCache = new map<ResourceName, string>();
		s_mAttachClassCache = new map<ResourceName, string>();
		s_mSlotVerdictCache = new map<string, int>();
		s_mDefaultAttachmentCache = new map<ResourceName, ref array<ResourceName>>();
		s_mDefaultAttachmentPins = new map<ResourceName, ref array<int>>();
	}

	//------------------------------------------------------------------------------------------------
	static ItemPreviewManagerEntity GetPreviewManager()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return null;

		ItemPreviewManagerEntity manager = world.GetItemPreviewManager();
		if (manager)
			return manager;

		Resource resource = Resource.Load(PREVIEW_MANAGER_PREFAB);
		if (resource && resource.IsValid())
			GetGame().SpawnEntityPrefabLocal(resource, world);

		return world.GetItemPreviewManager();
	}

	//------------------------------------------------------------------------------------------------
	//! Row/tile thumbnail in one call so UI code never touches the preview manager directly.
	//! Empty prefab clears the widget.
	static void SetThumbnail(ItemPreviewWidget widget, ResourceName prefab)
	{
		if (!widget)
			return;

		ItemPreviewManagerEntity manager = GetPreviewManager();
		if (!manager)
			return;

		if (prefab.IsEmpty())
			manager.SetPreviewItem(widget, null);
		else
			manager.SetPreviewItemFromPrefab(widget, prefab);
	}

	//------------------------------------------------------------------------------------------------
	//! Resolved through the preview manager cache, never a raw spawn, so missing-mod GUIDs cannot crash.
	protected static IEntity ResolveEntity(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return null;

		ItemPreviewManagerEntity manager = GetPreviewManager();
		if (!manager)
			return null;

		return manager.ResolvePreviewEntityForPrefab(prefab);
	}

	//------------------------------------------------------------------------------------------------
	static string GetClothAreaType(ResourceName prefab)
	{
		EnsureCaches();
		string cached;
		if (s_mAreaTypeCache.Find(prefab, cached))
			return cached;

		string areaType;
		IEntity entity = ResolveEntity(prefab);
		if (entity)
		{
			BaseLoadoutClothComponent clothComponent = BaseLoadoutClothComponent.Cast(entity.FindComponent(BaseLoadoutClothComponent));
			if (clothComponent && clothComponent.GetAreaType())
				areaType = clothComponent.GetAreaType().Type().ToString();
		}

		s_mAreaTypeCache.Insert(prefab, areaType);
		return areaType;
	}

	//------------------------------------------------------------------------------------------------
	// Include compartments and mounted pouch stores at every native inventory depth.
	static void CollectContainerStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		outStorages.Clear();
		if (!entity)
			return;

		set<BaseInventoryStorageComponent> roots = new set<BaseInventoryStorageComponent>();
		SCR_PlayerArsenalLoadout.FindStorageComponents(entity, roots);
		array<BaseInventoryStorageComponent> pending = {};
		set<BaseInventoryStorageComponent> visited = new set<BaseInventoryStorageComponent>();
		foreach (BaseInventoryStorageComponent root : roots)
		{
			if (!root || visited.Contains(root))
				continue;
			visited.Insert(root);
			pending.Insert(root);
		}

		for (int i = 0; i < pending.Count(); ++i)
		{
			BaseInventoryStorageComponent storage = pending[i];
			if (!ClothNodeStorageComponent.Cast(storage) && IsContainerStorage(storage))
				outStorages.Insert(storage);

			array<BaseInventoryStorageComponent> children = {};
			storage.GetOwnedStorages(children, 2, true);
			for (int slotId = 0; slotId < storage.GetSlotsCount(); ++slotId)
			{
				InventoryStorageSlot slot = storage.GetSlot(slotId);
				if (!slot || !slot.GetAttachedEntity())
					continue;
				set<BaseInventoryStorageComponent> attached = new set<BaseInventoryStorageComponent>();
				SCR_PlayerArsenalLoadout.FindStorageComponents(slot.GetAttachedEntity(), attached);
				foreach (BaseInventoryStorageComponent itemStorage : attached)
					children.Insert(itemStorage);
			}
			foreach (BaseInventoryStorageComponent child : children)
			{
				if (!child || visited.Contains(child))
					continue;
				visited.Insert(child);
				pending.Insert(child);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	static void GetContainerStorages(ResourceName prefab, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		CollectContainerStorages(ResolveEntity(prefab), outStorages);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsContainerStorage(BaseInventoryStorageComponent storage)
	{
		if (!storage || AttachmentsStorageComponent.Cast(storage) || BaseEquipmentStorageComponent.Cast(storage))
			return false;

		if (storage.GetMaxVolumeCapacity() > 0)
			return true;

		SCR_UniversalInventoryStorageComponent universal = SCR_UniversalInventoryStorageComponent.Cast(storage);
		return universal && universal.GetMaxLoad() > 0;
	}

	//------------------------------------------------------------------------------------------------
	static bool HasContainerStorage(ResourceName prefab)
	{
		EnsureCaches();
		int cached;
		if (s_mContainerStorageCache.Find(prefab, cached))
			return cached > 0;

		array<BaseInventoryStorageComponent> storages = {};
		GetContainerStorages(prefab, storages);
		bool hasStorage = !storages.IsEmpty();
		if (hasStorage)
			s_mContainerStorageCache.Insert(prefab, 1);
		else
			s_mContainerStorageCache.Insert(prefab, 0);
		return hasStorage;
	}

	//------------------------------------------------------------------------------------------------
	//! Weight capacity of a wearable container prefab, 0 when it is volume-only.
	static float GetStorageMaxLoad(ResourceName prefab)
	{
		EnsureCaches();
		float cached;
		if (s_mStorageLoadCache.Find(prefab, cached))
			return cached;

		float maxLoad;
		bool volumeOnly;
		array<BaseInventoryStorageComponent> storages = {};
		GetContainerStorages(prefab, storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			SCR_UniversalInventoryStorageComponent universal = SCR_UniversalInventoryStorageComponent.Cast(storage);
			if (!universal || universal.GetMaxLoad() <= 0)
			{
				volumeOnly = true;
				continue;
			}

			maxLoad += universal.GetMaxLoad();
		}
		if (volumeOnly)
			maxLoad = 0;

		s_mStorageLoadCache.Insert(prefab, maxLoad);
		return maxLoad;
	}

	//------------------------------------------------------------------------------------------------
	//! Volume capacity shared by universal, cloth-node, and modded wearable cargo storages.
	static float GetStorageMaxVolume(ResourceName prefab)
	{
		EnsureCaches();
		float cached;
		if (s_mStorageVolumeCache.Find(prefab, cached))
			return cached;

		float maxVolume;
		array<BaseInventoryStorageComponent> storages = {};
		GetContainerStorages(prefab, storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			maxVolume += storage.GetMaxVolumeCapacity();
		}

		s_mStorageVolumeCache.Insert(prefab, maxVolume);
		return maxVolume;
	}

	//------------------------------------------------------------------------------------------------
	//! Current capacity of a staged container, including storage supplied by mounted parts.
	static float GetLiveStorageMaxVolume(IEntity entity)
	{
		float maxVolume;
		array<BaseInventoryStorageComponent> storages = {};
		CollectContainerStorages(entity, storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			maxVolume += storage.GetMaxVolumeCapacity();
		}

		return maxVolume;
	}

	//------------------------------------------------------------------------------------------------
	static bool CanStoreInContainer(ResourceName containerPrefab, ResourceName itemPrefab)
	{
		EnsureCaches();
		string key = containerPrefab + "|" + itemPrefab;
		int cached;
		if (s_mStorageCompatCache.Find(key, cached))
			return cached > 0;

		bool compatible;
		array<BaseInventoryStorageComponent> storages = {};
		GetContainerStorages(containerPrefab, storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (CanStoreInStorage(storage, itemPrefab))
			{
				compatible = true;
				break;
			}
		}

		if (compatible)
			s_mStorageCompatCache.Insert(key, 1);
		else
			s_mStorageCompatCache.Insert(key, 0);
		return compatible;
	}

	//------------------------------------------------------------------------------------------------
	static bool CanStoreInStorage(BaseInventoryStorageComponent storage, ResourceName itemPrefab)
	{
		return storage
			&& storage.CanStoreResource(itemPrefab, -1)
			&& storage.PerformVolumeValidationForResource(itemPrefab, true);
	}

	//------------------------------------------------------------------------------------------------
	//! True when the item exposes at least one authored inspection hardpoint.

	// Clothing mounts use native equipment stores and loadout areas.
	static BaseInventoryStorageComponent AttachmentStorage(IEntity owner)
	{
		if (!owner) return null;
		BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(owner.FindComponent(WeaponAttachmentsStorageComponent));
		if (!storage) storage = BaseInventoryStorageComponent.Cast(owner.FindComponent(ClothNodeStorageComponent));
		if (!storage) storage = BaseInventoryStorageComponent.Cast(owner.FindComponent(BaseEquipmentStorageComponent));
		return storage;
	}

	static bool IsAttachmentStorage(BaseInventoryStorageComponent storage)
	{
		return storage && (WeaponAttachmentsStorageComponent.Cast(storage) || ClothNodeStorageComponent.Cast(storage) || BaseEquipmentStorageComponent.Cast(storage));
	}

	static bool MountAccepts(InventoryStorageSlot slot, ResourceName prefab)
	{
		if (!slot || prefab.IsEmpty() || slot.IsLocked()) return false;
		AttachmentSlotComponent weaponSlot = AttachmentSlotComponent.Cast(slot.GetParentContainer());
		if (weaponSlot)
			return weaponSlot.GetAttachmentSlotType() && SlotAcceptsItem(weaponSlot.GetAttachmentSlotType().Type(), prefab);
		BaseInventoryStorageComponent storage = slot.GetStorage();
		if (!IsAttachmentStorage(storage)) return false;
		IEntity item = ResolveEntity(prefab);
		if (!item) return false;
		LoadoutSlotInfo clothSlot = LoadoutSlotInfo.Cast(slot);
		if (clothSlot && clothSlot.GetAreaType())
		{
			BaseLoadoutClothComponent cloth = BaseLoadoutClothComponent.Cast(item.FindComponent(BaseLoadoutClothComponent));
			if (!cloth || !cloth.GetAreaType() || !cloth.GetAreaType().Type().IsInherited(clothSlot.GetAreaType().Type()))
				return false;
		}
		if (slot.GetAttachedEntity()) return storage.CanReplaceItem(item, slot.GetID());
		return storage.CanStoreItem(item, slot.GetID());
	}

	static string MountLabel(InventoryStorageSlot slot)
	{
		if (!slot) return string.Empty;
		string label = slot.GetSourceName();
		if (label.IsEmpty())
		{
			LoadoutSlotInfo clothSlot = LoadoutSlotInfo.Cast(slot);
			if (clothSlot && clothSlot.GetAreaType()) label = clothSlot.GetAreaType().Type().ToString();
		}
		label.Replace("Loadout", "");
		label.Replace("Area", "");
		label.Replace("_", " ");
		label.ToUpper();
		return label;
	}

	static bool HasVisibleAttachmentSlots(ResourceName prefab)
	{
		EnsureCaches();
		int cached;
		if (s_mVisibleAttachmentSlotCache.Find(prefab, cached))
			return cached > 0;

		bool hasVisibleSlot;
		IEntity entity = ResolveEntity(prefab);
		if (entity)
		{
			array<ref BIA_SlotNode> nodes = {};
			BuildSlotTree(entity, nodes);
			foreach (BIA_SlotNode node : nodes)
			{
				if (node && node.m_iDepth == 0 && node.m_bVisible)
				{
					hasVisibleSlot = true;
					break;
				}
			}
		}

		if (hasVisibleSlot)
			s_mVisibleAttachmentSlotCache.Insert(prefab, 1);
		else
			s_mVisibleAttachmentSlotCache.Insert(prefab, 0);
		return hasVisibleSlot;
	}

	//------------------------------------------------------------------------------------------------
	static float GetWeight(ResourceName prefab)
	{
		EnsureCaches();
		float cached;
		if (s_mWeightCache.Find(prefab, cached))
			return cached;

		float weight;
		IEntity entity = ResolveEntity(prefab);
		if (entity)
		{
			InventoryItemComponent itemComponent = InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
			if (itemComponent)
				weight = itemComponent.GetTotalWeight();
		}

		s_mWeightCache.Insert(prefab, weight);
		return weight;
	}

	//------------------------------------------------------------------------------------------------
	static float GetVolume(ResourceName prefab)
	{
		EnsureCaches();
		float cached;
		if (s_mVolumeCache.Find(prefab, cached))
			return cached;

		float volume;
		IEntity entity = ResolveEntity(prefab);
		if (entity)
		{
			InventoryItemComponent itemComponent = InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
			if (itemComponent)
			{
				ItemPhysicalAttributes attributes = ItemPhysicalAttributes.Cast(itemComponent.FindAttribute(ItemPhysicalAttributes));
				if (attributes)
					volume = attributes.GetVolume();
			}
		}

		s_mVolumeCache.Insert(prefab, volume);
		return volume;
	}

	//------------------------------------------------------------------------------------------------
	//! Preview entities live in a 30 second cache, so compatibility verdicts are cached by prefab pair.
	static bool CanAttach(ResourceName weaponPrefab, ResourceName attachmentPrefab)
	{
		EnsureCaches();
		string key = weaponPrefab + "|" + attachmentPrefab;
		int cached;
		if (s_mAttachCompatCache.Find(key, cached))
			return cached > 0;

		bool fits = false;
		IEntity weaponEntity = ResolveEntity(weaponPrefab);
		if (weaponEntity)
		{
			array<ref BIA_SlotNode> nodes = {};
			BIA_ItemIntel.BuildSlotTree(weaponEntity, nodes);
			foreach (BIA_SlotNode node : nodes)
			{
				if (SlotAcceptsItem(node.m_SlotTypename, attachmentPrefab))
				{
					fits = true;
					break;
				}
			}
		}

		if (fits)
			s_mAttachCompatCache.Insert(key, 1);
		else
			s_mAttachCompatCache.Insert(key, 0);
		return fits;
	}

	//------------------------------------------------------------------------------------------------
	//! Raw attachment-type class name of an attachment prefab, empty for anything else. Authored
	//! data — cached for the whole session and warmable at startup, so compatibility checks can
	//! run on typenames without ever resolving the entity again.
	static string GetAttachmentClass(ResourceName prefab)
	{
		EnsureCaches();
		string cached;
		if (s_mAttachClassCache.Find(prefab, cached))
			return cached;

		string className;
		IEntity entity = ResolveEntity(prefab);
		if (entity)
		{
			InventoryItemComponent itemComponent = InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
			if (itemComponent)
			{
				WeaponAttachmentAttributes attachmentAttributes = WeaponAttachmentAttributes.Cast(itemComponent.FindAttribute(WeaponAttachmentAttributes));
				if (attachmentAttributes && attachmentAttributes.GetAttachmentType())
					className = attachmentAttributes.GetAttachmentType().Type().ToString();
			}
		}

		s_mAttachClassCache.Insert(prefab, className);
		return className;
	}

	//------------------------------------------------------------------------------------------------
	static string GetAttachmentTypeName(ResourceName prefab)
	{
		EnsureCaches();
		string cached;
		if (s_mAttachTypeCache.Find(prefab, cached))
			return cached;

		string typeName = GetAttachmentClass(prefab);
		if (!typeName.IsEmpty())
		{
			typeName.Replace("Attachment", "");
			typeName.Replace("Type", "");
			typeName.ToUpper();
		}

		s_mAttachTypeCache.Insert(prefab, typeName);
		return typeName;
	}

	//------------------------------------------------------------------------------------------------
	//! Factory attachments a weapon prefab ships with. Variant prefabs define their look through
	//! these, so a draft weapon must start from them or syncing strips the variant parts.
	static void GetDefaultAttachments(ResourceName weaponPrefab, notnull out array<ResourceName> outAttachments, array<int> outPins = null)
	{
		outAttachments.Clear();
		if (outPins) outPins.Clear();
		EnsureCaches();
		array<ResourceName> cached = s_mDefaultAttachmentCache.Get(weaponPrefab);
		array<int> pins = s_mDefaultAttachmentPins.Get(weaponPrefab);
		if (!cached)
		{
			IEntity entity = ResolveEntity(weaponPrefab);
			if (!entity) return;
			cached = {};
			pins = {};
			CollectSubtreeAttachments(entity, cached, 0, pins);
			s_mDefaultAttachmentCache.Insert(weaponPrefab, cached);
			s_mDefaultAttachmentPins.Insert(weaponPrefab, pins);
		}
		outAttachments.Copy(cached);
		if (outPins && pins) outPins.Copy(pins);
	}

	//------------------------------------------------------------------------------------------------
	static void ClearSessionCache()
	{
		EnsureCaches();
		s_mAttachCompatCache.Clear();
		s_mStorageLoadCache.Clear();
		s_mStorageVolumeCache.Clear();
		s_mContainerStorageCache.Clear();
		s_mStorageCompatCache.Clear();
		s_mVisibleAttachmentSlotCache.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Depth-first slot tree of a live weapon entity: own hardpoints first, then each attached
	//! item's hardpoints beneath it, any depth. Magazine wells are not attachment slots and are
	//! handled by the Ammunition category instead.
	static void BuildSlotTree(IEntity weapon, notnull array<ref BIA_SlotNode> outNodes, int depth = 0, string parentLabel = "")
	{
		outNodes.Clear();
		CollectSlotNodes(weapon, outNodes, depth, parentLabel);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectSlotNodes(IEntity owner, notnull array<ref BIA_SlotNode> outNodes, int depth, string parentLabel)
	{
		if (!owner || depth > 4)
			return;

		BaseInventoryStorageComponent storage = AttachmentStorage(owner);
		if (!storage)
			return;

		int slotsCount = storage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			if (!slot)
				continue;

			AttachmentSlotComponent slotComponent = AttachmentSlotComponent.Cast(slot.GetParentContainer());
			if (BaseMuzzleComponent.Cast(slot.GetParentContainer())) continue;

			BIA_SlotNode node = new BIA_SlotNode();
			if (slotComponent && slotComponent.GetAttachmentSlotType())
			{
				node.m_SlotTypename = slotComponent.GetAttachmentSlotType().Type();
				node.m_sTypePretty = PrettyTypeName(node.m_SlotTypename.ToString());
			}
			else node.m_sTypePretty = MountLabel(slot);
			node.m_iDepth = depth;
			node.m_iStorageSlot = i;
			node.m_bVisible = !slotComponent || slotComponent.ShouldShowInInspection();
			if (parentLabel.IsEmpty())
				node.m_sLabel = node.m_sTypePretty;
			else
				node.m_sLabel = string.Format("%1 > %2", parentLabel, node.m_sTypePretty);

			IEntity attached = slot.GetAttachedEntity();
			if (attached)
				node.m_AttachedPrefab = SCR_ResourceNameUtils.GetPrefabName(attached);

			outNodes.Insert(node);

			if (attached)
				CollectSlotNodes(attached, outNodes, depth + 1, node.m_sTypePretty);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static string PrettyTypeName(string typeName)
	{
		typeName.Replace("Attachment", "");
		typeName.Replace("Type", "");
		typeName.ToUpper();
		return typeName;
	}

	//------------------------------------------------------------------------------------------------
	//! Same comparison the base compatibility predicate runs: the item's attachment type must
	//! inherit the slot's type. The verdict is a typename-level truth, so it caches by
	//! slot-type|attachment-class and needs no entity resolve once the class is known.
	static bool SlotAcceptsItem(typename slotTypename, ResourceName attachmentPrefab)
	{
		if (!slotTypename)
			return false;

		string attachClass = GetAttachmentClass(attachmentPrefab);
		if (attachClass.IsEmpty())
			return false;

		EnsureCaches();
		string key = slotTypename.ToString() + "|" + attachClass;
		int cachedVerdict;
		if (s_mSlotVerdictCache.Find(key, cachedVerdict))
			return cachedVerdict > 0;

		bool fits;
		typename attachType = attachClass.ToType();
		if (attachType)
			fits = attachType.IsInherited(slotTypename);

		if (fits)
			s_mSlotVerdictCache.Insert(key, 1);
		else
			s_mSlotVerdictCache.Insert(key, 0);
		return fits;
	}

	//------------------------------------------------------------------------------------------------
	//! Every attachment prefab in the entity's subtree, depth-first, so the flat draft list stays
	//! the complete authoritative set at all depths.
	static void CollectSubtreeAttachments(IEntity owner, notnull array<ResourceName> outPrefabs, int depth = 0, array<int> outPins = null)
	{
		if (!owner || depth > 4)
			return;

		BaseInventoryStorageComponent storage = AttachmentStorage(owner);
		if (!storage)
			return;

		int slotsCount = storage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			IEntity attached = storage.Get(i);
			if (!attached)
				continue;

			ResourceName prefab = SCR_ResourceNameUtils.GetPrefabName(attached);
			if (!prefab.IsEmpty())
			{
				outPrefabs.Insert(prefab);
				if (outPins)
				{
					if (depth == 0) outPins.Insert(i);
					else outPins.Insert(-1);
				}
			}
			CollectSubtreeAttachments(attached, outPrefabs, depth + 1, outPins);
		}
	}
}
