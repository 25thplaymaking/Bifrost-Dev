// Game Master attributes for a placed GM Trigger.

class DCO_TriggerAttributeBase : SCR_BaseValueListEditorAttribute
{
	protected DCO_TriggerComponent GetTrigger(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;
		return DCO_TriggerComponent.Cast(owner.FindComponent(DCO_TriggerComponent));
	}
}

// Who trips the trigger, as a labeled spinbox.
[BaseContainerProps()]
class DCO_TriggerConditionEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(trig.DCO_GetCondition());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetCondition(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		foreach (string name : DCO_TriggerComponent.COND_NAMES)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(name));
		return outEntries.Count();
	}
}

// What firing the trigger does, as a labeled spinbox.
[BaseContainerProps()]
class DCO_TriggerActionEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(trig.DCO_GetAction());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetAction(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		foreach (string name : DCO_TriggerComponent.ACTION_NAMES)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(name));
		return outEntries.Count();
	}
}

// Which AI group the SPAWN_GROUP action spawns, as a labeled spinbox.
[BaseContainerProps()]
class DCO_TriggerSpawnGroupEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(trig.DCO_GetSpawnGroup());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetSpawnGroup(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		foreach (string name : DCO_TriggerComponent.SPAWN_GROUP_NAMES)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(name));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TriggerRadiusEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trig.DCO_GetRadius());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetRadius(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TriggerCountEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trig.DCO_GetCountThreshold());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetCountThreshold(Math.Round(var.GetFloat()));
	}
}

[BaseContainerProps()]
class DCO_TriggerCooldownEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trig.DCO_GetCooldown());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetCooldown(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TriggerPairIdEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trig.DCO_GetPairId());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetPairId(Math.Round(var.GetFloat()));
	}
}

[BaseContainerProps()]
class DCO_TriggerFxRadiusEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trig.DCO_GetFxPairRadius());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetFxPairRadius(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TriggerEnabledEditorAttribute : SCR_BaseEditorAttribute
{
	protected DCO_TriggerComponent GetTrigger(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;
		return DCO_TriggerComponent.Cast(owner.FindComponent(DCO_TriggerComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(trig.DCO_GetEnabled());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetEnabled(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_TriggerRepeatEditorAttribute : DCO_TriggerEnabledEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(trig.DCO_GetRepeat());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetRepeat(var.GetBool());
	}
}
