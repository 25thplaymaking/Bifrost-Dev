modded class SCR_EditableEntityComponent
{
	[RplProp(onRplName: "DCO_ApplyMissionScale")]
	protected float m_fDCO_MissionScale;
	[RplProp(onRplName: "DCO_ApplyMissionInvincible")]
	protected int m_iDCO_MissionInvincible;

	bool DCO_SetMissionScale(float scale)
	{
		if (!Replication.IsServer() || !(scale >= 0.25 && scale <= 4.0) || !DCO_CanScale(GetOwner()))
			return false;
		m_fDCO_MissionScale = scale;
		DCO_ApplyMissionScale();
		Replication.BumpMe();
		return true;
	}

	static bool DCO_CanScale(IEntity entity)
	{
		return DCO_GetScaleIssue(entity).IsEmpty();
	}

	static string DCO_GetScaleIssue(IEntity entity)
	{
		if (!entity)
			return "The selected object no longer exists. Select it again.";
		if (ChimeraCharacter.Cast(entity))
			return "Characters cannot be scaled. Select a static prop or barricade.";
		if (Vehicle.Cast(entity))
			return "Vehicles cannot be scaled. Select a static prop or barricade.";
		if (entity.GetParent())
			return "Select the whole assembly instead of an attached part.";
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		if (!editable || editable.GetEntityType() != EEditableEntityType.GENERIC || SCR_EditableSystemComponent.Cast(editable))
			return "Select an editable static prop or barricade, not a system or inventory item.";

		// Composition roots have no mesh; validate the complete physical hierarchy.
		array<IEntity> parts = {entity};
		bool hasModel;
		for (int i = 0; i < parts.Count(); i++)
		{
			IEntity part = parts[i];
			if (ChimeraCharacter.Cast(part) || Vehicle.Cast(part))
				return "This assembly contains a character or vehicle and cannot be scaled.";
			Physics physics = part.GetPhysics();
			if (physics && (physics.IsDynamic() || physics.IsKinematic()))
				return "This object has moving physics parts. Select a fully static prop or barricade.";
			if (part.GetVObject())
				hasModel = true;
			IEntity child = part.GetChildren();
			while (child)
			{
				if (parts.Count() >= 512)
					return "This assembly is too large to scale. Select a smaller static assembly.";
				parts.Insert(child);
				child = child.GetSibling();
			}
		}
		if (!hasModel)
			return "This selection has no model to resize. Select a static prop or barricade.";
		return string.Empty;
	}

	protected void DCO_ApplyMissionScale()
	{
		IEntity entity = GetOwner();
		if (!entity || m_fDCO_MissionScale <= 0)
			return;
		entity.SetScale(m_fDCO_MissionScale);
		DCO_UpdateScaleHierarchy(entity);
	}

	protected static void DCO_UpdateScaleHierarchy(IEntity entity)
	{
		entity.Update();
		GenericEntity generic = GenericEntity.Cast(entity);
		if (generic)
			generic.OnTransformReset();
		IEntity child = entity.GetChildren();
		while (child)
		{
			DCO_UpdateScaleHierarchy(child);
			child = child.GetSibling();
		}
	}

	bool DCO_SetMissionInvincible(bool enabled)
	{
		if (!Replication.IsServer() || !GetOwner() || !GetOwner().FindComponent(DamageManagerComponent))
			return false;
		m_iDCO_MissionInvincible = 1;
		if (enabled)
			m_iDCO_MissionInvincible = 2;
		DCO_ApplyMissionInvincible();
		Replication.BumpMe();
		return true;
	}

	protected void DCO_ApplyMissionInvincible()
	{
		if (!GetOwner() || m_iDCO_MissionInvincible == 0)
			return;
		DamageManagerComponent damage = DamageManagerComponent.Cast(GetOwner().FindComponent(DamageManagerComponent));
		if (damage)
			damage.EnableDamageHandling(m_iDCO_MissionInvincible != 2);
	}
}

class DCO_GMTerrainHiddenState
{
	IEntity m_Entity;
	int m_iUsers;
	bool m_bVisible;
	int m_iCollisionMask;
}

[ComponentEditorProps(category: "Bifrost/Mission", description: "Replicated terrain-object hide area")]
class DCO_GMTerrainAreaComponentClass : ScriptComponentClass {}

