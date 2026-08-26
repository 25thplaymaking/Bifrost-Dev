// DCO GM draggable-panel handler.
class DCO_GMDraggable : ScriptedWidgetEventHandler
{
	// Raise-on-grab.
	protected static int s_TopZ = 0;
	static void Raise(Widget panel)
	{
		if (!panel)
			return;
		s_TopZ++;
		panel.SetZOrder(s_TopZ);
	}

	static void ResetRaise()
	{
		s_TopZ = 0;
	}

	protected Widget m_Target;	// the panel this grip moves.
	protected bool m_Dragging;
	protected int m_StartMouseX;	// cursor at drag start, native px.
	protected int m_StartMouseY;
	protected float m_StartPosX;	// panel FrameSlot pos at drag start, reference px.
	protected float m_StartPosY;

	void DCO_GMDraggable(Widget target)
	{
		m_Target = target;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (!m_Target)
			return false;
		// RIGHT-click on the grip = quick-hide.
		if (button == 1)
		{
			int idx = DCO_GMTheme.ElementIndexFor(m_Target.GetName());
			if (idx >= 0)
			{
				Widget root = m_Target;
				while (root.GetParent())
					root = root.GetParent();
				DCO_GMTheme.Get().SetElementEnabled(idx, false, root, false);
				Print("[DCO-GM] quick-hide: " + m_Target.GetName() + " (re-show via OPTIONS or RESET UI)", LogLevel.NORMAL);
			}
			return true;
		}
		if (button != 0)	// left drags, right quick-hides; nothing else is ours.
			return false;
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		m_StartMouseX = mx;
		m_StartMouseY = my;
		vector p = FrameSlot.GetPos(m_Target);
		m_StartPosX = p[0];
		m_StartPosY = p[1];
		Raise(m_Target);
		m_Dragging = true;
		GetGame().GetCallqueue().Remove(OnDragTick);
		GetGame().GetCallqueue().CallLater(OnDragTick, 0, true);
		return true;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (!m_Dragging)
			return false;
		StopDrag();
		return true;
	}

	protected void OnDragTick()
	{
		if (!m_Dragging || !m_Target)
		{
			StopDrag();
			return;
		}
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		int rawDx = mx - m_StartMouseX;
		int rawDy = my - m_StartMouseY;
		float dx = rawDx;
		float dy = rawDy;
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (ws)	// cursor is native px; FrameSlot pos is reference px -> unscale the delta.
		{
			dx = ws.DPIUnscale(rawDx);
			dy = ws.DPIUnscale(rawDy);
		}
		FrameSlot.SetPos(m_Target, m_StartPosX + dx, m_StartPosY + dy);
	}

	void StopDrag()
	{
		bool wasDragging = m_Dragging;
		m_Dragging = false;
		GetGame().GetCallqueue().Remove(OnDragTick);
		if (wasDragging && m_Target)
		{
			DCO_GMTheme.ClampPanelToViewport(m_Target);
			vector p = FrameSlot.GetPos(m_Target);
			vector s = FrameSlot.GetSize(m_Target);
			DCO_GMTheme.Get().SetPanelGeom(m_Target.GetName(), p[0], p[1], s[0], s[1]);
		}
	}
}
