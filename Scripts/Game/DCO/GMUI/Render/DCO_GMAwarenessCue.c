// DCO GM tactical-intelligence overlays.
class DCO_GMAIOverlaySample
{
	string m_sEntityId;
	bool m_bHasTarget;
	bool m_bTargetVisible;
	float m_fExposure;
	float m_fTraceFraction;
	vector m_vTargetPosition;
	string m_sTargetEntityId;
	ref array<vector> m_aPath = {};
}

class DCO_GMAIOverlayBatch
{
	int m_iSerial;
	ref array<ref DCO_GMAIOverlaySample> m_aSamples = {};
}

class DCO_GMAIOverlaySnapshotState
{
	ref map<string, ref DCO_GMAIOverlaySample> m_Samples = new map<string, ref DCO_GMAIOverlaySample>();
	ref array<SCR_EditableEntityComponent> m_ServerCharacters = {};
	int m_iSerial = -1;
}

class DCO_GMAIOverlaySnapshot
{
	static const int PATH_ALL = 1;
	static const int PATH_SELECTED = 2;
	static const int VISION_ALL = 4;
	static const int VISION_SELECTED = 8;
	static const int MAX_SAMPLES = 48;
	static const int CHUNK_SAMPLES = 16;
	static const int MAX_PATH_POINTS = 10;
	static const float RANGE = 450.0;
	static const int STALE_MS = 1500;
	static const int SERVER_LIST_REFRESH_MS = 1000;

	protected static ref DCO_GMAIOverlaySnapshotState s_State;
	protected static int s_iLastReceiveAt;
	protected static int s_iLastServerListRefreshAt;

	protected static DCO_GMAIOverlaySnapshotState State()
	{
		if (!s_State)
			s_State = new DCO_GMAIOverlaySnapshotState();
		return s_State;
	}

	static void BuildChunks(vector cameraPosition, int requestMask, string selectedIds, int serial, notnull array<string> chunks)
	{
		chunks.Clear();
		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		array<ref DCO_GMAIOverlaySample> pending = {};
		if (!core)
		{
			WriteChunk(serial, pending, chunks);
			return;
		}

		RefreshServerCharacters(core);
		int accepted;
		for (int pass = 0; pass < 2 && accepted < MAX_SAMPLES; pass++)
		{
			foreach (SCR_EditableEntityComponent editable : State().m_ServerCharacters)
			{
				if (!editable || editable.GetEntityType() != EEditableEntityType.CHARACTER)
					continue;
				IEntity owner = editable.GetOwner();
				if (!owner || vector.DistanceSq(owner.GetOrigin(), cameraPosition) > RANGE * RANGE)
					continue;
				RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
				if (!rpl || !rpl.Id().IsValid())
					continue;

				string entityId = rpl.Id().AsString();
				bool selected = selectedIds.Contains("|" + entityId + "|");
				if ((pass == 0) != selected)
					continue;
				bool wantsPath = (requestMask & PATH_ALL) != 0 || (selected && (requestMask & PATH_SELECTED) != 0);
				bool wantsVision = (requestMask & VISION_ALL) != 0 || (selected && (requestMask & VISION_SELECTED) != 0);
				if (!wantsPath && !wantsVision)
					continue;

				DCO_GMAIOverlaySample sample = new DCO_GMAIOverlaySample();
				sample.m_sEntityId = entityId;
				if (wantsPath)
					ReadPath(owner, sample.m_aPath);
				if (wantsVision)
					ReadTarget(owner, sample);
				if (sample.m_aPath.Count() < 2 && !sample.m_bHasTarget)
					continue;

				pending.Insert(sample);
				accepted++;
				if (pending.Count() >= CHUNK_SAMPLES)
				{
					WriteChunk(serial, pending, chunks);
					pending.Clear();
				}
				if (accepted >= MAX_SAMPLES)
					break;
			}
		}
		if (!pending.IsEmpty() || chunks.IsEmpty())
			WriteChunk(serial, pending, chunks);
	}

