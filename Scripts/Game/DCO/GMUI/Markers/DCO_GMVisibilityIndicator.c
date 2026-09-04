// Server-checked player line-of-sight indicator for GM cursor and placement positions.

class DCO_GMVisibilityServer
{
	static bool Evaluate(int requestingPlayerId, vector point, float maxDistance, out int viewerCount, out float nearestDistance)
	{
		viewerCount = 0;
		nearestDistance = -1;
		if (!Replication.IsServer())
			return false;
		PlayerManager players = GetGame().GetPlayerManager();
		BaseWorld world = GetGame().GetWorld();
		if (!players || !world)
			return false;
		maxDistance = Math.Clamp(maxDistance, 25.0, 2000.0);
		array<int> playerIds = {};
		players.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (playerId == requestingPlayerId)
				continue;
			IEntity entity = players.GetPlayerControlledEntity(playerId);
			if (!entity || entity.IsDeleted())
				continue;
			DamageManagerComponent damage = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
			if (damage && damage.IsDestroyed())
				continue;

			vector eye = entity.GetOrigin() + Vector(0, 1.55, 0);
			vector target = point + Vector(0, 0.35, 0);
			vector toTarget = target - eye;
			float distance = toTarget.Length();
			if (distance < 0.1 || distance > maxDistance)
				continue;
			toTarget = toTarget / distance;

			vector forward = entity.GetTransformAxis(2);
			CharacterHeadAimingComponent headAiming = CharacterHeadAimingComponent.Cast(entity.FindComponent(CharacterHeadAimingComponent));
			if (headAiming)
				forward = headAiming.GetAimingDirectionWorld();
			if (forward.LengthSq() < 0.001)
				continue;
			forward.Normalize();
			if (vector.Dot(forward, toTarget) < 0.15)
				continue;

			TraceParam trace = new TraceParam();
			trace.Start = eye;
			trace.End = target;
			trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			trace.Exclude = entity;
			float fraction = world.TraceMove(trace, null);
			if (fraction < 0.985)
				continue;

			viewerCount++;
			if (nearestDistance < 0 || distance < nearestDistance)
				nearestDistance = distance;
		}
		return viewerCount > 0;
	}
}

class DCO_GMVisibilityIndicator
{
	protected static ref DCO_GMVisibilityIndicator s_Instance;
	protected static const int POLL_MS = 250;
	protected static const int COLOR_CLEAR = 0xE02ECC71;
	protected static const int COLOR_SEEN = 0xE0E05252;
	protected static const int COLOR_PENDING = 0xE0D9892B;

	protected DCO_GMRenderManager m_Render;
	protected Widget m_wLabelLayer;
	protected TextWidget m_wLabel;
	protected SCR_PreviewEntityEditorComponent m_Preview;
	protected bool m_bActive;
	protected bool m_bEnabled = true;
	protected bool m_bPlacementOnly = true;
	protected float m_fMaxDistance = 500;
	protected bool m_bHasPoint;
	protected bool m_bPending;
	protected bool m_bSeen;
	protected int m_iViewers;
	protected float m_fNearestDistance = -1;
	protected int m_iSequence;
	protected int m_iAppliedSequence;
	protected vector m_vPoint;
	protected vector m_vRequestedPoint;

