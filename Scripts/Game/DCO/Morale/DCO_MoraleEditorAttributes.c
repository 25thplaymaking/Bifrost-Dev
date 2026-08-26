// Game Master attributes for the "Bifrost" settings tab.

// MORALE - master switch.
[BaseContainerProps()]
class DCO_EnableMoraleEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		// Global attribute: only valid in the Game Master "Scenario properties" panel, whose edited item is the game mode.
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnabled);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnabled = var.GetBool();
	}
}
// MORALE - break/flee threshold.
[BaseContainerProps()]
class DCO_FleeThresholdEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_MoraleSettings.Get().m_fFleeThreshold);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_fFleeThreshold = var.GetFloat();
	}
}

// MORALE - cohesion: per-casualty hit, leader-loss shock, contagion, per-member layer.
[BaseContainerProps()]
class DCO_MoraleLostPerCasualtyEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_MoraleSettings.Get().m_fMoraleLostPerCasualty);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_fMoraleLostPerCasualty = var.GetFloat();
	}
}

[BaseContainerProps()]
class DCO_MoraleLostOnLeaderLossEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_MoraleSettings.Get().m_fMoraleLostOnLeaderLoss);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_fMoraleLostOnLeaderLoss = var.GetFloat();
	}
}

// Morale contagion / cascade master toggle.
[BaseContainerProps()]
class DCO_EnableMoraleContagionEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableMoraleContagion);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableMoraleContagion = var.GetBool();
	}
}

// Per-member morale modifier.
[BaseContainerProps()]
class DCO_EnableMemberMoraleEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableMemberMorale);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableMemberMorale = var.GetBool();
	}
}

// MORALE - break outcome: smoke-covered withdrawal.
[BaseContainerProps()]
class DCO_EnableFleeSmokeEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableFleeSmoke);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableFleeSmoke = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_EnablePanicEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnablePanic);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnablePanic = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_PanicChanceEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_MoraleSettings.Get().m_fPanicChancePerTick);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_fPanicChancePerTick = var.GetFloat();
	}
}

[BaseContainerProps()]
class DCO_PanicDurationEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_MoraleSettings.Get().m_fPanicDurationSec);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_fPanicDurationSec = var.GetFloat();
	}
}

// MORALE - morale-to-accuracy bands.
[BaseContainerProps()]
class DCO_EnableMoraleAccuracyEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableMoraleAccuracy);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableMoraleAccuracy = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_AccuracyRookieMoraleEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_MoraleSettings.Get().m_fAccuracyRookieMorale);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_fAccuracyRookieMorale = var.GetFloat();
	}
}

[BaseContainerProps()]
class DCO_AccuracyRegularMoraleEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(DCO_MoraleSettings.Get().m_fAccuracyRegularMorale);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_fAccuracyRegularMorale = var.GetFloat();
	}
}

// END OF MORALE BLOCK - per-system master toggles below.

[BaseContainerProps()]
class DCO_EnableStanceCooldownEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableStanceCooldown);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableStanceCooldown = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_StanceCooldownInCoverEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bStanceCooldownInCover);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bStanceCooldownInCover = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_EnableAdaptiveFormationEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableAdaptiveFormation);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableAdaptiveFormation = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_EnableFriendlyFireEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableFriendlyFire);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableFriendlyFire = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_EnableStragglerMergeEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableStragglerMerge);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableStragglerMerge = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_EnableFleeFromArmorEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableFleeFromArmor);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableFleeFromArmor = var.GetBool();
	}
}

// Vision / night limiting.
[BaseContainerProps()]
class DCO_EnableVisionLimitEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableVisionLimit);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableVisionLimit = var.GetBool();
	}
}

// Adjust-to-cover stance.
[BaseContainerProps()]
class DCO_EnableCoverStanceEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableCoverStance);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableCoverStance = var.GetBool();
	}
}

// Real suppression.
[BaseContainerProps()]
class DCO_EnableSuppressionEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableSuppression);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableSuppression = var.GetBool();
	}
}

// Machine-gunner emplacement.
[BaseContainerProps()]
class DCO_EnableMachineGunnerEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableMachineGunner);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableMachineGunner = var.GetBool();
	}
}

// Emergency rearm.
[BaseContainerProps()]
class DCO_EnableEmergencyRearmEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableEmergencyRearm);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableEmergencyRearm = var.GetBool();
	}
}

// CQB / building assault.
[BaseContainerProps()]
class DCO_EnableCQBEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_MoraleSettings.Get().m_bEnableCQB);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
			DCO_MoraleSettings.Get().m_bEnableCQB = var.GetBool();
	}
}
