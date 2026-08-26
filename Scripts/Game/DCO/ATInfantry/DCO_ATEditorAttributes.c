
[BaseContainerProps()]
class DCO_EnableLauncherDisciplineEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_ATSettings.Get().m_bEnableLauncherDiscipline);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_ATSettings.Get().m_bEnableLauncherDiscipline = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_LauncherStowWhenDryEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_ATSettings.Get().m_bLauncherStowWhenDry);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_ATSettings.Get().m_bLauncherStowWhenDry = var.GetBool();
	}
}

// Metre slider -> stored float range: inside this, infantry targets are rifle work.
[BaseContainerProps()]
class DCO_LauncherInfantryRangeEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		int metres = Math.Round(DCO_ATSettings.Get().m_fLauncherInfantryRange);
		return SCR_BaseEditorAttributeVar.CreateInt(metres);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_ATSettings.Get().m_fLauncherInfantryRange = var.GetInt();
	}
}

// Infantry sheltering in/at a building may be rocketed regardless of range.
[BaseContainerProps()]
class DCO_LauncherVsBuildingsEditorAttribute : DCO_ServerEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		return SCR_BaseEditorAttributeVar.CreateBool(DCO_ATSettings.Get().m_bLauncherVsBuildings);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_ATSettings.Get().m_bLauncherVsBuildings = var.GetBool();
	}
}

[BaseContainerProps()]
class DCO_LauncherVsInfantryChanceEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		int pct = Math.Round(DCO_ATSettings.Get().m_fLauncherVsInfantryChance * 100);
		return SCR_BaseEditorAttributeVar.CreateInt(pct);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_ATSettings.Get().m_fLauncherVsInfantryChance = var.GetInt() / 100.0;
	}
}

[BaseContainerProps()]
class DCO_LauncherDisciplineCheckSecEditorAttribute : DCO_ServerValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item)) return null;
		int secs = Math.Round(DCO_ATSettings.Get().m_fLauncherDisciplineCheckSec);
		return SCR_BaseEditorAttributeVar.CreateInt(secs);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var) DCO_ATSettings.Get().m_fLauncherDisciplineCheckSec = var.GetInt();
	}
}
