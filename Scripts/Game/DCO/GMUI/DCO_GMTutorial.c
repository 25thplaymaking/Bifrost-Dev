// Bifrost GM quick-tutorial overlay plus the bare top-right INFO button that opens it.

// One documented control: the control's name and one line of what it does.
class DCO_TutorialRow
{
	string m_sKey;
	string m_sText;

	void DCO_TutorialRow(string key, string text)
	{
		m_sKey = key;
		m_sText = text;
	}
}

// One tutorial section: a tab title and the rows shown while that tab is picked.
class DCO_TutorialSection
{
	string m_sTitle;
	ref array<ref DCO_TutorialRow> m_aRows = {};

	void DCO_TutorialSection(string title)
	{
		m_sTitle = title;
	}

	void Add(string key, string text)
	{
		m_aRows.Insert(new DCO_TutorialRow(key, text));
	}
}

// One per button; carries its id so OnClick maps straight to the action.
class DCO_TutorialButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMTutorial m_Owner;
	protected int m_Id;

	void DCO_TutorialButtonHandler(DCO_GMTutorial owner, int id)
	{
		m_Owner = owner;
		m_Id = id;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnButton(m_Id);
		return false;
	}
}

class DCO_GMTutorial
{
	protected static ref DCO_GMTutorial s_Inst;

