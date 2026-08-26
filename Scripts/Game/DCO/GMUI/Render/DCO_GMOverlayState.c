enum EDCO_OverlayScope
{
	SELECTED,	// selected character, or every character inside a selected group.
	ALL
}

// Shared on/off + scope state for the GM tactical-overlay cues.
class DCO_GMOverlayState
{
	protected static ref DCO_GMOverlayState s_Instance;

	static const int OV_CONES    = 0;
	static const int OV_MOVEMENT = 1;
	static const int OV_MARKERS  = 2;
	static const int OV_FPS      = 3;

	bool m_bViewCones;
	bool m_bMovement;
	bool m_bMarkers;
	bool m_bPlayerFps;
	bool m_bGarrisonTP;	// highlight buildings that will receive a garrison teleport.

	int m_ScopeViewCones;
	int m_ScopeMovement;
	int m_ScopeMarkers;

	static DCO_GMOverlayState Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMOverlayState();
		return s_Instance;
	}

	bool AnyOn()
	{
		return m_bViewCones || m_bMovement || m_bMarkers || m_bGarrisonTP;
	}

	bool AnyUnitOverlay()
	{
		return m_bViewCones || m_bMovement || m_bMarkers;
	}

	// Generic on/off accessors so the data-driven overlay panel can flip any overlay by id without knowing its field.
	bool GetEnabled(int overlayId)
	{
		switch (overlayId)
		{
			case OV_CONES:    return m_bViewCones;
			case OV_MOVEMENT: return m_bMovement;
			case OV_MARKERS:  return m_bMarkers;
			case OV_FPS:      return m_bPlayerFps;
		}
		return false;
	}

	void SetEnabled(int overlayId, bool on)
	{
		switch (overlayId)
		{
			case OV_CONES:    m_bViewCones = on; break;
			case OV_MOVEMENT: m_bMovement = on;  break;
			case OV_MARKERS:  m_bMarkers = on;   break;
			case OV_FPS:
			{
				m_bPlayerFps = on;
				DCO_FpsMonitorClient.Get().SetActive(on);	// subscribe/unsubscribe the telemetry channel with the toggle.
				break;
			}
		}
	}

	int GetScope(int overlayId)
	{
		switch (overlayId)
		{
			case OV_CONES:    return m_ScopeViewCones;
			case OV_MOVEMENT: return m_ScopeMovement;
			case OV_MARKERS:  return m_ScopeMarkers;
		}
		return EDCO_OverlayScope.SELECTED;
	}

	void SetScope(int overlayId, int scope)
	{
		switch (overlayId)
		{
			case OV_CONES:    m_ScopeViewCones = scope; break;
			case OV_MOVEMENT: m_ScopeMovement = scope;  break;
			case OV_MARKERS:  m_ScopeMarkers = scope;   break;
		}
	}

	static int ScopeCount()
	{
		return 2;	// SELECTED / ALL.
	}

	static string ScopeName(int scope)
	{
		switch (scope)
		{
			case EDCO_OverlayScope.SELECTED: return "Selected";
			case EDCO_OverlayScope.ALL:     return "All";
		}
		return "Selected";
	}
}
