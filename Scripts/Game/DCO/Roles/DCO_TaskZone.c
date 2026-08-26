class DCO_ZoneShape
{
	static const float GROUND_LIFT = 0.3;

	static Shape FlatCircle(vector center, float radius, int colorARGB)
	{
		vector c = center + Vector(0, GROUND_LIFT, 0);
		vector p[64];
		for (int i = 0; i < 32; i++)
		{
			float a0 = (Math.PI2 * i) / 32.0;
			float a1 = (Math.PI2 * (i + 1)) / 32.0;
			p[i * 2]     = c + Vector(Math.Cos(a0) * radius, 0, Math.Sin(a0) * radius);
			p[i * 2 + 1] = c + Vector(Math.Cos(a1) * radius, 0, Math.Sin(a1) * radius);
		}
		return Shape.CreateLines(colorARGB, ShapeFlags.NOZBUFFER, p, 64);
	}
}

enum EDCO_ZoneRole
{
	NONE,
	QRF,	// groups inside become QRF responders; this circle is their hold/rally point.
	DEFEND,
	AMBUSH,
	AMBUSH_TRIGGER,	// kill zone: an enemy entering here springs paired AMBUSH zones.
	CLEAR,	// Reserved value; CQB clearing is waypoint-directed.
	REINFORCE,	// reserve groups stage here and use the proven QRF critical-support loop.
}

class DCO_TaskZoneRegistry
{
	protected static ref array<DCO_TaskZoneComponent> s_aZones;

	static void Register(DCO_TaskZoneComponent z)
	{
		if (!s_aZones)
			s_aZones = {};
		if (s_aZones.Find(z) < 0)
			s_aZones.Insert(z);
	}

	static void Unregister(DCO_TaskZoneComponent z)
	{
		if (s_aZones)
		{
			int i = s_aZones.Find(z);
			if (i >= 0)
				s_aZones.Remove(i);
		}
	}

	static array<DCO_TaskZoneComponent> GetZones()
	{
		if (!s_aZones)
			s_aZones = {};
		return s_aZones;
	}
}

class DCO_TaskZoneComponentClass : ScriptComponentClass
{
}

