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

class DCO_GMOptionsPanelStaticData
{
	ref array<float> m_SwatchHues = {32, 0, 120, 190, 225, 280};
	ref array<string> m_VisibilityLabels = {"OVERLAYS", "ORDERS", "NOTIFY", "CHAT", "GIZMO", "NAMETAGS",
		"TOPBAR", "BOTBAR", "EDIT", "CREATE", "TACTICS", "INFO"};
}

class DCO_GMOptionsPanel
{
	protected static ref DCO_GMOptionsPanelStaticData s_StaticData;

	protected static DCO_GMOptionsPanelStaticData StaticData()
	{
		if (!s_StaticData)
			s_StaticData = new DCO_GMOptionsPanelStaticData();
		return s_StaticData;
	}

	protected Widget m_wRoot;
	protected Widget m_wPanel;
	protected Widget m_wContent;
	protected ButtonWidget m_btnOpen;
	protected ButtonWidget m_btnClose;
	protected ButtonWidget m_btnMaster;
	protected ButtonWidget m_btnReopen;
	protected ButtonWidget m_btnFontCompact;
	protected ButtonWidget m_btnFontCommand;
	protected ButtonWidget m_btnNativeTools;
	protected DCO_GMContextMenu m_Menu;
	protected ref DCO_OptionsButtonHandler m_Handler;
	protected ref ScriptInvoker m_NativeToolsCallback = new ScriptInvoker();
	protected ref array<SCR_BaseEditorAction> m_NativeToolActions = {};
	protected SCR_ToolbarActionsEditorComponent m_ToolbarActions;
	protected ref DCO_GMSlider m_OpacitySlider;	// "Panel opacity" control.
	protected ref DCO_GMSlider m_HueSlider;	// "Accent colour" hue control.
	protected ref array<ref DCO_OptSwatchHandler> m_SwatchHandlers = {};
	protected ref array<ref DCO_OptVisibilityHandler> m_VisibilityHandlers = {};
	protected bool m_bOpen;
	protected bool m_bGeomInit;

	void Init(Widget root, DCO_GMContextMenu menu = null)
	{
		if (!root)
			return;
		m_wRoot = root;
		m_Menu = menu;
		m_Handler = new DCO_OptionsButtonHandler(this);
		m_NativeToolsCallback.Insert(OnNativeToolAction);

		m_wPanel   = root.FindAnyWidget("DCO_OptionsPanel");
		m_wContent = root.FindAnyWidget("DCO_OptionsVBox");
		m_btnOpen   = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptionsBtn"));
		m_btnClose  = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptionsClose"));
		m_btnMaster = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptMasterBtn"));
		m_btnReopen = ButtonWidget.Cast(root.FindAnyWidget(DCO_GMTheme.REOPEN_CHIP));
		m_btnFontCompact = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptFontCompact"));
		m_btnFontCommand = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptFontCommand"));
		m_btnNativeTools = ButtonWidget.Cast(root.FindAnyWidget("DCO_OptNativeTools"));

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
		if (m_btnNativeTools)
			m_btnNativeTools.AddHandler(m_Handler);
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

		for (int i = 0; i < StaticData().m_SwatchHues.Count(); i++)
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
		if (index < 0 || index >= StaticData().m_SwatchHues.Count())
			return false;
		float hue = StaticData().m_SwatchHues[index];
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
			label.SetText(StaticData().m_VisibilityLabels[i] + state);
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
		if (w == m_btnNativeTools)
		{
			OpenNativeTools();
			return true;
		}
		return false;
	}

	protected void OpenNativeTools()
	{
		if (!m_Menu || !m_btnNativeTools || !ResolveToolbarActions())
			return;

		vector cursorWorldPosition;
		GetCursorWorldPosition(cursorWorldPosition);
		array<ref SCR_EditorActionData> evaluatedActions = {};
		int flags;
		m_ToolbarActions.GetAndEvaluateActions(cursorWorldPosition, evaluatedActions, flags);

		m_NativeToolActions.Clear();
		array<string> labels = {};
		array<int> ids = {};
		array<bool> enabled = {};
		foreach (SCR_EditorActionData actionData : evaluatedActions)
		{
			if (!actionData)
				continue;
			SCR_BaseEditorAction action = actionData.GetAction();
			if (!IsNativeToolExposed(action))
				continue;

			ids.Insert(m_NativeToolActions.Count());
			m_NativeToolActions.Insert(action);
			labels.Insert(ResolveNativeToolLabel(action));
			enabled.Insert(actionData.GetCanBePerformed());
		}

		if (!labels.IsEmpty())
			m_Menu.ShowAdjacentWithAvailability(labels, ids, enabled, m_btnNativeTools, m_wPanel, "EDITOR TOOLS", m_NativeToolsCallback, null);
	}

