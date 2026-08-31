// SIM OPTIONS panel — the precise-mode component toggles for whichever object the gizmo is currently targeting.

// One per pill; carries its row index so OnClick maps straight to the right component toggle.
class DCO_SimPillHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMComponentPanel m_Owner;
	protected int m_Row;

	void DCO_SimPillHandler(DCO_GMComponentPanel owner, int row)
	{
		m_Owner = owner;
		m_Row = row;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnRowClicked(m_Row);
		return false;
	}
}

// One per stance button.
class DCO_SimStanceHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMComponentPanel m_Owner;
	protected int m_Value;

	void DCO_SimStanceHandler(DCO_GMComponentPanel owner, int value)
	{
		m_Owner = owner;
		m_Value = value;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnStanceClicked(m_Value);
		return false;
	}
}

class DCO_SimCloseHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMComponentPanel m_Owner;

	void DCO_SimCloseHandler(DCO_GMComponentPanel owner)
	{
		m_Owner = owner;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Owner)
			return false;
		m_Owner.SetOpen(false);
		return true;
	}
}

class DCO_GMComponentPanel
{
	protected static ref array<string> s_StanceButtons;

	protected static array<string> StanceButtons()
	{
		if (!s_StanceButtons)
			s_StanceButtons = {"DCO_SimStance_STAND", "DCO_SimStance_CROUCH", "DCO_SimStance_PRONE"};
		return s_StanceButtons;
	}

	static const int ROW_SIM       = 0;
	static const int ROW_DAMAGE    = 1;
	static const int ROW_AI        = 2;
	static const int ROW_COLLISION = 3;
	static const int ROW_WEAPON    = 4;
	static const int ROW_TRACER    = 5;
	static const int ROW_COUNT     = 6;

	static const int REFRESH_MS = 250;

	// Pill colours that are NOT the accent.
	static const int PILL_OFF_BG   = 0xFF2E333B;	// dark slate — "off".
	static const int PILL_NA_BG    = 0xFF1B1F25;	// darker still — "not applicable".
	static const int PILL_ON_TEXT  = 0xFF0B0D10;	// near-black, reads on the bright accent fill.
	static const int PILL_OFF_TEXT = 0xFFE6EDF2;	// body text.

	protected Widget m_wRoot;
	protected Widget m_wPanel;
	protected TextWidget m_wTargetLabel;
	protected ButtonWidget m_btnClose;

	protected ref array<TextWidget>  m_Labels    = {};
	protected ref array<ImageWidget> m_PillBgs   = {};
	protected ref array<TextWidget>  m_PillTexts = {};
	protected ref array<ref DCO_SimPillHandler> m_PillHandlers = {};
	protected ref array<ref DCO_SimStanceHandler> m_StanceHandlers = {};
	protected ref array<ImageWidget> m_StanceIcons = {};
	protected ref DCO_SimCloseHandler m_CloseHandler;

	protected bool m_bOpen;
	protected IEntity m_AuthorityStateTarget;

	void Init(Widget root)
	{
		if (!root)
			return;
		m_wRoot = root;

		m_wPanel       = root.FindAnyWidget("DCO_SimPanel");
		m_wTargetLabel = TextWidget.Cast(root.FindAnyWidget("DCO_SimTarget"));
		m_btnClose     = ButtonWidget.Cast(root.FindAnyWidget("DCO_SimClose"));

		if (m_btnClose)
		{
			m_CloseHandler = new DCO_SimCloseHandler(this);
			m_btnClose.AddHandler(m_CloseHandler);
		}

		for (int i = 0; i < ROW_COUNT; i++)
		{
			string prefix = "DCO_SimRow" + i.ToString();
			m_Labels.Insert(TextWidget.Cast(root.FindAnyWidget(prefix + "_Label")));
			m_PillBgs.Insert(ImageWidget.Cast(root.FindAnyWidget(prefix + "_PillBg")));
			m_PillTexts.Insert(TextWidget.Cast(root.FindAnyWidget(prefix + "_PillText")));

			ButtonWidget pill = ButtonWidget.Cast(root.FindAnyWidget(prefix + "_Pill"));
			if (pill)
			{
				DCO_SimPillHandler h = new DCO_SimPillHandler(this, i);
				pill.AddHandler(h);
				m_PillHandlers.Insert(h);
			}
		}

		for (int s = 0; s < StanceButtons().Count(); s++)
		{
			m_StanceIcons.Insert(ImageWidget.Cast(root.FindAnyWidget(StanceButtons()[s] + "_Icon")));
			BindStance(root, StanceButtons()[s], s);
		}

		if (m_wPanel)
			m_wPanel.SetVisible(false);	// hidden until the toolbar's SIM button opens it.
		m_bOpen = false;
	}

