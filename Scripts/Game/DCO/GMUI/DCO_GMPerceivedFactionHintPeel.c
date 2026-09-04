modded class SCR_PerceivedFactionManagerComponent
{
	override void ShowPerceivedFactionChangedHint(Faction playerPerceivedFaction)
	{
		if (DCO_GMUIController.IsActive())
			return;

		super.ShowPerceivedFactionChangedHint(playerPerceivedFaction);
	}
}
