// F6/(4): suppress the engine white player-name label the editor draws over player-CONTROLLED characters in Game Master.
modded class SCR_PlayerEditableEntityUIComponent
{
	protected TextWidget m_wDCO_NameWidget;
	protected static bool s_DCO_DiagPrinted = false;	// one-shot log proof the right seam fired this time.

	override void OnInit(SCR_EditableEntityComponent entity, SCR_UIInfo info, SCR_EditableEntityBaseSlotUIComponent slot)
	{
		super.OnInit(entity, info, slot);
		Widget widget = GetWidget();
		if (widget)
			m_wDCO_NameWidget = TextWidget.Cast(widget.FindAnyWidget(m_sPlayerNameWidgetName));
		DCO_ApplyNameVisibility();
	}

	override void OnRefresh(SCR_EditableEntityBaseSlotUIComponent slot)
	{
		super.OnRefresh(slot);
		DCO_ApplyNameVisibility();
	}

	// Hide the name in GM, restore it off-GM.
	protected void DCO_ApplyNameVisibility()
	{
		if (!m_wDCO_NameWidget)
			return;
		bool active = DCO_GMUIController.IsActive();
		m_wDCO_NameWidget.SetVisible(!active);
		if (active && !s_DCO_DiagPrinted)
		{
			Print("[DCO-GM] player-label suppress HIT (SCR_PlayerEditableEntityUIComponent name hidden, IsActive=1)", LogLevel.NORMAL);
			s_DCO_DiagPrinted = true;
		}
	}
}
