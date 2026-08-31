//! One warmup work item and the catalog metadata its intel reads need.
class GRSA_WarmupItem
{
	ResourceName m_Prefab;
	SCR_EArsenalItemType m_eType;
	SCR_EArsenalItemMode m_eMode;
}

//! Startup catalog scan owned by the game-mode manager: walks every arsenal item the loaded
//! entity catalogs offer and warms the per-prefab intel caches (cloth area, weight, display
//! name, attachment class, weapon factory attachments) in frame-sized slices, so the first
//! armory open pays map lookups instead of one preview-entity resolve per item. Never runs on
//! dedicated servers — the server apply path (GRSA_ApplyGate) reads catalogs only, and the
//! entity-derived intel feeds menus a dedicated server never draws.
class GRSA_CatalogWarmup
{
	//! Each resolve is a prefab load + pooled spawn — 4 per 100 ms keeps every slice inside a
	//! frame budget and warms a ~600-item catalog in ~15 s of load/lobby time.
	protected static const int ITEMS_PER_SLICE = 4;
	protected static const int SLICE_INTERVAL_MS = 100;

	protected ref array<ref GRSA_WarmupItem> m_aPending = {};
	protected int m_iTotal;
	protected bool m_bRunning;

	//------------------------------------------------------------------------------------------------
	void Begin()
	{
		if (m_bRunning)
			return;

		CollectItems();
		m_iTotal = m_aPending.Count();
		if (m_iTotal == 0)
			return;

		m_bRunning = true;
		GetGame().GetCallqueue().CallLater(Slice, SLICE_INTERVAL_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	void Cancel()
	{
		if (!m_bRunning)
			return;

		GetGame().GetCallqueue().Remove(Slice);
		m_bRunning = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Union of every faction and factionless arsenal catalog, deduplicated by prefab.
	protected void CollectItems()
	{
		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
		{
			GRSA_Log.Warn("Catalog warmup: no entity catalog manager, armory falls back to lazy caching");
			return;
		}

		SCR_EArsenalGameModeType gameModeType = SCR_ArsenalManagerComponent.GetArsenalGameModeType_Static();

		map<ResourceName, bool> seen = new map<ResourceName, bool>();
		array<SCR_ArsenalItem> arsenalItems = {};
		catalogManager.GetAllArsenalItems(arsenalItems, -1, -1, gameModeType);
		foreach (SCR_ArsenalItem arsenalItem : arsenalItems)
		{
			if (!arsenalItem)
				continue;

			ResourceName prefab = arsenalItem.GetItemResourceName();
			if (prefab.IsEmpty() || seen.Contains(prefab))
				continue;

			seen.Insert(prefab, true);
			GRSA_WarmupItem item = new GRSA_WarmupItem();
			item.m_Prefab = prefab;
			item.m_eType = arsenalItem.GetItemType();
			item.m_eMode = arsenalItem.GetItemMode();
			m_aPending.Insert(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! One frame-sized slice: each item pays its preview-entity resolve here, during load or
	//! lobby time, filling the same static caches the menus read later.
	protected void Slice()
	{
		int budget = ITEMS_PER_SLICE;
		while (budget > 0 && !m_aPending.IsEmpty())
		{
			int last = m_aPending.Count() - 1;
			GRSA_WarmupItem item = m_aPending[last];
			m_aPending.Remove(last);
			budget--;

			GRSA_ItemIntel.GetClothAreaType(item.m_Prefab);
			GRSA_ItemIntel.GetWeight(item.m_Prefab);
			GRSA_CatalogService.GetDisplayName(item.m_Prefab, null);

			if (item.m_eType & SCR_EArsenalItemType.WEAPON_ATTACHMENT)
				GRSA_ItemIntel.GetAttachmentClass(item.m_Prefab);

			if (item.m_eMode & (SCR_EArsenalItemMode.WEAPON | SCR_EArsenalItemMode.WEAPON_VARIANTS))
			{
				array<ResourceName> defaults = {};
				GRSA_ItemIntel.GetDefaultAttachments(item.m_Prefab, defaults);
			}

		}

		if (m_aPending.IsEmpty())
			Cancel();
	}
}
