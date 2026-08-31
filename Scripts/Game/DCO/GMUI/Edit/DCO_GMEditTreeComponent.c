
// One flattened, visible tree row.
class DCO_TreeRow
{
	SCR_EditableEntityComponent m_Entity;
	int m_Depth;
	bool m_bHasChildren;
	bool m_bExpanded;
	string m_Label;
	ResourceName m_Icon;
	ResourceName m_App6;
	bool m_bFaction;
}

class DCO_EditTreeButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMEditTreeComponent m_Owner;

	void DCO_EditTreeButtonHandler(DCO_GMEditTreeComponent owner)
	{
		m_Owner = owner;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnButton(w);
		return false;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnRowDoubleClick(w);
		return false;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		return button == 1;
	}

	// Capture the row before the editor builds its world context menu.
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button == 1 && m_Owner)
			return m_Owner.OnRowRightClick(w, x, y);
		return false;
	}
}

class DCO_ForceButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMEditTreeComponent m_Owner;
	protected int m_Index;
	protected bool m_bVisibility;

	void DCO_ForceButtonHandler(DCO_GMEditTreeComponent owner, int index, bool visibility)
	{
		m_Owner = owner;
		m_Index = index;
		m_bVisibility = visibility;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Owner)
			return false;
		return m_Owner.OnForceButton(m_Index, m_bVisibility);
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (button == 1 && m_Owner && !m_bVisibility)
			return m_Owner.OnForceWatchMenu(m_Index, x, y);
		return false;
	}

	// Keep the editor world context menu from opening beneath the force-card picker.
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return button == 1 && !m_bVisibility;
	}
}

class DCO_EditSearchHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMEditTreeComponent m_Owner;

	void DCO_EditSearchHandler(DCO_GMEditTreeComponent owner)
	{
		m_Owner = owner;
	}

	override bool OnChange(Widget w, bool finished)
	{
		if (m_Owner)
			m_Owner.OnSearchChanged();
		return false;
	}
}

class DCO_GMEditTreeComponent
{
	static const int ROWS = 28;
	static const int PLAYER_ROWS = 5;
	static const int FORCE_SLOTS = 4;
	static const int FORCE_PICK_PAGE_SIZE = 16;
	static const int FORCE_PICK_PREV = 30000;
	static const int FORCE_PICK_NEXT = 30001;
	static const int FORCE_PICK_SELECT_BASE = 30100;
	static const ResourceName FOLD_DOWN  = "{A3EE9DF5A7573679}img/icons/fold-down.edds";
	static const ResourceName FOLD_RIGHT = "{12D85C56D7B4F8AA}img/icons/fold-right.edds";	// collapsed.

	static const int CAT_ALL  = -1;
	static const int CAT_UNIT = 0;	// CHARACTER / GROUP / FACTION.
	static const int CAT_VEH  = 1;	// VEHICLE.
	static const int CAT_OBJ  = 2;	// GENERIC / SLOT / ITEM / SYSTEM / ...
	static const int CAT_LOC  = 3;
	static const int CAT_AREA = 4;

	protected int m_FilterCat = CAT_UNIT;
	protected ref array<ButtonWidget> m_CatBtns = {};
	protected ref array<int> m_CatValues = {};

	protected Widget m_wRoot;
	protected Widget m_wTree;
	protected ref DCO_EditTreeButtonHandler m_Handler;

	protected TextWidget m_wPageText;
	protected ButtonWidget m_btnPrev;
	protected ButtonWidget m_btnNext;

	protected ref array<ButtonWidget> m_RowBtns = {};
	protected ref array<TextWidget> m_RowLabels = {};
	protected ref array<Widget> m_RowStatusHosts = {};
	protected ref array<TextWidget> m_RowStatus = {};
	protected ref array<ImageWidget> m_RowIcons = {};
	protected ref array<ResourceName> m_RowLoadedIcons = {};
	protected ref array<ImageWidget> m_RowFold = {};
	protected ref array<Widget> m_ForceRows = {};
	protected Widget m_wForceOverview;
	protected ref array<TextWidget> m_ForceLabels = {};
	protected ref array<TextWidget> m_ForceHealth = {};
	protected ref array<TextWidget> m_ForceReady = {};
	protected ref array<TextWidget> m_ForceAmmo = {};
	protected ref array<ProgressBarWidget> m_ForceBars = {};
	protected ref array<ImageWidget> m_ForceIcons = {};
	protected ref array<ResourceName> m_ForceLoadedIcons = {};
	protected ref array<ImageWidget> m_ForceHideIcons = {};
	protected ref array<ImageWidget> m_ForceBackgrounds = {};
	protected ref array<ButtonWidget> m_ForceSelectBtns = {};
	protected ref array<ButtonWidget> m_ForceHideBtns = {};
	protected ref array<SCR_EditableEntityComponent> m_ForceEntities = {};
	protected ref array<SCR_EditableEntityComponent> m_AvailableForces = {};
	protected ref array<FactionKey> m_WatchedForceKeys = {};
	protected ref array<FactionKey> m_ForcePickKeys = {};
	protected ref array<ref DCO_ForceButtonHandler> m_ForceHandlers = {};
	protected ref set<SCR_EditableEntityComponent> m_HiddenForces = new set<SCR_EditableEntityComponent>();
	protected SCR_EditableEntityComponent m_SelectedForce;
	protected ref map<string, int> m_ForceRoundCache = new map<string, int>();
	protected ButtonWidget m_btnForcePrev;
	protected ButtonWidget m_btnForceNext;
	protected TextWidget m_wForcePageText;
	protected int m_ForcePickSlot = -1;
	protected int m_ForcePickPage;
	protected int m_ForceOverviewTick;

	protected ref array<ButtonWidget> m_PlayerBtns = {};
	protected ref array<TextWidget> m_PlayerLabels = {};
	protected ref array<Widget> m_PlayerFpsHosts = {};
	protected ref array<TextWidget> m_PlayerFps = {};
	protected ref array<Widget> m_PlayerHealthHosts = {};
	protected ref array<TextWidget> m_PlayerHealth = {};
	protected ref array<ImageWidget> m_PlayerIcons = {};
	protected ref array<SCR_EditableEntityComponent> m_PlayerEntities = {};
	protected SCR_PlayersManagerEditorComponent m_PlayersMgr;
	protected SCR_LayersEditorComponent m_LayersMgr;	// engine layer manager - enter/exit/create layers.

	protected ref array<ref DCO_TreeRow> m_Rows = {};
	protected ref set<SCR_EditableEntityComponent> m_Collapsed = new set<SCR_EditableEntityComponent>();

	protected EditBoxWidget m_wSearch;
	protected ref DCO_EditSearchHandler m_SearchHandler;
	protected string m_Search = "";
	protected string m_LastSearch = "";
	protected SCR_EditableEntityComponent m_LastSelected;
	protected ref DCO_GMUnitActions m_Actions = new DCO_GMUnitActions();
	protected ref DCO_GMGroupOrders m_GroupOrders = new DCO_GMGroupOrders();
	protected DCO_GMContextMenu m_Menu;	// shared menu, owned by the controller.
	protected ref ScriptInvoker m_MenuCb = new ScriptInvoker();
	protected int m_MenuX;
	protected int m_MenuY;
	protected int m_Page;
	protected bool m_bBound;

