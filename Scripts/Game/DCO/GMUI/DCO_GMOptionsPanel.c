// Bifrost GM UI OPTIONS panel.

class DCO_OptionsButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMOptionsPanel m_Owner;

	void DCO_OptionsButtonHandler(DCO_GMOptionsPanel owner)
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

// One per colour swatch; carries the swatch index so OnClick maps to that swatch's hue.
class DCO_OptSwatchHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMOptionsPanel m_Owner;
	protected int m_Index;

	void DCO_OptSwatchHandler(DCO_GMOptionsPanel owner, int index)
	{
		m_Owner = owner;
		m_Index = index;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnSwatch(m_Index);
		return false;
	}
}

class DCO_OptVisibilityHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMOptionsPanel m_Owner;
	protected int m_Index;

	void DCO_OptVisibilityHandler(DCO_GMOptionsPanel owner, int index)
	{
		m_Owner = owner;
		m_Index = index;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.ToggleVisibility(m_Index);
		return false;
	}
}

class DCO_GMOptionsPanel
{
	static const ref array<float> SWATCH_HUES = {32, 0, 120, 190, 225, 280};
	static const ref array<string> VIS_LABELS = {"OVERLAYS", "ORDERS", "NOTIFY", "CHAT", "GIZMO", "NAMETAGS",
		"TOPBAR", "BOTBAR", "EDIT", "CREATE", "TACTICS", "INFO"};

	protected Widget m_wRoot;
	protected Widget m_wPanel;
	protected Widget m_wContent;
	protected ButtonWidget m_btnOpen;
	protected ButtonWidget m_btnClose;
	protected ButtonWidget m_btnMaster;
	protected ButtonWidget m_btnReopen;
	protected ButtonWidget m_btnFontCompact;
	protected ButtonWidget m_btnFontCommand;
	protected ref DCO_OptionsButtonHandler m_Handler;
	protected ref DCO_GMSlider m_OpacitySlider;	// "Panel opacity" control.
	protected ref DCO_GMSlider m_HueSlider;	// "Accent colour" hue control.
	protected ref array<ref DCO_OptSwatchHandler> m_SwatchHandlers = {};
	protected ref array<ref DCO_OptVisibilityHandler> m_VisibilityHandlers = {};
	protected bool m_bOpen;
	protected bool m_bGeomInit;

