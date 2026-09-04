// Bifrost tactics placement flow - the click-order -> area-on-cursor -> place -> auto-open-options pipeline.
class DCO_GMTacticsFlow
{
	protected static ref DCO_GMTacticsFlow s_Inst;
	static DCO_GMTacticsFlow Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_GMTacticsFlow();
		return s_Inst;
	}

	protected static const ResourceName PREFAB_ZONE_QRF      = "{841779BF689F6FB5}Prefabs/E_DCO_TaskZone.et";
	protected static const ResourceName PREFAB_ZONE_AMBUSH   = "{A524C110BDE25731}Prefabs/E_DCO_TaskZone_Ambush.et";
	protected static const ResourceName PREFAB_ZONE_KILLZONE = "{43C3E316165CD677}Prefabs/E_DCO_TaskZone_AmbushKillZone.et";
	protected static const ResourceName PREFAB_ZONE_DEFEND   = "{1D28FFA184C2D8EC}Prefabs/E_DCO_TaskZone_Defend.et";
	protected static const ResourceName PREFAB_WP_CQBCLEAR   = "{4C7A6BF3CF7D9D89}Prefabs/E_AIWaypoint_DCO_CqbClear.et";

	protected static const int PREVIEW_TICK_MS = 100;	// 10 Hz, the render-pillar rate the gizmo also uses.
	protected static const float PREVIEW_HEIGHT = 3.0;

	protected Widget m_wRoot;
	protected SCR_PlacingEditorComponent m_Placing;
	protected SCR_PreviewEntityEditorComponent m_Preview;
	protected bool m_bSubscribed;

	protected int m_iPendingTac = -1;
	protected ResourceName m_PendingPrefab;
	protected SCR_EditableEntityComponent m_PendingGroup;
	protected ref array<SCR_EditableEntityComponent> m_PendingGroups;
	protected float m_fPendingRadius;
	protected int m_iPendingColor;
	protected float m_fPendingExpireMs = -1;
	protected ref Shape m_PreviewCircle;

	protected static const float PENDING_GRACE_MS = 2000.0;

	void Init(Widget shellRoot)
	{
		s_Inst = this;
		m_wRoot = shellRoot;
		TrySubscribe();
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(TickPreview);
		if (m_Placing && m_bSubscribed)
		{
			m_Placing.GetOnPlaceEntity().Remove(OnPlaced);
			m_Placing.GetOnSelectedPrefabChange().Remove(OnPrefabChange);
		}
		m_bSubscribed = false;
		m_PreviewCircle = null;
		m_iPendingTac = -1;
		m_PendingPrefab = ResourceName.Empty;
		m_PendingGroup = null;
		m_PendingGroups = null;
		m_Placing = null;
		m_Preview = null;
		m_wRoot = null;
	}

	// Resolve the ACTIVE editor-mode placing + preview components and hook the placement events once.
	protected bool TrySubscribe()
	{
		if (!m_Placing)
			m_Placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (!m_Preview)
			m_Preview = SCR_PreviewEntityEditorComponent.Cast(SCR_PreviewEntityEditorComponent.GetInstance(SCR_PreviewEntityEditorComponent, false, true));
		if (!m_Placing)
			return false;
		if (!m_bSubscribed)
		{
			m_Placing.GetOnPlaceEntity().Insert(OnPlaced);
			m_Placing.GetOnSelectedPrefabChange().Insert(OnPrefabChange);
			m_bSubscribed = true;
		}
		return true;
	}

	// Arm cursor placement for a TACTICS order.
	bool BeginPlacement(int tacId, SCR_EditableEntityComponent group, array<SCR_EditableEntityComponent> allGroups = null)
	{
		if (!TrySubscribe())
		{
			Print("[DCO-GM] tactics flow: no placing component - cannot arm placement", LogLevel.WARNING);
			return false;
		}

		ResourceName prefab;
		float radius = 50.0;
		int color = 0xFFFFFFFF;
		bool isZone = true;
		if (tacId == DCO_GMGroupOrders.TAC_PLACE_AMBUSH)        { prefab = PREFAB_ZONE_AMBUSH;   radius = 30.0; color = 0xFFB050FF; }
		else if (tacId == DCO_GMGroupOrders.TAC_PLACE_KILLZONE) { prefab = PREFAB_ZONE_KILLZONE; radius = 30.0; color = 0xFFFF3030; }
		else if (tacId == DCO_GMGroupOrders.TAC_PLACE_DEFEND)   { prefab = PREFAB_ZONE_DEFEND;   radius = 50.0; color = 0xFF40C040; }
		else if (tacId == DCO_GMGroupOrders.TAC_PLACE_QRF)      { prefab = PREFAB_ZONE_QRF;      radius = 50.0; color = 0xFF3FA9F5; }
		else if (tacId == DCO_GMGroupOrders.TAC_PLACE_CLEAR)    { prefab = PREFAB_WP_CQBCLEAR;   isZone = false; }
		else
			return false;

		if (!isZone && !group)
		{
			Print("[DCO-GM] tactics flow: Clear Building needs a selected group (waypoint attaches to it)", LogLevel.WARNING);
			return false;
		}

		UnlockBrowser();

		array<SCR_EditableEntityComponent> pendingGroups = {};
		if (allGroups)
		{
			foreach (SCR_EditableEntityComponent g : allGroups)
			{
				if (g && pendingGroups.Find(g) < 0)
					pendingGroups.Insert(g);
			}
		}
		if (pendingGroups.IsEmpty() && group)
			pendingGroups.Insert(group);

		set<SCR_EditableEntityComponent> recipients;
		if (!isZone)
		{
			recipients = new set<SCR_EditableEntityComponent>();
			foreach (SCR_EditableEntityComponent pg : pendingGroups)
				recipients.Insert(pg);
		}

		bool ok = m_Placing.SetSelectedPrefab(prefab, false, true, recipients, null);
		if (!ok)
		{
			Print("[DCO-GM] tactics flow: SetSelectedPrefab REFUSED (unregistered prefab or budget) - placement aborted", LogLevel.WARNING);
			CancelPending();
			return false;
		}

		vector t[4];
		bool previewAlive = m_Preview && m_Preview.GetPreviewTransform(t);
		if (!previewAlive)
		{
			Print("[DCO-GM] tactics flow: no preview entity for the armed prefab; placement cancelled", LogLevel.WARNING);
			m_Placing.SetSelectedPrefab(ResourceName.Empty);
			CancelPending();
			return false;
		}

		m_iPendingTac = tacId;
		m_PendingPrefab = prefab;
		m_PendingGroup = group;
		m_PendingGroups = pendingGroups;
		m_fPendingRadius = radius;
		m_iPendingColor = color;
		m_fPendingExpireMs = -1;	// actively placing.

		GetGame().GetCallqueue().Remove(TickPreview);
		if (isZone)
			GetGame().GetCallqueue().CallLater(TickPreview, PREVIEW_TICK_MS, true);	// waypoints keep their native preview.
		Print(string.Format("[DCO-GM] tactics placement armed: action=%1 prefab=%2 groups=%3", tacId, prefab, pendingGroups.Count()), LogLevel.NORMAL);
		return true;
	}

	// True while the armed TACTICS context still applies: actively placing, or inside the grace window of an in-flight place-once confirm.
	protected bool PendingValid()
	{
		if (m_iPendingTac < 0)
			return false;
		if (m_fPendingExpireMs < 0)
			return true;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;
		return world.GetWorldTime() <= m_fPendingExpireMs;
	}

	protected void UnlockBrowser()
	{
		IEntity owner = m_Placing.GetOwner();
		if (!owner)
			return;
		SCR_ContentBrowserEditorComponent cb = SCR_ContentBrowserEditorComponent.Cast(owner.FindComponent(SCR_ContentBrowserEditorComponent));
		if (!cb)
			return;
		cb.ResetAllLabels(false);
		cb.SetCurrentSearch("");
		cb.FilterEntries();
	}

	protected void TickPreview()
	{
		if (!m_Placing || m_iPendingTac < 0 || m_Placing.GetSelectedPrefab().IsEmpty())
		{
			// Placement over - just stop drawing; OnPrefabChange owns whether the context grace-expires or clears.
			GetGame().GetCallqueue().Remove(TickPreview);
			m_PreviewCircle = null;
			return;
		}
		vector t[4];
		if (!m_Preview || !m_Preview.GetPreviewTransform(t))
			return;	// preview momentarily unavailable - keep the last circle.
		m_PreviewCircle = DCO_ZoneShape.FlatCircle(t[3], m_fPendingRadius, m_iPendingColor);
	}

	protected void OnPlaced(int prefabID, SCR_EditableEntityComponent entity)
	{
		if (!entity)
			return;
		IEntity owner = entity.GetOwner();
		if (!owner)
			return;

		bool armed = PendingValid();
		bool pendingClear = armed && m_iPendingTac == DCO_GMGroupOrders.TAC_PLACE_CLEAR;

		DCO_TaskZoneComponent zone = DCO_TaskZoneComponent.Cast(owner.FindComponent(DCO_TaskZoneComponent));
		if (zone)
		{
			Print(string.Format("[DCO-GM] tactics zone placed: action=%1 role=%2 position=%3", m_iPendingTac, zone.DCO_GetRole(), owner.GetOrigin()), LogLevel.NORMAL);
			m_PreviewCircle = null;	// the placed zone draws its own circle from here.
			SCR_EditableEntityComponent grp;
			array<SCR_EditableEntityComponent> grps;
			if (armed)
			{
				grp = m_PendingGroup;
				grps = m_PendingGroups;
			}
			DCO_GMTacticsPanel.Get().OpenForZone(entity, grp, grps);
			if (m_fPendingExpireMs >= 0)
				CancelPending();	// the grace context was for exactly this confirm - consume it.
			return;
		}

		if (pendingClear && DCO_IntentWaypoint.Cast(owner))
		{
			Print(string.Format("[DCO-GM] CQB clear waypoint placed: position=%1 groups=%2", owner.GetOrigin(), m_PendingGroups.Count()), LogLevel.NORMAL);
			DCO_GMTacticsPanel.Get().OpenForClearOrder(entity);
			CancelPending();
			return;
		}
	}

	// Placement prefab changed under us.
	protected void OnPrefabChange(ResourceName newPrefab, ResourceName prevPrefab)
	{
		if (m_iPendingTac < 0)
			return;
		if (newPrefab == m_PendingPrefab)
			return;

		GetGame().GetCallqueue().Remove(TickPreview);
		m_PreviewCircle = null;

		if (newPrefab.IsEmpty())
		{
			BaseWorld world = GetGame().GetWorld();
			if (world && m_fPendingExpireMs < 0)
				m_fPendingExpireMs = world.GetWorldTime() + PENDING_GRACE_MS;
			return;
		}
		CancelPending();
	}

	protected void CancelPending()
	{
		GetGame().GetCallqueue().Remove(TickPreview);
		m_PreviewCircle = null;
		m_iPendingTac = -1;
		m_PendingPrefab = ResourceName.Empty;
		m_PendingGroup = null;
		m_PendingGroups = null;
		m_fPendingExpireMs = -1;
	}
}
