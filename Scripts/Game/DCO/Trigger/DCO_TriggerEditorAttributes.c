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
		foreach (string name : DCO_TriggerComponent.DCO_GetActionNames())
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
class DCO_TriggerShapeEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(trigger.DCO_GetShape());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetShape(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Ellipse"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Rectangle"));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TriggerActivationEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(trigger.DCO_GetActivation());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetActivation(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Present"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Not present"));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TriggerOwnerModeEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(trigger.DCO_GetOwnerMode());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetOwnerMode(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Area filter"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Synced group leader"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Any synced member"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("All synced members"));
		return outEntries.Count();
	}
}

// Applies one clearly described lifecycle to every Ctrl-drag linked AI group.
[BaseContainerProps()]
class DCO_TriggerLinkedUnitModeEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(trigger.DCO_GetLinkedUnitMode());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetLinkedUnitMode(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		foreach (string name : DCO_TriggerComponent.DCO_GetLinkedUnitModeNames())
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(name));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TriggerRadiusZEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trigger.DCO_GetRadiusZ());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetRadiusZ(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TriggerHeightEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trigger.DCO_GetHeight());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetHeight(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TriggerTimerModeEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(trigger.DCO_GetTimerMode());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetTimerMode(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Immediate"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Countdown"));
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Timeout"));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TriggerTimerMinEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trigger.DCO_GetTimerMin());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetTimerMin(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TriggerTimerMidEditorAttribute : DCO_TriggerTimerMinEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trigger.DCO_GetTimerMid());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetTimerMid(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TriggerTimerMaxEditorAttribute : DCO_TriggerTimerMinEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trigger.DCO_GetTimerMax());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetTimerMax(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TriggerIntervalEditorAttribute : DCO_TriggerAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(trigger.DCO_GetCheckInterval());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (trigger && var)
			trigger.DCO_SetCheckInterval(var.GetFloat());
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

// Read-only rows used by Bifrost's Finalize tab. The panel builds their labels
// from the current preview variables, so the review reflects uncommitted edits.
class DCO_TriggerReviewEditorAttributeBase : DCO_TriggerAttributeBase
{
	protected string m_sSummary = "Review will appear here.";

	void DCO_SetSummary(string summary)
	{
		m_sSummary = summary;
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!GetTrigger(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(0);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		outEntries.Insert(new SCR_BaseEditorAttributeEntryText(m_sSummary));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TriggerReviewAreaEditorAttribute : DCO_TriggerReviewEditorAttributeBase
{
}

[BaseContainerProps()]
class DCO_TriggerReviewActivationEditorAttribute : DCO_TriggerReviewEditorAttributeBase
{
}

[BaseContainerProps()]
class DCO_TriggerReviewResponseEditorAttribute : DCO_TriggerReviewEditorAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TriggerComponent trigger = GetTrigger(item);
		if (!trigger)
			return null;
		// The count is the only server-only datum the client cannot derive from
		// the preview attributes. The review row itself remains non-writable.
		return SCR_BaseEditorAttributeVar.CreateInt(trigger.DCO_GetSyncedGroupCount());
	}
}
