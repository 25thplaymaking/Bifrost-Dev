// Resource-addressed configurable dialogs compose with other mods without competing for the
// global chimeraMenus.conf override. The native proxy still owns focus, cursor, and teardown.
class DCO_ArsenalMenu : SCR_ConfigurableDialogUi
{
	protected static const ResourceName DIALOG_PRESETS =
		"{DD310776F2664F75}Configs/Dialogs/DCO_ArsenalDialogs.conf";
	protected static const string DIALOG_TAG = "DCO_Arsenal";
	protected static bool s_bOpen;
	protected ref DCO_GMArsenalPanel m_Panel;
	protected bool m_bCloseInputHeld;

	static bool IsOpen()
	{
		return DCO_GRSArmoryBridge.IsOpen();
	}

	static bool OpenForLocalPlayer(IEntity actionUser, DCO_ArsenalAccessComponent access)
	{
		return DCO_GRSArmoryBridge.OpenForPlayer(actionUser, access);
	}

	override protected void OnMenuOpen(SCR_ConfigurableDialogUiPreset preset)
	{
		s_bOpen = true;
		Widget root = GetRootWidget();
		PlayerController controller = GetGame().GetPlayerController();
		IEntity player;
		if (controller)
			player = controller.GetControlledEntity();
		if (!root)
		{
			Print("[DCO-ARS] arsenal menu closed: the configured layout has no root widget", LogLevel.WARNING);
			Close();
			return;
		}
		if (!player)
		{
			Print("[DCO-ARS] arsenal menu closed: no locally controlled player was found", LogLevel.WARNING);
			Close();
			return;
		}
		if (!DCO_ArsenalAccessComponent.CanUseNearby(player))
		{
			Print("[DCO-ARS] arsenal menu closed: the player left the access radius before initialization", LogLevel.WARNING);
			Close();
			return;
		}
		OverlaySlot.SetHorizontalAlign(root, LayoutHorizontalAlign.Stretch);
		OverlaySlot.SetVerticalAlign(root, LayoutVerticalAlign.Stretch);
		AlignableSlot.SetPadding(root, 0, 0, 0, 0);
		root.SetVisible(true);

		m_Panel = new DCO_GMArsenalPanel();
		m_Panel.Init(root, true);
		m_Panel.OpenForEntity(player, "YOUR LOADOUT", false);
		if (!m_Panel.IsOpen())
		{
			Print("[DCO-ARS] arsenal menu closed: the Bifrost loadout panel did not initialize", LogLevel.WARNING);
			Close();
			return;
		}

	}

	override void OnMenuUpdate(float tDelta)
	{
		InputManager input = GetGame().GetInputManager();
		if (!input)
			return;

		// The native character-preview helper reads its rotate and zoom actions from this context.
		input.ActivateContext("MenuContext");
		input.ActivateContext("InventoryMenuContext");
		if (m_Panel)
			m_Panel.UpdateCharacterPreview(tDelta);
		if (m_Panel && !m_Panel.IsOpen())
		{
			Close();
			return;
		}

		// Edge-trigger close input without duplicate listeners.
		bool closeInputHeld = input.GetActionValue("MenuBack") > 0 || input.GetActionValue("MenuOpen") > 0;
#ifdef WORKBENCH
		closeInputHeld = closeInputHeld
			|| input.GetActionValue("MenuBackWB") > 0
			|| input.GetActionValue("MenuOpenWB") > 0;
#endif
		if (closeInputHeld && !m_bCloseInputHeld)
			Close();
		m_bCloseInputHeld = closeInputHeld;
	}

	override void OnMenuClose()
	{
		s_bOpen = false;
		if (m_Panel)
		{
			m_Panel.Shutdown();
			m_Panel = null;
		}
		m_bCloseInputHeld = false;
	}

}
