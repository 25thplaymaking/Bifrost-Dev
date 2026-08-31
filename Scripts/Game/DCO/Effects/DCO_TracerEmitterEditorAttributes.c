// Game Master attributes for a placed FX: Tracer Emitter.

class DCO_TracerEmitterAttributeBase : SCR_BaseValueListEditorAttribute
{
	protected DCO_TracerEmitterComponent GetEmitter(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;
		return DCO_TracerEmitterComponent.Cast(owner.FindComponent(DCO_TracerEmitterComponent));
	}
}

[BaseContainerProps()]
class DCO_TracerRoundEditorAttribute : DCO_TracerEmitterAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(fx.DCO_GetRound());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetRound(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		foreach (string name : DCO_TracerEmitterComponent.DCO_GetRoundNames())
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(name));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_TracerDensityEditorAttribute : DCO_TracerEmitterAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetTracerEvery());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetTracerEvery(Math.Round(var.GetFloat()));
	}
}

[BaseContainerProps()]
class DCO_TracerRpmEditorAttribute : DCO_TracerEmitterAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetRpm());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetRpm(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TracerBurstEditorAttribute : DCO_TracerEmitterAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetBurstLen());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetBurstLen(Math.Round(var.GetFloat()));
	}
}

[BaseContainerProps()]
class DCO_TracerPauseEditorAttribute : DCO_TracerEmitterAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetPause());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetPause(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TracerLiveEditorAttribute : SCR_BaseEditorAttribute
{
	protected DCO_TracerEmitterComponent GetEmitter(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;
		return DCO_TracerEmitterComponent.Cast(owner.FindComponent(DCO_TracerEmitterComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_GetLive());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetLive(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_TracerSoundEditorAttribute : DCO_TracerLiveEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_GetSound());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetSound(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_TracerFiringEditorAttribute : DCO_TracerLiveEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_IsFiring());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TracerEmitterComponent fx = GetEmitter(item);
		if (fx && fx.DCO_IsFiring() != var.GetBool())
			fx.DCO_SetFiring(var.GetBool());	// edge-only: a Yes -> Yes rewrite would cancel and restart the in-progress burst.
	}
}
