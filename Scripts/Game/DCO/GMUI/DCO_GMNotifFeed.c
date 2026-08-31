// Bifrost notification feed - the shell-side replacement for the engine notification log.
class DCO_GMNotifFeed
{
	static const int ROWS = 6;
	static const int DISPLAY_MS = 20000;
	static const int PRUNE_MS = 1000;
	static const int BIND_RETRY_MS = 500;	// the component + the engine log can spawn after our shell does.
	static const int BIND_RETRIES = 40;

	protected Widget m_wRoot;
	protected ref array<TextWidget> m_Rows = {};
	protected TextWidget m_wSaveState;
	protected SCR_NotificationsComponent m_Manager;
	protected Widget m_wVanillaLog;	// the hidden engine log root - restored on Shutdown.
	protected int m_BindTries;
	protected bool m_SaveStateBound;

	protected ref array<ref SCR_NotificationData> m_Entries = {};	// newest first, capped at ROWS.
	protected ref array<float> m_EntryTimes = {};	// world-time ms each entry arrived.

	void Init(Widget root)
	{
		if (!root)
			return;
		m_wRoot = root;
		for (int i = 0; i < ROWS; i++)
			m_Rows.Insert(TextWidget.Cast(root.FindAnyWidget("DCO_Notif_Row" + i.ToString())));
		m_wSaveState = TextWidget.Cast(root.FindAnyWidget("DCO_NotifSaveState"));
		ShowSavingState(false);

		if (GetGame().GetSaveGameManager())
		{
			EventProvider.ConnectEvent(GetGame().GetSaveGameManager().OnBusyStateChanged, ShowSavingState);
			m_SaveStateBound = true;
		}

		m_BindTries = 0;
		GetGame().GetCallqueue().CallLater(BindPoll, BIND_RETRY_MS, true);
		BindPoll();

		GetGame().GetCallqueue().Remove(ReHideVanilla);
		GetGame().GetCallqueue().CallLater(ReHideVanilla, 2000, true);

		SCR_NotificationsLogComponent.DCO_SetSuppressed(true);
		SCR_NotificationsLogDisplay.DCO_SetSuppressed(true);
	}

	// Bounded retry: subscribe to the notifications component and hide the engine log, whichever appears first.
	protected void BindPoll()
	{
		m_BindTries++;

		if (!m_Manager)
		{
			m_Manager = SCR_NotificationsComponent.GetInstance();
			if (m_Manager)
			{
				m_Manager.GetOnNotification().Insert(OnNotification);
				array<SCR_NotificationData> history = {};
				int n = m_Manager.GetHistoryOldToNew(history, ROWS);
				for (int i = 0; i < n; i++)
					OnNotification(history[i]);	// old->new so the newest ends up on top.
			}
		}

		if (!m_wVanillaLog)
		{
			Widget ws = GetGame().GetWorkspace();
			if (ws)
				m_wVanillaLog = FindWidgetWithHandler(ws.GetChildren(), SCR_NotificationsLogComponent);
			if (m_wVanillaLog)
				m_wVanillaLog.SetVisible(false);
		}

		if ((m_Manager && m_wVanillaLog) || m_BindTries >= BIND_RETRIES)
			GetGame().GetCallqueue().Remove(BindPoll);
	}

	// Same signature contract as engine's consumer.
	protected bool OnNotification(SCR_NotificationData data)
	{
		// Re-assert the hide on EVERY event: engine's log raises itself when a notification arrives, which undid the one-time hide at bind.
		ReHideVanilla();

		if (!data)
			return false;
		string txt = data.GetText();
		if (txt.IsEmpty())
			return false;

		float now = 0;
		BaseWorld world = GetGame().GetWorld();
		if (world)
			now = world.GetWorldTime();

		m_Entries.InsertAt(data, 0);
		m_EntryTimes.InsertAt(now, 0);
		while (m_Entries.Count() > ROWS)
		{
			m_Entries.RemoveOrdered(m_Entries.Count() - 1);
			m_EntryTimes.RemoveOrdered(m_EntryTimes.Count() - 1);
		}

		Repaint();
		GetGame().GetCallqueue().Remove(PruneTick);
		GetGame().GetCallqueue().CallLater(PruneTick, PRUNE_MS, true);
		return false;
	}

