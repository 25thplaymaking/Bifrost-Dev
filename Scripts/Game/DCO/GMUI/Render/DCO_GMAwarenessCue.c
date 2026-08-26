// DCO GM tactical-intelligence overlays.
class DCO_GMAwarenessCue
{
	static const float CONE_RANGE = 45.0;
	static const int CONE_RAYS = 3;
	static const float CONE_HALF_FOV = 45.0;
	static const float EYE_H = 1.6;
	static const float CULL_CUES = 450.0;
	static const float CULL_MARKERS = 1600.0;
	static const int CAP = 140;
	static const int MARKER_POOL = 96;
	static const int PATH_LEG_CAP = 24;
	static const float MOVE_MIN = 0.06;
	static const float MARKER_HEAD_H = 2.15;
	static const float MARKER_SIZE = 32.0;
	static const float MARKER_GAP = 30.0;

	static const int CONE_PLAYER = 0xCC33CCFF;
	static const int CONE_AI = 0x99FFD15A;
	static const int CONE_BLOCKED = 0xE6FF9F43;
	static const int TARGET_VISIBLE = 0xFFFF4E42;
	static const int TARGET_PARTIAL = 0xFFFFC247;
	static const int TARGET_MEMORY = 0xE6FF8B3D;
	static const int MOVE_COLOR = 0xE63DEB72;
	static const int DEST_COLOR = 0xFF7CFF9D;

	protected DCO_GMRenderManager m_Render;
	protected SCR_EditableEntityCore m_Core;
	protected Widget m_wMarkerLayer;
	protected bool m_bActive;

	protected ref array<SCR_EditableEntityComponent> m_Chars = {};
	protected ref array<vector> m_PrevPos = {};
	protected ref array<ImageWidget> m_MarkerWidgets = {};
	protected ref array<ResourceName> m_MarkerTextures = {};
	protected ref array<vector> m_UsedMarkerPos = {};
	protected ref set<SCR_EditableEntityComponent> m_SelectedUnits = new set<SCR_EditableEntityComponent>();
	protected int m_RefreshCounter;

	void Start(DCO_GMRenderManager render, Widget shellRoot)
	{
		m_Render = render;
		m_Core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (shellRoot)
			m_wMarkerLayer = shellRoot.FindAnyWidget("DCO_AIRoleMarkerLayer");
		BuildMarkerPool();
		if (m_Render)
			m_Render.GetOnRender().Insert(OnRender);
		GetGame().GetCallqueue().CallLater(UpdateMarkers, 100, true);
		m_bActive = true;
		Print("[DCO-GM] tactical overlays STARTED (perception + nav paths + role markers)", LogLevel.NORMAL);
	}

	void Stop()
	{
		if (m_Render)
			m_Render.GetOnRender().Remove(OnRender);
		GetGame().GetCallqueue().Remove(UpdateMarkers);
		HideMarkerPool();
		foreach (ImageWidget marker : m_MarkerWidgets)
		{
			if (marker)
				marker.RemoveFromHierarchy();
		}
		m_MarkerWidgets.Clear();
		m_MarkerTextures.Clear();
		m_UsedMarkerPos.Clear();
		m_SelectedUnits.Clear();
		m_Chars.Clear();
		m_PrevPos.Clear();
		m_wMarkerLayer = null;
		m_Render = null;
		m_bActive = false;
	}

	protected void BuildMarkerPool()
	{
		if (!m_wMarkerLayer)
		{
			Print("[DCO-GM] role marker layer missing; world cues remain available", LogLevel.WARNING);
			return;
		}
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;
		int flags = WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS | WidgetFlags.STRETCH;
		for (int i = 0; i < MARKER_POOL; i++)
		{
			ImageWidget marker = ImageWidget.Cast(workspace.CreateWidget(WidgetType.ImageWidgetTypeID, flags, Color.White, 0, m_wMarkerLayer));
			if (!marker)
				break;
			FrameSlot.SetAlignment(marker, 0.5, 1.0);
			FrameSlot.SetSize(marker, MARKER_SIZE, MARKER_SIZE);
			marker.SetVisible(false);
			m_MarkerWidgets.Insert(marker);
			m_MarkerTextures.Insert(ResourceName.Empty);
		}
	}

