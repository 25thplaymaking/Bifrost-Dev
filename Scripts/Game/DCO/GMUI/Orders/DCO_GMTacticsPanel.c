
// One per button; carries its id so OnClick maps straight to the action.
class DCO_TacBtnHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMTacticsPanel m_Owner;
	protected int m_Id;

	void DCO_TacBtnHandler(DCO_GMTacticsPanel owner, int id)
	{
		m_Owner = owner;
		m_Id = id;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnButton(m_Id);
		return false;
	}
}

class DCO_TacRangeSlider : DCO_GMSlider
{
	void SetRange(float min, float max, float cur, string suffix)
	{
		m_Min = min;
		m_Max = max;
		m_Suffix = suffix;
		m_Cur = Math.Clamp(cur, min, max);
		Refresh();
	}
}

class DCO_GMTacticsPanel
{
	protected static ref DCO_GMTacticsPanel s_Inst;
	static DCO_GMTacticsPanel Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_GMTacticsPanel();
		return s_Inst;
	}

	protected static const int BTN_CLOSE      = 0;
	protected static const int BTN_SPRING     = 1;
	protected static const int BTN_REARM      = 2;
	protected static const int BTN_PAIR_MINUS = 3;
	protected static const int BTN_PAIR_PLUS  = 4;
	protected static const int BTN_SEND       = 5;

	protected static const int POLL_MS = 500;	// target-alive check + cheap repaint while open.
	protected static const float ROUTE_THROTTLE_MS = 100.0;

	protected Widget m_wRoot;
	protected Widget m_wPanel;
	protected TextWidget m_wTitle;
	protected TextWidget m_wTarget;
	protected Widget m_wRadiusRow;
	protected Widget m_wRangeRow;
	protected TextWidget m_wRangeLabel;
	protected Widget m_wPairRow;
	protected TextWidget m_wPairValue;
	protected Widget m_wSendRow;
	protected ImageWidget m_wSendPillBg;
	protected TextWidget m_wSendPillText;
	protected ButtonWidget m_btnSpring;
	protected ButtonWidget m_btnRearm;

	protected ref DCO_GMSlider m_RadiusSlider = new DCO_GMSlider();
	protected ref DCO_TacRangeSlider m_RangeSlider = new DCO_TacRangeSlider();
	protected ref array<ref DCO_TacBtnHandler> m_Handlers = {};

	protected bool m_bOpen;
	protected bool m_bClearMode;
	protected SCR_EditableEntityComponent m_Zone;
	protected DCO_TaskZoneComponent m_ZoneComp;
	protected SCR_EditableEntityComponent m_Group;	// ORDERS-box PRIMARY group, null when placed from CREATE.
	protected ref array<SCR_EditableEntityComponent> m_Groups = {};
	protected bool m_bSendGroup;	// per-placement opt-in, DEFAULTS OFF, committed on CLOSE.
	protected int m_iPair;
	protected float m_fLastRouteMs = -1;

	void Init(Widget shellRoot)
	{
		if (!shellRoot)
			return;
		m_wRoot = shellRoot;
		s_Inst = this;

		m_wPanel      = shellRoot.FindAnyWidget("DCO_TacticsPanel");
		m_wTitle      = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_TacticsTitle"));
		m_wTarget     = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_TacticsTarget"));
		m_wRadiusRow  = shellRoot.FindAnyWidget("DCO_TacRadiusRow");
		m_wRangeRow   = shellRoot.FindAnyWidget("DCO_TacRangeRow");
		m_wRangeLabel = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_TacRange_Label"));
		m_wPairRow    = shellRoot.FindAnyWidget("DCO_TacPairRow");
		m_wPairValue  = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_TacPair_Value"));
		m_wSendRow    = shellRoot.FindAnyWidget("DCO_TacSendRow");
		m_wSendPillBg   = ImageWidget.Cast(shellRoot.FindAnyWidget("DCO_TacSend_PillBg"));
		m_wSendPillText = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_TacSend_PillText"));
		m_btnSpring   = ButtonWidget.Cast(shellRoot.FindAnyWidget("DCO_TacticsSpring"));
		m_btnRearm    = ButtonWidget.Cast(shellRoot.FindAnyWidget("DCO_TacticsRearm"));

		BindBtn("DCO_TacticsClose", BTN_CLOSE);
		BindBtn("DCO_TacticsSpring", BTN_SPRING);
		BindBtn("DCO_TacticsRearm", BTN_REARM);
		BindBtn("DCO_TacPair_Minus", BTN_PAIR_MINUS);
		BindBtn("DCO_TacPair_Plus", BTN_PAIR_PLUS);
		BindBtn("DCO_TacSend_Pill", BTN_SEND);

		m_RadiusSlider.Init(shellRoot, "DCO_TacRadius_Track", "DCO_TacRadius_Fill", "DCO_TacRadius_Value", 5, 500, 50, " m");
		m_RadiusSlider.GetOnChange().Insert(OnRadiusChanged);
		m_RangeSlider.Init(shellRoot, "DCO_TacRange_Track", "DCO_TacRange_Fill", "DCO_TacRange_Value", 10, 300, 50, " m");
		m_RangeSlider.GetOnChange().Insert(OnRangeChanged);

		if (m_wPanel)
			m_wPanel.SetVisible(false);
		m_bOpen = false;
	}

	protected void BindBtn(string name, int id)
	{
		if (!m_wRoot)
			return;
		ButtonWidget b = ButtonWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!b)
			return;
		DCO_TacBtnHandler h = new DCO_TacBtnHandler(this, id);
		b.AddHandler(h);
		m_Handlers.Insert(h);
	}

	bool IsOpen()
	{
		return m_bOpen;
	}

	void OpenForZone(SCR_EditableEntityComponent zoneEditable, SCR_EditableEntityComponent group, array<SCR_EditableEntityComponent> allGroups = null)
	{
		if (!m_wPanel || !zoneEditable)
			return;
		IEntity owner = zoneEditable.GetOwner();
		if (!owner)
			return;
		DCO_TaskZoneComponent comp = DCO_TaskZoneComponent.Cast(owner.FindComponent(DCO_TaskZoneComponent));
		if (!comp)
			return;

		m_Zone = zoneEditable;
		m_ZoneComp = comp;
		m_Group = group;
		m_Groups.Clear();
		if (allGroups)
		{
			foreach (SCR_EditableEntityComponent g : allGroups)
			{
				if (g && m_Groups.Find(g) < 0)
					m_Groups.Insert(g);
			}
		}
		if (m_Groups.IsEmpty() && group)
			m_Groups.Insert(group);
		m_bClearMode = false;
		m_bSendGroup = false;	// per-placement opt-in, always back to OFF.
		m_iPair = comp.DCO_GetPairId();

		EDCO_ZoneRole role = comp.DCO_GetRole();
		if (m_wTitle)
			m_wTitle.SetText(RoleTitle(role));
		if (m_wTarget)
			m_wTarget.SetText(TargetName(zoneEditable));

		// RADIUS - always.
		Show(m_wRadiusRow, true);
		m_RadiusSlider.SetValue(comp.DCO_GetRadius());

		bool hasRange = role == EDCO_ZoneRole.QRF || role == EDCO_ZoneRole.AMBUSH;
		Show(m_wRangeRow, hasRange);
		if (hasRange)
		{
			float push = comp.DCO_GetPushRange();
			if (role == EDCO_ZoneRole.QRF)
			{
				if (push <= 0)
					push = 1500;
				m_RangeSlider.SetRange(200, 3000, push, " m");
				if (m_wRangeLabel)
					m_wRangeLabel.SetText("QRF RANGE");
			}
			else
			{
				if (push <= 0)
					push = 50;
				m_RangeSlider.SetRange(10, 300, push, " m");	// bounds match the registered Ambush Range attribute.
				if (m_wRangeLabel)
					m_wRangeLabel.SetText("TRIGGER RANGE");
			}
		}

		// PAIR - ambush position + kill-zone.
		Show(m_wPairRow, role == EDCO_ZoneRole.AMBUSH || role == EDCO_ZoneRole.AMBUSH_TRIGGER);
		RefreshPair();

		bool canSend = !m_Groups.IsEmpty() && (role == EDCO_ZoneRole.QRF || role == EDCO_ZoneRole.DEFEND || role == EDCO_ZoneRole.AMBUSH);
		Show(m_wSendRow, canSend);
		RefreshSendPill();

		Show(m_btnSpring, role == EDCO_ZoneRole.AMBUSH);
		Show(m_btnRearm, role == EDCO_ZoneRole.AMBUSH_TRIGGER);

		SetOpen(true);
	}

	void OpenForClearOrder(SCR_EditableEntityComponent wpEditable)
	{
		if (!m_wPanel || !wpEditable)
			return;
		m_Zone = wpEditable;
		m_ZoneComp = null;
		m_Group = null;
		m_Groups.Clear();
		m_bClearMode = true;
		m_bSendGroup = false;

		if (m_wTitle)
			m_wTitle.SetText("CLEAR BUILDING");
		if (m_wTarget)
			m_wTarget.SetText("order placed - the group will clear the building at the waypoint");
		Show(m_wRadiusRow, false);
		Show(m_wRangeRow, false);
		Show(m_wPairRow, false);
		Show(m_wSendRow, false);
		Show(m_btnSpring, false);
		Show(m_btnRearm, false);

		SetOpen(true);
	}

	// CLOSE button path: commits the send-group opt-in, then hides.
	void Close(bool commit)
	{
		if (commit && m_bSendGroup && !m_Groups.IsEmpty() && m_Zone && !m_bClearMode)
		{
			IEntity zoneEnt = m_Zone.GetOwner();
			if (zoneEnt)
			{
				foreach (SCR_EditableEntityComponent g : m_Groups)
				{
					if (!g)
						continue;
					IEntity groupEnt = g.GetOwner();
					if (!groupEnt)
						continue;
					DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_SENDGROUP, groupEnt, zoneEnt.GetOrigin());
				}
			}
		}
		SetOpen(false);
	}

	void CloseSilent()
	{
		SetOpen(false);
	}

	protected void SetOpen(bool open)
	{
		bool wasOpen = m_bOpen;
		if (open && !DCO_GMTheme.Get().IsElementEnabled(DCO_GMTheme.UI_TACTICS))
			DCO_GMTheme.Get().SetElementEnabled(DCO_GMTheme.UI_TACTICS, true, m_wRoot, false);
		m_bOpen = open;
		if (m_wPanel)
			m_wPanel.SetVisible(open);
		if (!open && wasOpen)
			DCO_GMUIController.ReleaseMenuFocus();
		GetGame().GetCallqueue().Remove(Poll);
		if (open)
		{
			m_RadiusSlider.Refresh();	// tracks need a laid-out screen size for the fill width.
			m_RangeSlider.Refresh();
			GetGame().GetCallqueue().CallLater(ClampToViewport, 0, false);	// saved geometry may have come from another resolution.
			GetGame().GetCallqueue().CallLater(Poll, POLL_MS, true);
		}
		else
		{
			m_Zone = null;
			m_ZoneComp = null;
			m_Group = null;
			m_Groups.Clear();
			m_bSendGroup = false;
		}
	}

	protected void ClampToViewport()
	{
		if (m_bOpen)
			DCO_GMTheme.ClampPanelToViewport(m_wPanel);
	}

	protected void Poll()
	{
		if (!m_bOpen)
			return;
		if (DCO_GMTheme.Get().IsMasterHidden())
			return;
		if (!m_Zone || !m_Zone.GetOwner())
		{
			CloseSilent();
			return;
		}
		if (m_Group && !m_Group.GetOwner())
			m_Group = null;	// primary died - drop it but keep configuring the zone.
		for (int i = m_Groups.Count() - 1; i >= 0; i--)
		{
			if (!m_Groups[i] || !m_Groups[i].GetOwner())
				m_Groups.Remove(i);
		}
	}

	bool OnButton(int id)
	{
		if (id == BTN_CLOSE)
		{
			Close(true);
			return true;
		}
		if (m_bClearMode)
			return true;	// clear mode has no other live controls.
		IEntity zoneEnt;
		if (m_Zone)
			zoneEnt = m_Zone.GetOwner();
		if (!zoneEnt)
			return true;

		switch (id)
		{
			case BTN_SPRING:
			{
				DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_ZONE_SPRING, zoneEnt, vector.Zero);
				Print("[DCO-GM] tactics panel: SPRING NOW routed", LogLevel.NORMAL);
				break;
			}
			case BTN_REARM:
			{
				DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_ZONE_REARM, zoneEnt, vector.Zero);
				Print("[DCO-GM] tactics panel: kill-zone RE-ARM routed", LogLevel.NORMAL);
				break;
			}
			case BTN_PAIR_MINUS:
			{
				StepPair(zoneEnt, -1);
				break;
			}
			case BTN_PAIR_PLUS:
			{
				StepPair(zoneEnt, 1);
				break;
			}
			case BTN_SEND:
			{
				m_bSendGroup = !m_bSendGroup;
				RefreshSendPill();
				break;
			}
		}
		return true;
	}

	protected void StepPair(IEntity zoneEnt, int delta)
	{
		m_iPair = Math.ClampInt(m_iPair + delta, 0, 50);
		if (m_ZoneComp)
			m_ZoneComp.DCO_SetPairId(m_iPair);
		DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_ZONE_PAIR, zoneEnt, Vector(m_iPair, 0, 0));
		RefreshPair();
	}

	protected void OnRadiusChanged(float value)
	{
		if (!m_ZoneComp || !m_Zone)
			return;
		m_ZoneComp.DCO_SetRadius(value);	// local echo + immediate redraw.
		IEntity zoneEnt = m_Zone.GetOwner();
		if (zoneEnt && ShouldRoute(m_RadiusSlider.IsDragging()))
			DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_ZONE_RADIUS, zoneEnt, Vector(value, 0, 0));
	}

	protected void OnRangeChanged(float value)
	{
		if (!m_ZoneComp || !m_Zone)
			return;
		m_ZoneComp.DCO_SetPushRange(value);	// local echo.
		IEntity zoneEnt = m_Zone.GetOwner();
		if (zoneEnt && ShouldRoute(m_RangeSlider.IsDragging()))
			DCO_GMToolsServer.Route(DCO_GMToolsServer.TOOL_ZONE_RANGE, zoneEnt, Vector(value, 0, 0));
	}

	// True when a relay write should go out now: always on a release event, at most every 100 ms mid-drag.
	protected bool ShouldRoute(bool dragging)
	{
		if (!dragging)
			return true;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return true;
		float now = world.GetWorldTime();
		if (m_fLastRouteMs >= 0 && (now - m_fLastRouteMs) < ROUTE_THROTTLE_MS)
			return false;
		m_fLastRouteMs = now;
		return true;
	}

	protected void RefreshPair()
	{
		if (m_wPairValue)
		{
			if (m_iPair == 0)
				m_wPairValue.SetText("AUTO");
			else
				m_wPairValue.SetText(m_iPair.ToString());
		}
	}

	protected void RefreshSendPill()
	{
		if (m_wSendPillBg)
		{
			if (m_bSendGroup)
				m_wSendPillBg.SetColor(DCO_GMTheme.Get().m_AccentColor);
			else
				m_wSendPillBg.SetColorInt(DCO_GMComponentPanel.PILL_OFF_BG);
		}
		if (m_wSendPillText)
		{
			if (m_bSendGroup)
			{
				m_wSendPillText.SetText("ON");
				m_wSendPillText.SetColorInt(DCO_GMComponentPanel.PILL_ON_TEXT);
			}
			else
			{
				m_wSendPillText.SetText("OFF");
				m_wSendPillText.SetColorInt(DCO_GMComponentPanel.PILL_OFF_TEXT);
			}
		}
	}

	protected string RoleTitle(EDCO_ZoneRole role)
	{
		if (role == EDCO_ZoneRole.QRF)
			return "QRF STAGE AREA";
		if (role == EDCO_ZoneRole.DEFEND)
			return "DEFEND AREA";
		if (role == EDCO_ZoneRole.AMBUSH)
			return "AMBUSH POSITION";
		if (role == EDCO_ZoneRole.AMBUSH_TRIGGER)
			return "KILL-ZONE";
		return "TASK ZONE";
	}

	protected string TargetName(SCR_EditableEntityComponent e)
	{
		if (e)
		{
			SCR_UIInfo info = e.GetInfo();
			if (info)
			{
				string nm = info.GetName();
				if (!nm.IsEmpty())
					return nm;
			}
		}
		return "placed zone";
	}

	protected void Show(Widget w, bool visible)
	{
		if (w)
			w.SetVisible(visible);
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(ClampToViewport);
		m_RadiusSlider.Shutdown();
		m_RangeSlider.Shutdown();
		m_bOpen = false;
		m_bClearMode = false;
		m_Zone = null;
		m_ZoneComp = null;
		m_Group = null;
		m_Groups.Clear();
		m_Handlers.Clear();
		m_wPanel = null;
		m_wTitle = null;
		m_wTarget = null;
		m_wRadiusRow = null;
		m_wRangeRow = null;
		m_wRangeLabel = null;
		m_wPairRow = null;
		m_wPairValue = null;
		m_wSendRow = null;
		m_wSendPillBg = null;
		m_wSendPillText = null;
		m_btnSpring = null;
		m_btnRearm = null;
		m_wRoot = null;
	}
}