	protected static void RefreshServerCharacters(SCR_EditableEntityCore core)
	{
		int now = System.GetTickCount();
		if (s_iLastServerListRefreshAt > 0 && now - s_iLastServerListRefreshAt < SERVER_LIST_REFRESH_MS)
			return;
		s_iLastServerListRefreshAt = now;
		State().m_ServerCharacters.Clear();
		set<SCR_EditableEntityComponent> all = new set<SCR_EditableEntityComponent>();
		core.GetAllEntities(all);
		foreach (SCR_EditableEntityComponent editable : all)
		{
			if (editable && editable.GetEntityType() == EEditableEntityType.CHARACTER)
				State().m_ServerCharacters.Insert(editable);
		}
	}

	protected static void ReadPath(IEntity owner, notnull array<vector> output)
	{
		AIBaseMovementComponent movement = DCO_GMAwarenessCue.GetMovement(owner);
		if (!movement)
			return;
		array<vector> source = {};
		movement.GetCurrentPath(source);
		if (source.Count() < 2)
			return;
		int copyCount = Math.Min(source.Count(), MAX_PATH_POINTS);
		for (int i = 0; i < copyCount; i++)
			output.Insert(source[i]);
		if (source.Count() > MAX_PATH_POINTS)
			output[MAX_PATH_POINTS - 1] = source[source.Count() - 1];
	}

	protected static void ReadTarget(IEntity owner, DCO_GMAIOverlaySample sample)
	{
		SCR_AICombatComponent combat = DCO_GMAwarenessCue.GetCombat(owner);
		if (!combat)
			return;
		BaseTarget target = combat.GetCurrentTarget();
		if (!target)
			target = combat.GetLastSeenEnemy();
		if (!target || target.GetTimeSinceSeen() > 11.0)
			return;

		sample.m_bHasTarget = true;
		sample.m_bTargetVisible = target.GetTimeSinceSeen() <= SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_VISIBLE;
		sample.m_fExposure = target.GetExposure();
		sample.m_fTraceFraction = target.GetTraceFraction();
		sample.m_vTargetPosition = target.GetLastSeenPosition();
		IEntity targetEntity = target.GetTargetEntity();
		if (sample.m_bTargetVisible && targetEntity)
		{
			sample.m_vTargetPosition = targetEntity.GetOrigin();
			RplComponent targetRpl = RplComponent.Cast(targetEntity.FindComponent(RplComponent));
			if (targetRpl && targetRpl.Id().IsValid())
				sample.m_sTargetEntityId = targetRpl.Id().AsString();
		}
	}

	protected static void WriteChunk(int serial, notnull array<ref DCO_GMAIOverlaySample> samples, notnull array<string> chunks)
	{
		DCO_GMAIOverlayBatch batch = new DCO_GMAIOverlayBatch();
		batch.m_iSerial = serial;
		foreach (DCO_GMAIOverlaySample sample : samples)
			batch.m_aSamples.Insert(sample);
		JsonSaveContext save = new JsonSaveContext();
		if (!save.WriteValue("", batch))
			return;
		chunks.Insert(save.SaveToString());
	}

	static void Receive(string payload)
	{
		if (payload.IsEmpty())
			return;
		JsonLoadContext load = new JsonLoadContext();
		if (!load.LoadFromString(payload))
			return;
		DCO_GMAIOverlayBatch batch = new DCO_GMAIOverlayBatch();
		if (!load.ReadValue("", batch) || !batch || batch.m_iSerial < State().m_iSerial)
			return;
		if (batch.m_iSerial > State().m_iSerial)
		{
			State().m_Samples.Clear();
			State().m_iSerial = batch.m_iSerial;
		}
		foreach (DCO_GMAIOverlaySample sample : batch.m_aSamples)
		{
			if (sample && !sample.m_sEntityId.IsEmpty())
				State().m_Samples.Set(sample.m_sEntityId, sample);
		}
		s_iLastReceiveAt = System.GetTickCount();
	}