	protected void OnRender(DCO_GMRenderManager render)
	{
		if (!m_bActive || !render)
			return;
		DCO_GMOverlayState state = DCO_GMOverlayState.Get();
		if (!state.m_bViewCones && !state.m_bMovement)
			return;

		RefreshIfNeeded();
		RefreshSelectedUnits();
		BaseWorld world = GetGame().GetWorld();
		vector cameraPos;
		bool haveCamera = GetCameraPos(cameraPos);
		for (int i = 0; i < m_Chars.Count(); i++)
		{
			SCR_EditableEntityComponent editable = m_Chars[i];
			if (!editable)
				continue;
			IEntity owner = editable.GetOwner();
			if (!owner)
				continue;

			vector matrix[4];
			owner.GetWorldTransform(matrix);
			vector position = matrix[3];
			if (haveCamera && vector.DistanceSq(position, cameraPos) > CULL_CUES * CULL_CUES)
				continue;

			if (state.m_bMovement && IsInScope(editable, state.GetScope(DCO_GMOverlayState.OV_MOVEMENT)))
				DrawMovement(render, owner, position, m_PrevPos[i]);
			m_PrevPos[i] = position;

			if (state.m_bViewCones && IsInScope(editable, state.GetScope(DCO_GMOverlayState.OV_CONES)))
			{
				vector eye = position + Vector(0, EYE_H, 0);
				DrawViewCone(render, world, owner, eye, matrix[2], editable.GetPlayerID() > 0);
				DrawPerceivedTarget(render, owner, eye);
			}
		}
	}

	// Selection semantics match the GM: selecting a character affects that character; selecting a group affects its member characters.
	protected void RefreshSelectedUnits()
	{
		m_SelectedUnits.Clear();
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
		foreach (SCR_EditableEntityComponent editable : selected)
		{
			if (!editable)
				continue;
			if (editable.GetEntityType() == EEditableEntityType.CHARACTER)
			{
				m_SelectedUnits.Insert(editable);
				continue;
			}
			if (editable.GetEntityType() != EEditableEntityType.GROUP)
				continue;
			set<SCR_EditableEntityComponent> members = new set<SCR_EditableEntityComponent>();
			editable.GetChildren(members, true);
			foreach (SCR_EditableEntityComponent member : members)
			{
				if (member && member.GetEntityType() == EEditableEntityType.CHARACTER)
					m_SelectedUnits.Insert(member);
			}
		}
	}

	protected bool IsInScope(SCR_EditableEntityComponent editable, int scope)
	{
		if (scope == EDCO_OverlayScope.ALL)
			return true;
		return editable && m_SelectedUnits.Contains(editable);
	}

	protected void RefreshIfNeeded()
	{
		m_RefreshCounter++;
		if (m_RefreshCounter < 10 && !m_Chars.IsEmpty())
			return;
		RefreshList();
		m_RefreshCounter = 0;
	}

	protected bool GetCameraPos(out vector position)
	{
		SCR_ManualCamera camera = SCR_CameraEditorComponent.GetCameraInstance();
		if (!camera)
			return false;
		position = camera.GetOrigin();
		return true;
	}

	protected void RefreshList()
	{
		m_Chars.Clear();
		m_PrevPos.Clear();
		if (!m_Core)
			m_Core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (!m_Core)
			return;

		vector cameraPos;
		bool haveCamera = GetCameraPos(cameraPos);
		set<SCR_EditableEntityComponent> all = new set<SCR_EditableEntityComponent>();
		m_Core.GetAllEntities(all);
		foreach (SCR_EditableEntityComponent editable : all)
		{
			if (!editable || editable.GetEntityType() != EEditableEntityType.CHARACTER)
				continue;
			IEntity owner = editable.GetOwner();
			if (!owner)
				continue;
			vector position = owner.GetOrigin();
			if (haveCamera && vector.DistanceSq(position, cameraPos) > CULL_MARKERS * CULL_MARKERS)
				continue;
			m_Chars.Insert(editable);
			m_PrevPos.Insert(position);
			if (m_Chars.Count() >= CAP)
				break;
		}
	}