	static DCO_GMTutorial Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_GMTutorial();
		return s_Inst;
	}

	static const int SECTION_COUNT = 6;
	static const int ROW_POOL      = 18;	// layout pool size; the longest section uses 17, the rest hide.

	protected static const int BTN_INFO     = 0;
	protected static const int BTN_BACKDROP = 1;
	protected static const int BTN_SEC_BASE = 10;

	// layout GUID wired after Workbench registration.
	protected static const ResourceName TUT_LAYOUT = "{F0A67D908EFBCDBA}UI/layouts/DCO_GMTutorial.layout";

	protected static const float PANEL_W  = 1180;
	protected static const float PANEL_H  = 620;
	protected static const float CHIP_S   = 28;
	protected static const float CHIP_X   = -10;
	protected static const float CHIP_Y   = 10;

	protected Widget m_wRoot;	// our own layout root, parented under the GM shell root.
	protected Widget m_wOverlay;
	protected Widget m_wInfoBox;	// the always-on INFO chip.
	protected TextWidget m_wSecTitle;
	protected ref array<TextWidget> m_SecLabels = {};
	protected ref array<ImageWidget> m_SecIcons = {};
	protected ref array<Widget> m_Rows = {};
	protected ref array<TextWidget> m_RowKeys = {};
	protected ref array<TextWidget> m_RowTexts = {};
	protected ref array<ref DCO_TutorialButtonHandler> m_Handlers = {};
	protected ref array<ref DCO_TutorialSection> m_Sections;

	protected int m_iSection;
	protected bool m_bOpen;
	protected bool m_bHidden;
	protected bool m_bOpenBeforeHidden;	// exact-layout restore: remember whether the tutorial itself was open.

	// Toggle the overlay.
	static void Toggle()
	{
		if (s_Inst)
			s_Inst.SetOpen(!s_Inst.m_bOpen);
	}

	static bool IsOpen()
	{
		return s_Inst != null && s_Inst.m_bOpen;
	}

	static void Close()
	{
		if (s_Inst)
			s_Inst.SetOpen(false);
	}

	// Master-hide hook: the cinematic hide must take the INFO chip with it and never leave the overlay up.
	static void SetHidden(bool hidden)
	{
		if (!s_Inst)
			return;
		if (hidden == s_Inst.m_bHidden)
			return;
		if (hidden)
		{
			s_Inst.m_bOpenBeforeHidden = s_Inst.m_bOpen;
			s_Inst.m_bHidden = true;
			s_Inst.SetOpen(false);
			if (s_Inst.m_wInfoBox)
				s_Inst.m_wInfoBox.SetVisible(false);
			return;
		}

		s_Inst.m_bHidden = false;
		if (s_Inst.m_wInfoBox)
			s_Inst.m_wInfoBox.SetVisible(true);
		if (s_Inst.m_bOpenBeforeHidden)
			s_Inst.SetOpen(true);
		s_Inst.m_bOpenBeforeHidden = false;
	}

	void Init(Widget shellRoot)
	{
		if (!shellRoot)
			return;
		BuildContent();
		Shutdown();	// this singleton outlives the shell; a rebuild must not stack a second widget tree on it.

		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return;
		m_wRoot = ws.CreateWidgets(TUT_LAYOUT, shellRoot);
		if (!m_wRoot)
		{
			Print("[DCO-GM] tutorial layout failed to instantiate - verify the registered layout resource", LogLevel.WARNING);
			return;
		}
		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);

		m_wOverlay  = m_wRoot.FindAnyWidget("DCO_TutOverlay");
		m_wInfoBox  = m_wRoot.FindAnyWidget("DCO_TutInfoBox");
		m_wSecTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_TutSecTitle"));

		int buttons = 0;
		int icons = 0;
		buttons += Bind("DCO_TutInfoBtn", BTN_INFO);
		buttons += Bind("DCO_TutBackdrop", BTN_BACKDROP);
		if (m_wRoot.FindAnyWidget("DCO_TutInfoIcon"))
			icons++;
		for (int s = 0; s < SECTION_COUNT; s++)
		{
			buttons += Bind("DCO_TutSec" + s.ToString(), BTN_SEC_BASE + s);
			m_SecLabels.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_TutSec" + s.ToString() + "_Label")));
			ImageWidget ico = ImageWidget.Cast(m_wRoot.FindAnyWidget("DCO_TutSec" + s.ToString() + "_Icon"));
			m_SecIcons.Insert(ico);
			if (ico)
				icons++;
		}

		int rows = 0;
		for (int r = 0; r < ROW_POOL; r++)
		{
			Widget row = m_wRoot.FindAnyWidget("DCO_TutRow" + r.ToString());
			m_Rows.Insert(row);
			m_RowKeys.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_TutRow" + r.ToString() + "_Key")));
			m_RowTexts.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_TutRow" + r.ToString() + "_Txt")));
			if (row)
				rows++;
		}

		PinBox("DCO_TutPanel",  0.5, 0.5, 0.5, 0.5, PANEL_W, PANEL_H, 0, 0);
		PinBox("DCO_TutInfoBox", 1, 0,     1,   0,   CHIP_S,  CHIP_S, CHIP_X, CHIP_Y);

		m_iSection = 0;
		m_bOpen = false;
		if (m_wOverlay)
			m_wOverlay.SetVisible(false);	// closed = invisible = no handler in this subtree can fire.
		if (m_wInfoBox)
			m_wInfoBox.SetVisible(!m_bHidden);	// a rebuild under master-hide must not pop the chip back.
		ShowSection(0);

		Print(string.Format("[DCO-GM] tutorial bound (sections=%1/%2 rows=%3/%4 buttons=%5/%6 icons=%7/%8 overlay=%9)",
			m_Sections.Count(), SECTION_COUNT, rows, ROW_POOL, buttons, SECTION_COUNT + 2,
			icons, SECTION_COUNT + 1, m_wOverlay != null && m_wInfoBox != null), LogLevel.NORMAL);
	}

	void Shutdown()
	{
		m_bOpen = false;
		m_bHidden = false;
		m_bOpenBeforeHidden = false;
		ClearBindings();
		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();	// the shell root dies too, but never leave a second owner of it.
			m_wRoot = null;
		}
	}

	// Drop every cached widget + handler.
	protected void ClearBindings()
	{
		m_Handlers.Clear();
		m_SecLabels.Clear();
		m_SecIcons.Clear();
		m_Rows.Clear();
		m_RowKeys.Clear();
		m_RowTexts.Clear();
		m_wSecTitle = null;
		m_wOverlay = null;
		m_wInfoBox = null;
	}

	protected int Bind(string name, int id)
	{
		ButtonWidget b = ButtonWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!b)
			return 0;
		DCO_TutorialButtonHandler h = new DCO_TutorialButtonHandler(this, id);
		b.AddHandler(h);
		m_Handlers.Insert(h);
		return 1;
	}

	protected void PinBox(string name, float anchorX, float anchorY, float alignX, float alignY, float w, float h, float posX, float posY)
	{
		Widget wd = m_wRoot.FindAnyWidget(name);
		if (!wd)
			return;
		FrameSlot.SetAnchor(wd, anchorX, anchorY);
		FrameSlot.SetAlignment(wd, alignX, alignY);
		FrameSlot.SetSize(wd, w, h);
		FrameSlot.SetPos(wd, posX, posY);
	}

	bool OnButton(int id)
	{
		if (id == BTN_BACKDROP)
			return true;	// eat the click so the world never takes it while the overlay is up.
		if (id == BTN_INFO)
		{
			SetOpen(!m_bOpen);
			return true;
		}
		int sec = id - BTN_SEC_BASE;
		if (m_Sections && sec >= 0 && sec < m_Sections.Count())
		{
			ShowSection(sec);
			return true;
		}
		return false;
	}

	protected void SetOpen(bool open)
	{
		if (open && m_bHidden)
			return;	// master-hide wins: nothing reopens while the chrome is down.
		if (open == m_bOpen)
			return;
		m_bOpen = open;
		if (m_wOverlay)
			m_wOverlay.SetVisible(open);
		if (open)
		{
			ShowSection(m_iSection);
			return;
		}
		DCO_GMUIController.ReleaseMenuFocus();
	}

	// Paint one section: light its tab, retitle the right column, fill the row pool and hide the spares.
	protected void ShowSection(int index)
	{
		if (!m_Sections || index < 0 || index >= m_Sections.Count())
			return;
		m_iSection = index;

		DCO_GMTheme theme = DCO_GMTheme.Get();
		for (int s = 0; s < m_SecLabels.Count(); s++)
		{
			bool active = s == index;
			TextWidget lbl = m_SecLabels[s];
			if (lbl)
			{
				lbl.SetText(m_Sections[s].m_sTitle);
				if (active)
				{
					lbl.SetColor(theme.m_AccentColor);
					lbl.SetOpacity(1.0);
				}
				else
				{
					lbl.SetColor(theme.m_TextColor);
					lbl.SetOpacity(0.55);
				}
			}
			ImageWidget ico = m_SecIcons[s];
			if (!ico)
				continue;
			if (active)
			{
				ico.SetColor(theme.m_AccentColor);
				ico.SetOpacity(1.0);
			}
			else
			{
				ico.SetColor(theme.m_TextColor);
				ico.SetOpacity(0.55);
			}
		}

		DCO_TutorialSection section = m_Sections[index];
		if (m_wSecTitle)
			m_wSecTitle.SetText(section.m_sTitle);

		for (int r = 0; r < m_Rows.Count(); r++)
		{
			bool used = r < section.m_aRows.Count();
			Widget row = m_Rows[r];
			if (row)
				row.SetVisible(used);
			if (!used)
				continue;
			DCO_TutorialRow data = section.m_aRows[r];
			TextWidget key = m_RowKeys[r];
			if (key)
				key.SetText(data.m_sKey);
			TextWidget txt = m_RowTexts[r];
			if (txt)
				txt.SetText(data.m_sText);
		}

		if (section.m_aRows.Count() > m_Rows.Count())
			Print(string.Format("[DCO-GM] tutorial section %1 has %2 rows but the pool holds %3 - extras not shown",
				section.m_sTitle, section.m_aRows.Count(), m_Rows.Count()), LogLevel.WARNING);
	}

	protected DCO_TutorialSection AddSection(string title)
	{
		DCO_TutorialSection s = new DCO_TutorialSection(title);
		m_Sections.Insert(s);
		return s;
	}

	// The static control table.
	protected void BuildContent()
	{
		if (m_Sections)
			return;
		m_Sections = {};

		DCO_TutorialSection cam = AddSection("CAMERA & SELECTION");
		cam.Add("LMB on the world", "Select the entity under the cursor.");
		cam.Add("Drag on the world", "Box-select several entities. Disabled while PRECISE is on.");
		cam.Add("RMB on the world", "Open the Bifrost menu for the entity or the ground under the cursor.");
		cam.Add("SPACE", "Create player here. The vanilla action is still live - only its hint strip is hidden.");
		cam.Add("RMB ground: Create Player Here", "Pick a man from the list and spawn straight into him.");
		cam.Add("EDIT row (click)", "Select that entity. A row with children expands or collapses instead.");
		cam.Add("EDIT row (double-click)", "Open that entity's attributes.");
		cam.Add("EDIT row (RMB)", "Orient or follow the camera, take control, or open attributes.");
		cam.Add("PLAYERS row (double-click)", "Toggle the FPS watch on that player.");
		cam.Add("EDIT filter chips", "Narrow the tree to ALL, UNIT, VEH, OBJ, LOC or AREA.");
		cam.Add("EDIT page arrows", "Page the tree up and down.");
		cam.Add("EDIT tab (top bar)", "Show or hide the whole left column.");

		DCO_TutorialSection giz = AddSection("GIZMO & PRECISE");
		giz.Add("PRECISE (top bar)", "Toggle precise mode: vanilla free-drag off, gizmo handles on.");
		giz.Add("Entering PRECISE", "Pins the entity you are manipulating to the gizmo for the whole session.");
		giz.Add("Stray world clicks", "While pinned they never switch and never clear the gizmo target.");
		giz.Add("Box select in PRECISE", "Disabled - a marquee could only fight the pin.");
		giz.Add("EDIT list pick", "The one deliberate surface that re-pins the gizmo to another entity.");
		giz.Add("Drag the X / Y / Z arrow", "Move along that axis. Y is the height handle.");
		giz.Add("Drag a plane square", "Move in the XY, XZ or YZ plane.");
		giz.Add("MOVE / ROTATE", "Choose move arrows or rotation rings for the pinned object.");
		giz.Add("WORLD / LOCAL", "Axis basis: world axes, or the object's own axes.");
		giz.Add("SNAP", "Quantise the drag delta to that grid step, in metres.");
		giz.Add("SURFACE", "Drop a finished move onto the ground beneath it. Height drags are exempt.");
		giz.Add("SIM OPTIONS", "Open the per-object component toggles for the gizmo target.");
		giz.Add("SIM rows", "Flip Simulation, Damage, AI, Collision, Weapon or FX FIRE. N/A rows are dim and inert.");
		giz.Add("SIM stance buttons", "Force stand, crouch or prone on a character target.");
		giz.Add("Readout fields", "Type an exact position or rotation, then leave the field to commit it.");
		giz.Add("Readout nudges", "Step one position axis by the current snap value.");

		DCO_TutorialSection ord = AddSection("ORDERS & TACTICS");
		ord.Add("Select a group", "The ORDERS box fills in. Orders reach EVERY selected group, not just the leader.");
		ord.Add("STANCE", "Stand, Crouch, Prone, or Auto to release the GM stance lock.");
		ord.Add("FORMATION", "Wedge, Line, Column or Stagger.");
		ord.Add("BEHAVIOR", "Hold Fire or Resume Fire.");
		ord.Add("TACTICS", "Ambush Position, Kill-Zone, Defend Area, QRF Stage Area or Clear Building.");
		ord.Add("WAYPOINTS / OBJECTIVES / SPAWN", "The native command-bar commands, moved onto this box.");
		ord.Add("LMB after arming an order", "Places the armed zone, waypoint or command at the cursor.");
		ord.Add("RADIUS slider", "Resize the placed circle. The world circle tracks the drag.");
		ord.Add("RANGE slider", "QRF response range, or the ambush trigger range.");
		ord.Add("PAIR ID stepper", "Link an ambush position to its kill-zone.");
		ord.Add("SEND GROUP pill", "Send the ordering group to the zone. Off by default, committed on CLOSE.");
		ord.Add("SPRING NOW / RE-ARM", "Release an ambush now, or clear a kill-zone's one-shot latch.");
		ord.Add("RMB a group in the world", "The same order verbs, appended to the context menu.");

		DCO_TutorialSection ars = AddSection("ARSENAL");
		ars.Add("RMB character: Edit Loadout", "Open the Bifrost Arsenal on that unit.");
		ars.Add("RMB character name: Restock Ammo", "Refill every magazine the living unit still carries.");
		ars.Add("RMB character: Reset Loadout", "Restore the kit the unit had before editing, without opening the panel.");
		ars.Add("Left category strip", "Pick a gear category. Clicking an item equips it into that slot.");
		ars.Add("Right list", "Ammo and items for the active context. Clicking one adds it.");
		ars.Add("Bag icon on a right row", "Stock the carried inventory with that magazine.");
		ars.Add("Crate tab", "Switch the right column to EVERYTHING CARRIED.");
		ars.Add("Back / ALL ITEMS", "Leave the item context for the full pool, and back again.");
		ars.Add("Search box", "Filter the visible list as you type.");
		ars.Add("CLEAR CAT", "Empty the active category.");
		ars.Add("RESET", "Restore the kit from when editing began.");
		ars.Add("UNDO / REDO", "Step back and forward through the kit history.");
		ars.Add("LOADOUTS", "Open the saved-loadout panel: save, load, delete, page.");
		ars.Add("? (hint)", "Toggle the arsenal's own cheat sheet.");
		ars.Add("CLOSE", "Leave the arsenal. The target is held in place for the whole edit session.");

		DCO_TutorialSection fx = AddSection("FX & TRIGGERS");
		fx.Add("CREATE: FX tab", "The Bifrost effect emitters. Place one like any other entity.");
		fx.Add("Explosion emitter", "Ground warhead tiers, or Hydra M229 / S-5KO rocket delivery.");
		fx.Add("Air support emitter", "UH-1H flyby and gunrun passes over the point.");
		fx.Add("Audio emitter", "Plays a chosen sound bank event. It never damages anything.");
		fx.Add("Mortar emitter", "Real 81mm shells scattered in the spread ring - whistle first, then impact.");
		fx.Add("Tracer emitter", "Streams tracer rounds: round type, density, rate, burst length and pause.");
		fx.Add("LIVE vs COSMETIC", "LIVE detonates for real. COSMETIC is sound and particles only.");
		fx.Add("Emitter attributes", "Double-click the emitter, or Edit / Attributes, to size and arm it.");
		fx.Add("SIM OPTIONS FX row", "FIRE toggle for the selected emitter.");
		fx.Add("RMB ground: Place Trigger Here", "Drop a trigger circle at the cursor.");
		fx.Add("Trigger condition", "Who trips it: any character, players only, or one faction.");
		fx.Add("Trigger action", "Notify, send QRF, spring an ambush, spawn a group, or fire a paired emitter.");
		fx.Add("Trigger fire mode", "Once latches until Enabled is toggled. Repeat re-fires per cooldown.");
		fx.Add("RMB ground: Place Map Marker", "Drop a NATO map marker at the cursor.");

		DCO_TutorialSection hud = AddSection("HUD & LAYOUT");
		hud.Add("Options icon (bottom bar)", "Open the UI OPTIONS panel.");
		hud.Add("Panel opacity slider", "0 percent is solid. The maximum stops short of invisible on purpose.");
		hud.Add("Accent hue slider / swatches", "Recolour the whole UI live. Saved to your profile.");
		hud.Add("HUD switches (OPTIONS)", "Persistently show or hide OVERLAYS, ORDERS, NOTIFY, CHAT, GIZMO, NAMETAGS, TOPBAR, BOTBAR, EDIT, CREATE, TACTICS and INFO.");
		hud.Add("RESET UI (bottom bar)", "Re-pin every panel to its default place and size, and re-show both columns.");
		hud.Add("Top-left grip", "Drag a floating panel. Its position is saved per panel.");
		hud.Add("Bottom-right grip", "Resize a floating panel on both axes.");
		hud.Add("EDIT / CREATE tabs", "Toggle each side column on its own.");
		hud.Add("Gear cog (top bar)", "Open the inline scenario / global attributes panel.");
		hud.Add("Overlays box", "View Cones, Movement, Markers and Player FPS, each with its own scope.");
		hud.Add("Wheel over the CREATE list", "Scroll it three rows a notch.");
		hud.Add("CREATE tabs", "MEN, GRP, OBJ, SYS, FX. Click the live tab again for everything.");
		hud.Add("CREATE search box", "Filter the catalog as you type.");
		hud.Add("ENTER", "Opens the vanilla chat input. The Bifrost feed shows the log.");
		hud.Add("Toggle Interface (I default)", "Hide all vanilla and Bifrost chrome for cinematics; press again to restore the exact layout.");
		hud.Add("Recovery chip (bottom left)", "Appears when BOTBAR is hidden; click it to reopen UI OPTIONS.");
		hud.Add("INFO (top right)", "Opens and closes this tutorial. ESC closes it too.");
	}
}
