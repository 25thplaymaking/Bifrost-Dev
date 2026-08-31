//! One hardpoint in the recursive weapon slot tree: the weapon's own slots, then the slots of
//! every attached item, any depth.
class GRSA_SlotNode
{
	string m_sLabel;
	string m_sTypePretty;
	typename m_SlotTypename;
	ResourceName m_AttachedPrefab;
	int m_iDepth;
}

class GRSA_ItemIntel
{
	//! Base game preview manager prefab, spawned locally so no custom preview world resource is ever shipped.
	protected static const ResourceName PREVIEW_MANAGER_PREFAB = "{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et";

	// Built on first use - eager static initializers charge the module-init budget shared by every loaded mod.
	protected static ref map<ResourceName, string> s_mAreaTypeCache;
	protected static ref map<ResourceName, float> s_mWeightCache;
	protected static ref map<ResourceName, float> s_mStorageLoadCache;
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
		s_mStorageLoadCache = new map<ResourceName, float>();
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
	//! Weight capacity of a wearable container prefab, 0 when it has no player-facing storage.
	static float GetStorageMaxLoad(ResourceName prefab)
	{
		EnsureCaches();
		float cached;
		if (s_mStorageLoadCache.Find(prefab, cached))
			return cached;

		float maxLoad;
		IEntity entity = ResolveEntity(prefab);
		if (entity)
		{
			SCR_UniversalInventoryStorageComponent storage = SCR_UniversalInventoryStorageComponent.Cast(entity.FindComponent(SCR_UniversalInventoryStorageComponent));
			if (storage)
				maxLoad = storage.GetMaxLoad();
		}

		s_mStorageLoadCache.Insert(prefab, maxLoad);
		return maxLoad;
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