	protected AIBaseMovementComponent GetMovement(IEntity owner)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(owner);
		if (!character)
			return null;
		AIControlComponent control = character.GetAIControlComponent();
		if (!control)
			return null;
		AIAgent agent = control.GetAIAgent();
		if (!agent)
			return null;
		return AIBaseMovementComponent.Cast(agent.FindComponent(AIBaseMovementComponent));
	}

	protected SCR_AICombatComponent GetCombat(IEntity owner)
	{
		SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		if (combat)
			return combat;
		ChimeraCharacter character = ChimeraCharacter.Cast(owner);
		if (!character)
			return null;
		AIControlComponent control = character.GetAIControlComponent();
		if (!control)
			return null;
		AIAgent agent = control.GetAIAgent();
		if (!agent)
			return null;
		return SCR_AICombatComponent.Cast(agent.FindComponent(SCR_AICombatComponent));
	}

	protected void DrawMovement(DCO_GMRenderManager render, IEntity owner, vector current, vector previous)
	{
		AIBaseMovementComponent movement = GetMovement(owner);
		array<vector> path = {};
		if (movement)
			movement.GetCurrentPath(path);

		if (path.Count() >= 2)
		{
			int last = Math.Min(path.Count() - 1, PATH_LEG_CAP);
			for (int i = 0; i < last; i++)
				render.DrawLine(path[i] + Vector(0, 0.12, 0), path[i + 1] + Vector(0, 0.12, 0), MOVE_COLOR);
			vector destination = path[last] + Vector(0, 0.14, 0);
			render.DrawArrow(path[last - 1] + Vector(0, 0.12, 0), destination, 0.24, DEST_COLOR);
			render.DrawRing(destination, Vector(1, 0, 0), Vector(0, 0, 1), 0.7, DEST_COLOR);
			render.DrawStick(path[last], 0.8, DEST_COLOR);
			return;
		}

		vector delta = current - previous;
		delta[1] = 0;
		float step = delta.Length();
		if (step < MOVE_MIN)
			return;
		vector direction = delta / step;
		float speed = step * 10.0;
		float length = Math.Clamp(speed * 1.2, 1.5, 8.0);
		vector chest = current + Vector(0, 1.0, 0);
		render.DrawArrow(chest, chest + direction * length, 0.25, MOVE_COLOR);
	}

	protected void DrawViewCone(DCO_GMRenderManager render, BaseWorld world, IEntity self, vector eye, vector forward, bool isPlayer)
	{
		forward[1] = 0;
		if (forward.Length() < 0.001)
			return;
		forward.Normalize();
		vector baseAngles = forward.VectorToAngles();
		int clearColor = CONE_AI;
		if (isPlayer)
			clearColor = CONE_PLAYER;

		for (int k = 0; k < CONE_RAYS; k++)
		{
			float fractionAcross = (k / (CONE_RAYS - 1.0)) * 2.0 - 1.0;
			vector angles = baseAngles;
			angles[0] = angles[0] + fractionAcross * CONE_HALF_FOV;
			vector direction = angles.AnglesToVector();
			vector end = eye + direction * CONE_RANGE;
			float traceFraction = 1.0;
			if (world)
			{
				TraceParam trace = new TraceParam();
				trace.Start = eye;
				trace.End = end;
				trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
				trace.Exclude = self;
				traceFraction = world.TraceMove(trace, null);
				end = eye + direction * (traceFraction * CONE_RANGE);
			}
			render.DrawLine(eye, end, clearColor);
			if (traceFraction < 0.995)
				render.DrawSphere(end, 0.18, CONE_BLOCKED);
		}
	}

	protected void DrawPerceivedTarget(DCO_GMRenderManager render, IEntity observer, vector eye)
	{
		SCR_AICombatComponent combat = GetCombat(observer);
		if (!combat)
			return;
		BaseTarget target = combat.GetCurrentTarget();
		if (!target)
			target = combat.GetLastSeenEnemy();
		if (!target)
			return;

		float seenAge = target.GetTimeSinceSeen();
		bool visible = seenAge <= SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_VISIBLE;
		IEntity targetEntity = target.GetTargetEntity();
		vector targetPosition = target.GetLastSeenPosition();
		if (visible && targetEntity)
			targetPosition = targetEntity.GetOrigin();

		if (visible)
		{
			float exposure = target.GetExposure();
			float traceFraction = target.GetTraceFraction();
			int color = TARGET_VISIBLE;
			if (exposure < 0.65 || traceFraction < 0.75)
				color = TARGET_PARTIAL;
			render.DrawLine(eye, targetPosition + Vector(0, 1.0, 0), color);
			if (targetEntity)
				DrawTargetBox(render, targetEntity, color);
			else
				render.DrawRing(targetPosition, Vector(1, 0, 0), Vector(0, 0, 1), 0.9, color);
			return;
		}

		if (seenAge > 11.0)
			return;
		vector memory = target.GetLastSeenPosition();
		render.DrawRing(memory + Vector(0, 0.08, 0), Vector(1, 0, 0), Vector(0, 0, 1), 1.0, TARGET_MEMORY);
		render.DrawLine(memory + Vector(-0.65, 0.1, 0), memory + Vector(0.65, 0.1, 0), TARGET_MEMORY);
		render.DrawLine(memory + Vector(0, 0.1, -0.65), memory + Vector(0, 0.1, 0.65), TARGET_MEMORY);
		render.DrawLine(eye, memory + Vector(0, 0.2, 0), TARGET_MEMORY);
	}

	protected void DrawTargetBox(DCO_GMRenderManager render, IEntity target, int color)
	{
		vector minimum;
		vector maximum;
		target.GetBounds(minimum, maximum);
		vector origin = target.GetOrigin();
		minimum = origin + minimum;
		maximum = origin + maximum;
		if (maximum[1] - minimum[1] < 0.2)
		{
			minimum = origin + Vector(-0.4, 0, -0.4);
			maximum = origin + Vector(0.4, 1.9, 0.4);
		}
		render.DrawBox(minimum, maximum, color);
	}

	protected void UpdateMarkers()
	{
		if (!m_bActive)
			return;
		DCO_GMOverlayState state = DCO_GMOverlayState.Get();
		if (!state.m_bMarkers || DCO_GMTheme.Get().IsMasterHidden())
		{
			HideMarkerPool();
			return;
		}
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		if (!workspace || !world || m_MarkerWidgets.IsEmpty())
		{
			HideMarkerPool();
			return;
		}

		RefreshIfNeeded();
		RefreshSelectedUnits();
		m_UsedMarkerPos.Clear();
		int usedCount = 0;
		for (int i = 0; i < m_Chars.Count() && usedCount < m_MarkerWidgets.Count(); i++)
		{
			SCR_EditableEntityComponent editable = m_Chars[i];
			if (!editable)
				continue;
			if (!IsInScope(editable, state.GetScope(DCO_GMOverlayState.OV_MARKERS)))
				continue;
			IEntity owner = editable.GetOwner();
			if (!owner)
				continue;
			vector screen = workspace.ProjWorldToScreen(owner.GetOrigin() + Vector(0, MARKER_HEAD_H, 0), world);
			if (screen[2] < 0)
				continue;
			vector placed;
			if (!FindMarkerPosition(screen, placed))
				continue;

			ResourceName texture = DCO_App6Icons.ForEntity(editable);
			if (texture.IsEmpty())
				continue;
			ImageWidget marker = m_MarkerWidgets[usedCount];
			if (m_MarkerTextures[usedCount] != texture)
			{
				marker.LoadImageTexture(0, texture);
				m_MarkerTextures[usedCount] = texture;
			}
			FrameSlot.SetPos(marker, placed[0], placed[1]);
			marker.SetOpacity(0.96);
			marker.SetVisible(true);
			m_UsedMarkerPos.Insert(placed);
			usedCount++;
		}
		for (int j = usedCount; j < m_MarkerWidgets.Count(); j++)
			m_MarkerWidgets[j].SetVisible(false);
	}

	protected bool FindMarkerPosition(vector requested, out vector placed)
	{
		vector candidates[9];
		candidates[0] = requested;
		candidates[1] = requested + Vector(0, -MARKER_GAP, 0);
		candidates[2] = requested + Vector(MARKER_GAP, 0, 0);
		candidates[3] = requested + Vector(-MARKER_GAP, 0, 0);
		candidates[4] = requested + Vector(MARKER_GAP, -MARKER_GAP, 0);
		candidates[5] = requested + Vector(-MARKER_GAP, -MARKER_GAP, 0);
		candidates[6] = requested + Vector(0, -MARKER_GAP * 2.0, 0);
		candidates[7] = requested + Vector(MARKER_GAP * 2.0, 0, 0);
		candidates[8] = requested + Vector(-MARKER_GAP * 2.0, 0, 0);

		for (int i = 0; i < 9; i++)
		{
			bool clear = true;
			foreach (vector used : m_UsedMarkerPos)
			{
				float dx = candidates[i][0] - used[0];
				float dy = candidates[i][1] - used[1];
				if (dx * dx + dy * dy < MARKER_GAP * MARKER_GAP)
				{
					clear = false;
					break;
				}
			}
			if (clear)
			{
				placed = candidates[i];
				return true;
			}
		}
		return false;
	}

	protected void HideMarkerPool()
	{
		foreach (ImageWidget marker : m_MarkerWidgets)
		{
			if (marker)
				marker.SetVisible(false);
		}
	}
}
