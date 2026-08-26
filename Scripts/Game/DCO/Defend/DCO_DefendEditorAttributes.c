// Per-group "Bifrost Defend" GM attribute.

[BaseContainerProps()]
class DCO_DefenderEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (!util)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(util.DCO_IsDefender());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (util)
			util.DCO_SetDefender(var.GetBool());
	}
}
