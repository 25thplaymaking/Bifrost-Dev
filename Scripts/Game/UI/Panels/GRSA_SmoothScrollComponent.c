//! Wheel scrolling with glide for a ScrollLayoutWidget: each wheel tick adds travel distance and
//! the position eases toward it every frame, so long lists fly and settle instead of stepping.
class GRSA_SmoothScrollComponent : ScriptedWidgetComponent
{
	[Attribute(defvalue: "116", desc: "Pixels of travel added per wheel tick")]
	protected float m_fWheelStep;

	[Attribute(defvalue: "8", desc: "Ease rate toward the target, higher settles faster")]
	protected float m_fSmoothRate;

	[Attribute(defvalue: "0", desc: "Apply wheel travel to the horizontal axis instead of the vertical one")]
	protected bool m_bHorizontal;

	[Attribute(desc: "Optional previous control; disabled and labelled START at the first item")]
	protected string m_sStartIndicatorName;

	[Attribute(desc: "Optional next control; disabled and labelled END at the last item")]
	protected string m_sEndIndicatorName;

	protected ScrollLayoutWidget m_wScroll;
	protected Widget m_wStartIndicator;
	protected Widget m_wEndIndicator;
	protected float m_fRemaining;
	protected float m_fCarouselContentWidth;
	protected int m_iCarouselItemCount;
	protected float m_fCarouselItemSpan;
	protected float m_fCarouselTargetX;
	protected bool m_bCarouselMoving;

	// Built on first use - an eager static initializer charges the module-init budget shared by every loaded mod.
	protected static ref array<GRSA_SmoothScrollComponent> s_aInstances;

	protected static void EnsureInstances()
	{
		if (!s_aInstances)
			s_aInstances = new array<GRSA_SmoothScrollComponent>();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wScroll = ScrollLayoutWidget.Cast(w);
		if (m_wScroll)
		{
			EnsureInstances();
			s_aInstances.Insert(this);
			ResolveIndicators();
		}
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		EnsureInstances();
		s_aInstances.RemoveItem(this);
		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(UpdateEdgeIndicators);
		m_wScroll = null;
		m_wStartIndicator = null;
		m_wEndIndicator = null;
		super.HandlerDeattached(w);
	}

