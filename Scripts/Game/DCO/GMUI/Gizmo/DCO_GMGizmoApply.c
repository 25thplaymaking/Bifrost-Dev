class DCO_GMGizmoApply
{
	static const float MAX_PLANE_DELTA = 25.0;

	// Round `f` to the nearest multiple of `step`.
	static float SnapFloat(float f, float step)
	{
		if (step <= 0)
			return f;
		return Math.Round(f / step) * step;
	}

	// Component-wise SnapFloat.
	static vector SnapVec(vector v, float step)
	{
		if (step <= 0)
			return v;
		return Vector(SnapFloat(v[0], step), SnapFloat(v[1], step), SnapFloat(v[2], step));
	}

	static vector MoveAlongAxis(vector origin, vector axisDir, vector grabRo, vector grabRd, vector curRo, vector curRd, float step = 0)
	{
		float s0, d0, s1, d1;
		DCO_GMGizmoMath.ClosestPointsRayLine(grabRo, grabRd, origin, axisDir, s0, d0);
		DCO_GMGizmoMath.ClosestPointsRayLine(curRo,  curRd,  origin, axisDir, s1, d1);
		float delta = s1 - s0;
		if (delta > MAX_PLANE_DELTA)
			delta = MAX_PLANE_DELTA;
		else if (delta < -MAX_PLANE_DELTA)
			delta = -MAX_PLANE_DELTA;
		return origin + axisDir * SnapFloat(delta, step);
	}

	static vector MoveInPlane(vector origin, vector planeN, vector u, vector v, vector grabRo, vector grabRd, vector curRo, vector curRd, float step = 0)
	{
		vector h0, h1;
		if (!DCO_GMGizmoMath.RayPlane(grabRo, grabRd, origin, planeN, h0))
			return origin;
		if (!DCO_GMGizmoMath.RayPlane(curRo, curRd, origin, planeN, h1))
			return origin;
		vector d = h1 - h0;
		float dLen = d.Length();
		if (dLen > MAX_PLANE_DELTA)
			d = d * (MAX_PLANE_DELTA / dLen);
		return origin + u * SnapFloat(vector.Dot(d, u), step) + v * SnapFloat(vector.Dot(d, v), step);
	}
}
