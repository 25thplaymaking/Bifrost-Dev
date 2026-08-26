class DCO_GMGizmoRender
{
	static const int COL_X      = 0xFFE0402A;
	static const int COL_Y      = 0xFF40E020;
	static const int COL_Z      = 0xFF3060FF;
	static const int COL_HILITE = 0xFFFFE060;	// amber highlight for the hovered/grabbed handle.

	// Draw the three MOVE axis arrows AND the three plane squares.
	void DrawMove(DCO_GMRenderManager r, vector origin, vector axes[3], float len, int hoverHandle)
	{
		DrawAxis(r, origin, axes[0], len, COL_X, hoverHandle == DCO_GMGizmoMath.AXIS_X);
		DrawAxis(r, origin, axes[1], len, COL_Y, hoverHandle == DCO_GMGizmoMath.AXIS_Y);
		DrawAxis(r, origin, axes[2], len, COL_Z, hoverHandle == DCO_GMGizmoMath.AXIS_Z);
		DrawPlanes(r, origin, axes, len, hoverHandle);
	}

	protected void DrawAxis(DCO_GMRenderManager r, vector origin, vector dir, float len, int col, bool hot)
	{
		int c = col;
		if (hot)
			c = COL_HILITE;
		vector to = origin + dir * len;
		r.DrawArrow(origin, to, len * 0.12, c);
	}

	protected void DrawPlanes(DCO_GMRenderManager r, vector origin, vector axes[3], float len, int hoverHandle)
	{
		DrawOneQuad(r, origin, axes, len, DCO_GMGizmoMath.PLANE_XY, COL_Z, hoverHandle);
		DrawOneQuad(r, origin, axes, len, DCO_GMGizmoMath.PLANE_XZ, COL_Y, hoverHandle);
		DrawOneQuad(r, origin, axes, len, DCO_GMGizmoMath.PLANE_YZ, COL_X, hoverHandle);
	}

	protected void DrawOneQuad(DCO_GMRenderManager r, vector origin, vector axes[3], float len, int handle, int col, int hoverHandle)
	{
		vector u, v, n;
		if (!DCO_GMGizmoMath.PlaneHandleAxes(handle, axes, u, v, n))
			return;

		int c = col;
		if (hoverHandle == handle)
			c = COL_HILITE;

		float a = len * DCO_GMGizmoMath.QUAD_NEAR;
		float b = len * DCO_GMGizmoMath.QUAD_FAR;
		r.DrawQuad(origin + u * a + v * a,
				   origin + u * b + v * a,
				   origin + u * b + v * b,
				   origin + u * a + v * b,
				   c);
	}
}
