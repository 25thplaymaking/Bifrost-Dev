// Bifrost GM context-menu bridge.

class DCO_GMContextMenuBridge
{
	static const int NATIVE_BASE = 1000;	// menu action ids >= 1000 map into m_NativeActions.
	static const int ID_CREATE_PLAYER = 50;	// reserved id: "Create Player Here" routes to our MEN picker, not ActionPerform.
	static const int ID_HIDE_TERRAIN    = 60;
	static const int ID_RESTORE_TERRAIN = 61;
	static const int ID_SOUND           = 63;	// (deferred: sound emitter needs an imported soundset asset).
	static const int ID_MARKER          = 64;
	static const int ID_TRIGGER         = 65;
	static const int ID_INVULN          = 66;
	static const int ID_FPS             = 67;	// show this client's FPS.
	static const int ID_VISIBILITY      = 68;
	static const int ID_MARK_TP         = 69;	// hovered-unit: mark for teleport.
	static const int ID_TP_HERE         = 70;	// empty-ground: teleport the marked unit here.
	static const int ID_FLYBY           = 71;
	static const int ID_EDIT_LOADOUT    = 72;	// hovered-character: open the Bifrost Arsenal on the unit.
	static const int ID_RESET_LOADOUT   = 73;

	protected static const float PANEL_CLAIM_WINDOW_MS = 250.0;
	protected static float s_fPanelClaimAtMs = -1;
	static void ClaimRightClick()
	{
		BaseWorld world = GetGame().GetWorld();
		if (world)
			s_fPanelClaimAtMs = world.GetWorldTime();
	}
	// True when a panel claimed a right-click within the same click's window; always clears the claim.
	protected static bool ConsumeFreshClaim()
	{
		if (s_fPanelClaimAtMs < 0)
			return false;
		BaseWorld world = GetGame().GetWorld();
		bool fresh = world && (world.GetWorldTime() - s_fPanelClaimAtMs) <= PANEL_CLAIM_WINDOW_MS;
		s_fPanelClaimAtMs = -1;
		return fresh;
	}

	protected DCO_GMContextMenu m_Menu;	// shared, owned by the controller.
	protected ref DCO_GMGroupOrders m_GroupOrders = new DCO_GMGroupOrders();
	protected ref DCO_GMCreatePlayerPicker m_Picker;
	protected SCR_ContextActionsEditorComponent m_Ctx;
	protected Widget m_wVanillaMenu;
	protected Widget m_wRoot;
	protected ref ScriptInvoker m_MenuCb = new ScriptInvoker();

	protected ref array<SCR_BaseEditorAction> m_NativeActions = {};
	protected SCR_EditableEntityComponent m_Entity;
	protected vector m_CursorPos;
	protected int m_Flags;
	protected bool m_bContextMissingLogged;

	void Init(Widget shellRoot, DCO_GMContextMenu menu)
	{
		m_wRoot = shellRoot;
		m_Menu = menu;
		m_MenuCb.Insert(OnMenuAction);

		m_Picker = new DCO_GMCreatePlayerPicker();
		m_Picker.Init(shellRoot, menu);

		ResolveContextComponent();
		GetGame().GetCallqueue().CallLater(PollContextComponent, 500, true);
		Print(string.Format("[DCO-GM] context bridge init (ctx=%1)", m_Ctx != null), LogLevel.NORMAL);
	}

	protected void PollContextComponent()
	{
		ResolveContextComponent();
	}

	protected bool ResolveContextComponent()
	{
		SCR_ContextActionsEditorComponent current = SCR_ContextActionsEditorComponent.Cast(
			SCR_ContextActionsEditorComponent.GetInstance(SCR_ContextActionsEditorComponent, false));
		if (current == m_Ctx)
			return m_Ctx != null;

		if (m_Ctx)
		{
			ScriptInvoker oldInvoker = m_Ctx.GetOnMenuOpen();
			if (oldInvoker)
				oldInvoker.Remove(OnVanillaMenuOpen);
		}
		m_Ctx = current;
		if (m_Ctx)
		{
			ScriptInvoker newInvoker = m_Ctx.GetOnMenuOpen();
			if (newInvoker)
				newInvoker.Insert(OnVanillaMenuOpen);
			Print("[DCO-GM] context bridge bound to current editor-mode context component", LogLevel.NORMAL);
			m_bContextMissingLogged = false;
			return true;
		}
		if (!m_bContextMissingLogged)
		{
			Print("[DCO-GM] context bridge waiting for an active editor-mode context component", LogLevel.WARNING);
			m_bContextMissingLogged = true;
		}
		return false;
	}