	//------------------------------------------------------------------------------------------------
	//! Gives a horizontal strip enough geometry to report its endpoint state.
	void ConfigureCarousel(int itemCount, float itemSpan)
	{
		if (!m_wStartIndicator || !m_wEndIndicator)
			ResolveIndicators();

		m_iCarouselItemCount = itemCount;
		m_fCarouselItemSpan = Math.Max(1, itemSpan);
		m_fCarouselContentWidth = Math.Max(0, itemCount * itemSpan);
		m_bCarouselMoving = false;
		GetGame().GetCallqueue().Remove(UpdateEdgeIndicators);
		GetGame().GetCallqueue().CallLater(UpdateEdgeIndicators, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	void ClearCarousel()
	{
		m_iCarouselItemCount = 0;
		m_fCarouselItemSpan = 0;
		m_fCarouselContentWidth = 0;
		m_fCarouselTargetX = 0;
		m_bCarouselMoving = false;
		m_fRemaining = 0;
		SetIndicatorVisible(m_wStartIndicator, false);
		SetIndicatorVisible(m_wEndIndicator, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Moves a horizontal strip by most of one visible page, leaving one tile as visual context.
	void GlidePage(int direction)
	{
		if (!m_wScroll || !m_bHorizontal || direction == 0)
			return;

		int visibleCount = GetVisibleItemCount();
		int maxStart = Math.Max(0, m_iCarouselItemCount - visibleCount);
		if (maxStart == 0)
			return;

		int pageStep = Math.Max(1, visibleCount - 1);
		int currentStart = GetCarouselStart(maxStart);
		SetCarouselStart(Math.ClampInt(currentStart + direction * pageStep, 0, maxStart), maxStart);
	}

	//------------------------------------------------------------------------------------------------
	//! Centers a selected tile and clamps the target to the first or last complete page.
	void GlideToIndex(int index)
	{
		if (!m_wScroll || !m_bHorizontal || index < 0 || index >= m_iCarouselItemCount)
			return;

		int visibleCount = GetVisibleItemCount();
		int maxStart = Math.Max(0, m_iCarouselItemCount - visibleCount);
		if (maxStart == 0)
			return;

		int targetStart = Math.Round(index - visibleCount * 0.5 + 0.5);
		SetCarouselStart(Math.ClampInt(targetStart, 0, maxStart), maxStart);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!m_wScroll)
			return false;

		if (m_bHorizontal && m_iCarouselItemCount > 0)
		{
			MoveCarouselByTiles(-wheel);
			return true;
		}

		m_fRemaining -= wheel * m_fWheelStep;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! True when the cursor sits over any live, visible smooth-scroll region — the stage camera
	//! yields its wheel zoom to list scrolling (the stage render target is full-bleed, so a pure
	//! rect test on it covers the side panels too).
	static bool IsCursorOverAny(int x, int y)
	{
		EnsureInstances();
		foreach (GRSA_SmoothScrollComponent instance : s_aInstances)
		{
			if (!instance || !instance.m_wScroll || !instance.m_wScroll.IsVisibleInHierarchy())
				continue;

			float sizeX, sizeY, posX, posY;
			instance.m_wScroll.GetScreenSize(sizeX, sizeY);
			instance.m_wScroll.GetScreenPos(posX, posY);
			if (x >= posX && x <= posX + sizeX && y >= posY && y <= posY + sizeY)
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void TickAll(float tDelta)
	{
		EnsureInstances();
		foreach (GRSA_SmoothScrollComponent instance : s_aInstances)
		{
			if (instance)
				instance.Tick(tDelta);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick(float tDelta)
	{
		if (!m_wScroll)
			return;

		if (m_fCarouselContentWidth > 0)
			UpdateEdgeIndicators();
		if (m_bHorizontal && m_iCarouselItemCount > 0)
		{
			TickCarousel(tDelta);
			return;
		}

		if (m_fRemaining == 0)
			return;

		if (m_fRemaining > -0.5 && m_fRemaining < 0.5)
		{
			m_fRemaining = 0;
			return;
		}

		float alpha = Math.Min(1, tDelta * m_fSmoothRate);
		float step = m_fRemaining * alpha;

		float normalizedX, normalizedY;
		m_wScroll.GetSliderPos(normalizedX, normalizedY);
		if (m_bHorizontal && ((m_fRemaining < 0 && normalizedX <= 0.001) || (m_fRemaining > 0 && normalizedX >= 0.999)))
		{
			m_fRemaining = 0;
			UpdateEdgeIndicators();
			return;
		}

		float x, y;
		m_wScroll.GetSliderPosPixels(x, y);
		if (m_bHorizontal)
			m_wScroll.SetSliderPosPixels(x + step, y);
		else
			m_wScroll.SetSliderPosPixels(x, y + step);
		m_fRemaining -= step;
	}

	//------------------------------------------------------------------------------------------------
	protected void MoveCarouselByTiles(int tileDelta)
	{
		if (tileDelta == 0)
			return;

		int visibleCount = GetVisibleItemCount();
		int maxStart = Math.Max(0, m_iCarouselItemCount - visibleCount);
		if (maxStart == 0)
			return;

		int currentStart = GetCarouselStart(maxStart);
		SetCarouselStart(Math.ClampInt(currentStart + tileDelta, 0, maxStart), maxStart);
	}

	//------------------------------------------------------------------------------------------------
	protected int GetVisibleItemCount()
	{
		if (!m_wScroll || m_fCarouselItemSpan <= 0)
			return 1;

		float viewW, viewH;
		m_wScroll.GetScreenSize(viewW, viewH);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			viewW = workspace.DPIUnscale(viewW);

		int visibleCount = Math.Floor(viewW / m_fCarouselItemSpan);
		return Math.ClampInt(visibleCount, 1, Math.Max(1, m_iCarouselItemCount));
	}

	//------------------------------------------------------------------------------------------------
	protected int GetCarouselStart(int maxStart)
	{
		float x, y;
		m_wScroll.GetSliderPos(x, y);
		if (m_bCarouselMoving)
			x = m_fCarouselTargetX;
		return Math.ClampInt(Math.Round(x * maxStart), 0, maxStart);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetCarouselStart(int startIndex, int maxStart)
	{
		if (maxStart <= 0)
			return;

		m_fCarouselTargetX = Math.Clamp(startIndex * 1.0 / maxStart, 0, 1);
		m_bCarouselMoving = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void TickCarousel(float tDelta)
	{
		if (!m_bCarouselMoving)
			return;

		float x, y;
		m_wScroll.GetSliderPos(x, y);
		float remaining = m_fCarouselTargetX - x;
		if (remaining > -0.001 && remaining < 0.001)
		{
			m_wScroll.SetSliderPos(m_fCarouselTargetX, y);
			m_bCarouselMoving = false;
			UpdateEdgeIndicators();
			return;
		}

		float alpha = Math.Min(1, tDelta * m_fSmoothRate);
		m_wScroll.SetSliderPos(x + remaining * alpha, y);
	}

	//------------------------------------------------------------------------------------------------
	protected void ResolveIndicators()
	{
		if (!m_wScroll || !m_wScroll.GetParent())
			return;

		Widget scope = m_wScroll.GetParent();
		if (!m_sStartIndicatorName.IsEmpty())
			m_wStartIndicator = scope.FindAnyWidget(m_sStartIndicatorName);
		if (!m_sEndIndicatorName.IsEmpty())
			m_wEndIndicator = scope.FindAnyWidget(m_sEndIndicatorName);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateEdgeIndicators()
	{
		if (!m_wScroll || m_fCarouselContentWidth <= 0)
			return;

		float viewW, viewH;
		m_wScroll.GetScreenSize(viewW, viewH);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			viewW = workspace.DPIUnscale(viewW);

		bool overflow = viewW > 0 && m_fCarouselContentWidth > viewW + 1;
		float x, y;
		m_wScroll.GetSliderPos(x, y);
		bool atStart = x <= 0.005;
		bool atEnd = x >= 0.995;
		string startLabel = "PREV";
		if (atStart)
			startLabel = "START";
		string endLabel = "NEXT";
		if (atEnd)
			endLabel = "END";
		UpdateCarouselControl(m_wStartIndicator, overflow, !atStart, startLabel);
		UpdateCarouselControl(m_wEndIndicator, overflow, !atEnd, endLabel);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateCarouselControl(Widget control, bool visible, bool enabled, string label)
	{
		if (!control)
			return;

		control.SetVisible(visible);
		control.SetEnabled(enabled);
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.Cast(control.FindHandler(SCR_ButtonTextComponent));
		if (button)
			button.SetText(label);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetIndicatorVisible(Widget indicator, bool visible)
	{
		if (indicator && indicator.IsVisible() != visible)
			indicator.SetVisible(visible);
	}
}