class DCO_TaskZoneComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "What task this circle assigns to the groups inside it.", "", ParamEnumArray.FromEnum(EDCO_ZoneRole), category: "Bifrost"), RplProp()]
	EDCO_ZoneRole m_eRole;

	[Attribute("50", UIWidgets.Slider, "Circle radius (m). Groups whose leader is inside become assigned; for a kill zone, an enemy inside springs the ambush.", "5 500 5", category: "Bifrost"), RplProp()]
	float m_fRadius;

	[Attribute("3", UIWidgets.Slider, "How often (s) the zone re-evaluates which groups/enemies are inside.", "0.5 15 0.5", category: "Bifrost")]
	float m_fCheckSec;

	[Attribute("0", UIWidgets.Slider, "Pair ID linking an Ambush Position to its Kill-Zone(s). Set the SAME non-zero number on a position and each of its kill-zones. 0 = unlinked: a kill-zone then springs the NEAREST ambush position and deletes nothing.", "0 50 1", category: "Bifrost"), RplProp()]
	int m_iPairId;

	[Attribute("0", UIWidgets.Slider, "Role range (m) pushed to the groups this zone manages: QRF response range for a QRF zone, ambush spring range for an Ambush Position. 0 = leave each group's own setting untouched.", "0 3000 10", category: "Bifrost"), RplProp()]
	float m_fPushRange;

	protected ref array<SCR_AIGroup> m_aManaged = {};

	[RplProp()]
	protected bool m_bDCO_Tripped = false;

	// GM-visible ground circle.
	protected ref Shape m_DCO_VisualShape;
	protected ref Shape m_DCO_TriggerRing;
	protected static const float DCO_VISUAL_HEIGHT = 3.0;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame() || !GetGame().InPlayMode())
			return;

		GetGame().GetCallqueue().CallLater(DCO_DrawVisual, 500, false);
		GetGame().GetCallqueue().CallLater(DCO_DrawVisual, (int)(m_fCheckSec * 1000.0), true);

		if (!Replication.IsServer())
			return;

		DCO_TaskZoneRegistry.Register(this);
		GetGame().GetCallqueue().CallLater(DCO_Tick, (int)(m_fCheckSec * 1000.0), true);
	}

	void ~DCO_TaskZoneComponent()
	{
		DCO_TaskZoneRegistry.Unregister(this);
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(DCO_Tick);
			GetGame().GetCallqueue().Remove(DCO_DrawVisual);
		}
		DCO_ClearAllManaged();
		m_DCO_VisualShape = null;
		m_DCO_TriggerRing = null;
	}

	vector DCO_GetCenter()
	{
		IEntity owner = GetOwner();
		if (owner)
			return owner.GetOrigin();
		return vector.Zero;
	}

	float DCO_GetRadius()		{ return m_fRadius; }
	EDCO_ZoneRole DCO_GetRole()	{ return m_eRole; }
	int DCO_GetPairId()			{ return m_iPairId; }
	void DCO_SetPairId(int id)
	{
		m_iPairId = Math.ClampInt(id, 0, 50);
		if (Replication.IsServer())
			Replication.BumpMe();
	}
	float DCO_GetPushRange()	{ return m_fPushRange; }

	void DCO_SetRadius(float r)
	{
		m_fRadius = Math.Clamp(r, 5.0, 500.0);
		DCO_DrawVisual();
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	// GM range write.
	void DCO_SetPushRange(float r)
	{
		m_fPushRange = Math.Clamp(r, 0.0, 3000.0);
		if (Replication.IsServer())
			Replication.BumpMe();
		for (int i = 0, c = m_aManaged.Count(); i < c; i++)
		{
			SCR_AIGroup grp = m_aManaged[i];
			if (grp)
				DCO_PushRangeTo(grp);
		}
	}

	void DCO_RearmTrigger()
	{
		m_bDCO_Tripped = false;
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	protected void DCO_PushRangeTo(SCR_AIGroup grp)
	{
		if (m_fPushRange <= 0)
			return;
		SCR_AIGroupUtilityComponent util = grp.GetGroupUtilityComponent();
		if (!util)
			return;
		if (m_eRole == EDCO_ZoneRole.QRF || m_eRole == EDCO_ZoneRole.REINFORCE)
			util.DCO_SetQRFRange(m_fPushRange);
		else if (m_eRole == EDCO_ZoneRole.AMBUSH)
			util.DCO_SetAmbushRange(m_fPushRange);
	}

	protected float DCO_FlatDistSq(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}

	protected void DCO_Tick()
	{
		if (!Replication.IsServer() || m_eRole == EDCO_ZoneRole.NONE)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		// Visual circle is parented to this zone, so it follows automatically - no per-tick reposition.

		if (m_eRole == EDCO_ZoneRole.AMBUSH_TRIGGER)
		{
			DCO_TickTrigger();
			return;
		}

		DCO_TickGroupAssignment();
	}

	// ARGB ground-circle colour per role, so a Defend zone no longer reads as a red kill marker.
	protected int DCO_VisualColor()
	{
		switch (m_eRole)
		{
			case EDCO_ZoneRole.QRF:				return 0xFF3FA9F5;	// blue.
			case EDCO_ZoneRole.DEFEND:			return 0xFF40C040;	// green.
			case EDCO_ZoneRole.AMBUSH:			return 0xFFB050FF;	// purple.
			case EDCO_ZoneRole.AMBUSH_TRIGGER:	return 0xFFFF3030;	// red = the actual kill zone.
			case EDCO_ZoneRole.CLEAR:			return 0xFFFF8C00;	// dark orange = clear zone.
			case EDCO_ZoneRole.REINFORCE:		return 0xFFFFD24A;	// yellow = reserve/reinforce.
		}
		return 0xFFFFFFFF;
	}

	protected void DCO_DrawVisual()
	{
		if (!DCO_GMRights.IsLocalGameMaster())
		{
			m_DCO_VisualShape = null;
			m_DCO_TriggerRing = null;
			return;
		}

		if (m_eRole == EDCO_ZoneRole.NONE || m_fRadius < 1.0)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		int color = 0xFFFFFFFF;
		if (m_eRole == EDCO_ZoneRole.AMBUSH_TRIGGER)
			color = 0xFFFF3030;
		m_DCO_VisualShape = DCO_ZoneShape.FlatCircle(owner.GetOrigin(), m_fRadius, color);

		m_DCO_TriggerRing = null;
		if (m_eRole == EDCO_ZoneRole.AMBUSH)
		{
			float triggerRadius = m_fPushRange;
			if (triggerRadius <= 1.0)
				triggerRadius = 50.0;
			m_DCO_TriggerRing = DCO_ZoneShape.FlatCircle(owner.GetOrigin(), triggerRadius, 0xFFFF3030);
		}
	}

	// Apply this zone's role to every group whose leader is inside the circle, and clear it from any group that has since left.
	protected void DCO_TickGroupAssignment()
	{
		vector center = DCO_GetCenter();
		float radiusSq = m_fRadius * m_fRadius;

		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);

		array<SCR_AIGroup> inside = {};
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
			if (!grp || inside.Find(grp) >= 0)
				continue;
			IEntity leader = grp.GetLeaderEntity();
			if (!leader)
				continue;
			if (DCO_PlayerUtil.IsPlayer(leader))
				continue;
			if (DCO_FlatDistSq(leader.GetOrigin(), center) <= radiusSq)
				inside.Insert(grp);
		}

		// Apply to groups now inside that we weren't already managing.
		for (int i = 0, c = inside.Count(); i < c; i++)
		{
			SCR_AIGroup grp = inside[i];
			if (m_aManaged.Find(grp) < 0)
			{
				DCO_ApplyRoleTo(grp, true);
				m_aManaged.Insert(grp);
			}
		}

		for (int i = m_aManaged.Count() - 1; i >= 0; i--)
		{
			SCR_AIGroup grp = m_aManaged[i];
			if (!grp || inside.Find(grp) < 0)
			{
				if (grp)
					DCO_ApplyRoleTo(grp, false);
				m_aManaged.Remove(i);
			}
		}
	}

	// Kill zone tick.
	protected void DCO_TickTrigger()
	{
		if (m_bDCO_Tripped)
			return;

		array<DCO_TaskZoneComponent> positions = {};
		DCO_GetPairedPositions(positions);
		if (positions.IsEmpty())
			return;	// no armed ambush paired yet.

		Faction ambusherFaction;
		foreach (DCO_TaskZoneComponent p : positions)
		{
			ambusherFaction = p.DCO_GetFirstManagedFaction();
			if (ambusherFaction)
				break;
		}
		if (!ambusherFaction)
			return;	// the paired ambush has no group assigned yet.

		if (!DCO_EnemyInsideRelativeTo(ambusherFaction))
			return;

		// Tripped - spring every paired ambush position once.
		m_bDCO_Tripped = true;
		Replication.BumpMe();
		foreach (DCO_TaskZoneComponent p : positions)
			p.DCO_SpringManagedAmbushes();

		if (m_iPairId != 0)
			DCO_DeleteSiblingTriggers();
	}

	protected void DCO_GetPairedPositions(out array<DCO_TaskZoneComponent> result)
	{
		array<DCO_TaskZoneComponent> zones = DCO_TaskZoneRegistry.GetZones();

		if (m_iPairId != 0)
		{
			foreach (DCO_TaskZoneComponent z : zones)
			{
				if (!z || z == this)
					continue;
				if (z.DCO_GetRole() == EDCO_ZoneRole.AMBUSH && z.DCO_GetPairId() == m_iPairId)
					result.Insert(z);
			}
			return;
		}

		// Pair ID 0: nearest AMBUSH zone by flat distance.
		vector center = DCO_GetCenter();
		DCO_TaskZoneComponent nearest;
		float bestSq = -1;
		foreach (DCO_TaskZoneComponent z : zones)
		{
			if (!z || z == this || z.DCO_GetRole() != EDCO_ZoneRole.AMBUSH)
				continue;
			float dSq = DCO_FlatDistSq(z.DCO_GetCenter(), center);
			if (bestSq < 0 || dSq < bestSq)
			{
				bestSq = dSq;
				nearest = z;
			}
		}
		if (nearest)
			result.Insert(nearest);
	}

	protected void DCO_DeleteSiblingTriggers()
	{
		array<DCO_TaskZoneComponent> zones = DCO_TaskZoneRegistry.GetZones();
		array<IEntity> toDelete = {};
		foreach (DCO_TaskZoneComponent z : zones)
		{
			if (!z || z == this)
				continue;
			if (z.DCO_GetRole() != EDCO_ZoneRole.AMBUSH_TRIGGER)
				continue;
			if (z.DCO_GetPairId() != m_iPairId)
				continue;
			IEntity e = z.GetOwner();
			if (e)
				toDelete.Insert(e);
		}

		foreach (IEntity e : toDelete)
			SCR_EntityHelper.DeleteEntityAndChildren(e);
	}

	Faction DCO_GetFirstManagedFaction()
	{
		for (int i = 0, c = m_aManaged.Count(); i < c; i++)
		{
			SCR_AIGroup grp = m_aManaged[i];
			if (grp)
			{
				Faction f = grp.GetFaction();
				if (f)
					return f;
			}
		}
		return null;
	}

	protected bool DCO_EnemyInsideRelativeTo(Faction refFaction)
	{
		vector center = DCO_GetCenter();
		float radiusSq = m_fRadius * m_fRadius;
		SCR_Faction refScr = SCR_Faction.Cast(refFaction);

		// AI.
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (aiWorld)
		{
			array<AIAgent> agents = {};
			aiWorld.GetAIAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;
				IEntity ent = agent.GetControlledEntity();
				if (!ent)
					continue;
				if (DCO_FlatDistSq(ent.GetOrigin(), center) > radiusSq)
					continue;
				if (DCO_IsHostile(refScr, ent))
					return true;
			}
		}

		// Players.
		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			array<int> ids = {};
			pm.GetPlayers(ids);
			foreach (int id : ids)
			{
				IEntity ent = pm.GetPlayerControlledEntity(id);
				if (!ent)
					continue;
				if (DCO_FlatDistSq(ent.GetOrigin(), center) > radiusSq)
					continue;
				if (DCO_IsHostile(refScr, ent))
					return true;
			}
		}

		return false;
	}

	protected bool DCO_IsHostile(SCR_Faction refScr, IEntity ent)
	{
		if (!refScr)
			return false;
		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (!fac)
			return false;
		Faction other = fac.GetAffiliatedFaction();
		if (!other || other == refScr)
			return false;
		return refScr.IsFactionEnemy(other);
	}

	void DCO_SpringManagedAmbushes()
	{
		for (int i = 0, c = m_aManaged.Count(); i < c; i++)
		{
			SCR_AIGroup grp = m_aManaged[i];
			if (!grp)
				continue;
			SCR_AIGroupUtilityComponent util = grp.GetGroupUtilityComponent();
			if (util)
				util.DCO_SpringAmbush();
		}
	}

	protected void DCO_ApplyRoleTo(SCR_AIGroup grp, bool enable)
	{
		SCR_AIGroupUtilityComponent util = grp.GetGroupUtilityComponent();
		if (!util)
			return;

		switch (m_eRole)
		{
			case EDCO_ZoneRole.QRF:
			{
				util.DCO_SetQRFResponder(enable);
				if (enable)
				{
					util.DCO_SetQRFHoldPosition(DCO_GetCenter());
					DCO_PushRangeTo(grp);
				}
				else
					util.DCO_ClearQRFHoldPosition();
				break;
			}
			case EDCO_ZoneRole.REINFORCE:
			{
				util.DCO_SetQRFResponder(enable);
				if (enable)
				{
					util.DCO_SetQRFHoldPosition(DCO_GetCenter());
					DCO_PushRangeTo(grp);
				}
				else
					util.DCO_ClearQRFHoldPosition();
				break;
			}
			case EDCO_ZoneRole.DEFEND:
			{
				util.DCO_SetDefender(enable);
				break;
			}
			case EDCO_ZoneRole.AMBUSH:
			{
				util.DCO_SetAmbusher(enable);
				if (enable)
					DCO_PushRangeTo(grp);
				break;
			}
			case EDCO_ZoneRole.CLEAR:
			{
				break;
			}
		}
	}

	protected void DCO_ClearAllManaged()
	{
		for (int i = m_aManaged.Count() - 1; i >= 0; i--)
		{
			SCR_AIGroup grp = m_aManaged[i];
			if (grp)
				DCO_ApplyRoleTo(grp, false);
		}
		m_aManaged.Clear();
	}
}

