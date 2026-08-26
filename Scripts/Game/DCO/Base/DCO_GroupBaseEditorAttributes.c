// Per-group Base Settings GM attributes.

[BaseContainerProps()]
class DCO_GroupOverrideEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (!util)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(util.DCO_GetGrpOverride());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (util)
			util.DCO_SetGrpOverride(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_GroupSkillEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (!util)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(util.DCO_GetGrpSkill());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (util)
			util.DCO_SetGrpSkill(var.GetInt());
	}
}

[BaseContainerProps()]
class DCO_GroupPerceptionEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (!util)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(util.DCO_GetGrpPerception());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (util)
			util.DCO_SetGrpPerception(var.GetInt());
	}
}

[BaseContainerProps()]
class DCO_GroupReactionEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (!util)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(util.DCO_GetGrpReaction());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (util)
			util.DCO_SetGrpReaction(var.GetInt());
	}
}

[BaseContainerProps()]
class DCO_GroupFireRateEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (!util)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(util.DCO_GetGrpFireRate());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (util)
			util.DCO_SetGrpFireRate(var.GetInt());
	}
}