	protected bool ResolveToolbarActions()
	{
		m_ToolbarActions = SCR_ToolbarActionsEditorComponent.Cast(
			SCR_ToolbarActionsEditorComponent.GetInstance(SCR_ToolbarActionsEditorComponent, false));
		return m_ToolbarActions != null;
	}

	protected bool GetCursorWorldPosition(out vector cursorWorldPosition)
	{
		cursorWorldPosition = vector.Zero;
		SCR_MenuLayoutEditorComponent menuLayout = SCR_MenuLayoutEditorComponent.Cast(
			SCR_MenuLayoutEditorComponent.GetInstance(SCR_MenuLayoutEditorComponent, false));
		return menuLayout && menuLayout.GetCursorWorldPos(cursorWorldPosition);
	}

	protected bool IsNativeToolExposed(SCR_BaseEditorAction action)
	{
		if (!action)
			return false;

		string actionType = action.Type().ToString();
		if (actionType == "SCR_TakeScreenshotDebugToolbarAction"
			|| actionType == "SCR_ToggleInterfaceToolbarAction"
			|| actionType == "SCR_HintSequenceToolbarAction"
			|| actionType == "SCR_PauseGameTimeToolbarAction")
			return false;

		EEditorActionGroup actionGroup = action.GetActionGroup();
		return actionGroup == EEditorActionGroup.SAVING
			|| actionGroup == EEditorActionGroup.SIMULATION
			|| actionGroup == EEditorActionGroup.TOOLS
			|| actionGroup == EEditorActionGroup.DYNAMIC;
	}

	protected string ResolveNativeToolLabel(SCR_BaseEditorAction action)
	{
		SCR_UIInfo info = action.GetInfo();
		SCR_BaseToggleToolbarAction toggleAction = SCR_BaseToggleToolbarAction.Cast(action);
		if (toggleAction && toggleAction.GetCurrentHighlight() && toggleAction.GetInfoToggled())
			info = toggleAction.GetInfoToggled();
		if (!info || info.GetName().IsEmpty())
			return NativeToolFallback(action.Type().ToString());

		string authoredName = info.GetName();
		if (authoredName[0] != "#")
			return authoredName;

		string translatedName = WidgetManager.Translate(authoredName);
		if (!translatedName.IsEmpty() && translatedName[0] != "#")
		{
			string unprefixedName = authoredName.Substring(1, authoredName.Length() - 1);
			if (translatedName != unprefixedName)
				return translatedName;
		}
		return NativeToolFallback(action.Type().ToString());
	}

	protected string NativeToolFallback(string actionType)
	{
		actionType.Replace("SCR_", "");
		actionType.Replace("ToolbarAction", "");
		actionType.Replace("Action", "");
		actionType.Replace("_", " ");
		return actionType;
	}

	protected void OnNativeToolAction(int actionId, SCR_EditableEntityComponent entity)
	{
		if (!m_NativeToolActions.IsIndexValid(actionId))
			return;
		SCR_BaseEditorAction selectedAction = m_NativeToolActions[actionId];
		if (!selectedAction || !ResolveToolbarActions())
			return;

		vector cursorWorldPosition;
		GetCursorWorldPosition(cursorWorldPosition);
		array<ref SCR_EditorActionData> evaluatedActions = {};
		int flags;
		m_ToolbarActions.GetAndEvaluateActions(cursorWorldPosition, evaluatedActions, flags);
		foreach (SCR_EditorActionData actionData : evaluatedActions)
		{
			if (!actionData || actionData.GetAction() != selectedAction || !actionData.GetCanBePerformed())
				continue;
			SetOpen(false);
			m_ToolbarActions.ActionPerform(selectedAction, cursorWorldPosition, flags);
			return;
		}
	}

	bool CloseForBack()
	{
		if (!m_bOpen)
			return false;
		SetOpen(false);
		return true;
	}

	protected void SetOpen(bool open)
	{
		bool wasOpen = m_bOpen;
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
		if (!open && wasOpen)
			DCO_GMUIController.ReleaseMenuFocus();
		if (open)
			GetGame().GetCallqueue().CallLater(RefreshControls, 60);	// after the panel is laid out, size the slider fills.
	}

	protected void ApplyDefaultGeometry()
	{
		if (!m_wPanel)
			return;
		FrameSlot.SetAnchor(m_wPanel, 0.5, 0.5);
		FrameSlot.SetAlignment(m_wPanel, 0.5, 0.5);
		FrameSlot.SetSize(m_wPanel, 420, 540);
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
		m_btnNativeTools = null;
		m_NativeToolsCallback.Remove(OnNativeToolAction);
		m_NativeToolActions.Clear();
		m_ToolbarActions = null;
		m_Menu = null;
		m_wPanel = null;
		m_wContent = null;
		m_Handler = null;
		m_wRoot = null;
	}
}
