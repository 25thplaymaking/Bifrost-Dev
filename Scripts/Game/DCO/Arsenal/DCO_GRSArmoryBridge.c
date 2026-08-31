// Bifrost entry adapter for the permission-granted GRS Armory V2 interface.
// Preview and draft state stay client-side; every live inventory mutation is validated and
// executed on the server by GRSA_ResourcePlayerControllerInventory.
class DCO_GRSArmoryBridge
{
	static bool IsOpen()
	{
		if (GRSA_ShellMenu.IsArmoryOpen())
			return true;
		MenuManager manager = GetGame().GetMenuManager();
		if (!manager)
			return false;
		return manager.FindMenuByPreset(ChimeraMenuPreset.GRSA_ArmoryV2) != null;
	}

	static bool OpenForPlayer(IEntity actionUser, DCO_ArsenalAccessComponent access)
	{
		PlayerController controller = GetGame().GetPlayerController();
		IEntity player;
		if (controller)
			player = controller.GetControlledEntity();
		if (!player || player != actionUser || !access
			|| !DCO_ArsenalAccessComponent.IsUsableBy(access.GetOwner(), player))
		{
			Print("[DCO-ARS] GRS Armory refused: player is not beside an active Arsenal Access binding", LogLevel.WARNING);
			return false;
		}

		return OpenForTarget(player, access);
	}

	static bool OpenForGameMaster(IEntity target)
	{
		if (!DCO_GMRights.IsLocalGameMaster())
		{
			Print("[DCO-ARS] GRS Armory refused: local Game Master authority is not open", LogLevel.WARNING);
			return false;
		}
		if (!target || !target.FindComponent(RplComponent) || !target.FindComponent(InventoryStorageManagerComponent))
		{
			Print("[DCO-ARS] GRS Armory refused: target is not a replicated inventory character", LogLevel.WARNING);
			return false;
		}

		return OpenForTarget(target, null);
	}

	protected static bool OpenForTarget(IEntity target, DCO_ArsenalAccessComponent access)
	{
		MenuManager manager = GetGame().GetMenuManager();
		if (!manager || !target)
			return false;
		if (IsOpen())
			return true;

		RplComponent targetRpl = RplComponent.Cast(target.FindComponent(RplComponent));
		if (!targetRpl || !targetRpl.Id().IsValid())
		{
			Print("[DCO-ARS] GRS Armory refused: edit target has no valid replication id", LogLevel.WARNING);
			return false;
		}

		if (access)
			GRSA_Theme.BeginStationSession(access.GetAccentColor(), access.GetPanelOpacity());
		else
			GRSA_Theme.BeginLocalSession();

		GRSA_DraftService.BeginForTarget(target, true);
		MenuBase menu = manager.OpenMenu(ChimeraMenuPreset.GRSA_ArmoryV2);
		if (!menu)
		{
			GRSA_DraftService.Clear();
			GRSA_Theme.EndSession();
			Print("[DCO-ARS] GRS Armory failed: menu preset did not open", LogLevel.WARNING);
			return false;
		}

		return true;
	}
}
