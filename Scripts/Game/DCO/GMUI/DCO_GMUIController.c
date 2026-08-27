// Shell controller for the Bifrost GM UI.
class DCO_GMUIController
{

	protected ResourceName m_PanelLayout;
	protected Widget m_wRoot;
	protected ref DCO_GMTopBarComponent m_TopBar;
	protected ref DCO_GMCreatePanelComponent m_CreatePanel;
	protected ref DCO_GMPlacementConfirm m_PlaceConfirm;	// restores the world-click that spawns a previewed entity.
	protected ref DCO_GMEditTreeComponent m_EditTree;
	protected ref DCO_GMScenarioPanel m_Scenario;
	protected ref DCO_GMOptionsPanel m_Options;
	protected ref DCO_GMPreciseBar m_PreciseBar;
	protected ref DCO_GMGizmoPanel m_GizmoPanel;	// live numeric transform readout, fed by the gizmo each render tick.
	protected ref DCO_GMNotifFeed m_NotifFeed;
	protected ref DCO_GMChatFeed m_ChatFeed;
	protected ref DCO_GMContextMenu m_Menu;
	protected ref DCO_GMOrdersPanel m_OrdersPanel;
	protected ref DCO_GMContextMenuBridge m_Bridge;
	protected ref DCO_GMRenderManager m_Render;
	protected ref DCO_GMNametags m_Nametags;
	protected ref DCO_GMAwarenessCue m_Awareness;
	protected ref DCO_GMOverlayPanel m_OverlayPanel;	// checkbox panel that toggles the overlays.
	protected ref array<ref DCO_GMDraggable> m_Draggables = {};
	protected ref array<ref DCO_GMResizable> m_Resizables = {};
	protected ref DCO_VanillaUIVisibility m_VanillaHide = new DCO_VanillaUIVisibility();
	protected ref DCO_GMLayoutResetHandler m_ResetHandler;
	protected bool m_bEditShown = true;
	protected bool m_bCreateShown = true;
	protected bool m_bBuilt;
	protected int m_PeelAttempts;
	protected int m_iViewportW;	// live game-workspace size; Workbench can resize this independently of its outer window.
	protected int m_iViewportH;

	protected static DCO_GMUIController s_Instance;
	protected static int s_iPauseSuppressedUntil;
	protected static const int PAUSE_SUPPRESS_MS = 200;

	static bool IsActive()
	{
		return s_Instance != null && s_Instance.m_bBuilt;
	}

