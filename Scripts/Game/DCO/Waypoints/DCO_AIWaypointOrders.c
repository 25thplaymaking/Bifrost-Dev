enum EDCO_WaypointApproach
{
	NATIVE,
	TACTICAL,
	RUSH,
	CHARGE,
	DCO_FLANK,
	DCO_COVERED
}

modded class SCR_AIWaypoint
{
	[RplProp()]
	protected int m_iDCOApproach = EDCO_WaypointApproach.NATIVE;

	// AIWaypoint's engine-owned completion radius is not available as a script
	// RplProp. Mirror it so remote GMs and JIP clients drive the native white
	// completion circle and the server's arrival test from the same value.
	[RplProp(onRplName: "DCO_OnCompletionRadiusReplicated")]
	protected float m_fDCOCompletionRadius = -1;

	int DCO_GetApproach()
	{
		return m_iDCOApproach;
	}

	void DCO_SetApproach(int approach)
	{
		approach = Math.Clamp(approach, EDCO_WaypointApproach.NATIVE, EDCO_WaypointApproach.DCO_COVERED);
		if (m_iDCOApproach == approach)
			return;

		m_iDCOApproach = approach;
		if (Replication.IsServer())
			Replication.BumpMe();

		GetOnWaypointPropertiesChanged().Invoke();
	}

	float DCO_GetCompletionRadius()
	{
		if (m_fDCOCompletionRadius >= 1)
			return m_fDCOCompletionRadius;
		return GetCompletionRadius();
	}

	void DCO_SetCompletionRadius(float radius)
	{
		radius = Math.Clamp(radius, 1, 500);
		if (m_fDCOCompletionRadius == radius && GetCompletionRadius() == radius)
			return;

		m_fDCOCompletionRadius = radius;
		SetCompletionRadius(radius);
		if (Replication.IsServer())
			Replication.BumpMe();

		GetOnWaypointPropertiesChanged().Invoke();
		DCO_RefreshCompletionCircle();
	}

	protected void DCO_OnCompletionRadiusReplicated()
	{
		if (m_fDCOCompletionRadius < 1)
			return;
		SetCompletionRadius(m_fDCOCompletionRadius);
		GetOnWaypointPropertiesChanged().Invoke();
		DCO_RefreshCompletionCircle();
	}

	protected void DCO_RefreshCompletionCircle()
	{
		SCR_WaypointAreaMeshComponent circle = SCR_WaypointAreaMeshComponent.Cast(FindComponent(SCR_WaypointAreaMeshComponent));
		if (circle)
			circle.GenerateAreaMesh();
	}
}

class DCO_AIWaypointAttributeHelper
{
	static SCR_AIWaypoint GetWaypoint(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;

		return SCR_AIWaypoint.Cast(editable.GetOwner());
	}
}

[BaseContainerProps()]
class DCO_AIWaypointRadiusEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIWaypoint waypoint = DCO_AIWaypointAttributeHelper.GetWaypoint(item);
		if (!waypoint)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(waypoint.DCO_GetCompletionRadius());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_AIWaypoint waypoint = DCO_AIWaypointAttributeHelper.GetWaypoint(item);
		if (!waypoint)
			return;

		waypoint.DCO_SetCompletionRadius(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_AIWaypointApproachEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIWaypoint waypoint = DCO_AIWaypointAttributeHelper.GetWaypoint(item);
		if (!waypoint)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(waypoint.DCO_GetApproach());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_AIWaypoint waypoint = DCO_AIWaypointAttributeHelper.GetWaypoint(item);
		if (waypoint)
			waypoint.DCO_SetApproach(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Native"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Tactical"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Rush"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Charge"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("DCO Flank"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("DCO Covered"));
		return outEntries.Count();
	}
}
