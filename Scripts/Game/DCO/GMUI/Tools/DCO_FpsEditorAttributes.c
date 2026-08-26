// FPS-monitor GM attributes: the GLOBAL enable lives on GM Properties, the PER-PLAYER watch lives on that player's own unit attributes.

[BaseContainerProps()]
class DCO_FpsMonitorEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_GMOverlayState.Get().GetEnabled(DCO_GMOverlayState.OV_FPS));
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_GMOverlayState.Get().SetEnabled(DCO_GMOverlayState.OV_FPS, var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_FpsWatchEditorAttribute : SCR_BaseEditorAttribute
{
	protected static int PlayerIdFor(Managed item)
	{
		SCR_EditableEntityComponent e = SCR_EditableEntityComponent.Cast(item);
		if (!e || e.GetEntityType() != EEditableEntityType.CHARACTER)
			return -1;
		if (!e.HasEntityState(EEditableEntityState.PLAYER))
			return -1;	// players only - AI has no client to measure.
		int pid = e.GetPlayerID();
		if (pid <= 0)
			return -1;
		return pid;
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		int pid = PlayerIdFor(item);
		if (pid < 0)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_FpsMonitorClient.Get().IsWatched(pid));
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		int pid = PlayerIdFor(item);
		if (pid < 0)
			return;
		DCO_FpsMonitorClient.Get().SetWatch(pid, var.GetBool());
	}
}
