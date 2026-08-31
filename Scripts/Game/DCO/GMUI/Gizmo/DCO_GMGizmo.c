class DCO_GMGizmo
{
	protected static ref DCO_GMGizmo s_Inst;
	static DCO_GMGizmo Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_GMGizmo();
		return s_Inst;
	}

	// Precise-placement mode.
	protected static bool s_bPreciseMode;
	static bool IsPreciseModeActive() { return s_bPreciseMode; }

	protected static ref ScriptInvoker s_OnPreciseChanged;
	static ScriptInvoker GetOnPreciseChanged()
	{
		if (!s_OnPreciseChanged)
			s_OnPreciseChanged = new ScriptInvoker();
		return s_OnPreciseChanged;
	}

	// Flip precise mode.
	static bool TogglePreciseMode()
	{
		SetPreciseMode(!s_bPreciseMode);
		return s_bPreciseMode;
	}

	static void SetPreciseMode(bool on)
	{
		s_bPreciseMode = on;
		if (s_Inst)
		{
			if (on)
			{
				s_Inst.PinLiveSelection();	// entering precise locks onto whatever is being manipulated right now.
			}
			else
			{
				s_Inst.AbortDrag();	// dropping out of precise mode ends any active handle drag cleanly.
				s_Inst.ReleasePin();
			}
		}
		GetOnPreciseChanged().Invoke(on);
	}

	// True while a precise pin is held.
	static bool DCO_IsPreciseLocked()
	{
		if (!s_bPreciseMode || !s_Inst)
			return false;
		return s_Inst.m_Pin != null;
	}

	// A direct pick re-pins the gizmo.
	static void NotifyDeliberatePick(SCR_EditableEntityComponent e)
	{
		if (!s_bPreciseMode || !s_Inst)
			return;
		s_Inst.SetPin(e);
	}

	protected DCO_GMRenderManager m_Render;
	protected ref DCO_GMGizmoRender m_Draw    = new DCO_GMGizmoRender();
	protected ref DCO_GMGizmoRotate m_DrawRot = new DCO_GMGizmoRotate();
	protected SCR_EditableEntityComponent m_Target;
	protected EDCO_GizmoMode  m_Mode  = EDCO_GizmoMode.MOVE;
	protected EDCO_GizmoSpace m_Space = EDCO_GizmoSpace.WORLD;
	protected int m_Hover = -1;

	// Holds the precise target until exit.
	protected SCR_EditableEntityComponent m_Pin;

	// Grid snap.
	protected bool  m_bSnap;
	protected float m_MoveStep = 0.25;

	// Surface snap.
	protected bool m_bSurfaceSnap;

	// The live numeric readout.
	protected DCO_GMGizmoPanel m_Panel;

	// Drag state.
	protected bool    m_bDragging;
	protected int     m_GrabHandle = -1;
	protected vector  m_GrabRo, m_GrabRd;	// cursor ray captured at grab.
	protected vector  m_StartOrigin;
	protected IEntity m_DragOwner;
	protected vector  m_LastDragPos;
	protected vector  m_LastDragAngles;
	protected bool    m_bHasDragTransform;
	protected int     m_iLastPreviewAt;

	// Holds the grab-time axis basis.
	protected vector m_GrabAxes[3];

	// ROTATE drag state.
	protected vector m_StartMat[4];	// target's world transform at grab.
	protected float  m_GrabAngle;
	protected bool   m_bGrabAngleOk;

	static const float SCREEN_K = 0.14;
	static const float VERT_MOTION_EPS = 0.3;
	static const int PREVIEW_INTERVAL_MS = 50;

	void Start(DCO_GMRenderManager render)
	{
		m_Render = render;
		if (m_Render)
			m_Render.GetOnRender().Insert(OnRender);

		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.AddActionListener("EditorTransform", EActionTrigger.DOWN, OnLmbDown);
			im.AddActionListener("EditorTransform", EActionTrigger.UP,   OnLmbUp);
		}
	}

	void Stop()
	{
		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.RemoveActionListener("EditorTransform", EActionTrigger.DOWN, OnLmbDown);
			im.RemoveActionListener("EditorTransform", EActionTrigger.UP,   OnLmbUp);
		}
		if (m_Render)
			m_Render.GetOnRender().Remove(OnRender);
		m_Render = null;
		m_Target = null;
		ReleasePin();
		m_Hover = -1;
		m_bDragging = false;
		m_GrabHandle = -1;
		m_DragOwner = null;
		m_bHasDragTransform = false;
		PanelHide();	// blank the readout before we let go of it, so a rebuilt shell never inherits stale numbers.
		m_Panel = null;
	}

	SCR_EditableEntityComponent GetTarget() { return m_Target; }
	bool IsDragging() { return m_bDragging; }
	void CancelInteraction() { AbortDrag(); }

	void SetPanel(DCO_GMGizmoPanel panel)
	{
		m_Panel = panel;
	}

	// Which handle set the gizmo shows and drives: MOVE arrows or ROTATE rings.
	void SetMode(EDCO_GizmoMode mode)
	{
		if (m_Mode == mode)
			return;
		m_Mode = mode;
		AbortDrag();
	}

	EDCO_GizmoMode GetMode() { return m_Mode; }

	// Reference frame for the handles.
	void SetSpace(EDCO_GizmoSpace space)
	{
		if (m_Space == space)
			return;
		m_Space = space;
		AbortDrag();
	}

	EDCO_GizmoSpace GetSpace() { return m_Space; }

	// Flip WORLD <-> LOCAL.
	bool ToggleSpace()
	{
		if (m_Space == EDCO_GizmoSpace.LOCAL)
			SetSpace(EDCO_GizmoSpace.WORLD);
		else
			SetSpace(EDCO_GizmoSpace.LOCAL);
		return m_Space == EDCO_GizmoSpace.LOCAL;
	}

	// Grid snap and surface snap.
	bool IsSnapOn() { return m_bSnap; }

	bool ToggleSnap()
	{
		m_bSnap = !m_bSnap;
		return m_bSnap;
	}

	float GetMoveStep() { return m_MoveStep; }

	void SetMoveStep(float step)
	{
		if (step > 0)
			m_MoveStep = step;
	}

	bool IsSurfaceSnapOn() { return m_bSurfaceSnap; }

	bool ToggleSurfaceSnap()
	{
		m_bSurfaceSnap = !m_bSurfaceSnap;
		return m_bSurfaceSnap;
	}

	protected float ActiveStep()
	{
		if (!m_bSnap)
			return 0;
		return m_MoveStep;
	}

	protected SCR_EditableEntityComponent SoleSelectedMovable()
	{
		set<SCR_EditableEntityComponent> sel = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(sel, EEditableEntityState.SELECTED);
		SCR_EditableEntityComponent only;
		int n = 0;
		foreach (SCR_EditableEntityComponent e : sel)
		{
			if (!e)
				continue;
			if (!IsGizmoMovable(e))
				continue;
			only = e;
			n++;
			if (n > 1)
				return null;	// multi-select -> no gizmo in v1.
		}
		if (n == 1)
			return only;
		return null;
	}

	// Sets m_Target for this tick.
	protected void ResolveTarget()
	{
		if (m_Pin && (!m_Pin.GetOwner() || m_Pin.IsDestroyed()))
			ReleasePin();

		if (m_Pin)
		{
			m_Target = m_Pin;
			// Keep what the GM SEES matching what the gizmo MOVES.
			SCR_EditableEntityComponent live = SoleSelectedMovable();
			if (live && live != m_Pin)
			{
				live.SetEntityState(EEditableEntityState.SELECTED, false);
				m_Pin.SetEntityState(EEditableEntityState.SELECTED, true);
			}
			return;
		}

		m_Target = SoleSelectedMovable();
		if (m_Target)
			SetPin(m_Target);	// first target of the session takes the lock; only deliberate surfaces move it after.
	}

	// Take the precise lock.
	protected void SetPin(SCR_EditableEntityComponent e)
	{
		if (!e || !IsGizmoMovable(e))
			return;
		if (m_Pin == e)
			return;
		m_Pin = e;
		m_Target = e;
	}

	// Drop the precise lock.
	protected void ReleasePin()
	{
		if (!m_Pin)
			return;
		m_Pin = null;
	}

	// Pin whatever the live selection is manipulating, used on precise-ENTER.
	protected void PinLiveSelection()
	{
		m_Target = null;
		SetPin(SoleSelectedMovable());
	}

	// True for editables the gizmo can physically move: characters, vehicles, and generic props/objects/items.
	protected bool IsGizmoMovable(SCR_EditableEntityComponent e)
	{
		if (!e || !e.GetOwner() || e.IsDestroyed())
			return false;
		EEditableEntityType t = e.GetEntityType();
		return t == EEditableEntityType.CHARACTER
			|| t == EEditableEntityType.VEHICLE
			|| t == EEditableEntityType.GENERIC
			|| t == EEditableEntityType.ITEM;
	}

	// Axis basis for the handles.
	protected void AxisBasis(IEntity owner, out vector axes[3])
	{
		axes[0] = Vector(1, 0, 0);
		axes[1] = Vector(0, 1, 0);
		axes[2] = Vector(0, 0, 1);

		if (m_Space != EDCO_GizmoSpace.LOCAL || !owner)
			return;

		vector m[4];
		owner.GetWorldTransform(m);
		vector x = m[0];
		vector y = m[1];
		vector z = m[2];
		if (x.Length() < 0.0001 || y.Length() < 0.0001 || z.Length() < 0.0001)
			return;	// degenerate basis -> keep world axes.
		x.Normalize();
		y.Normalize();
		z.Normalize();
		axes[0] = x;
		axes[1] = y;
		axes[2] = z;
	}

	// Keeps handle size stable across camera zoom.
	protected bool ScreenScaledLength(vector origin, out float len)
	{
		SCR_ManualCamera cam = SCR_CameraEditorComponent.GetCameraInstance();
		if (!cam)
		{
			len = 3.0;
			return true;
		}
		float d = vector.Distance(cam.GetOrigin(), origin);
		len = Math.Clamp(d * SCREEN_K, 0.6, 40.0);
		return true;
	}

	bool CursorRay(out vector ro, out vector rd)
	{
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return false;
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		vector dir;
		ro = ws.ProjScreenToWorld(ws.DPIUnscale(mx), ws.DPIUnscale(my), dir, GetGame().GetWorld());
		rd = dir;
		rd.Normalize();
		return true;
	}

	// LMB down: if the cursor is over a handle of the current target, begin a drag.
	void OnLmbDown(float value, EActionTrigger reason)
	{
		if (DCO_GMUIController.IsNativePropertiesOpen())
			return;
		if (!s_bPreciseMode)
			return;	// precise mode off -> gizmo is inert, engine drag owns the LMB.
		if (!m_Target)
			return;
		IEntity owner = m_Target.GetOwner();
		if (!owner)
			return;
		vector origin = owner.GetOrigin();
		float len;
		ScreenScaledLength(origin, len);
		vector axes[3];
		AxisBasis(owner, axes);
		vector ro, rd;
		if (!CursorRay(ro, rd))
			return;

		if (DCO_GMAttach.IsArmed())
		{
			DCO_GMAttach.TryAttach(owner, ro, rd);
			return;
		}

		int handle;
		if (m_Mode == EDCO_GizmoMode.ROTATE)
			handle = DCO_GMGizmoRotate.PickRing(origin, axes, len, ro, rd, len * 0.12);
		else
			handle = DCO_GMGizmoPick.PickMove(origin, axes, len, ro, rd, len * 0.12);	// planes tested before arrows.
		if (handle < 0)
			return;	// not on a handle.

		m_bDragging = true;
		m_GrabHandle = handle;
		m_GrabRo = ro;
		m_GrabRd = rd;
		m_StartOrigin = origin;
		m_DragOwner = owner;
		m_bHasDragTransform = false;
		m_iLastPreviewAt = 0;
		m_GrabAxes[0] = axes[0];
		m_GrabAxes[1] = axes[1];
		m_GrabAxes[2] = axes[2];

		if (m_Mode == EDCO_GizmoMode.ROTATE)
		{
			// Freeze the transform the whole drag is measured against, plus the angle the cursor grabbed at.
			owner.GetWorldTransform(m_StartMat);
			m_bGrabAngleOk = DCO_GMGizmoRotate.RingAngle(origin, m_GrabAxes[handle], ro, rd, m_GrabAngle);
		}
	}

	// LMB up: end the drag.
	void OnLmbUp(float value, EActionTrigger reason)
	{
		if (DCO_GMUIController.IsNativePropertiesOpen())
		{
			AbortDrag();
			return;
		}
		if (!m_bDragging)
			return;
		if (m_DragOwner && m_bHasDragTransform)
			DCO_GMToolsServer.RouteTransform(m_DragOwner, m_LastDragPos, m_LastDragAngles, true);
		m_bDragging = false;
		m_GrabHandle = -1;
		m_DragOwner = null;
		m_bHasDragTransform = false;
	}

	protected void AbortDrag()
	{
		m_bDragging = false;
		m_GrabHandle = -1;
		m_DragOwner = null;
		m_bHasDragTransform = false;
	}

	protected void RouteDragTransform(vector pos, vector anglesDeg)
	{
		m_LastDragPos = pos;
		m_LastDragAngles = anglesDeg;
		m_bHasDragTransform = true;
		int now = System.GetTickCount();
		if (m_iLastPreviewAt && now - m_iLastPreviewAt < PREVIEW_INTERVAL_MS)
			return;
		m_iLastPreviewAt = now;
		DCO_GMToolsServer.RouteTransform(m_DragOwner, pos, anglesDeg, false);
	}

	protected vector ApplySurfaceSnap(vector pos)
	{
		if (!m_bSurfaceSnap || !m_DragOwner || MotionIsVertical())
			return pos;
		array<IEntity> excl = {};
		excl.Insert(m_DragOwner);
		vector snapped = pos;
		SCR_TerrainHelper.SnapToGeometry(snapped, pos, excl);
		return snapped;
	}

	protected bool MotionIsVertical()
	{
		if (DCO_GMGizmoMath.IsPlaneHandle(m_GrabHandle))
		{
			vector u, v, n;
			if (!DCO_GMGizmoMath.PlaneHandleAxes(m_GrabHandle, m_GrabAxes, u, v, n))
				return false;
			return Math.AbsFloat(u[1]) > VERT_MOTION_EPS || Math.AbsFloat(v[1]) > VERT_MOTION_EPS;
		}
		if (m_GrabHandle < DCO_GMGizmoMath.AXIS_X || m_GrabHandle > DCO_GMGizmoMath.AXIS_Z)
			return false;
		vector dir = m_GrabAxes[m_GrabHandle];
		return Math.AbsFloat(dir[1]) > VERT_MOTION_EPS;
	}

	protected void DragMove(vector cro, vector crd)
	{
		vector newOrigin;
		if (DCO_GMGizmoMath.IsPlaneHandle(m_GrabHandle))
		{
			vector u, v, n;
			if (!DCO_GMGizmoMath.PlaneHandleAxes(m_GrabHandle, m_GrabAxes, u, v, n))
				return;
			newOrigin = DCO_GMGizmoApply.MoveInPlane(m_StartOrigin, n, u, v, m_GrabRo, m_GrabRd, cro, crd, ActiveStep());
		}
		else
		{
			newOrigin = DCO_GMGizmoApply.MoveAlongAxis(m_StartOrigin, m_GrabAxes[m_GrabHandle], m_GrabRo, m_GrabRd, cro, crd, ActiveStep());
		}

		newOrigin = ApplySurfaceSnap(newOrigin);

		vector m[4];
		m_DragOwner.GetWorldTransform(m);
		vector rot[3];
		rot[0] = m[0]; rot[1] = m[1]; rot[2] = m[2];
		vector angles = Math3D.MatrixToAngles(rot);
		RouteDragTransform(newOrigin, angles);
	}

	// Push the target's live transform to the readout box, or blank it when there is nothing to report.
	protected void FeedPanel(IEntity owner)
	{
		if (!m_Panel)
			return;
		if (!owner)
		{
			m_Panel.Update(null, vector.Zero, vector.Zero, false);
			return;
		}
		vector m[4];
		owner.GetWorldTransform(m);
		vector rot[3];
		rot[0] = m[0]; rot[1] = m[1]; rot[2] = m[2];
		m_Panel.Update(owner, m[3], Math3D.MatrixToAngles(rot), true);
	}

	protected void PanelHide()
	{
		if (m_Panel)
			m_Panel.Update(null, vector.Zero, vector.Zero, false);
	}

	protected void OnRender(DCO_GMRenderManager r)
	{
		if (DCO_GMUIController.IsNativePropertiesOpen())
			return;
		if (!s_bPreciseMode)
		{
			PanelHide();	// arrows hidden -> the readout goes with them, leaving engine place/drag untouched.
			return;
		}
		if (!m_bDragging)
			ResolveTarget();	// freeze target resolution during a drag so a selection flicker can't drop the gizmo.
		if (!m_Target)
		{
			PanelHide();
			return;
		}
		IEntity owner = m_Target.GetOwner();
		if (!owner)
		{
			PanelHide();
			return;
		}
		if (DCO_GMAttach.IsArmed())
		{
			PanelHide();
			vector aro, ard;
			if (CursorRay(aro, ard))
				DCO_GMAttach.UpdateHoverHighlight(owner, aro, ard);
			return;
		}
		DCO_GMAttach.ClearHighlight();

		vector origin = owner.GetOrigin();
		float len;
		ScreenScaledLength(origin, len);
		vector axes[3];
		AxisBasis(owner, axes);

		if (m_bDragging && m_DragOwner)
		{
			vector cro, crd;
			if (CursorRay(cro, crd))
			{
				if (m_Mode == EDCO_GizmoMode.ROTATE)
				{
					float nowAngle;
					if (m_bGrabAngleOk && DCO_GMGizmoRotate.RingAngle(m_StartOrigin, m_GrabAxes[m_GrabHandle], cro, crd, nowAngle))
					{
						float delta = DCO_GMGizmoMath.WrapAngle(nowAngle - m_GrabAngle);
						vector angles = DCO_GMGizmoRotate.ApplyRotation(m_StartMat, m_StartOrigin, m_GrabAxes[m_GrabHandle], delta);
						RouteDragTransform(m_StartOrigin, angles);
					}
				}
				else
				{
					DragMove(cro, crd);	// axis or plane translate, snapped + surface-settled, via the dedi-safe relay.
				}
			}
			m_Hover = m_GrabHandle;	// keep the grabbed handle lit while dragging.
		}
		else
		{
			m_Hover = -1;
			vector ro, rd;
			if (CursorRay(ro, rd))
			{
				if (m_Mode == EDCO_GizmoMode.ROTATE)
					m_Hover = DCO_GMGizmoRotate.PickRing(origin, axes, len, ro, rd, len * 0.12);
				else
					m_Hover = DCO_GMGizmoPick.PickMove(origin, axes, len, ro, rd, len * 0.12);
			}
		}

		if (m_Mode == EDCO_GizmoMode.ROTATE)
		{
			if (m_bDragging)
				m_DrawRot.DrawRotate(r, origin, m_GrabAxes, len, m_Hover);
			else
				m_DrawRot.DrawRotate(r, origin, axes, len, m_Hover);
		}
		else
			m_Draw.DrawMove(r, origin, axes, len, m_Hover);

		FeedPanel(owner);	// last: the readout reports the transform this tick just wrote.
	}
}