	void Init(Widget root)
	{
		if (!root)
			return;
		m_wRoot = root;
		m_Handler = new DCO_OptionsButtonHandler(this);

		m_wPanel   = root.FindAnyWidget("DCO_OptionsPanel");
		m_wContent = root.FindAnyWidget("DCO_OptionsVBox");
		m_btnOpen   = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptionsBtn"));
		m_btnClose  = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptionsClose"));
		m_btnMaster = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptMasterBtn"));
		m_btnReopen = ButtonWidget.Cast(root.FindAnyWidget(DCO_GMTheme.REOPEN_CHIP));
		m_btnFontCompact = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptFontCompact"));
		m_btnFontCommand = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptFontCommand"));

		if (m_btnOpen)
			m_btnOpen.AddHandler(m_Handler);
		if (m_btnClose)
			m_btnClose.AddHandler(m_Handler);
		if (m_btnMaster)
			m_btnMaster.AddHandler(m_Handler);
		if (m_btnFontCompact)
			m_btnFontCompact.AddHandler(m_Handler);
		if (m_btnFontCommand)
			m_btnFontCommand.AddHandler(m_Handler);
		if (m_btnReopen)
		{
			m_btnReopen.AddHandler(m_Handler);
			// Bottom-left, clear of the engine/DCO status bars.
			FrameSlot.SetAnchor(m_btnReopen, 0, 1);
			FrameSlot.SetAlignment(m_btnReopen, 0, 1);
			FrameSlot.SetSize(m_btnReopen, 30, 30);
			FrameSlot.SetPos(m_btnReopen, 8, -8);
		}

		if (m_wPanel)
			m_wPanel.SetVisible(false);	// hidden until the bottom-bar button is clicked.
		m_bOpen = false;

		// Panel-opacity slider.
		m_OpacitySlider = new DCO_GMSlider();
		m_OpacitySlider.Init(root, "DCO_OptOpacity_Track", "DCO_OptOpacity_Fill", "DCO_OptOpacity_Value",
			0, 100.0 - DCO_GMTheme.OPACITY_MIN * 100.0, (1.0 - DCO_GMTheme.Get().m_PanelOpacity) * 100, "%");
		m_OpacitySlider.GetOnChange().Insert(OnOpacityChanged);

		m_HueSlider = new DCO_GMSlider();
		m_HueSlider.Init(root, "DCO_OptHue_Track", "DCO_OptHue_Fill", "DCO_OptHue_Value",
			0, 360, DCO_GMTheme.Get().m_AccentHue, "");
		m_HueSlider.GetOnChange().Insert(OnAccentHueChanged);

		for (int i = 0; i < SWATCH_HUES.Count(); i++)
		{
			ButtonWidget sw = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptSwatch" + i.ToString()));
			if (sw)
			{
				DCO_OptSwatchHandler h = new DCO_OptSwatchHandler(this, i);
				sw.AddHandler(h);
				m_SwatchHandlers.Insert(h);
			}
		}

		// Per-element HUD switches.
		for (int vi = 0; vi < DCO_GMTheme.UI_ELEMENT_COUNT; vi++)
		{
			ButtonWidget vb = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptVis" + vi.ToString()));
			if (!vb)
				continue;
			DCO_OptVisibilityHandler vh = new DCO_OptVisibilityHandler(this, vi);
			vb.AddHandler(vh);
			m_VisibilityHandlers.Insert(vh);
		}
		DCO_GMTheme.Get().ApplyElementVisibility(root);
		DCO_GMTheme.Get().ApplyDisplayFont(root);
		RefreshVisibility();
		RefreshFontMode();

		Print(string.Format("[DCO-GM] options panel bound (open/panel/close=%1/%2/%3 swatches=%4 visibility switches=%5 master/chip=%6/%7 fonts=%8)",
			m_btnOpen != null, m_wPanel != null, m_btnClose != null, m_SwatchHandlers.Count(),
			m_VisibilityHandlers.Count(), m_btnMaster != null, m_btnReopen != null,
			m_btnFontCompact != null && m_btnFontCommand != null),
			LogLevel.NORMAL);
	}

	protected void OnOpacityChanged(float value)
	{
		// Slider value = see-through % (0 = solid, 100 = fully see-through); the panel opacity it applies is the inverse.
		DCO_GMTheme.Get().SetPanelOpacity(1.0 - value / 100, m_wRoot);
	}

	protected void OnAccentHueChanged(float hue)
	{
		DCO_GMTheme.Get().SetAccentHue(hue, m_wRoot);	// tracks hue + recolors live + persists.
		RefreshFontMode();
	}

	// Preset swatch clicked: jump the accent to that hue and sync the hue slider.
	bool OnSwatch(int index)
	{
		if (index < 0 || index >= SWATCH_HUES.Count())
			return false;
		float hue = SWATCH_HUES[index];
		if (m_HueSlider)
			m_HueSlider.SetValue(hue);
		DCO_GMTheme.Get().SetAccentHue(hue, m_wRoot);
		RefreshFontMode();
		return true;
	}

	bool ToggleVisibility(int index)
	{
		DCO_GMTheme theme = DCO_GMTheme.Get();
		theme.SetElementEnabled(index, !theme.IsElementEnabled(index), m_wRoot);
		RefreshVisibility();
		return true;
	}

	protected void RefreshVisibility()
	{
		DCO_GMTheme theme = DCO_GMTheme.Get();
		for (int i = 0; i < DCO_GMTheme.UI_ELEMENT_COUNT; i++)
		{
			TextWidget label = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_OptVis" + i.ToString() + "_Label"));
			if (!label)
				continue;
			bool enabled = theme.IsElementEnabled(i);
			string state = " OFF";
			if (enabled)
				state = " ON";
			label.SetText(VIS_LABELS[i] + state);
			Color stateColor = theme.m_DisabledColor;
			if (enabled)
				stateColor = theme.m_AccentColor;
			label.SetColor(stateColor);

			ImageWidget icon = ImageWidget.Cast(m_wRoot.FindAnyWidget("DCO_OptVis" + i.ToString() + "_Icon"));
			if (icon)
				icon.SetColor(stateColor);
		}
	}

