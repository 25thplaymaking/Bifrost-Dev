// Central theme for the Bifrost GM GM UI.
class DCO_GMTheme
{
	protected static ref DCO_GMTheme s_Instance;

	// Client-side per-user UI preferences.
	static const string PROFILE_PATH = "$profile:DCO_GMTheme.json";

	// Command-readout typography.
	static const int FONT_COMPACT = 0;
	static const int FONT_COMMAND = 1;
	static const ResourceName FONT_FACE_COMPACT = "{3E7733BAC8C831F6}UI/Fonts/RobotoCondensed/RobotoCondensed_Regular.fnt";
	static const ResourceName FONT_FACE_COMMAND = "{CD2634D279AB011A}UI/Fonts/Roboto/Roboto_Bold.fnt";
	static const ref array<string> COMMAND_FONT_WIDGETS = {
		"DCO_ClockMode", "DCO_ClockText", "DCO_CompassHeading", "DCO_CompassTick0", "DCO_CompassTick1",
		"DCO_CompassText", "DCO_CompassTick3", "DCO_CompassTick4", "DCO_TopSelection", "DCO_TopWorldState",
		"DCO_BottomBrand", "DCO_BottomStatus"
	};

	// Persisted HUD element switches.
	static const int UI_OVERLAYS = 0;
	static const int UI_ORDERS = 1;
	static const int UI_NOTIFICATIONS = 2;
	static const int UI_CHAT = 3;
	static const int UI_GIZMO = 4;
	static const int UI_NAMETAGS = 5;
	static const int UI_TOPBAR = 6;
	static const int UI_BOTTOMBAR = 7;
	static const int UI_EDIT = 8;
	static const int UI_CREATE = 9;
	static const int UI_TACTICS = 10;
	static const int UI_INFO = 11;
	static const int UI_ELEMENT_COUNT = 12;
	protected ref array<bool> m_ElementEnabled = {};	// live/session visibility.
	protected ref array<bool> m_ElementPersisted = {};	// last deliberate OPTIONS preference; session-only toggles never overwrite it.

	// The bottom bar carries the OPTIONS button, so hiding it would strand every switch.
	static const string REOPEN_CHIP = "DCO_LayoutChip";

	static const ref array<string> MASTER_EXTRA = {
		"DCO_ScenarioPanel", "DCO_OptionsPanel", "DCO_SimPanel", "DCO_ArsenalScreen",
		"DCO_ContextMenu", "DCO_MenuBackdrop", "DCO_HoverPreview", "DCO_LayoutChip"
	};

	protected bool m_bMasterHidden;
	protected ref map<string, bool> m_MasterStash = new map<string, bool>();	// widget name -> visibility before the hide.
	protected bool m_bCueCones;	// world-cue states parked for the duration of a master-hide.
	protected bool m_bCueMovement;
	protected bool m_bCueMarkers;

	ref Color m_PanelColor;
	ref Color m_AccentColor;
	ref Color m_TextColor;
	ref Color m_HeaderColor;
	ref Color m_DividerColor;	// thin amber dividers.
	ref Color m_LabelColor;
	ref Color m_MutedColor;
	ref Color m_DisabledColor;	// disabled / off-state text.
	ref Color m_TrackColor;
	float m_PanelOpacity;
	float m_AccentHue;
	int m_DisplayFontMode;	// compact condensed or heavier command readouts - PERSISTED.

	// The accent's fixed saturation/brightness.
	static const float ACCENT_S = 0.80;
	static const float ACCENT_V = 0.85;
	static const float ACCENT_DEF_HUE = 32;	// default amber hue.
	static const int   ACCENT_DEF_R = 217;
	static const int   ACCENT_DEF_G = 137;
	static const int   ACCENT_DEF_B = 43;

	// Semantic status colours - NOT the accent, never touched by the hue recolor sweep.
	static const int SEM_HOSTILE  = 0xFFE2483C;	// enemy / danger red.
	static const int SEM_FRIENDLY = 0xFF3FBF6A;	// friendly / confirm green.
	static const int SEM_PLAYER   = 0xFF3FB6E6;	// player cyan.
	static const int SEM_AXIS_X   = 0xFFE0402A;
	static const int SEM_AXIS_Y   = 0xFF40E020;
	static const int SEM_AXIS_Z   = 0xFF3060FF;

	static const int FS_COLUMN = 18;
	static const int FS_TITLE = 14;
	static const int FS_HEAD  = 13;
	static const int FS_BODY  = 12;
	static const int FS_SMALL = 11;

