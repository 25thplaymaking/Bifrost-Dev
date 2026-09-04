modded class SCR_AIWaypoint
{
	// AIWaypoint's engine-owned completion radius is not available as a script
	// RplProp. Mirror it so remote GMs and JIP clients drive the native white
	// completion circle and the server's arrival test from the same value.
	[RplProp(onRplName: "DCO_OnCompletionRadiusReplicated")]
	protected float m_fDCOCompletionRadius = -1;

	float DCO_GetCompletionRadius()
	{
		if (m_fDCOCompletionRadius >= 1)
			return m_fDCOCompletionRadius;
		return GetCompletionRadius();
	}

	void DCO_SetCompletionRadius(float radius)
	{
		if (!Replication.IsServer())
			return;
		radius = Math.Clamp(radius, 1, 500);
		if (m_fDCOCompletionRadius == radius && GetCompletionRadius() == radius)
			return;

		m_fDCOCompletionRadius = radius;
		SetCompletionRadius(radius);
		Replication.BumpMe();

		GetOnWaypointPropertiesChanged().Invoke();
		DCO_RefreshCompletionCircle();
		Print(string.Format("[DCO-GM] waypoint completion radius updated: waypoint=%1 radius=%2m areaMesh=%3", this, radius,
			FindComponent(SCR_WaypointAreaMeshComponent) != null), LogLevel.NORMAL);
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