	static void ReleaseMenuFocus()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.SetFocusedWidget(null, true);
	}

	static bool ShouldSuppressPauseOpen()
	{
		int now = System.GetTickCount();
		if (s_iPauseSuppressedUntil > now)
			return true;
		if (!s_Instance || !s_Instance.CloseTopCaptivatingMenu())
		{
			s_iPauseSuppressedUntil = 0;
			return false;
		}
		s_iPauseSuppressedUntil = now + PAUSE_SUPPRESS_MS;
		return true;
	}

	static void RevealInCreate(string name)
	{
		if (s_Instance && s_Instance.m_bBuilt && s_Instance.m_CreatePanel)
			s_Instance.m_CreatePanel.RevealByName(name);
	}

	// Theme-owned EDIT/CREATE visibility must route through their component owners, not raw widget writes.
	static void ApplyColumnVisible(bool editColumn, bool visible)
	{
		if (!s_Instance)
			return;
		if (editColumn)
		{
			s_Instance.m_bEditShown = visible;
			if (s_Instance.m_wRoot)
			{
				Widget tree = s_Instance.m_wRoot.FindAnyWidget("DCO_EditTree");
				if (tree)
				{
					tree.SetVisible(visible);
					if (visible)
					{
						tree.SetOpacity(0.15);
						AnimateWidget.Opacity(tree, 1.0, 7.0, true);
					}
				}
			}
		}
		else
		{
			s_Instance.m_bCreateShown = visible;
			if (s_Instance.m_CreatePanel)
				s_Instance.m_CreatePanel.Show(visible);
			if (visible && s_Instance.m_wRoot)
			{
				Widget browser = s_Instance.m_wRoot.FindAnyWidget("DCO_CreateBrowser");
				if (browser)
				{
					browser.SetOpacity(0.15);
					AnimateWidget.Opacity(browser, 1.0, 7.0, true);
				}
			}
		}
		if (s_Instance.m_TopBar)
			s_Instance.m_TopBar.SyncTabState(s_Instance.m_bEditShown, s_Instance.m_bCreateShown);
	}

	void DCO_GMUIController(ResourceName panelLayout)
	{
		m_PanelLayout = panelLayout;
	}

	void Activate()
	{
		GetGame().OnInputDeviceIsGamepadInvoker().Insert(OnDeviceChanged);
		Evaluate();
	}

	void Deactivate()
	{
		GetGame().OnInputDeviceIsGamepadInvoker().Remove(OnDeviceChanged);
		Teardown();
	}

	protected void OnDeviceChanged(bool isGamepad)
	{
		Evaluate();
	}

	protected void AddMenuActionListeners()
	{
		InputManager im = GetGame().GetInputManager();
		if (!im)
			return;
		im.AddActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, OnMenuAction);
		im.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnMenuAction);
		#ifdef WORKBENCH
		im.AddActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, OnMenuAction);
		im.AddActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, OnMenuAction);
		#endif
	}

	protected void RemoveMenuActionListeners()
	{
		InputManager im = GetGame().GetInputManager();
		if (!im)
			return;
		im.RemoveActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, OnMenuAction);
		im.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnMenuAction);
		#ifdef WORKBENCH
		im.RemoveActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, OnMenuAction);
		im.RemoveActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, OnMenuAction);
		#endif
	}

	protected void OnMenuAction(float value, EActionTrigger reason)
	{
		int now = System.GetTickCount();
		if (s_iPauseSuppressedUntil > now)
			return;
		if (CloseTopCaptivatingMenu())
		{
			InputManager im = GetGame().GetInputManager();
			if (im)
			{
				im.SetActionValue(UIConstants.MENU_ACTION_OPEN, 0);
				im.SetActionValue(UIConstants.MENU_ACTION_BACK, 0);
				#ifdef WORKBENCH
				im.SetActionValue(UIConstants.MENU_ACTION_OPEN_WB, 0);
				im.SetActionValue(UIConstants.MENU_ACTION_BACK_WB, 0);
				#endif
			}
			s_iPauseSuppressedUntil = now + PAUSE_SUPPRESS_MS;
		}
	}

	protected bool CloseTopCaptivatingMenu()
	{
		// The full-screen outside-click catcher is always the topmost captor.
		if (m_Menu && m_Menu.IsOpen())
		{
			m_Menu.Hide();
			return true;
		}
		if (DCO_GMTutorial.IsOpen())
		{
			DCO_GMTutorial.Close();
			return true;
		}
		if (DCO_GMArsenalPanel.Get().IsOpen())
		{
			DCO_GMArsenalPanel.Get().CloseSilent();
			return true;
		}
		if (m_Scenario && m_Scenario.CloseForBack())
			return true;
		if (DCO_GMTacticsPanel.Get().IsOpen())
		{
			DCO_GMTacticsPanel.Get().CloseSilent();
			return true;
		}
		if (m_PreciseBar && m_PreciseBar.CloseForBack())
			return true;
		if (m_Options && m_Options.CloseForBack())
			return true;
		return false;
	}

	// Build for mouse+keyboard, tear down for gamepad.
	protected void Evaluate()
	{
		bool kbm = GetGame().GetInputManager().IsUsingMouseAndKeyboard();
		if (kbm && !m_bBuilt)
			Build();
		else if (!kbm && m_bBuilt)
			Teardown();
	}

	protected void Build()
	{
		if (m_PanelLayout.IsEmpty())
			return;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		DCO_GMTheme.Get();

		m_wRoot = workspace.CreateWidgets(m_PanelLayout);	// workspace root = on top + clickable.
		if (!m_wRoot)
		{
			Print("[DCO-GM] shell mount FAIL: CreateWidgets null for " + m_PanelLayout, LogLevel.WARNING);
			return;
		}
		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);
		m_wRoot.SetOpacity(0.0);
		s_Instance = this;	// visibility restoration below needs the component-owning live instance.

		m_CreatePanel = new DCO_GMCreatePanelComponent();
		m_CreatePanel.Init(m_wRoot);

		m_PlaceConfirm = new DCO_GMPlacementConfirm();
		m_PlaceConfirm.Start(m_wRoot);

		m_Menu = new DCO_GMContextMenu();
		m_Menu.Init(m_wRoot);

		m_EditTree = new DCO_GMEditTreeComponent();
		m_EditTree.Init(m_wRoot, m_Menu);

		m_OrdersPanel = new DCO_GMOrdersPanel();
		m_OrdersPanel.Init(m_wRoot, m_Menu);

		DCO_GMTacticsPanel.Get().Init(m_wRoot);
		DCO_GMTacticsFlow.Get().Init(m_wRoot);
		DCO_ArsenalAccessPlacement.Get().Init();

		DCO_GMArsenalPanel.Get().Init(m_wRoot);

		DCO_GMGameplayPanel.Get().Init(m_wRoot);

		// Replace the engine GM right-click menu with ours on world right-clicks.
		m_Bridge = new DCO_GMContextMenuBridge();
		m_Bridge.Init(m_wRoot, m_Menu);

		m_Render = new DCO_GMRenderManager();
		m_Render.Start(m_wRoot);

		m_Nametags = new DCO_GMNametags();
		m_Nametags.Init(m_wRoot);
		m_Nametags.Start();

	// Mounts tactical paths, perception cues, and role markers.
		m_Awareness = new DCO_GMAwarenessCue();
		m_Awareness.Start(m_Render, m_wRoot);

		DCO_GMGizmo.Get().Start(m_Render);
		m_OverlayPanel = new DCO_GMOverlayPanel();
		m_OverlayPanel.Init(m_wRoot, m_Menu);	// shared menu pool drives the per-overlay scope dropdowns.

		m_TopBar = new DCO_GMTopBarComponent();
		m_TopBar.Init(m_wRoot, this);

		// Inline scenario/global-attributes panel, opened by the gear cog left of the EDIT/CREATE tabs.
		m_Scenario = new DCO_GMScenarioPanel();
		m_Scenario.Init(m_wRoot, m_Menu);

		m_Options = new DCO_GMOptionsPanel();
		m_Options.Init(m_wRoot);

		m_PreciseBar = new DCO_GMPreciseBar();
		m_PreciseBar.Init(m_wRoot);

		// Live numeric transform readout.
		m_GizmoPanel = new DCO_GMGizmoPanel();
		m_GizmoPanel.Init(m_wRoot);
		DCO_GMGizmo.Get().SetPanel(m_GizmoPanel);

		// Bifrost-native notification + chat feeds.
		m_NotifFeed = new DCO_GMNotifFeed();
		m_NotifFeed.Init(m_wRoot);
		m_ChatFeed = new DCO_GMChatFeed();
		m_ChatFeed.Init(m_wRoot);

		DCO_GMTutorial.Get().Init(m_wRoot);

		DCO_GMTheme.Get().ApplyElementVisibility(m_wRoot);

		ApplyLayout();

		// Make the floating panels draggable by their top-left grip.
		MakeDraggable("DCO_OverlayDrag",  "DCO_OverlayBar");
		MakeDraggable("DCO_OrdersDrag",   "DCO_OrdersBox");

		MakeResizable("DCO_OverlayResize",  "DCO_OverlayBar",    180, 110);
		MakeResizable("DCO_OrdersResize",   "DCO_OrdersBox",     180, 140);
		MakeDraggable("DCO_OptionsDrag",   "DCO_OptionsPanel");
		MakeResizable("DCO_OptionsResize", "DCO_OptionsPanel",  380, 500);

		// The gizmo readout is a floating box like Overlays/Orders, so it gets the same grips.
		MakeDraggable("DCO_GizmoDrag",   "DCO_GizmoPanel");
		MakeResizable("DCO_GizmoResize", "DCO_GizmoPanel",     260, 190);

		// The notification + chat feeds float too - same grips, same geometry persistence, RESET UI covers them.
		MakeDraggable("DCO_NotifDrag",   "DCO_NotifPanel");
		MakeResizable("DCO_NotifResize", "DCO_NotifPanel",     240, 110);
		MakeDraggable("DCO_ChatDrag",    "DCO_ChatPanel");
		MakeResizable("DCO_ChatResize",  "DCO_ChatPanel",      280, 120);
		MakeDraggable("DCO_TacticsDrag",   "DCO_TacticsPanel");
		MakeResizable("DCO_TacticsResize", "DCO_TacticsPanel", 250, 360);

		// Hover feedback.
		DCO_GMHover.Clear();
		array<string> hoverBtns = {
			"DCO_ETCat_ALL", "DCO_ETCat_UNIT", "DCO_ETCat_VEH", "DCO_ETCat_OBJ", "DCO_ETCat_LOC", "DCO_ETCat_AREA",
			"DCO_Btn_Pause", "DCO_Btn_Resume",
			"DCO_Clk_STOP", "DCO_Clk_1", "DCO_Clk_4", "DCO_Clk_12", "DCO_Clk_60",
			"DCO_Btn_ScopeSelected", "DCO_Btn_ScopeAI",
			"DCO_Chk_AI", "DCO_Chk_Physics",
			"DCO_Btn_GizmoSnap", "DCO_Btn_GizmoSpace", "DCO_Btn_GizmoSurf",
			"DCO_Btn_PreciseAttach", "DCO_Btn_PreciseDetach", "DCO_Btn_Precise"
		};
		foreach (string hb : hoverBtns)
		{
			DCO_GMHover.Wire(m_wRoot, hb, hb + "_Label");
		}
		array<string> createCatBtns = {"DCO_Cat_ALL", "DCO_Cat_MEN", "DCO_Cat_GRP", "DCO_Cat_OBJ", "DCO_Cat_SYS", "DCO_Cat_FX"};
		foreach (string cb : createCatBtns)
			DCO_GMHover.Wire(m_wRoot, cb, cb + "_Icon");
		DCO_GMHover.WirePool(m_wRoot, "DCO_OV_Row%1_Toggle", "DCO_OV_Row%1_TLabel", 0, 6);
		DCO_GMHover.WirePool(m_wRoot, "DCO_OV_Row%1_Scope",  "DCO_OV_Row%1_SLabel", 0, 6);

		DCO_GMHover.Wire(m_wRoot, "DCO_Tab_Edit",         "DCO_Tab_Edit_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_Tab_Create",       "DCO_Tab_Create_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_ScenarioCog",      "DCO_ScenarioCogIcon");
		DCO_GMHover.Wire(m_wRoot, "DCO_OptionsBtn",       "DCO_OptionsBtnIcon");
		DCO_GMHover.Wire(m_wRoot, "DCO_OptMasterBtn",     "DCO_OptMasterIcon");
		DCO_GMHover.Wire(m_wRoot, "DCO_OptFontCompact",   "DCO_OptFontCompact_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_OptFontCommand",   "DCO_OptFontCommand_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_QuickEditBtn",     "DCO_QuickEditBtn_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_QuickDeployBtn",   "DCO_QuickDeployBtn_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_QuickCleanBtn",    "DCO_QuickCleanBtn_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_LayoutResetBtn",   "DCO_LayoutResetBtn_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_LayoutChip",       "DCO_LayoutChipIcon");
		DCO_GMHover.Wire(m_wRoot, "DCO_Btn_PreciseMove",  "DCO_Btn_PreciseMove_Icon");
		DCO_GMHover.Wire(m_wRoot, "DCO_Btn_PreciseRotate","DCO_Btn_PreciseRotate_Icon");
		DCO_GMHover.Wire(m_wRoot, "DCO_Btn_PreciseSim",   "DCO_Btn_PreciseSim_Icon");
		DCO_GMHover.Wire(m_wRoot, "DCO_ScenarioClose",    "DCO_ScenarioClose_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_OptionsClose",     "DCO_OptionsClose_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_SimClose",         "DCO_SimClose_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_TacticsClose",     "DCO_TacticsClose_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_TacticsSpring",    "DCO_TacticsSpring_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_TacticsRearm",     "DCO_TacticsRearm_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_TacPair_Minus",    "DCO_TacPair_Minus_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_TacPair_Plus",     "DCO_TacPair_Plus_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_TacSend_Pill",     "DCO_TacSend_PillBg");
		DCO_GMHover.Wire(m_wRoot, "DCO_Tree_Prev",        "DCO_Tree_Prev_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_Tree_Next",        "DCO_Tree_Next_Label");
		DCO_GMHover.Wire(m_wRoot, "DCO_SimStance_STAND",  "DCO_SimStance_STAND_Icon");
		DCO_GMHover.Wire(m_wRoot, "DCO_SimStance_CROUCH", "DCO_SimStance_CROUCH_Icon");
		DCO_GMHover.Wire(m_wRoot, "DCO_SimStance_PRONE",  "DCO_SimStance_PRONE_Icon");
		array<string> ordBtns = {"DCO_Ord_Stance", "DCO_Ord_Formation", "DCO_Ord_Behavior", "DCO_Ord_Tactics",
			"DCO_Ord_Waypoints", "DCO_Ord_Objectives", "DCO_Ord_Spawn"};
		foreach (string ob : ordBtns)
		{
			DCO_GMHover.Wire(m_wRoot, ob, ob + "_Label");
		}
		array<string> grips = {"DCO_ScenarioDrag", "DCO_ScenarioResize", "DCO_OrdersDrag", "DCO_OrdersResize",
			"DCO_OverlayDrag", "DCO_OverlayResize", "DCO_OptionsDrag", "DCO_OptionsResize",
			"DCO_GizmoDrag", "DCO_GizmoResize", "DCO_NotifDrag", "DCO_NotifResize", "DCO_ChatDrag", "DCO_ChatResize",
			"DCO_TacticsDrag", "DCO_TacticsResize"};
		foreach (string gr : grips)
		{
			DCO_GMHover.Wire(m_wRoot, gr, gr + "_Icon");
		}
		DCO_GMHover.WirePool(m_wRoot, "DCO_SimRow%1_Pill", "DCO_SimRow%1_PillBg", 0, 6);
		DCO_GMHover.WirePool(m_wRoot, "DCO_Menu_%1",       "DCO_Menu_%1_Label",   0, 18);
		DCO_GMHover.WirePool(m_wRoot, "DCO_ArsCat%1",      "DCO_ArsCatIco%1",     0, 11);
		DCO_GMHover.Start();
		Print(string.Format("[DCO-GM] hover wired (%1 controls)", DCO_GMHover.Count()), LogLevel.NORMAL);

		m_ResetHandler = new DCO_GMLayoutResetHandler(this);
		array<string> utilityButtons = {"DCO_QuickEditBtn", "DCO_QuickDeployBtn", "DCO_QuickCleanBtn", "DCO_LayoutResetBtn"};
		foreach (string utilityName : utilityButtons)
		{
			ButtonWidget utilityBtn = ButtonWidget.Cast(m_wRoot.FindAnyWidget(utilityName));
			if (utilityBtn)
				utilityBtn.AddHandler(m_ResetHandler);
			else
				Print("[DCO-GM] utility bind skipped: " + utilityName, LogLevel.WARNING);
		}

		AddMasterHideListener();
		AddMenuActionListeners();

		m_PeelAttempts = 0;
		GetGame().GetCallqueue().CallLater(PeelVanillaPoll, 100, true);

		m_bBuilt = true;
		s_Instance = this;
		GetGame().GetCallqueue().CallLater(PollViewport, 500, true);
		AnimateWidget.Opacity(m_wRoot, 1.0, 5.0, true);
		Print("[DCO-GM] shell BUILT (PC / mouse+keyboard)", LogLevel.NORMAL);
	}

	protected void AddMasterHideListener()
	{
		SCR_MenuEditorComponent menuManager = SCR_MenuEditorComponent.Cast(
			SCR_MenuEditorComponent.GetInstance(SCR_MenuEditorComponent));
		if (menuManager)
			menuManager.GetOnVisibilityChange().Insert(OnVanillaUIVisibility);
	}

	protected void RemoveMasterHideListener()
	{
		SCR_MenuEditorComponent menuManager = SCR_MenuEditorComponent.Cast(
			SCR_MenuEditorComponent.GetInstance(SCR_MenuEditorComponent));
		if (menuManager)
			menuManager.GetOnVisibilityChange().Remove(OnVanillaUIVisibility);
	}

	// engine interface visibility flipped: mirror it.
	protected void OnVanillaUIVisibility(bool visible)
	{
		if (!m_wRoot)
			return;
		if (DCO_GMTheme.Get().IsMasterHidden() == !visible)
			return;
		DCO_GMTheme.Get().ToggleMasterHide(m_wRoot);
	}

	static void MasterToggle()
	{
		SCR_MenuEditorComponent menuManager = SCR_MenuEditorComponent.Cast(
			SCR_MenuEditorComponent.GetInstance(SCR_MenuEditorComponent));
		if (menuManager)
		{
			menuManager.ToggleVisible();
			return;
		}
		if (s_Instance && s_Instance.m_wRoot)
			DCO_GMTheme.Get().ToggleMasterHide(s_Instance.m_wRoot);
	}

	// LAYOUT FOUNDATION.
	protected void ApplyLayout()
	{
		if (!m_wRoot)
			return;

		const float REF_H = 1080;	// reference coordinates used by the layout serializer.
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int viewportW;
		int viewportH;
		if (workspace)
		{
			viewportW = workspace.GetWidth();
			viewportH = workspace.GetHeight();
		}
		m_iViewportW = viewportW;
		m_iViewportH = viewportH;

		bool compactViewport = viewportW > 0 && viewportH > 0 && (viewportW < 1600 || viewportH < 900);
		float BOT_BAR_H = 42;
		float TOP_Y = 54;
		float EDIT_W = 270;
		float CREATE_W = 278;
		if (compactViewport)
		{
			BOT_BAR_H = 50;
			TOP_Y = 68;
			EDIT_W = 350;
			CREATE_W = 366;
		}
		float BOTTOM_Y = REF_H - BOT_BAR_H;
		float PANEL_H = BOTTOM_Y - TOP_Y;
		const float SIDE_GAP  = 0;
		float WIDGET_GAP = 12;
		if (compactViewport)
			WIDGET_GAP = 14;
		string layoutProfile = "standard";
		if (compactViewport)
			layoutProfile = "compact";
		Print(string.Format("[DCO-GM] layout profile=%1 workspace=%2x%3", layoutProfile, viewportW, viewportH));

		// These two authored size hosts are intentionally adjusted with the same viewport profile.
		SizeLayoutWidget treeScrollHost = SizeLayoutWidget.Cast(m_wRoot.FindAnyWidget("DCO_TreeScroll_SizeHost"));
		if (treeScrollHost)
		{
			if (compactViewport)
				treeScrollHost.SetHeightOverride(250);
			else
				treeScrollHost.SetHeightOverride(330);
		}
		SizeLayoutWidget compassHost = SizeLayoutWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompassHost"));
		if (compassHost)
		{
			if (compactViewport)
			{
				compassHost.SetWidthOverride(220);
				compassHost.SetHeightOverride(48);
			}
			else
			{
				compassHost.SetWidthOverride(280);
				compassHost.SetHeightOverride(48);
			}
		}
		Widget compassTick;
		compassTick = m_wRoot.FindAnyWidget("DCO_CompassTick0");
		if (compassTick)
			compassTick.SetVisible(!compactViewport);
		compassTick = m_wRoot.FindAnyWidget("DCO_CompassTick1");
		if (compassTick)
			compassTick.SetVisible(!compactViewport);
		compassTick = m_wRoot.FindAnyWidget("DCO_CompassTick3");
		if (compassTick)
			compassTick.SetVisible(!compactViewport);
		compassTick = m_wRoot.FindAnyWidget("DCO_CompassTick4");
		if (compassTick)
			compassTick.SetVisible(!compactViewport);

		PinPanel("DCO_TopBar",    0, 0,                1, TOP_Y / REF_H,   0, 0, 0, 0);	// full-width top bar.
		PinPanel("DCO_BottomBar", 0, BOTTOM_Y / REF_H, 1, 1,      0, 0, 0, 0);

		PinBox("DCO_EditTree",      0, 0,   0, 0,   EDIT_W,   PANEL_H,   0,         TOP_Y);	// left column, below the top bar.
		PinBox("DCO_CreateBrowser", 1, 0,   1, 0,   CREATE_W, PANEL_H,   -SIDE_GAP, TOP_Y);
		float ORDERS_W = 208;	// wide enough for the longest category button row.
		float ORDERS_H = 332;
		float ORDERS_B = 54;	// bottom margin.
		if (compactViewport)
		{
			ORDERS_W = 264;
			ORDERS_H = 420;
			ORDERS_B = 62;
		}
		float ORDERS_R = CREATE_W + WIDGET_GAP;
		PinBox("DCO_OrdersBox", 1, 1,   1, 1,   ORDERS_W, ORDERS_H,   -ORDERS_R, -ORDERS_B);	// bottom-right corner.

		float overlayW = 300;
		float overlayH = 190;
		float scenarioW = 980;
		float scenarioH = 760;
		float optionsW = 420;
		float optionsH = 500;
		if (compactViewport)
		{
			overlayW = 380;
			overlayH = 250;
			scenarioW = 1100;
			scenarioH = 850;
			optionsW = 520;
			optionsH = 660;
		}
		PinBox("DCO_OverlayBar", 0, 0,   0, 0,   overlayW, overlayH,   EDIT_W + WIDGET_GAP, TOP_Y + 16);

		PinBox("DCO_ScenarioPanel", 0.5, 0.5,   0.5, 0.5,   scenarioW, scenarioH,   0, 0);
		PinBox("DCO_OptionsPanel",  0.5, 0.5,   0.5, 0.5,   optionsW, optionsH,   0, 0);	// appearance, readout type + floating-panel controls.

		float SIM_W = 250;
		float SIM_H = 350;
		if (compactViewport)
		{
			SIM_W = 310;
			SIM_H = 440;
		}
		PinBox("DCO_SimPanel", 1, 1,   1, 1,   SIM_W, SIM_H,   -(ORDERS_R + ORDERS_W + WIDGET_GAP), -(BOT_BAR_H + 6));

		float GIZ_W = 300;
		float GIZ_H = 230;	// title + the two readout lines, with room for the drag/resize grips.
		if (compactViewport)
		{
			GIZ_W = 380;
			GIZ_H = 300;
		}
		PinBox("DCO_GizmoPanel", 1, 1,   1, 1,   GIZ_W, GIZ_H,   -(ORDERS_R + ORDERS_W + WIDGET_GAP + SIM_W + WIDGET_GAP), -(BOT_BAR_H + 6));

		// Bifrost notification + chat feeds.
		float notifW = 400;
		float notifH = 196;
		float chatW = 440;
		float chatH = 220;
		float tacticsW = 260;
		float tacticsH = 400;
		if (compactViewport)
		{
			notifW = 500;
			notifH = 250;
			chatW = 560;
			chatH = 280;
			tacticsW = 330;
			tacticsH = 500;
		}
		PinBox("DCO_NotifPanel", 1, 0,   1, 0,   notifW, notifH,   -(CREATE_W + WIDGET_GAP), TOP_Y + 4);
		PinBox("DCO_ChatPanel",  0, 1,   0, 1,   chatW, chatH,   EDIT_W + WIDGET_GAP, -(BOT_BAR_H + 6));

		PinBox("DCO_TacticsPanel", 1, 1,   1, 1,   tacticsW, tacticsH,   -ORDERS_R, -(ORDERS_B + ORDERS_H + WIDGET_GAP));

		DCO_GMTheme.Get().ApplyAccent(m_wRoot);
		DCO_GMTheme.Get().ApplyOpacity(m_wRoot);
		DCO_GMTheme.Get().ApplyDisplayFont(m_wRoot);

		ApplySavedGeom("DCO_OrdersBox", ORDERS_W, ORDERS_H);
		ApplySavedGeom("DCO_OverlayBar", overlayW, overlayH);
		ApplySavedGeom("DCO_OptionsPanel", optionsW, optionsH);
		ApplySavedGeom("DCO_GizmoPanel", GIZ_W, GIZ_H);
		ApplySavedGeom("DCO_NotifPanel", notifW, notifH);
		ApplySavedGeom("DCO_ChatPanel", chatW, chatH);
		ApplySavedGeom("DCO_TacticsPanel", tacticsW, tacticsH);

		GetGame().GetCallqueue().Remove(ClampRestoredPanels);
		GetGame().GetCallqueue().CallLater(ClampRestoredPanels, 250, false);


		// Z floor for the modal/topmost chrome.
		SetTopZ("DCO_ArsenalScreen",       9000);	// full-screen mode under every popup.
		SetTopZ("DCO_ScenarioPanel",       9100);
		SetTopZ("DCO_OptionsPanel",        9200);
		SetTopZ("DCO_TutorialRoot",        9600);
		SetTopZ("DCO_MenuBackdrop",        9800);
		SetTopZ("DCO_ContextMenu",         9850);
		SetTopZ("DCO_HoverPreview",        9900);
		SetTopZ("DCO_LayoutChip",          9950);	// the recovery chip stays reachable above everything.
	}

	// Workbench's embedded game surface changes independently when the editor panes or window are resized.
	protected void PollViewport()
	{
		if (!m_bBuilt || !m_wRoot)
			return;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;
		int viewportW = workspace.GetWidth();
		int viewportH = workspace.GetHeight();
		if (viewportW == m_iViewportW && viewportH == m_iViewportH)
			return;
		ApplyLayout();
	}

	protected void SetTopZ(string name, int z)
	{
		Widget w = m_wRoot.FindAnyWidget(name);
		if (w)
			w.SetZOrder(z);
	}

	protected void ClampRestoredPanels()
	{
		if (!m_wRoot)
			return;
		DCO_GMTheme.ClampPanelToViewport(m_wRoot.FindAnyWidget("DCO_OrdersBox"));
		DCO_GMTheme.ClampPanelToViewport(m_wRoot.FindAnyWidget("DCO_OverlayBar"));
		DCO_GMTheme.ClampPanelToViewport(m_wRoot.FindAnyWidget("DCO_OptionsPanel"));
		DCO_GMTheme.ClampPanelToViewport(m_wRoot.FindAnyWidget("DCO_GizmoPanel"));
		DCO_GMTheme.ClampPanelToViewport(m_wRoot.FindAnyWidget("DCO_NotifPanel"));
		DCO_GMTheme.ClampPanelToViewport(m_wRoot.FindAnyWidget("DCO_ChatPanel"));
		DCO_GMTheme.ClampPanelToViewport(m_wRoot.FindAnyWidget("DCO_TacticsPanel"));
	}

	protected void ApplySavedGeom(string name, float minW, float minH)
	{
		float x, y, w, h;
		if (!DCO_GMTheme.Get().GetPanelGeom(name, x, y, w, h))
			return;
		w = Math.Max(w, minW);
		h = Math.Max(h, minH);
		Widget wd = m_wRoot.FindAnyWidget(name);
		if (!wd)
			return;
		FrameSlot.SetSize(wd, w, h);
		FrameSlot.SetPos(wd, x, y);	// raw restore; the deferred ClampRestoredPanels pass rescues off-screen geometry AFTER layout.
	}

	protected void PinPanel(string name, float aMinX, float aMinY, float aMaxX, float aMaxY, float offL, float offT, float offR, float offB)
	{
		Widget w = m_wRoot.FindAnyWidget(name);
		if (!w)
			return;
		FrameSlot.SetAnchorMin(w, aMinX, aMinY);
		FrameSlot.SetAnchorMax(w, aMaxX, aMaxY);
		FrameSlot.SetOffsets(w, offL, offT, offR, offB);
	}

	protected void PinBox(string name, float anchorX, float anchorY, float alignX, float alignY, float w, float h, float posX, float posY)
	{
		Widget wd = m_wRoot.FindAnyWidget(name);
		if (!wd)
			return;
		FrameSlot.SetAnchor(wd, anchorX, anchorY);
		FrameSlot.SetAlignment(wd, alignX, alignY);
		FrameSlot.SetSize(wd, w, h);
		FrameSlot.SetPos(wd, posX, posY);
	}

	// Lets the panel's grip reposition it.
	protected void MakeDraggable(string handleName, string panelName)
	{
		if (!m_wRoot)
			return;
		ButtonWidget handle = ButtonWidget.Cast(m_wRoot.FindAnyWidget(handleName));
		Widget panel = m_wRoot.FindAnyWidget(panelName);
		if (!handle || !panel)
		{
			Print(string.Format("[DCO-GM] draggable wire skipped (handle=%1 / panel=%2 found=%3,%4)",
				handleName, panelName, handle != null, panel != null), LogLevel.WARNING);
			return;
		}
		DCO_GMDraggable d = new DCO_GMDraggable(panel);
		handle.AddHandler(d);
		m_Draggables.Insert(d);
	}

	// Attach a resize handler to a panel's bottom-right grip button so the GM can resize it on both axes.
	protected void MakeResizable(string handleName, string panelName, float minW, float minH)
	{
		if (!m_wRoot)
			return;
		ButtonWidget handle = ButtonWidget.Cast(m_wRoot.FindAnyWidget(handleName));
		Widget panel = m_wRoot.FindAnyWidget(panelName);
		if (!handle || !panel)
		{
			Print(string.Format("[DCO-GM] resizable wire skipped (handle=%1 / panel=%2 found=%3,%4)",
				handleName, panelName, handle != null, panel != null), LogLevel.WARNING);
			return;
		}
		DCO_GMResizable r = new DCO_GMResizable(panel, minW, minH);
		handle.AddHandler(r);
		m_Resizables.Insert(r);
	}

	// Fast-poll peel: hide the engine GM chrome and the three player-HUD feedback slots replaced by Bifrost.
	protected void PeelVanillaPoll()
	{
		m_PeelAttempts++;
		Widget ws = GetGame().GetWorkspace();
		if (!ws)
			return;

		array<string> targets = {
			"Mode_Edit_Element_Top",
			"Mode_Edit_Element_Right",	// entity browser grid + scenario warning.
			"Mode_Edit_Element_Bottom",	// placing quick-bar + command bar + cycle-waypoints.
			"Mode_Edit_Element_Left",
			"BottomShade",	// pure visual gradient.
			"Slot_Notifications",
			"Slot_Hints",	// native full hint cards; Bifrost help lives in the INFO panel.
			"Slot_AvailableActions"	// native keybind strip; actions remain active, only their duplicate HUD is hidden.
		};
		int found = 0;
		foreach (string nm : targets)
		{
			if (ws.FindAnyWidget(nm))
				found++;
			m_VanillaHide.Hide(ws, nm);
		}

		if (found >= targets.Count() || m_PeelAttempts >= 30)
		{
			GetGame().GetCallqueue().Remove(PeelVanillaPoll);
			Print(string.Format("[DCO-GM] vanilla peel done after %1 polls: %2/%3 native HUD containers hidden",
				m_PeelAttempts, found, targets.Count()), LogLevel.NORMAL);
		}
	}

	// Top-bar EDIT tab clicked: toggle the left EDIT column.
	bool ToggleEditPanel()
	{
		DCO_GMTheme.Get().SetElementEnabled(DCO_GMTheme.UI_EDIT, !m_bEditShown, m_wRoot, false);
		return m_bEditShown;
	}

	bool ToggleCreatePanel()
	{
		// Session-only, same reasoning as ToggleEditPanel.
		DCO_GMTheme.Get().SetElementEnabled(DCO_GMTheme.UI_CREATE, !m_bCreateShown, m_wRoot, false);
		return m_bCreateShown;
	}

	// The top clock is an editing surface, not decorative text.
	void OpenScenarioTimeAndDate()
	{
		if (m_Scenario)
			m_Scenario.OpenTimeAndDate();
	}

	// Bottom utility rail: the top bar remains navigation/context, while this rail is the fast workspace control.
	void OnUtilityButton(Widget w)
	{
		if (!w)
			return;
		string name = w.GetName();
		if (name == "DCO_QuickEditBtn")
			ToggleEditPanel();
		else if (name == "DCO_QuickDeployBtn")
		{
	// Reuses the last valid placement.
			if (!m_CreatePanel || !m_CreatePanel.RepeatLastPlacement())
			{
				DCO_GMTheme.Get().SetElementEnabled(DCO_GMTheme.UI_CREATE, true, m_wRoot, false);
				if (m_CreatePanel)
					m_CreatePanel.ShowRedeployPrompt();
			}
		}
		else if (name == "DCO_QuickCleanBtn")
		{
			bool show = !m_bEditShown && !m_bCreateShown;
			DCO_GMTheme.Get().SetElementEnabled(DCO_GMTheme.UI_EDIT, show, m_wRoot, false);
			DCO_GMTheme.Get().SetElementEnabled(DCO_GMTheme.UI_CREATE, show, m_wRoot, false);
		}
		else if (name == "DCO_LayoutResetBtn")
			ResetPanelLayout();
	}

	// Restores saved panel geometry and visibility defaults.
	void ResetPanelLayout()
	{
		DCO_GMTheme.Get().ClearPanelGeoms();
		DCO_GMTheme.Get().ResetElementVisibility(m_wRoot);
		m_bEditShown = true;
		m_bCreateShown = true;
		if (m_CreatePanel)
			m_CreatePanel.Show(true);
		if (m_wRoot)
		{
			Widget tree = m_wRoot.FindAnyWidget("DCO_EditTree");
			if (tree)
				tree.SetVisible(true);
		}
		ApplyLayout();	// ApplySavedGeom no-ops now the geoms are cleared - every panel lands on its PinBox default.
		if (m_TopBar)
			m_TopBar.SyncTabState(true, true);
		Print("[DCO-GM] panel layout RESET to defaults", LogLevel.NORMAL);
	}

	protected void Teardown()
	{
		RemoveMenuActionListeners();
		s_iPauseSuppressedUntil = 0;
		RemoveMasterHideListener();
		DCO_GMTheme.Get().ClearMasterHide();	// restore parked world-cue state before the shell lifetime ends.
		GetGame().GetCallqueue().Remove(PeelVanillaPoll);	// stop the peel poll if it's still running.
		GetGame().GetCallqueue().Remove(PollViewport);	// stop workspace resize polling before the root is removed.
		foreach (DCO_GMDraggable d : m_Draggables)	// stop any in-progress drag poll before the widgets go away.
			d.StopDrag();
		m_Draggables.Clear();
		foreach (DCO_GMResizable r : m_Resizables)	// likewise stop any in-progress resize poll.
			r.StopResize();
		m_Resizables.Clear();
		DCO_GMDraggable.ResetRaise();	// next shell build starts its raise-on-grab z counter from zero.
		// Release the server freeze when the GM leaves the editor.
		DCO_GMPauseServer.RoutePause(EDCO_PauseScope.ALL_AI, 0, false);
		m_ResetHandler = null;	// its button dies with the root; drop our keep-alive ref.
		m_VanillaHide.RestoreAll();	// un-hide any engine surfaces we hid.
		DCO_GMTutorial.Get().Shutdown();	// drops its ESC listener before the shell root takes its widgets away.
		if (m_TopBar)
		{
			m_TopBar.Shutdown();
			m_TopBar = null;
		}
		if (m_Scenario)
		{
			m_Scenario.Shutdown();
			m_Scenario = null;
		}
		if (m_Options)
		{
			m_Options.Shutdown();
			m_Options = null;
		}
		if (m_Awareness)	// unsubscribe the cue before the pillar it draws on stops.
		{
			m_Awareness.Stop();
			m_Awareness = null;
		}
		// Resets state that outlives the shell.
		DCO_GMAttach.DetachAll();	// Releases attached objects before their controls close.
		DCO_GMGizmo.SetPreciseMode(false);
		if (m_PreciseBar)	// unsubscribe from the gizmo's precise-mode invoker before the gizmo stops.
		{
			m_PreciseBar.Shutdown();
			m_PreciseBar = null;
		}
		if (m_NotifFeed)	// unsubscribes + restores the engine notification log.
		{
			m_NotifFeed.Shutdown();
			m_NotifFeed = null;
		}
		if (m_ChatFeed)	// stops the poll + restores the engine chat panel.
		{
			m_ChatFeed.Shutdown();
			m_ChatFeed = null;
		}
		DCO_GMGizmo.Get().Stop();
		if (m_GizmoPanel)	// AFTER the gizmo stops feeding it, so the last write can never land on a torn-down panel.
		{
			m_GizmoPanel.Shutdown();
			m_GizmoPanel = null;
		}
		if (m_OverlayPanel)
		{
			m_OverlayPanel.Shutdown();
			m_OverlayPanel = null;
		}
		if (m_Nametags)
		{
			m_Nametags.Shutdown();
			m_Nametags = null;
		}
		if (m_Render)
		{
			m_Render.Stop();
			m_Render = null;
		}
		if (m_Bridge)
		{
			m_Bridge.Shutdown();
			m_Bridge = null;
		}
		DCO_ArsenalAccessPlacement.Get().Shutdown();
		DCO_GMTacticsFlow.Get().Shutdown();	// disarm any in-flight placement before its widgets die.
		DCO_GMTacticsPanel.Get().Shutdown();
		DCO_GMArsenalPanel.Get().Shutdown();
		DCO_GMGameplayPanel.Get().Shutdown();	// stops its session-time timer before the shell root takes its widgets.
		if (m_OrdersPanel)
		{
			m_OrdersPanel.Shutdown();
			m_OrdersPanel = null;
		}
		m_Menu = null;
		if (m_EditTree)
		{
			m_EditTree.Shutdown();
			m_EditTree = null;
		}
		if (m_PlaceConfirm)
		{
			m_PlaceConfirm.Stop();
			m_PlaceConfirm = null;
		}
		if (m_CreatePanel)
		{
			m_CreatePanel.Shutdown();
			m_CreatePanel = null;
		}
		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
		m_bBuilt = false;
		if (s_Instance == this)
			s_Instance = null;
		DCO_GMHover.Clear();
		Print("[DCO-GM] shell TORN DOWN", LogLevel.NORMAL);
	}
}

class DCO_GMLayoutResetHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMUIController m_Owner;

	void DCO_GMLayoutResetHandler(DCO_GMUIController owner)
	{
		m_Owner = owner;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Owner)
			return false;
		m_Owner.OnUtilityButton(w);
		return true;
	}
}