class DCO_GMTerrainAreaComponent : ScriptComponent
{
	protected static ref array<DCO_GMTerrainAreaComponent> s_Areas;
	protected static ref array<ref DCO_GMTerrainHiddenState> s_Hidden;
	[Attribute("35", UIWidgets.Slider, "Radius in metres", "5 100 1"), RplProp()]
	protected float m_fRadius;
	[Attribute("1", UIWidgets.CheckBox, "Hide scenery inside this area"), RplProp()]
	protected bool m_bEnabled;
	protected float m_fAppliedRadius;
	protected vector m_vAppliedPosition;
	protected ref array<IEntity> m_Claimed = {};

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_Areas)
			s_Areas = {};
		if (!s_Hidden)
			s_Hidden = {};
		s_Areas.Insert(this);
		GetGame().GetCallqueue().CallLater(Refresh, 1000, true);
	}

	float GetRadius() { return m_fRadius; }
	bool IsEnabled() { return m_bEnabled; }
	void SetEnabled(bool enabled)
	{
		if (!Replication.IsServer()) return;
		ReleaseClaims();
		m_bEnabled = enabled;
		Replication.BumpMe();
		Refresh();
	}

	void Configure(float radius)
	{
		if (!Replication.IsServer())
			return;
		m_fRadius = Math.Clamp(radius, 5, 100);
		Replication.BumpMe();
		Refresh();
	}

	static int Count()
	{
		if (!s_Areas)
			return 0;
		return s_Areas.Count();
	}

	static void DrawCues(DCO_GMRenderManager render)
	{
		if (!DCO_GMRights.IsLocalGameMaster()) return;
		SCR_PlacingEditorComponent placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (placing && placing.GetSelectedPrefab() == DCO_PlacementCatalog.TERRAIN_AREA_RESOURCE)
		{
			SCR_MenuLayoutEditorComponent layout = SCR_MenuLayoutEditorComponent.Cast(SCR_MenuLayoutEditorComponent.GetInstance(SCR_MenuLayoutEditorComponent, false));
			vector cursor;
			if (layout && layout.GetCursorWorldPos(cursor)) DrawGroundRing(render, cursor, 35, 0xFF68B7CC);
		}
		if (!s_Areas) return;
		foreach (DCO_GMTerrainAreaComponent area : s_Areas)
		{
			if (!area || !area.GetOwner() || area.m_fRadius <= 0) continue;
			int color = 0xFFD9892B;
			if (!area.m_bEnabled) color = 0xFF9CA3AA;
			DrawGroundRing(render, area.GetOwner().GetOrigin(), area.m_fRadius, color);
		}
	}

	protected static void DrawGroundRing(DCO_GMRenderManager render, vector center, float radius, int color)
	{
		vector previous;
		for (int i = 0; i <= 48; i++)
		{
			float angle = Math.PI2 * i / 48.0;
			vector point = center + Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius);
			point[1] = GetGame().GetWorld().GetSurfaceY(point[0], point[2]) + 0.2;
			if (i > 0) render.DrawLine(previous, point, color);
			previous = point;
		}
	}

	static void RestoreAll()
	{
		if (!Replication.IsServer() || !s_Areas)
			return;
		array<IEntity> entities = {};
		foreach (DCO_GMTerrainAreaComponent area : s_Areas)
		{
			if (area && area.GetOwner())
				entities.Insert(area.GetOwner());
		}
		foreach (IEntity entity : entities)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}

	protected void Refresh()
	{
		if (!GetOwner()) return;
		vector position = GetOwner().GetOrigin();
		if (!m_bEnabled || m_fAppliedRadius != m_fRadius || vector.DistanceSq(position, m_vAppliedPosition) > 0.01)
		{
			ReleaseClaims();
			m_fAppliedRadius = m_fRadius;
			m_vAppliedPosition = position;
		}
		if (!m_bEnabled || m_fRadius <= 0) return;
		for (int i = m_Claimed.Count() - 1; i >= 0; i--)
		{
			if (!m_Claimed[i])
				m_Claimed.Remove(i);
		}
		for (int j = s_Hidden.Count() - 1; j >= 0; j--)
		{
			if (!s_Hidden[j].m_Entity)
				s_Hidden.Remove(j);
		}
		// Already-hidden roots may be absent from the spatial query after collision is disabled.
		array<IEntity> overlaps = {};
		foreach (DCO_GMTerrainHiddenState hidden : s_Hidden)
		{
			if (hidden.m_Entity && !hidden.m_Entity.GetParent() && InsideRadius(hidden.m_Entity.GetOrigin()))
				overlaps.Insert(hidden.m_Entity);
		}
		foreach (IEntity overlap : overlaps) Claim(overlap);
		vector extent = Vector(m_fRadius, 500, m_fRadius);
		GetGame().GetWorld().QueryEntitiesByAABB(position - extent, position + extent, Collect);
	}

	protected bool InsideRadius(vector position)
	{
		vector delta = position - GetOwner().GetOrigin();
		delta[1] = 0;
		return delta.LengthSq() <= m_fRadius * m_fRadius;
	}

	protected bool Collect(IEntity entity)
	{
		if (!entity || entity.GetParent() || !InsideRadius(entity.GetOrigin()) || ChimeraCharacter.Cast(entity) || Vehicle.Cast(entity))
			return true;
		// Only map scenery is claimed; editable objects retain their own state.
		if (entity.FindComponent(SCR_EditableEntityComponent))
			return true;
		Physics physics = entity.GetPhysics();
		if (physics && (physics.IsDynamic() || physics.IsKinematic())) return true;
		if (!entity.GetVObject() && !Building.Cast(entity) && !Tree.Cast(entity) && !TreeEntity.Cast(entity)) return true;
		Claim(entity);
		return true;
	}

	protected void Claim(IEntity entity)
	{
		if (!entity || m_Claimed.Contains(entity))
			return;
		m_Claimed.Insert(entity);
		DCO_GMTerrainHiddenState state;
		foreach (DCO_GMTerrainHiddenState candidate : s_Hidden)
		{
			if (candidate.m_Entity == entity)
			{
				state = candidate;
				break;
			}
		}
		if (!state)
		{
			state = new DCO_GMTerrainHiddenState();
			state.m_Entity = entity;
			state.m_bVisible = (entity.GetFlags() & EntityFlags.VISIBLE) != 0;
			Physics physics = entity.GetPhysics();
			if (physics)
			{
				state.m_iCollisionMask = physics.GetInteractionLayer();
				physics.SetInteractionLayer(0);
			}
			entity.ClearFlags(EntityFlags.VISIBLE, false);
			s_Hidden.Insert(state);
		}
		state.m_iUsers++;
		IEntity child = entity.GetChildren();
		while (child)
		{
			Claim(child);
			child = child.GetSibling();
		}
	}

	protected void ReleaseClaims()
	{
		if (s_Hidden)
		{
			for (int i = s_Hidden.Count() - 1; i >= 0; i--)
			{
				DCO_GMTerrainHiddenState state = s_Hidden[i];
				if (!m_Claimed.Contains(state.m_Entity))
					continue;
				state.m_iUsers--;
				if (state.m_iUsers > 0)
					continue;
				if (state.m_Entity)
				{
					if (state.m_bVisible)
						state.m_Entity.SetFlags(EntityFlags.VISIBLE, false);
					Physics physics = state.m_Entity.GetPhysics();
					if (physics)
						physics.SetInteractionLayer(state.m_iCollisionMask);
				}
				s_Hidden.Remove(i);
			}
		}
		m_Claimed.Clear();
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Refresh);
		if (s_Areas) s_Areas.RemoveItem(this);
		ReleaseClaims();
		super.OnDelete(owner);
	}
}

