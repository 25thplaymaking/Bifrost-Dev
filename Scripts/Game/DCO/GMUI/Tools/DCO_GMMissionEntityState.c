// Overlapping hidden parents and children share the original local visibility.
class DCO_GMMissionHiddenPart
{
	protected static ref map<IEntity, ref DCO_GMMissionHiddenPart> s_Parts;
	IEntity m_Entity;
	protected int m_iUsers;
	protected bool m_bVisible;

	static DCO_GMMissionHiddenPart Find(IEntity entity)
	{
		if (!s_Parts) return null;
		return s_Parts.Get(entity);
	}

	static DCO_GMMissionHiddenPart Acquire(IEntity entity)
	{
		if (!s_Parts) s_Parts = new map<IEntity, ref DCO_GMMissionHiddenPart>();
		DCO_GMMissionHiddenPart part = s_Parts.Get(entity);
		if (!part)
		{
			part = new DCO_GMMissionHiddenPart();
			part.m_Entity = entity;
			part.m_bVisible = (entity.GetFlags() & EntityFlags.VISIBLE) != 0;
			s_Parts.Insert(entity, part);
		}
		part.m_iUsers++;
		return part;
	}

	static void Release(DCO_GMMissionHiddenPart part)
	{
		if (!part) return;
		part.m_iUsers--;
		if (part.m_iUsers > 0) return;
		if (part.m_Entity)
		{
			if (part.m_bVisible) part.m_Entity.SetFlags(EntityFlags.VISIBLE, false);
			else part.m_Entity.ClearFlags(EntityFlags.VISIBLE, false);
		}
		// Find by value as deleted entities no longer provide a usable map key.
		for (int i = s_Parts.Count() - 1; i >= 0; i--)
		{
			if (s_Parts.GetElement(i) == part) s_Parts.RemoveElement(i);
		}
		if (s_Parts.Count() == 0) s_Parts = null;
	}
}

