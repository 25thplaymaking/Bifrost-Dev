modded class SCR_FindInContentBrowserContextAction
{
	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		if (DCO_GMUIController.IsActive() && hoveredEntity)
		{
			SCR_EditableEntityUIInfo uiInfo = SCR_EditableEntityUIInfo.Cast(hoveredEntity.GetInfo());
			if (uiInfo)
			{
				DCO_GMUIController.RevealInCreate(WidgetManager.Translate(uiInfo.GetName()));
				return;
			}
		}
		super.Perform(hoveredEntity, selectedEntities, cursorWorldPosition, flags, param);
	}
}
