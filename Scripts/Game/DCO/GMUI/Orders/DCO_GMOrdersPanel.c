
class DCO_OrdersPanelHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMOrdersPanel m_Owner;

	void DCO_OrdersPanelHandler(DCO_GMOrdersPanel owner)
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

class DCO_GMOrdersPanel
{
	protected Widget m_wRoot;
	protected Widget m_wBox;
	protected DCO_GMContextMenu m_Menu;
	protected ref DCO_OrdersPanelHandler m_Handler;
	protected ref DCO_GMGroupOrders m_Orders = new DCO_GMGroupOrders();
	protected ref ScriptInvoker m_Cb = new ScriptInvoker();

	protected ref DCO_GMCommandOrders m_Commands = new DCO_GMCommandOrders();	// imported native command-bar commands.

	protected TextWidget m_wHint;
	protected TextWidget m_wTitle;
	protected ButtonWidget m_btnStance;
	protected ButtonWidget m_btnFormation;
	protected ButtonWidget m_btnBehavior;
	protected ButtonWidget m_btnTactics;
	protected ButtonWidget m_btnWaypoints;
	protected ButtonWidget m_btnObjectives;
	protected ButtonWidget m_btnSpawn;

	protected SCR_EditableEntityComponent m_CurrentGroup;
	protected ref array<SCR_EditableEntityComponent> m_aGroups = {};	// EVERY selected group - orders fan out over all of them.

	void Init(Widget shellRoot, DCO_GMContextMenu menu)
	{
		if (!shellRoot)
			return;
		m_wRoot = shellRoot;
		m_Menu = menu;
		m_wBox = shellRoot.FindAnyWidget("DCO_OrdersBox");
		if (!m_wBox)
		{
			Print("[DCO-GM] orders box: DCO_OrdersBox not found (layout not reloaded yet?)", LogLevel.WARNING);
			return;
		}
		m_Handler = new DCO_OrdersPanelHandler(this);
		m_wHint  = TextWidget.Cast(m_wBox.FindAnyWidget("DCO_OrdersHint"));
		m_wTitle = TextWidget.Cast(m_wBox.FindAnyWidget("DCO_OrdersTitle"));
		m_btnStance     = Bind("DCO_Ord_Stance");
		m_btnFormation  = Bind("DCO_Ord_Formation");
		m_btnBehavior   = Bind("DCO_Ord_Behavior");
		m_btnTactics    = Bind("DCO_Ord_Tactics");
		m_btnWaypoints  = Bind("DCO_Ord_Waypoints");
		m_btnObjectives = Bind("DCO_Ord_Objectives");
		m_btnSpawn      = Bind("DCO_Ord_Spawn");
		m_Cb.Insert(OnOrderPicked);
		Refresh();
		GetGame().GetCallqueue().CallLater(PollSelection, 300, true);	// track the live selection.
		Print("[DCO-GM] orders box bound (selection-driven group orders)", LogLevel.NORMAL);
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(PollSelection);
	}

	protected ButtonWidget Bind(string name)
	{
		ButtonWidget b = ButtonWidget.Cast(m_wBox.FindAnyWidget(name));
		if (b)
			b.AddHandler(m_Handler);
		return b;
	}

	protected void PollSelection()
	{
		array<SCR_EditableEntityComponent> groups = {};
		ResolveSelectedGroups(groups);
		if (SameAsTracked(groups))
			return;
		m_aGroups.Clear();
		foreach (SCR_EditableEntityComponent g : groups)
			m_aGroups.Insert(g);
		if (m_aGroups.IsEmpty())
			m_CurrentGroup = null;
		else
			m_CurrentGroup = m_aGroups[0];
		Refresh();
	}

	// EVERY selected group, deduped and in selection order.
	protected void ResolveSelectedGroups(notnull array<SCR_EditableEntityComponent> outGroups)
	{
		set<SCR_EditableEntityComponent> sel = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(sel, EEditableEntityState.SELECTED);	// engine typo "Enitities".
		foreach (SCR_EditableEntityComponent e : sel)
		{
			SCR_EditableEntityComponent g = ResolveGroup(e);
			if (g && outGroups.Find(g) < 0)
				outGroups.Insert(g);
		}
	}

	protected bool SameAsTracked(notnull array<SCR_EditableEntityComponent> groups)
	{
		if (groups.Count() != m_aGroups.Count())
			return false;
		for (int i = 0; i < groups.Count(); i++)
		{
			if (groups[i] != m_aGroups[i])
				return false;
		}
		return true;
	}

