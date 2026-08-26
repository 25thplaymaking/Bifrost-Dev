// Game Master attributes for the tactical-movement layer.

[BaseContainerProps()]
class DCO_EnableTacticalMoveEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableTacticalMove);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableTacticalMove = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_EnableStandoffEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableStandoff);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableStandoff = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_EnableExposureScoringEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableExposureScoring);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableExposureScoring = var.GetBool();
	}
}

// DCO-native flanking maneuver toggle.
[BaseContainerProps()]
class DCO_EnableFlankingEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableFlanking);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableFlanking = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_FlankAngleEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_TacticalMoveSettings.Get().m_fFlankAngleDeg);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_fFlankAngleDeg = var.GetFloat();
	}
}

[BaseContainerProps()]
class DCO_AvoidThreatFunnelEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bAvoidThreatFunnel);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bAvoidThreatFunnel = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_MinMoveDistanceEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_TacticalMoveSettings.Get().m_fMinMoveDistance);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_fMinMoveDistance = var.GetFloat();
	}
}

[BaseContainerProps()]
class DCO_CheckInIntervalEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_TacticalMoveSettings.Get().m_fCheckInInterval);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_fCheckInInterval = var.GetFloat();
	}
}

[BaseContainerProps()]
class DCO_ObservePauseEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_TacticalMoveSettings.Get().m_fObservePause);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_fObservePause = var.GetFloat();
	}
}

// Procedural tactical path.
[BaseContainerProps()]
class DCO_EnableProceduralPathEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableProceduralPath);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableProceduralPath = var.GetBool();
	}
}

// Traveling overwatch.
[BaseContainerProps()]
class DCO_EnableTravelingOverwatchEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableTravelingOverwatch);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableTravelingOverwatch = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_EnableBoundingOverwatchEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableBaseOfFireSplit);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableBaseOfFireSplit = var.GetBool();
	}
}

// Cover-aware placement.
[BaseContainerProps()]
class DCO_EnableCoverSeekEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableCoverSeek);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableCoverSeek = var.GetBool();
	}
}

// React to contact.
[BaseContainerProps()]
class DCO_EnableReactToContactEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableReactToContact);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableReactToContact = var.GetBool();
	}
}

// Tactical brain / COA selection.
[BaseContainerProps()]
class DCO_EnableTacticalBrainEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableTacticalBrain);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableTacticalBrain = var.GetBool();
	}
}

// DCO formation shapes.
[BaseContainerProps()]
class DCO_EnableDcoFormationsEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableDcoFormations);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableDcoFormations = var.GetBool();
	}
}


[BaseContainerProps()]
class DCO_EnableTacticalCoordinatorEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_TacticalMoveSettings.Get().m_bEnableTacticalCoordinator);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_TacticalMoveSettings.Get().m_bEnableTacticalCoordinator = var.GetBool();
	}
}
