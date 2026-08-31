modded class SCR_SelectionEditorUIComponent
{
	override protected void DrawFrameDown(bool isToggle)
	{
		if (DCO_GMUIController.IsNativePropertiesOpen() || DCO_GMGizmo.IsPreciseModeActive())
		{
			m_bIsDrawingFrameCancelled = true;	// the same refusal engine uses for an off-area click.
			return;
		}
		super.DrawFrameDown(isToggle);
	}
}
