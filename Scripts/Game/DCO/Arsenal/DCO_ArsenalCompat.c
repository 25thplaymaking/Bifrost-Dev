// Bifrost Arsenal - weapon compatibility introspection.
class DCO_ArsenalCompat
{
	protected static ref map<string, string> s_AttachTypes = new map<string, string>();
	protected static ref map<string, string> s_MagWells = new map<string, string>();

	static bool IsIntegralAttachment(IEntity item)
	{
		if (!item)
			return false;
		InventoryItemComponent ic = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!ic)
			return false;
		InventoryStorageSlot slot = ic.GetParentSlot();
		if (!slot)
			return false;
		AttachmentSlotComponent asc = AttachmentSlotComponent.Cast(slot.GetParentContainer());
		if (!asc)
			return false;
		return !asc.ShouldShowInInspection();
	}

	// What the live weapon accepts: attachment-slot types enumerated from the live item's components, and magazine wells per muzzle.
	static void GetWeaponSlotTypes(IEntity weapon, out notnull array<typename> outTypes)
	{
		outTypes.Clear();
		if (!weapon)
			return;
		array<Managed> comps = {};
		weapon.FindComponents(AttachmentSlotComponent, comps);
		foreach (Managed m : comps)
		{
			AttachmentSlotComponent slot = AttachmentSlotComponent.Cast(m);
			if (!slot)
				continue;
			if (!slot.ShouldShowInInspection())
				continue;
			BaseAttachmentType t = slot.GetAttachmentSlotType();
			if (!t)
				continue;
			if (outTypes.Find(t.Type()) < 0)
				outTypes.Insert(t.Type());
		}
	}

	static void GetWeaponMagWells(IEntity weapon, out notnull array<typename> outWells)
	{
		outWells.Clear();
		if (!weapon)
			return;
		array<Managed> comps = {};
		weapon.FindComponents(BaseMuzzleComponent, comps);
		foreach (Managed m : comps)
		{
			BaseMuzzleComponent muzzle = BaseMuzzleComponent.Cast(m);
			if (!muzzle)
				continue;
			BaseMagazineWell well = muzzle.GetMagazineWell();
			if (!well)
				continue;
			if (outWells.Find(well.Type()) < 0)
				outWells.Insert(well.Type());
		}
	}

	static void FilterForWeapon(IEntity weapon, string search, out notnull array<DCO_ArsenalEntry> outEntries)
	{
		outEntries.Clear();
		array<typename> slotTypes = {};
		array<typename> wells = {};
		GetWeaponSlotTypes(weapon, slotTypes);
		GetWeaponMagWells(weapon, wells);

		DCO_ArsenalCatalog cat = DCO_ArsenalCatalog.Get();
		array<DCO_ArsenalEntry> pool = {};
		int attTotal = 0;
		int magTotal = 0;

		cat.GetEntries(EDCO_ArsenalCategory.ATTACHMENTS, search, "", pool);
		attTotal = pool.Count();
		foreach (DCO_ArsenalEntry e : pool)
		{
			if (AttachmentFitsAny(e.m_Prefab, slotTypes))
				outEntries.Insert(e);
		}
		int attKept = outEntries.Count();

		cat.GetEntries(EDCO_ArsenalCategory.MAGAZINES, search, "", pool);
		magTotal = pool.Count();
		foreach (DCO_ArsenalEntry e : pool)
		{
			if (MagazineFitsAny(e.m_Prefab, wells))
				outEntries.Insert(e);
		}
		Print(string.Format("[DCO-ARS][compat] weapon slots=%1 wells=%2 -> att %3/%4, mag %5/%6",
			slotTypes.Count(), wells.Count(), attKept, attTotal, outEntries.Count() - attKept, magTotal), LogLevel.NORMAL);
	}

	static bool AttachmentFitsAny(ResourceName prefab, array<typename> slotTypes)
	{
		if (slotTypes.IsEmpty())
			return false;
		typename at = ClassifiedAttachType(prefab);
		if (!at)
			return false;
		foreach (typename slotType : slotTypes)
		{
			if (at.IsInherited(slotType))
				return true;
		}
		return false;
	}

	static bool MagazineFitsAny(ResourceName prefab, array<typename> wells)
	{
		if (wells.IsEmpty())
			return false;
		typename mw = ClassifiedMagWell(prefab);
		if (!mw)
			return false;
		foreach (typename well : wells)
		{
			if (mw.IsInherited(well) || well.IsInherited(mw))
				return true;
		}
		return false;
	}

	static bool IsMagazinePrefab(ResourceName prefab)
	{
		return ClassifiedMagWell(prefab) != typename.Empty;
	}

	static typename AttachTypeOfPrefab(ResourceName prefab)
	{
		return ClassifiedAttachType(prefab);
	}

	// Cached prefab-source classification.
	protected static typename ClassifiedAttachType(ResourceName prefab)
	{
		string key = prefab;
		string cached;
		if (s_AttachTypes.Find(key, cached))
			return cached.ToType();
		string name = ReadAttachmentTypeName(prefab);
		s_AttachTypes.Insert(key, name);
		return name.ToType();
	}

	protected static typename ClassifiedMagWell(ResourceName prefab)
	{
		string key = prefab;
		string cached;
		if (s_MagWells.Find(key, cached))
			return cached.ToType();
		string name = ReadMagWellName(prefab);
		s_MagWells.Insert(key, name);
		return name.ToType();
	}

	protected static string ReadAttachmentTypeName(ResourceName prefab)
	{
		IEntitySource src = LoadSource(prefab);
		if (!src)
			return "";
		for (int i = 0, n = src.GetComponentCount(); i < n; i++)
		{
			IEntityComponentSource comp = src.GetComponent(i);
			if (!comp)
				continue;
			typename ct = comp.GetClassName().ToType();
			if (!ct)
				continue;
			bool carrier = ct.IsInherited(InventoryItemComponent) || ct.IsInherited(WeaponAttachmentsStorageComponent);
			if (!carrier)
				continue;
			string name = AttachTypeFromComponent(comp.ToBaseContainer());
			if (!name.IsEmpty())
				return name;
		}
		return "";
	}

	protected static string AttachTypeFromComponent(BaseContainer compContainer)
	{
		if (!compContainer)
			return "";
		BaseContainer attrs = compContainer.GetObject("Attributes");
		if (!attrs)
			return "";
		BaseContainerList custom = attrs.GetObjectArray("CustomAttributes");
		if (!custom)
			return "";
		for (int j = 0, m = custom.Count(); j < m; j++)
		{
			BaseContainer entry = custom.Get(j);
			if (!entry)
				continue;
			typename et = entry.GetClassName().ToType();
			if (!et || !et.IsInherited(WeaponAttachmentAttributes))
				continue;
			BaseContainer typeObj = entry.GetObject("AttachmentType");
			if (typeObj)
				return typeObj.GetClassName();
		}
		return "";
	}

	protected static string ReadMagWellName(ResourceName prefab)
	{
		IEntitySource src = LoadSource(prefab);
		if (!src)
			return "";
		for (int i = 0, n = src.GetComponentCount(); i < n; i++)
		{
			IEntityComponentSource comp = src.GetComponent(i);
			if (!comp)
				continue;
			typename ct = comp.GetClassName().ToType();
			if (!ct || !ct.IsInherited(BaseMagazineComponent))
				continue;
			BaseContainer cc = comp.ToBaseContainer();
			if (!cc)
				continue;
			BaseContainer well = cc.GetObject("MagazineWell");
			if (well)
				return well.GetClassName();
		}
		return "";
	}

	protected static IEntitySource LoadSource(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return null;
		Resource res = Resource.Load(prefab);
		if (!res || !res.IsValid())
			return null;
		return res.GetResource().ToEntitySource();
	}
}
