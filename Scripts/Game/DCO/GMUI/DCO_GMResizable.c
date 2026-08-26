// DCO GM resizable-panel handler.
class DCO_GMResizable : ScriptedWidgetEventHandler
{
	protected Widget m_Target;
	protected bool m_Resizing;
	protected int m_StartMouseX;	// cursor at resize start, native px.
	protected int m_StartMouseY;
	protected float m_StartW;	// panel size at resize start, reference px.
	protected float m_StartH;
	protected float m_StartPosX;	// panel FrameSlot pos at resize start, reference px.
	protected float m_StartPosY;
	protected float m_AlignX;
	protected float m_AlignY;
	protected float m_MinW;
	protected float m_MinH;

	void DCO_GMResizable(Widget target, float minW = 150, float minH = 90)
	{
		m_Target = target;
		m_MinW = minW;
		m_MinH = minH;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 0 || !m_Target)	// left button only.
			return false;
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		m_StartMouseX = mx;
		m_StartMouseY = my;
		vector sz = FrameSlot.GetSize(m_Target);
		m_StartW = sz[0];
		m_StartH = sz[1];
		vector p = FrameSlot.GetPos(m_Target);
		m_StartPosX = p[0];
		m_StartPosY = p[1];
		vector al = FrameSlot.GetAlignment(m_Target);
		m_AlignX = al[0];
		m_AlignY = al[1];
		DCO_GMDraggable.Raise(m_Target);	// grabbing the resize grip also brings the panel to the front.
		m_Resizing = true;
		GetGame().GetCallqueue().Remove(OnResizeTick);
		GetGame().GetCallqueue().CallLater(OnResizeTick, 0, true);
		return true;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (!m_Resizing)
			return false;
		StopResize();
		return true;
	}

	protected void OnResizeTick()
	{
		if (!m_Resizing || !m_Target)
		{
			StopResize();
			return;
		}
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		int rawDx = mx - m_StartMouseX;
		int rawDy = my - m_StartMouseY;
		float dx = rawDx;
		float dy = rawDy;
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (ws)	// cursor is native px; FrameSlot size/pos are reference px -> unscale the delta.
		{
			dx = ws.DPIUnscale(rawDx);
			dy = ws.DPIUnscale(rawDy);
		}
		float newW = m_StartW + dx;
		float newH = m_StartH + dy;
		if (newW < m_MinW)
			newW = m_MinW;
		if (newH < m_MinH)
			newH = m_MinH;
		FrameSlot.SetSize(m_Target, newW, newH);
		// hold the top-left corner fixed: shift the alignment pivot by alignment * sizeDelta.
		FrameSlot.SetPos(m_Target, m_StartPosX + m_AlignX * (newW - m_StartW), m_StartPosY + m_AlignY * (newH - m_StartH));
	}

	void StopResize()
	{
		bool wasResizing = m_Resizing;
		m_Resizing = false;
		GetGame().GetCallqueue().Remove(OnResizeTick);
		if (wasResizing && m_Target)
		{
			vector p = FrameSlot.GetPos(m_Target);
			vector s = FrameSlot.GetSize(m_Target);
			DCO_GMTheme.Get().SetPanelGeom(m_Target.GetName(), p[0], p[1], s[0], s[1]);
		}
	}
}
