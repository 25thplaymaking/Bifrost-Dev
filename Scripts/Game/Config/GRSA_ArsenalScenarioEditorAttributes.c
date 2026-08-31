//! GM Scenario Settings entries for the server-owned Arsenal policy.

[BaseContainerProps()]
class GRSA_CatalogScopeEditorAttribute : DCO_ServerFloatHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(ConvertValueToIndex(GRSA_ArsenalScenarioSettings.Get().m_eCatalogScope));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		float value;
		if (!ConvertIndexToValue(var.GetInt(), value))
			return;
		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		settings.m_eCatalogScope = Math.Round(value);
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class GRSA_AllowWeaponsEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(GRSA_ArsenalScenarioSettings.Get().m_bAllowWeapons);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		settings.m_bAllowWeapons = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class GRSA_AllowWearablesEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(GRSA_ArsenalScenarioSettings.Get().m_bAllowWearables);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		settings.m_bAllowWearables = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class GRSA_AllowFieldGearEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(GRSA_ArsenalScenarioSettings.Get().m_bAllowFieldGear);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		settings.m_bAllowFieldGear = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class GRSA_AllowKitChangesEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(GRSA_ArsenalScenarioSettings.Get().m_bAllowKitChanges);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		settings.m_bAllowKitChanges = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class GRSA_UseRankLocksEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(GRSA_ArsenalScenarioSettings.Get().m_bUseRankLocks);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		settings.m_bUseRankLocks = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class GRSA_UseSuppliesEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(GRSA_ArsenalScenarioSettings.Get().m_bUseSupplies);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		settings.m_bUseSupplies = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class GRSA_RestrictKitFactionEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(GRSA_ArsenalScenarioSettings.Get().m_bRestrictKitFaction);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		GRSA_ArsenalScenarioSettings settings = GRSA_ArsenalScenarioSettings.Get();
		settings.m_bRestrictKitFaction = var.GetBool();
		settings.Broadcast();
	}
}
