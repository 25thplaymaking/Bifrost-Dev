// DCO GM overlay panel - a VERTICAL list of rows, each a checkbox to turn one overlay option on/off.

class DCO_OverlayPanelHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMOverlayPanel m_Owner;

	void DCO_OverlayPanelHandler(DCO_GMOverlayPanel owner)
	{
		m_Owner = owner;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnButton(w);
		return false;
	}
}

class DCO_GMOverlayPanel
{
	static const int MAX_ROWS = 6;

	protected Widget m_wRoot;
	protected Widget m_wBar;
	protected DCO_GMContextMenu m_Menu;
	protected ref DCO_OverlayPanelHandler m_Handler;
	protected ref ScriptInvoker m_ScopeCb = new ScriptInvoker();

	protected ref array<int> m_OverlayId = {};
	protected ref array<string> m_OverlayName = {};

	protected ref array<ButtonWidget> m_RowToggle = {};
	protected ref array<TextWidget> m_RowToggleLabel = {};
	protected ref array<ImageWidget> m_RowTick = {};	// tick glyph per row - replaces the old ASCII "[X]/[  ]" box.
	protected ref array<ButtonWidget> m_RowScope = {};
	protected ref array<TextWidget> m_RowScopeLabel = {};

	protected int m_PendingScopeRow = -1;	// the row whose scope dropdown is currently open.

	void Init(Widget shellRoot, DCO_GMContextMenu menu)
	{
		if (!shellRoot)
			return;
		m_wRoot = shellRoot;
		m_Menu = menu;
		m_wBar = shellRoot.FindAnyWidget("DCO_OverlayBar");
		if (!m_wBar)
		{
			Print("[DCO-GM] overlay panel: DCO_OverlayBar not found (layout not reloaded yet?)", LogLevel.WARNING);
			return;
		}
		m_Handler = new DCO_OverlayPanelHandler(this);
		m_ScopeCb.Insert(OnScopePicked);

		AddOverlay(DCO_GMOverlayState.OV_CONES,    "AI Vision");
		AddOverlay(DCO_GMOverlayState.OV_MOVEMENT, "Nav Paths");
		AddOverlay(DCO_GMOverlayState.OV_MARKERS,  "Role IDs");
		AddOverlay(DCO_GMOverlayState.OV_FPS,      "Player FPS");	// all-players FPS readout on the PLAYERS list.

		BindRows();
		Print("[DCO-GM] overlay panel bound (rows + per-overlay scope dropdowns)", LogLevel.NORMAL);
	}

	void Shutdown()
	{
	}

	protected void AddOverlay(int id, string name)
	{
		m_OverlayId.Insert(id);
		m_OverlayName.Insert(name);
	}

// Keeps the overlay compact by binding active descriptors to pooled rows and hiding unused rows.
	protected void BindRows()
	{
		int n = m_OverlayId.Count();
		for (int i = 0; i < MAX_ROWS; i++)
		{
			Widget row = m_wBar.FindAnyWidget(string.Format("DCO_OV_Row%1", i));
			if (i < n)
			{
				ButtonWidget toggle = ButtonWidget.Cast(m_wBar.FindAnyWidget(string.Format("DCO_OV_Row%1_Toggle", i)));
				TextWidget tlbl     = TextWidget.Cast(m_wBar.FindAnyWidget(string.Format("DCO_OV_Row%1_TLabel", i)));
				ImageWidget tick    = ImageWidget.Cast(m_wBar.FindAnyWidget(string.Format("DCO_OV_Row%1_Tick", i)));
				ButtonWidget scope  = ButtonWidget.Cast(m_wBar.FindAnyWidget(string.Format("DCO_OV_Row%1_Scope", i)));
				TextWidget slbl     = TextWidget.Cast(m_wBar.FindAnyWidget(string.Format("DCO_OV_Row%1_SLabel", i)));
				if (toggle)
					toggle.AddHandler(m_Handler);
				if (scope)
					scope.AddHandler(m_Handler);
				m_RowToggle.Insert(toggle);
				m_RowToggleLabel.Insert(tlbl);
				m_RowTick.Insert(tick);
				m_RowScope.Insert(scope);
				m_RowScopeLabel.Insert(slbl);
				if (row)
					row.SetVisible(true);
				UpdateRow(i);
			}
			else if (row)
			{
				row.SetVisible(false);
			}
		}
	}

	bool OnButton(Widget w)
	{
		for (int i = 0; i < m_RowToggle.Count(); i++)
		{
			if (w == m_RowToggle[i])
			{
				ToggleOverlay(i);
				return true;
			}
			if (w == m_RowScope[i])
			{
				OpenScopeMenu(i, w);
				return true;
			}
		}
		return false;
	}

	protected void ToggleOverlay(int row)
	{
		int id = m_OverlayId[row];
		DCO_GMOverlayState st = DCO_GMOverlayState.Get();
		st.SetEnabled(id, !st.GetEnabled(id));
		UpdateRow(row);
	}

	// Reuse the viewport-aware shared menu.
	protected void OpenScopeMenu(int row, Widget scopeBtn)
	{
		if (!m_Menu)
			return;
		m_PendingScopeRow = row;
		array<string> labels = {};
		array<int> ids = {};
		for (int s = 0; s < DCO_GMOverlayState.ScopeCount(); s++)
		{
			labels.Insert(DCO_GMOverlayState.ScopeName(s));
			ids.Insert(s);
		}
		m_Menu.ShowAdjacent(labels, ids, scopeBtn, m_wBar, "TARGET SCOPE", m_ScopeCb, null);
	}

	protected void OnScopePicked(int scopeValue, SCR_EditableEntityComponent e)
	{
		if (m_PendingScopeRow < 0 || m_PendingScopeRow >= m_OverlayId.Count())
			return;
		int row = m_PendingScopeRow;
		m_PendingScopeRow = -1;
		DCO_GMOverlayState.Get().SetScope(m_OverlayId[row], scopeValue);
		UpdateRow(row);
	}

	protected void UpdateRow(int row)
	{
		if (row < 0 || row >= m_OverlayId.Count())
			return;
		int id = m_OverlayId[row];
		DCO_GMOverlayState st = DCO_GMOverlayState.Get();
		bool on = st.GetEnabled(id);

		TextWidget tlbl = m_RowToggleLabel[row];
		if (tlbl)
		{
			tlbl.SetText(m_OverlayName[row]);
			if (on)
				tlbl.SetColor(DCO_GMTheme.Get().m_AccentColor);
			else
				tlbl.SetColor(DCO_GMTheme.Get().m_MutedColor);
		}

		if (row < m_RowTick.Count() && m_RowTick[row])
		{
			ImageWidget tick = m_RowTick[row];
			tick.SetColor(DCO_GMTheme.Get().m_AccentColor);
			if (on)
				tick.SetOpacity(1.0);
			else
				tick.SetOpacity(0.0);
		}

		ButtonWidget scope = m_RowScope[row];
		if (scope)
			scope.SetVisible(id != DCO_GMOverlayState.OV_FPS);

		TextWidget slbl = m_RowScopeLabel[row];
		if (slbl)
		{
			slbl.SetText(DCO_GMOverlayState.ScopeName(st.GetScope(id)) + "  ▾");
			if (on)
				slbl.SetColor(DCO_GMTheme.Get().m_LabelColor);	// scope only matters while the overlay is on.
			else
				slbl.SetColor(DCO_GMTheme.Get().m_DisabledColor);
		}
	}
}
