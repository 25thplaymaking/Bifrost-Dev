// Lets Game Masters link placed task zones through a shared pair tag.
[BaseContainerProps()]
class DCO_TaskZonePairIdEditorAttribute : SCR_BaseValueListEditorAttribute
{
	protected DCO_TaskZoneComponent GetZone(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;

		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;

		return DCO_TaskZoneComponent.Cast(owner.FindComponent(DCO_TaskZoneComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TaskZoneComponent zone = GetZone(item);
		if (!zone)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(zone.DCO_GetPairId());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		DCO_TaskZoneComponent zone = GetZone(item);
		if (zone)
			zone.DCO_SetPairId((int)Math.Round(var.GetFloat()));
	}
}
