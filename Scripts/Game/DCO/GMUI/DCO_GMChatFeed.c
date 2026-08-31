// Bifrost chat feed - the shell-side replacement for the engine chat panel's message log.
class DCO_GMChatFeed
{
	static const int ROWS = 8;
	static const int POLL_MS = 200;	// message-count poll + engine open-state mirror.
	static const int BIND_RETRIES = 50;	// engine panel search attempts before settling for feed-only mode.

	protected Widget m_wRoot;
	protected ref array<TextWidget> m_Rows = {};
	protected int m_LastCount = -1;

	protected Widget m_wVanillaChat;	// engine chat panel root: hidden while closed, shown while open.
	protected SCR_ChatPanel m_VanillaPanel;
	protected int m_BindTries;

	void Init(Widget root)
	{
		if (!root)
			return;
		m_wRoot = root;
		for (int i = 0; i < ROWS; i++)
			m_Rows.Insert(TextWidget.Cast(root.FindAnyWidget("DCO_Chat_Row" + i.ToString())));

		m_LastCount = -1;
		m_BindTries = 0;
		GetGame().GetCallqueue().CallLater(Poll, POLL_MS, true);
		Poll();
	}

	protected void Poll()
	{
		SCR_ChatPanelManager mgr = SCR_ChatPanelManager.GetInstance();

		if (!m_VanillaPanel && m_BindTries < BIND_RETRIES)
		{
			m_BindTries++;
			Widget ws = GetGame().GetWorkspace();
			if (ws)
			{
				Widget hit = FindWidgetWithHandler(ws.GetChildren(), SCR_ChatPanel);
				if (hit)
				{
					m_wVanillaChat = hit;
					m_VanillaPanel = SCR_ChatPanel.Cast(hit.FindHandler(SCR_ChatPanel));
				}
			}
		}

		// Mirror: the engine panel exists ONLY as the input surface, so it is visible exactly while open.
		if (m_wVanillaChat && m_VanillaPanel)
			m_wVanillaChat.SetVisible(m_VanillaPanel.IsOpen());

		if (!mgr)
			return;
		array<ref SCR_ChatMessage> msgs = mgr.GetMessages();
		if (!msgs)
			return;
		if (msgs.Count() != m_LastCount)
		{
			m_LastCount = msgs.Count();
			Repaint(msgs);
		}
	}

	// Chat convention: newest at the BOTTOM.
	protected void Repaint(array<ref SCR_ChatMessage> msgs)
	{
		int total = msgs.Count();
		for (int r = 0; r < m_Rows.Count(); r++)
		{
			TextWidget row = m_Rows[r];
			if (!row)
				continue;
			int idx = total - ROWS + r;
			if (idx < 0 || idx >= total || !msgs[idx])
			{
				row.SetText(" ");
				continue;
			}
			SCR_ChatMessage msg = msgs[idx];
			string sender = "";
			SCR_ChatMessageGeneral general = SCR_ChatMessageGeneral.Cast(msg);
			if (general && !general.m_sSenderName.IsEmpty())
			{
				sender = general.m_sSenderName;
				if (SCR_ChatMessagePrivate.Cast(msg))
					sender += " (private)";
			}
			if (sender.IsEmpty())
			{
				row.ClearFlags(WidgetFlags.NO_LOCALIZATION);
				row.SetText(msg.m_sMessage);
			}
			else
			{
				row.SetFlags(WidgetFlags.NO_LOCALIZATION);
				row.SetText(sender + ": " + msg.m_sMessage);
			}
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

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(Poll);
		if (m_wVanillaChat)
		{
			m_wVanillaChat.SetVisible(true);	// reversible - engine chat returns when the shell goes away.
			m_wVanillaChat = null;
		}
		m_VanillaPanel = null;
		m_Rows.Clear();
		m_wRoot = null;
	}
}
