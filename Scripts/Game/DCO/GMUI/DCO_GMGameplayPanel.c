// DCO GM Gameplay panel controller.
class DCO_GameplayBtnHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMGameplayPanel m_Ctrl;
	void DCO_GameplayBtnHandler(DCO_GMGameplayPanel c) { m_Ctrl = c; }
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Ctrl)
			return m_Ctrl.OnButton(w);
		return false;
	}
}

class DCO_GMGameplayPanel
{
	protected static ref DCO_GMGameplayPanel s_Instance;
	static DCO_GMGameplayPanel Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMGameplayPanel();
		return s_Instance;
	}

	protected Widget m_wRoot;	// the GM shell root - the gameplay controls live INSIDE the EDIT column.
	protected TextWidget m_wSessionTime;
	protected TextWidget m_wClockValue;
	protected TextWidget m_wStatus;
	protected ref DCO_GameplayBtnHandler m_Handler;

	protected ButtonWidget m_btnPause, m_btnResume;
	protected ButtonWidget m_scopeSelected, m_scopeAI;
	protected ButtonWidget m_chkAI, m_chkPhysics;

	// Clock speed pills, parallel to CLOCK_MULTS.
	protected ref array<ButtonWidget> m_ClkBtns = {};
	protected static const ref array<string> CLOCK_NAMES = {"DCO_Clk_STOP", "DCO_Clk_1", "DCO_Clk_4", "DCO_Clk_12", "DCO_Clk_60"};
	protected static const ref array<float>  CLOCK_MULTS = {0, 1, 4, 12, 60};
	protected int m_ClockIdx = 1;	// 1x real time.

	// UI state.
	protected int  m_Scope = EDCO_PauseScope.ALL_AI;
	protected bool m_AspAI = true;
	protected bool m_AspPhysics = true;

	// Themed - follows the GM's accent-hue choice, one consistent accent via the theme.
	protected Color ColOn()  { return DCO_GMTheme.Get().m_AccentColor; }
	protected Color ColOff() { return DCO_GMTheme.Get().m_MutedColor; }

	protected static const int PLATE_REST = 0xFF292E36;
	protected static const int TEXT_ON_ACCENT = 0xFF0B0D10;

	void Init(Widget root)
	{
		if (!root)
			return;
		m_wRoot = root;
		m_Handler = new DCO_GameplayBtnHandler(this);

		m_wSessionTime = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_SessionTimeText"));
		m_wClockValue  = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ClockValue"));
		m_wStatus      = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_PauseStatus"));

		m_btnPause  = Bind("DCO_Btn_Pause");
		m_btnResume = Bind("DCO_Btn_Resume");
		m_scopeSelected = Bind("DCO_Btn_ScopeSelected");
		m_scopeAI       = Bind("DCO_Btn_ScopeAI");
		m_chkAI      = Bind("DCO_Chk_AI");
		m_chkPhysics = Bind("DCO_Chk_Physics");

		m_ClkBtns.Clear();
		foreach (string cn : CLOCK_NAMES)
			m_ClkBtns.Insert(Bind(cn));

		RefreshChecks();
		SetClockValueLabel(m_ClockIdx);
		UpdatePauseStatus();
		DCO_GMTheme.Get().OnThemeChanged.Insert(RefreshChecks);	// live re-tint when the GM changes the accent hue.
		GetGame().GetCallqueue().Remove(TickTime);	// defensive: never double-schedule on a re-mount.
		GetGame().GetCallqueue().CallLater(TickTime, 1000, true);	// session-time + status readout.
	}

	protected ButtonWidget Bind(string name)
	{
		ButtonWidget b = ButtonWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (b)
			b.AddHandler(m_Handler);
		else
			Print("[DCO-GM] gameplay panel: missing widget " + name, LogLevel.WARNING);
		return b;
	}

	bool OnButton(Widget w)
	{
		if (w == m_btnPause)  { DoPause(true);  return true; }
		if (w == m_btnResume) { DoPause(false); return true; }

		if (w == m_scopeSelected) { m_Scope = EDCO_PauseScope.SELECTED;    RefreshChecks(); return true; }
		if (w == m_scopeAI)       { m_Scope = EDCO_PauseScope.ALL_AI;      RefreshChecks(); return true; }

		if (w == m_chkAI)      { m_AspAI = !m_AspAI;           RefreshChecks(); return true; }
		if (w == m_chkPhysics) { m_AspPhysics = !m_AspPhysics; RefreshChecks(); return true; }

		for (int i = 0; i < m_ClkBtns.Count(); i++)
		{
			if (w == m_ClkBtns[i])
			{
				SetClock(i);
				return true;
			}
		}
		return false;
	}

	protected void DoPause(bool on)
	{
		int mask = 0;
		if (m_AspAI)
			mask |= EDCO_PauseAspect.AI;
		if (m_AspPhysics)
			mask |= EDCO_PauseAspect.PHYSICS;

		DCO_GMPauseServer.RoutePause(m_Scope, mask, on);

		UpdatePauseStatus();
		GetGame().GetCallqueue().Remove(UpdatePauseStatus);
		GetGame().GetCallqueue().CallLater(UpdatePauseStatus, 150, false);
	}

	protected void UpdatePauseStatus()
	{
		int frozen = 0;
		bool worldPaused = false;
		bool authority = Replication.IsServer();
		bool pauseActive = DCO_GMPausePresentationState.IsPaused();
		if (authority)
		{
			frozen = DCO_GMPauseCore.Get().GetFrozenCount();
			worldPaused = DCO_GMPauseCore.IsWorldPaused();
			pauseActive = worldPaused || frozen > 0;
		}

		if (m_wStatus)
		{
			if (!authority && pauseActive)
			{
				m_wStatus.SetText("PAUSE ACTIVE ON SERVER");
				m_wStatus.SetColor(ColOn());
			}
			else if (worldPaused)
			{
				m_wStatus.SetText("WORLD PAUSED - game time stopped");
				m_wStatus.SetColor(ColOn());
			}
			else if (frozen > 0)
			{
				m_wStatus.SetText(string.Format("FROZEN - %1 held", frozen));
				m_wStatus.SetColor(ColOn());
			}
			else
			{
				m_wStatus.SetText("World running");
				m_wStatus.SetColor(DCO_GMTheme.Get().m_DisabledColor);
			}
		}

		// PAUSE reads as engaged while the world is paused or anything is held.
		TintLabel(m_btnPause, "DCO_Btn_Pause_Label", pauseActive);
	}

	protected void SetClock(int idx)
	{
		if (idx < 0 || idx >= CLOCK_MULTS.Count())
			return;
		m_ClockIdx = idx;
		SetClockValueLabel(idx);
		RefreshChecks();
		DCO_GMPauseServer.RouteClock(CLOCK_MULTS[idx]);
	}

	protected void SetClockValueLabel(int idx)
	{
		if (!m_wClockValue)
			return;
		if (idx <= 0)
			m_wClockValue.SetText("STOPPED");
		else
			m_wClockValue.SetText(CLOCK_MULTS[idx].ToString(1, 0) + "x");
	}

	// Tint the active scope + clock pills + ticked aspects.
	protected void RefreshChecks()
	{
		TintLabel(m_scopeSelected, "DCO_Btn_ScopeSelected_Label", m_Scope == EDCO_PauseScope.SELECTED);
		TintLabel(m_scopeAI,       "DCO_Btn_ScopeAI_Label",       m_Scope == EDCO_PauseScope.ALL_AI);

		for (int i = 0; i < m_ClkBtns.Count(); i++)
			TintLabel(m_ClkBtns[i], CLOCK_NAMES[i] + "_Label", i == m_ClockIdx);

		SetCheckLabel(m_chkAI,      "DCO_Chk_AI_Label",      "AI",             m_AspAI);
		SetCheckLabel(m_chkPhysics, "DCO_Chk_Physics_Label", "PHYSICS",        m_AspPhysics);
		UpdatePauseStatus();
	}

	protected void TintLabel(Widget btn, string labelName, bool active)
	{
		if (!btn)
			return;
		TextWidget t = TextWidget.Cast(btn.FindAnyWidget(labelName));
		if (!t)
			return;

		string plateName = labelName;
		plateName.Replace("_Label", "_Bg");
		Widget plate = btn.FindAnyWidget(plateName);

		if (active)
		{
			if (plate)
				plate.SetColor(ColOn());
			t.SetColorInt(TEXT_ON_ACCENT);
		}
		else
		{
			if (plate)
				plate.SetColorInt(PLATE_REST);
			t.SetColor(ColOff());
		}
	}

	protected void SetCheckLabel(Widget btn, string labelName, string caption, bool ticked)
	{
		if (!btn)
			return;
		TextWidget t = TextWidget.Cast(btn.FindAnyWidget(labelName));
		if (!t)
			return;
		t.SetText(caption);
		if (ticked)
			t.SetColor(ColOn());
		else
			t.SetColor(ColOff());

		// Real tick glyph.
		string tickName = labelName;
		tickName.Replace("_Label", "_Tick");
		ImageWidget tick = ImageWidget.Cast(btn.FindAnyWidget(tickName));
		if (!tick)
			return;
		tick.SetColor(ColOn());
		if (ticked)
			tick.SetOpacity(1.0);
		else
			tick.SetOpacity(0.0);
	}

	protected void TickTime()
	{
		UpdatePauseStatus();	// keeps the freeze readout truthful even when state changes outside this panel.
		if (!m_wSessionTime)
			return;
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return;
		TimeAndWeatherManagerEntity twm = world.GetTimeAndWeatherManager();
		if (!twm)
			return;
		int h, m, s;
		twm.GetHoursMinutesSeconds(h, m, s);
		m_wSessionTime.SetText("In-game " + FmtTwo(h) + ":" + FmtTwo(m));
	}

	protected string FmtTwo(int v)
	{
		if (v < 10)
			return "0" + v.ToString();
		return v.ToString();
	}

	void Shutdown()
	{
		DCO_GMTheme.Get().OnThemeChanged.Remove(RefreshChecks);
		GetGame().GetCallqueue().Remove(TickTime);
		GetGame().GetCallqueue().Remove(UpdatePauseStatus);
	}
}
