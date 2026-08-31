// DCO CREATE-panel budget display and budget-cap toggle.
modded class SCR_BudgetEditorComponent
{
	[RplProp(onRplName: "DCO_OnBudgetLimitsChanged")]
	protected bool m_bDCOBudgetLimitsEnabled = true;

	void DCO_SetBudgetLimitsEnabled(bool enabled)
	{
		if (Replication.IsServer())
		{
			DCO_ApplyBudgetLimitsEnabled(enabled);
			return;
		}
		if (!m_RplComponent || !m_RplComponent.Id().IsValid())
			return;
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (pc)
			pc.DCO_SendGMBudgetLimits(m_RplComponent.Id(), enabled);
	}

	bool DCO_AreBudgetLimitsEnabled()
	{
		return m_bDCOBudgetLimitsEnabled;
	}

	void DCO_ApplyBudgetLimitsEnabled(bool enabled)
	{
		if (!Replication.IsServer() || m_bDCOBudgetLimitsEnabled == enabled)
			return;
		m_bDCOBudgetLimitsEnabled = enabled;
		Replication.BumpMe();
		DCO_OnBudgetLimitsChanged();
	}

	protected void DCO_OnBudgetLimitsChanged()
	{
		DCO_GMBudgetReadout.OnLimitsReplicated(this);
	}

	override protected bool IsBudgetCapEnabled()
	{
		return m_bDCOBudgetLimitsEnabled && super.IsBudgetCapEnabled();
	}
}

class DCO_GMBudgetServer
{
	static void Apply(RplId componentOwnerId, bool enabled)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(componentOwnerId));
		if (!rpl || !rpl.GetEntity())
			return;
		SCR_BudgetEditorComponent budget = SCR_BudgetEditorComponent.Cast(rpl.GetEntity().FindComponent(SCR_BudgetEditorComponent));
		if (budget)
			budget.DCO_ApplyBudgetLimitsEnabled(enabled);
	}
}

// engine performs a second server-side accumulated-budget check after the component check.
modded class SCR_PlacingEditorComponent
{
	override bool IsThereEnoughBudgetToSpawn(IEntityComponentSource entitySource)
	{
		if (m_BudgetManager && !m_BudgetManager.DCO_AreBudgetLimitsEnabled())
			return true;

		return super.IsThereEnoughBudgetToSpawn(entitySource);
	}
}

class DCO_GMBudgetToggleHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMBudgetReadout m_Owner;

	void Init(DCO_GMBudgetReadout owner)
	{
		m_Owner = owner;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Owner || !w || w.GetName() != "DCO_BudgetLimitsToggle")
			return false;

		m_Owner.ToggleLimits();
		return true;
	}
}

class DCO_GMBudgetReadout
{
	static const int CELLS = 4;
	protected static ref DCO_GMBudgetReadout s_Active;

	protected Widget m_wBrowser;
	protected SCR_BudgetEditorComponent m_Budget;
	protected ref array<Widget> m_Rows = {};
	protected ref array<TextWidget> m_Labels = {};
	protected ref array<TextWidget> m_Stats = {};
	protected ref array<ProgressBarWidget> m_Bars = {};
	protected ButtonWidget m_LimitsButton;
	protected TextWidget m_LimitsLabel;
	protected ref DCO_GMBudgetToggleHandler m_ToggleHandler;
	protected bool m_bBound;

