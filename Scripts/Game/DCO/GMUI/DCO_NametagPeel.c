modded class SCR_NameTagDisplay
{
	override bool CanDisplayNameTag(notnull IEntity entity)
	{
		if (DCO_GMUIController.IsActive())
			return false;
		return super.CanDisplayNameTag(entity);
	}
}