[BaseContainerProps()]
class DCO_TerrainRadiusEditorAttribute : SCR_BaseValueListEditorAttribute
{
	protected DCO_GMTerrainAreaComponent GetArea(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner()) return null;
		return DCO_GMTerrainAreaComponent.Cast(editable.GetOwner().FindComponent(DCO_GMTerrainAreaComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GMTerrainAreaComponent area = GetArea(item);
		if (!area) return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(area.GetRadius());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var || !Replication.IsServer() || !DCO_GMRights.Allow(playerID, "Terrain hide radius")) return;
		DCO_GMTerrainAreaComponent area = GetArea(item);
		if (area) area.Configure(var.GetFloat());
	}
}

[BaseContainerProps()]
class DCO_TerrainEnabledEditorAttribute : SCR_BaseEditorAttribute
{
	protected DCO_GMTerrainAreaComponent GetArea(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner()) return null;
		return DCO_GMTerrainAreaComponent.Cast(editable.GetOwner().FindComponent(DCO_GMTerrainAreaComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GMTerrainAreaComponent area = GetArea(item);
		if (!area) return null;
		return SCR_BaseEditorAttributeVar.CreateBool(area.IsEnabled());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var || !Replication.IsServer() || !DCO_GMRights.Allow(playerID, "Terrain hide enabled")) return;
		DCO_GMTerrainAreaComponent area = GetArea(item);
		if (area) area.SetEnabled(var.GetBool());
	}
}