	void Init(Widget browserRoot)
	{
		if (!browserRoot)
			return;

		m_wBrowser = browserRoot;
		s_Active = this;
		for (int i = 0; i < CELLS; i++)
		{
			m_Rows.Insert(m_wBrowser.FindAnyWidget(string.Format("DCO_BudgetRow_%1", i)));
			m_Labels.Insert(TextWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_BudgetLabel_%1", i))));
			m_Stats.Insert(TextWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_BudgetStat_%1", i))));

			ProgressBarWidget bar = ProgressBarWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_BudgetBar_%1", i)));
			m_Bars.Insert(bar);
			if (bar)
			{
				bar.SetMin(0);
				bar.SetMax(1);
				bar.SetCurrent(0);
				bar.SetDrawBackground(false);
			}
		}

		m_LimitsButton = ButtonWidget.Cast(m_wBrowser.FindAnyWidget("DCO_BudgetLimitsToggle"));
		m_LimitsLabel = TextWidget.Cast(m_wBrowser.FindAnyWidget("DCO_BudgetLimitsLabel"));
		if (m_LimitsButton)
		{
			m_ToggleHandler = new DCO_GMBudgetToggleHandler();
			m_ToggleHandler.Init(this);
			m_LimitsButton.AddHandler(m_ToggleHandler);
		}

		m_Budget = SCR_BudgetEditorComponent.Cast(SCR_BudgetEditorComponent.GetInstance(SCR_BudgetEditorComponent, false, true));
		if (!m_Budget)
		{
			Print("[DCO-GM] budget display: no SCR_BudgetEditorComponent instance", LogLevel.WARNING);
			return;
		}

		m_Budget.Event_OnBudgetUpdated.Insert(OnBudgetUpdated);
		m_Budget.Event_OnBudgetMaxUpdated.Insert(OnBudgetMaxUpdated);
		m_bBound = true;
		m_Budget.DemandBudgetUpdateFromServer();
		Refresh();
	}

	void Shutdown()
	{
		if (m_Budget && m_bBound)
		{
			m_Budget.Event_OnBudgetUpdated.Remove(OnBudgetUpdated);
			m_Budget.Event_OnBudgetMaxUpdated.Remove(OnBudgetMaxUpdated);
		}

		if (m_LimitsButton && m_ToggleHandler)
			m_LimitsButton.RemoveHandler(m_ToggleHandler);

		m_bBound = false;
		if (s_Active == this)
			s_Active = null;
	}

	static void OnLimitsReplicated(SCR_BudgetEditorComponent budget)
	{
		if (s_Active && s_Active.m_Budget == budget)
			s_Active.Refresh();
	}

	void ToggleLimits()
	{
		if (!m_Budget)
			return;

		m_Budget.DCO_SetBudgetLimitsEnabled(!m_Budget.DCO_AreBudgetLimitsEnabled());
		Refresh();
	}

	protected void OnBudgetUpdated(EEditableEntityBudget budgetType, int originalBudgetValue, int updatedBudgetValue, int maxBudgetValue)
	{
		Refresh();
	}

	protected void OnBudgetMaxUpdated(EEditableEntityBudget budgetType, int currentBudgetValue, int maxBudgetValue)
	{
		Refresh();
	}

	protected void Refresh()
	{
		if (!m_Budget)
			return;

		bool limitsEnabled = m_Budget.DCO_AreBudgetLimitsEnabled();
		if (m_LimitsLabel)
		{
			if (limitsEnabled)
				m_LimitsLabel.SetText("LIMITS ON  /  CLICK TO DISABLE");
			else
				m_LimitsLabel.SetText("LIMITS OFF  /  CLICK TO ENABLE");
		}

		for (int i = 0; i < CELLS; i++)
		{
			EEditableEntityBudget type = BudgetTypeForCell(i);
			int maxValue;
			bool available = m_Budget.GetMaxBudgetValue(type, maxValue) && maxValue > 0;

			if (m_Rows[i])
				m_Rows[i].SetVisible(available);
			if (!available)
				continue;

			int currentValue = m_Budget.GetCurrentBudgetValue(type);
			if (m_Labels[i])
				m_Labels[i].SetText(BudgetLabel(type));

			if (!limitsEnabled)
			{
				if (m_Stats[i])
				{
					m_Stats[i].SetText(string.Format("%1 / NO LIMIT", currentValue));
					m_Stats[i].SetColor(Color.FromRGBA(86, 194, 112, 255));
				}
				if (m_Bars[i])
				{
					m_Bars[i].SetCurrent(0);
					m_Bars[i].SetColor(Color.FromRGBA(86, 194, 112, 255));
				}
				continue;
			}

			float usage = Math.Clamp(currentValue / (float)maxValue, 0.0, 1.0);
			Color usageColor = UsageColor(usage);
			if (m_Stats[i])
			{
				m_Stats[i].SetText(string.Format("%1 / %2  (%3%%)", currentValue, maxValue, Math.Round(usage * 100)));
				m_Stats[i].SetColor(usageColor);
			}
			if (m_Bars[i])
			{
				m_Bars[i].SetCurrent(usage);
				m_Bars[i].SetColor(usageColor);
			}
		}
	}

	protected EEditableEntityBudget BudgetTypeForCell(int index)
	{
		switch (index)
		{
			case 0: return EEditableEntityBudget.AI;
			case 1: return EEditableEntityBudget.VEHICLES;
			case 2: return EEditableEntityBudget.PROPS;
		}
		return EEditableEntityBudget.SYSTEMS;
	}

	protected string BudgetLabel(EEditableEntityBudget type)
	{
		switch (type)
		{
			case EEditableEntityBudget.AI:       return "AI UNITS";
			case EEditableEntityBudget.VEHICLES: return "VEHICLES";
			case EEditableEntityBudget.PROPS:    return "PROPS";
			case EEditableEntityBudget.SYSTEMS:  return "SYSTEMS";
		}
		return "BUDGET";
	}

	protected Color UsageColor(float usage)
	{
		Color green = Color.FromRGBA(86, 194, 112, 255);
		Color amber = Color.FromRGBA(232, 176, 68, 255);
		Color red = Color.FromRGBA(220, 70, 70, 255);

		if (usage <= 0.65)
			return green.LerpNew(amber, usage / 0.65);

		return amber.LerpNew(red, (usage - 0.65) / 0.35);
	}
}
