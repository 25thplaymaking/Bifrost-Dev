class DCO_GMMissionButton : ScriptedWidgetEventHandler
{
	DCO_GMMissionPanel m_Panel;
	int m_Action;
	void DCO_GMMissionButton(DCO_GMMissionPanel panel, int action) { m_Panel = panel; m_Action = action; }
	override bool OnClick(Widget w, int x, int y, int button) { return m_Panel.OnAction(m_Action); }
}

class DCO_GMMissionPanel
{
	protected static ref DCO_GMMissionPanel s_Instance;
	protected Widget m_Root;
	protected Widget m_Panel;
	protected TextWidget m_Heading;
	protected TextWidget m_Help;
	protected TextWidget m_Status;
	protected EditBoxWidget m_Title;
	protected MultilineEditBoxWidget m_Body;
	protected EditBoxWidget m_Value;
	protected ref array<ref DCO_GMMissionButton> m_Handlers = {};
	protected ref array<RplId> m_Targets = {};
	protected ref array<ref DCO_GMMarkerRecord> m_Positions = {};
	protected int m_Tool;
	protected int m_PendingTargetTool;
	protected int m_Scope;
	protected bool m_Include;
	protected bool m_Pending;
	protected int m_NamedIndex;
	protected int m_NamedId;
	protected int m_RequestSequence;
	protected vector m_Position;

