//! GRS Armory semantic theme bridged to Bifrost's persisted per-client palette.
//! The supplied interface structure and GRS identity remain intact while actionable accents
//! follow the same hue the user selected for the rest of Bifrost.
class GRSA_Theme
{
	protected static bool s_bStationSession;
	protected static ref Color s_StationAccent;
	protected static float s_fStationOpacity;
	protected static ref array<string> s_aPanelSurfaces;

	static void BeginStationSession(Color accent, float opacity)
	{
		s_bStationSession = accent != null;
		s_StationAccent = accent;
		s_fStationOpacity = Math.Clamp(opacity, DCO_GMTheme.OPACITY_MIN, 1.0);
	}

	static void BeginLocalSession()
	{
		s_bStationSession = false;
		s_StationAccent = null;
		s_fStationOpacity = 0.0;
	}

	static void EndSession()
	{
		BeginLocalSession();
	}

	static Color Accent()
	{
		if (s_bStationSession && s_StationAccent)
			return s_StationAccent;
		return DCO_GMTheme.Get().m_AccentColor;
	}

	static float PanelOpacity()
	{
		if (s_bStationSession)
			return s_fStationOpacity;
		return DCO_GMTheme.Get().m_PanelOpacity;
	}

	static Color AccentHover()
	{
		return Scale(Accent(), 1.12);
	}

	static Color AccentDeep()
	{
		return Scale(Accent(), 0.64);
	}

	static Color Separator()
	{
		return Accent();
	}

	static Color TextPrimary()
	{
		return DCO_GMTheme.Get().m_TextColor;
	}

	static Color TextHeader()
	{
		return DCO_GMTheme.Get().m_LabelColor;
	}

	static void Apply(Widget root)
	{
		ApplyAccentWidgets(root);
		ApplyPanelOpacity(root);
	}

	static void ApplyAccentWidgets(Widget root)
	{
		if (!root)
			return;
		ApplyAccentRecursive(root);
	}

	static Color MapLayoutAccent(Color source)
	{
		if (!source)
			return source;

		int red = Math.Round(source.R() * 255);
		int green = Math.Round(source.G() * 255);
		int blue = Math.Round(source.B() * 255);
		float scale;
		if (ChannelsNear(red, green, blue, 217, 137, 43))
			scale = 1.0;
		else if (ChannelsNear(red, green, blue, 82, 41, 9))
			scale = 0.38;
		else
			return source;

		Color accent = Accent();
		return Color.FromRGBA(
			Math.Clamp(Math.Round(accent.R() * 255 * scale), 0, 255),
			Math.Clamp(Math.Round(accent.G() * 255 * scale), 0, 255),
			Math.Clamp(Math.Round(accent.B() * 255 * scale), 0, 255),
			Math.Clamp(Math.Round(source.A() * 255), 0, 255));
	}

	static Color MapButtonAccent(Color source)
	{
		if (!source)
			return source;

		int red = Math.Round(source.R() * 255);
		int green = Math.Round(source.G() * 255);
		int blue = Math.Round(source.B() * 255);
		float scale;
		if (ChannelsNear(red, green, blue, 217, 137, 43))
			scale = 0.42;
		else if (ChannelsNear(red, green, blue, 82, 41, 9))
			scale = 0.25;
		else
			return source;

		Color accent = Accent();
		return Color.FromRGBA(
			Math.Clamp(Math.Round(accent.R() * 255 * scale), 0, 255),
			Math.Clamp(Math.Round(accent.G() * 255 * scale), 0, 255),
			Math.Clamp(Math.Round(accent.B() * 255 * scale), 0, 255),
			Math.Clamp(Math.Round(source.A() * 255), 0, 255));
	}

	protected static void ApplyAccentRecursive(Widget widget)
	{
		Widget cursor = widget;
		while (cursor)
		{
			int packed = cursor.GetColorInt();
			int red = (packed >> 16) & 0xFF;
			int green = (packed >> 8) & 0xFF;
			int blue = packed & 0xFF;
			float scale;
			if (ChannelsNear(red, green, blue, 217, 137, 43))
				scale = 1.0;
			else if (ChannelsNear(red, green, blue, 82, 41, 9))
				scale = 0.38;
			else
				scale = -1.0;

			if (scale >= 0.0)
			{
				Color accent = Accent();
				int accentRed = Math.Clamp(Math.Round(accent.R() * 255 * scale), 0, 255);
				int accentGreen = Math.Clamp(Math.Round(accent.G() * 255 * scale), 0, 255);
				int accentBlue = Math.Clamp(Math.Round(accent.B() * 255 * scale), 0, 255);
				cursor.SetColorInt((packed & 0xFF000000) | (accentRed << 16) | (accentGreen << 8) | accentBlue);
			}

			ApplyAccentRecursive(cursor.GetChildren());
			cursor = cursor.GetSibling();
		}
	}

	protected static void ApplyPanelOpacity(Widget root)
	{
		if (!s_aPanelSurfaces)
		{
			s_aPanelSurfaces = {
				"BackdropFill", "HeaderBg", "FooterBg", "LeftRailBg", "RightRailBg",
				"ItemListBg", "StatsBg", "ReceiverBg", "CandidatesBg", "KitListBg", "SettingsBg", "GunsmithLeavePanelBg"
			};
		}

		float opacity = PanelOpacity();
		foreach (string name : s_aPanelSurfaces)
		{
			Widget surface = root.FindAnyWidget(name);
			if (surface)
				surface.SetOpacity(opacity);
		}
	}

	protected static bool ChannelsNear(int red, int green, int blue, int expectedRed, int expectedGreen, int expectedBlue)
	{
		return Math.AbsInt(red - expectedRed) <= 3
			&& Math.AbsInt(green - expectedGreen) <= 3
			&& Math.AbsInt(blue - expectedBlue) <= 3;
	}

	protected static Color Scale(Color source, float scale)
	{
		if (!source)
			return Color.FromRGBA(217, 137, 43, 255);
		int red = Math.Clamp(Math.Round(source.R() * 255 * scale), 0, 255);
		int green = Math.Clamp(Math.Round(source.G() * 255 * scale), 0, 255);
		int blue = Math.Clamp(Math.Round(source.B() * 255 * scale), 0, 255);
		return Color.FromRGBA(red, green, blue, 255);
	}
}

//! Keeps every Arsenal text button's interaction states on the active station palette, including
//! buttons created later when a list or tab is populated.
class GRSA_ThemedButtonTextComponent : SCR_ButtonTextComponent
{
	override void HandlerAttached(Widget w)
	{
		m_BackgroundDefault = GRSA_Theme.MapButtonAccent(m_BackgroundDefault);
		m_BackgroundHovered = GRSA_Theme.MapButtonAccent(m_BackgroundHovered);
		m_BackgroundSelected = GRSA_Theme.MapButtonAccent(m_BackgroundSelected);
		m_BackgroundSelectedHovered = GRSA_Theme.MapButtonAccent(m_BackgroundSelectedHovered);
		m_BackgroundClicked = GRSA_Theme.MapButtonAccent(m_BackgroundClicked);
		super.HandlerAttached(w);
		GRSA_Theme.ApplyAccentWidgets(w);
	}
}
