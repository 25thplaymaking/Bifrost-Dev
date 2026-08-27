
class DCO_PreciseBarButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMPreciseBar m_Owner;

	void DCO_PreciseBarButtonHandler(DCO_GMPreciseBar owner)
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

class DCO_GMPreciseBar
{
	protected Widget m_wRoot;
	protected Widget m_wTools;
	protected ButtonWidget m_btnMove;
	protected ButtonWidget m_btnRotate;
	protected ButtonWidget m_btnAttach;
	protected ButtonWidget m_btnDetach;
	protected ButtonWidget m_btnSim;
	protected ButtonWidget m_btnSpace;
	protected ButtonWidget m_btnSnap;
	protected ButtonWidget m_btnSurf;
	protected ref DCO_PreciseBarButtonHandler m_Handler;
	protected ref DCO_GMComponentPanel m_SimPanel;	// SIM OPTIONS panel this button opens.

	void Init(Widget root)
	{
		if (!root)
			return;
		m_wRoot = root;
		m_Handler = new DCO_PreciseBarButtonHandler(this);

		m_wTools   = root.FindAnyWidget("DCO_PreciseTools");
		m_btnMove  = Bind("DCO_Btn_PreciseMove");
		m_btnRotate = Bind("DCO_Btn_PreciseRotate");
		m_btnAttach = Bind("DCO_Btn_PreciseAttach");
		m_btnDetach = Bind("DCO_Btn_PreciseDetach");
		m_btnSim   = Bind("DCO_Btn_PreciseSim");
		m_btnSpace = Bind("DCO_Btn_GizmoSpace");
		m_btnSnap  = Bind("DCO_Btn_GizmoSnap");
		m_btnSurf  = Bind("DCO_Btn_GizmoSurf");

		m_SimPanel = new DCO_GMComponentPanel();
		m_SimPanel.Init(root);

		DCO_GMGizmo.GetOnPreciseChanged().Insert(OnPreciseChanged);

		// Seed from the live state rather than assuming OFF, so a rebuilt GM UI matches whatever the gizmo holds.
		OnPreciseChanged(DCO_GMGizmo.IsPreciseModeActive());

		Print(string.Format("[DCO-GM] precise toolbar bound (group=%1 move=%2 rotate=%3 sim=%4 space=%5 snap=%6 surf=%7)",
			m_wTools != null, m_btnMove != null, m_btnRotate != null, m_btnSim != null,
			m_btnSpace != null, m_btnSnap != null, m_btnSurf != null), LogLevel.NORMAL);
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
		DCO_GMGizmo.GetOnPreciseChanged().Remove(OnPreciseChanged);
		if (m_SimPanel)
		{
			m_SimPanel.Shutdown();	// stops its refresh poll — a stray CallLater would outlive the GM UI.
			m_SimPanel = null;
		}
		// Break the handler cycle during teardown.
		m_Handler = null;
		m_btnMove = null;
		m_btnRotate = null;
		m_btnSim = null;
		m_btnSpace = null;
		m_btnSnap = null;
		m_btnSurf = null;
		m_wTools = null;
		m_wRoot = null;
	}

	// Keeps the toolbar aligned with precise mode.
	protected void OnPreciseChanged(bool on)
	{
		if (m_wTools)
			m_wTools.SetVisible(on);
		if (on)
		{
			RefreshModeTint();
			return;
		}
		if (m_SimPanel)
			m_SimPanel.SetOpen(false);
	}

	bool OnButton(Widget w)
	{
		if (w == m_btnMove)
		{
			DCO_GMGizmo.Get().SetMode(EDCO_GizmoMode.MOVE);
			RefreshModeTint();
			return true;
		}
		if (w == m_btnRotate)
		{
			DCO_GMGizmo.Get().SetMode(EDCO_GizmoMode.ROTATE);
			RefreshModeTint();
			return true;
		}
		if (w == m_btnAttach)
		{
			// Arm attach: the next world click bonds the selected object to whatever is under the cursor.
			DCO_GMAttach.SetArmed(!DCO_GMAttach.IsArmed());
			RefreshModeTint();
			return true;
		}
		if (w == m_btnDetach)
		{
			SCR_EditableEntityComponent t = DCO_GMGizmo.Get().GetTarget();
			if (t)
				DCO_GMAttach.DetachSelected(t.GetOwner());
			return true;
		}
		if (w == m_btnSim)
		{
			if (m_SimPanel)
			{
				m_SimPanel.Toggle();
				RefreshModeTint();	// the SIM icon lights while its panel is open.
			}
			return true;
		}
		if (w == m_btnSpace)
		{
			DCO_GMGizmo.Get().ToggleSpace();
			RefreshModeTint();
			return true;
		}
		if (w == m_btnSnap)
		{
			DCO_GMGizmo.Get().ToggleSnap();
			RefreshModeTint();
			return true;
		}
		if (w == m_btnSurf)
		{
			DCO_GMGizmo.Get().ToggleSurfaceSnap();
			RefreshModeTint();
			return true;
		}
		return false;
	}

	protected void RefreshModeTint()
	{
		DCO_GMGizmo giz = DCO_GMGizmo.Get();
		bool rotate = giz.GetMode() == EDCO_GizmoMode.ROTATE;
		TintIcon("DCO_Btn_PreciseMove_Icon",   !rotate);
		TintIcon("DCO_Btn_PreciseRotate_Icon",  rotate);
		TintIcon("DCO_Btn_PreciseSim_Icon",     m_SimPanel && m_SimPanel.IsOpen());
		TintLabel("DCO_Btn_PreciseAttach_Label", DCO_GMAttach.IsArmed());	// lit while waiting for the target click.

		bool local = giz.GetSpace() == EDCO_GizmoSpace.LOCAL;
		if (local)
			SetLabel("DCO_Btn_GizmoSpace_Label", "LOCAL");
		else
			SetLabel("DCO_Btn_GizmoSpace_Label", "WORLD");
		TintLabel("DCO_Btn_GizmoSpace_Label", local);

		// The snap button carries its increment, so the GM never has to guess what "SNAP" means in metres.
		float step = giz.GetMoveStep();
		SetLabel("DCO_Btn_GizmoSnap_Label", string.Format("SNAP %1", step.ToString(-1, 2)));
		TintLabel("DCO_Btn_GizmoSnap_Label", giz.IsSnapOn());

		TintLabel("DCO_Btn_GizmoSurf_Label", giz.IsSurfaceSnapOn());
	}

	protected void TintIcon(string name, bool active)
	{
		if (!m_wRoot)
			return;
		ImageWidget img = ImageWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!img)
			return;
		DCO_GMTheme theme = DCO_GMTheme.Get();
		if (active)
		{
			img.SetColor(theme.m_AccentColor);
			img.SetOpacity(1.0);
		}
		else
		{
			img.SetColor(theme.m_TextColor);
			img.SetOpacity(0.55);
		}
	}

	// The text-button twin of TintIcon, so the modifier toggles read at a glance exactly like the icon buttons.
	protected void TintLabel(string name, bool active)
	{
		if (!m_wRoot)
			return;
		TextWidget t = TextWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!t)
			return;
		DCO_GMTheme theme = DCO_GMTheme.Get();
		if (active)
		{
			t.SetColor(theme.m_AccentColor);
			t.SetOpacity(1.0);
		}
		else
		{
			t.SetColor(theme.m_TextColor);
			t.SetOpacity(0.55);
		}
	}

	protected void SetLabel(string name, string text)
	{
		if (!m_wRoot)
			return;
		TextWidget t = TextWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (t)
			t.SetText(text);
	}
}