	protected Widget FindVanillaMenu()
	{
		Widget w;
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (ws)
			w = ws.FindAnyWidget("ContextMenu");
		if (!w && m_wRoot)
		{
			Widget top = m_wRoot;
			int guard = 0;
			while (top.GetParent() && guard < 24)
			{
				top = top.GetParent();
				guard++;
			}
			w = top.FindAnyWidget("ContextMenu");
		}
		if (w)
			Print("[DCO-GM] vanilla ContextMenu widget resolved -> suppressing", LogLevel.NORMAL);
		else
			Print("[DCO-GM] vanilla ContextMenu widget NOT found", LogLevel.WARNING);
		return w;
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(PollContextComponent);
		if (m_Ctx)
		{
			ScriptInvoker inv = m_Ctx.GetOnMenuOpen();
			if (inv)
				inv.Remove(OnVanillaMenuOpen);
		}
		if (m_Picker)
		{
			m_Picker.Shutdown();
			m_Picker = null;
		}
		GetGame().GetCallqueue().Remove(HideVanillaDeferred);
		GetGame().GetCallqueue().Remove(BuildAndShowMenu);
	}

	// Replaces the engine world menu with Bifrost actions.
	protected void OnVanillaMenuOpen(notnull array<SCR_BaseEditorAction> actions, vector cursorWorldPosition, out notnull array<ref SCR_EditorActionData> filteredActions, out int flags = 0)
	{
		if (!m_Ctx || !m_Menu)
			return;
		m_Entity = m_Ctx.GetHoveredEntity();
		m_CursorPos = cursorWorldPosition;
		m_Flags = flags;
		GetGame().GetCallqueue().Remove(BuildAndShowMenu);
		GetGame().GetCallqueue().CallLater(BuildAndShowMenu, 0);
	}

	protected void BuildAndShowMenu()
	{
		if (!ResolveContextComponent() || !m_Menu)
			return;

		if (ConsumeFreshClaim())
		{
			HideVanilla();
			return;
		}

		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);

		array<SCR_BaseEditorAction> all = {};
		m_Ctx.GetActions(all);

		m_NativeActions.Clear();
		array<string> labels = {};
		array<int> ids = {};
		foreach (SCR_BaseEditorAction action : all)
		{
			if (!action)
				continue;
			if (!action.CanBeShown(m_Entity, selected, m_CursorPos, m_Flags))
				continue;

			SCR_PlaceEntityContextAction placeAction = SCR_PlaceEntityContextAction.Cast(action);
			if (placeAction && placeAction.DCO_PlacingFlag() == EEditorPlacingFlags.CHARACTER_PLAYER)
				continue;

			string label = "Action";
			SCR_UIInfo info = action.GetInfo();
			if (info && !info.GetName().IsEmpty())
				label = info.GetName();

			int idx = m_NativeActions.Count();
			m_NativeActions.Insert(action);
			labels.Insert(label);
			ids.Insert(NATIVE_BASE + idx);
		}

		// Append DCO group orders when a group is hovered.
		if (m_Entity && m_Entity.GetEntityType() == EEditableEntityType.GROUP)
		{
			array<string> ordLabels = {};
			array<int> ordIds = {};
			DCO_GMGroupOrders.BuildMenu(ordLabels, ordIds);
			for (int i = 0; i < ordIds.Count(); i++)
			{
				labels.Insert(ordLabels[i]);
				ids.Insert(ordIds[i]);
			}
		}

		if (m_Entity && (m_Entity.GetEntityType() == EEditableEntityType.CHARACTER || m_Entity.GetEntityType() == EEditableEntityType.VEHICLE))
		{
			labels.Insert("Toggle Invulnerability");
			ids.Insert(ID_INVULN);
			labels.Insert("Toggle Visibility");
			ids.Insert(ID_VISIBILITY);
			labels.Insert("Mark for Teleport");
			ids.Insert(ID_MARK_TP);
			if (m_Entity.GetEntityType() == EEditableEntityType.CHARACTER)
			{
				labels.Insert("Edit Loadout");
				ids.Insert(ID_EDIT_LOADOUT);
				labels.Insert("Reset Loadout");
				ids.Insert(ID_RESET_LOADOUT);
			}
			if (m_Entity.GetEntityType() == EEditableEntityType.VEHICLE)
			{
				labels.Insert("Send on Flyby");
				ids.Insert(ID_FLYBY);
			}
		}

