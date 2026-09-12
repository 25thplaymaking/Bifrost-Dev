class DCO_ArsenalMenu
{
	static bool IsOpen()
	{
		return DCO_BIArmoryBridge.IsOpen();
	}

	static bool OpenForLocalPlayer(IEntity actionUser, DCO_ArsenalAccessComponent access)
	{
		return DCO_BIArmoryBridge.OpenForPlayer(actionUser, access);
	}
}