	protected void BindStance(Widget root, string name, int value)
	{
		ButtonWidget b = ButtonWidget.Cast(root.FindAnyWidget(name));
		if (!b)
			return;
		DCO_SimStanceHandler h = new DCO_SimStanceHandler(this, value);
		b.AddHandler(h);
		m_StanceHandlers.Insert(h);
	}

	bool IsOpen()
	{
		return m_bOpen;
	}

	void Toggle()
	{
		SetOpen(!m_bOpen);
	}

	// Open/close.
	void SetOpen(bool open)
	{
		bool wasOpen = m_bOpen;
		m_bOpen = open;
		if (m_wPanel)
			m_wPanel.SetVisible(open);
		if (!open && wasOpen)
			DCO_GMUIController.ReleaseMenuFocus();

		GetGame().GetCallqueue().Remove(Refresh);
		if (open)
		{
			m_AuthorityStateTarget = null;
			Refresh();	// paint immediately so the panel is never blank for a frame.
			GetGame().GetCallqueue().CallLater(Refresh, REFRESH_MS, true);
		}
	}

	protected IEntity TargetOwner()
	{
		SCR_EditableEntityComponent e = DCO_GMGizmo.Get().GetTarget();
		if (!e)
			return null;
		return e.GetOwner();
	}

	// A pill was clicked.
	bool OnRowClicked(int row)
	{
		IEntity owner = TargetOwner();
		if (!owner)
			return true;

		DCO_GMTools tools = DCO_GMTools.Get();
		int toolId;
		switch (row)
		{
			case ROW_SIM:
			{
				if (!tools.CanToggleSim(owner) || DCO_PlayerUtil.IsPlayer(owner))
					return true;
				toolId = DCO_GMToolsServer.TOOL_SIM;
				break;
			}
			case ROW_DAMAGE:
			{
				if (!tools.HasDamage(owner))
					return true;
				toolId = DCO_GMToolsServer.TOOL_INVULN;	// the existing leg - no duplicate relay for damage.
				break;
			}
			case ROW_AI:
			{
				if (!tools.HasAI(owner) || DCO_PlayerUtil.IsPlayer(owner))
					return true;	// never act on players.
				toolId = DCO_GMToolsServer.TOOL_AI;
				break;
			}
			case ROW_COLLISION:
			{
				if (!tools.CanToggleCollision(owner))
					return true;	// rigid bodies only - a character's capsule is not layer-governed.
				toolId = DCO_GMToolsServer.TOOL_COLLISION;
				break;
			}
			case ROW_WEAPON:
			{
				if (!tools.IsCharacter(owner) || DCO_PlayerUtil.IsPlayer(owner))
					return true;
				toolId = DCO_GMToolsServer.TOOL_WEAPONRAISED;
				break;
			}
			case ROW_TRACER:
			{
				if (!tools.IsTracerEmitter(owner))
					return true;
				toolId = DCO_GMToolsServer.TOOL_TRACER_FIRE;
				break;
			}
			default:
				return false;
		}

		DCO_GMToolsServer.Route(toolId, owner, vector.Zero);
		Refresh();
		return true;
	}