	static DCO_GMVisibilityIndicator Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMVisibilityIndicator();
		return s_Instance;
	}

	void Start(DCO_GMRenderManager render, Widget shellRoot)
	{
		Stop();
		m_Render = render;
		m_wLabelLayer = shellRoot.FindAnyWidget("DCO_AIRoleMarkerLayer");
		BuildLabel();
		if (m_Render)
			m_Render.GetOnRender().Insert(OnRender);
		m_bActive = true;
		GetGame().GetCallqueue().CallLater(Poll, POLL_MS, true);
	}

	void Stop()
	{
		m_bActive = false;
		GetGame().GetCallqueue().Remove(Poll);
		if (m_Render)
			m_Render.GetOnRender().Remove(OnRender);
		if (m_wLabel)
		{
			m_wLabel.RemoveFromHierarchy();
			m_wLabel = null;
		}
		m_wLabelLayer = null;
		m_Render = null;
		m_Preview = null;
		m_bHasPoint = false;
	}

	protected void BuildLabel()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace || !m_wLabelLayer)
			return;
		int flags = WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS | WidgetFlags.STRETCH | WidgetFlags.NO_LOCALIZATION;
		m_wLabel = TextWidget.Cast(workspace.CreateWidget(WidgetType.TextWidgetTypeID, flags, Color.White, 0, m_wLabelLayer));
		if (!m_wLabel)
			return;
		FrameSlot.SetAlignment(m_wLabel, 0.5, 1.0);
		FrameSlot.SetSize(m_wLabel, 300, 30);
		m_wLabel.SetExactFontSize(18);
		m_wLabel.SetVisible(false);
	}

	bool IsEnabled()
	{
		return m_bEnabled;
	}

	bool IsPlacementOnly()
	{
		return m_bPlacementOnly;
	}

	float GetMaxDistance()
	{
		return m_fMaxDistance;
	}

	void ToggleEnabled()
	{
		m_bEnabled = !m_bEnabled;
		if (!m_bEnabled)
		{
			m_bHasPoint = false;
			if (m_wLabel)
				m_wLabel.SetVisible(false);
		}
	}

	void TogglePlacementOnly()
	{
		m_bPlacementOnly = !m_bPlacementOnly;
	}

	void CycleDistance()
	{
		if (m_fMaxDistance < 250)
			m_fMaxDistance = 250;
		else if (m_fMaxDistance < 500)
			m_fMaxDistance = 500;
		else if (m_fMaxDistance < 1000)
			m_fMaxDistance = 1000;
		else if (m_fMaxDistance < 2000)
			m_fMaxDistance = 2000;
		else
			m_fMaxDistance = 100;
	}

	protected bool IsPlacementActive()
	{
		if (!m_Preview)
			m_Preview = SCR_PreviewEntityEditorComponent.Cast(SCR_PreviewEntityEditorComponent.GetInstance(SCR_PreviewEntityEditorComponent, false, true));
		return m_Preview && m_Preview.IsEditing() && m_Preview.IsChange();
	}

	protected void Poll()
	{
		if (!UpdateCursorPoint())
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
		{
			m_vRequestedPoint = m_vPoint;
			controller.DCO_RequestGMVisibilityCheck(m_vRequestedPoint, m_fMaxDistance, ++m_iSequence);
		}
	}

	protected bool UpdateCursorPoint()
	{
		if (!m_bActive || !m_bEnabled || DCO_GMTheme.Get().IsMasterHidden())
		{
			m_bHasPoint = false;
			return false;
		}
		if (m_bPlacementOnly && !IsPlacementActive())
		{
			m_bHasPoint = false;
			return false;
		}
		SCR_MenuLayoutEditorComponent menuLayout = SCR_MenuLayoutEditorComponent.Cast(
			SCR_MenuLayoutEditorComponent.GetInstance(SCR_MenuLayoutEditorComponent, false));
		vector point;
		if (!menuLayout || !menuLayout.GetCursorWorldPos(point) || !SCR_Global.IsPositionWithinTerrainBounds(point))
		{
			m_bHasPoint = false;
			return false;
		}

		if (!m_bHasPoint || vector.DistanceSq(m_vPoint, point) > 0.0001)
			m_bPending = true;
		m_vPoint = point;
		m_bHasPoint = true;
		return true;
	}

	void OnResult(int sequence, bool seen, int viewerCount, float nearestDistance)
	{
		if (sequence < m_iAppliedSequence || sequence != m_iSequence)
			return;
		if (vector.DistanceSq(m_vPoint, m_vRequestedPoint) > 0.0001)
			return;
		m_iAppliedSequence = sequence;
		m_bPending = false;
		m_bSeen = seen;
		m_iViewers = viewerCount;
		m_fNearestDistance = nearestDistance;
	}

	protected void OnRender(DCO_GMRenderManager render)
	{
		UpdateCursorPoint();
		if (!m_bActive || !m_bEnabled || !m_bHasPoint || !render || DCO_GMTheme.Get().IsMasterHidden())
		{
			if (m_wLabel)
				m_wLabel.SetVisible(false);
			return;
		}
		int color = COLOR_PENDING;
		if (!m_bPending)
		{
			color = COLOR_CLEAR;
			if (m_bSeen)
				color = COLOR_SEEN;
		}
		vector point = m_vPoint + Vector(0, 0.05, 0);
		render.DrawRing(point, Vector(1, 0, 0), Vector(0, 0, 1), 0.85, color);
		render.DrawLine(point + Vector(-0.45, 0, 0), point + Vector(0.45, 0, 0), color, 3.0);
		render.DrawLine(point + Vector(0, 0, -0.45), point + Vector(0, 0, 0.45), color, 3.0);
		UpdateLabel(color);
	}

	protected void UpdateLabel(int color)
	{
		if (!m_wLabel)
			return;
		if (m_bPending)
		{
			m_wLabel.SetVisible(false);
			return;
		}
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		if (!workspace || !world)
		{
			m_wLabel.SetVisible(false);
			return;
		}
		vector screen = workspace.ProjWorldToScreen(m_vPoint + Vector(0, 0.2, 0), world);
		if (screen[2] < 0)
		{
			m_wLabel.SetVisible(false);
			return;
		}
		string text = "CONCEALED FROM LIVE PLAYERS";
		if (m_bSeen)
			text = string.Format("VISIBLE TO %1  ·  NEAREST %2 M", m_iViewers, Math.Round(m_fNearestDistance));
		m_wLabel.SetText(text);
		m_wLabel.SetColor(Color.FromInt(color));
		FrameSlot.SetPos(m_wLabel, screen[0], screen[1] - 18);
		m_wLabel.SetVisible(true);
	}
}
