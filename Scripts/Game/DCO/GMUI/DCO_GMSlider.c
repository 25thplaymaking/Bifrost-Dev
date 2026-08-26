// Reusable self-contained draggable slider for the DCO GM UI.
class DCO_GMSliderHandle : ScriptedWidgetEventHandler
{
	protected DCO_GMSlider m_Owner;

	void DCO_GMSliderHandle(DCO_GMSlider owner)
	{
		m_Owner = owner;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 0 || !m_Owner)
			return false;
		m_Owner.BeginDrag();
		return true;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (!m_Owner || !m_Owner.IsDragging())
			return false;
		m_Owner.EndDrag();
		return true;
	}
}

class DCO_GMSlider
{
	protected Widget m_Track;	// full-width drag target; its screen rect maps cursor -> fraction.
	protected ImageWidget m_Fill;	// amber fill, width = fraction * track width.
	protected TextWidget m_Value;
	protected Widget m_Thumb;
	protected ref array<ImageWidget> m_Ticks = {};
	protected ref array<TextWidget> m_TickLabels = {};
	protected ref DCO_GMSliderHandle m_Handle;
	protected float m_Min, m_Max, m_Cur;
	protected float m_Step;
	protected string m_Suffix;
	protected bool m_Dragging;
	protected bool m_TimeScale;
	protected int m_ScaleDivisions;
	protected ref ScriptInvoker m_OnChange = new ScriptInvoker();	// (float value) - live during drag + on release.
	protected const float FILL_H = 5;
	protected const float THUMB_SIZE = 14;
	protected const float LABEL_WIDTH = 34;
	protected const float SCALE_CENTER_Y = 20;

	void Init(Widget root, string track, string fill, string value, float min, float max, float cur, string suffix)
	{
		if (!root)
			return;
		m_Track = root.FindAnyWidget(track);
		m_Fill  = ImageWidget.Cast(root.FindAnyWidget(fill));
		m_Value = TextWidget.Cast(root.FindAnyWidget(value));
		m_Min = min;
		m_Max = max;
		m_Cur = cur;
		m_Suffix = suffix;
		if (!m_Track || !m_Fill)
		{
			Print(string.Format("[DCO-GM] slider missing widgets (track=%1 fill=%2 / %3)", m_Track != null, m_Fill != null, track), LogLevel.WARNING);
			return;
		}
		ButtonWidget tb = ButtonWidget.Cast(m_Track);
		m_Handle = new DCO_GMSliderHandle(this);
		if (tb)
			tb.AddHandler(m_Handle);
	}

	// Optional visual scale used by scenario-setting value bars.
	void ConfigureScale(Widget root, string thumbName, string tickPrefix, string labelPrefix, bool timeScale, float step = 0)
	{
		if (!root)
			return;

		m_Thumb = root.FindAnyWidget(thumbName);
		m_Ticks.Clear();
		m_TickLabels.Clear();
		for (int i = 0; i < 7; i++)
		{
			ImageWidget tick = ImageWidget.Cast(root.FindAnyWidget(tickPrefix + i.ToString()));
			TextWidget label = TextWidget.Cast(root.FindAnyWidget(labelPrefix + i.ToString()));
			if (tick)
				m_Ticks.Insert(tick);
			if (label)
				m_TickLabels.Insert(label);
		}

		m_TimeScale = timeScale;
		m_Step = step;
		if (timeScale)
			m_ScaleDivisions = 6;
		else
			m_ScaleDivisions = 4;
		Refresh();
	}

	ScriptInvoker GetOnChange() { return m_OnChange; }
	bool IsDragging() { return m_Dragging; }
	float GetValue() { return m_Cur; }

	void SetValue(float v)
	{
		m_Cur = Math.Clamp(v, m_Min, m_Max);
		Refresh();
	}

	// Repaint the fill width + value label from the current value.
	void Refresh()
	{
		float frac = 0;
		if (m_Max > m_Min)
			frac = (m_Cur - m_Min) / (m_Max - m_Min);
		if (m_Fill && m_Track)
		{
			float tw = TrackWidthRef();
			if (tw > 1)
				m_Fill.SetSize(frac * tw, FILL_H);
			RefreshScale(frac, tw);
		}
		if (m_Value)
		{
			int iv = Math.Round(m_Cur);
			m_Value.SetText(iv.ToString() + m_Suffix);
		}
	}

