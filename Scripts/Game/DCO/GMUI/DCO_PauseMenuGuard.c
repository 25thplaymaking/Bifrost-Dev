modded class PauseMenuUI
{
	override void OnMenuOpen()
	{
		bool suppress = DCO_GMUIController.ShouldSuppressPauseOpen();
		super.OnMenuOpen();
		if (suppress)
			GetGame().GetCallqueue().CallLater(Close, 0, false);
	}
}