	protected void RefreshFontMode()
	{
		int mode = DCO_GMTheme.Get().m_DisplayFontMode;
		TextWidget compact = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_OptFontCompact_Label"));
		TextWidget command = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_OptFontCommand_Label"));
		if (compact)
		{
			compact.SetColor(DCO_GMTheme.Get().m_MutedColor);
			if (mode == DCO_GMTheme.FONT_COMPACT)
				compact.SetColor(DCO_GMTheme.Get().m_AccentColor);
		}
		if (command)
		{
			command.SetColor(DCO_GMTheme.Get().m_MutedColor);
			if (mode == DCO_GMTheme.FONT_COMMAND)
				command.SetColor(DCO_GMTheme.Get().m_AccentColor);
		}
	}

	protected void RefreshControls()
	{
		if (m_OpacitySlider)
			m_OpacitySlider.SetValue((1.0 - DCO_GMTheme.Get().m_PanelOpacity) * 100);
		if (m_HueSlider)
			m_HueSlider.SetValue(DCO_GMTheme.Get().m_AccentHue);	// reflect the saved/current hue + re-size its fill.
		RefreshVisibility();
		RefreshFontMode();
	}

	bool OnButton(Widget w)
	{
		if (w == m_btnOpen)
		{
			SetOpen(!m_bOpen);
			return true;
		}
		if (w == m_btnClose)
		{
			SetOpen(false);
			return true;
		}
		if (w == m_btnReopen)
		{
			SetOpen(true);
			return true;
		}
		if (w == m_btnMaster)
		{
			DCO_GMUIController.MasterToggle();
			return true;
		}
		if (w == m_btnFontCompact)
		{
			DCO_GMTheme.Get().SetDisplayFontMode(DCO_GMTheme.FONT_COMPACT, m_wRoot);
			RefreshFontMode();
			return true;
		}
		if (w == m_btnFontCommand)
		{
			DCO_GMTheme.Get().SetDisplayFontMode(DCO_GMTheme.FONT_COMMAND, m_wRoot);
			RefreshFontMode();
			return true;
		}
		return false;
	}

	protected void SetOpen(bool open)
	{
		m_bOpen = open;
		if (open && !m_bGeomInit)	// position sensibly the first time, then keep wherever the GM drags/resizes it.
		{
			float gx, gy, gw, gh;
			if (!DCO_GMTheme.Get().GetPanelGeom("DCO_OptionsPanel", gx, gy, gw, gh))
				ApplyDefaultGeometry();
			m_bGeomInit = true;
		}
		if (m_wPanel)
			m_wPanel.SetVisible(open);
		if (open)
			GetGame().GetCallqueue().CallLater(RefreshControls, 60);	// after the panel is laid out, size the slider fills.
	}

	protected void ApplyDefaultGeometry()
	{
		if (!m_wPanel)
			return;
		FrameSlot.SetAnchor(m_wPanel, 0.5, 0.5);
		FrameSlot.SetAlignment(m_wPanel, 0.5, 0.5);
		FrameSlot.SetSize(m_wPanel, 420, 500);
		FrameSlot.SetPos(m_wPanel, 0, 0);
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(RefreshControls);
		if (m_OpacitySlider)
		{
			m_OpacitySlider.Shutdown();
			m_OpacitySlider = null;
		}
		if (m_HueSlider)
		{
			m_HueSlider.Shutdown();
			m_HueSlider = null;
		}
		m_SwatchHandlers.Clear();
		m_VisibilityHandlers.Clear();
		m_btnOpen = null;
		m_btnClose = null;
		m_btnMaster = null;
		m_btnReopen = null;
		m_btnFontCompact = null;
		m_btnFontCommand = null;
		m_wPanel = null;
		m_wContent = null;
		m_Handler = null;
		m_wRoot = null;
	}
}
