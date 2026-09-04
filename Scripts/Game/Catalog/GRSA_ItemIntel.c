//! One hardpoint in the recursive weapon slot tree: the weapon's own slots, then the slots of
//! every attached item, any depth.
class GRSA_SlotNode
{
	string m_sLabel;
	string m_sTypePretty;
	typename m_SlotTypename;
	ResourceName m_AttachedPrefab;
	int m_iDepth;
	int m_iStorageSlot = -1;
	bool m_bVisible;
}

class GRSA_ItemIntel
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
	//! Concrete cargo stores exposed by a worn item. Cloth roots route to their owned pouch stores.
	static void CollectContainerStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		outStorages.Clear();
		if (!entity)
			return;

		array<BaseInventoryStorageComponent> direct = {};
		array<BaseInventoryStorageComponent> nested = {};
		set<BaseInventoryStorageComponent> storages = new set<BaseInventoryStorageComponent>();
		SCR_PlayerArsenalLoadout.FindStorageComponents(entity, storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			ClothNodeStorageComponent clothRoot = ClothNodeStorageComponent.Cast(storage);
			if (clothRoot)
			{
				array<BaseInventoryStorageComponent> ownedStorages = {};
				clothRoot.GetOwnedStorages(ownedStorages, 1, false);
				foreach (BaseInventoryStorageComponent ownedStorage : ownedStorages)
				{
					if (ClothNodeStorageComponent.Cast(ownedStorage) || !IsContainerStorage(ownedStorage))
						continue;
					if (direct.Find(ownedStorage) == -1 && nested.Find(ownedStorage) == -1)
						nested.Insert(ownedStorage);
				}
				continue;
			}

			if (!IsContainerStorage(storage))
				continue;

			if (storage.GetOwner() == entity)
			{
				if (direct.Find(storage) == -1)
					direct.Insert(storage);
			}
			else if (nested.Find(storage) == -1)
				nested.Insert(storage);
		}

		foreach (BaseInventoryStorageComponent storage : direct)
			outStorages.Insert(storage);

		foreach (BaseInventoryStorageComponent storage : nested)
			outStorages.Insert(storage);
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
			array<ref GRSA_SlotNode> nodes = {};
			BuildSlotTree(entity, nodes);
			foreach (GRSA_SlotNode node : nodes)
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
			array<ref GRSA_SlotNode> nodes = {};
			GRSA_ItemIntel.BuildSlotTree(weaponEntity, nodes);
			foreach (GRSA_SlotNode node : nodes)
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
	static void GetDefaultAttachments(ResourceName weaponPrefab, notnull out array<ResourceName> outAttachments)
	{
		outAttachments.Clear();

		EnsureCaches();
		array<ResourceName> cached = s_mDefaultAttachmentCache.Get(weaponPrefab);
		if (cached)
		{
			foreach (ResourceName prefab : cached)
			{
				outAttachments.Insert(prefab);
			}
			return;
		}

		ref array<ResourceName> defaults = {};
		IEntity entity = ResolveEntity(weaponPrefab);
		if (entity)
			CollectSubtreeAttachments(entity, defaults);

		s_mDefaultAttachmentCache.Insert(weaponPrefab, defaults);
		foreach (ResourceName prefab : defaults)
		{
			outAttachments.Insert(prefab);
		}
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
	static void BuildSlotTree(IEntity weapon, notnull array<ref GRSA_SlotNode> outNodes, int depth = 0, string parentLabel = "")
	{
		outNodes.Clear();
		CollectSlotNodes(weapon, outNodes, depth, parentLabel);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectSlotNodes(IEntity owner, notnull array<ref GRSA_SlotNode> outNodes, int depth, string parentLabel)
	{
		if (!owner || depth > 4)
			return;

		WeaponAttachmentsStorageComponent storage = WeaponAttachmentsStorageComponent.Cast(owner.FindComponent(WeaponAttachmentsStorageComponent));
		if (!storage)
			return;

		int slotsCount = storage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			if (!slot)
				continue;

			AttachmentSlotComponent slotComponent = AttachmentSlotComponent.Cast(slot.GetParentContainer());
			if (!slotComponent || !slotComponent.GetAttachmentSlotType())
				continue;

			GRSA_SlotNode node = new GRSA_SlotNode();
			node.m_SlotTypename = slotComponent.GetAttachmentSlotType().Type();
			node.m_sTypePretty = PrettyTypeName(node.m_SlotTypename.ToString());
			node.m_iDepth = depth;
			node.m_iStorageSlot = i;
			node.m_bVisible = slotComponent.ShouldShowInInspection();
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
	static void CollectSubtreeAttachments(IEntity owner, notnull array<ResourceName> outPrefabs, int depth = 0)
	{
		if (!owner || depth > 4)
			return;

		WeaponAttachmentsStorageComponent storage = WeaponAttachmentsStorageComponent.Cast(owner.FindComponent(WeaponAttachmentsStorageComponent));
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
				outPrefabs.Insert(prefab);

			CollectSubtreeAttachments(attached, outPrefabs, depth + 1);
		}
	}
}