	static DCO_GMMissionPanel Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMMissionPanel();
		return s_Instance;
	}
	void Init(Widget shell)
	{
		Shutdown();
		m_Root = GetGame().GetWorkspace().CreateWidgets("{DCA6090430000000}UI/layouts/DCO_GMMissionTools.layout", shell);
		if (!m_Root)
			return;
		FrameSlot.SetAnchorMin(m_Root, 0, 0);
		FrameSlot.SetAnchorMax(m_Root, 1, 1);
		FrameSlot.SetOffsets(m_Root, 0, 0, 0, 0);
		m_Root.SetZOrder(9850);
		m_Panel = m_Root.FindAnyWidget("DCO_MissionPanel");
		m_Heading = TextWidget.Cast(m_Root.FindAnyWidget("DCO_MissionHeading"));
		m_Help = TextWidget.Cast(m_Root.FindAnyWidget("DCO_MissionHelp"));
		m_Status = TextWidget.Cast(m_Root.FindAnyWidget("DCO_MissionStatus"));
		m_Title = EditBoxWidget.Cast(m_Root.FindAnyWidget("DCO_MissionTitle"));
		m_Body = MultilineEditBoxWidget.Cast(m_Root.FindAnyWidget("DCO_MissionBody"));
		m_Value = EditBoxWidget.Cast(m_Root.FindAnyWidget("DCO_MissionValue"));
		array<string> buttons = {"Close", "Apply", "Scope", "Include", "Previous", "Next"};
		for (int i = 0; i < buttons.Count(); i++)
		{
			Widget button = m_Root.FindAnyWidget("DCO_Mission" + buttons[i]);
			DCO_GMMissionButton handler = new DCO_GMMissionButton(this, i);
			if (button)
				button.AddHandler(handler);
			m_Handlers.Insert(handler);
		}
		DCO_GMMarkerService.Get().GetOnChanged().Insert(RefreshPositions);
		ApplyLayout();
		m_Root.SetVisible(false);
	}
	void WireHover()
	{
		array<string> buttons = {"Close", "Apply", "Scope", "Include", "Previous", "Next"};
		foreach (string button : buttons)
			DCO_GMHover.Wire(m_Root, "DCO_Mission" + button, "DCO_Mission" + button + "_Label");
	}
	void ApplyLayout()
	{
		if (!m_Panel || !m_Root)
			return;
		m_Root.Update();
		float width, height;
		m_Root.GetScreenSize(width, height);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		width = workspace.DPIUnscale(width);
		height = workspace.DPIUnscale(height);
		FrameSlot.SetAnchor(m_Panel, 0.5, 0.5);
		FrameSlot.SetAlignment(m_Panel, 0.5, 0.5);
		FrameSlot.SetSize(m_Panel, Math.Min(900, Math.Max(320, width - 40)), Math.Min(740, Math.Max(280, height - 40)));
		FrameSlot.SetPos(m_Panel, 0, 0);
		DCO_GMTheme.Get().ApplyAccent(m_Root);
		DCO_GMTheme.Get().ApplyOpacity(m_Root);
		DCO_GMTheme.Get().ApplyDisplayFont(m_Root);
	}
	void Shutdown()
	{
		m_RequestSequence++;
		m_Pending = false;
		m_PendingTargetTool = 0;
		DCO_GMMarkerService.Get().GetOnChanged().Remove(RefreshPositions);
		if (m_Root)
			m_Root.RemoveFromHierarchy();
		m_Root = null;
		m_Panel = null;
		m_Handlers.Clear();
		m_Targets.Clear();
		m_Positions.Clear();
		m_NamedId = 0;
	}
	bool IsOpen() { return m_Root && m_Root.IsVisible(); }
	bool CloseForBack()
	{
		if (m_PendingTargetTool > 0)
		{
			m_PendingTargetTool = 0;
			return true;
		}
		if (!m_Root || !m_Root.IsVisible())
			return false;
		m_Root.SetVisible(false);
		m_RequestSequence++;
		m_Pending = false;
		DCO_GMUIController.ReleaseMenuFocus();
		return true;
	}
	void Open(int tool, vector position, SCR_EditableEntityComponent fallback, bool targetOnly = false)
	{
		if (tool == DCO_GMMissionTool.HIDE)
		{
			CloseForBack();
			SCR_PlacingEditorComponent placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
			if (placing)
				placing.SetSelectedPrefab(DCO_PlacementCatalog.TERRAIN_AREA_RESOURCE);
			return;
		}
		m_PendingTargetTool = 0;
		if (!m_Root)
			return;
		DCO_GMCompositionPanel.Get().CloseForBack();
		DCO_GMMarkerPanel.Get().CloseForBack();
		m_Tool = tool;
		m_Position = position;
		m_Scope = 0;
		m_Include = false;
		m_Pending = false;
		m_NamedIndex = 0;
		m_NamedId = 0;
		m_RequestSequence++;
		m_Targets.Clear();
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		if (!targetOnly && !NeedsGround(tool))
			SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
		if (selected.IsEmpty() && fallback && !NeedsGround(tool))
			selected.Insert(fallback);
		foreach (SCR_EditableEntityComponent entity : selected)
		{
			RplId id;
			if (entity && entity.IsReplicated(id) && id.IsValid())
				m_Targets.Insert(id);
		}
		m_Title.SetText("");
		m_Body.SetText("");
		m_Value.SetText("1");
		m_Heading.SetText(DCO_GMMissionTool.Name(tool));
		Label("Title_Caption", "Name");
		Label("Title_Help", "Enter a short, recognizable name. Maximum 64 characters.");
		Label("Body_Caption", "Message");
		Label("Body_Help", "Write the text players will read. Maximum 2048 characters.");
		Label("Scope_Caption", "Audience");
		Label("Scope_Help", "Click below to cycle through who receives this message.");
		Label("Apply_Label", "APPLY");
		SizeLayoutWidget bodyFrame = SizeLayoutWidget.Cast(m_Root.FindAnyWidget("DCO_MissionBodyFrame"));
		if (bodyFrame) bodyFrame.SetHeightOverride(150);
		string help;
		switch (tool)
		{
			case DCO_GMMissionTool.RESTORE:
				help = "Choose RESTORE ALL TERRAIN to remove every hide area and restore the hidden structures and trees.";
				Label("Apply_Label", "RESTORE ALL TERRAIN");
				break;
			case DCO_GMMissionTool.SCALE:
				if (selected.Count() == 1 && selected[0] && SCR_EditableEntityComponent.DCO_CanScale(selected[0].GetOwner()))
					m_Value.SetText(selected[0].GetOwner().GetScale().ToString());
				help = "1. Select a static prop or whole barricade assembly.\n2. Enter a scale below.\n3. Choose APPLY SCALE. Characters, vehicles and assemblies with moving physics parts are not supported.";
				Label("Value_Caption", "Uniform scale");
				Label("Value_Help", "Enter 0.25 to 4.0. 1.0 is original size, 0.5 is half size, and 2.0 is double size.");
				Label("Apply_Label", "APPLY SCALE");
				break;
			case DCO_GMMissionTool.INVINCIBLE:
				m_Scope = 1;
				help = "1. Select the units or vehicles.\n2. Choose whether invincibility is on or off; optionally include current crew.\n3. Choose APPLY DAMAGE SETTING. Existing damage is not repaired.";
				Label("Scope_Caption", "Damage setting");
				Label("Scope_Help", "Click below to switch invincibility on or off.");
				Label("Apply_Label", "APPLY DAMAGE SETTING");
				break;
			case DCO_GMMissionTool.INTEL:
				help = "1. Select one prop as the clue.\n2. Enter the intel title and text, then choose its audience.\n3. Choose SAVE INTEL. Players approach the prop and use Read Intel to add it to their map journal.";
				Label("Title_Caption", "Intel title");
				Label("Title_Help", "For example: Convoy route. This is the journal entry's name. Maximum 64 characters.");
				Label("Body_Caption", "Intel text");
				Label("Body_Help", "Write the information players discover, such as a route, objective or meeting time. Maximum 2048 characters.");
				Label("Scope_Help", "Click below to choose the finder only, the finder's faction, or all players. Intel is shared only after discovery.");
				Label("Apply_Label", "SAVE INTEL");
				break;
			case DCO_GMMissionTool.HINT:
				help = "1. Enter a title and message.\n2. Choose SEND HINT to notify all current players.";
				Label("Title_Caption", "Title (optional)");
				Label("Title_Help", "For example: Mission update. Leave blank to use Game Master. Maximum 64 characters.");
				Label("Apply_Label", "SEND HINT");
				break;
			case DCO_GMMissionTool.CHATTER:
				m_Scope = 1;
				help = "1. Select one living AI speaker, or clear selection to speak as your faction HQ.\n2. Write the message and choose its audience.\n3. Choose SEND CHATTER. Players receive it in the chat feed, marked [AI].";
				Label("Apply_Label", "SEND CHATTER");
				break;
			case DCO_GMMissionTool.TELEPORTER:
				help = "1. Select a prop and name this endpoint.\n2. Enter a link name and choose SAVE ENDPOINT.\n3. Repeat on a second prop using the same link name. Players approach either prop and use Travel to reach the other.";
				Label("Title_Caption", "Endpoint name");
				Label("Title_Help", "For example: Main Base or Forward Camp. Players see this destination name. Maximum 64 characters.");
				Label("Body_Caption", "Link name");
				Label("Body_Help", "For example: base-shuttle. Enter exactly the same name on both endpoints. Use one line, up to 64 characters; a link connects exactly two props.");
				if (bodyFrame) bodyFrame.SetHeightOverride(48);
				Label("Apply_Label", "SAVE ENDPOINT");
				break;
			case DCO_GMMissionTool.NAMED:
				help = "1. Select AI groups or supported QRF/defence/air/fire-support modules.\n2. Choose a saved position below.\n3. Choose USE THIS POSITION. Groups receive a move waypoint; modules move their operating centre. An active strike keeps its current target.";
				Label("Apply_Label", "USE THIS POSITION");
				break;
			case DCO_GMMissionTool.REMOVE:
				help = "1. Select the props with intel or teleport interactions.\n2. Choose REMOVE INTERACTION. The props themselves stay in the world.";
				Label("Apply_Label", "REMOVE INTERACTION");
				break;
			default:
				help = "1. Choose the ground position.\n2. Give it a name below.\n3. Create the position, then use Use Named Position to direct selected groups or supported modules there.";
				Label("Title_Caption", DCO_GMMissionTool.Name(tool) + " - name");
				Label("Title_Help", "For example: LZ Falcon, RP Oak or Target Bridge. This name appears in the saved-position picker. Maximum 64 characters.");
				Label("Apply_Label", DCO_GMMissionTool.Name(tool));
				break;
		}
		m_Help.SetText(help);
		m_Status.SetText(string.Format("%1 object(s) selected.", m_Targets.Count()));
		if (NeedsGround(tool)) m_Status.SetText("Ground position chosen. Enter a name to save it.");
		m_Status.SetColor(DCO_GMTheme.Get().m_MutedColor);
		m_Root.SetVisible(true);
		ApplyLayout();
		RefreshPositions();
		RefreshControls();
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller && tool == DCO_GMMissionTool.NAMED)
			controller.DCO_RequestGMMarkerSnapshot();
		if (controller && m_Targets.Count() == 1 && (tool == DCO_GMMissionTool.INTEL || tool == DCO_GMMissionTool.TELEPORTER))
		{
			DCO_GMMissionInteractionComponent existing = DCO_GMMissionInteractionComponent.FindTarget(m_Targets[0]);
			if (existing)
			{
				m_Pending = true;
				m_Status.SetText("Loading existing interaction settings...");
				RefreshControls();
				controller.DCO_RequestMissionEdit(m_Targets[0], tool, ++m_RequestSequence);
			}
		}
		if (m_Title.IsVisibleInHierarchy()) GetGame().GetWorkspace().SetFocusedWidget(m_Title);
		else if (m_Value.IsVisibleInHierarchy()) GetGame().GetWorkspace().SetFocusedWidget(m_Value);
		else GetGame().GetWorkspace().SetFocusedWidget(m_Root.FindAnyWidget("DCO_MissionApply"));
	}
	bool IsTargeting() { return m_PendingTargetTool > 0; }
	string TargetingInstruction()
	{
		if (!IsTargeting()) return "";
		if (NeedsGround(m_PendingTargetTool)) return "CLICK TERRAIN";
		return "CLICK AN OBJECT";
	}
	void CancelTargeting() { m_PendingTargetTool = 0; }
	protected bool NeedsGround(int tool)
	{
		return tool == DCO_GMMissionTool.HIDE || tool == DCO_GMMissionTool.LZ || tool == DCO_GMMissionTool.RP || tool == DCO_GMMissionTool.TARGET;
	}
	void BeginFromCatalog(int tool)
	{
		CloseForBack();
		DCO_GMCompositionPanel.Get().CloseForBack();
		bool ground = NeedsGround(tool);
		bool object = tool == DCO_GMMissionTool.SCALE || tool == DCO_GMMissionTool.INVINCIBLE || tool == DCO_GMMissionTool.INTEL || tool == DCO_GMMissionTool.TELEPORTER || tool == DCO_GMMissionTool.NAMED || tool == DCO_GMMissionTool.REMOVE;
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		if (object)
			SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
		if (ground || (object && selected.IsEmpty()))
		{
			m_PendingTargetTool = tool;
			return;
		}
		Open(tool, vector.Zero, null);
	}
	void ConfirmTarget(vector position, SCR_EditableEntityComponent target)
	{
		if (!IsTargeting())
			return;
		if (NeedsGround(m_PendingTargetTool))
		{
			if (!SCR_Global.IsPositionWithinTerrainBounds(position))
				return;
		}
		else if (!target)
			return;
		int tool = m_PendingTargetTool;
		Open(tool, position, target, !NeedsGround(tool));
	}
	protected void RefreshPositions()
	{
		array<DCO_GMMarkerRecord> records = {};
		DCO_GMMarkerService.Get().GetRecords(records);
		UpdatePositions(records);
	}
	protected void UpdatePositions(array<DCO_GMMarkerRecord> records)
	{
		m_Positions.Clear();
		foreach (DCO_GMMarkerRecord record : records)
		{
			if (record && record.m_iScope == DCO_GMMarkerScope.SERVER && record.m_iKind >= 1 && record.m_iKind <= 3)
				m_Positions.Insert(record.Copy());
		}
		m_NamedIndex = -1;
		if (m_NamedId == 0 && !m_Positions.IsEmpty())
			m_NamedId = m_Positions[0].m_iId;
		for (int i = 0; i < m_Positions.Count(); i++)
		{
			if (m_Positions[i].m_iId == m_NamedId)
				m_NamedIndex = i;
		}
		RefreshControls();
	}
	protected DCO_GMMarkerRecord SelectedPosition()
	{
		if (m_NamedIndex < 0 || m_NamedIndex >= m_Positions.Count()) return null;
		return m_Positions[m_NamedIndex];
	}
	protected void Visible(string suffix, bool visible)
	{
		Widget widget = m_Root.FindAnyWidget("DCO_Mission" + suffix);
		if (widget)
			widget.SetVisible(visible);
	}
	protected void Label(string suffix, string text)
	{
		TextWidget widget = TextWidget.Cast(m_Root.FindAnyWidget("DCO_Mission" + suffix));
		if (widget)
			widget.SetText(text);
	}
	protected void RefreshControls()
	{
		if (!m_Root)
			return;
		bool textTool = m_Tool == 5 || m_Tool == 6 || m_Tool == 8 || m_Tool >= 11;
		Visible("TitleField", textTool);
		Visible("BodyField", m_Tool >= 5 && m_Tool <= 8);
		Visible("ValueField", m_Tool == 1 || m_Tool == 3);
		Visible("ScopeField", m_Tool == 4 || m_Tool == 5 || m_Tool == 7);
		Visible("Include", m_Tool == 4 || m_Tool == 5);
		Visible("NamedField", m_Tool == 9);
		string scope = "Finder only";
		if (m_Scope == 1) scope = "Finder's faction";
		if (m_Scope == 2) scope = "All players";
		if (m_Tool == 7)
		{
			scope = "All players";
			if (m_Scope == 1) scope = "Speaker's faction";
			if (m_Scope == 2) scope = "Nearby (100 m)";
		}
		if (m_Tool == 4)
		{
			scope = "Invincible OFF";
			if (m_Scope == 1) scope = "Invincible ON";
		}
		Label("Scope_Label", scope);
		string include = "[ ] Include crew";
		if (m_Include) include = "[X] Include crew";
		if (m_Tool == 5)
		{
			include = "[ ] Remove clue object after discovery";
			if (m_Include) include = "[X] Remove clue object after discovery";
		}
		Label("Include_Label", include);
		string named = "No server LZ / RP / Target available";
		DCO_GMMarkerRecord position = SelectedPosition();
		if (!m_Positions.IsEmpty() && !position) named = "Position removed. Choose another with Previous or Next.";
		if (position)
			named = DCO_GMMarkerKind.Label(position.m_iKind) + " - " + position.m_sName;
		Label("Named", named);
		m_Root.FindAnyWidget("DCO_MissionApply").SetEnabled(!m_Pending && (m_Tool != DCO_GMMissionTool.NAMED || position != null));
		bool canChoose = m_Positions.Count() > 1 || (!m_Positions.IsEmpty() && !position);
		m_Root.FindAnyWidget("DCO_MissionPrevious").SetEnabled(canChoose);
		m_Root.FindAnyWidget("DCO_MissionNext").SetEnabled(canChoose);
	}
	bool OnAction(int action)
	{
		if (action == 0) return CloseForBack();
		if (m_Pending) return true;
		if (action == 2)
		{
			int count = 3;
			if (m_Tool == 4) count = 2;
			m_Scope = (m_Scope + 1) % count;
		}
		if (action == 3) m_Include = !m_Include;
		if ((action == 4 || action == 5) && !m_Positions.IsEmpty())
		{
			int delta = 1;
			if (action == 4) delta = -1;
			if (m_NamedIndex < 0)
			{
				m_NamedIndex = 0;
				if (delta < 0) m_NamedIndex = m_Positions.Count() - 1;
			}
			else
				m_NamedIndex = (m_NamedIndex + delta + m_Positions.Count()) % m_Positions.Count();
			m_NamedId = m_Positions[m_NamedIndex].m_iId;
		}
		if (action == 1)
		{
			if (!ValidateTextFields()) return true;
			SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (!controller) return true;
			vector options = Vector(m_Scope, 0, 0);
			if (m_Include) options[1] = 1;
			if (m_Tool == 1 || m_Tool == 3) options[0] = m_Value.GetText().ToFloat();
			if (m_Tool == 9)
			{
				DCO_GMMarkerRecord position = SelectedPosition();
				if (!position) return true;
				options[0] = position.m_iId;
			}
			m_Pending = true;
			m_Status.SetText("Applying...");
			controller.DCO_SendMissionTool(m_Tool, m_Targets, m_Position, options, m_Title.GetText(), m_Body.GetText(), ++m_RequestSequence);
		}
		RefreshControls();
		return true;
	}
	protected bool ValidateTextFields()
	{
		string title = m_Title.GetText();
		string body = m_Body.GetText();
		title.TrimInPlace();
		body.TrimInPlace();
		string issue;
		if (title.Length() > 64) issue = "Shorten the name or title to 64 characters or fewer.";
		else if ((m_Tool == 5 || m_Tool == 8 || m_Tool >= 11) && title.IsEmpty()) issue = "Enter a name or title in the first text box.";
		else if (m_Tool >= 5 && m_Tool <= 8 && body.IsEmpty()) issue = "Enter the message, intel text or link name in the labeled text box.";
		else if (body.Length() > 2048) issue = "Shorten the message to 2048 characters or fewer.";
		else if (m_Tool == 8 && (body.Length() > 64 || body.Contains("\n") || body.Contains("\r"))) issue = "Use a single-line link name of 64 characters or fewer.";
		else if (m_Tool == 1 && !(m_Value.GetText().ToFloat() >= 5 && m_Value.GetText().ToFloat() <= 100)) issue = "Enter a radius from 5 to 100 metres.";
		else if (m_Tool == 3 && !(m_Value.GetText().ToFloat() >= 0.25 && m_Value.GetText().ToFloat() <= 4)) issue = "Enter a scale from 0.25 to 4.0. Use 1.0 for original size.";
		if (issue.IsEmpty()) return true;
		OnResult(false, issue);
		return false;
	}
	void OnResult(bool success, string result)
	{
		m_Pending = false;
		if (m_Status)
		{
			m_Status.SetText(result);
			m_Status.SetColor(DCO_GMTheme.Get().m_TextColor);
			if (!success) m_Status.SetColor(Color.FromRGBA(224, 82, 82, 255));
		}
		RefreshControls();
		Print("[DCO-GM] " + result);
	}
	protected bool AcceptsReply(int requestSequence)
	{
		return IsOpen() && m_Pending && requestSequence == m_RequestSequence;
	}
	void OnRequestResult(int requestSequence, bool success, string result)
	{
		if (!AcceptsReply(requestSequence)) return;
		OnResult(success, result);
	}
	void OnEdit(int requestSequence, RplId id, string title, string body, int scope, bool removeClue)
	{
		if (!AcceptsReply(requestSequence) || (m_Tool != DCO_GMMissionTool.INTEL && m_Tool != DCO_GMMissionTool.TELEPORTER) || m_Targets.Count() != 1 || m_Targets[0] != id)
			return;
		m_Title.SetText(title);
		m_Body.SetText(body);
		m_Scope = scope;
		m_Include = removeClue;
		m_Pending = false;
		m_Status.SetText("Existing interaction loaded. Apply saves your changes.");
		RefreshControls();
	}
}
