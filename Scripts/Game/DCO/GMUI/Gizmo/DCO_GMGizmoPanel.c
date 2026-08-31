
class DCO_GizmoFieldHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMGizmoPanel m_Owner;

	void DCO_GizmoFieldHandler(DCO_GMGizmoPanel owner)
	{
		m_Owner = owner;
	}

	override bool OnWriteModeLeave(Widget w)
	{
		if (m_Owner)
			m_Owner.CommitFields();
		return false;
	}
}

class DCO_GizmoNudgeHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMGizmoPanel m_Owner;
	protected int m_Index;

	void DCO_GizmoNudgeHandler(DCO_GMGizmoPanel owner, int index)
	{
		m_Owner = owner;
		m_Index = index;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.Nudge(m_Index);
		return false;
	}
}

class DCO_GMGizmoPanel
{
	static const string W_PANEL   = "DCO_GizmoPanel";
	static const string W_READOUT = "DCO_GizmoReadout";

	protected Widget m_wPanel;
	protected TextWidget m_wReadout;
	protected ref array<EditBoxWidget> m_Fields = {};
	protected ref array<ref DCO_GizmoFieldHandler> m_FieldHandlers = {};
	protected ref array<ref DCO_GizmoNudgeHandler> m_NudgeHandlers = {};
	protected IEntity m_Target;

	// Last string pushed to the widget.
	protected string m_LastText;
	protected bool m_bVisible;

	void Init(Widget shellRoot)
	{
		if (!shellRoot)
			return;
		m_wPanel   = shellRoot.FindAnyWidget(W_PANEL);
		m_wReadout = TextWidget.Cast(shellRoot.FindAnyWidget(W_READOUT));

		for (int i = 0; i < 6; i++)
		{
			EditBoxWidget field = EditBoxWidget.Cast(shellRoot.FindAnyWidget("DCO_GizmoField" + i.ToString()));
			if (field)
			{
				DCO_GizmoFieldHandler fh = new DCO_GizmoFieldHandler(this);
				field.AddHandler(fh);
				m_Fields.Insert(field);
				m_FieldHandlers.Insert(fh);
			}
			ButtonWidget neg = ButtonWidget.Cast(shellRoot.FindAnyWidget("DCO_GizmoNudge" + (i * 2).ToString()));
			ButtonWidget pos = ButtonWidget.Cast(shellRoot.FindAnyWidget("DCO_GizmoNudge" + (i * 2 + 1).ToString()));
			if (i < 3 && neg)
			{
				DCO_GizmoNudgeHandler nh0 = new DCO_GizmoNudgeHandler(this, i * 2);
				neg.AddHandler(nh0);
				m_NudgeHandlers.Insert(nh0);
			}
			if (i < 3 && pos)
			{
				DCO_GizmoNudgeHandler nh1 = new DCO_GizmoNudgeHandler(this, i * 2 + 1);
				pos.AddHandler(nh1);
				m_NudgeHandlers.Insert(nh1);
			}
		}

		if (m_wPanel)
			m_wPanel.SetVisible(false);	// nothing selected yet — the box only appears with a gizmo target.
		m_bVisible = false;
		m_LastText = string.Empty;

	}

	// Push the live transform.
	void Update(IEntity target, vector pos, vector anglesDeg, bool visible)
	{
		m_Target = target;
		if (!DCO_GMTheme.Get().IsElementEnabled(DCO_GMTheme.UI_GIZMO))
			visible = false;
		if (DCO_GMArsenalPanel.Get().IsOpen())
			visible = false;

		if (!visible)
		{
			if (m_bVisible && m_wPanel)
				m_wPanel.SetVisible(false);
			m_bVisible = false;
			return;
		}

		if (!m_bVisible && m_wPanel)
			m_wPanel.SetVisible(true);
		m_bVisible = true;


		if (m_Fields.Count() == 6)
		{
			float values[6] = {pos[0], pos[1], pos[2], anglesDeg[0], anglesDeg[1], anglesDeg[2]};
			for (int fi = 0; fi < 6; fi++)
			{
				EditBoxWidget field = m_Fields[fi];
				if (field && !field.IsInWriteMode())
					field.SetText(F1(values[fi]));
			}
		}

		if (!m_wReadout)
			return;

		string txt = string.Format("X %1  Y %2  Z %3\nYaw %4  Pitch %5  Roll %6",
			F1(pos[0]), F1(pos[1]), F1(pos[2]),
			F1(anglesDeg[0]), F1(anglesDeg[1]), F1(anglesDeg[2]));
		if (txt == m_LastText)
			return;
		m_LastText = txt;
		m_wReadout.SetText(txt);
	}

	// Commit all six typed fields together when any field leaves write mode.
	void CommitFields()
	{
		if (!m_Target || m_Fields.Count() != 6)
			return;
		vector pos = Vector(m_Fields[0].GetText().ToFloat(), m_Fields[1].GetText().ToFloat(), m_Fields[2].GetText().ToFloat());
		vector ang = Vector(m_Fields[3].GetText().ToFloat(), m_Fields[4].GetText().ToFloat(), m_Fields[5].GetText().ToFloat());
		DCO_GMToolsServer.RouteTransform(m_Target, pos, ang);
	}

	// Position nudges use the configured snap increment even when drag snapping is currently off.
	bool Nudge(int index)
	{
		if (!m_Target || index < 0 || index > 5)
			return false;
		DCO_GMGizmo gizmo = DCO_GMGizmo.Get();
		float step = gizmo.GetMoveStep();
		if (step <= 0)
			step = 0.25;
		int axis = index / 2;
		float sign = -1.0;
		if ((index % 2) == 1)
			sign = 1.0;
		vector mat[4];
		m_Target.GetWorldTransform(mat);
		vector dir = vector.Zero;
		if (gizmo.GetSpace() == EDCO_GizmoSpace.LOCAL)
		{
			dir = mat[axis];
			dir.Normalize();
		}
		else
		{
			if (axis == 0) dir = Vector(1, 0, 0);
			else if (axis == 1) dir = Vector(0, 1, 0);
			else dir = Vector(0, 0, 1);
		}
		vector rot[3];
		rot[0] = mat[0]; rot[1] = mat[1]; rot[2] = mat[2];
		DCO_GMToolsServer.RouteTransform(m_Target, mat[3] + dir * step * sign, Math3D.MatrixToAngles(rot));
		return true;
	}

	// One decimal place.
	protected static string F1(float v)
	{
		return v.ToString(-1, 1);
	}

	void Shutdown()
	{
		m_bVisible = false;
		m_LastText = string.Empty;
		m_Target = null;
		m_Fields.Clear();
		m_FieldHandlers.Clear();
		m_NudgeHandlers.Clear();
		m_wReadout = null;
		m_wPanel = null;
	}
}
