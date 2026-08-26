// Per-group QRF GM attributes.

// Resolve the AI group utility component from a selected editable entity, or null if not a group.
class DCO_QRFAttributeHelper
{
	static SCR_AIGroupUtilityComponent GetGroupUtility(Managed item)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroup)
			return null;

		SCR_AIGroup aiGroup = editableGroup.GetAIGroupComponent();
		if (!aiGroup)
			return null;

		return aiGroup.GetGroupUtilityComponent();
	}
}
[BaseContainerProps()]
class DCO_QRFResponderEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (!util)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(util.DCO_IsQRFResponder());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (util)
			util.DCO_SetQRFResponder(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_QRFRangeEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (!util)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(util.DCO_GetQRFRange());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(item);
		if (util)
			util.DCO_SetQRFRange(var.GetFloat());
	}

	override void PreviewVariable(bool setPreview, SCR_AttributesManagerEditorComponent manager)
	{
		if (!setPreview || !manager)
		{
			DCO_QRFRangeVisual.Hide();
			return;
		}

		float radius = 0;
		SCR_BaseEditorAttributeVar var = GetVariable();
		if (var)
			radius = var.GetFloat();

		array<Managed> items = {};
		manager.GetEditedItems(items);
		foreach (Managed item : items)
		{
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
			if (!editable)
				continue;

			IEntity owner = editable.GetOwner();
			if (owner)
			{
				DCO_QRFRangeVisual.Show(owner.GetOrigin(), radius);
				return;
			}
		}
	}
}
