// Bifrost GM top-bar binder.

class DCO_TopBarButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMTopBarComponent m_Owner;

	void DCO_TopBarButtonHandler(DCO_GMTopBarComponent owner)
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

class DCO_GMTopBarComponent
{
	protected Widget m_wRoot;
	protected TextWidget m_wClock;
	protected TextWidget m_wCompass;
	protected TextWidget m_wCompassHeading;
	protected ref array<TextWidget> m_CompassTicks = {};
	protected TextWidget m_wSelection;
	protected TextWidget m_wWorldState;
	protected ImageWidget m_wWorldStateIco;
	protected ImageWidget m_wWorldStateRail;
	protected ButtonWidget m_btnEdit;
	protected ButtonWidget m_btnCreate;
	protected ButtonWidget m_btnPrecise;
	protected ButtonWidget m_btnClock;	// opens Scenario Settings directly at Time and date.
	protected ref DCO_TopBarButtonHandler m_Handler;
	protected DCO_GMUIController m_Controller;

	void Init(Widget root, DCO_GMUIController controller = null)
	{
		if (!root)
			return;
		m_wRoot = root;
		m_Controller = controller;
		m_Handler = new DCO_TopBarButtonHandler(this);

		m_wClock    = TextWidget.Cast(root.FindAnyWidget("DCO_ClockText"));
		m_wCompass  = TextWidget.Cast(root.FindAnyWidget("DCO_CompassText"));
		m_wCompassHeading = TextWidget.Cast(root.FindAnyWidget("DCO_CompassHeading"));
		for (int compassIndex = 0; compassIndex < 5; compassIndex++)
		{
			string tickName = "DCO_CompassTick" + compassIndex.ToString();
			if (compassIndex == 2)
				tickName = "DCO_CompassText";
			m_CompassTicks.Insert(TextWidget.Cast(root.FindAnyWidget(tickName)));
		}
		m_wSelection = TextWidget.Cast(root.FindAnyWidget("DCO_TopSelection"));
		m_wWorldState = TextWidget.Cast(root.FindAnyWidget("DCO_TopWorldState"));
		m_wWorldStateIco = ImageWidget.Cast(root.FindAnyWidget("DCO_TopWorldStateIco"));
		m_wWorldStateRail = ImageWidget.Cast(root.FindAnyWidget("DCO_TopWorldRail"));
		m_btnEdit   = Bind("DCO_Tab_Edit");
		m_btnCreate = Bind("DCO_Tab_Create");
		m_btnPrecise = Bind("DCO_Btn_Precise");
		m_btnClock = Bind("DCO_ClockButton");

		ApplyTheme();
		SyncTabState(true, true);	// both columns start visible - each tab lights while its column is shown.
		DCO_GMGizmo.SetPreciseMode(false);	// every GM session starts in engine place/drag.
		SetPreciseActive(false);

		GetGame().GetCallqueue().CallLater(UpdateClock, 1000, true);
		GetGame().GetCallqueue().CallLater(UpdateCompass, 250, true);
		GetGame().GetCallqueue().CallLater(UpdateContext, 250, true);
		UpdateClock();
		UpdateCompass();
		UpdateContext();
	}

	protected ButtonWidget Bind(string name)
	{
		ButtonWidget b = ButtonWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (b)
			b.AddHandler(m_Handler);
		return b;
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(UpdateClock);
		GetGame().GetCallqueue().Remove(UpdateCompass);
		GetGame().GetCallqueue().Remove(UpdateContext);
	}

	bool OnButton(Widget w)
	{
		if (w == m_btnEdit)
		{
			if (m_Controller)
				SetTabLit("DCO_Tab_Edit_Label", "DCO_Tab_Edit_Icon", m_Controller.ToggleEditPanel());
			return true;
		}
		if (w == m_btnCreate)
		{
			if (m_Controller)
				SetTabLit("DCO_Tab_Create_Label", "DCO_Tab_Create_Icon", m_Controller.ToggleCreatePanel());
			return true;
		}
		if (w == m_btnPrecise)
		{
			bool on = DCO_GMGizmo.TogglePreciseMode();
			SetPreciseActive(on);
			return true;
		}
		if (w == m_btnClock)
		{
			if (m_Controller)
				m_Controller.OpenScenarioTimeAndDate();
			return true;
		}
		return false;
	}

	protected void SetPreciseActive(bool on)
	{
		TextWidget lbl = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_Btn_Precise_Label"));
		ImageWidget icon = ImageWidget.Cast(m_wRoot.FindAnyWidget("DCO_Btn_Precise_Icon"));
		if (!lbl || !icon)
			return;
		DCO_GMTheme theme = DCO_GMTheme.Get();
		if (on)
		{
			lbl.SetColor(theme.m_AccentColor);
			lbl.SetOpacity(1.0);
			icon.SetColor(theme.m_AccentColor);
			icon.SetOpacity(1.0);
		}
		else
		{
			lbl.SetColor(theme.m_TextColor);
			lbl.SetOpacity(0.55);
			icon.SetColor(theme.m_TextColor);
			icon.SetOpacity(0.55);
		}
	}