	// Panel-opacity floor.
	static const float OPACITY_MIN = 0.15;

	static const ref array<string> PANEL_ROOTS = {
		"DCO_OverlayBar", "DCO_OrdersBox", "DCO_ScenarioPanel", "DCO_ContextMenu", "DCO_OptionsPanel",
		"DCO_EditTree", "DCO_CreateBrowser", "DCO_TopBar", "DCO_BottomBar", "DCO_SimPanel",
		"DCO_GizmoPanel", "DCO_NotifPanel", "DCO_ChatPanel", "DCO_TacticsPanel"
	};

	static const ref array<string> PANEL_BORDERS = {
		"DCO_OverlayBorder", "DCO_OrdersBorder", "DCO_ScenarioBorder", "DCO_MenuBorder", "DCO_OptionsBorder",
		"DCO_EditBorder", "DCO_CreateBorder", "DCO_TopBarBorder", "DCO_BottomBarBorder", "DCO_SimBorder",
		"DCO_GizmoBorder", "DCO_NotifBorder", "DCO_ChatBorder", "DCO_TacticsBorder"
	};
	static const float BORDER_EASE_HI = 1.0;
	static const float BORDER_EASE_LO = 0.8;

	static const ref array<string> GEOM_PANELS = {"DCO_OrdersBox", "DCO_OverlayBar", "DCO_OptionsPanel", "DCO_GizmoPanel",
		"DCO_NotifPanel", "DCO_ChatPanel", "DCO_TacticsPanel"};
	protected ref map<string, vector> m_GeomPos = new map<string, vector>();
	protected ref map<string, vector> m_GeomSize = new map<string, vector>();

	protected ref array<Widget> m_AccentWidgets;
	protected Widget m_CachedRoot;	// the root the cache was built against; differs after a GM re-mount => rebuild.

	ref ScriptInvoker OnThemeChanged = new ScriptInvoker();

	static DCO_GMTheme Get()
	{
		if (!s_Instance)
		{
			s_Instance = new DCO_GMTheme();
			s_Instance.LoadDefaults();
		}
		return s_Instance;
	}

	void LoadDefaults()
	{
		m_PanelColor   = Color.FromRGBA(11, 13, 16, 224);	// ~0.043 0.050 0.063 @ 0.88.
		m_TextColor    = Color.FromRGBA(230, 237, 242, 255);
		m_HeaderColor  = Color.FromRGBA(ACCENT_DEF_R, ACCENT_DEF_G, ACCENT_DEF_B, 255);
		m_DividerColor = Color.FromRGBA(ACCENT_DEF_R, ACCENT_DEF_G, ACCENT_DEF_B, 77);	// amber @ ~0.30.
		m_PanelOpacity = 1.0;
		m_AccentHue    = ACCENT_DEF_HUE;
		m_DisplayFontMode = FONT_COMPACT;
		m_AccentColor  = Color.FromRGBA(ACCENT_DEF_R, ACCENT_DEF_G, ACCENT_DEF_B, 255);	// #D9892B amber.
		m_LabelColor    = Color.FromRGBA(204, 214, 224, 255);	// secondary/interactive label grey.
		m_MutedColor    = Color.FromRGBA(168, 178, 189, 255);	// muted/inactive grey.
		m_DisabledColor = Color.FromRGBA(108, 118, 129, 255);	// disabled/off grey.
		m_TrackColor    = Color.FromRGBA(46, 51, 58, 255);

		m_ElementEnabled.Clear();
		m_ElementPersisted.Clear();
		for (int i = 0; i < UI_ELEMENT_COUNT; i++)
		{
			m_ElementEnabled.Insert(true);
			m_ElementPersisted.Insert(true);
		}

		LoadProfile();	// override opacity + accent hue from $profile if the GM saved a theme before.
	}