	[ReceiverAttribute()]
	protected void ShowSavingState(bool state)
	{
		if (!m_wSaveState)
			return;

		m_wSaveState.SetVisible(state);
		if (state)
			m_wSaveState.SetText("SAVING...");
		else
			m_wSaveState.SetText(" ");
	}

	// Age entries out so stale notifications do not sit on screen forever.
	protected void PruneTick()
	{
		if (m_Entries.IsEmpty())
		{
			GetGame().GetCallqueue().Remove(PruneTick);
			return;
		}
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		bool changed = false;
		for (int i = m_Entries.Count() - 1; i >= 0; i--)
		{
			if (now - m_EntryTimes[i] > DISPLAY_MS)
			{
				m_Entries.RemoveOrdered(i);
				m_EntryTimes.RemoveOrdered(i);
				changed = true;
			}
		}
		if (changed)
			Repaint();
		if (m_Entries.IsEmpty())
			GetGame().GetCallqueue().Remove(PruneTick);
	}

	// Newest entry on the top row.
	protected void Repaint()
	{
		for (int i = 0; i < m_Rows.Count(); i++)
		{
			TextWidget row = m_Rows[i];
			if (!row)
				continue;
			if (i >= m_Entries.Count() || !m_Entries[i])
			{
				row.SetText(" ");
				continue;
			}
			SCR_NotificationData data = m_Entries[i];
			string p1, p2, p3, p4, p5, p6;
			data.GetNotificationTextEntries(p1, p2, p3, p4, p5, p6);
			SCR_NotificationDisplayData displayData = data.GetDisplayData();
			if (displayData && displayData.MergeParam1With2())
				row.SetTextFormat(data.GetText(), p1, WidgetManager.Translate(p2, p1), p3, p4, p5, p6);
			else
				row.SetTextFormat(data.GetText(), p1, p2, p3, p4, p5, p6);
			row.SetOpacity(1.0 - 0.08 * i);	// subtle age fade down the list.
		}
	}

	protected Widget FindWidgetWithHandler(Widget w, typename handlerType)
	{
		while (w)
		{
			if (w.FindHandler(handlerType))
				return w;
			Widget hit = FindWidgetWithHandler(w.GetChildren(), handlerType);
			if (hit)
				return hit;
			w = w.GetSibling();
		}
		return null;
	}

	protected void ReHideVanilla()
	{
		if (!m_wVanillaLog)
		{
			Widget ws = GetGame().GetWorkspace();
			if (ws)
				m_wVanillaLog = FindWidgetWithHandler(ws.GetChildren(), SCR_NotificationsLogComponent);
		}
		if (m_wVanillaLog && m_wVanillaLog.IsVisible())
			m_wVanillaLog.SetVisible(false);
	}

	void Shutdown()
	{
		SCR_NotificationsLogComponent.DCO_SetSuppressed(false);
		SCR_NotificationsLogDisplay.DCO_SetSuppressed(false);
		if (m_SaveStateBound && GetGame().GetSaveGameManager())
		{
			EventProvider.DisconnectEvent(GetGame().GetSaveGameManager().OnBusyStateChanged, ShowSavingState);
			m_SaveStateBound = false;
		}
		GetGame().GetCallqueue().Remove(BindPoll);
		GetGame().GetCallqueue().Remove(PruneTick);
		GetGame().GetCallqueue().Remove(ReHideVanilla);
		if (m_Manager)
		{
			m_Manager.GetOnNotification().Remove(OnNotification);
			m_Manager = null;
		}
		if (m_wVanillaLog)
		{
			m_wVanillaLog.SetVisible(true);	// reversible peel - the engine log returns when the shell goes away.
			m_wVanillaLog = null;
		}
		m_Entries.Clear();
		m_EntryTimes.Clear();
		m_Rows.Clear();
		m_wSaveState = null;
		m_wRoot = null;
	}
}
