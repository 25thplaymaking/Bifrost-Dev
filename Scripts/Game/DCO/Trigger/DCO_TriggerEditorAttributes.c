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
	protected FactionKey m_MissingFactionKey;

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		m_MissingFactionKey = "";
		FactionKey factionKey = trig.DCO_GetConditionFactionKey();
		if (!factionKey.IsEmpty() && DCO_FactionCatalog.IndexOf(factionKey) < 0)
			m_MissingFactionKey = factionKey;
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
		for (int i = 0; i < DCO_TriggerComponent.DCO_GetConditionCount(); i++)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(DCO_TriggerComponent.DCO_GetConditionName(i)));
		if (!m_MissingFactionKey.IsEmpty())
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Missing faction · " + m_MissingFactionKey));
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

// Narrows the group picker without replacing the selected resource with a faction preset.
[BaseContainerProps()]
class DCO_TriggerSpawnFactionEditorAttribute : DCO_TriggerAttributeBase
{
	protected FactionKey m_MissingFactionKey;

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		m_MissingFactionKey = "";
		FactionKey factionKey = trig.DCO_GetSpawnFactionKey();
		if (!factionKey.IsEmpty() && DCO_TriggerGroupCatalog.FactionIndexOf(factionKey) < 0)
			m_MissingFactionKey = factionKey;
		return SCR_BaseEditorAttributeVar.CreateInt(trig.DCO_GetSpawnFaction());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TriggerComponent trig = GetTrigger(item);
		if (trig)
			trig.DCO_SetSpawnFaction(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("All factions"));
		for (int i = 0; i < DCO_TriggerGroupCatalog.FactionCount(); i++)
		{
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(string.Format("%1 · %2",
				DCO_TriggerGroupCatalog.FactionNameAt(i), DCO_TriggerGroupCatalog.FactionKeyAt(i))));
		}
		if (!m_MissingFactionKey.IsEmpty())
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Missing faction · " + m_MissingFactionKey));
		return outEntries.Count();
	}
}

// Which cataloged AI group the SPAWN_GROUP action spawns.
[BaseContainerProps()]
class DCO_TriggerSpawnGroupEditorAttribute : DCO_TriggerAttributeBase
{
	protected FactionKey m_FactionKey;
	protected ResourceName m_MissingPrefab;

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trig = GetTrigger(item);
		if (!trig)
			return null;
		m_FactionKey = trig.DCO_GetSpawnFactionKey();
		m_MissingPrefab = "";
		ResourceName prefab = trig.DCO_GetSpawnGroupPrefab();
		if (!prefab.IsEmpty() && DCO_TriggerGroupCatalog.GroupIndexOf(m_FactionKey, prefab) < 0)
			m_MissingPrefab = prefab;
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
		for (int i = 0; i < DCO_TriggerGroupCatalog.GroupCount(m_FactionKey); i++)
		{
			DCO_TriggerGroupEntry entry = DCO_TriggerGroupCatalog.GroupAt(m_FactionKey, i);
			if (!entry)
				continue;
			string label = entry.m_Name;
			if (m_FactionKey.IsEmpty() && !entry.m_FactionKey.IsEmpty())
				label = string.Format("%1 · %2", label, entry.m_FactionKey);
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(label));
		}
		if (!m_MissingPrefab.IsEmpty())
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Missing mod · " + m_MissingPrefab));
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
