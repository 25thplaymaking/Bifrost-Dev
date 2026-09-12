class DCO_EditorAttributeScope
{
	static bool IsBifrost(SCR_BaseEditorAttribute attribute)
	{
		if (!attribute)
			return false;
		string type = attribute.Type().ToString();
		return type.StartsWith("DCO_") || type.StartsWith("BIA_");
	}

	static bool AppliesToAll(SCR_BaseEditorAttribute attribute, array<Managed> items, SCR_AttributesManagerEditorComponent manager)
	{
		if (!attribute || !items || items.IsEmpty())
			return false;
		foreach (Managed item : items)
		{
			if (!item || !attribute.ReadVariable(item, manager))
				return false;
		}
		return true;
	}
}

modded class SCR_AttributesManagerEditorComponent
{
	override protected int GetVariables(bool onlyServer, notnull array<Managed> items, notnull out array<int> outIds,
		notnull out array<ref SCR_BaseEditorAttributeVar> outVars, notnull out array<ref EEditorAttributeMultiSelect> outAttributesMultiSelect)
	{
		super.GetVariables(onlyServer, items, outIds, outVars, outAttributesMultiSelect);
		SCR_AttributesManagerEditorComponentClass data = SCR_AttributesManagerEditorComponentClass.Cast(GetEditorComponentData());
		if (!data)
			return outVars.Count();
		for (int i = outIds.Count() - 1; i >= 0; i--)
		{
			SCR_BaseEditorAttribute attribute = data.GetAttribute(outIds[i]);
			// Server-owned settings are checked only during the server collection phase.
			if (!DCO_EditorAttributeScope.IsBifrost(attribute) || attribute.IsServer() != onlyServer)
				continue;
			if (DCO_EditorAttributeScope.AppliesToAll(attribute, items, this))
				continue;
			outIds.RemoveOrdered(i);
			outVars.RemoveOrdered(i);
			outAttributesMultiSelect.RemoveOrdered(i);
		}
		return outVars.Count();
	}
}
