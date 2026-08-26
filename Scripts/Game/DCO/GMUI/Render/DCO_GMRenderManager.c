// Draws client world cues on a retained UI canvas.
class DCO_GMRenderManager
{
	static const int MAX_COMMANDS = 4096;

	protected CanvasWidget m_wCanvas;
	protected ref array<ref CanvasWidgetCommand> m_DrawCommands = {};
	protected ref ScriptInvoker m_OnRender = new ScriptInvoker();
	protected bool m_bActive;
	protected bool m_bCommandCapLogged;
	protected int m_iLastDiagAt;
	protected int m_iLastDiagCount = -1;

	ScriptInvoker GetOnRender()
	{
		return m_OnRender;
	}

	void Start(Widget shellRoot)
	{
		if (m_bActive)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace && shellRoot)
		{
			// Use an authored full-screen canvas for stable size and layering.
			m_wCanvas = CanvasWidget.Cast(shellRoot.FindAnyWidget("DCO_WorldCueCanvas"));
			if (m_wCanvas)
				m_wCanvas.SetDrawCommands(m_DrawCommands);
		}

		m_bActive = true;
		GetGame().GetCallqueue().CallLater(Update, 100, true);
		if (m_wCanvas)
			Print(string.Format("[DCO-GM] GM-client canvas renderer STARTED (client=%1x%2)", workspace.GetWidth(), workspace.GetHeight()), LogLevel.NORMAL);
		else
			Print("[DCO-GM] GM-client canvas renderer FAILED: canvas unavailable", LogLevel.ERROR);
	}

	void Stop()
	{
		m_bActive = false;
		GetGame().GetCallqueue().Remove(Update);
		m_DrawCommands.Clear();
		if (m_wCanvas)
		{
			m_wCanvas.SetDrawCommands(m_DrawCommands);
			m_wCanvas = null;
		}
		Print("[DCO-GM] GM-client canvas renderer STOPPED", LogLevel.NORMAL);
	}

	protected void Update()
	{
		m_DrawCommands.Clear();
		m_bCommandCapLogged = false;
		if (!m_bActive || !m_wCanvas || DCO_GMTheme.Get().IsMasterHidden())
		{
			if (m_wCanvas)
				m_wCanvas.SetDrawCommands(m_DrawCommands);
			return;
		}

		m_OnRender.Invoke(this);
		DrawSelectionBoxes();
		m_wCanvas.SetDrawCommands(m_DrawCommands);
		LogRendererHealth();
	}

	protected void LogRendererHealth()
	{
		int count = m_DrawCommands.Count();
		int now = System.GetTickCount();
		bool edge = (count == 0) != (m_iLastDiagCount == 0);
		if (m_iLastDiagCount < 0 || edge || now - m_iLastDiagAt >= 5000)
		{
			Print(string.Format("[DCO-GM] canvas renderer health: commands=%1 canvas=%2", count, m_wCanvas != null), LogLevel.NORMAL);
			m_iLastDiagAt = now;
			m_iLastDiagCount = count;
		}
	}

	protected bool Project(vector worldPosition, out vector screenPosition)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		if (!workspace || !world)
			return false;
		screenPosition = workspace.ProjWorldToScreenNative(worldPosition, world);
		return screenPosition[2] >= 0;
	}

	protected void AddScreenLine(vector from, vector to, int colorARGB, float width = 2.0)
	{
		if (m_DrawCommands.Count() >= MAX_COMMANDS)
		{
			if (!m_bCommandCapLogged)
			{
				m_bCommandCapLogged = true;
				Print(string.Format("[DCO-GM] canvas renderer command cap reached (%1); remaining cues skipped this frame", MAX_COMMANDS), LogLevel.WARNING);
			}
			return;
		}
		LineDrawCommand line = new LineDrawCommand();
		line.m_iColor = colorARGB;
		line.m_fOutlineWidth = 0;
		line.m_fWidth = width;
		line.m_Vertices = {from[0], from[1], to[0], to[1]};
		m_DrawCommands.Insert(line);
	}

	protected void AddScreenPolyline(notnull array<float> vertices, int colorARGB, float width = 2.0, bool enclose = false)
	{
		if (vertices.Count() < 4 || m_DrawCommands.Count() >= MAX_COMMANDS)
			return;
		LineDrawCommand line = new LineDrawCommand();
		line.m_iColor = colorARGB;
		line.m_fOutlineWidth = 0;
		line.m_fWidth = width;
		line.m_bShouldEnclose = enclose;
		line.m_Vertices = vertices;
		m_DrawCommands.Insert(line);
	}

	void DrawSphere(vector pos, float radius, int colorARGB)
	{
		vector center;
		vector edge;
		if (!Project(pos, center) || !Project(pos + Vector(radius, 0, 0), edge))
			return;
		float pixelRadius = vector.Distance(Vector(center[0], center[1], 0), Vector(edge[0], edge[1], 0));
		if (pixelRadius < 2)
			pixelRadius = 2;
		array<float> vertices = {};
		m_wCanvas.TessellateCircle(center, pixelRadius, 16, vertices);
		AddScreenPolyline(vertices, colorARGB, 2.0, true);
	}

	void DrawArrow(vector from, vector to, float size, int colorARGB)
	{
		vector a;
		vector b;
		if (!Project(from, a) || !Project(to, b))
			return;
		AddScreenLine(a, b, colorARGB);
		vector delta = Vector(b[0] - a[0], b[1] - a[1], 0);
		float length = delta.Length();
		if (length < 1)
			return;
		delta = delta / length;
		vector side = Vector(-delta[1], delta[0], 0);
		float head = Math.Clamp(size * 34.0, 6.0, 16.0);
		vector left = b - delta * head + side * head * 0.55;
		vector right = b - delta * head - side * head * 0.55;
		array<float> arrowHead = {left[0], left[1], b[0], b[1], right[0], right[1]};
		AddScreenPolyline(arrowHead, colorARGB);
	}

	void DrawLine(vector from, vector to, int colorARGB)
	{
		vector a;
		vector b;
		if (Project(from, a) && Project(to, b))
			AddScreenLine(a, b, colorARGB);
	}

	void DrawStick(vector groundPos, float height, int colorARGB)
	{
		DrawArrow(groundPos, groundPos + Vector(0, height, 0), 0.15, colorARGB);
	}

	void DrawBox(vector mn, vector mx, int colorARGB)
	{
		vector c0 = Vector(mn[0], mn[1], mn[2]);
		vector c1 = Vector(mx[0], mn[1], mn[2]);
		vector c2 = Vector(mx[0], mn[1], mx[2]);
		vector c3 = Vector(mn[0], mn[1], mx[2]);
		vector c4 = Vector(mn[0], mx[1], mn[2]);
		vector c5 = Vector(mx[0], mx[1], mn[2]);
		vector c6 = Vector(mx[0], mx[1], mx[2]);
		vector c7 = Vector(mn[0], mx[1], mx[2]);
		DrawLine(c0, c1, colorARGB); DrawLine(c1, c2, colorARGB); DrawLine(c2, c3, colorARGB); DrawLine(c3, c0, colorARGB);
		DrawLine(c4, c5, colorARGB); DrawLine(c5, c6, colorARGB); DrawLine(c6, c7, colorARGB); DrawLine(c7, c4, colorARGB);
		DrawLine(c0, c4, colorARGB); DrawLine(c1, c5, colorARGB); DrawLine(c2, c6, colorARGB); DrawLine(c3, c7, colorARGB);
	}

	void DrawQuad(vector p0, vector p1, vector p2, vector p3, int colorARGB)
	{
		DrawLine(p0, p1, colorARGB);
		DrawLine(p1, p2, colorARGB);
		DrawLine(p2, p3, colorARGB);
		DrawLine(p3, p0, colorARGB);
	}

	void DrawRing(vector center, vector u, vector v, float radius, int colorARGB)
	{
		vector previous = center + u * radius;
		for (int i = 1; i <= 24; i++)
		{
			float angle = Math.PI2 * i / 24.0;
			vector next = center + u * (Math.Cos(angle) * radius) + v * (Math.Sin(angle) * radius);
			DrawLine(previous, next, colorARGB);
			previous = next;
		}
	}

	protected int AccentARGB(int alpha)
	{
		Color c = DCO_GMTheme.Get().m_AccentColor;
		int r = 217, g = 137, b = 43;
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
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
		foreach (SCR_EditableEntityComponent editable : selected)
		{
			if (!editable)
				continue;
			bool isUnit = SCR_EditableCharacterComponent.Cast(editable) != null || SCR_EditableVehicleComponent.Cast(editable) != null;
			if (!isUnit)
				continue;
			IEntity owner = editable.GetOwner();
			if (!owner)
				continue;
			vector minimum;
			vector maximum;
			owner.GetBounds(minimum, maximum);
			vector origin = owner.GetOrigin();
			minimum = origin + minimum;
			maximum = origin + maximum;
			if (maximum[1] - minimum[1] < 0.2)
			{
				minimum = origin + Vector(-0.4, 0, -0.4);
				maximum = origin + Vector(0.4, 1.8, 0.4);
			}
			float cx = (minimum[0] + maximum[0]) * 0.5;
			float cz = (minimum[2] + maximum[2]) * 0.5;
			float halfW = Math.Max(1.0, (maximum[0] - minimum[0]) * 0.5 + 0.2);
			float halfD = Math.Max(1.0, (maximum[2] - minimum[2]) * 0.5 + 0.2);
			DrawBox(Vector(cx - halfW, minimum[1], cz - halfD), Vector(cx + halfW, Math.Max(minimum[1] + 2.2, maximum[1] + 0.25), cz + halfD), boxColor);
		}
	}
}
