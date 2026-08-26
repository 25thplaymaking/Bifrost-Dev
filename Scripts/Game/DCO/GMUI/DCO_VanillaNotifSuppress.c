modded class SCR_NotificationsLogComponent
{
	protected static bool s_bDCO_Suppress;
	protected static ref array<SCR_NotificationsLogComponent> s_aDCO_Live = {};

	static void DCO_SetSuppressed(bool suppress)
	{
		s_bDCO_Suppress = suppress;
		foreach (SCR_NotificationsLogComponent component : s_aDCO_Live)
		{
			if (!component)
				continue;
			if (suppress)
				component.DCO_ApplySuppression();
			else
				component.OnSettingsChanged();
		}
	}

	protected void DCO_ApplySuppression()
	{
		if (s_bDCO_Suppress && m_wRoot)
			m_wRoot.SetVisible(false);
	}

	override bool OnNotification(SCR_NotificationData data)
	{
		if (s_bDCO_Suppress)
		{
			DCO_ApplySuppression();
			return false;
		}
		return super.OnNotification(data);
	}

	override void OnSettingsChanged()
	{
		super.OnSettingsChanged();
		DCO_ApplySuppression();
	}

	override void HandlerAttachedScripted(Widget w)
	{
		super.HandlerAttachedScripted(w);
		if (s_aDCO_Live.Find(this) == -1)
			s_aDCO_Live.Insert(this);
		DCO_ApplySuppression();
	}

	override void HandlerDeattached(Widget w)
	{
		s_aDCO_Live.RemoveItem(this);
		super.HandlerDeattached(w);
	}
}

// The SECOND engine notification UI.
modded class SCR_NotificationsLogDisplay
{
	protected static bool s_bDCO_Suppress;
	protected static ref array<SCR_NotificationsLogDisplay> s_aDCO_Live = {};

	static void DCO_SetSuppressed(bool suppress)
	{
		if (s_bDCO_Suppress == suppress)
			return;
		s_bDCO_Suppress = suppress;
		foreach (SCR_NotificationsLogDisplay d : s_aDCO_Live)
		{
			if (!d)
				continue;
			if (suppress)
				d.Show(false);
			else
				d.Show(true);
		}
	}

	override void DisplayStartDraw(IEntity owner)
	{
		super.DisplayStartDraw(owner);
		if (s_aDCO_Live.Find(this) == -1)
			s_aDCO_Live.Insert(this);
		if (s_bDCO_Suppress)
			Show(false);
	}

	override void DisplayStopDraw(IEntity owner)
	{
		s_aDCO_Live.RemoveItem(this);
		super.DisplayStopDraw(owner);
	}

	override void Show(bool show, float speed = UIConstants.FADE_RATE_INSTANT, EAnimationCurve curve = EAnimationCurve.LINEAR)
	{
		if (show && s_bDCO_Suppress)
			return;
		super.Show(show, speed, curve);
	}
}

modded class SCR_AvailableActionsDisplay
{
	override event void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);
		if (DCO_GMUIController.IsActive() && m_wRoot)
			m_wRoot.SetVisible(false);
	}
}