		if (!m_Entity && m_CursorPos != vector.Zero && SCR_Global.IsPositionWithinTerrainBounds(m_CursorPos))
		{
			labels.Insert("Create Player Here");
			ids.Insert(ID_CREATE_PLAYER);
			labels.Insert("Hide Terrain (nearby)");
			ids.Insert(ID_HIDE_TERRAIN);
			if (DCO_GMTools.Get().HasHiddenTerrain())
			{
				labels.Insert("Restore Hidden Terrain");
				ids.Insert(ID_RESTORE_TERRAIN);
			}
			labels.Insert("Place Map Marker");
			ids.Insert(ID_MARKER);
			labels.Insert("Place Trigger Here");
			ids.Insert(ID_TRIGGER);
			labels.Insert("Show FPS (this client)");
			ids.Insert(ID_FPS);
			if (DCO_GMTools.Get().HasTeleportMark())
			{
				labels.Insert("Teleport Marked Unit Here");
				ids.Insert(ID_TP_HERE);
			}
		}

		if (!labels.IsEmpty())
		{
			int mx, my;
			WidgetManager.GetMousePos(mx, my);
			m_Menu.Show(labels, ids, mx, my, m_MenuCb, m_Entity);
		}

		HideVanilla();
	}

	protected void HideVanilla()
	{
		if (!m_wVanillaMenu)
			m_wVanillaMenu = FindVanillaMenu();
		if (m_wVanillaMenu)
			m_wVanillaMenu.SetVisible(false);
		GetGame().GetCallqueue().Remove(HideVanillaDeferred);
		GetGame().GetCallqueue().CallLater(HideVanillaDeferred, 1);
	}

	protected void HideVanillaDeferred()
	{
		if (!m_wVanillaMenu)
			m_wVanillaMenu = FindVanillaMenu();
		if (m_wVanillaMenu)
			m_wVanillaMenu.SetVisible(false);
	}

	void OnMenuAction(int actionId, SCR_EditableEntityComponent e)
	{
		if (actionId == ID_CREATE_PLAYER)
		{
			if (m_Picker)
				m_Picker.Open(m_CursorPos);
			return;
		}
		// GM sandbox tools — act at the captured cursor world position.
		if (actionId == ID_HIDE_TERRAIN)    { DCO_GMTools.Get().HideTerrainAt(m_CursorPos); return; }
		if (actionId == ID_RESTORE_TERRAIN) { DCO_GMTools.Get().RestoreTerrain();           return; }
		if (actionId == ID_MARKER)          { DCO_GMTools.Get().PlaceMarkerAt(m_CursorPos); return; }
		if (actionId == ID_TRIGGER)         { DCO_GMTools.Get().PlaceTriggerAt(m_CursorPos);return; }
		if (actionId == ID_INVULN)          { DCO_GMTools.Get().ToggleInvuln(e);            return; }
		if (actionId == ID_FPS)             { DCO_GMTools.Get().ShowFps();                  return; }
		if (actionId == ID_VISIBILITY)      { DCO_GMTools.Get().ToggleVisibility(e);        return; }
		if (actionId == ID_MARK_TP)         { DCO_GMTools.Get().MarkForTeleport(e);         return; }
		if (actionId == ID_TP_HERE)         { DCO_GMTools.Get().TeleportMarkedTo(m_CursorPos);return; }
		if (actionId == ID_FLYBY)           { DCO_GMTools.Get().SendOnFlyby(e);             return; }
		if (actionId == ID_EDIT_LOADOUT)    { DCO_GMArsenalPanel.Get().OpenFor(e);         return; }
		if (actionId == ID_RESET_LOADOUT)   { if (e && e.GetOwner()) DCO_ArsenalServer.Route(DCO_ArsenalServer.VERB_RESET, e.GetOwner(), ""); return; }
		if (actionId >= NATIVE_BASE)
		{
			int idx = actionId - NATIVE_BASE;
			if (m_Ctx && idx >= 0 && idx < m_NativeActions.Count())
				m_Ctx.ActionPerform(m_NativeActions[idx], m_CursorPos, m_Flags);
			return;
		}
		if (DCO_GMGroupOrders.IsOrderAction(actionId))
			m_GroupOrders.Apply(actionId, e);
	}
}
