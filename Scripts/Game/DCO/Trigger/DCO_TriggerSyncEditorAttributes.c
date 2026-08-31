class DCO_TriggerSyncAttributeHelper
{
	static bool GetBinding(Managed item, out DCO_TriggerComponent trigger, out DCO_TriggerBinding binding)
	{
		SCR_EditableGroupComponent editable = SCR_EditableGroupComponent.Cast(item);
		if (!editable || !editable.GetOwner())
			return false;
		RplComponent rpl = RplComponent.Cast(editable.GetOwner().FindComponent(RplComponent));
		if (!rpl || !rpl.Id().IsValid())
			return false;
		return DCO_TriggerSyncRegistry.Find(rpl.Id(), trigger, binding);
	}
}

[BaseContainerProps()]
class DCO_TriggerSyncStatusEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger;
		DCO_TriggerBinding binding;
		if (!DCO_TriggerSyncAttributeHelper.GetBinding(item, trigger, binding))
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(0);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		// Entry construction runs on the owning GM client after the server sends
		// the value snapshot. Keep the label self-contained instead of relying on
		// server-only binding objects that deliberately are not replicated.
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Synced to Bifrost Trigger"));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TriggerSyncSpawnEditorAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger;
		DCO_TriggerBinding binding;
		if (!DCO_TriggerSyncAttributeHelper.GetBinding(item, trigger, binding))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(binding.m_bSpawnOnTrigger);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trigger;
		DCO_TriggerBinding binding;
		if (DCO_TriggerSyncAttributeHelper.GetBinding(item, trigger, binding))
			trigger.DCO_SetSyncedSpawn(binding.m_iId, var.GetBool());
	}
}

class DCO_TriggerSyncActionEditorAttributeBase : SCR_BaseValueListEditorAttribute
{
	protected int GetStepIndex() { return 0; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger;
		DCO_TriggerBinding binding;
		if (!DCO_TriggerSyncAttributeHelper.GetBinding(item, trigger, binding))
			return null;
		DCO_TriggerSequenceStep step = binding.GetStep(GetStepIndex());
		if (!step)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(step.m_eAction);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trigger;
		DCO_TriggerBinding binding;
		if (DCO_TriggerSyncAttributeHelper.GetBinding(item, trigger, binding))
			trigger.DCO_SetSyncedAction(binding.m_iId, GetStepIndex(), var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("None"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Move to position"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Defend position (ends queue)"));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TriggerSyncAction1EditorAttribute : DCO_TriggerSyncActionEditorAttributeBase
{
	override protected int GetStepIndex() { return 0; }
}

[BaseContainerProps()]
class DCO_TriggerSyncAction2EditorAttribute : DCO_TriggerSyncActionEditorAttributeBase
{
	override protected int GetStepIndex() { return 1; }
}

[BaseContainerProps()]
class DCO_TriggerSyncAction3EditorAttribute : DCO_TriggerSyncActionEditorAttributeBase
{
	override protected int GetStepIndex() { return 2; }
}

class DCO_TriggerSyncTargetEditorAttributeBase : SCR_BaseValueListEditorAttribute
{
	protected int GetStepIndex() { return 0; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger;
		DCO_TriggerBinding binding;
		if (!DCO_TriggerSyncAttributeHelper.GetBinding(item, trigger, binding))
			return null;
		DCO_TriggerSequenceStep step = binding.GetStep(GetStepIndex());
		if (!step)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(step.m_iTargetChoice);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trigger;
		DCO_TriggerBinding binding;
		if (DCO_TriggerSyncAttributeHelper.GetBinding(item, trigger, binding))
			trigger.DCO_SetSyncedTarget(binding.m_iId, GetStepIndex(), var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		int count = DCO_TriggerObjectiveCatalog.Count();
		for (int i = 0; i < count; i++)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(DCO_TriggerObjectiveCatalog.LabelAt(i)));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TriggerSyncTarget1EditorAttribute : DCO_TriggerSyncTargetEditorAttributeBase
{
	override protected int GetStepIndex() { return 0; }
}

[BaseContainerProps()]
class DCO_TriggerSyncTarget2EditorAttribute : DCO_TriggerSyncTargetEditorAttributeBase
{
	override protected int GetStepIndex() { return 1; }
}

[BaseContainerProps()]
class DCO_TriggerSyncTarget3EditorAttribute : DCO_TriggerSyncTargetEditorAttributeBase
{
	override protected int GetStepIndex() { return 2; }
}
