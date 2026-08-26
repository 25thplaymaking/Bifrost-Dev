// GM attributes for the "Bifrost BASE SETTINGS" tab.

[BaseContainerProps()]
class DCO_EnableBaseSettingsEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_BaseSettings.Get().m_bEnableBaseSettings);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_BaseSettings.Get().m_bEnableBaseSettings = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_AiSkillEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		return SCR_BaseEditorAttributeVar.CreateInt(DCO_BaseSettings.Get().m_iAiSkill);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_BaseSettings.Get().m_iAiSkill = var.GetInt();
	}
}

[BaseContainerProps()]
class DCO_PerceptionEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		return SCR_BaseEditorAttributeVar.CreateInt(DCO_BaseSettings.Get().m_iPerception);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_BaseSettings.Get().m_iPerception = var.GetInt();
	}
}

[BaseContainerProps()]
class DCO_ReactionTimeEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		return SCR_BaseEditorAttributeVar.CreateInt(DCO_BaseSettings.Get().m_iReactionTime);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_BaseSettings.Get().m_iReactionTime = var.GetInt();
	}
}

[BaseContainerProps()]
class DCO_FireRateEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		return SCR_BaseEditorAttributeVar.CreateInt(DCO_BaseSettings.Get().m_iFireRate);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_BaseSettings.Get().m_iFireRate = var.GetInt();
	}
}

// Stance dropdown.
[BaseContainerProps()]
class DCO_GlobalStanceEditorAttribute : DCO_ServerFloatHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		int index = ConvertValueToIndex(DCO_BaseSettings.Get().m_eGlobalStance);
		if (index != -1)
			return SCR_BaseEditorAttributeVar.CreateInt(index);
		return null;
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		float val;
		if (!ConvertIndexToValue(var.GetInt(), val)) return;
		DCO_BaseSettings.Get().m_eGlobalStance = Math.Round(val);
	}
}

// Default-formation dropdown.
[BaseContainerProps()]
class DCO_DefaultFormationEditorAttribute : DCO_ServerFloatHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		int index = ConvertValueToIndex(DCO_BaseSettings.Get().m_eDefaultSpawnFormation);
		if (index != -1)
			return SCR_BaseEditorAttributeVar.CreateInt(index);
		return null;
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		float val;
		if (!ConvertIndexToValue(var.GetInt(), val)) return;
		DCO_BaseSettings.Get().m_eDefaultSpawnFormation = Math.Round(val);
	}
}

// Troop-grade dropdown.
[BaseContainerProps()]
class DCO_GlobalGradeEditorAttribute : DCO_ServerFloatHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		int index = ConvertValueToIndex(DCO_BaseSettings.Get().m_eGlobalGrade);
		if (index != -1)
			return SCR_BaseEditorAttributeVar.CreateInt(index);
		return null;
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		float val;
		if (!ConvertIndexToValue(var.GetInt(), val)) return;
		DCO_BaseSettings cfg = DCO_BaseSettings.Get();
		int g = Math.Round(val);
		cfg.m_eGlobalGrade = g;
		DCO_BaseSettingsUtil.ApplyGrade(cfg, g);
	}
}
