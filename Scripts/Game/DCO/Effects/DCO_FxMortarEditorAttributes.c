// Game Master attributes for a placed FX: Mortar Strike Emitter.

class DCO_FxMortarAttributeBase : SCR_BaseValueListEditorAttribute
{
	protected DCO_FxMortarComponent GetEmitter(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;
		return DCO_FxMortarComponent.Cast(owner.FindComponent(DCO_FxMortarComponent));
	}
}

[BaseContainerProps()]
class DCO_FxMortarSpreadEditorAttribute : DCO_FxMortarAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetSpread());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetSpread(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxMortarCountEditorAttribute : DCO_FxMortarAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetSalvoCount());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetSalvoCount(Math.Round(var.GetFloat()));
	}
}

[BaseContainerProps()]
class DCO_FxMortarIntervalEditorAttribute : DCO_FxMortarAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetInterval());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetInterval(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxMortarSalvoPauseEditorAttribute : DCO_FxMortarAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetSalvoPause());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetSalvoPause(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxMortarLiveEditorAttribute : SCR_BaseEditorAttribute
{
	protected DCO_FxMortarComponent GetEmitter(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;
		return DCO_FxMortarComponent.Cast(owner.FindComponent(DCO_FxMortarComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_GetLive());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetLive(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_FxMortarSoundEditorAttribute : DCO_FxMortarLiveEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_GetSound());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetSound(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_FxMortarFiringEditorAttribute : DCO_FxMortarLiveEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_IsFiring());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxMortarComponent fx = GetEmitter(item);
		if (fx && fx.DCO_IsFiring() != var.GetBool())
			fx.DCO_SetFiring(var.GetBool());	// edge-only: a Yes -> Yes rewrite would cancel and restart the in-progress salvo.
	}
}
