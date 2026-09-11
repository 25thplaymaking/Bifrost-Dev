//! GM Scenario Settings entries for the server-owned Arsenal policy.

[BaseContainerProps()]
class BIA_CatalogScopeEditorAttribute : DCO_ServerFloatHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(ConvertValueToIndex(BIA_ArsenalScenarioSettings.Get().m_eCatalogScope));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		float value;
		if (!ConvertIndexToValue(var.GetInt(), value))
			return;
		BIA_ArsenalScenarioSettings settings = BIA_ArsenalScenarioSettings.Get();
		settings.m_eCatalogScope = Math.Round(value);
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class BIA_AllowWeaponsEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(BIA_ArsenalScenarioSettings.Get().m_bAllowWeapons);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		BIA_ArsenalScenarioSettings settings = BIA_ArsenalScenarioSettings.Get();
		settings.m_bAllowWeapons = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class BIA_AllowWearablesEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(BIA_ArsenalScenarioSettings.Get().m_bAllowWearables);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		BIA_ArsenalScenarioSettings settings = BIA_ArsenalScenarioSettings.Get();
		settings.m_bAllowWearables = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class BIA_AllowFieldGearEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(BIA_ArsenalScenarioSettings.Get().m_bAllowFieldGear);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		BIA_ArsenalScenarioSettings settings = BIA_ArsenalScenarioSettings.Get();
		settings.m_bAllowFieldGear = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class BIA_AllowKitChangesEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(BIA_ArsenalScenarioSettings.Get().m_bAllowKitChanges);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		BIA_ArsenalScenarioSettings settings = BIA_ArsenalScenarioSettings.Get();
		settings.m_bAllowKitChanges = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class BIA_UseRankLocksEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(BIA_ArsenalScenarioSettings.Get().m_bUseRankLocks);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		BIA_ArsenalScenarioSettings settings = BIA_ArsenalScenarioSettings.Get();
		settings.m_bUseRankLocks = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class BIA_UseSuppliesEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(BIA_ArsenalScenarioSettings.Get().m_bUseSupplies);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		BIA_ArsenalScenarioSettings settings = BIA_ArsenalScenarioSettings.Get();
		settings.m_bUseSupplies = var.GetBool();
		settings.Broadcast();
	}
}

[BaseContainerProps()]
class BIA_RestrictKitFactionEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(BIA_ArsenalScenarioSettings.Get().m_bRestrictKitFaction);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		BIA_ArsenalScenarioSettings settings = BIA_ArsenalScenarioSettings.Get();
		settings.m_bRestrictKitFaction = var.GetBool();
		settings.Broadcast();
	}
}
