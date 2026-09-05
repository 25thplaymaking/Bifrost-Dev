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
		SCR_AttributesManagerEditorComponent manager = SCR_AttributesManagerEditorComponent.Cast(SCR_AttributesManagerEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		array<Managed> items = {};
		if (manager) manager.GetEditedItems(items);
		SCR_EditableEntityComponent target;
		if (items.Count() == 1) target = SCR_EditableEntityComponent.Cast(items[0]);
		if (target && target.GetOwner())
		{
			DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.Cast(target.GetOwner().FindComponent(DCO_GMMissionInteractionComponent));
			if (point && point.m_bStandalone)
			{
				RemoveAutoClose();
				CloseSelf();
				DCO_GMUIController.CancelPropertySession();
				DCO_GMMissionPanel.Get().Open(DCO_GMMissionTool.TELEPORTER, point.GetOwner().GetOrigin(), target, true);
				return;
			}
		}
		if (!DCO_GMUIController.ShouldHandoffNativeProperties())
			return;
		RemoveAutoClose();
		CloseSelf();
	}

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		DCO_GMUIController.TouchNativeProperties();
	}

	override void OnMenuClose()
	{
		super.OnMenuClose();
		DCO_GMUIController.SetNativePropertiesOpen(false);
	}
}
