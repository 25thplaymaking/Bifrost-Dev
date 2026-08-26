// Game Master attributes for a placed FX: Explosion Emitter.

class DCO_FxExplosionAttributeBase : SCR_BaseValueListEditorAttribute
{
	protected int DCO_FamilyMask() { return 0xFF; }

	protected DCO_FxExplosionComponent GetEmitter(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;
		DCO_FxExplosionComponent fx = DCO_FxExplosionComponent.Cast(owner.FindComponent(DCO_FxExplosionComponent));
		if (!fx || (DCO_FamilyMask() & fx.DCO_GetFamilyBit()) == 0)
			return null;
		return fx;
	}
}

[BaseContainerProps()]
class DCO_FxExplosionSizeEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.STRIKE); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(fx.DCO_GetSize());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetSize(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		foreach (string name : DCO_FxExplosionComponent.SIZE_NAMES)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(name));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_FxExplosionRepeatEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AUDIO); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetRepeat());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetRepeat(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxStrikeRepeatEditorAttribute : DCO_FxExplosionRepeatEditorAttribute
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.STRIKE); }
}

[BaseContainerProps()]
class DCO_FxAirRepeatEditorAttribute : DCO_FxExplosionRepeatEditorAttribute
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT); }
}

[BaseContainerProps()]
class DCO_FxExplosionLiveEditorAttribute : SCR_BaseEditorAttribute
{
	// LIVE arms real ordnance, which only the strike and gunrun deliveries have - an Audio emitter has nothing to arm.
	protected int DCO_FamilyMask()
	{
		return (1 << EDCO_FxFamily.STRIKE) | (1 << EDCO_FxFamily.AIRSUPPORT) | (1 << EDCO_FxFamily.LOITER);
	}

	protected DCO_FxExplosionComponent GetEmitter(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;
		DCO_FxExplosionComponent fx = DCO_FxExplosionComponent.Cast(owner.FindComponent(DCO_FxExplosionComponent));
		if (!fx || (DCO_FamilyMask() & fx.DCO_GetFamilyBit()) == 0)
			return null;
		return fx;
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_GetLive());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetLive(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_FxExplosionSoundEditorAttribute : DCO_FxExplosionLiveEditorAttribute
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.STRIKE) | (1 << EDCO_FxFamily.AIRSUPPORT) | (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_GetSound());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetSound(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_FxExplosionFiringEditorAttribute : DCO_FxExplosionLiveEditorAttribute
{
	protected override int DCO_FamilyMask() { return 0xFF; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_IsFiring());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx && fx.DCO_IsFiring() != var.GetBool())
			fx.DCO_SetFiring(var.GetBool());	// edge-only: a Yes -> Yes rewrite would cancel and restart the in-progress barrage.
	}
}


class DCO_FxDeliveryAttributeBase : DCO_FxExplosionAttributeBase
{
	protected int DCO_Family() { return EDCO_FxFamily.STRIKE; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(fx.DCO_GetDeliveryIndex());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetDeliveryIndex(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		array<int> allowed = {};
		DCO_FxExplosionComponent.DCO_FamilyDeliveries(DCO_Family(), allowed);
		foreach (int ordinal : allowed)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(DCO_FxExplosionComponent.DELIVERY_NAMES[ordinal]));
		return outEntries.Count();
	}
}

// Explosion / Strike placeable: Ground Warhead, Hydra 70 M229, S-5KO.
[BaseContainerProps()]
class DCO_FxExplosionDeliveryEditorAttribute : DCO_FxDeliveryAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.STRIKE); }
	protected override int DCO_Family() { return EDCO_FxFamily.STRIKE; }
}

// Air Support placeable: UH-1H Flyby, UH-1H Gunrun.
[BaseContainerProps()]
class DCO_FxAirDeliveryEditorAttribute : DCO_FxDeliveryAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT); }
	protected override int DCO_Family() { return EDCO_FxFamily.AIRSUPPORT; }
}

// Loiter placeable: Observation Orbit, Armed Orbit.
[BaseContainerProps()]
class DCO_FxLoiterDeliveryEditorAttribute : DCO_FxDeliveryAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.LOITER); }
	protected override int DCO_Family() { return EDCO_FxFamily.LOITER; }
}

