// Reversible engine-UI hide registry.
class DCO_VanillaUIVisibility
{
	protected ref array<Widget> m_Hidden = {};
	protected ref array<bool> m_WasVisible = {};

	void Hide(Widget searchRoot, string widgetName)
	{
		if (!searchRoot)
			return;
		Widget w = searchRoot.FindAnyWidget(widgetName);
		if (!w)
			return;
		if (m_Hidden.Find(w) == -1)
		{
			m_Hidden.Insert(w);
			m_WasVisible.Insert(w.IsVisible());
		}
		w.SetVisible(false);
	}

	void RestoreAll()
	{
		for (int i = 0; i < m_Hidden.Count(); i++)
		{
			Widget w = m_Hidden[i];
			if (w)
				w.SetVisible(m_WasVisible[i]);
		}
		m_Hidden.Clear();
		m_WasVisible.Clear();
	}
}