	// A stance button was clicked.
	bool OnStanceClicked(int value)
	{
		IEntity owner = TargetOwner();
		if (!owner)
			return true;
		if (!DCO_GMTools.Get().IsCharacter(owner) || DCO_PlayerUtil.IsPlayer(owner))
			return true;

		DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_STANCE, owner, Vector(value, 0, 0));
		Refresh();
		return true;
	}

	// Repaint every row from the live target.
	protected void Refresh()
	{
		if (!m_bOpen)
			return;

		IEntity owner = TargetOwner();
		if (owner != m_AuthorityStateTarget)
		{
			m_AuthorityStateTarget = owner;
			DCO_GMToolsServer.RequestAuthorityState(owner);
		}

		if (m_wTargetLabel)
		{
			if (owner)
				m_wTargetLabel.SetText(TargetName());
			else
				m_wTargetLabel.SetText("no object selected");
		}

		DCO_GMTools tools = DCO_GMTools.Get();
		bool poseable = owner && tools.IsCharacter(owner) && !DCO_PlayerUtil.IsPlayer(owner);

		SetRow(ROW_SIM,       owner && tools.CanToggleSim(owner) && !DCO_PlayerUtil.IsPlayer(owner), tools.IsSimOn(owner));
		SetRow(ROW_DAMAGE,    owner && tools.HasDamage(owner),           tools.IsDamageOn(owner));
		SetRow(ROW_AI,        owner && tools.HasAI(owner) && !DCO_PlayerUtil.IsPlayer(owner), tools.IsAIOn(owner));
		SetRow(ROW_COLLISION, owner && tools.CanToggleCollision(owner),  tools.IsCollisionOn(owner));
		SetRow(ROW_WEAPON,    poseable,                                  tools.IsWeaponRaisedOn(owner));
		SetRow(ROW_TRACER,    owner && tools.IsTracerEmitter(owner),     tools.IsTracerFiring(owner));

		RefreshPose(owner, poseable, tools);

	}

	protected void RefreshPose(IEntity owner, bool poseable, DCO_GMTools tools)
	{
		int stance = -1;
		if (poseable)
			stance = tools.GetStanceOrd(owner);

		for (int i = 0; i < m_StanceIcons.Count(); i++)
		{
			ImageWidget g = m_StanceIcons[i];
			if (!g)
				continue;
			if (!poseable)
			{
				g.SetColorInt(PILL_OFF_TEXT);
				g.SetOpacity(0.35);
				continue;
			}
			g.SetOpacity(1.0);
			if (i == stance)
				g.SetColor(DCO_GMTheme.Get().m_AccentColor);	// the stance it is actually in.
			else
				g.SetColorInt(PILL_OFF_TEXT);
		}
	}

	protected string TargetName()
	{
		SCR_EditableEntityComponent e = DCO_GMGizmo.Get().GetTarget();
		if (!e)
			return "no object selected";
		SCR_UIInfo info = e.GetInfo();
		if (info)
		{
			string nm = info.GetName();
			if (!nm.IsEmpty())
				return nm;
		}
		return "selected object";
	}

	// Paint one row.
	protected void SetRow(int row, bool applicable, bool on)
	{
		if (row < 0 || row >= m_Labels.Count())
			return;
		TextWidget label = m_Labels[row];
		ImageWidget bg   = m_PillBgs[row];
		TextWidget txt   = m_PillTexts[row];

		if (!applicable)
		{
			if (label)
				label.SetOpacity(0.35);
			if (bg)
			{
				bg.SetColorInt(PILL_NA_BG);
				bg.SetOpacity(0.5);
			}
			if (txt)
			{
				txt.SetText("N/A");
				txt.SetColorInt(PILL_OFF_TEXT);
				txt.SetOpacity(0.5);
			}
			return;
		}

		if (label)
			label.SetOpacity(1.0);
		if (bg)
			bg.SetOpacity(1.0);
		if (txt)
			txt.SetOpacity(1.0);

		if (on)
		{
			if (bg)
				bg.SetColor(DCO_GMTheme.Get().m_AccentColor);	// live accent, so the pill follows the GM's theme.
			if (txt)
			{
				txt.SetText("ON");
				txt.SetColorInt(PILL_ON_TEXT);
			}
			return;
		}

		if (bg)
			bg.SetColorInt(PILL_OFF_BG);
		if (txt)
		{
			txt.SetText("OFF");
			txt.SetColorInt(PILL_OFF_TEXT);
		}
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(Refresh);
		m_bOpen = false;
		m_PillHandlers.Clear();
		m_StanceHandlers.Clear();
		m_StanceIcons.Clear();
		m_Labels.Clear();
		m_PillBgs.Clear();
		m_PillTexts.Clear();
		m_CloseHandler = null;
		m_btnClose = null;
		m_wTargetLabel = null;
		m_wPanel = null;
		m_wRoot = null;
	}
}
