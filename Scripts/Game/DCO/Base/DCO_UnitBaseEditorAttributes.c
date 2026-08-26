// Per-CHARACTER Base Settings GM attributes.

class DCO_UnitAttributeHelper
{
	// The AI character's combat component behind an attributes-dialog item; null hides the attribute.
	static SCR_AICombatComponent GetCharacterCombat(Managed item)
	{
		SCR_EditableEntityComponent e = SCR_EditableEntityComponent.Cast(item);
		if (!e || e.GetEntityType() != EEditableEntityType.CHARACTER)
			return null;
		if (e.HasEntityState(EEditableEntityState.PLAYER))
			return null;	// AI only - a player's combat settings are their own.
		IEntity owner = e.GetOwner();
		if (!owner)
			return null;
		return SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
	}
}

[BaseContainerProps()]
class DCO_UnitOverrideEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (!combat)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(combat.DCO_GetUnitOverride());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (combat)
			combat.DCO_SetUnitOverride(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_UnitSkillEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (!combat)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(combat.DCO_GetUnitSkill());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (combat)
			combat.DCO_SetUnitSkill(var.GetInt());
	}
}

[BaseContainerProps()]
class DCO_UnitPerceptionEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (!combat)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(combat.DCO_GetUnitPerception());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (combat)
			combat.DCO_SetUnitPerception(var.GetInt());
	}
}

[BaseContainerProps()]
class DCO_UnitReactionEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (!combat)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(combat.DCO_GetUnitReaction());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (combat)
			combat.DCO_SetUnitReaction(var.GetInt());
	}
}

[BaseContainerProps()]
class DCO_UnitFireRateEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (!combat)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(combat.DCO_GetUnitFireRate());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AICombatComponent combat = DCO_UnitAttributeHelper.GetCharacterCombat(item);
		if (combat)
			combat.DCO_SetUnitFireRate(var.GetInt());
	}
}
