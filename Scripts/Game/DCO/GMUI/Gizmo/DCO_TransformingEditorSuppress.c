modded class SCR_TransformingEditorComponent
{
	override void StartEditing(SCR_EditableEntityComponent pivot, notnull set<SCR_EditableEntityComponent> entities, vector transform[4])
	{
		if (DCO_GMGizmo.IsPreciseModeActive())
			return;	// precise mode -> the Bifrost gizmo arrows own movement; block engine free-drag.
		super.StartEditing(pivot, entities, transform);
	}
}
