class DCO_GearRackSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType() { return DCO_GearRackComponent; }
	override static EComponentDeserializeEvent GetDeserializeEvent() { return EComponentDeserializeEvent.BEFORE_POSTINIT; }

	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(component);
		context.WriteValue("version", 1);
		context.WriteValue("name", rack.GetRackName());
		context.WriteValue("operator", rack.GetOperatorName());
		context.WriteValue("persistent", rack.IsPersistent());
		return ESerializeResult.OK;
	}

	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		int version;
		string name, operatorName;
		bool persistent;
		if (!context.ReadValue("version", version) || version != 1
			|| !context.ReadValue("name", name) || !context.ReadValue("operator", operatorName)
			|| !context.ReadValue("persistent", persistent)) return false;
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(component);
		rack.RestoreRackSettings(name, operatorName, persistent);
		return true;
	}
}
