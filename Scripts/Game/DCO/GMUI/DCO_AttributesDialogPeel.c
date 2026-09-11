modded class EditorAttributesDialogUI
{
	protected bool m_bDCO_DialogOpen;
	protected bool m_bDCO_HandingOff;
	protected bool m_bDCO_AttributesEnded;
	protected SCR_AttributesManagerEditorComponent m_DCO_Manager;

	override void OnMenuOpen()
	{
		DCO_TestDiagnostics.Event("gm.native.open");
		m_bDCO_DialogOpen = true;
		m_bDCO_HandingOff = false;
		m_bDCO_AttributesEnded = false;
		m_DCO_Manager = SCR_AttributesManagerEditorComponent.Cast(SCR_AttributesManagerEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		if (m_DCO_Manager)
		{
			// Mark completion before the native close callback runs.
			m_DCO_Manager.GetOnAttributesConfirm().Insert(DCO_OnAttributesEnded);
			m_DCO_Manager.GetOnAttributesCancel().Insert(DCO_OnAttributesEnded);
		}
		super.OnMenuOpen();
		if (!m_bDCO_DialogOpen) return;
		DCO_GMUIController.SetNativePropertiesOpen(true);
		// The manager broadcasts its final attribute list after OpenDialog returns,
		// so defer the supported-layout handoff by one UI tick.
		GetGame().GetCallqueue().CallLater(DCO_HandoffToBifrost, 0, false);
	}

	protected void DCO_HandoffToBifrost()
	{
		DCO_TestDiagnostics.Event("gm.native.handoff", string.Format("dialogOpen=%1 supported=%2", m_bDCO_DialogOpen, DCO_GMUIController.ShouldHandoffNativeProperties()));
		if (!m_bDCO_DialogOpen) return;
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
				m_bDCO_HandingOff = true;
				RemoveAutoClose();
				CloseSelf();
				DCO_GMUIController.CancelPropertySession();
				DCO_GMMissionPanel.Get().Open(DCO_GMMissionTool.TELEPORTER, point.GetOwner().GetOrigin(), target, true);
				return;
			}
		}
		if (!DCO_GMUIController.ShouldHandoffNativeProperties())
			return;
		m_bDCO_HandingOff = true;
		RemoveAutoClose();
		CloseSelf();
	}

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (m_bDCO_DialogOpen) DCO_GMUIController.TouchNativeProperties();
	}

	protected void DCO_OnAttributesEnded(array<SCR_BaseEditorAttribute> attributes)
	{
		m_bDCO_AttributesEnded = true;
	}

	override void OnMenuClose()
	{
		DCO_TestDiagnostics.Event("gm.native.close", string.Format("handoff=%1 ended=%2", m_bDCO_HandingOff, m_bDCO_AttributesEnded));
		m_bDCO_DialogOpen = false;
		GetGame().GetCallqueue().Remove(DCO_HandoffToBifrost);
		DCO_GMUIController.SetNativePropertiesOpen(false);
		super.OnMenuClose();
		if (m_DCO_Manager)
		{
			m_DCO_Manager.GetOnAttributesConfirm().Remove(DCO_OnAttributesEnded);
			m_DCO_Manager.GetOnAttributesCancel().Remove(DCO_OnAttributesEnded);
			if (!m_bDCO_HandingOff && !m_bDCO_AttributesEnded)
				m_DCO_Manager.CancelEditing();
		}
		m_DCO_Manager = null;
	}
}