[BaseContainerProps()]
class DCO_FxLoiterTimeEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetLoiterSec());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetLoiterSec(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxTargetFactionEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(fx.DCO_GetTargetFaction());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetTargetFaction(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		foreach (string name : DCO_FxExplosionComponent.TARGET_FACTION_NAMES)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(name));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_FxTargetTypeEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(fx.DCO_GetTargetType());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetTargetType(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		foreach (string name : DCO_FxExplosionComponent.TARGET_TYPE_NAMES)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(name));
		return outEntries.Count();
	}
}

// Loiter: gunrun fires the whole station regardless of the Rounds budget.
[BaseContainerProps()]
class DCO_FxContinuousFireEditorAttribute : DCO_FxExplosionLiveEditorAttribute
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_GetContinuousFire());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetContinuousFire(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_FxAircraftEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT) | (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(fx.DCO_GetAircraftIndex());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetAircraftIndex(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		for (int i = 0; i < DCO_FxAircraftCatalog.Count(); i++)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(DCO_FxAircraftCatalog.NameAt(i)));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_FxExplosionBarrageCountEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.STRIKE); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetBarrageCount());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetBarrageCount(Math.Round(var.GetFloat()));
	}
}

[BaseContainerProps()]
class DCO_FxAirPassCountEditorAttribute : DCO_FxExplosionBarrageCountEditorAttribute
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT); }
}

[BaseContainerProps()]
class DCO_FxExplosionScatterEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AUDIO); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetScatter());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetScatter(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxStrikeScatterEditorAttribute : DCO_FxExplosionScatterEditorAttribute
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.STRIKE); }
}

[BaseContainerProps()]
class DCO_FxAirScatterEditorAttribute : DCO_FxExplosionScatterEditorAttribute
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT) | (1 << EDCO_FxFamily.LOITER); }
}

[BaseContainerProps()]
class DCO_FxExplosionShotSpacingEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.STRIKE); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetShotSpacing());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetShotSpacing(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxAirPassIntervalEditorAttribute : DCO_FxExplosionShotSpacingEditorAttribute
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT); }
}

[BaseContainerProps()]
class DCO_FxExplosionTrackPlayersEditorAttribute : DCO_FxExplosionLiveEditorAttribute
{
	protected override int DCO_FamilyMask() { return 0xFF; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_GetTrackPlayers());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetTrackPlayers(var.GetBool());
	}
}

[BaseContainerProps()]
class DCO_FxExplosionTrackingRadiusEditorAttribute : DCO_FxExplosionAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetTrackingRadius());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetTrackingRadius(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxExplosionParticleScaleEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.STRIKE); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetParticleScale());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx)
			fx.DCO_SetParticleScale(var.GetFloat());
	}
}


[BaseContainerProps()]
class DCO_FxExplosionFlybySpeedEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT) | (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx) return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetFlybySpeed());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx) fx.DCO_SetFlybySpeed(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxExplosionFlybyHeightEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT) | (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx) return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetFlybyHeight());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx) fx.DCO_SetFlybyHeight(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxExplosionFlybyDistanceEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT) | (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx) return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetFlybyDistance());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx) fx.DCO_SetFlybyDistance(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxExplosionGunrunRoundsEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT) | (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx) return null;
		// Keep this attribute in the active session even when Continuous Fire begins enabled.
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetGunrunRounds());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx) fx.DCO_SetGunrunRounds(Math.Round(var.GetFloat()));
	}
}

[BaseContainerProps()]
class DCO_FxExplosionGunrunRpmEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AIRSUPPORT) | (1 << EDCO_FxFamily.LOITER); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx) return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetGunrunRpm());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx) fx.DCO_SetGunrunRpm(var.GetFloat());
	}
}


[BaseContainerProps()]
class DCO_FxExplosionCustomSoundRadiusEditorAttribute : DCO_FxExplosionAttributeBase
{
	protected override int DCO_FamilyMask() { return (1 << EDCO_FxFamily.AUDIO); }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (!fx) return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetCustomSoundRadius());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		DCO_FxExplosionComponent fx = GetEmitter(item);
		if (fx) fx.DCO_SetCustomSoundRadius(var.GetFloat());
	}
}
