class DCO_GMGizmoRotate
{
	// Draw the three rotation rings.
	void DrawRotate(DCO_GMRenderManager r, vector origin, vector axes[3], float radius, int hoverHandle)
	{
		DrawOneRing(r, origin, axes[0], radius, DCO_GMGizmoRender.COL_X, hoverHandle == 0);
		DrawOneRing(r, origin, axes[1], radius, DCO_GMGizmoRender.COL_Y, hoverHandle == 1);
		DrawOneRing(r, origin, axes[2], radius, DCO_GMGizmoRender.COL_Z, hoverHandle == 2);
	}

	protected void DrawOneRing(DCO_GMRenderManager r, vector origin, vector axis, float radius, int col, bool hot)
	{
		int c = col;
		if (hot)
			c = DCO_GMGizmoRender.COL_HILITE;
		vector u, v;
		DCO_GMGizmoMath.PlaneBasis(axis, u, v);
		r.DrawRing(origin, u, v, radius, c);
	}

	static int PickRing(vector origin, vector axes[3], float radius, vector ro, vector rd, float tol)
	{
		int best = -1;
		float bestErr = tol;
		for (int i = 0; i < 3; i++)
		{
			vector hit;
			if (!DCO_GMGizmoMath.RayPlane(ro, rd, origin, axes[i], hit))
				continue;
			float err = Math.AbsFloat(vector.Distance(hit, origin) - radius);
			if (err < bestErr)
			{
				bestErr = err;
				best = i;
			}
		}
		return best;
	}

	static bool RingAngle(vector origin, vector axis, vector ro, vector rd, out float angle)
	{
		vector hit;
		if (!DCO_GMGizmoMath.RayPlane(ro, rd, origin, axis, hit))
			return false;
		vector u, v;
		DCO_GMGizmoMath.PlaneBasis(axis, u, v);
		angle = DCO_GMGizmoMath.AngleOnPlane(hit, origin, u, v);
		return true;
	}

	// Apply a drag to the transform CAPTURED AT GRAB and hand back the resulting Euler angles for the relay.
	static vector ApplyRotation(vector startMat[4], vector pivot, vector axis, float deltaRad)
	{
		vector outMat[4];
		SCR_Math3D.RotateAround(startMat, pivot, axis, deltaRad, outMat);
		vector rot[3];
		rot[0] = outMat[0];
		rot[1] = outMat[1];
		rot[2] = outMat[2];
		return Math3D.MatrixToAngles(rot);	// the relay carries Euler angles; SetEntityTransform rebuilds the matrix.
	}
}
