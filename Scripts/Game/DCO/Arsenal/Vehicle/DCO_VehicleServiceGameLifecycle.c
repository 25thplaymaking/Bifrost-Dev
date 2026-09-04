modded class ArmaReforgerScripted
{
	override void OnGameEnd()
	{
		DCO_VehicleServiceMenu.ShutdownForGameEnd();
		super.OnGameEnd();
	}
}