	// A group selection -> itself; a character selection -> its parent group; anything else -> null.
	protected SCR_EditableEntityComponent ResolveGroup(SCR_EditableEntityComponent e)
	{
		if (!e)
			return null;
		if (e.GetEntityType() == EEditableEntityType.GROUP)
			return e;
		if (e.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			SCR_EditableEntityComponent p = e.GetParentEntity();
			if (p && p.GetEntityType() == EEditableEntityType.GROUP)
				return p;
		}
		return null;
	}

	protected void Refresh()
	{
		bool hasGroup = m_CurrentGroup != null;
		if (m_wHint)
			m_wHint.SetVisible(!hasGroup);
		SetBtn(m_btnStance, hasGroup);
		SetBtn(m_btnFormation, hasGroup);
		SetBtn(m_btnBehavior, hasGroup);
		SetBtn(m_btnTactics, hasGroup);
		SetBtn(m_btnWaypoints, hasGroup);
		SetBtn(m_btnObjectives, hasGroup);
		SetBtn(m_btnSpawn, hasGroup);
		if (m_wTitle)
		{
			if (m_aGroups.Count() > 1)
				m_wTitle.SetText(string.Format("COMMAND  ·  %1 GROUPS", m_aGroups.Count()));	// orders apply to all of them.
			else if (hasGroup)
				m_wTitle.SetText("COMMAND  ·  GROUP");
			else
				m_wTitle.SetText("COMMAND");
		}
	}

	protected void SetBtn(ButtonWidget b, bool vis)
	{
		if (b)
			b.SetVisible(vis);
	}

	bool OnButton(Widget w)
	{
		if (!m_CurrentGroup || !m_Menu)
			return false;
		int cat = -1;
		bool isCmd = false;
		if (w == m_btnStance)
			cat = DCO_GMGroupOrders.SUB_STANCE;
		else if (w == m_btnFormation)
			cat = DCO_GMGroupOrders.SUB_FORMATION;
		else if (w == m_btnBehavior)
			cat = DCO_GMGroupOrders.SUB_BEHAVIOR;
		else if (w == m_btnTactics)
			cat = DCO_GMGroupOrders.SUB_TACTICS;
		else if (w == m_btnWaypoints)
		{
			cat = DCO_GMCommandOrders.CAT_WAYPOINTS;
			isCmd = true;
		}
		else if (w == m_btnObjectives)
		{
			cat = DCO_GMCommandOrders.CAT_OBJECTIVES;
			isCmd = true;
		}
		else if (w == m_btnSpawn)
		{
			cat = DCO_GMCommandOrders.CAT_SPAWN;
			isCmd = true;
		}
		if (cat < 0)
			return false;

		array<string> labels = {};
		array<int> ids = {};
		if (isCmd)
			m_Commands.BuildCategoryOptions(cat, labels, ids);
		else
			DCO_GMGroupOrders.BuildCategoryOptions(cat, labels, ids);	// DCO orders.

		// Anchor beside the COMMAND surface.
		string menuTitle = "COMMAND OPTIONS";
		if (w == m_btnStance) menuTitle = "STANCE";
		else if (w == m_btnFormation) menuTitle = "FORMATION";
		else if (w == m_btnBehavior) menuTitle = "BEHAVIOR";
		else if (w == m_btnTactics) menuTitle = "TACTICS";
		else if (w == m_btnWaypoints) menuTitle = "ORDERS";
		else if (w == m_btnObjectives) menuTitle = "OBJECTIVES";
		else if (w == m_btnSpawn) menuTitle = "SPAWN POINT";
		m_Menu.ShowAdjacent(labels, ids, w, m_wBox, menuTitle, m_Cb, m_CurrentGroup);
		return true;
	}

	protected void OnOrderPicked(int actionId, SCR_EditableEntityComponent e)
	{
		if (!e)
			return;
		array<SCR_EditableEntityComponent> targets = {};
		foreach (SCR_EditableEntityComponent g : m_aGroups)
		{
			if (g)
				targets.Insert(g);
		}
		if (targets.IsEmpty())
			targets.Insert(e);	// selection changed under the open menu - honour what it was opened for.

		if (DCO_GMGroupOrders.IsTacticPlacement(actionId))
		{
			DCO_GMTacticsFlow.Get().BeginPlacement(actionId, targets[0], targets);
			return;
		}
		if (DCO_GMCommandOrders.IsCommandLeaf(actionId))
		{
			m_Commands.Trigger(actionId, targets[0], targets);	// native command -> arm placing; world-click places it.
			return;
		}
		foreach (SCR_EditableEntityComponent g : targets)
			m_Orders.Apply(actionId, g);
	}
}
