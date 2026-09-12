// Gives Ctrl-drag on an AI group the Arma-style sync gesture while preserving
// the stock Ctrl-click and selection-frame behavior for every other target.
modded class SCR_SelectionEditorUIComponent
{
	override protected bool IsInputDisabled()
	{
		if (DCO_GMUIController.IsWorldInputBlocked(true))
			return true;
		return super.IsInputDisabled();
	}

	protected void DCO_CancelSelectionFrame()
	{
		// A queued release must not commit after a modal takes ownership.
		GetGame().GetCallqueue().Remove(ConfirmFrame);
		m_bIsDrawingFrameConfirmed = false;
		m_bIsDrawingFrameCancelled = true;
		m_bIsAnimatingFrame = false;
		ResetFrame();
	}

	override protected void DrawFrameDown(bool isToggle)
	{
		if (DCO_GMUIController.IsWorldInputBlocked(true))
		{
			DCO_CancelSelectionFrame();
			return;
		}
		super.DrawFrameDown(isToggle);
	}

	override protected void DrawFramePressed(bool isToggle)
	{
		if (DCO_GMUIController.IsWorldInputBlocked(true))
		{
			DCO_CancelSelectionFrame();
			return;
		}
		super.DrawFramePressed(isToggle);
	}

	override protected void DrawFrameUp(bool isToggle)
	{
		if (DCO_GMUIController.IsWorldInputBlocked(true))
		{
			DCO_CancelSelectionFrame();
			return;
		}
		super.DrawFrameUp(isToggle);
	}

	override protected void ConfirmFrame(bool isToggle)
	{
		if (DCO_GMUIController.IsWorldInputBlocked(true))
		{
			DCO_CancelSelectionFrame();
			return;
		}
		super.ConfirmFrame(isToggle);
	}

	override protected void OnMenuUpdate(float tDelta)
	{
		if (DCO_GMUIController.IsModalActive() && (m_bIsDrawingFrame || m_bIsAnimatingFrame || m_bIsDrawingFrameConfirmed))
			DCO_CancelSelectionFrame();
		super.OnMenuUpdate(tDelta);
	}

	override protected void EditorSetSelection(float value = 1, EActionTrigger reason = EActionTrigger.DOWN)
	{
		if (IsInputDisabled())
			return;
		vector cursorWorldPosition;
		bool hasCursorWorldPosition;
		SCR_CursorEditorUIComponent cursor = SCR_CursorEditorUIComponent.Cast(
			GetRootComponent().FindComponent(SCR_CursorEditorUIComponent));
		if (cursor)
			hasCursorWorldPosition = cursor.GetCursorWorldPos(cursorWorldPosition);
		if (DCO_VehicleServiceAccessPlacement.Get().SelectAtCursor(cursorWorldPosition, hasCursorWorldPosition))
			return;
		if (DCO_ArsenalAccessPlacement.Get().SelectFromFocused(m_FocusedManager, cursorWorldPosition, hasCursorWorldPosition))
			return;
		if (DCO_AIAnimationFxTool.Get().SelectFromFocused(m_FocusedManager))
			return;
		super.EditorSetSelection(value, reason);
	}

	override protected void EditorDrawToggleSelectionDown(float value, EActionTrigger reason)
	{
		if (DCO_GMUIController.IsWorldInputBlocked(true))
		{
			DCO_CancelSelectionFrame();
			return;
		}
		if (IsInputDisabled())
			return;
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
		if (DCO_VehicleServiceAccessPlacement.Get().Cancel())
			return;
		if (DCO_ArsenalAccessPlacement.Get().Cancel())
			return;
		if (DCO_AIAnimationFxTool.Get().Cancel())
			return;
		if (DCO_TriggerSyncDrag.Get().Cancel())
			return;
		super.EditorDrawSelectionCancel(value, reason);
	}
}
