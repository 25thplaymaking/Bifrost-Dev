// Custom DCO Game Master right-click context actions.

class DCO_GroupContextActionBase : SCR_BaseContextAction
{
	override bool IsServer()
	{
		return true;	// mutates server-authoritative AI groups.
	}

	override bool CanBeShown(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		return DCO_HasGroup(selectedEntities);
	}

	override bool CanBePerformed(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		return DCO_HasGroup(selectedEntities);
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent e : selectedEntities)
		{
			SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(e);
			if (util)
				DCO_ApplyToGroup(util);
		}
	}

	// True if at least one selected entity resolves to an AI group.
	protected bool DCO_HasGroup(set<SCR_EditableEntityComponent> selectedEntities)
	{
		foreach (SCR_EditableEntityComponent e : selectedEntities)
		{
			if (DCO_QRFAttributeHelper.GetGroupUtility(e))
				return true;
		}
		return false;
	}

	// Overridden per concrete action.
	void DCO_ApplyToGroup(SCR_AIGroupUtilityComponent util)
	{
	}
}

[BaseContainerProps()]
class DCO_HoldFireContextAction : DCO_GroupContextActionBase
{
	override void DCO_ApplyToGroup(SCR_AIGroupUtilityComponent util)
	{
		util.DCO_SetManualHold(true);
	}
}

[BaseContainerProps()]
class DCO_ResumeFireContextAction : DCO_GroupContextActionBase
{
	override void DCO_ApplyToGroup(SCR_AIGroupUtilityComponent util)
	{
		util.DCO_SetManualHold(false);
	}
}

[BaseContainerProps()]
class DCO_SetupAmbushContextAction : DCO_GroupContextActionBase
{
	override void DCO_ApplyToGroup(SCR_AIGroupUtilityComponent util)
	{
		util.DCO_SetAmbusher(true);
	}
}

[BaseContainerProps()]
class DCO_CancelAmbushContextAction : DCO_GroupContextActionBase
{
	override void DCO_ApplyToGroup(SCR_AIGroupUtilityComponent util)
	{
		util.DCO_SetAmbusher(false);
	}
}

[BaseContainerProps()]
class DCO_QrfStageContextAction : DCO_GroupContextActionBase
{
	override void DCO_ApplyToGroup(SCR_AIGroupUtilityComponent util)
	{
		util.DCO_SetQRFResponder(true);
	}
}
