// Protects the task tab from layout drift by preserving the hierarchy expected by its generated binding.
modded class SCR_TaskListTabEntryUIComponent
{
	override void ChangeTabColor(Color color)
	{
		if (m_Widgets && !m_Widgets.m_wBackground && m_wRoot)
			m_Widgets.m_wBackground = SmartPanelWidget.Cast(m_wRoot.FindAnyWidget("m_wBackground"));

		if (!m_Widgets || !m_Widgets.m_wBackground)
		{
			Print("[BIFROST] Task-list tab background missing; color update skipped", LogLevel.WARNING);
			return;
		}

		super.ChangeTabColor(color);
	}
}
