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
	static const int ID_DOORS_OPEN      = 74;
	static const int ID_DOORS_CLOSE     = 75;
	static const int ID_LIGHTS_ON       = 76;
	static const int ID_LIGHTS_OFF      = 77;
	static const int ID_SURRENDER       = 78;
	static const int ID_RESTORE_AI      = 79;
	static const int ID_COMPOSITIONS    = 80;
	static const int ID_MOVE_SERVICE_ACCESS = 81;

	protected static const float PANEL_CLAIM_WINDOW_MS = 250.0;
	protected static float s_fPanelClaimAtMs;
	protected static bool s_bPanelClaimed;
	static void ClaimRightClick()
	{
		BaseWorld world = GetGame().GetWorld();
		if (world)
		{
			s_fPanelClaimAtMs = world.GetWorldTime();
			s_bPanelClaimed = true;
		}
	}
	// True when a panel claimed a right-click within the same click's window; always clears the claim.
	protected static bool ConsumeFreshClaim()
	{
		if (!s_bPanelClaimed)
			return false;
		BaseWorld world = GetGame().GetWorld();
		bool fresh = world && (world.GetWorldTime() - s_fPanelClaimAtMs) <= PANEL_CLAIM_WINDOW_MS;
		s_bPanelClaimed = false;
		return fresh;
	}

	protected static DCO_VehicleServiceZoneComponent ResolveVehicleServiceZone(SCR_EditableEntityComponent editable)
	{
		if (!editable || !editable.GetOwner())
			return null;
		IEntity owner = editable.GetOwner();
		DCO_VehicleServiceZoneComponent zone = DCO_VehicleServiceZoneComponent.Cast(
			owner.FindComponent(DCO_VehicleServiceZoneComponent));
		if (zone)
			return zone;
		DCO_VehicleServiceAccessComponent access = DCO_VehicleServiceAccessComponent.Cast(
			owner.FindComponent(DCO_VehicleServiceAccessComponent));
		if (access)
			return access.GetZone();
		return null;
	}

	protected DCO_GMContextMenu m_Menu;	// shared, owned by the controller.
	protected ref DCO_GMGroupOrders m_GroupOrders = new DCO_GMGroupOrders();
	protected ref DCO_GMCreatePlayerPicker m_Picker;
	protected SCR_ContextActionsEditorComponent m_Ctx;
	protected Widget m_wVanillaMenu;
	protected Widget m_wRoot;
	protected ref ScriptInvoker m_MenuCb = new ScriptInvoker();

	protected ref array<SCR_BaseEditorAction> m_NativeActions = {};
	protected ref array<SCR_BaseEditorAction> m_EvaluatedNativeActions = {};
	protected ref array<bool> m_EvaluatedNativeCanPerform = {};
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
		if (!w)
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
		if (DCO_GMUIController.IsNativePropertiesOpen())
			return;
		if (!m_Ctx || !m_Menu)
			return;
		m_Entity = m_Ctx.GetHoveredEntity();
		m_CursorPos = cursorWorldPosition;
		m_Flags = flags;
		m_EvaluatedNativeActions.Clear();
		m_EvaluatedNativeCanPerform.Clear();
		foreach (SCR_EditorActionData actionData : filteredActions)
		{
			if (actionData && actionData.GetAction())
			{
				m_EvaluatedNativeActions.Insert(actionData.GetAction());
				m_EvaluatedNativeCanPerform.Insert(actionData.GetCanBePerformed());
			}
		}
		GetGame().GetCallqueue().Remove(BuildAndShowMenu);
		GetGame().GetCallqueue().CallLater(BuildAndShowMenu, 0);
	}

	protected void BuildAndShowMenu()
	{
		if (DCO_GMUIController.IsNativePropertiesOpen())
			return;
		if (!ResolveContextComponent() || !m_Menu)
			return;

		if (ConsumeFreshClaim())
		{
			HideVanilla();
			return;
		}

		m_NativeActions.Clear();
		array<string> labels = {};
		array<int> ids = {};
		array<bool> enabled = {};
		for (int evaluatedIndex = 0; evaluatedIndex < m_EvaluatedNativeActions.Count(); evaluatedIndex++)
		{
			SCR_BaseEditorAction action = m_EvaluatedNativeActions[evaluatedIndex];
			if (!action)
				continue;

			SCR_PlaceEntityContextAction placeAction = SCR_PlaceEntityContextAction.Cast(action);
			if (placeAction && placeAction.DCO_PlacingFlag() == EEditorPlacingFlags.CHARACTER_PLAYER)
				continue;

			string label = ResolveNativeActionLabel(action);

			int idx = m_NativeActions.Count();
			m_NativeActions.Insert(action);
			labels.Insert(label);
			ids.Insert(NATIVE_BASE + idx);
			enabled.Insert(evaluatedIndex < m_EvaluatedNativeCanPerform.Count() && m_EvaluatedNativeCanPerform[evaluatedIndex]);
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

		if (DCO_GMWorldControlClient.HasDoorTarget(m_Entity))
		{
			labels.Insert("Open Doors for Selection");
			ids.Insert(ID_DOORS_OPEN);
			labels.Insert("Close Doors for Selection");
			ids.Insert(ID_DOORS_CLOSE);
		}
		if (DCO_GMWorldControlClient.HasLightTarget(m_Entity))
		{
			labels.Insert("Turn Selected Lights On");
			ids.Insert(ID_LIGHTS_ON);
			labels.Insert("Turn Selected Lights Off");
			ids.Insert(ID_LIGHTS_OFF);
		}
		if (DCO_GMWorldControlClient.HasSurrenderTarget(m_Entity, false))
		{
			labels.Insert("Surrender Selected AI");
			ids.Insert(ID_SURRENDER);
		}
		if (DCO_GMWorldControlClient.HasSurrenderTarget(m_Entity, true))
		{
			labels.Insert("Restore Selected AI");
			ids.Insert(ID_RESTORE_AI);
		}
		if (ResolveVehicleServiceZone(m_Entity))
		{
			labels.Insert("Move Vehicle Service Access");
			ids.Insert(ID_MOVE_SERVICE_ACCESS);
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
			if (m_Entity.GetEntityType() == EEditableEntityType.VEHICLE && DCO_FxAircraftCatalog.IsSupportedHelicopter(m_Entity.GetOwner()))
			{
				labels.Insert("Send Helicopter on Flyby (Despawns)");
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
			labels.Insert("Markers & Intel");
			ids.Insert(ID_MARKER);
			labels.Insert("Compositions");
			ids.Insert(ID_COMPOSITIONS);
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
			while (enabled.Count() < labels.Count())
				enabled.Insert(true);
			int mx, my;
			WidgetManager.GetMousePos(mx, my);
			m_Menu.ShowWithAvailability(labels, ids, enabled, mx, my, m_MenuCb, m_Entity);
		}

		HideVanilla();
	}

	protected string ResolveNativeActionLabel(SCR_BaseEditorAction action)
	{
		if (!action)
			return "Action";

		string actionType = action.Type().ToString();
		if (actionType == "SCR_FindInContentBrowserContextAction")
			return "Find in Content Browser";
		if (actionType == "SCR_OpenAttributeWindowContextAction")
			return "Edit Properties";

		SCR_UIInfo info = action.GetInfo();
		if (!info || info.GetName().IsEmpty())
			return ResolveNativeActionFallback(actionType);

		string authoredName = info.GetName();
		if (authoredName[0] != "#")
			return authoredName;

		string translatedName = WidgetManager.Translate(authoredName);
		if (!translatedName.IsEmpty() && translatedName[0] != "#")
		{
			string unprefixedName = authoredName.Substring(1, authoredName.Length() - 1);
			if (translatedName != unprefixedName)
				return translatedName;
		}

		return ResolveNativeActionFallback(actionType);
	}

	protected string ResolveNativeActionFallback(string actionType)
	{
		actionType.Replace("SCR_", "");
		actionType.Replace("ContextAction", "");
		actionType.Replace("ToolbarAction", "");
		actionType.Replace("Action", "");
		actionType.Replace("_", " ");
		if (actionType.IsEmpty())
			return "Action";
		return actionType;
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
		if (actionId != ID_MOVE_SERVICE_ACCESS)
			DCO_VehicleServiceAccessPlacement.Get().Cancel();
		if (actionId == ID_CREATE_PLAYER)
		{
			if (m_Picker)
				m_Picker.Open(m_CursorPos);
			return;
		}
		// Apply sandbox actions at the captured cursor position.
		if (actionId == ID_HIDE_TERRAIN)    { DCO_GMTools.Get().HideTerrainAt(m_CursorPos); return; }
		if (actionId == ID_RESTORE_TERRAIN) { DCO_GMTools.Get().RestoreTerrain();           return; }
		if (actionId == ID_MARKER)          { DCO_GMCompositionPanel.Get().CloseForBack(); DCO_GMMarkerPanel.Get().Open(m_CursorPos); return; }
		if (actionId == ID_COMPOSITIONS)    { DCO_GMMarkerPanel.Get().CloseForBack(); DCO_GMCompositionPanel.Get().Open(m_CursorPos); return; }
		if (actionId == ID_TRIGGER)         { DCO_GMTools.Get().PlaceTriggerAt(m_CursorPos);return; }
		if (actionId == ID_INVULN)          { DCO_GMTools.Get().ToggleInvuln(e);            return; }
		if (actionId == ID_FPS)             { DCO_GMTools.Get().ShowFps();                  return; }
		if (actionId == ID_VISIBILITY)      { DCO_GMTools.Get().ToggleVisibility(e);        return; }
		if (actionId == ID_MARK_TP)         { DCO_GMTools.Get().MarkForTeleport(e);         return; }
		if (actionId == ID_TP_HERE)         { DCO_GMTools.Get().TeleportMarkedTo(m_CursorPos);return; }
		if (actionId == ID_FLYBY)           { DCO_GMTools.Get().SendOnFlyby(e);             return; }
		if (actionId == ID_EDIT_LOADOUT)    { if (e) DCO_GRSArmoryBridge.OpenForGameMaster(e.GetOwner()); return; }
		if (actionId == ID_RESET_LOADOUT)   { if (e && e.GetOwner()) DCO_ArsenalServer.Route(DCO_ArsenalServer.VERB_RESET, e.GetOwner(), ""); return; }
		if (actionId == ID_DOORS_OPEN)      { DCO_GMWorldControlClient.RouteSelected(EDCO_GMWorldControlAction.OPEN_DOORS, e); return; }
		if (actionId == ID_DOORS_CLOSE)     { DCO_GMWorldControlClient.RouteSelected(EDCO_GMWorldControlAction.CLOSE_DOORS, e); return; }
		if (actionId == ID_LIGHTS_ON)       { DCO_GMWorldControlClient.RouteSelected(EDCO_GMWorldControlAction.LIGHTS_ON, e); return; }
		if (actionId == ID_LIGHTS_OFF)      { DCO_GMWorldControlClient.RouteSelected(EDCO_GMWorldControlAction.LIGHTS_OFF, e); return; }
		if (actionId == ID_SURRENDER)       { DCO_GMWorldControlClient.RouteSelected(EDCO_GMWorldControlAction.SURRENDER, e); return; }
		if (actionId == ID_RESTORE_AI)      { DCO_GMWorldControlClient.RouteSelected(EDCO_GMWorldControlAction.RESTORE, e); return; }
		if (actionId == ID_MOVE_SERVICE_ACCESS)
		{
			DCO_ArsenalAccessPlacement.Get().Cancel();
			DCO_AIAnimationFxTool.Get().Cancel();
			DCO_VehicleServiceAccessPlacement.Get().BeginTargeting(ResolveVehicleServiceZone(e));
			return;
		}
		if (actionId >= NATIVE_BASE)
		{
			int idx = actionId - NATIVE_BASE;
			if (m_Ctx && idx >= 0 && idx < m_NativeActions.Count())
			{
				SCR_BaseEditorAction nativeAction = m_NativeActions[idx];
				DCO_ContentBrowserGate.AllowNativeActionBrowser();
				m_Ctx.ActionPerform(nativeAction, m_CursorPos, m_Flags);
				DCO_ContentBrowserGate.ClearNativeActionBrowserAllowance();
			}
			return;
		}
		if (DCO_GMGroupOrders.IsOrderAction(actionId))
			m_GroupOrders.Apply(actionId, e);
	}
}