	protected void LoadProfile()
	{
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(PROFILE_PATH))
			return;	// no saved theme yet - keep defaults.
		ctx.ReadValue("opacity", m_PanelOpacity);
		ctx.ReadValue("accentHue", m_AccentHue);
		ctx.ReadValue("displayFontMode", m_DisplayFontMode);
		for (int vi = 0; vi < UI_ELEMENT_COUNT; vi++)
		{
			bool enabled = true;
			if (ctx.ReadValue("hudVisible_" + vi.ToString(), enabled))
			{
				m_ElementEnabled[vi] = enabled;
				m_ElementPersisted[vi] = enabled;
			}
		}
		foreach (string pn : GEOM_PANELS)
		{
			float gx, gy, gw, gh;
			if (ctx.ReadValue("geomX_" + pn, gx) && ctx.ReadValue("geomY_" + pn, gy)
				&& ctx.ReadValue("geomW_" + pn, gw) && ctx.ReadValue("geomH_" + pn, gh))
			{
				m_GeomPos.Set(pn, Vector(gx, gy, 0));
				m_GeomSize.Set(pn, Vector(gw, gh, 0));
			}
		}
		m_PanelOpacity = Math.Clamp(m_PanelOpacity, OPACITY_MIN, 1.0);
		m_DisplayFontMode = Math.Clamp(m_DisplayFontMode, FONT_COMPACT, FONT_COMMAND);
		m_AccentColor  = AccentFromHue(m_AccentHue);	// derive the live colour from the saved hue.
	}

	protected void SaveDeferred()
	{
		GetGame().GetCallqueue().Remove(SaveNow);
		GetGame().GetCallqueue().CallLater(SaveNow, 500);
	}

	void SaveNow()
	{
		JsonSaveContext ctx = new JsonSaveContext();
		ctx.WriteValue("opacity", m_PanelOpacity);
		ctx.WriteValue("accentHue", m_AccentHue);
		ctx.WriteValue("displayFontMode", m_DisplayFontMode);
		// Persist the last deliberate OPTIONS preference, never a live session-only tab/programmatic override.
		for (int vi = 0; vi < UI_ELEMENT_COUNT; vi++)
			ctx.WriteValue("hudVisible_" + vi.ToString(), m_ElementPersisted[vi]);
		foreach (string pn : GEOM_PANELS)
		{
			vector p;
			vector s;
			if (!m_GeomPos.Find(pn, p) || !m_GeomSize.Find(pn, s))
				continue;
			ctx.WriteValue("geomX_" + pn, p[0]);
			ctx.WriteValue("geomY_" + pn, p[1]);
			ctx.WriteValue("geomW_" + pn, s[0]);
			ctx.WriteValue("geomH_" + pn, s[1]);
		}
		ctx.SaveToFile(PROFILE_PATH);
	}

	void SetPanelGeom(string name, float x, float y, float w, float h)
	{
		if (!GEOM_PANELS.Contains(name))
			return;
		m_GeomPos.Set(name, Vector(x, y, 0));
		m_GeomSize.Set(name, Vector(w, h, 0));
		SaveDeferred();
	}

	// Fetch a saved geometry; false when the GM never customized this panel.
	bool GetPanelGeom(string name, out float x, out float y, out float w, out float h)
	{
		vector p;
		vector s;
		if (!m_GeomPos.Find(name, p) || !m_GeomSize.Find(name, s))
			return false;
		x = p[0];
		y = p[1];
		w = s[0];
		h = s[1];
		return true;
	}

	void ClearPanelGeoms()
	{
		m_GeomPos.Clear();
		m_GeomSize.Clear();
		SaveNow();
	}

	static void ClampPanelToViewport(Widget panel)
	{
		if (!panel)
			return;
		Widget root = panel.GetParent();
		if (!root)
			return;
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return;

		float vx, vy, vw, vh;
		root.GetScreenPos(vx, vy);
		root.GetScreenSize(vw, vh);
		if (vw <= 0 || vh <= 0)
			return;	// shell not laid out - never "correct" against a degenerate viewport.

		float px, py, pw, ph;
		panel.GetScreenPos(px, py);
		panel.GetScreenSize(pw, ph);
		if (pw <= 0 || ph <= 0)
			return;

		float refVx = ws.DPIUnscale(vx);
		float refVy = ws.DPIUnscale(vy);
		float refVw = ws.DPIUnscale(vw);
		float refVh = ws.DPIUnscale(vh);
		float refPx = ws.DPIUnscale(px);
		float refPy = ws.DPIUnscale(py);
		float refPw = ws.DPIUnscale(pw);

		float margin = 48;
		float dx = 0;
		float dy = 0;
		if (refPx + refPw < refVx + margin)
			dx = (refVx + margin) - (refPx + refPw);	// off the left: bring the panel's right edge back in.
		else if (refPx > refVx + refVw - margin)
			dx = (refVx + refVw - margin) - refPx;	// off the right: bring the left edge back in.
		if (refPy < refVy)
			dy = refVy - refPy;	// the grip is the TOP edge: never above the shell.
		else if (refPy > refVy + refVh - margin)
			dy = (refVy + refVh - margin) - refPy;	// off the bottom: keep the grip reachable.

		if (dx == 0 && dy == 0)
			return;

		vector p = FrameSlot.GetPos(panel);
		FrameSlot.SetPos(panel, p[0] + dx, p[1] + dy);
	}

	// HUD element visibility.
	bool IsElementEnabled(int index)
	{
		if (index < 0 || index >= m_ElementEnabled.Count())
			return true;
		return m_ElementEnabled[index];
	}

	void SetElementEnabled(int index, bool enabled, Widget root, bool persist = true)
	{
		if (index < 0 || index >= m_ElementEnabled.Count())
			return;
		m_ElementEnabled[index] = enabled;
		ApplyElementVisibility(root, index);
		if (persist)
		{
			m_ElementPersisted[index] = enabled;
			SaveDeferred();
		}
	}

	void ResetElementVisibility(Widget root)
	{
		for (int i = 0; i < UI_ELEMENT_COUNT; i++)
		{
			m_ElementEnabled[i] = true;
			m_ElementPersisted[i] = true;
		}
		ApplyElementVisibility(root);
		SaveNow();
	}

	void ApplyElementVisibility(Widget root, int onlyIndex = -1)
	{
		if (!root)
			return;
		for (int i = 0; i < UI_ELEMENT_COUNT; i++)
		{
			if (onlyIndex >= 0 && i != onlyIndex)
				continue;
			if (i == UI_EDIT || i == UI_CREATE)
			{
				DCO_GMUIController.ApplyColumnVisible(i == UI_EDIT, m_ElementEnabled[i]);
				continue;
			}
			if (i == UI_TACTICS && !m_ElementEnabled[i])
				DCO_GMTacticsPanel.Get().CloseSilent();	// stop its hidden poll/state, not just its pixels.
			Widget w = root.FindAnyWidget(ElementWidget(i));
			if (!w)
				continue;
			if (!m_ElementEnabled[i])
				w.SetVisible(false);
			else if (!IsGateOnly(i))
				w.SetVisible(true);
		}

		if (onlyIndex < 0 || onlyIndex == UI_BOTTOMBAR)
			ApplyReopenChip(root);
	}

	// The widget each switch owns.
	static string ElementWidget(int index)
	{
		switch (index)
		{
			case UI_OVERLAYS:      return "DCO_OverlayBar";
			case UI_ORDERS:        return "DCO_OrdersBox";
			case UI_NOTIFICATIONS: return "DCO_NotifPanel";
			case UI_CHAT:          return "DCO_ChatPanel";
			case UI_GIZMO:         return "DCO_GizmoPanel";
			case UI_NAMETAGS:      return "DCO_NametagLayer";
			case UI_TOPBAR:        return "DCO_TopBar";
			case UI_BOTTOMBAR:     return "DCO_BottomBar";
			case UI_EDIT:          return "DCO_EditTree";
			case UI_CREATE:        return "DCO_CreateBrowser";
			case UI_TACTICS:       return "DCO_TacticsPanel";
			case UI_INFO:          return "DCO_TutInfoBox";
		}
		return string.Empty;
	}

	// Reverse of ElementWidget: the switch index owning a panel widget, or -1 for a widget outside the visibility system.
	static int ElementIndexFor(string widgetName)
	{
		for (int i = 0; i < UI_ELEMENT_COUNT; i++)
		{
			if (ElementWidget(i) == widgetName)
				return i;
		}
		return -1;
	}

	// Panels their owner only raises when it has something to show.
	static bool IsGateOnly(int index)
	{
		return index == UI_GIZMO || index == UI_TACTICS;
	}

	protected void ApplyReopenChip(Widget root)
	{
		Widget chip = root.FindAnyWidget(REOPEN_CHIP);
		if (chip)
			chip.SetVisible(!m_ElementEnabled[UI_BOTTOMBAR] && !m_bMasterHidden);
	}

	bool IsMasterHidden()
	{
		return m_bMasterHidden;
	}

	// Cinematic master-hide.
	void ToggleMasterHide(Widget root)
	{
		SetMasterHidden(!m_bMasterHidden, root);
	}

	void SetMasterHidden(bool hidden, Widget root)
	{
		if (!root || hidden == m_bMasterHidden)
			return;
		m_bMasterHidden = hidden;

		if (hidden)
		{
			m_MasterStash.Clear();
			for (int i = 0; i < UI_ELEMENT_COUNT; i++)
				StashHide(root, ElementWidget(i));
			foreach (string extra : MASTER_EXTRA)
				StashHide(root, extra);
			StashCues();
			DCO_GMTutorial.SetHidden(true);	// takes the INFO chip and parks an open tutorial for exact restore.
			root.SetVisible(false);	// parent gate: owner polls cannot leak a child panel back onto a cinematic screen.
			Print("[DCO-GM] cinematic master-hide ON", LogLevel.NORMAL);
			return;
		}

		root.SetVisible(true);
		DCO_GMTutorial.SetHidden(false);	// first: it restores its internal open state; the stash has the last word.
		RestoreCues();
		foreach (string name, bool wasVisible : m_MasterStash)
		{
			Widget w = root.FindAnyWidget(name);
			if (w)
				w.SetVisible(wasVisible);
		}
		m_MasterStash.Clear();
		Print("[DCO-GM] cinematic master-hide OFF (layout restored)", LogLevel.NORMAL);
	}

	// Drop the master-hide state without touching widgets - for a shell teardown, whose widgets are about to die.
	void ClearMasterHide()
	{
		if (m_bMasterHidden)
			RestoreCues();
		m_bMasterHidden = false;
		m_MasterStash.Clear();
	}

	protected void StashHide(Widget root, string name)
	{
		if (name.IsEmpty())
			return;
		Widget w = root.FindAnyWidget(name);
		if (!w)
			return;
		m_MasterStash.Set(name, w.IsVisible());
		w.SetVisible(false);
	}

	protected void StashCues()
	{
		DCO_GMOverlayState cues = DCO_GMOverlayState.Get();
		m_bCueCones    = cues.GetEnabled(DCO_GMOverlayState.OV_CONES);
		m_bCueMovement = cues.GetEnabled(DCO_GMOverlayState.OV_MOVEMENT);
		m_bCueMarkers  = cues.GetEnabled(DCO_GMOverlayState.OV_MARKERS);
		cues.SetEnabled(DCO_GMOverlayState.OV_CONES, false);
		cues.SetEnabled(DCO_GMOverlayState.OV_MOVEMENT, false);
		cues.SetEnabled(DCO_GMOverlayState.OV_MARKERS, false);
	}

	protected void RestoreCues()
	{
		DCO_GMOverlayState cues = DCO_GMOverlayState.Get();
		cues.SetEnabled(DCO_GMOverlayState.OV_CONES, m_bCueCones);
		cues.SetEnabled(DCO_GMOverlayState.OV_MOVEMENT, m_bCueMovement);
		cues.SetEnabled(DCO_GMOverlayState.OV_MARKERS, m_bCueMarkers);
	}

	static Color AccentFromHue(float hue)
	{
		return ColorFromHSV(hue, ACCENT_S, ACCENT_V);
	}

	// Standard HSV->RGB.
	static Color ColorFromHSV(float h, float s, float v)
	{
		while (h >= 360.0) h -= 360.0;
		while (h < 0.0)    h += 360.0;
		float c  = v * s;
		float hp = h / 60.0;
		int sector = hp;	// truncate 0..5.
		int half   = hp / 2.0;
		float hmod2 = hp - 2.0 * half;
		float t = hmod2 - 1.0;
		if (t < 0) t = -t;
		float x = c * (1.0 - t);
		float m = v - c;
		float r, g, b;
		switch (sector)
		{
			case 0: { r = c; g = x; b = 0; break; }
			case 1: { r = x; g = c; b = 0; break; }
			case 2: { r = 0; g = c; b = x; break; }
			case 3: { r = 0; g = x; b = c; break; }
			case 4: { r = x; g = 0; b = c; break; }
			default:{ r = c; g = 0; b = x; break; }	// 5.
		}
		return Color.FromRGBA((int)((r + m) * 255 + 0.5), (int)((g + m) * 255 + 0.5), (int)((b + m) * 255 + 0.5), 255);
	}

	void SetPanelOpacity(float v, Widget root)
	{
		m_PanelOpacity = Math.Clamp(v, OPACITY_MIN, 1.0);
		ApplyOpacity(root);
		SaveDeferred();
	}

	void ApplyOpacity(Widget root)
	{
		if (!root)
			return;
		foreach (string name : PANEL_ROOTS)
		{
			Widget w = root.FindAnyWidget(name);
			if (w)
				w.SetOpacity(m_PanelOpacity);
		}

		float borderFrac = Math.Clamp((m_PanelOpacity - BORDER_EASE_LO) / (BORDER_EASE_HI - BORDER_EASE_LO), 0.0, 1.0);
		foreach (string bname : PANEL_BORDERS)
		{
			Widget b = root.FindAnyWidget(bname);
			if (b)
				b.SetOpacity(borderFrac);
		}
	}

	void SetAccentHue(float hue, Widget root)
	{
		m_AccentHue = hue;
		m_AccentColor = AccentFromHue(hue);
		ApplyAccent(root);
		OnThemeChanged.Invoke();
		SaveDeferred();
	}

	void SetAccent(Color color, Widget root)
	{
		if (!color)
			return;
		m_AccentColor = color;
		ApplyAccent(root);
		OnThemeChanged.Invoke();
	}

	// Switch the command readouts between the dense default face and a heavier at-a-glance face.
	void SetDisplayFontMode(int mode, Widget root)
	{
		m_DisplayFontMode = Math.Clamp(mode, FONT_COMPACT, FONT_COMMAND);
		ApplyDisplayFont(root);
		SaveDeferred();
	}

	void ApplyDisplayFont(Widget root)
	{
		if (!root)
			return;
		ResourceName face = FONT_FACE_COMPACT;
		if (m_DisplayFontMode == FONT_COMMAND)
			face = FONT_FACE_COMMAND;
		foreach (string name : COMMAND_FONT_WIDGETS)
		{
			TextWidget text = TextWidget.Cast(root.FindAnyWidget(name));
			if (text)
				text.SetFont(face);
		}
	}

	void ApplyAccent(Widget root)
	{
		if (!root || !m_AccentColor)
			return;
		if (root != m_CachedRoot || !m_AccentWidgets)
		{
			m_AccentWidgets = {};
			CollectAccent(root.GetChildren());	// walk descendants of the GM-UI root only.
			m_CachedRoot = root;
		}
		int nr = Math.Round(m_AccentColor.R() * 255);
		int ng = Math.Round(m_AccentColor.G() * 255);
		int nb = Math.Round(m_AccentColor.B() * 255);
		int newRGB = (nr << 16) | (ng << 8) | nb;	// 0x00RRGGBB.
		foreach (Widget w : m_AccentWidgets)
		{
			if (!w)
				continue;
			int packed = w.GetColorInt();	// 0xAARRGGBB.
			int a = packed & 0xFF000000;	// keep this widget's own alpha.
			w.SetColorInt(a | newRGB);
		}
	}

	protected void CollectAccent(Widget w)
	{
		while (w)
		{
			if (w.GetName() == "DCO_OptSwatchRow" || w.GetName() == "DCO_OptFontRow")	// fixed/explicit option states.
			{
				w = w.GetSibling();
				continue;
			}
			bool isIcon = w.GetName().Contains("_Icon");
			int packed = w.GetColorInt();
			int wr = (packed >> 16) & 0xFF;
			int wg = (packed >> 8) & 0xFF;
			int wb = packed & 0xFF;
			if (!isIcon && ChannelNear(wr, ACCENT_DEF_R) && ChannelNear(wg, ACCENT_DEF_G) && ChannelNear(wb, ACCENT_DEF_B))
				m_AccentWidgets.Insert(w);
			CollectAccent(w.GetChildren());
			w = w.GetSibling();
		}
	}

	protected bool ChannelNear(int a, int b)
	{
		int d = a - b;
		if (d < 0)
			d = -d;
		return d <= 3;	// tolerance for the float->int round-trip of the layout colour.
	}

	void ApplyAccentTo(Widget root, string name)
	{
		if (!root)
			return;
		Widget w = root.FindAnyWidget(name);
		if (!w)
			return;
		int nr = Math.Round(m_AccentColor.R() * 255);
		int ng = Math.Round(m_AccentColor.G() * 255);
		int nb = Math.Round(m_AccentColor.B() * 255);
		int rgb = (nr << 16) | (ng << 8) | nb;
		int alpha = w.GetColorInt() & 0xFF000000;

		ImageWidget img = ImageWidget.Cast(w);
		if (img)
		{
			img.SetColorInt(alpha | rgb);
			return;
		}
		TextWidget txt = TextWidget.Cast(w);
		if (txt)
			txt.SetColorInt(alpha | rgb);
	}

	void ApplyTextTo(Widget root, string name)
	{
		if (!root)
			return;
		Widget w = root.FindAnyWidget(name);
		if (!w)
			return;
		TextWidget txt = TextWidget.Cast(w);
		if (txt)
			txt.SetColor(m_TextColor);
	}
}
