//! Horizontally pages a clipped frame by moving its content slot. The start index is the source
//! of truth, so arrows, wheel input, and focused tiles all resolve to the same deterministic page.
class GRSA_CarouselComponent : ScriptedWidgetComponent
{
	[Attribute(defvalue: "CandidatesContentSize", desc: "Direct child moved inside the clipped viewport")]
	protected string m_sContentName;

	[Attribute(defvalue: "10", desc: "Ease rate toward the selected page")]
	protected float m_fSmoothRate;

	[Attribute(desc: "Optional previous control")]
	protected string m_sStartIndicatorName;

	[Attribute(desc: "Optional next control")]
	protected string m_sEndIndicatorName;

	protected FrameWidget m_wViewport;
	protected Widget m_wContent;
	protected Widget m_wStartIndicator;
	protected Widget m_wEndIndicator;
	protected int m_iItemCount;
	protected int m_iStartIndex;
	protected int m_iVisibleCount = 1;
	protected int m_iMaxStart;
	protected float m_fItemSpan = 1;
	protected float m_fViewWidth;
	protected float m_fViewHeight;
	protected float m_fContentWidth;
	protected float m_fCurrentX;
	protected float m_fTargetX;
	protected bool m_bConfigured;
	protected bool m_bGeometryReady;

	// Built on first use to avoid adding eager module initialization work.
	protected static ref array<GRSA_CarouselComponent> s_aInstances;

	//------------------------------------------------------------------------------------------------
	protected static void EnsureInstances()
	{
		if (!s_aInstances)
			s_aInstances = new array<GRSA_CarouselComponent>();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wViewport = FrameWidget.Cast(w);
		if (!m_wViewport)
			return;

		m_wContent = m_wViewport.FindAnyWidget(m_sContentName);
		EnsureInstances();
		s_aInstances.Insert(this);
		ResolveIndicators();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		EnsureInstances();
		s_aInstances.RemoveItem(this);
		m_wViewport = null;
		m_wContent = null;
		m_wStartIndicator = null;
		m_wEndIndicator = null;
		super.HandlerDeattached(w);
	}

	//------------------------------------------------------------------------------------------------
	void Configure(int itemCount, float itemSpan, int startIndex = 0)
	{
		if (!m_wViewport || !m_wContent)
			return;

		m_iItemCount = Math.Max(0, itemCount);
		m_fItemSpan = Math.Max(1, itemSpan);
		m_fContentWidth = Math.Max(1, m_iItemCount * m_fItemSpan);
		m_iVisibleCount = 1;
		m_iMaxStart = Math.Max(0, m_iItemCount - 1);
		m_iStartIndex = Math.ClampInt(startIndex, 0, Math.Max(0, m_iItemCount - 1));
		m_bConfigured = true;
		m_bGeometryReady = false;

		if (!RefreshGeometry(true))
		{
			FrameSlot.SetPosX(m_wContent, 0);
			UpdateCarouselControl(m_wStartIndicator, false, false, "START");
			UpdateCarouselControl(m_wEndIndicator, false, false, "END");
		}
	}

	//------------------------------------------------------------------------------------------------
	void Clear()
	{
		m_iItemCount = 0;
		m_iStartIndex = 0;
		m_iVisibleCount = 1;
		m_iMaxStart = 0;
		m_fContentWidth = 1;
		m_fCurrentX = 0;
		m_fTargetX = 0;
		m_fViewWidth = 0;
		m_fViewHeight = 0;
		m_bConfigured = false;
		m_bGeometryReady = false;
		if (m_wContent)
			FrameSlot.SetPosX(m_wContent, 0);
		UpdateCarouselControl(m_wStartIndicator, false, false, "START");
		UpdateCarouselControl(m_wEndIndicator, false, false, "END");
	}

	//------------------------------------------------------------------------------------------------
	int GetStartIndex()
	{
		return m_iStartIndex;
	}

	//------------------------------------------------------------------------------------------------
	void GlidePage(int direction)
	{
		if (direction == 0 || m_iMaxStart <= 0)
			return;

		int pageStep = Math.Max(1, m_iVisibleCount - 1);
		SetStart(Math.ClampInt(m_iStartIndex + direction * pageStep, 0, m_iMaxStart));
	}

	//------------------------------------------------------------------------------------------------
	void GlideToIndex(int index)
	{
		if (index < 0 || index >= m_iItemCount || m_iMaxStart <= 0)
			return;

		int target = m_iStartIndex;
		if (index < m_iStartIndex)
			target = index;
		else if (index >= m_iStartIndex + m_iVisibleCount)
			target = index - m_iVisibleCount + 1;
		SetStart(Math.ClampInt(target, 0, m_iMaxStart));
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!m_wViewport || wheel == 0)
			return false;

