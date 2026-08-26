modded class EditorAttributesDialogUI
{
	override void OnMenuOpen()
	{
		if (DCO_GMUIController.IsActive())
		{
			CloseSelf();	// our inline DCO scenario panel replaces this dialog; never show it while our UI owns the screen.
			return;
		}
		super.OnMenuOpen();
	}
}
