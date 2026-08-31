modded class EditorAttributesDialogUI
{
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		DCO_GMUIController.SetNativePropertiesOpen(true);
		// The manager broadcasts its final attribute list after OpenDialog returns,
		// so defer the supported-layout handoff by one UI tick.
		GetGame().GetCallqueue().CallLater(DCO_HandoffToBifrost, 0, false);
	}

	protected void DCO_HandoffToBifrost()
	{
		if (!DCO_GMUIController.ShouldHandoffNativeProperties())
			return;
		RemoveAutoClose();
		CloseSelf();
	}

	override void OnMenuClose()
	{
		super.OnMenuClose();
		DCO_GMUIController.SetNativePropertiesOpen(false);
	}
}