	// Light a tab amber while its column is SHOWN, dim grey while hidden.
	protected void SetTabLit(string labelName, string iconName, bool shown)
	{
		TextWidget lbl = TextWidget.Cast(m_wRoot.FindAnyWidget(labelName));
		ImageWidget icon = ImageWidget.Cast(m_wRoot.FindAnyWidget(iconName));
		if (!lbl || !icon)
			return;
		DCO_GMTheme theme = DCO_GMTheme.Get();
		if (shown)
		{
			lbl.SetColor(theme.m_AccentColor);
			lbl.SetOpacity(1.0);
			icon.SetColor(theme.m_AccentColor);
			icon.SetOpacity(1.0);
		}
		else
		{
			lbl.SetColor(theme.m_TextColor);
			lbl.SetOpacity(0.55);
			icon.SetColor(theme.m_TextColor);
			icon.SetOpacity(0.55);
		}
	}

	// Reflect both columns' visibility on the tabs at once.
	void SyncTabState(bool editShown, bool createShown)
	{
		SetTabLit("DCO_Tab_Edit_Label", "DCO_Tab_Edit_Icon", editShown);
		SetTabLit("DCO_Tab_Create_Label", "DCO_Tab_Create_Icon", createShown);
	}

	protected void ApplyTheme()
	{
		DCO_GMTheme theme = DCO_GMTheme.Get();
		ImageWidget bg = ImageWidget.Cast(m_wRoot.FindAnyWidget("DCO_TopBarBg"));
		if (bg)
			bg.SetColor(theme.m_PanelColor);
		if (m_wClock)
			m_wClock.SetColor(theme.m_AccentColor);
		if (m_wCompass)
			m_wCompass.SetColor(theme.m_TextColor);
		if (m_wCompassHeading)
			m_wCompassHeading.SetColor(theme.m_AccentColor);
		for (int compassIndex = 0; compassIndex < m_CompassTicks.Count(); compassIndex++)
		{
			if (!m_CompassTicks[compassIndex])
				continue;
			if (compassIndex == 2)
				m_CompassTicks[compassIndex].SetColor(theme.m_TextColor);
			else
				m_CompassTicks[compassIndex].SetColor(theme.m_MutedColor);
		}
		if (m_wSelection)
			m_wSelection.SetColor(theme.m_TextColor);
	}

	void UpdateClock()
	{
		if (!m_wClock)
			return;
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return;
		TimeAndWeatherManagerEntity twm = world.GetTimeAndWeatherManager();
		if (!twm)
			return;
		int h, m, s;
		twm.GetHoursMinutesSeconds(h, m, s);
		m_wClock.SetText(FmtTwo(h) + ":" + FmtTwo(m));
	}

	protected string FmtTwo(int v)
	{
		if (v < 10)
			return "0" + v.ToString();
		return v.ToString();
	}

	protected string FmtThree(int v)
	{
		if (v < 10)
			return "00" + v.ToString();
		if (v < 100)
			return "0" + v.ToString();
		return v.ToString();
	}

	void UpdateCompass()
	{
		if (!m_wCompass)
			return;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		vector transform[4];
		world.GetCurrentCamera(transform);
		float yaw = -Math3D.MatrixToAngles(transform)[0];
		while (yaw < 0)
			yaw += 360;
		while (yaw >= 360)
			yaw -= 360;

		int heading = Math.Round(yaw);
		if (heading >= 360)
			heading = 0;
		if (m_wCompassHeading)
			m_wCompassHeading.SetText(FmtThree(heading));

		int center = Math.Round(yaw / 45.0);
		if (center >= 8)
			center = 0;
		for (int i = 0; i < m_CompassTicks.Count(); i++)
		{
			if (m_CompassTicks[i])
				m_CompassTicks[i].SetText(CardinalAt(center + i - 2));
		}
		m_wCompass.SetText(CardinalAt(center));
	}

	void UpdateContext()
	{
		if (m_wSelection)
		{
			set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
			SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
			m_wSelection.SetText(string.Format("SELECTION  %1", selected.Count()));
		}
		if (m_wWorldState)
		{
			bool hardPaused = Replication.IsServer() && DCO_GMPauseCore.IsWorldPaused();
			bool frozen;
			string stateText;
			Color stateColor;
			if (Replication.IsServer())
				frozen = DCO_GMPauseCore.Get().IsActive();
			else
				frozen = DCO_GMPausePresentationState.IsPaused();
			if (hardPaused)
			{
				stateText = "PAUSED";
				stateColor = Color.FromRGBA(235, 185, 80, 255);
			}
			else if (frozen)
			{
				stateText = "FROZEN";
				stateColor = Color.FromRGBA(235, 185, 80, 255);
			}
			else
			{
				stateText = "ACTIVE";
				stateColor = Color.FromInt(DCO_GMTheme.SEM_FRIENDLY);
			}
			m_wWorldState.SetText(stateText);
			m_wWorldState.SetColor(stateColor);
			if (m_wWorldStateIco)
				m_wWorldStateIco.SetColor(stateColor);
			if (m_wWorldStateRail)
				m_wWorldStateRail.SetColor(stateColor);
		}
	}

	protected string YawToCardinal(float yaw)
	{
		float y = yaw;
		while (y < 0)
			y += 360;
		while (y >= 360)
			y -= 360;
		int idx = Math.Round(y / 45.0);
		if (idx >= 8)
			idx = 0;
		array<string> dirs = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
		return dirs[idx];
	}

	protected string CardinalAt(int index)
	{
		int wrapped = index;
		while (wrapped < 0)
			wrapped += 8;
		while (wrapped >= 8)
			wrapped -= 8;
		array<string> dirs = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
		return dirs[wrapped];
	}
}
