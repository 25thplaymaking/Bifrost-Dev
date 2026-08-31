// Hover feedback for the GM shell.
class DCO_GMHoverState
{
	ref array<Widget> m_Buttons = {};
	ref array<Widget> m_Labels = {};
	ref array<bool> m_IsBg = {};
}

class DCO_GMHover
{
	protected static ref DCO_GMHoverState s_State;

	protected static DCO_GMHoverState State()
	{
		if (!s_State)
			s_State = new DCO_GMHoverState();
		return s_State;
	}

	protected static Widget s_Current;	// label currently tinted, or null.
	protected static int    s_PrevColor;	// its colour before we touched it.
	protected static int    s_SetColor;

	// Hover is a TWO-stage lift.
	protected static const float ACCENT_PULL = 1.00;	// fully to the live accent.
	protected static const float WHITE_LIFT  = 0.45;	// then hard toward white.
	protected static const int   TICK_MS   = 60;	// poll cadence - cheap: a few dozen pointer compares.
	protected static const int   MAX_CLIMB = 6;	// parent hops from the cursor widget up to a registered button.

	// Register `buttonName` as a hover root, tinting `targetName` (usually its label).
	static void Wire(Widget root, string buttonName, string targetName)
	{
		if (!root)
			return;
		Widget btn = root.FindAnyWidget(buttonName);
		if (!btn)
			return;

		// Prefer a dedicated background plate.
		bool isBg = true;
		Widget target = root.FindAnyWidget(buttonName + "_Bg");
		if (!target)
		{
			isBg = false;
			target = root.FindAnyWidget(targetName);
		}
		if (!target)
			return;

		State().m_Buttons.Insert(btn);
		State().m_Labels.Insert(target);
		State().m_IsBg.Insert(isBg);
	}

	static void WirePool(Widget root, string buttonFmt, string targetFmt, int from, int count)
	{
		for (int i = from; i < from + count; i++)
			Wire(root, string.Format(buttonFmt, i), string.Format(targetFmt, i));
	}

	static int Count()
	{
		return State().m_Buttons.Count();
	}

	static void Start()
	{
		GetGame().GetCallqueue().Remove(Tick);	// never double-schedule across a GM re-entry.
		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	static void Clear()
	{
		GetGame().GetCallqueue().Remove(Tick);
		Restore();
		State().m_Buttons.Clear();
		State().m_Labels.Clear();
		State().m_IsBg.Clear();
	}

	protected static void Restore()
	{
		// Restore ONLY when the widget still shows the colour hover wrote.
		if (s_Current && s_Current.GetColorInt() == s_SetColor)
			s_Current.SetColorInt(s_PrevColor);
		s_Current = null;
	}

	protected static void Tick()
	{
		if (State().m_Buttons.IsEmpty())
			return;

		Widget hit = WidgetManager.GetWidgetUnderCursor();
		Widget label;
		bool  bgMode = false;

		// Climb from the widget under the cursor to the nearest registered button.
		int hops = 0;
		while (hit && hops < MAX_CLIMB)
		{
			int idx = State().m_Buttons.Find(hit);
			if (idx != -1)
			{
				label  = State().m_Labels[idx];
				bgMode = State().m_IsBg[idx];
				break;
			}
			hit = hit.GetParent();
			hops++;
		}

		if (label == s_Current)
			return;	// nothing changed - no work, no flicker.

		Restore();

		if (label)
		{
			s_PrevColor = label.GetColorInt();
			if (bgMode)
				s_SetColor = Plate(s_PrevColor);
			else
				s_SetColor = Lift(s_PrevColor);
			label.SetColorInt(s_SetColor);
			s_Current = label;
		}
	}

	// Hover colour for a background plate: blend the plate's CURRENT colour 35% toward the live accent, preserving its alpha.
	protected static const float PLATE_PULL = 0.35;

	protected static int Plate(int prev)
	{
		Color accent = DCO_GMTheme.Get().m_AccentColor;
		if (!accent)
			return prev;

		int a = prev & 0xFF000000;
		int r = (prev >> 16) & 0xFF;
		int g = (prev >> 8) & 0xFF;
		int b = prev & 0xFF;

		int ar = Math.Round(accent.R() * 255);
		int ag = Math.Round(accent.G() * 255);
		int ab = Math.Round(accent.B() * 255);

		r = Clamp8(Math.Round(r + (ar - r) * PLATE_PULL));
		g = Clamp8(Math.Round(g + (ag - g) * PLATE_PULL));
		b = Clamp8(Math.Round(b + (ab - b) * PLATE_PULL));

		return a | (r << 16) | (g << 8) | b;
	}

	// Blend a packed ARGB colour toward the live accent, preserving alpha.
	protected static int Lift(int packed)
	{
		Color accent = DCO_GMTheme.Get().m_AccentColor;
		if (!accent)
			return packed;

		int a = packed & 0xFF000000;
		int r = (packed >> 16) & 0xFF;
		int g = (packed >> 8) & 0xFF;
		int b = packed & 0xFF;

		int ar = Math.Round(accent.R() * 255);
		int ag = Math.Round(accent.G() * 255);
		int ab = Math.Round(accent.B() * 255);

		r = Math.Round(r + (ar - r) * ACCENT_PULL);
		g = Math.Round(g + (ag - g) * ACCENT_PULL);
		b = Math.Round(b + (ab - b) * ACCENT_PULL);

		r = Clamp8(Math.Round(r + (255 - r) * WHITE_LIFT));
		g = Clamp8(Math.Round(g + (255 - g) * WHITE_LIFT));
		b = Clamp8(Math.Round(b + (255 - b) * WHITE_LIFT));

		return a | (r << 16) | (g << 8) | b;
	}

	// Integer clamp.
	protected static int Clamp8(int v)
	{
		if (v < 0)
			return 0;
		if (v > 255)
			return 255;
		return v;
	}
}
