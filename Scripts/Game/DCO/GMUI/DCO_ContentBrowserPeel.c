modded class SCR_ContentBrowserEditorComponent
{
	override bool OpenBrowserMenu()
	{
		if (DCO_GMUIController.IsActive())
			return false;	// DCO CREATE panel replaces the GM content browser; swallow every open while active.
		return super.OpenBrowserMenu();
	}
}
