// DCO native-GM-waypoint intent framework.

enum EDCO_WaypointIntentType
{
	NONE,
	CQB_CLEAR,
	QRF,
}

// Per-group placed-intent state.
modded class SCR_AIGroupUtilityComponent
{
	protected EDCO_WaypointIntentType	m_eDCO_WpIntent = EDCO_WaypointIntentType.NONE;
	protected vector					m_vDCO_WpIntentPos;
	protected AIWaypoint				m_DCO_WpIntentWaypoint;

	EDCO_WaypointIntentType DCO_GetWaypointIntent()
	{
		return m_eDCO_WpIntent;
	}

	vector DCO_GetWaypointIntentPos()
	{
		return m_vDCO_WpIntentPos;
	}

	AIWaypoint DCO_GetWaypointIntentWaypoint()
	{
		return m_DCO_WpIntentWaypoint;
	}

	void DCO_SetWaypointIntent(EDCO_WaypointIntentType type, vector pos, AIWaypoint wp)
	{
		m_eDCO_WpIntent = type;
		m_vDCO_WpIntentPos = pos;
		m_DCO_WpIntentWaypoint = wp;
	}

	void DCO_ClearWaypointIntent(AIWaypoint wp)
	{
		if (wp && m_DCO_WpIntentWaypoint && wp != m_DCO_WpIntentWaypoint)
			return;
		m_eDCO_WpIntent = EDCO_WaypointIntentType.NONE;
		m_vDCO_WpIntentPos = vector.Zero;
		m_DCO_WpIntentWaypoint = null;
	}

	void DCO_CompleteWaypointIntent()
	{
		AIWaypoint wp = m_DCO_WpIntentWaypoint;
		m_eDCO_WpIntent = EDCO_WaypointIntentType.NONE;
		m_vDCO_WpIntentPos = vector.Zero;
		m_DCO_WpIntentWaypoint = null;
		if (wp && m_Owner)
			m_Owner.RemoveWaypoint(wp);
	}
}

class DCO_WaypointIntentUtil
{
	// CONSTRAINT: the intent must be resolved LIVE from the group's current waypoint.
	protected static void Sync(SCR_AIGroupUtilityComponent util)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(util.GetOwner());
		if (!grp)
			return;
		AIWaypoint current = grp.GetCurrentWaypoint();
		DCO_IntentWaypoint intentWp = DCO_IntentWaypoint.Cast(current);
		if (!intentWp)
		{
			if (util.DCO_GetWaypointIntent() != EDCO_WaypointIntentType.NONE)
			{
				util.DCO_ClearWaypointIntent(util.DCO_GetWaypointIntentWaypoint());
				if (DCO_CqbClearSettings.Get().m_bDebugCqbClear)
					Print(string.Format("[DCO-WPI] intent CLEARED - current waypoint is %1", current), LogLevel.NORMAL);
			}
			return;
		}
		if (util.DCO_GetWaypointIntentWaypoint() == current)
		{
			util.DCO_SetWaypointIntent(intentWp.DCO_GetIntentType(), intentWp.GetOrigin(), current);
			return;
		}
		util.DCO_SetWaypointIntent(intentWp.DCO_GetIntentType(), intentWp.GetOrigin(), current);
		if (DCO_CqbClearSettings.Get().m_bDebugCqbClear)
			Print(string.Format("[DCO-WPI] intent ARMED type=%1 at %2", intentWp.DCO_GetIntentType(), intentWp.GetOrigin()), LogLevel.NORMAL);
	}

	static EDCO_WaypointIntentType GetIntent(SCR_AIGroupUtilityComponent util)
	{
		if (!util)
			return EDCO_WaypointIntentType.NONE;
		Sync(util);
		return util.DCO_GetWaypointIntent();
	}

	static bool HasIntent(SCR_AIGroupUtilityComponent util, EDCO_WaypointIntentType type)
	{
		if (!util)
			return false;
		Sync(util);
		return util.DCO_GetWaypointIntent() == type;
	}

	static vector GetIntentPos(SCR_AIGroupUtilityComponent util)
	{
		if (!util)
			return vector.Zero;
		Sync(util);	// same live resolve as GetIntent/HasIntent - never hand back a stale/zero position.
		return util.DCO_GetWaypointIntentPos();
	}

	// Finish the active intent: removes the GM waypoint and clears the state.
	static void Complete(SCR_AIGroupUtilityComponent util)
	{
		if (util)
			util.DCO_CompleteWaypointIntent();
	}

	static void ReleaseWaypointMovement(SCR_AIGroupUtilityComponent util)
	{
		if (!util)
			return;
		Sync(util);
		AIWaypoint wp = util.DCO_GetWaypointIntentWaypoint();
		if (wp)
			util.CancelActivitiesRelatedToWaypoint(wp, SCR_AIMoveActivity, true);
	}

	// Per-intent convenience so callers never need to reference the enum symbol directly.
	static bool IsCqbDirected(SCR_AIGroupUtilityComponent util)		{ return HasIntent(util, EDCO_WaypointIntentType.CQB_CLEAR); }
	static bool IsQrfDirected(SCR_AIGroupUtilityComponent util)		{ return HasIntent(util, EDCO_WaypointIntentType.QRF); }
}

// The generic DCO waypoint entity.
class DCO_IntentWaypointClass : SCR_AIWaypointClass
{
}

class DCO_IntentWaypoint : SCR_AIWaypoint
{
	[Attribute("0", UIWidgets.ComboBox, "Which DCO behaviour this waypoint drives when the group reaches it.", "", ParamEnumArray.FromEnum(EDCO_WaypointIntentType))]
	protected EDCO_WaypointIntentType m_eDCO_IntentType;

	EDCO_WaypointIntentType DCO_GetIntentType()
	{
		return m_eDCO_IntentType;
	}

	override SCR_AIWaypointState CreateWaypointState(SCR_AIGroupUtilityComponent groupUtilityComp)
	{
		return new DCO_IntentWaypointState(groupUtilityComp, this);
	}
}

// Waypoint processing state.
class DCO_IntentWaypointState : SCR_AIWaypointState
{
	override void OnExecuteWaypointTree()
	{
		super.OnExecuteWaypointTree();
		DCO_IntentWaypoint wp = DCO_IntentWaypoint.Cast(m_Waypoint);
		if (wp && m_Utility)
			m_Utility.DCO_SetWaypointIntent(wp.DCO_GetIntentType(), wp.GetOrigin(), m_Waypoint);
	}

	override void OnDeselected()
	{
		if (m_Utility)
			m_Utility.DCO_ClearWaypointIntent(m_Waypoint);
		super.OnDeselected();
	}
}
