modded class SCR_NameTagDisplay
{
	protected static bool s_DCO_DiagPrinted = false;

	override bool CanDisplayNameTag(notnull IEntity entity)
	{
		if (DCO_GMUIController.IsActive())
		{
			if (!s_DCO_DiagPrinted)
			{
				Print("[DCO-GM] nametag suppress HIT (SCR_NameTagDisplay.CanDisplayNameTag, IsActive=1)", LogLevel.NORMAL);
				s_DCO_DiagPrinted = true;
			}
			return false;
		}
		return super.CanDisplayNameTag(entity);
	}
}
