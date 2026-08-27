modded class EditorAttributesDialogUI
{
	override void OnMenuOpen()
	{
		if (DCO_GMUIController.IsActive())
		{
			CloseSelf();	// Our inline scenario panel owns these settings while Bifrost GM is active.
			return;
		}
		super.OnMenuOpen();
	}
}