class DCO_TaskZoneGMTools
{
	static DCO_TaskZoneComponent ZoneOf(IEntity target)
	{
		if (!target)
			return null;
		return DCO_TaskZoneComponent.Cast(target.FindComponent(DCO_TaskZoneComponent));
	}

	static void SetRadius(IEntity target, float r)
	{
		DCO_TaskZoneComponent z = ZoneOf(target);
		if (z)
			z.DCO_SetRadius(r);
	}

	static void SetPushRange(IEntity target, float r)
	{
		DCO_TaskZoneComponent z = ZoneOf(target);
		if (z)
			z.DCO_SetPushRange(r);
	}

	static void SetPairId(IEntity target, int id)
	{
		DCO_TaskZoneComponent z = ZoneOf(target);
		if (z)
			z.DCO_SetPairId(Math.ClampInt(id, 0, 50));
	}

	static void Spring(IEntity target)
	{
		DCO_TaskZoneComponent z = ZoneOf(target);
		if (z)
			z.DCO_SpringManagedAmbushes();
	}

	static void Rearm(IEntity target)
	{
		DCO_TaskZoneComponent z = ZoneOf(target);
		if (z)
			z.DCO_RearmTrigger();
	}

	static void SendGroupToZone(IEntity groupEnt, vector zoneCenter)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(groupEnt);
		if (!grp)
			return;
		IEntity leader = grp.GetLeaderEntity();
		if (!leader || DCO_PlayerUtil.IsPlayer(leader))
			return;	// never order a player-led group.

		// Nearest registered zone to the payload center.
		DCO_TaskZoneComponent zone;
		float bestSq = 15.0 * 15.0;
		array<DCO_TaskZoneComponent> zones = DCO_TaskZoneRegistry.GetZones();
		foreach (DCO_TaskZoneComponent z : zones)
		{
			if (!z)
				continue;
			vector c = z.DCO_GetCenter();
			float dx = c[0] - zoneCenter[0];
			float dz = c[2] - zoneCenter[2];
			float dSq = dx * dx + dz * dz;
			if (dSq <= bestSq)
			{
				bestSq = dSq;
				zone = z;
			}
		}
		if (!zone)
		{
			Print("[DCO-GM] send-group: no registered zone at the commit position - order dropped", LogLevel.WARNING);
			return;
		}

		AICommunicationComponent comms = AICommunicationComponent.Cast(groupEnt.FindComponent(AICommunicationComponent));
		if (!comms)
			return;
		DCO_VehicleUtil.OrderGroupMoveToEntity(grp, zone.GetOwner(), comms);
		Print("[DCO-GM] send-group: move order issued toward the placed zone", LogLevel.NORMAL);
	}
}