modded class SCR_EditableEntityComponent
{
	[RplProp(onRplName: "DCO_ApplyMissionScale")]
	protected float m_fDCO_MissionScale;
	protected int m_iDCO_PresentationOwnedEvents;
	[RplProp(onRplName: "DCO_ApplyMissionVisibility")]
	protected bool m_bDCO_MissionHidden;
	protected ref array<ref DCO_GMMissionHiddenPart> m_DCO_HiddenParts;
	[RplProp()] protected float m_fDCO_MovementFactor;
	protected ref map<HitZone, float> m_DCO_BaseHealth;
	bool DCO_HasMissionScale() { return m_fDCO_MissionScale > 0; }
	float DCO_GetMovementFactor()
	{
		if (m_fDCO_MovementFactor > 0) return m_fDCO_MovementFactor;
		return 1;
	}
	bool DCO_SetMovementFactor(float factor)
	{
		if (!Replication.IsServer() || !(factor >= 0.01 && factor <= 100)) return false;
		m_fDCO_MovementFactor = factor;
		Replication.BumpMe();
		return true;
	}
	protected void DCO_ScaleHealth(float scale)
	{
		if (!Replication.IsServer() || !ChimeraCharacter.Cast(GetOwner())) return;
		DamageManagerComponent damage = DamageManagerComponent.Cast(GetOwner().FindComponent(DamageManagerComponent));
		if (!damage) return;
		array<HitZone> zones = {};
		damage.GetAllHitZones(zones);
		HitZone rootZone = damage.GetDefaultHitZone();
		if (rootZone && !zones.Contains(rootZone)) zones.Insert(rootZone);
		if (!m_DCO_BaseHealth) m_DCO_BaseHealth = new map<HitZone, float>();
		foreach (HitZone zone : zones)
		{
			if (!zone) continue;
			float baseHealth;
			if (!m_DCO_BaseHealth.Find(zone, baseHealth))
			{
				baseHealth = zone.GetMaxHealth();
				m_DCO_BaseHealth.Insert(zone, baseHealth);
			}
			zone.SetMaxHealth(baseHealth * scale, ESetMaxHealthFlags.SCALED);
		}
	}
	[RplProp(onRplName: "DCO_ApplyMissionInvincible")]
	protected int m_iDCO_MissionInvincible;

	bool DCO_SetMissionScale(float scale)
	{
		DCO_TestDiagnostics.Event("gm.scale.request", string.Format("value=%1", scale));
		if (!Replication.IsServer() || !DCO_IsMissionScaleValid(scale) || !DCO_CanScale(GetOwner()))
			return false;
		IEntity entity = GetOwner();
		float previousScale = entity.GetScale();
		entity.SetScale(scale);
		if (!float.AlmostEqual(entity.GetScale(), scale, 0.0001))
		{
			entity.SetScale(previousScale);
			return false;
		}
		m_fDCO_MissionScale = scale;
		DCO_ScaleHealth(scale);
		DCO_ApplyMissionScale();
		Replication.BumpMe();
		return true;
	}

	void DCO_ReceiveMissionScale(float scale)
	{
		if (Replication.IsServer() || !DCO_IsMissionScaleValid(scale) || !GetOwner())
			return;
		RplId diagnosticId;
		IsReplicated(diagnosticId);
		DCO_TestDiagnostics.Event("gm.scale.receive", string.Format("target=%1 value=%2", diagnosticId, scale));
		m_fDCO_MissionScale = scale;
		DCO_ApplyMissionScale();
	}

	override bool RplSave(ScriptBitWriter writer)
	{
		if (!super.RplSave(writer))
			return false;
		bool hasScale = m_fDCO_MissionScale > 0;
		writer.WriteBool(hasScale);
		if (hasScale)
			writer.WriteFloat(m_fDCO_MissionScale);
		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		if (!super.RplLoad(reader))
			return false;
		bool hasScale;
		if (!reader.ReadBool(hasScale))
			return false;
		if (!hasScale)
		{
			m_fDCO_MissionScale = 0;
			return true;
		}
		if (!reader.ReadFloat(m_fDCO_MissionScale))
			return false;
		RplId diagnosticId;
		IsReplicated(diagnosticId);
		DCO_TestDiagnostics.Event("gm.scale.jip", string.Format("target=%1 value=%2", diagnosticId, m_fDCO_MissionScale));
		DCO_ApplyMissionScale();
		return true;
	}

	static bool DCO_IsMissionScaleValid(float scale)
	{
		return scale >= 0.01 && scale <= 100.0;
	}

	static SCR_EditableEntityComponent DCO_ResolveMissionTarget(SCR_EditableEntityComponent editable)
	{
		SCR_EditablePlayerDelegateComponent player = SCR_EditablePlayerDelegateComponent.Cast(editable);
		if (player)
			return player.GetControlledEntity();
		return editable;
	}

	static bool DCO_CanScale(IEntity entity)
	{
		return DCO_GetScaleIssue(entity).IsEmpty();
	}

	static string DCO_GetScaleIssue(IEntity entity)
	{
		if (!entity || entity.IsDeleted())
			return "The selected object no longer exists. Select it again.";
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		RplId id;
		if (!editable || !editable.IsReplicated(id) || !id.IsValid())
			return "This object has no replicated GM target. Select a GM-editable entity.";
		return string.Empty;
	}

	protected void DCO_ApplyMissionScale()
	{
		IEntity entity = GetOwner();
		if (!entity || m_fDCO_MissionScale <= 0) return;
		RplId diagnosticId;
		IsReplicated(diagnosticId);
		DCO_TestDiagnostics.Event("gm.scale.apply", string.Format("target=%1 requested=%2 before=%3", diagnosticId, m_fDCO_MissionScale, entity.GetScale()));
		entity.SetScale(m_fDCO_MissionScale);
		DCO_TestDiagnostics.Event("gm.scale.result", string.Format("target=%1 actual=%2", diagnosticId, entity.GetScale()), !float.AlmostEqual(entity.GetScale(), m_fDCO_MissionScale, 0.0001));
		DCO_UpdatePresentationEvents();
	}

	bool DCO_IsMissionHidden() { return m_bDCO_MissionHidden; }

	bool DCO_SetMissionHidden(bool hidden)
	{
		if (!Replication.IsServer() || !DCO_CanScale(GetOwner())) return false;
		m_bDCO_MissionHidden = hidden;
		DCO_ApplyMissionVisibility();
		Replication.BumpMe();
		return true;
	}

	protected void DCO_UpdatePresentationEvents()
	{
		IEntity entity = GetOwner();
		if (!entity) return;
		if ((m_fDCO_MissionScale > 0 && m_fDCO_MissionScale != 1.0) || m_bDCO_MissionHidden)
		{
			// FRAME keeps transformed entities active and their rendering bounds updated.
			int required = EntityEvent.FRAME | EntityEvent.POSTFRAME;
			m_iDCO_PresentationOwnedEvents |= required & ~GetEventMask();
			SetEventMask(entity, required);
		}
		else if (m_iDCO_PresentationOwnedEvents)
		{
			ClearEventMask(entity, m_iDCO_PresentationOwnedEvents);
			m_iDCO_PresentationOwnedEvents = 0;
		}
	}

	protected void DCO_ApplyMissionVisibility()
	{
		RplId diagnosticId;
		IsReplicated(diagnosticId);
		DCO_TestDiagnostics.Event("gm.visibility.apply", string.Format("target=%1 hidden=%2", diagnosticId, m_bDCO_MissionHidden));
		DCO_UpdateMissionVisibility();
		DCO_UpdatePresentationEvents();
	}

	protected bool DCO_IsVisibilityPart(IEntity entity)
	{
		IEntity root = GetOwner();
		while (entity)
		{
			if (entity == root) return true;
			entity = entity.GetParent();
		}
		return false;
	}

	protected void DCO_HideVisibilityTree(IEntity entity)
	{
		if (!entity) return;
		DCO_GMMissionHiddenPart part = DCO_GMMissionHiddenPart.Find(entity);
		if (!part || !m_DCO_HiddenParts.Contains(part))
			m_DCO_HiddenParts.Insert(DCO_GMMissionHiddenPart.Acquire(entity));
		if (entity.GetFlags() & EntityFlags.VISIBLE) entity.ClearFlags(EntityFlags.VISIBLE, false);
		IEntity child = entity.GetChildren();
		while (child)
		{
			DCO_HideVisibilityTree(child);
			child = child.GetSibling();
		}
	}

	protected void DCO_UpdateMissionVisibility()
	{
		if (m_DCO_HiddenParts)
		{
			for (int i = m_DCO_HiddenParts.Count() - 1; i >= 0; i--)
			{
				DCO_GMMissionHiddenPart part = m_DCO_HiddenParts[i];
				if (m_bDCO_MissionHidden && part.m_Entity && DCO_IsVisibilityPart(part.m_Entity)) continue;
				DCO_GMMissionHiddenPart.Release(part);
				m_DCO_HiddenParts.Remove(i);
			}
		}
		if (!m_bDCO_MissionHidden) return;
		if (!m_DCO_HiddenParts) m_DCO_HiddenParts = {};
		// Catch equipment attached later and flags restored by character actions.
		DCO_HideVisibilityTree(GetOwner());
	}

	protected void DCO_RetainMissionPresentation(IEntity owner)
	{
		if (!owner) return;
		if (m_fDCO_MissionScale > 0 && !float.AlmostEqual(owner.GetScale(), m_fDCO_MissionScale, 0.0001))
			owner.SetScale(m_fDCO_MissionScale);
		if (m_bDCO_MissionHidden) DCO_UpdateMissionVisibility();
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		DCO_RetainMissionPresentation(owner);
	}

	override void EOnPostFrame(IEntity owner, float timeSlice)
	{
		super.EOnPostFrame(owner, timeSlice);
		DCO_RetainMissionPresentation(owner);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_DCO_HiddenParts)
		{
			foreach (DCO_GMMissionHiddenPart part : m_DCO_HiddenParts)
				DCO_GMMissionHiddenPart.Release(part);
			m_DCO_HiddenParts = null;
		}
		super.OnDelete(owner);
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

modded class SCR_CharacterControllerComponent
{
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		super.OnPrepareControls(owner, am, dt, player);
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(owner.FindComponent(SCR_EditableEntityComponent));
		if (editable && editable.DCO_HasMissionScale())
			OverrideMaxSpeed(Math.Clamp(editable.DCO_GetMovementFactor() * owner.GetScale(), 0.01, 1));
	}
	override void OnApplyControls(IEntity owner, float timeSlice)
	{
		super.OnApplyControls(owner, timeSlice);
		DCO_AssistMovement(owner, timeSlice);
	}
	protected void DCO_AssistMovement(IEntity owner, float timeSlice)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(owner.FindComponent(SCR_EditableEntityComponent));
		if (!editable || !editable.DCO_HasMissionScale()) return;
		float factor = Math.Clamp(editable.DCO_GetMovementFactor() * owner.GetScale(), 0.01, 100);
		if (factor <= 1 || timeSlice <= 0) return;
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		// Player prediction and server-owned AI each apply the assistance only on their simulation owner.
		if (rpl && !rpl.IsOwner()) return;
		if (!rpl && !Replication.IsServer()) return;
		CharacterAnimationComponent animation = GetAnimationComponent();
		if (!animation || animation.PhysicsIsFalling() || animation.PhysicsIsLinked() || animation.IsRagdollActive()) return;
		CharacterCommandHandlerComponent commands = animation.GetCommandHandler();
		if (!commands || !commands.GetCommandMove() || IsDead() || IsUnconscious() || IsChangingStance()) return;
		CompartmentAccessComponent access = CompartmentAccessComponent.Cast(owner.FindComponent(CompartmentAccessComponent));
		if (access && access.IsInCompartment()) return;
		vector velocity = GetMovementVelocity();
		velocity[1] = 0;
		float speed = velocity.Length();
		if (speed < 0.05 || GetMovementInput().LengthSq() < 0.001) return;
		float extraSpeed = Math.Max(0, Math.Min(speed * factor, 35) - speed);
		vector displacement = velocity.Normalized() * extraSpeed * Math.Min(timeSlice, 0.05);
		if (displacement.LengthSq() < 0.000001) return;
		vector mins, maxs;
		if (!animation.GetCollisionMinMax(GetStance(), mins, maxs)) return;
		float scale = owner.GetScale();
		float radius = Math.Max(Math.Max(Math.AbsFloat(mins[0]), Math.AbsFloat(maxs[0])), Math.Max(Math.AbsFloat(mins[2]), Math.AbsFloat(maxs[2]))) * scale;
		TraceBox trace = new TraceBox();
		trace.Start = owner.GetOrigin();
		trace.End = trace.Start + displacement;
		trace.Mins = Vector(-radius, mins[1] * scale + 0.05, -radius);
		trace.Maxs = Vector(radius, maxs[1] * scale, radius);
		trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		trace.TargetLayers = EPhysicsLayerDefs.Character;
		trace.Exclude = owner;
		float fraction = owner.GetWorld().TraceMove(trace, null);
		float distance = displacement.Length();
		if (fraction < 1) fraction = Math.Max(0, fraction - Math.Min(0.02, distance) / distance);
		if (fraction <= 0) return;
		owner.SetOrigin(trace.Start + displacement * fraction);
		// Keep the kinematic body aligned without a teleport broadcast on every frame.
		OnTransformReset(true, GetVelocity());
	}

}