	static DCO_GMAIOverlaySample Find(IEntity entity)
	{
		if (!entity || System.GetTickCount() - s_iLastReceiveAt > STALE_MS)
			return null;
		RplComponent rpl = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (!rpl || !rpl.Id().IsValid())
			return null;
		DCO_GMAIOverlaySample sample;
		State().m_Samples.Find(rpl.Id().AsString(), sample);
		return sample;
	}

	static void Clear()
	{
		State().m_Samples.Clear();
		State().m_iSerial = -1;
		s_iLastReceiveAt = 0;
		State().m_ServerCharacters.Clear();
		s_iLastServerListRefreshAt = 0;
	}
}

class DCO_GMConeTraceCache
{
	ref array<float> m_Fractions = {1.0, 1.0, 1.0};
	int m_iLastTraceAt;
}

class DCO_GMAwarenessCue
{
	static const float CONE_RANGE = 45.0;
	static const int CONE_RAYS = 3;
	static const float CONE_HALF_FOV = 45.0;
	static const float EYE_H = 1.6;
	static const float CULL_CUES = 450.0;
	static const float CULL_MARKERS = 1600.0;
	static const int CAP = 140;
	static const int MARKER_POOL = 64;
	static const int PATH_LEG_CAP = 24;
	static const float MOVE_MIN = 0.06;
	static const float MARKER_HEAD_H = 2.15;
	static const float MARKER_SIZE = 32.0;
	static const float MARKER_GAP = 30.0;
	static const int SNAPSHOT_REQUEST_MS = 250;
	static const int SELECTION_REFRESH_MS = 100;
	static const int LIST_REFRESH_MS = 1000;
	static const int CONE_TRACE_MS = 150;
	static const int MARKER_CELL_STRIDE = 8192;
	static const float ROUTE_LIFT = 0.12;

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
	protected ref array<ResourceName> m_CharIcons = {};
	protected ref array<ref DCO_GMConeTraceCache> m_ConeTraces = {};
	protected ref map<string, IEntity> m_EntityByRplId = new map<string, IEntity>();
	protected ref array<ImageWidget> m_MarkerWidgets = {};
	protected ref array<ResourceName> m_MarkerTextures = {};
	protected ref set<int> m_UsedMarkerCells = new set<int>();
	protected ref set<SCR_EditableEntityComponent> m_SelectedUnits = new set<SCR_EditableEntityComponent>();
	protected int m_iLastRefreshAt;
	protected int m_iLastSelectionRefreshAt;
	protected int m_iLastSnapshotRequestAt;
	protected int m_iLastVisibleMarkers = -1;
	protected int m_iLastRenderAt;

	void Start(DCO_GMRenderManager render, Widget shellRoot)
	{
		m_Render = render;
		m_Core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (shellRoot)
			m_wMarkerLayer = shellRoot.FindAnyWidget("DCO_AIRoleMarkerLayer");
		BuildMarkerPool();
		if (m_Render)
			m_Render.GetOnRender().Insert(OnRender);
		m_bActive = true;
	}

