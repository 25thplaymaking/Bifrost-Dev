enum EDCO_AmbientFxPreset
{
	CAMPFIRE,
	HEAVY_SMOKE,
	ELECTRIC_SPARKS,
	FIREFLIES
}

class DCO_FxAmbientComponentClass : ScriptComponentClass
{
}

class DCO_FxAmbientComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "Installed native emitter particle preset.", "", ParamEnumArray.FromEnum(EDCO_AmbientFxPreset), category: "Bifrost"), RplProp(onRplName: "DCO_OnVisualChanged")]
	protected int m_iPreset;

	[Attribute("1", UIWidgets.Slider, "Visual scale. Does not change gameplay.", "0.25 4 0.25", category: "Bifrost"), RplProp(onRplName: "DCO_OnVisualChanged")]
	protected float m_fScale;

	[Attribute("1", UIWidgets.CheckBox, "Show this emitter effect.", category: "Bifrost"), RplProp(onRplName: "DCO_OnVisualChanged")]
	protected bool m_bEnabled;

	protected ParticleEffectEntity m_Visual;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame() || !GetGame().InPlayMode())
			return;
		GetGame().GetCallqueue().CallLater(DCO_RebuildVisual, 0, false);
	}

	void ~DCO_FxAmbientComponent()
	{
		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(DCO_RebuildVisual);
		DCO_RemoveVisual();
	}

	static string DCO_PresetName(int preset)
	{
		switch (preset)
		{
			case EDCO_AmbientFxPreset.CAMPFIRE: return "Campfire";
			case EDCO_AmbientFxPreset.HEAVY_SMOKE: return "Heavy smoke";
			case EDCO_AmbientFxPreset.ELECTRIC_SPARKS: return "Electric sparks";
			case EDCO_AmbientFxPreset.FIREFLIES: return "Fireflies";
		}
		return "Emitter effect";
	}

	protected static ResourceName DCO_PresetResource(int preset)
	{
		switch (preset)
		{
			case EDCO_AmbientFxPreset.CAMPFIRE: return "Particles/Enviroment/Campfire_medium_normal.ptc";
			case EDCO_AmbientFxPreset.HEAVY_SMOKE: return "Particles/Props/Smoke_Generator_big.ptc";
			case EDCO_AmbientFxPreset.ELECTRIC_SPARKS: return "Particles/Props/Sparks_Electric_Medium.ptc";
			case EDCO_AmbientFxPreset.FIREFLIES: return "Particles/Enviroment/Fireflies.ptc";
		}
		return ResourceName.Empty;
	}

	int DCO_GetPreset()
	{
		return m_iPreset;
	}

	float DCO_GetScale()
	{
		return m_fScale;
	}

	bool DCO_GetEnabled()
	{
		return m_bEnabled;
	}

	void DCO_SetPreset(int preset)
	{
		if (!Replication.IsServer())
			return;
		preset = Math.ClampInt(preset, EDCO_AmbientFxPreset.CAMPFIRE, EDCO_AmbientFxPreset.FIREFLIES);
		if (m_iPreset == preset)
			return;
		m_iPreset = preset;
		DCO_CommitVisualChange();
	}

	void DCO_SetScale(float scale)
	{
		if (!Replication.IsServer())
			return;
		scale = Math.Clamp(scale, 0.25, 4.0);
		if (Math.AbsFloat(m_fScale - scale) < 0.001)
			return;
		m_fScale = scale;
		DCO_CommitVisualChange();
	}

	void DCO_SetEnabled(bool enabled)
	{
		if (!Replication.IsServer() || m_bEnabled == enabled)
			return;
		m_bEnabled = enabled;
		DCO_CommitVisualChange();
	}

	protected void DCO_CommitVisualChange()
	{
		Replication.BumpMe();
		DCO_RebuildVisual();
	}

	protected void DCO_OnVisualChanged()
	{
		DCO_RebuildVisual();
	}

	protected void DCO_RemoveVisual()
	{
		if (!m_Visual)
			return;
		SCR_EntityHelper.DeleteEntityAndChildren(m_Visual);
		m_Visual = null;
	}

	protected void DCO_RebuildVisual()
	{
		DCO_RemoveVisual();
		if (!m_bEnabled || System.IsConsoleApp())
			return;

		IEntity owner = GetOwner();
		if (!owner || !owner.GetWorld())
			return;
		ResourceName effectPath = DCO_PresetResource(m_iPreset);
		Resource effectResource;
		if (!effectPath.IsEmpty())
			effectResource = Resource.Load(effectPath);
		if (!effectResource)
		{
			return;
		}

		ParticleEffectEntitySpawnParams params = new ParticleEffectEntitySpawnParams();
		params.TargetWorld = owner.GetWorld();
		params.TransformMode = ETransformMode.WORLD;
		params.UseFrameEvent = true;
		params.DeleteWhenStopped = true;
		params.PlayOnSpawn = true;
		params.PlayOnHeadlessClient = false;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = owner.GetOrigin();
		m_Visual = ParticleEffectEntity.SpawnParticleEffect(effectPath, params);
		if (!m_Visual)
			return;
		m_Visual.SetScale(Math.Clamp(m_fScale, 0.25, 4.0));
		vector localTransform[4];
		Math3D.MatrixIdentity4(localTransform);
		m_Visual.SetFollowParent(owner, localTransform);
	}
}

class DCO_FxAmbientAttributeBase : SCR_BaseValueListEditorAttribute
{
	protected DCO_FxAmbientComponent GetEmitter(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner())
			return null;
		return DCO_FxAmbientComponent.Cast(editable.GetOwner().FindComponent(DCO_FxAmbientComponent));
	}
}

[BaseContainerProps()]
class DCO_FxAmbientPresetEditorAttribute : DCO_FxAmbientAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxAmbientComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(fx.DCO_GetPreset());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_FxAmbientComponent fx = GetEmitter(item);
		if (fx && var)
			fx.DCO_SetPreset(var.GetInt());
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		for (int i = EDCO_AmbientFxPreset.CAMPFIRE; i <= EDCO_AmbientFxPreset.FIREFLIES; i++)
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText(DCO_FxAmbientComponent.DCO_PresetName(i)));
		return outEntries.Count();
	}
}

[BaseContainerProps()]
class DCO_FxAmbientScaleEditorAttribute : DCO_FxAmbientAttributeBase
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxAmbientComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(fx.DCO_GetScale());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_FxAmbientComponent fx = GetEmitter(item);
		if (fx && var)
			fx.DCO_SetScale(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_FxAmbientEnabledEditorAttribute : SCR_BaseEditorAttribute
{
	protected DCO_FxAmbientComponent GetEmitter(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner())
			return null;
		return DCO_FxAmbientComponent.Cast(editable.GetOwner().FindComponent(DCO_FxAmbientComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxAmbientComponent fx = GetEmitter(item);
		if (!fx)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(fx.DCO_GetEnabled());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_FxAmbientComponent fx = GetEmitter(item);
		if (fx && var)
			fx.DCO_SetEnabled(var.GetBool());
	}
}
