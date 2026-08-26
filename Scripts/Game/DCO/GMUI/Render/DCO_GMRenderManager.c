
class DCO_GMRenderManager
{
	protected ref array<ref Shape> m_Shapes = {};
	protected ref ScriptInvoker m_OnRender = new ScriptInvoker();
	protected bool m_bActive;

	// Cues subscribe their draw method here; invoked every render tick with this manager.
	ScriptInvoker GetOnRender()
	{
		return m_OnRender;
	}

	void Start()
	{
		if (m_bActive)
			return;
		m_bActive = true;
		GetGame().GetCallqueue().CallLater(Update, 100, true);	// ~10 Hz client-side redraw.
		Print("[DCO-GM] GM-client render pillar STARTED (MP-safe, GM-only)", LogLevel.NORMAL);
	}

	void Stop()
	{
		m_bActive = false;
		GetGame().GetCallqueue().Remove(Update);
		m_Shapes.Clear();	// release all held shapes.
		Print("[DCO-GM] GM-client render pillar STOPPED", LogLevel.NORMAL);
	}

	protected void Update()
	{
		m_Shapes.Clear();
		if (!m_bActive || DCO_GMTheme.Get().IsMasterHidden())
			return;	// cinematic mode is a parent gate for every DCO world cue, including selection boxes and gizmos.
		m_OnRender.Invoke(this);	// registered cues draw.
		DrawSelectionBoxes();	// F5 - boxes around the GM's selected units.
	}

	void DrawSphere(vector pos, float radius, int colorARGB)
	{
		Shape s = Shape.CreateSphere(colorARGB, ShapeFlags.WIREFRAME | ShapeFlags.NOZBUFFER, pos, radius);
		if (s)
			m_Shapes.Insert(s);
	}

	void DrawArrow(vector from, vector to, float size, int colorARGB)
	{
		Shape s = Shape.CreateArrow(from, to, size, colorARGB, ShapeFlags.NOZBUFFER);
		if (s)
			m_Shapes.Insert(s);
	}

	// One clean world-space segment.
	void DrawLine(vector from, vector to, int colorARGB)
	{
		vector p[2];
		p[0] = from;
		p[1] = to;
		Shape s = Shape.CreateLines(colorARGB, ShapeFlags.NOZBUFFER, p, 2);
		if (s)
			m_Shapes.Insert(s);
	}

	void DrawStick(vector groundPos, float height, int colorARGB)
	{
		vector top = groundPos + Vector(0, height, 0);
		Shape s = Shape.CreateArrow(groundPos, top, 0.15, colorARGB, ShapeFlags.NOZBUFFER);
		if (s)
			m_Shapes.Insert(s);
	}

	// Wireframe box from a world-space min/max corner.
	void DrawBox(vector mn, vector mx, int colorARGB)
	{
		vector c0 = Vector(mn[0], mn[1], mn[2]);	// bottom: front-left.
		vector c1 = Vector(mx[0], mn[1], mn[2]);	// front-right.
		vector c2 = Vector(mx[0], mn[1], mx[2]);	// back-right.
		vector c3 = Vector(mn[0], mn[1], mx[2]);	// back-left.
		vector c4 = Vector(mn[0], mx[1], mn[2]);
		vector c5 = Vector(mx[0], mx[1], mn[2]);
		vector c6 = Vector(mx[0], mx[1], mx[2]);
		vector c7 = Vector(mn[0], mx[1], mx[2]);

		vector p[16];
		p[0]  = c0; p[1]  = c1; p[2]  = c2; p[3]  = c3; p[4] = c0;	// bottom ring.
		p[5]  = c4; p[6]  = c5; p[7]  = c6; p[8]  = c7; p[9] = c4;
		p[10] = c5; p[11] = c1;	// retrace top edge c4-c5 -> vertical c1-c5.
		p[12] = c2; p[13] = c6;	// retrace bottom edge c1-c2 -> vertical c2-c6.
		p[14] = c7; p[15] = c3;	// retrace top edge c6-c7 -> vertical c3-c7.

		Shape s = Shape.CreateLines(colorARGB, ShapeFlags.NOZBUFFER, p, 16);
		if (s)
			m_Shapes.Insert(s);
	}

	void DrawQuad(vector p0, vector p1, vector p2, vector p3, int colorARGB)
	{
		vector p[8];
		p[0] = p0; p[1] = p1;
		p[2] = p1; p[3] = p2;
		p[4] = p2; p[5] = p3;
		p[6] = p3; p[7] = p0;

		Shape s = Shape.CreateLines(colorARGB, ShapeFlags.NOZBUFFER, p, 8);
		if (s)
			m_Shapes.Insert(s);
	}

	void DrawRing(vector center, vector u, vector v, float radius, int colorARGB)
	{
		vector p[64];	// 32 segments x 2 endpoints each.
		for (int i = 0; i < 32; i++)
		{
			float a0 = (Math.PI2 * i) / 32.0;
			float a1 = (Math.PI2 * (i + 1)) / 32.0;
			p[i * 2]     = center + u * (Math.Cos(a0) * radius) + v * (Math.Sin(a0) * radius);
			p[i * 2 + 1] = center + u * (Math.Cos(a1) * radius) + v * (Math.Sin(a1) * radius);
		}
		Shape s = Shape.CreateLines(colorARGB, ShapeFlags.NOZBUFFER, p, 64);
		if (s)
			m_Shapes.Insert(s);
	}

	protected int AccentARGB(int alpha)
	{
		Color c = DCO_GMTheme.Get().m_AccentColor;
		int r = 217, g = 137, b = 43;	// amber fallback.
		if (c)
		{
			r = Math.Round(c.R() * 255);
			g = Math.Round(c.G() * 255);
			b = Math.Round(c.B() * 255);
		}
		return ((alpha & 0xFF) << 24) | (r << 16) | (g << 8) | b;
	}

	protected void DrawSelectionBoxes()
	{
		int boxColor = AccentARGB(255);

		set<SCR_EditableEntityComponent> sel = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(sel, EEditableEntityState.SELECTED);
		foreach (SCR_EditableEntityComponent e : sel)
		{
			if (!e)
				continue;

			// Only physical units get a box.
			bool isUnit = (SCR_EditableCharacterComponent.Cast(e) != null) || (SCR_EditableVehicleComponent.Cast(e) != null);
			if (!isUnit)
				continue;

			IEntity owner = e.GetOwner();
			if (!owner)
				continue;

			vector mn, mx;
			owner.GetBounds(mn, mx);
			vector origin = owner.GetOrigin();
			vector bmin = origin + mn;
			vector bmax = origin + mx;

			if (bmax[1] - bmin[1] < 0.2)
			{
				bmin = origin + Vector(-0.4, 0.0, -0.4);
				bmax = origin + Vector(0.4, 1.8, 0.4);
			}

			// Roomy configurable cube centered on the entity.
			float cx = (bmin[0] + bmax[0]) * 0.5;
			float cz = (bmin[2] + bmax[2]) * 0.5;
			float halfW = Math.Max(1.0, (bmax[0] - bmin[0]) * 0.5 + 0.2);
			float halfD = Math.Max(1.0, (bmax[2] - bmin[2]) * 0.5 + 0.2);
			float baseY = bmin[1];
			float topY  = Math.Max(bmin[1] + 2.2, bmax[1] + 0.25);

			vector wmn = Vector(cx - halfW, baseY, cz - halfD);
			vector wmx = Vector(cx + halfW, topY,  cz + halfD);

			DrawBox(wmn, wmx, boxColor);
		}
	}
}