	void Stop()
	{
		if (m_Render)
			m_Render.GetOnRender().Remove(OnRender);
		HideMarkerPool();
		foreach (ImageWidget marker : m_MarkerWidgets)
		{
			if (marker)
				marker.RemoveFromHierarchy();
		}
		m_MarkerWidgets.Clear();
		m_MarkerTextures.Clear();
		m_UsedMarkerCells.Clear();
		m_SelectedUnits.Clear();
		m_Chars.Clear();
		m_PrevPos.Clear();
		m_CharIcons.Clear();
		m_ConeTraces.Clear();
		m_EntityByRplId.Clear();
		DCO_GMAIOverlaySnapshot.Clear();
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
		RefreshIfNeeded();
		RefreshSelectedUnitsIfNeeded();
		UpdateMarkers();
		int now = System.GetTickCount();
		float frameSeconds = Math.Clamp((now - m_iLastRenderAt) * 0.001, 0.01, 0.2);
		m_iLastRenderAt = now;
		DCO_GMOverlayState state = DCO_GMOverlayState.Get();
		if (!state.m_bViewCones && !state.m_bMovement)
			return;

		BaseWorld world = GetGame().GetWorld();
		vector cameraPos;
		bool haveCamera = GetCameraPos(cameraPos);
		RequestAuthoritySnapshot(state, cameraPos, haveCamera);
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
				DrawMovement(render, world, owner, position, m_PrevPos[i], frameSeconds);
			m_PrevPos[i] = position;

			if (state.m_bViewCones && IsInScope(editable, state.GetScope(DCO_GMOverlayState.OV_CONES)))
			{
				vector eye = position + Vector(0, EYE_H, 0);
				DCO_GMConeTraceCache coneTrace;
				if (i < m_ConeTraces.Count())
					coneTrace = m_ConeTraces[i];
				DrawViewCone(render, world, owner, eye, matrix[2], editable.GetPlayerID() > 0, coneTrace, now);
				DrawPerceivedTarget(render, owner, eye);
			}
		}
	}

	protected void RequestAuthoritySnapshot(DCO_GMOverlayState state, vector cameraPosition, bool haveCamera)
	{
		if (!haveCamera || Replication.IsServer())
			return;
		int now = System.GetTickCount();
		if (now - m_iLastSnapshotRequestAt < SNAPSHOT_REQUEST_MS)
			return;
		int requestMask;
		if (state.m_bMovement)
		{
			if (state.GetScope(DCO_GMOverlayState.OV_MOVEMENT) == EDCO_OverlayScope.ALL)
				requestMask |= DCO_GMAIOverlaySnapshot.PATH_ALL;
			else
				requestMask |= DCO_GMAIOverlaySnapshot.PATH_SELECTED;
		}
		if (state.m_bViewCones)
		{
			if (state.GetScope(DCO_GMOverlayState.OV_CONES) == EDCO_OverlayScope.ALL)
				requestMask |= DCO_GMAIOverlaySnapshot.VISION_ALL;
			else
				requestMask |= DCO_GMAIOverlaySnapshot.VISION_SELECTED;
		}
		if (requestMask == 0)
			return;

		string selectedIds = "|";
		foreach (SCR_EditableEntityComponent editable : m_SelectedUnits)
		{
			if (!editable || !editable.GetOwner())
				continue;
			RplComponent rpl = RplComponent.Cast(editable.GetOwner().FindComponent(RplComponent));
			if (rpl && rpl.Id().IsValid())
				selectedIds += rpl.Id().AsString() + "|";
		}
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (pc)
		{
			pc.DCO_RequestGMAIOverlay(cameraPosition, requestMask, selectedIds);
			m_iLastSnapshotRequestAt = now;
		}
	}

	// Expands selected groups to their member characters.
	protected void RefreshSelectedUnitsIfNeeded()
	{
		int now = System.GetTickCount();
		if (now - m_iLastSelectionRefreshAt < SELECTION_REFRESH_MS)
			return;
		m_iLastSelectionRefreshAt = now;
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
		int now = System.GetTickCount();
		if (now - m_iLastRefreshAt < LIST_REFRESH_MS)
			return;
		RefreshList();
		m_iLastRefreshAt = now;
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
		m_CharIcons.Clear();
		m_ConeTraces.Clear();
		m_EntityByRplId.Clear();
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
			if (!editable)
				continue;
			bool isCharacter = editable.GetEntityType() == EEditableEntityType.CHARACTER;
			bool isVehicle = SCR_EditableVehicleComponent.Cast(editable) != null;
			if (!isCharacter && !isVehicle)
				continue;
			IEntity owner = editable.GetOwner();
			if (!owner)
				continue;
			vector position = owner.GetOrigin();
			if (haveCamera && vector.DistanceSq(position, cameraPos) > CULL_MARKERS * CULL_MARKERS)
				continue;
			RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
			if (rpl && rpl.Id().IsValid())
				m_EntityByRplId.Set(rpl.Id().AsString(), owner);
			if (!isCharacter)
				continue;
			m_Chars.Insert(editable);
			m_PrevPos.Insert(position);
			m_CharIcons.Insert(DCO_App6Icons.ForEntity(editable));
			DCO_GMConeTraceCache coneTrace = new DCO_GMConeTraceCache();
			coneTrace.m_iLastTraceAt = System.GetTickCount() - CONE_TRACE_MS + (m_Chars.Count() % 5) * 30;
			m_ConeTraces.Insert(coneTrace);
			if (m_Chars.Count() >= CAP)
				break;
		}
	}

	static AIBaseMovementComponent GetMovement(IEntity owner)
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
		return agent.GetMovementComponent();
	}

	static SCR_AICombatComponent GetCombat(IEntity owner)
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

	protected void DrawMovement(DCO_GMRenderManager render, BaseWorld world, IEntity owner, vector current, vector previous, float frameSeconds)
	{
		AIBaseMovementComponent movement = GetMovement(owner);
		array<vector> path = {};
		if (movement)
			movement.GetCurrentPath(path);
		if (path.Count() < 2)
		{
			DCO_GMAIOverlaySample sample = DCO_GMAIOverlaySnapshot.Find(owner);
			if (sample)
				path.Copy(sample.m_aPath);
		}

		if (path.Count() >= 2)
		{
			int last = Math.Min(path.Count() - 1, PATH_LEG_CAP);
			vector from = GroundRoutePoint(world, current);
			for (int i = 1; i <= last; i++)
			{
				vector to = GroundRoutePoint(world, path[i]);
				if (i == last)
					render.DrawArrow(from, to, 0.24, DEST_COLOR);
				else
					render.DrawLine(from, to, MOVE_COLOR, 3.0);
				from = to;
			}
			DrawRouteDestination(render, from);
			return;
		}

		vector delta = current - previous;
		delta[1] = 0;
		float step = delta.Length();
		if (step < MOVE_MIN)
			return;
		vector direction = delta / step;
		float speed = step / Math.Max(frameSeconds, 0.01);
		float length = Math.Clamp(speed * 1.2, 1.5, 8.0);
		vector ground = GroundRoutePoint(world, current);
		vector projected = GroundRoutePoint(world, current + direction * length);
		render.DrawArrow(ground, projected, 0.25, MOVE_COLOR);
	}

	protected vector GroundRoutePoint(BaseWorld world, vector position)
	{
		if (world)
			position[1] = world.GetSurfaceY(position[0], position[2]);
		position[1] = position[1] + ROUTE_LIFT;
		return position;
	}

	protected void DrawRouteDestination(DCO_GMRenderManager render, vector center)
	{
		float radius = 0.7;
		vector north = center + Vector(0, 0, radius);
		vector east = center + Vector(radius, 0, 0);
		vector south = center + Vector(0, 0, -radius);
		vector west = center + Vector(-radius, 0, 0);
		render.DrawLine(north, east, DEST_COLOR, 3.0);
		render.DrawLine(east, south, DEST_COLOR, 3.0);
		render.DrawLine(south, west, DEST_COLOR, 3.0);
		render.DrawLine(west, north, DEST_COLOR, 3.0);
	}

	protected void DrawViewCone(DCO_GMRenderManager render, BaseWorld world, IEntity self, vector eye, vector forward, bool isPlayer, DCO_GMConeTraceCache traceCache, int now)
	{
		forward[1] = 0;
		if (forward.Length() < 0.001)
			return;
		forward.Normalize();
		vector baseAngles = forward.VectorToAngles();
		int clearColor = CONE_AI;
		if (isPlayer)
			clearColor = CONE_PLAYER;
		bool refreshTrace = traceCache && now - traceCache.m_iLastTraceAt >= CONE_TRACE_MS;

		for (int k = 0; k < CONE_RAYS; k++)
		{
			float fractionAcross = (k / (CONE_RAYS - 1.0)) * 2.0 - 1.0;
			vector angles = baseAngles;
			angles[0] = angles[0] + fractionAcross * CONE_HALF_FOV;
			vector direction = angles.AnglesToVector();
			float traceFraction = 1.0;
			if (traceCache && k < traceCache.m_Fractions.Count())
				traceFraction = traceCache.m_Fractions[k];
			if (world && refreshTrace)
			{
				TraceParam trace = new TraceParam();
				trace.Start = eye;
				trace.End = eye + direction * CONE_RANGE;
				trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
				trace.Exclude = self;
				traceFraction = world.TraceMove(trace, null);
				traceCache.m_Fractions[k] = traceFraction;
			}
			vector end = eye + direction * (traceFraction * CONE_RANGE);
			render.DrawLine(eye, end, clearColor);
			if (traceFraction < 0.995)
				render.DrawSphere(end, 0.18, CONE_BLOCKED);
		}
		if (refreshTrace)
			traceCache.m_iLastTraceAt = now;
	}

	protected void DrawPerceivedTarget(DCO_GMRenderManager render, IEntity observer, vector eye)
	{
		SCR_AICombatComponent combat = GetCombat(observer);
		BaseTarget target;
		if (combat)
		{
			target = combat.GetCurrentTarget();
			if (!target)
				target = combat.GetLastSeenEnemy();
		}
		if (!target)
		{
			DrawReplicatedTarget(render, observer, eye);
			return;
		}

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

	protected void DrawReplicatedTarget(DCO_GMRenderManager render, IEntity observer, vector eye)
	{
		DCO_GMAIOverlaySample sample = DCO_GMAIOverlaySnapshot.Find(observer);
		if (!sample || !sample.m_bHasTarget)
			return;
		int color = TARGET_MEMORY;
		if (sample.m_bTargetVisible)
		{
			color = TARGET_VISIBLE;
			if (sample.m_fExposure < 0.65 || sample.m_fTraceFraction < 0.75)
				color = TARGET_PARTIAL;
		}
		vector targetPosition = sample.m_vTargetPosition;
		IEntity targetEntity;
		if (sample.m_bTargetVisible && !sample.m_sTargetEntityId.IsEmpty())
			m_EntityByRplId.Find(sample.m_sTargetEntityId, targetEntity);
		if (targetEntity)
		{
			targetPosition = targetEntity.GetOrigin();
			render.DrawLine(eye, targetPosition + Vector(0, 1.0, 0), color);
			DrawTargetBox(render, targetEntity, color);
			return;
		}
		render.DrawLine(eye, targetPosition + Vector(0, 0.2, 0), color);
		render.DrawRing(targetPosition + Vector(0, 0.08, 0), Vector(1, 0, 0), Vector(0, 0, 1), 0.9, color);
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
			if (m_iLastVisibleMarkers != 0)
			{
				HideMarkerPool();
				m_iLastVisibleMarkers = 0;
			}
			return;
		}
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		if (!workspace || !world || m_MarkerWidgets.IsEmpty())
		{
			if (m_iLastVisibleMarkers != 0)
			{
				HideMarkerPool();
				m_iLastVisibleMarkers = 0;
			}
			return;
		}

		m_UsedMarkerCells.Clear();
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
			if (screen[2] < 0 || screen[0] < -MARKER_SIZE || screen[1] < -MARKER_SIZE
				|| screen[0] > workspace.GetWidth() + MARKER_SIZE || screen[1] > workspace.GetHeight() + MARKER_SIZE)
				continue;
			vector placed;
			if (!FindMarkerPosition(screen, placed))
				continue;

			ResourceName texture;
			if (i < m_CharIcons.Count())
				texture = m_CharIcons[i];
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
			usedCount++;
		}
		for (int j = usedCount; j < m_MarkerWidgets.Count(); j++)
			m_MarkerWidgets[j].SetVisible(false);
		m_iLastVisibleMarkers = usedCount;
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
			int key = MarkerCellKey(candidates[i]);
			if (!m_UsedMarkerCells.Contains(key))
			{
				placed = candidates[i];
				m_UsedMarkerCells.Insert(key);
				return true;
			}
		}
		return false;
	}

	protected int MarkerCellKey(vector position)
	{
		int x = Math.Round(position[0] / MARKER_GAP);
		int y = Math.Round(position[1] / MARKER_GAP);
		return x + y * MARKER_CELL_STRIDE;
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