	void Init(Widget shellRoot, DCO_GMContextMenu menu)
	{
		if (!shellRoot)
			return;
		m_wRoot = shellRoot;
		m_Menu = menu;
		m_wTree = shellRoot.FindAnyWidget("DCO_EditTree");
		if (!m_wTree)
		{
			Print("[DCO-GM] EDIT tree: DCO_EditTree not found (layout not reloaded yet?)", LogLevel.WARNING);
			return;
		}

		m_Handler = new DCO_EditTreeButtonHandler(this);
		m_wPageText = TextWidget.Cast(m_wTree.FindAnyWidget("DCO_Tree_PageText"));
		m_btnPrev = BindButton("DCO_Tree_Prev");
		m_btnNext = BindButton("DCO_Tree_Next");
		for (int i = 0; i < ROWS; i++)
		{
			m_RowBtns.Insert(BindButton(string.Format("DCO_TreeRow_%1", i)));
			m_RowLabels.Insert(TextWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_TreeRow_%1_Label", i))));
			m_RowStatusHosts.Insert(m_wTree.FindAnyWidget(string.Format("DCO_TreeRow_%1_StatusHost", i)));
			m_RowStatus.Insert(TextWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_TreeRow_%1_Status", i))));
			m_RowIcons.Insert(ImageWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_TreeRow_%1_Icon", i))));
			m_RowLoadedIcons.Insert(ResourceName.Empty);
			m_RowFold.Insert(ImageWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_TreeRow_%1_Fold", i))));
		}
		m_wForceOverview = m_wTree.FindAnyWidget("DCO_ForceOverview");
		m_btnForcePrev = BindButton("DCO_ForcePrev");
		m_btnForceNext = BindButton("DCO_ForceNext");
		m_wForcePageText = TextWidget.Cast(m_wTree.FindAnyWidget("DCO_ForcePage"));
		if (m_btnForcePrev)
			m_btnForcePrev.SetVisible(false);
		if (m_btnForceNext)
			m_btnForceNext.SetVisible(false);
		for (int i = 0; i < FORCE_SLOTS; i++)
		{
			m_ForceRows.Insert(m_wTree.FindAnyWidget(string.Format("DCO_ForceRow_%1", i)));
			m_ForceLabels.Insert(TextWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceLabel_%1", i))));
			m_ForceHealth.Insert(TextWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceHealth_%1", i))));
			m_ForceReady.Insert(TextWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceReady_%1", i))));
			m_ForceAmmo.Insert(TextWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceAmmo_%1", i))));
			m_ForceBars.Insert(ProgressBarWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceBar_%1", i))));
			m_ForceIcons.Insert(ImageWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceIcon_%1", i))));
			m_ForceLoadedIcons.Insert(ResourceName.Empty);
			m_ForceHideIcons.Insert(ImageWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceHideIcon_%1", i))));
			m_ForceBackgrounds.Insert(ImageWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceBg_%1", i))));

			ButtonWidget selectButton = ButtonWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceSelect_%1", i)));
			ButtonWidget hideButton = ButtonWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_ForceHide_%1", i)));
			m_ForceSelectBtns.Insert(selectButton);
			m_ForceHideBtns.Insert(hideButton);
			if (selectButton)
			{
				DCO_ForceButtonHandler selectHandler = new DCO_ForceButtonHandler(this, i, false);
				selectButton.AddHandler(selectHandler);
				m_ForceHandlers.Insert(selectHandler);
			}
			if (hideButton)
			{
				DCO_ForceButtonHandler hideHandler = new DCO_ForceButtonHandler(this, i, true);
				hideButton.AddHandler(hideHandler);
				m_ForceHandlers.Insert(hideHandler);
			}
		}

		array<string> catNames = {"DCO_ETCat_ALL", "DCO_ETCat_UNIT", "DCO_ETCat_VEH", "DCO_ETCat_OBJ", "DCO_ETCat_LOC", "DCO_ETCat_AREA"};
		array<int> catVals = {CAT_ALL, CAT_UNIT, CAT_VEH, CAT_OBJ, CAT_LOC, CAT_AREA};
		for (int i = 0; i < catNames.Count(); i++)
		{
			ButtonWidget cb = BindButton(catNames[i]);
			if (cb)
			{
				m_CatBtns.Insert(cb);
				m_CatValues.Insert(catVals[i]);
			}
		}
		HighlightFilter();

		for (int i = 0; i < PLAYER_ROWS; i++)
		{
			m_PlayerBtns.Insert(BindButton(string.Format("DCO_PlayerRow_%1", i)));
			m_PlayerLabels.Insert(TextWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_PlayerRow_%1_Label", i))));
			m_PlayerFpsHosts.Insert(m_wTree.FindAnyWidget(string.Format("DCO_PlayerRow_%1_FpsHost", i)));
			m_PlayerFps.Insert(TextWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_PlayerRow_%1_Fps", i))));
			m_PlayerHealthHosts.Insert(m_wTree.FindAnyWidget(string.Format("DCO_PlayerRow_%1_HealthHost", i)));
			m_PlayerHealth.Insert(TextWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_PlayerRow_%1_Health", i))));
			m_PlayerIcons.Insert(ImageWidget.Cast(m_wTree.FindAnyWidget(string.Format("DCO_PlayerRow_%1_Icon", i))));
		}
		m_PlayersMgr = SCR_PlayersManagerEditorComponent.Cast(SCR_PlayersManagerEditorComponent.GetInstance(SCR_PlayersManagerEditorComponent, true));
		m_LayersMgr = SCR_LayersEditorComponent.Cast(SCR_LayersEditorComponent.GetInstance(SCR_LayersEditorComponent, true));
		if (m_LayersMgr)
			m_LayersMgr.SetEditingLayersEnabled(true);	// Layer navigation requires editing to be enabled while the panel is active.

		m_bBound = true;

		m_MenuCb.Insert(OnContextAction);

		m_wSearch = EditBoxWidget.Cast(m_wTree.FindAnyWidget("DCO_TreeSearch"));
		if (m_wSearch)
		{
			m_SearchHandler = new DCO_EditSearchHandler(this);
			m_wSearch.AddHandler(m_SearchHandler);
		}

		// Live refresh so the tree tracks placements/removals/selection.
		GetGame().GetCallqueue().CallLater(Rebuild, 1000, true);
		GetGame().GetCallqueue().CallLater(PollSearch, 400, true);
		Rebuild();
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(Rebuild);
		GetGame().GetCallqueue().Remove(PollSearch);
		if (m_Actions)
			m_Actions.Shutdown();
		if (m_Menu)
			m_Menu.Hide();
		if (m_LayersMgr)
			m_LayersMgr.SetEditingLayersEnabled(false);
		if (m_wSearch && m_SearchHandler)
			m_wSearch.RemoveHandler(m_SearchHandler);
		m_SearchHandler = null;
	}

	protected ButtonWidget BindButton(string name)
	{
		ButtonWidget b = ButtonWidget.Cast(m_wTree.FindAnyWidget(name));
		if (b)
			b.AddHandler(m_Handler);
		return b;
	}

	void Show(bool show)
	{
		if (m_wTree)
			m_wTree.SetVisible(show);
	}

	// Rebuild the flattened row list from the live editable-entity hierarchy, then repaint.
	void Rebuild()
	{
		if (!m_bBound)
			return;

		RebuildPlayers();

		m_Rows.Clear();
		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (!core)
		{
			Repaint();
			return;
		}

		set<SCR_EditableEntityComponent> all = new set<SCR_EditableEntityComponent>();
		core.GetAllEntities(all);
		UpdateForceOverview(all);

		// The force cards are the UNIT view's first level.
		if (m_FilterCat == CAT_UNIT && !m_SelectedForce && m_Search.IsEmpty())
		{
			Repaint();
			return;
		}

		// SEARCHING flattens to matches only - a hit must never sit inside a collapsed subtree.
		if (!m_Search.IsEmpty())
		{
			array<ref DCO_TreeRow> cand = {};
			array<string> keys = {};
			foreach (SCR_EditableEntityComponent se : all)
			{
				if (!se)
					continue;
				if (m_SelectedForce && !IsInsideForce(se, m_SelectedForce))
					continue;
				if (se.GetPlayerID() > 0 && se.GetEntityType() == EEditableEntityType.CHARACTER)
					continue;	// players live in the Players box below, same as the browse tree.
				if (m_FilterCat != CAT_ALL && EntityCategory(se) != m_FilterCat)
					continue;
				DCO_TreeRow row = new DCO_TreeRow();
				row.m_Entity = se;
				row.m_Depth = 0;
				row.m_bHasChildren = false;	// flat result list - no chevron, no expansion.
				row.m_bExpanded = false;
				row.m_Label = EntityLabel(se);
				row.m_Icon = EntityIcon(se);
				row.m_App6 = App6IconFor(se);
				row.m_bFaction = se.GetEntityType() == EEditableEntityType.FACTION;
				cand.Insert(row);
				keys.Insert(row.m_Label);
			}
			array<int> hits = {};
			WidgetManager.SearchLocalized(m_Search, keys, hits);
			foreach (int hi : hits)
			{
				if (hi >= 0 && hi < cand.Count())
					m_Rows.Insert(cand[hi]);
			}
			Repaint();
			return;
		}

		// A selected force becomes the roster root.
		if (m_FilterCat == CAT_UNIT && m_SelectedForce)
		{
			foreach (SCR_EditableEntityComponent candidate : all)
			{
				if (!candidate || candidate == m_SelectedForce || !IsInsideForce(candidate, m_SelectedForce))
					continue;
				if (candidate.GetPlayerID() > 0 && candidate.GetEntityType() == EEditableEntityType.CHARACTER)
					continue;	// players remain in the dedicated Players box.
				if (EntityCategory(candidate) != CAT_UNIT)
					continue;

				SCR_EditableEntityComponent parent = candidate.GetParentEntity();
				if (parent && parent != m_SelectedForce && EntityCategory(parent) == CAT_UNIT && IsInsideForce(parent, m_SelectedForce))
					continue;	// emitted recursively below its group / unit parent.

				Flatten(candidate, 0);
			}
			Repaint();
			return;
		}

		// Roots = entities with no editable parent.
		foreach (SCR_EditableEntityComponent e : all)
		{
			if (!e)
				continue;
			if (e.GetParentEntity())
				continue;	// not a root - emitted under its parent.
			Flatten(e, 0);
		}
		Repaint();
	}

	// Compact live readiness summary.
	protected void UpdateForceOverview(notnull set<SCR_EditableEntityComponent> all)
	{
		m_ForceOverviewTick++;
		bool refreshRounds = m_ForceOverviewTick == 1 || m_ForceOverviewTick >= 5;
		if (refreshRounds)
			m_ForceOverviewTick = 0;

		array<SCR_EditableEntityComponent> factions = {};
		foreach (SCR_EditableEntityComponent candidate : all)
		{
			if (!candidate || candidate.GetEntityType() != EEditableEntityType.FACTION)
				continue;
			InsertSortedFaction(factions, candidate);
		}
		m_AvailableForces.Clear();
		foreach (SCR_EditableEntityComponent availableForce : factions)
			m_AvailableForces.Insert(availableForce);
		ReconcileWatchedForces(factions);
		if (m_wForcePageText)
			m_wForcePageText.SetText(string.Format("%1/%2", m_WatchedForceKeys.Count(), factions.Count()));
		if (m_btnForcePrev)
			m_btnForcePrev.SetVisible(false);
		if (m_btnForceNext)
			m_btnForceNext.SetVisible(false);

		m_ForceEntities.Clear();
		int slot;
		for (slot = 0; slot < m_WatchedForceKeys.Count() && slot < m_ForceRows.Count(); slot++)
		{
			SCR_EditableEntityComponent faction = FindForceByKey(factions, m_WatchedForceKeys[slot]);
			if (!faction)
				continue;
			string forceCacheKey = ForceKey(faction);
			int total;
			int ready;
			float healthTotal;
			int healthSamples;
			int rounds;
			foreach (SCR_EditableEntityComponent member : all)
			{
				if (!member || member.GetEntityType() != EEditableEntityType.CHARACTER)
					continue;
				if (member.GetPlayerID() > 0 || !IsInsideForce(member, faction))
					continue;
				total++;
				if (!member.IsDestroyed())
					ready++;
				IEntity owner = member.GetOwner();
				DamageManagerComponent damage;
				if (owner)
					damage = DamageManagerComponent.Cast(owner.FindComponent(DamageManagerComponent));
				if (damage)
				{
					healthTotal += damage.GetHealthScaled();
					healthSamples++;
				}
				if (refreshRounds && owner)
					rounds += CountCharacterRounds(owner);
			}
			int cachedRounds;
			bool hasCachedRounds = m_ForceRoundCache.Find(forceCacheKey, cachedRounds);
			if (refreshRounds || !hasCachedRounds)
			{
				cachedRounds = rounds;
				m_ForceRoundCache.Set(forceCacheKey, rounds);
			}
			m_ForceEntities.Insert(faction);
			int health = 0;
			if (healthSamples > 0)
				health = Math.Clamp(Math.Round((healthTotal / healthSamples) * 100.0), 0, 100);
			if (m_ForceRows[slot])
				m_ForceRows[slot].SetVisible(true);
			if (m_ForceLabels[slot])
				m_ForceLabels[slot].SetText(BoundDisplay(EntityLabel(faction), 34));
			if (m_ForceHealth[slot])
				m_ForceHealth[slot].SetText(string.Format("%1%%", health));
			if (m_ForceReady[slot])
				m_ForceReady[slot].SetText(string.Format("%1/%2 READY", ready, total));
			if (m_ForceAmmo[slot])
			{
				int perMember;
				if (total > 0)
					perMember = cachedRounds / total;
				m_ForceAmmo[slot].SetText(string.Format("%1 RDS · %2/M", cachedRounds, perMember));
			}
			if (m_ForceBars[slot])
				m_ForceBars[slot].SetCurrent(health / 100.0);
			if (m_ForceIcons[slot])
			{
				ResourceName forceIcon = App6IconFor(faction);
				if (!forceIcon.IsEmpty())
				{
					// ImageWidget texture loads are state changes, not repaint operations.
					if (m_ForceLoadedIcons[slot] != forceIcon)
					{
						m_ForceIcons[slot].LoadImageTexture(0, forceIcon);
						m_ForceLoadedIcons[slot] = forceIcon;
					}
					m_ForceIcons[slot].SetVisible(true);
				}
				else
				{
					m_ForceLoadedIcons[slot] = ResourceName.Empty;
					m_ForceIcons[slot].SetVisible(false);
				}
			}

			bool hidden = m_HiddenForces.Contains(faction);
			if (m_ForceRows[slot])
			{
				if (hidden)
					m_ForceRows[slot].SetOpacity(0.36);
				else
					m_ForceRows[slot].SetOpacity(1.0);
			}
			if (m_ForceHideIcons[slot])
			{
				if (hidden)
					m_ForceHideIcons[slot].SetColor(DCO_GMTheme.Get().m_DisabledColor);
				else
					m_ForceHideIcons[slot].SetColor(DCO_GMTheme.Get().m_AccentColor);
			}
			if (m_ForceBackgrounds[slot])
			{
				if (faction == m_SelectedForce)
				{
					m_ForceBackgrounds[slot].SetColor(DCO_GMTheme.Get().m_AccentColor);
					m_ForceBackgrounds[slot].SetOpacity(0.24);
				}
				else
				{
					m_ForceBackgrounds[slot].SetColorInt(0xFF1A1F26);
					m_ForceBackgrounds[slot].SetOpacity(0.78);
				}
			}
		}
		for (int i = slot; i < m_ForceRows.Count(); i++)
		{
			if (m_ForceRows[i])
				m_ForceRows[i].SetVisible(false);
		}
	}

	// Watched slots are intentionally client-local: they filter replicated editor entities and never mutate gameplay state.
	protected void ReconcileWatchedForces(notnull array<SCR_EditableEntityComponent> factions)
	{
		set<FactionKey> retained = new set<FactionKey>();
		for (int i = m_WatchedForceKeys.Count() - 1; i >= 0; i--)
		{
			FactionKey key = m_WatchedForceKeys[i];
			if (key.IsEmpty() || !FindForceByKey(factions, key) || retained.Contains(key))
			{
				m_WatchedForceKeys.RemoveOrdered(i);
				continue;
			}
			retained.Insert(key);
		}

		foreach (SCR_EditableEntityComponent faction : factions)
		{
			if (m_WatchedForceKeys.Count() >= FORCE_SLOTS)
				break;
			FactionKey key = ForceKey(faction);
			if (key.IsEmpty() || retained.Contains(key))
				continue;
			m_WatchedForceKeys.Insert(key);
			retained.Insert(key);
		}

		if (m_SelectedForce && !factions.Contains(m_SelectedForce))
			m_SelectedForce = null;
	}

	protected SCR_EditableEntityComponent FindForceByKey(notnull array<SCR_EditableEntityComponent> factions, FactionKey key)
	{
		foreach (SCR_EditableEntityComponent faction : factions)
		{
			if (faction && ForceKey(faction) == key)
				return faction;
		}
		return null;
	}

	protected string ForceKey(SCR_EditableEntityComponent factionEntity)
	{
		if (!factionEntity)
			return "";
		Faction faction = factionEntity.GetFaction();
		if (faction && !faction.GetFactionKey().IsEmpty())
			return faction.GetFactionKey();
		SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.Cast(factionEntity.GetInfo());
		if (info && !info.GetFactionKey().IsEmpty())
			return info.GetFactionKey();
		return EntityLabel(factionEntity);
	}

	protected void InsertSortedFaction(notnull array<SCR_EditableEntityComponent> factions, SCR_EditableEntityComponent faction)
	{
		FactionKey key = ForceKey(faction);
		int insertAt = factions.Count();
		for (int i = 0; i < factions.Count(); i++)
		{
			if (DCO_FactionCatalog.Compare(key, ForceKey(factions[i])) < 0)
			{
				insertAt = i;
				break;
			}
		}
		factions.InsertAt(faction, insertAt);
	}

	protected bool IsInsideForce(SCR_EditableEntityComponent entity, SCR_EditableEntityComponent force)
	{
		if (!entity || !force)
			return false;

		Faction forceFaction = force.GetFaction();
		SCR_EditableEntityComponent cursor = entity;
		while (cursor)
		{
			if (cursor == force)
				return true;
			if (forceFaction && cursor.GetFaction() == forceFaction)
				return true;
			cursor = cursor.GetParentEntity();
		}
		return false;
	}

	bool OnForceButton(int index, bool visibility)
	{
		if (index < 0 || index >= m_ForceEntities.Count())
			return false;
		SCR_EditableEntityComponent faction = m_ForceEntities[index];
		if (!faction)
			return false;

		if (visibility)
		{
			if (m_HiddenForces.Contains(faction))
				m_HiddenForces.RemoveItem(faction);
			else
			{
				m_HiddenForces.Insert(faction);
				if (m_SelectedForce == faction)
					m_SelectedForce = null;
			}
		}
		else
		{
			if (m_HiddenForces.Contains(faction))
				m_HiddenForces.RemoveItem(faction);
			if (m_SelectedForce == faction)
				m_SelectedForce = null;
			else
				m_SelectedForce = faction;
		}

		m_Page = 0;
		Rebuild();
		return true;
	}

	bool OnForceWatchMenu(int index, int x, int y)
	{
		if (!m_Menu || index < 0 || index >= m_ForceEntities.Count() || m_AvailableForces.IsEmpty())
			return false;
		SCR_EditableEntityComponent anchorForce = m_ForceEntities[index];
		if (!anchorForce)
			return false;

		DCO_GMContextMenuBridge.ClaimRightClick();
		m_MenuX = x;
		m_MenuY = y;
		m_ForcePickSlot = index;
		m_ForcePickPage = 0;
		m_ForcePickKeys.Clear();
		foreach (SCR_EditableEntityComponent faction : m_AvailableForces)
			m_ForcePickKeys.Insert(ForceKey(faction));
		ShowForceWatchPage(anchorForce);
		return true;
	}

	protected void ShowForceWatchPage(SCR_EditableEntityComponent anchorForce)
	{
		if (!m_Menu || m_ForcePickSlot < 0 || m_ForcePickSlot >= m_WatchedForceKeys.Count() || m_ForcePickKeys.IsEmpty())
			return;

		int pageCount = Math.Max(1, (m_ForcePickKeys.Count() + FORCE_PICK_PAGE_SIZE - 1) / FORCE_PICK_PAGE_SIZE);
		m_ForcePickPage = Math.Clamp(m_ForcePickPage, 0, pageCount - 1);
		array<string> labels = {};
		array<int> ids = {};
		if (m_ForcePickPage > 0)
		{
			labels.Insert("< PREVIOUS");
			ids.Insert(FORCE_PICK_PREV);
		}

		int first = m_ForcePickPage * FORCE_PICK_PAGE_SIZE;
		int last = Math.Min(first + FORCE_PICK_PAGE_SIZE, m_ForcePickKeys.Count());
		int selectedId = -1;
		for (int i = first; i < last; i++)
		{
			FactionKey key = m_ForcePickKeys[i];
			string label = DCO_FactionCatalog.NameFor(key);
			int watchedSlot = m_WatchedForceKeys.Find(key);
			if (watchedSlot >= 0)
				label = label + string.Format("  ·  SLOT %1", watchedSlot + 1);
			labels.Insert(label);
			ids.Insert(FORCE_PICK_SELECT_BASE + i);
			if (key == m_WatchedForceKeys[m_ForcePickSlot])
				selectedId = FORCE_PICK_SELECT_BASE + i;
		}

		if (m_ForcePickPage < pageCount - 1)
		{
			labels.Insert("NEXT >");
			ids.Insert(FORCE_PICK_NEXT);
		}

		string subtitle = string.Format("Choose the faction shown on command card %1  ·  PAGE %2/%3", m_ForcePickSlot + 1, m_ForcePickPage + 1, pageCount);
		m_Menu.ShowTitledDetailed(labels, ids, m_MenuX, m_MenuY, string.Format("FORCE WATCH  ·  SLOT %1", m_ForcePickSlot + 1), subtitle, selectedId, m_MenuCb, anchorForce);
	}

	protected bool HandleForceWatchAction(int actionId, SCR_EditableEntityComponent anchorForce)
	{
		if (actionId == FORCE_PICK_PREV)
		{
			m_ForcePickPage--;
			ShowForceWatchPage(anchorForce);
			return true;
		}
		if (actionId == FORCE_PICK_NEXT)
		{
			m_ForcePickPage++;
			ShowForceWatchPage(anchorForce);
			return true;
		}
		if (actionId < FORCE_PICK_SELECT_BASE)
			return false;

		int selectedIndex = actionId - FORCE_PICK_SELECT_BASE;
		if (selectedIndex < 0 || selectedIndex >= m_ForcePickKeys.Count() || m_ForcePickSlot < 0 || m_ForcePickSlot >= m_WatchedForceKeys.Count())
			return true;
		FactionKey selectedKey = m_ForcePickKeys[selectedIndex];
		if (!FindForceByKey(m_AvailableForces, selectedKey))
			return true;

		FactionKey displacedKey = m_WatchedForceKeys[m_ForcePickSlot];
		int otherSlot = m_WatchedForceKeys.Find(selectedKey);
		if (otherSlot >= 0 && otherSlot != m_ForcePickSlot)
			m_WatchedForceKeys[otherSlot] = displacedKey;
		m_WatchedForceKeys[m_ForcePickSlot] = selectedKey;
		m_SelectedForce = null;
		m_Page = 0;
		Rebuild();
		return true;
	}

	// Builds a complete inventory view by collecting every item from the character's managed storage hierarchy.
	protected int CountCharacterRounds(notnull IEntity owner)
	{
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
			return 0;
		array<IEntity> items = {};
		inventory.GetItems(items);
		int rounds;
		foreach (IEntity item : items)
		{
			if (!item)
				continue;
			BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
			if (magazine)
				rounds += magazine.GetAmmoCount();
		}
		return rounds;
	}

	protected void PollSearch()
	{
		if (!m_wSearch)
			return;
		string cur = m_wSearch.GetText();
		if (cur == m_LastSearch)
			return;
		m_LastSearch = cur;
		m_Search = cur;
		m_Page = 0;	// a changed query reshapes the list - the old page offset is meaningless.
		Rebuild();
	}

	void OnSearchChanged()
	{
		PollSearch();
	}

	protected void Flatten(SCR_EditableEntityComponent e, int depth)
	{
		if (e.GetPlayerID() > 0 && e.GetEntityType() == EEditableEntityType.CHARACTER)
			return;
		if (!SubtreeMatches(e))
			return;	// neither this entity nor any descendant matches the active type filter.

		set<SCR_EditableEntityComponent> kids = new set<SCR_EditableEntityComponent>();
		e.GetChildren(kids, true);
		bool hasKids = kids.Count() > 0;
		bool expanded = !m_Collapsed.Contains(e);

		DCO_TreeRow row = new DCO_TreeRow();
		row.m_Entity = e;
		row.m_Depth = depth;
		row.m_bHasChildren = hasKids;
		row.m_bExpanded = expanded;
		row.m_Label = EntityLabel(e);
		row.m_Icon = EntityIcon(e);
		row.m_App6 = App6IconFor(e);
		row.m_bFaction = e.GetEntityType() == EEditableEntityType.FACTION;
		m_Rows.Insert(row);

		if (hasKids && expanded)
		{
			foreach (SCR_EditableEntityComponent k : kids)
			{
				if (k)
					Flatten(k, depth + 1);
			}
		}
	}

	protected string EntityLabel(SCR_EditableEntityComponent e)
	{
		int pid = e.GetPlayerID();
		if (pid > 0)
		{
			PlayerManager pm = GetGame().GetPlayerManager();
			if (pm)
			{
				string un = pm.GetPlayerName(pid);
				if (!un.IsEmpty())
					return un;
			}
		}
		SCR_UIInfo info = e.GetInfo();
		if (info)
		{
			string nm = info.GetName();
			if (!nm.IsEmpty())
				return nm;
		}
		return "Entity";
	}

	protected string BoundDisplay(string value, int maxChars)
	{
		if (maxChars < 4 || value.Length() <= maxChars)
			return value;
		return value.Substring(0, maxChars - 3) + "...";
	}

	protected ResourceName EntityIcon(SCR_EditableEntityComponent e)
	{
		SCR_EditableEntityUIInfo eui = SCR_EditableEntityUIInfo.Cast(e.GetInfo());
		if (eui)
			return eui.GetImage();
		return ResourceName.Empty;
	}

	protected ResourceName App6IconFor(SCR_EditableEntityComponent e)
	{
		if (!e)
			return ResourceName.Empty;
		SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.Cast(e.GetInfo());
		if (!info)
			return ResourceName.Empty;
		if (e.GetEntityType() == EEditableEntityType.FACTION)
			return DCO_App6Icons.FactionIcon(ForceKey(e));
		array<EEditableEntityLabel> labels = {};
		info.GetEntityLabels(labels);
		return DCO_App6Icons.GetIcon(labels, info.GetName(), info.GetFactionKey(), e.GetEntityType());
	}

	// One primary type-filter bucket per entity.
	protected int EntityCategory(SCR_EditableEntityComponent e)
	{
		EEditableEntityType t = e.GetEntityType();
		switch (t)
		{
			case EEditableEntityType.CHARACTER:
			case EEditableEntityType.GROUP:
			case EEditableEntityType.FACTION:
				return CAT_UNIT;
			case EEditableEntityType.VEHICLE:
				return CAT_VEH;
			case EEditableEntityType.COMMENT:
				return CAT_LOC;
		}
		if (SCR_Enum.HasFlag(e.GetEntityFlags(), EEditableEntityFlag.HAS_AREA))
			return CAT_AREA;
		return CAT_OBJ;
	}

	protected bool SubtreeMatches(SCR_EditableEntityComponent e)
	{
		if (m_FilterCat == CAT_ALL)
			return true;
		if (EntityCategory(e) == m_FilterCat)
			return true;
		set<SCR_EditableEntityComponent> kids = new set<SCR_EditableEntityComponent>();
		e.GetChildren(kids, true);
		foreach (SCR_EditableEntityComponent k : kids)
		{
			if (k && SubtreeMatches(k))
				return true;
		}
		return false;
	}

	protected static const int PLATE_REST = 0xFF292E36;	// slate 0.16 0.18 0.21 - keep in sync with the layout plates.
	protected static const int TEXT_ON_ACCENT = 0xFF0B0D10;	// panel-dark text on a filled accent pill.

	protected void HighlightFilter()
	{
		TextWidget head = TextWidget.Cast(m_wTree.FindAnyWidget("DCO_TreeHeader"));
		if (head)
		{
			string crumb = "EDIT";
			switch (m_FilterCat)
			{
				case CAT_UNIT:
				{
					crumb = "EDIT · FORCES";
					if (m_SelectedForce)
						crumb = "EDIT · " + EntityLabel(m_SelectedForce);
					break;
				}
				case CAT_VEH:  crumb = "EDIT · VEHICLES";  break;
				case CAT_OBJ:  crumb = "EDIT · OBJECTS";   break;
				case CAT_LOC:  crumb = "EDIT · LOCATIONS"; break;
				case CAT_AREA: crumb = "EDIT · AREAS";     break;
			}
			head.SetText(BoundDisplay(crumb, 34));
		}
		if (m_wForceOverview)
			m_wForceOverview.SetVisible(m_FilterCat == CAT_UNIT);

		DCO_GMTheme theme = DCO_GMTheme.Get();
		for (int i = 0; i < m_CatBtns.Count(); i++)
		{
			ButtonWidget b = m_CatBtns[i];
			if (!b)
				continue;
			TextWidget lbl = TextWidget.Cast(b.FindAnyWidget(b.GetName() + "_Label"));
			ImageWidget icon = ImageWidget.Cast(b.FindAnyWidget(b.GetName() + "_Icon"));
			Widget plate = b.FindAnyWidget(b.GetName() + "_Bg");
			bool active = m_CatValues[i] == m_FilterCat;
			if (plate)
			{
				if (active)
					plate.SetColor(theme.m_AccentColor);
				else
					plate.SetColorInt(PLATE_REST);
			}
			if (lbl)
			{
				if (active)
					lbl.SetColorInt(TEXT_ON_ACCENT);
				else
					lbl.SetColor(theme.m_MutedColor);
			}
			if (icon)
			{
				if (active)
					icon.SetColorInt(TEXT_ON_ACCENT);
				else
					icon.SetColor(theme.m_MutedColor);
			}
		}
	}

	protected void Repaint()
	{
		int total = m_Rows.Count();
		int pages = (total + ROWS - 1) / ROWS;
		if (pages < 1)
			pages = 1;
		if (m_Page >= pages)
			m_Page = pages - 1;
		if (m_Page < 0)
			m_Page = 0;

		DCO_GMTheme theme = DCO_GMTheme.Get();
		for (int r = 0; r < ROWS; r++)
		{
			int idx = m_Page * ROWS + r;
			ButtonWidget btn = m_RowBtns[r];
			TextWidget lbl = m_RowLabels[r];
			Widget statusHost = m_RowStatusHosts[r];
			TextWidget status = m_RowStatus[r];
			ImageWidget ico = m_RowIcons[r];
			if (idx >= total)
			{
				if (btn) btn.SetVisible(false);
				continue;
			}
			DCO_TreeRow row = m_Rows[idx];
			if (btn) btn.SetVisible(true);

			// Indent by depth.
			string indent = "";
			for (int d = 0; d < row.m_Depth; d++)
				indent = indent + "   ";
			string prefix = indent;

			ImageWidget fold = m_RowFold[r];
			if (fold)
			{
				fold.SetVisible(true);
				if (row.m_bHasChildren)
				{
					float foldRot = 0;
					if (row.m_bExpanded)
						foldRot = 90;
					fold.SetRotation(foldRot);
					fold.SetColor(theme.m_AccentColor);
					fold.SetOpacity(1.0);
				}
				else
					fold.SetOpacity(0.0);
			}

			bool isDead = IsDeadCharacter(row.m_Entity);
			string statusText = CharacterStatusColumn(row.m_Entity);
			if (statusHost)
				statusHost.SetVisible(!statusText.IsEmpty());
			if (status)
				status.SetText(statusText);

			if (lbl)
			{
				lbl.SetDesiredFontSize(18);
				lbl.SetMinFontSize(9);
				lbl.SetText(BoundDisplay(prefix + row.m_Label, 48));
				if (isDead)
				{
					lbl.SetColor(Color.FromRGBA(145, 150, 154, 255));
					if (status) status.SetColor(Color.FromRGBA(145, 150, 154, 255));
				}
				else if (row.m_Entity == m_LastSelected)
				{
					lbl.SetColor(Color.FromRGBA(255, 255, 255, 255));	// selected row highlight.
					if (status) status.SetColor(Color.FromRGBA(255, 255, 255, 255));
				}
				else if (row.m_bHasChildren)
				{
					lbl.SetColor(theme.m_AccentColor);
					if (status) status.SetColor(theme.m_AccentColor);
				}
				else
				{
					lbl.SetColor(theme.m_TextColor);
					if (status) status.SetColor(theme.m_TextColor);
				}
			}
			if (ico)
			{
				ResourceName useIcon = row.m_Icon;
				bool isApp6 = !row.m_App6.IsEmpty();
				if (isApp6)
					useIcon = row.m_App6;
				if (!useIcon.IsEmpty())
				{
					if (m_RowLoadedIcons[r] != useIcon)
					{
						ico.LoadImageTexture(0, useIcon);
						m_RowLoadedIcons[r] = useIcon;
					}
					ico.SetOpacity(1.0);
					if (isDead)
						ico.SetColor(Color.FromRGBA(145, 150, 154, 255));
					else
						ico.SetColor(Color.FromRGBA(255, 255, 255, 255));
					if (isApp6)
					{
						// Faction affiliation frames sit a touch smaller than unit icons; units at the default 18.
						if (row.m_bFaction)
							ico.SetSize(14, 14);
						else
							ico.SetSize(18, 18);
					}
					else
					{
						ico.SetSize(18, 18);	// restore the default size on a reused pool row showing a native icon.
					}
					ico.SetVisible(true);
				}
				else
				{
					m_RowLoadedIcons[r] = ResourceName.Empty;
					ico.SetVisible(false);
				}
			}
		}

		// Never present an unexplained blank panel.
		bool forceOverviewOnly = m_FilterCat == CAT_UNIT && !m_SelectedForce && m_Search.IsEmpty();
		if (total == 0 && !forceOverviewOnly && !m_RowBtns.IsEmpty())
		{
			ButtonWidget emptyRow = m_RowBtns[0];
			if (emptyRow)
				emptyRow.SetVisible(true);
			if (!m_RowLabels.IsEmpty() && m_RowLabels[0])
			{
				// Let longer empty-state copy shrink within the narrow Force Command panel.
				m_RowLabels[0].SetDesiredFontSize(14);
				m_RowLabels[0].SetMinFontSize(9);
				if (!m_Search.IsEmpty())
					m_RowLabels[0].SetText("NO DEPLOYED ENTITIES MATCH THE SEARCH");
				else if (m_SelectedForce)
					m_RowLabels[0].SetText("NO DEPLOYED ENTITIES IN THIS FORCE");
				else
					m_RowLabels[0].SetText("NO DEPLOYED ENTITIES IN THIS CATEGORY");
				m_RowLabels[0].SetColor(theme.m_MutedColor);
			}
			if (!m_RowIcons.IsEmpty() && m_RowIcons[0])
				m_RowIcons[0].SetVisible(false);
			if (!m_RowFold.IsEmpty() && m_RowFold[0])
				m_RowFold[0].SetVisible(false);
			if (!m_RowStatusHosts.IsEmpty() && m_RowStatusHosts[0])
				m_RowStatusHosts[0].SetVisible(false);
		}

		if (m_wPageText)
			m_wPageText.SetText(string.Format("%1 / %2", m_Page + 1, pages));
	}

	bool OnButton(Widget w)
	{
		if (w == m_btnForcePrev || w == m_btnForceNext)
			return true;
		if (w == m_btnPrev)
		{
			m_Page--;
			Repaint();
			return true;
		}
		if (w == m_btnNext)
		{
			m_Page++;
			Repaint();
			return true;
		}
		for (int i = 0; i < m_CatBtns.Count(); i++)
		{
			if (w == m_CatBtns[i])
			{
				m_FilterCat = m_CatValues[i];
				if (m_FilterCat != CAT_UNIT)
					m_SelectedForce = null;
				HighlightFilter();
				m_Page = 0;
				Rebuild();
				return true;
			}
		}
		for (int r = 0; r < m_RowBtns.Count(); r++)
		{
			if (w == m_RowBtns[r])
			{
				OnRowClicked(r);
				return true;
			}
		}
		int p = PlayerRowIndexOf(w);
		if (p >= 0 && p < m_PlayerEntities.Count())
		{
			Select(m_PlayerEntities[p]);
			return true;
		}
		return false;
	}

	protected void OnRowClicked(int r)
	{
		int idx = m_Page * ROWS + r;
		if (idx < 0 || idx >= m_Rows.Count())
			return;
		DCO_TreeRow row = m_Rows[idx];
		if (!row.m_Entity)
			return;

		if (row.m_bHasChildren)
		{
			// Toggle expand/collapse on a parent row.
			if (m_Collapsed.Contains(row.m_Entity))
				m_Collapsed.RemoveItem(row.m_Entity);
			else
				m_Collapsed.Insert(row.m_Entity);
			Rebuild();
			return;
		}

		Select(row.m_Entity);
	}

	// Single-select: deselect the previous, select this.
	protected void Select(SCR_EditableEntityComponent e)
	{
		if (!e)
			return;
		if (m_LastSelected && m_LastSelected != e)
			m_LastSelected.SetEntityState(EEditableEntityState.SELECTED, false);
		e.SetEntityState(EEditableEntityState.SELECTED, true);
		m_LastSelected = e;
		DCO_GMGizmo.NotifyDeliberatePick(e);	// Direct tree selection may re-pin the precise gizmo.
	}

	protected int RowIndexOf(Widget w)
	{
		for (int r = 0; r < m_RowBtns.Count(); r++)
		{
			if (w == m_RowBtns[r])
				return r;
		}
		return -1;
	}

	protected SCR_EditableEntityComponent EntityAt(int r)
	{
		if (r < 0)
			return null;
		int idx = m_Page * ROWS + r;
		if (idx < 0 || idx >= m_Rows.Count())
			return null;
		return m_Rows[idx].m_Entity;
	}

	// Populate the Players box from the connected players + their controlled units.
	protected void RebuildPlayers()
	{
		m_PlayerEntities.Clear();
		map<int, SCR_EditableEntityComponent> players = new map<int, SCR_EditableEntityComponent>();
		if (m_PlayersMgr)
			m_PlayersMgr.GetPlayers(players);
		PlayerManager pm = GetGame().GetPlayerManager();

		for (int i = 0; i < m_PlayerBtns.Count(); i++)
		{
			ButtonWidget b = m_PlayerBtns[i];
			TextWidget lbl = m_PlayerLabels[i];
			Widget fpsHost = m_PlayerFpsHosts[i];
			TextWidget fpsLabel = m_PlayerFps[i];
			Widget healthHost = m_PlayerHealthHosts[i];
			TextWidget healthLabel = m_PlayerHealth[i];
			ImageWidget ico = m_PlayerIcons[i];
			if (i < players.Count())
			{
				int pid = players.GetKey(i);
				SCR_EditableEntityComponent e = players.GetElement(i);
				m_PlayerEntities.Insert(e);

				string name = "Player " + pid.ToString();
				if (pm)
				{
					string un = pm.GetPlayerName(pid);
					if (!un.IsEmpty())
						name = un;
				}
				Color rowColor = DCO_GMTheme.Get().m_TextColor;
				string fpsText = "";
				int fps = DCO_FpsMonitorClient.Get().GetFps(pid);
				if (fps >= 0)
				{
					fpsText = string.Format("%1 FPS", fps);
					if (fps >= 45)
						rowColor = Color.FromInt(DCO_GMTheme.SEM_FRIENDLY);	// green - healthy.
					else if (fps >= 25)
						rowColor = Color.FromRGBA(235, 185, 80, 255);	// amber - strained.
					else
						rowColor = Color.FromInt(DCO_GMTheme.SEM_HOSTILE);	// red - struggling.
				}
				else if (DCO_FpsMonitorClient.Get().IsWatched(pid))
				{
					fpsText = "-- FPS";	// watch armed by double-click; first sample lands with the next poll.
				}
				string healthText = CharacterStatusColumn(e);
				bool isDead = e && IsDeadCharacter(e);
				if (lbl)
				{
					if (isDead)
						rowColor = Color.FromRGBA(145, 150, 154, 255);
					lbl.SetText(BoundDisplay(name, 34));
					lbl.SetColor(rowColor);
				}
				if (fpsHost)
					fpsHost.SetVisible(!fpsText.IsEmpty());
				if (fpsLabel)
				{
					fpsLabel.SetText(fpsText);
					fpsLabel.SetColor(rowColor);
				}
				if (healthHost)
					healthHost.SetVisible(!healthText.IsEmpty());
				if (healthLabel)
				{
					healthLabel.SetText(healthText);
					if (isDead)
						healthLabel.SetColor(Color.FromRGBA(145, 150, 154, 255));
					else
						healthLabel.SetColor(DCO_GMTheme.Get().m_TextColor);
				}
				if (ico)
				{
					ResourceName icon = ResourceName.Empty;
					if (e)
						icon = EntityIcon(e);
					if (!icon.IsEmpty())
					{
						ico.LoadImageTexture(0, icon);
						if (e && IsDeadCharacter(e))
							ico.SetColor(Color.FromRGBA(145, 150, 154, 255));
						else
							ico.SetColor(Color.FromRGBA(255, 255, 255, 255));
						ico.SetVisible(true);
					}
					else
					{
						ico.SetVisible(false);
					}
				}
				if (b)
					b.SetVisible(true);
			}
			else if (b)
			{
				b.SetVisible(false);
			}
		}
		if (players.Count() > PLAYER_ROWS)
			Print(string.Format("[DCO-GM] %1 players but %2 player rows - extras not listed", players.Count(), PLAYER_ROWS), LogLevel.WARNING);
	}

	protected int PlayerRowIndexOf(Widget w)
	{
		for (int i = 0; i < m_PlayerBtns.Count(); i++)
		{
			if (w == m_PlayerBtns[i])
				return i;
		}
		return -1;
	}

	// The entity behind a clicked button, whether it's a tree row or a player row.
	protected SCR_EditableEntityComponent EntityForButton(Widget w)
	{
		int r = RowIndexOf(w);
		if (r >= 0)
			return EntityAt(r);
		int p = PlayerRowIndexOf(w);
		if (p >= 0 && p < m_PlayerEntities.Count())
			return m_PlayerEntities[p];
		return null;
	}

	bool OnRowDoubleClick(Widget w)
	{
		int p = PlayerRowIndexOf(w);
		if (p >= 0)
		{
			if (p >= m_PlayerEntities.Count() || !m_PlayerEntities[p])
				return false;
			int pid = m_PlayerEntities[p].GetPlayerID();
			if (pid <= 0)
				return false;
			DCO_FpsMonitorClient.Get().ToggleWatch(pid);
			RebuildPlayers();
			return true;
		}

		SCR_EditableEntityComponent e = EntityForButton(w);
		if (!e)
			return false;
		Select(e);
		if (m_LayersMgr && e.CanEnterLayer(m_LayersMgr))
		{
			m_LayersMgr.ToggleCurrentLayer(e);
			return true;
		}
		m_Actions.EditAttributes(e);
		return true;
	}

	bool OnRowRightClick(Widget w, int x, int y)
	{
		SCR_EditableEntityComponent e = EntityForButton(w);
		if (!e)
			return false;
		DCO_GMContextMenuBridge.ClaimRightClick();	// stop the world bridge from clobbering our tree menu.
		Select(e);

		array<string> labels = {};
		array<int> ids = {};
		if (e.GetEntityType() == EEditableEntityType.GROUP)
		{
			DCO_GMGroupOrders.BuildRoot(labels, ids);
		}
		else
		{
			BuildEntityMenu(e, labels, ids);
		}
		m_MenuX = x;	// remember where the menu opened so submenu drill-ins re-show in place.
		m_MenuY = y;
		if (m_Menu && !labels.IsEmpty())
			m_Menu.Show(labels, ids, x, y, m_MenuCb, e);
		return true;
	}

	protected string EntityNoun(SCR_EditableEntityComponent e)
	{
		switch (e.GetEntityType())
		{
			case EEditableEntityType.CHARACTER: return "Unit";
			case EEditableEntityType.VEHICLE:   return "Vehicle";
			case EEditableEntityType.COMMENT:   return "Location";
			case EEditableEntityType.FACTION:   return "Faction";
			case EEditableEntityType.GROUP:     return "Group";
		}
		if (SCR_Enum.HasFlag(e.GetEntityFlags(), EEditableEntityFlag.HAS_AREA))
			return "Area";
		return "Object";
	}

	// Compact casualty/health state for character rows.
	protected bool IsDeadCharacter(SCR_EditableEntityComponent e)
	{
		return e && e.GetEntityType() == EEditableEntityType.CHARACTER && e.IsDestroyed();
	}

	protected string CharacterStatus(SCR_EditableEntityComponent e)
	{
		if (!e || e.GetEntityType() != EEditableEntityType.CHARACTER)
			return "";
		if (e.IsDestroyed())
			return "  ·  DEAD";

		IEntity owner = e.GetOwner();
		if (!owner)
			return "";
		DamageManagerComponent damage = DamageManagerComponent.Cast(owner.FindComponent(DamageManagerComponent));
		if (!damage)
			return "";
		int health = Math.Clamp(Math.Round(damage.GetHealthScaled() * 100.0), 0, 100);
		return string.Format("  ·  HP %1%%", health);
	}

	protected string CharacterStatusColumn(SCR_EditableEntityComponent e)
	{
		if (!e || e.GetEntityType() != EEditableEntityType.CHARACTER)
			return "";
		if (e.IsDestroyed())
			return "DEAD";

		IEntity owner = e.GetOwner();
		if (!owner)
			return "";
		DamageManagerComponent damage = DamageManagerComponent.Cast(owner.FindComponent(DamageManagerComponent));
		if (!damage)
			return "";
		int health = Math.Clamp(Math.Round(damage.GetHealthScaled() * 100.0), 0, 100);
		return string.Format("HP %1%%", health);
	}

	protected void BuildEntityMenu(SCR_EditableEntityComponent e, notnull array<string> labels, notnull array<int> ids)
	{
		int cat = EntityCategory(e);
		EEditableEntityType type = e.GetEntityType();
		string noun = EntityNoun(e);

		if (type != EEditableEntityType.FACTION)
		{
			labels.Insert("Orient Camera to " + noun);
			ids.Insert(DCO_GMUnitActions.ACT_ORIENT);

			string followLabel = "Follow " + noun + " with Camera";
			if (cat == CAT_LOC || cat == CAT_AREA)
				followLabel = "Lock Camera to " + noun;
			labels.Insert(followLabel);
			ids.Insert(DCO_GMUnitActions.ACT_FOLLOW);
		}

		if (type == EEditableEntityType.CHARACTER || type == EEditableEntityType.VEHICLE)
		{
			labels.Insert("Mark for Teleport");
			ids.Insert(DCO_GMUnitActions.ACT_MARK_TELEPORT);
		}

		// Take Control - only units/vehicles, and only when actually possessable.
		if ((cat == CAT_UNIT || cat == CAT_VEH) && m_Actions.CanPerform(DCO_GMUnitActions.ACT_CONTROL, e))
		{
			labels.Insert("Take Control");
			ids.Insert(DCO_GMUnitActions.ACT_CONTROL);
		}

		// Edit / attributes - anything that supports it.
		if (m_Actions.CanPerform(DCO_GMUnitActions.ACT_EDIT, e))
		{
			labels.Insert("Edit / Attributes");
			ids.Insert(DCO_GMUnitActions.ACT_EDIT);
		}

		// Character management reuses the existing Bifrost Arsenal and its authority-checked server route.
		if (type == EEditableEntityType.CHARACTER && !e.IsDestroyed())
		{
			labels.Insert("Edit Loadout");
			ids.Insert(DCO_GMUnitActions.ACT_ARSENAL);
			labels.Insert("Restock Ammo");
			ids.Insert(DCO_GMUnitActions.ACT_RESTOCK);
		}

		if (m_LayersMgr)
		{
			if (e.CanEnterLayer(m_LayersMgr))
			{
				labels.Insert("Enter Layer");
				ids.Insert(DCO_GMUnitActions.ACT_ENTER_LAYER);
			}
			labels.Insert("Group Selection into New Layer");
			ids.Insert(DCO_GMUnitActions.ACT_CREATE_LAYER);
			if (m_LayersMgr.GetCurrentLayer())
			{
				labels.Insert("Exit Layer");
				ids.Insert(DCO_GMUnitActions.ACT_EXIT_LAYER);
			}
		}
	}

	void OnContextAction(int actionId, SCR_EditableEntityComponent e)
	{
		if (!e)
			return;
		if (HandleForceWatchAction(actionId, e))
			return;
		if (DCO_GMGroupOrders.IsSubmenu(actionId))
		{
			array<string> labels = {};
			array<int> ids = {};
			DCO_GMGroupOrders.BuildSubmenu(actionId, labels, ids);
			if (m_Menu && !labels.IsEmpty())
				m_Menu.Show(labels, ids, m_MenuX, m_MenuY, m_MenuCb, e);
			return;
		}
		if (actionId == DCO_GMUnitActions.ACT_ENTER_LAYER)
		{
			if (m_LayersMgr)
				m_LayersMgr.ToggleCurrentLayer(e);
			return;
		}
		if (actionId == DCO_GMUnitActions.ACT_EXIT_LAYER)
		{
			if (m_LayersMgr)
				m_LayersMgr.SetCurrentLayerToParent();
			return;
		}
		if (actionId == DCO_GMUnitActions.ACT_CREATE_LAYER)
		{
			if (m_LayersMgr)
			{
				set<SCR_EditableEntityComponent> sel = new set<SCR_EditableEntityComponent>();
				SCR_BaseEditableEntityFilter.GetEnititiesStatic(sel, EEditableEntityState.SELECTED);
				if (sel.IsEmpty())
					sel.Insert(e);	// nothing multi-selected - group at least the clicked entity.
				vector pos = vector.Zero;
				IEntity own = e.GetOwner();
				if (own)
					pos = own.GetOrigin();
				m_LayersMgr.CreateNewLayerWithSelected(sel, pos);
			}
			return;
		}

		if (DCO_GMGroupOrders.IsOrderAction(actionId))
			m_GroupOrders.Apply(actionId, e);
		else
			m_Actions.Perform(actionId, e);
	}
}
