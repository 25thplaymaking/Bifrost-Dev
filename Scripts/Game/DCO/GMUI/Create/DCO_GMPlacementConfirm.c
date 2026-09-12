class DCO_GMPlacementConfirm
{
	protected Widget m_wRoot;
	protected SCR_PlacingEditorComponent m_Placing;
	protected bool m_bActive;
	protected bool m_bConfirmedThisFrame;	// debounce repeated single-shot callbacks within one input frame.

	void Start(Widget shellRoot)
	{
		m_wRoot = shellRoot;
		InputManager im = GetGame().GetInputManager();
		if (!im)
			return;
		im.AddActionListener("EditorPlaceAndCancel", EActionTrigger.DOWN, OnPlaceOnce);
		im.AddActionListener("EditorSetSelection", EActionTrigger.DOWN, OnTargetClick);
		m_bActive = true;
	}

	void Stop()
	{
		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.RemoveActionListener("EditorPlaceAndCancel", EActionTrigger.DOWN, OnPlaceOnce);
			im.RemoveActionListener("EditorSetSelection", EActionTrigger.DOWN, OnTargetClick);
		}
		GetGame().GetCallqueue().Remove(ClearConfirmGuard);
		m_bActive = false;
		m_bConfirmedThisFrame = false;
		m_Placing = null;
	}

	protected void OnPlaceOnce(float value, EActionTrigger reason)
	{
		TryConfirm(true);
	}

	protected void TryConfirm(bool placeOne)
	{
		if (!m_bActive)
			return;
		if (DCO_GMUIController.IsModalActive())
			return;
		DCO_GMCompositionPanel compositions = DCO_GMCompositionPanel.Get();
		DCO_GMMissionPanel missionTools = DCO_GMMissionPanel.Get();
		if (missionTools.IsTargeting())
		{
			if (IsCursorOverPanels())
				return;
			SCR_MenuLayoutEditorComponent missionLayout = SCR_MenuLayoutEditorComponent.Cast(SCR_MenuLayoutEditorComponent.GetInstance(SCR_MenuLayoutEditorComponent, false));
			SCR_ContextActionsEditorComponent context = SCR_ContextActionsEditorComponent.Cast(SCR_ContextActionsEditorComponent.GetInstance(SCR_ContextActionsEditorComponent, false));
			vector missionPosition;
			SCR_EditableEntityComponent target;
			if (context)
				target = context.GetHoveredEntity();
			if (missionLayout && missionLayout.GetCursorWorldPos(missionPosition))
				missionTools.ConfirmTarget(missionPosition, target);
			return;
		}
		if (compositions.IsTargetingPlacement())
		{
			if (IsCursorOverPanels())
				return;
			SCR_MenuLayoutEditorComponent menuLayout = SCR_MenuLayoutEditorComponent.Cast(
				SCR_MenuLayoutEditorComponent.GetInstance(SCR_MenuLayoutEditorComponent, false));
			vector cursorPosition;
			if (menuLayout && menuLayout.GetCursorWorldPos(cursorPosition))
				compositions.PlaceAtWorldCursor(cursorPosition);
			return;
		}
		if (!m_Placing)
			m_Placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (!m_Placing)
			return;
		if (m_bConfirmedThisFrame)
			return;	// already confirmed this click.
		if (m_Placing.GetSelectedPrefab().IsEmpty())
			return;
		if (IsCursorOverPanels())
			return;	// clicking our own UI, not the world.

		ResourceName prefab = m_Placing.GetSelectedPrefab();
		EnsureAvailableForSpawn(prefab);
		m_bConfirmedThisFrame = true;
		GetGame().GetCallqueue().CallLater(ClearConfirmGuard, 0);
		m_Placing.CreateEntity(placeOne, false);
	}

	protected void ClearConfirmGuard()
	{
		m_bConfirmedThisFrame = false;
	}

	protected void EnsureAvailableForSpawn(ResourceName prefab)
	{
		IEntity owner = m_Placing.GetOwner();
		if (!owner)
			return;
		SCR_ContentBrowserEditorComponent cb = SCR_ContentBrowserEditorComponent.Cast(owner.FindComponent(SCR_ContentBrowserEditorComponent));
		if (!cb)
			return;
		int pid = -1;
		SCR_PlacingEditorComponentClass pdata = SCR_PlacingEditorComponentClass.Cast(m_Placing.GetEditorComponentData());
		if (pdata)
			pid = pdata.GetPrefabID(prefab);
		if (pid >= 0 && cb.IsPrefabIDAvailable(pid))
			return;	// already placeable - leave the browser filter alone.
		cb.ResetAllLabels(false);
		cb.SetCurrentSearch("");
		cb.FilterEntries();
	}
	protected void OnTargetClick(float value, EActionTrigger reason)
	{
		if (DCO_GMMissionPanel.Get().IsTargeting() || DCO_GMCompositionPanel.Get().IsTargetingPlacement())
			TryConfirm(true);
	}

	protected bool IsCursorOverPanels()
	{
		if (!m_wRoot || !m_wRoot.IsEnabledInHierarchy() || DCO_GMUIController.IsModalActive())
			return true;
		if (DCO_GMUIController.HasVisibleModal(m_wRoot))
			return true;
		int x, y;
		WidgetManager.GetMousePos(x, y);
		return DCO_GMUIController.IsPointerOverPanel(m_wRoot, x, y);
	}
}
