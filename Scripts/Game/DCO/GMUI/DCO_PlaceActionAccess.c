// DCO accessor for the engine place-entity context action.
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
modded class SCR_PlaceEntityContextAction
{
	EEditorPlacingFlags DCO_PlacingFlag()
	{
		return m_PlacingFlag;
	}
}
