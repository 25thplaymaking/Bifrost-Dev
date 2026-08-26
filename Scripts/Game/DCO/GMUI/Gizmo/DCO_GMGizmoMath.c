class DCO_GMGizmoMath
{
	// Handle ids, centralised so render / pick / apply / the controller can never disagree about what "4" means.
	static const int AXIS_X   = 0;
	static const int AXIS_Y   = 1;
	static const int AXIS_Z   = 2;
	static const int PLANE_XY = 3;
	static const int PLANE_XZ = 4;
	static const int PLANE_YZ = 5;

// Keeps plane handles proportional by spanning the same near-to-far range along both axes.
	static const float QUAD_NEAR = 0.22;
	static const float QUAD_FAR  = 0.52;

	static void ClosestPointsRayLine(vector ro, vector rd, vector la, vector ld, out float sLine, out float dist)
	{
		// w0 = rayOrigin - lineOrigin.
		vector r = ro - la;
		float a = vector.Dot(rd, rd);
		if (a < 0.00001)
		{
			sLine = 0;
			dist = 1000000000.0;
			return;
		}
		float b = vector.Dot(rd, ld);
		float c = vector.Dot(ld, ld);
		float d = vector.Dot(rd, r);
		float e = vector.Dot(ld, r);
		float denom = a * c - b * b;
		float s;
		if (denom > 0.00001)
			s = (a * e - b * d) / denom;
		else
			s = 0;	// near-parallel -> pick the line origin.
		float t = (b * s - d) / a;
		if (t < 0)
			t = 0;	// clamp to the ray's forward half.
		vector pRay  = ro + rd * t;
		vector pLine = la + ld * s;
		sLine = s;
		dist = vector.Distance(pRay, pLine);
	}

	static bool RayPlane(vector ro, vector rd, vector planeP, vector planeN, out vector hit)
	{
		if (Math.AbsFloat(vector.Dot(rd, planeN)) < 0.0001)
			return false;
		hit = SCR_Math3D.IntersectPlane(ro, rd, planeP, planeN);
		if (vector.Dot(hit - ro, rd) < 0)
			return false;
		return true;
	}

	static bool PlaneHandleAxes(int handle, vector axes[3], out vector u, out vector v, out vector n)
	{
		switch (handle)
		{
			case PLANE_XY:
			{
				u = axes[AXIS_X]; v = axes[AXIS_Y]; n = axes[AXIS_Z];
				return true;
			}
			case PLANE_XZ:
			{
				u = axes[AXIS_X]; v = axes[AXIS_Z]; n = axes[AXIS_Y];
				return true;
			}
			case PLANE_YZ:
			{
				u = axes[AXIS_Y]; v = axes[AXIS_Z]; n = axes[AXIS_X];
				return true;
			}
		}
		return false;
	}

	static bool IsPlaneHandle(int handle)
	{
		return handle >= PLANE_XY && handle <= PLANE_YZ;
	}

	static void PlaneBasis(vector axis, out vector u, out vector v)
	{
		vector seed = Vector(0, 1, 0);
		if (Math.AbsFloat(vector.Dot(axis, seed)) > 0.9)
			seed = Vector(1, 0, 0);
		u = SCR_Math3D.Cross(seed, axis, true);
		u.Normalize();
		v = SCR_Math3D.Cross(axis, u, true);
		v.Normalize();
	}

	static float AngleOnPlane(vector p, vector origin, vector u, vector v)
	{
		vector d = p - origin;
		return Math.Atan2(vector.Dot(d, v), vector.Dot(d, u));
	}

	static float WrapAngle(float a)
	{
		while (a > Math.PI)
			a -= Math.PI2;
		while (a <= -Math.PI)
			a += Math.PI2;
		return a;
	}
}

// Gizmo transform mode.
enum EDCO_GizmoMode
{
	MOVE,
	ROTATE,
	SCALE
}

// Reference frame the handles/axes use.
enum EDCO_GizmoSpace
{
	WORLD,
	LOCAL
}
