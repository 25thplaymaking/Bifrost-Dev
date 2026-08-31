[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
modded class SCR_FindInContentBrowserContextAction
{
	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		if (DCO_GMUIController.IsActive() && hoveredEntity)
		{
			SCR_EditableEntityUIInfo uiInfo = SCR_EditableEntityUIInfo.Cast(hoveredEntity.GetInfo());
			if (uiInfo)
			{
				string displayName = DCO_GMDisplayName.Resolve(uiInfo.GetName(), hoveredEntity.GetPrefab(), "Entity");
				DCO_GMUIController.RevealInCreate(displayName);
				return;
			}
		}
		super.Perform(hoveredEntity, selectedEntities, cursorWorldPosition, flags, param);
	}
}