modded class CharacterCamera3rdPersonBase
{
	override void OnUpdate(float pDt, out ScriptedCameraItemResult pOutResult)
	{
		super.OnUpdate(pDt, pOutResult);
		if (!m_OwnerCharacter) return;
		float scale = m_OwnerCharacter.GetScale();
		if (float.AlmostEqual(scale, 1, 0.0001)) return;
		// Keep the clipping plane appropriate to the character's size.
		pOutResult.m_fNearPlane = Math.Clamp(0.04 * scale, 0.002, 0.2);
	}
}

modded class SCR_CharacterCameraHandlerComponent
{
	override void CollisionSolver(float pDt, inout ScriptedCameraItemResult pOutResult, inout vector resCamTM[4], bool isKeyframe)
	{
		ChimeraCharacter owner = ChimeraCharacter.Cast(pOutResult.m_pOwner);
		if (!owner || float.AlmostEqual(owner.GetScale(), 1, 0.0001) || !Is3rdPersonView() || owner.IsInVehicle() || pOutResult.m_fPositionModelSpace != 1 || pOutResult.m_pWSAttachmentReference)
		{
			super.CollisionSolver(pDt, pOutResult, resCamTM, isKeyframe);
			return;
		}
		float scale = owner.GetScale();
		vector ownerTM[4], rotationTM[4], localCamera[4];
		owner.GetWorldTransform(ownerTM);
		Math3D.MatrixCopy(ownerTM, rotationTM);
		for (int axis = 0; axis < 3; axis++) rotationTM[axis].Normalize();
		Math3D.MatrixCopy(pOutResult.m_CameraTM, localCamera);
		vector headingRotation[4];
		Math3D.MatrixIdentity4(headingRotation);
		if (pOutResult.m_fUseHeading > 0)
		{
			vector localRotation[4], heading[4];
			owner.GetLocalTransform(localRotation);
			for (int index = 0; index < 3; index++) localRotation[index].Normalize();
			localRotation[3] = vector.Zero;
			Math3D.AnglesToMatrix(Vector(-pOutResult.m_fHeading * Math.RAD2DEG, 0, 0), heading);
			Math3D.MatrixInvMultiply4(localRotation, heading, headingRotation);
			float rotation[4], blended[4], identity[4] = {0, 0, 0, 1};
			Math3D.MatrixToQuat(headingRotation, rotation);
			Math3D.QuatLerp(blended, identity, rotation, pOutResult.m_fUseHeading);
			Math3D.AnglesToMatrix(Math3D.QuatToAngles(blended), headingRotation);
			Math3D.MatrixMultiply4(headingRotation, localCamera, localCamera);
		}
		vector worldCamera[4];
		Math3D.MatrixMultiply4(rotationTM, localCamera, worldCamera);
		worldCamera[3] = localCamera[3].Multiply4(ownerTM);
		vector boom = pOutResult.m_vBacktraceDir.Multiply3(headingRotation).Multiply3(rotationTM);
		boom = vector.Lerp(worldCamera[2], boom, pOutResult.m_fUseBacktraceDir).Normalized();
		vector pivot = worldCamera[3];
		vector desired = pivot - boom * pOutResult.m_fDistance * scale;
		if (pOutResult.m_bAllowCollisionSolver && !isKeyframe)
		{
			TraceSphere trace = new TraceSphere();
			trace.Radius = Math.Clamp(0.06 * scale, 0.005, 0.5);
			trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			trace.LayerMask = TRACE_LAYER_CAMERA;
			trace.Exclude = owner;
			trace.Start = pivot - worldCamera[0] * pOutResult.m_fShoulderDist * scale;
			trace.End = pivot;
			float side = ChimeraCharacter.TraceMoveWithoutCharacters(owner.GetWorld(), trace);
			pivot = vector.Lerp(trace.Start, trace.End, Math.Max(0, side - 0.01));
			desired = pivot - boom * pOutResult.m_fDistance * scale;
			trace.Start = pivot;
			trace.End = desired;
			float back = ChimeraCharacter.TraceMoveWithoutCharacters(owner.GetWorld(), trace);
			desired = vector.Lerp(pivot, desired, Math.Max(0, back - 0.01));
		}
		// Inverse rigid transforms require unit axes; apply size only to translation.
		Math3D.MatrixCopy(localCamera, resCamTM);
		resCamTM[3] = desired.InvMultiply4(rotationTM) / scale;
	}
}