		SetStart(Math.ClampInt(m_iStartIndex - wheel, 0, m_iMaxStart));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsCursorOverAny(int x, int y)
	{
		EnsureInstances();
		foreach (GRSA_CarouselComponent instance : s_aInstances)
		{
			if (!instance || !instance.m_wViewport || !instance.m_wViewport.IsVisibleInHierarchy())
				continue;

			float sizeX, sizeY, posX, posY;
			instance.m_wViewport.GetScreenSize(sizeX, sizeY);
			instance.m_wViewport.GetScreenPos(posX, posY);
			if (x >= posX && x <= posX + sizeX && y >= posY && y <= posY + sizeY)
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void TickAll(float tDelta)
	{
		EnsureInstances();
		foreach (GRSA_CarouselComponent instance : s_aInstances)
		{
			if (instance)
				instance.Tick(tDelta);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick(float tDelta)
	{
		if (!m_wContent || !m_bConfigured)
			return;

		bool snap = !m_bGeometryReady;
		if (!RefreshGeometry(snap))
			return;

		float remaining = m_fTargetX - m_fCurrentX;
		if (remaining > -0.1 && remaining < 0.1)
		{
			if (m_fCurrentX != m_fTargetX)
			{
				m_fCurrentX = m_fTargetX;
				FrameSlot.SetPosX(m_wContent, m_fCurrentX);
			}
			return;
		}

		float alpha = Math.Min(1, tDelta * m_fSmoothRate);
		m_fCurrentX = m_fCurrentX + remaining * alpha;
		FrameSlot.SetPosX(m_wContent, m_fCurrentX);
	}

	//------------------------------------------------------------------------------------------------
	//! Dynamic rows are built while their panel is becoming visible. Wait until the viewport has
	//! real screen geometry, then keep it current across resolution and safe-zone changes.
	protected bool RefreshGeometry(bool snap)
	{
		if (!m_wViewport || !m_wContent || !m_bConfigured)
			return false;

		m_wViewport.Update();
		m_wContent.Update();

		float viewWidth;
		float viewHeight;
		m_wViewport.GetScreenSize(viewWidth, viewHeight);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
		{
			viewWidth = workspace.DPIUnscale(viewWidth);
			viewHeight = workspace.DPIUnscale(viewHeight);
		}

		if (viewWidth <= 2 || viewHeight <= 2)
			return false;

		bool geometryChanged = !m_bGeometryReady;
		if (Math.AbsFloat(viewWidth - m_fViewWidth) > 0.5 || Math.AbsFloat(viewHeight - m_fViewHeight) > 0.5)
			geometryChanged = true;
		if (!geometryChanged)
			return true;

		m_fViewWidth = viewWidth;
		m_fViewHeight = viewHeight;
		m_iVisibleCount = Math.ClampInt(Math.Floor(m_fViewWidth / m_fItemSpan), 1, Math.Max(1, m_iItemCount));
		m_iMaxStart = Math.Max(0, m_iItemCount - m_iVisibleCount);
		m_iStartIndex = Math.ClampInt(m_iStartIndex, 0, m_iMaxStart);

		FrameSlot.SetAnchor(m_wContent, 0, 0);
		FrameSlot.SetAlignment(m_wContent, 0, 0);
		FrameSlot.SetSize(m_wContent, m_fContentWidth, m_fViewHeight);
		FrameSlot.SetPosY(m_wContent, 0);
		m_fTargetX = OffsetForStart(m_iStartIndex);
		if (snap)
			m_fCurrentX = m_fTargetX;
		FrameSlot.SetPosX(m_wContent, m_fCurrentX);
		m_wContent.Update();

		m_bGeometryReady = true;
		UpdateControls();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStart(int startIndex)
	{
		startIndex = Math.ClampInt(startIndex, 0, m_iMaxStart);
		if (startIndex == m_iStartIndex && m_fTargetX == OffsetForStart(startIndex))
			return;

		m_iStartIndex = startIndex;
		m_fTargetX = OffsetForStart(startIndex);
		UpdateControls();
	}

	//------------------------------------------------------------------------------------------------
	protected float OffsetForStart(int startIndex)
	{
		float maxOffset = Math.Max(0, m_fContentWidth - m_fViewWidth);
		return -Math.Min(startIndex * m_fItemSpan, maxOffset);
	}

	//------------------------------------------------------------------------------------------------
	protected void ResolveIndicators()
	{
		if (!m_wViewport || !m_wViewport.GetParent())
			return;

		Widget scope = m_wViewport.GetParent();
		if (!m_sStartIndicatorName.IsEmpty())
			m_wStartIndicator = scope.FindAnyWidget(m_sStartIndicatorName);
		if (!m_sEndIndicatorName.IsEmpty())
			m_wEndIndicator = scope.FindAnyWidget(m_sEndIndicatorName);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateControls()
	{
		if (!m_wStartIndicator || !m_wEndIndicator)
			ResolveIndicators();

		bool overflow = m_iMaxStart > 0;
		bool atStart = m_iStartIndex <= 0;
		bool atEnd = m_iStartIndex >= m_iMaxStart;
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
}