	protected void RefreshScale(float valueFraction, float trackWidth)
	{
		if (!m_Thumb || m_ScaleDivisions <= 0 || trackWidth <= 1)
			return;

		Color accent = DCO_GMTheme.Get().m_AccentColor;
		float scaleStart = THUMB_SIZE * 0.5;
		float scaleWidth = trackWidth - THUMB_SIZE;
		m_Thumb.SetColor(accent);
		FrameSlot.SetSize(m_Thumb, THUMB_SIZE, THUMB_SIZE);
		FrameSlot.SetPos(m_Thumb, valueFraction * scaleWidth, SCALE_CENTER_Y - THUMB_SIZE * 0.5);
		m_Thumb.SetVisible(true);

		int visibleCount = m_ScaleDivisions + 1;
		for (int i = 0; i < m_Ticks.Count(); i++)
		{
			bool visible = i < visibleCount;
			m_Ticks[i].SetVisible(visible);
			if (!visible)
				continue;

			float fraction = GetScaleFraction(i);
			float tickHeight = 7;
			if (i == 0 || i == m_ScaleDivisions || i == m_ScaleDivisions / 2)
				tickHeight = 11;
			FrameSlot.SetSize(m_Ticks[i], 1, tickHeight);
			FrameSlot.SetPos(m_Ticks[i], scaleStart + fraction * scaleWidth, SCALE_CENTER_Y - tickHeight * 0.5);
		}

		for (int labelIndex = 0; labelIndex < m_TickLabels.Count(); labelIndex++)
		{
			bool labelVisible = labelIndex < visibleCount;
			m_TickLabels[labelIndex].SetVisible(labelVisible);
			if (!labelVisible)
				continue;

			float labelFraction = GetScaleFraction(labelIndex);
			FrameSlot.SetSize(m_TickLabels[labelIndex], LABEL_WIDTH, 12);
			FrameSlot.SetPos(m_TickLabels[labelIndex], labelFraction * (trackWidth - LABEL_WIDTH), 25);
			m_TickLabels[labelIndex].SetText(FormatScaleLabel(labelFraction));
		}
	}

	protected float GetScaleFraction(int index)
	{
		float fraction = index / (float)m_ScaleDivisions;
		if (m_TimeScale || index == 0 || index == m_ScaleDivisions || m_Max <= m_Min)
			return fraction;

		float divisionValue = m_Max / m_ScaleDivisions;
		if (m_Min < 0 || m_Min >= divisionValue)
			return fraction;

		float value = divisionValue * index;
		if (m_Step > 0)
			value = Math.Round(value / m_Step) * m_Step;
		return Math.Clamp((value - m_Min) / (m_Max - m_Min), 0, 1);
	}

	protected string FormatScaleLabel(float fraction)
	{
		if (m_TimeScale)
		{
			int hour = Math.Round(24 * fraction);
			if (hour < 10)
				return "0" + hour.ToString();
			return hour.ToString();
		}

		float value = m_Min + fraction * (m_Max - m_Min);
		if (m_Step > 0)
			value = Math.Round(value / m_Step) * m_Step;
		float rounded = Math.Round(value * 100) / 100.0;
		return rounded.ToString();
	}

	protected float TrackWidthRef()
	{
		float w, h;
		m_Track.GetScreenSize(w, h);
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (ws)
			return ws.DPIUnscale(w);
		return w;
	}

	void BeginDrag()
	{
		m_Dragging = true;
		GetGame().GetCallqueue().Remove(Tick);
		GetGame().GetCallqueue().CallLater(Tick, 0, true);
	}

	void EndDrag()
	{
		m_Dragging = false;
		GetGame().GetCallqueue().Remove(Tick);
		m_OnChange.Invoke(m_Cur);
	}

	protected void Tick()
	{
		if (!m_Dragging || !m_Track)
		{
			EndDrag();
			return;
		}
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		float tx, ty, tw, th;
		m_Track.GetScreenPos(tx, ty);
		m_Track.GetScreenSize(tw, th);
		float frac = 0;
		if (tw > 0)
			frac = Math.Clamp((mx - tx) / tw, 0, 1);
		m_Cur = m_Min + frac * (m_Max - m_Min);
		Refresh();
		m_OnChange.Invoke(m_Cur);
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(Tick);
		m_Ticks.Clear();
		m_TickLabels.Clear();
		m_Thumb = null;
	}
}
