class DCO_GMGizmoPick
{
	static int PickAxis(vector origin, vector axes[3], float len, vector ro, vector rd, float pickWorldR)
	{
		int best = -1;
		float bestDist = pickWorldR;
		for (int i = 0; i < 3; i++)
		{
			float s, dist;
			DCO_GMGizmoMath.ClosestPointsRayLine(ro, rd, origin, axes[i], s, dist);
			if (s < 0 || s > len)
				continue;	// beyond the arrow's drawn length.
			if (dist < bestDist)
			{
				bestDist = dist;
				best = i;
			}
		}
		return best;
	}

	static int PickPlane(vector origin, vector axes[3], float len, vector ro, vector rd)
	{
		float qNear = len * DCO_GMGizmoMath.QUAD_NEAR;
		float qFar  = len * DCO_GMGizmoMath.QUAD_FAR;

		int best = -1;
		float bestT = 1000000.0;
		for (int h = DCO_GMGizmoMath.PLANE_XY; h <= DCO_GMGizmoMath.PLANE_YZ; h++)
		{
			vector u, v, n;
			if (!DCO_GMGizmoMath.PlaneHandleAxes(h, axes, u, v, n))
				continue;

			vector hit;
			if (!DCO_GMGizmoMath.RayPlane(ro, rd, origin, n, hit))
				continue;	// edge-on to this plane.

			float t = vector.Dot(hit - ro, rd);
			if (t <= 0)
				continue;	// the intersection is behind the camera.

			vector d = hit - origin;
			float du = vector.Dot(d, u);
			float dv = vector.Dot(d, v);
			if (du < qNear || du > qFar || dv < qNear || dv > qFar)
				continue;	// outside the drawn square.

			if (t < bestT)
			{
				bestT = t;
				best = h;
			}
		}
		return best;
	}

	static int PickMove(vector origin, vector axes[3], float len, vector ro, vector rd, float pickWorldR)
	{
		int h = PickPlane(origin, axes, len, ro, rd);
		if (h >= 0)
			return h;
		return PickAxis(origin, axes, len, ro, rd, pickWorldR);
	}
}
