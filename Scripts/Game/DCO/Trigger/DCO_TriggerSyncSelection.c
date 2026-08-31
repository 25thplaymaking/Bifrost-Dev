// Gives Ctrl-drag on an AI group the Arma-style sync gesture while preserving
// the stock Ctrl-click and selection-frame behavior for every other target.
modded class SCR_SelectionEditorUIComponent
{
	override protected bool IsInputDisabled()
	{
		if (DCO_GMUIController.IsNativePropertiesOpen())
			return true;
		return super.IsInputDisabled();
	}

	override protected void EditorSetSelection(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		vector cursorWorldPosition;
		bool hasCursorWorldPosition;
		SCR_CursorEditorUIComponent cursor = SCR_CursorEditorUIComponent.Cast(
			GetRootComponent().FindComponent(SCR_CursorEditorUIComponent));
		if (cursor)
			hasCursorWorldPosition = cursor.GetCursorWorldPos(cursorWorldPosition);
		if (DCO_ArsenalAccessPlacement.Get().SelectFromFocused(m_FocusedManager, cursorWorldPosition, hasCursorWorldPosition))
			return;
		if (DCO_AIAnimationFxTool.Get().SelectFromFocused(m_FocusedManager))
			return;
		super.EditorSetSelection(value, reason);
	}

	override protected void EditorDrawToggleSelectionDown(float value, EActionTrigger reason)
	{
		DCO_TriggerSyncDrag.Get().BeginFromFocused(m_FocusedManager);
		super.EditorDrawToggleSelectionDown(value, reason);
	}

	override protected void EditorDrawToggleSelectionPressed(float value, EActionTrigger reason)
	{
		if (DCO_TriggerSyncDrag.Get().Update())
			return;
		super.EditorDrawToggleSelectionPressed(value, reason);
	}

	override protected void EditorDrawToggleSelectionUp(float value, EActionTrigger reason)
	{
		DCO_TriggerSyncDrag.Get().Finish();
		super.EditorDrawToggleSelectionUp(value, reason);
	}

	override protected void EditorDrawSelectionCancel(float value, EActionTrigger reason)
	{
		if (DCO_ArsenalAccessPlacement.Get().Cancel())
			return;
		if (DCO_AIAnimationFxTool.Get().Cancel())
			return;
		if (DCO_TriggerSyncDrag.Get().Cancel())
			return;
		super.EditorDrawSelectionCancel(value, reason);
	}
}
