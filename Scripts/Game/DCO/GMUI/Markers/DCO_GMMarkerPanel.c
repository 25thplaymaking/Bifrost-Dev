// Bifrost marker/intel editor and GM-only world presentation.

class DCO_GMMarkerPanelButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMMarkerPanel m_Owner;
	protected int m_iAction;

	void DCO_GMMarkerPanelButtonHandler(DCO_GMMarkerPanel owner, int action)
	{
		m_Owner = owner;
		m_iAction = action;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		return m_Owner && m_Owner.OnAction(m_iAction);
	}
}

class DCO_GMMarkerTreeRow
{
	bool m_bHeader;
	string m_sHeader;
	DCO_GMMarkerRecord m_Record;
}

class DCO_GMMarkerPanel
{
	protected static ref DCO_GMMarkerPanel s_Instance;
	protected static const ResourceName LAYOUT = "{D4C06B89F140A217}UI/layouts/DCO_GMMarkers.layout";
	protected static const int ROW_COUNT = 14;
	protected static const int LABEL_POOL = 48;
	protected static const int ACTION_CLOSE = 1;
	protected static const int ACTION_SCOPE_LOCAL = 2;
	protected static const int ACTION_SCOPE_SERVER = 3;
	protected static const int ACTION_KIND_BASE = 10;
	protected static const int ACTION_PLACE = 20;
	protected static const int ACTION_UPDATE = 21;
	protected static const int ACTION_DELETE = 22;
	protected static const int ACTION_PREV = 23;
	protected static const int ACTION_NEXT = 24;
	protected static const int ACTION_VIS_TOGGLE = 25;
	protected static const int ACTION_VIS_PLACEMENT = 26;
	protected static const int ACTION_VIS_DISTANCE = 27;
	protected static const int ACTION_ROW_BASE = 100;
	protected static const float DEG_TO_RAD = 0.0174533;
	protected static const int COLOR_LOCAL = 0xE035B6D4;
	protected static const int COLOR_SERVER = 0xE0D9892B;
	protected static const int COLOR_TARGET = 0xE0E05252;

	protected Widget m_wRoot;
	protected Widget m_wPanel;
	protected Widget m_wLabelLayer;
	protected TextWidget m_wPosition;
	protected EditBoxWidget m_wName;
	protected EditBoxWidget m_wSizeX;
	protected EditBoxWidget m_wSizeZ;
	protected EditBoxWidget m_wRotation;
	protected TextWidget m_wPage;
	protected ref array<ButtonWidget> m_RowButtons = {};
	protected ref array<TextWidget> m_RowLabels = {};
	protected ref array<TextWidget> m_KindLabels = {};
	protected ref array<TextWidget> m_WorldLabels = {};
	protected ref array<ref DCO_GMMarkerPanelButtonHandler> m_Handlers = {};
	protected ref array<ref DCO_GMMarkerTreeRow> m_TreeRows = {};
	protected DCO_GMRenderManager m_Render;
	protected bool m_bOpen;
	protected vector m_vPosition;
	protected int m_iKind;
	protected int m_iScope;
	protected int m_iPage;
	protected int m_iSelectedId;
	protected int m_iSelectedScope;

	static DCO_GMMarkerPanel Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMMarkerPanel();
		return s_Instance;
	}

	void Init(Widget shellRoot, DCO_GMRenderManager render)
	{
		Shutdown();
		if (!shellRoot)
			return;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;
		m_wRoot = workspace.CreateWidgets(LAYOUT, shellRoot);
		if (!m_wRoot)
		{
			Print("[DCO-GM] marker panel layout failed to instantiate", LogLevel.ERROR);
			return;
		}
		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);
		m_wPanel = m_wRoot.FindAnyWidget("DCO_MarkerPanel");
		if (m_wPanel)
		{
			FrameSlot.SetAnchor(m_wPanel, 0.5, 0.5);
			FrameSlot.SetAlignment(m_wPanel, 0.5, 0.5);
			FrameSlot.SetSize(m_wPanel, 620, 790);
			FrameSlot.SetPos(m_wPanel, 0, 0);
			m_wPanel.SetVisible(false);
		}
		m_wPosition = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerPosition"));
		m_wName = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerName"));
		m_wSizeX = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerSizeX"));
		m_wSizeZ = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerSizeZ"));
		m_wRotation = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerRotation"));
		m_wPage = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerPage"));

		Bind("DCO_MarkerClose", ACTION_CLOSE);
		Bind("DCO_MarkerScopeLocal", ACTION_SCOPE_LOCAL);
		Bind("DCO_MarkerScopeServer", ACTION_SCOPE_SERVER);
		for (int kind = 0; kind < DCO_GMMarkerKind.COUNT; kind++)
		{
			Bind("DCO_MarkerKind" + kind.ToString(), ACTION_KIND_BASE + kind);
			m_KindLabels.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerKind" + kind.ToString() + "_Label")));
		}
		Bind("DCO_MarkerPlace", ACTION_PLACE);
		Bind("DCO_MarkerUpdate", ACTION_UPDATE);
		Bind("DCO_MarkerDelete", ACTION_DELETE);
		Bind("DCO_MarkerPrev", ACTION_PREV);
		Bind("DCO_MarkerNext", ACTION_NEXT);
		Bind("DCO_MarkerVisToggle", ACTION_VIS_TOGGLE);
		Bind("DCO_MarkerVisPlacement", ACTION_VIS_PLACEMENT);
		Bind("DCO_MarkerVisDistance", ACTION_VIS_DISTANCE);
		for (int rowIndex = 0; rowIndex < ROW_COUNT; rowIndex++)
		{
			ButtonWidget row = ButtonWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerRow" + rowIndex.ToString()));
			m_RowButtons.Insert(row);
			m_RowLabels.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerRow" + rowIndex.ToString() + "_Label")));
			Bind("DCO_MarkerRow" + rowIndex.ToString(), ACTION_ROW_BASE + rowIndex);
		}

		m_Render = render;
		if (m_Render)
			m_Render.GetOnRender().Insert(OnRender);
		m_wLabelLayer = shellRoot.FindAnyWidget("DCO_AIRoleMarkerLayer");
		BuildWorldLabels();
		DCO_GMMarkerService.Get().GetOnChanged().Insert(OnRegistryChanged);
		DCO_GMMarkerService.Get().Initialize();
		DCO_GMVisibilityIndicator.Get().Start(render, shellRoot);
		m_iKind = DCO_GMMarkerKind.POINT;
		m_iScope = DCO_GMMarkerScope.LOCAL;
		m_iSelectedId = 0;
		m_iSelectedScope = DCO_GMMarkerScope.LOCAL;
		m_iPage = 0;
		m_bOpen = false;
		RefreshState();
		DCO_GMTheme.Get().ApplyAccent(m_wRoot);
	}

	void Shutdown()
	{
		DCO_GMVisibilityIndicator.Get().Stop();
		DCO_GMMarkerService.Get().GetOnChanged().Remove(OnRegistryChanged);
		if (m_Render)
			m_Render.GetOnRender().Remove(OnRender);
		m_Render = null;
		foreach (TextWidget label : m_WorldLabels)
		{
			if (label)
				label.RemoveFromHierarchy();
		}
		m_WorldLabels.Clear();
		m_Handlers.Clear();
		m_RowButtons.Clear();
		m_RowLabels.Clear();
		m_KindLabels.Clear();
		m_TreeRows.Clear();
		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
		m_wPanel = null;
		m_wLabelLayer = null;
		m_wPosition = null;
		m_wName = null;
		m_wSizeX = null;
		m_wSizeZ = null;
		m_wRotation = null;
		m_wPage = null;
		m_bOpen = false;
	}

	protected void Bind(string widgetName, int action)
	{
		ButtonWidget button = ButtonWidget.Cast(m_wRoot.FindAnyWidget(widgetName));
		if (!button)
			return;
		DCO_GMMarkerPanelButtonHandler handler = new DCO_GMMarkerPanelButtonHandler(this, action);
		button.AddHandler(handler);
		m_Handlers.Insert(handler);
	}

	protected void BuildWorldLabels()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace || !m_wLabelLayer)
			return;
		int flags = WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS | WidgetFlags.STRETCH | WidgetFlags.NO_LOCALIZATION;
		for (int i = 0; i < LABEL_POOL; i++)
		{
			TextWidget label = TextWidget.Cast(workspace.CreateWidget(WidgetType.TextWidgetTypeID, flags, Color.White, 0, m_wLabelLayer));
			if (!label)
				break;
			FrameSlot.SetAlignment(label, 0.5, 1.0);
			FrameSlot.SetSize(label, 280, 26);
			label.SetExactFontSize(17);
			label.SetVisible(false);
			m_WorldLabels.Insert(label);
		}
	}

	void Open(vector position)
	{
		if (!m_wPanel)
			return;
		if (position == vector.Zero)
		{
			SCR_MenuLayoutEditorComponent menuLayout = SCR_MenuLayoutEditorComponent.Cast(
				SCR_MenuLayoutEditorComponent.GetInstance(SCR_MenuLayoutEditorComponent, false));
			if (menuLayout)
				menuLayout.GetCursorWorldPos(position);
		}
		if (SCR_Global.IsPositionWithinTerrainBounds(position))
			m_vPosition = position;
		m_iSelectedId = 0;
		m_iSelectedScope = DCO_GMMarkerScope.LOCAL;
		m_iPage = 0;
		ResetFields();
		RefreshState();
		m_wPanel.SetVisible(true);
		m_bOpen = true;
	}

	void OpenManager()
	{
		Open(vector.Zero);
	}

	bool CloseForBack()
	{
		if (!m_bOpen)
			return false;
		Close();
		return true;
	}

	protected void Close()
	{
		m_bOpen = false;
		if (m_wPanel)
			m_wPanel.SetVisible(false);
		DCO_GMUIController.ReleaseMenuFocus();
	}

	bool OnAction(int action)
	{
		if (action == ACTION_CLOSE)
		{
			Close();
			return true;
		}
		if (action == ACTION_SCOPE_LOCAL || action == ACTION_SCOPE_SERVER)
		{
			m_iScope = DCO_GMMarkerScope.LOCAL;
			if (action == ACTION_SCOPE_SERVER)
				m_iScope = DCO_GMMarkerScope.SERVER;
			RefreshState();
			return true;
		}
		if (action >= ACTION_KIND_BASE && action < ACTION_KIND_BASE + DCO_GMMarkerKind.COUNT)
		{
			m_iKind = action - ACTION_KIND_BASE;
			RefreshState();
			return true;
		}
		if (action == ACTION_PLACE)
		{
			vector sizeRotation;
			string name;
			ReadFields(sizeRotation, name);
			DCO_GMMarkerService.Get().Create(m_iKind, m_iScope, m_vPosition, sizeRotation, name);
			m_iSelectedId = 0;
			return true;
		}
		if (action == ACTION_UPDATE)
		{
			if (m_iSelectedId == 0)
				return true;
			vector sizeRotation;
			string name;
			ReadFields(sizeRotation, name);
			DCO_GMMarkerService.Get().Update(m_iSelectedId, m_iSelectedScope, m_iKind, m_iScope, m_vPosition, sizeRotation, name);
			m_iSelectedId = 0;
			return true;
		}
		if (action == ACTION_DELETE)
		{
			if (m_iSelectedId != 0)
				DCO_GMMarkerService.Get().Delete(m_iSelectedId, m_iSelectedScope);
			m_iSelectedId = 0;
			return true;
		}
		if (action == ACTION_PREV || action == ACTION_NEXT)
		{
			int pages = Math.Max(1, (m_TreeRows.Count() + ROW_COUNT - 1) / ROW_COUNT);
			if (action == ACTION_PREV)
				m_iPage = Math.Max(0, m_iPage - 1);
			else
				m_iPage = Math.Min(pages - 1, m_iPage + 1);
			RefreshTreeRows();
			return true;
		}
		if (action == ACTION_VIS_TOGGLE)
			DCO_GMVisibilityIndicator.Get().ToggleEnabled();
		else if (action == ACTION_VIS_PLACEMENT)
			DCO_GMVisibilityIndicator.Get().TogglePlacementOnly();
		else if (action == ACTION_VIS_DISTANCE)
			DCO_GMVisibilityIndicator.Get().CycleDistance();
		else if (action >= ACTION_ROW_BASE && action < ACTION_ROW_BASE + ROW_COUNT)
		{
			SelectTreeRow(action - ACTION_ROW_BASE);
			return true;
		}
		RefreshVisibilityControls();
		return true;
	}

	protected void ReadFields(out vector sizeRotation, out string name)
	{
		name = "";
		if (m_wName)
			name = m_wName.GetText();
		float sizeX = 25;
		float sizeZ = 25;
		float rotation;
		if (m_wSizeX && !m_wSizeX.GetText().IsEmpty())
			sizeX = m_wSizeX.GetText().ToFloat();
		if (m_wSizeZ && !m_wSizeZ.GetText().IsEmpty())
			sizeZ = m_wSizeZ.GetText().ToFloat();
		if (m_wRotation && !m_wRotation.GetText().IsEmpty())
			rotation = m_wRotation.GetText().ToFloat();
		sizeRotation = Vector(sizeX, sizeZ, rotation);
	}

	protected void ResetFields()
	{
		if (m_wName)
			m_wName.SetText("");
		if (m_wSizeX)
			m_wSizeX.SetText("25");
		if (m_wSizeZ)
			m_wSizeZ.SetText("25");
		if (m_wRotation)
			m_wRotation.SetText("0");
	}

	protected void OnRegistryChanged()
	{
		BuildTree();
		RefreshTreeRows();
	}

	protected void RefreshState()
	{
		if (m_wPosition)
			m_wPosition.SetText(string.Format("POSITION  %1  /  %2  /  %3", Math.Round(m_vPosition[0]), Math.Round(m_vPosition[1]), Math.Round(m_vPosition[2])));
		Color accent = DCO_GMTheme.Get().m_AccentColor;
		Color muted = DCO_GMTheme.Get().m_MutedColor;
		TextWidget localLabel = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerScopeLocal_Label"));
		TextWidget serverLabel = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerScopeServer_Label"));
		if (localLabel)
		{
			localLabel.SetColor(muted);
			if (m_iScope == DCO_GMMarkerScope.LOCAL)
				localLabel.SetColor(accent);
		}
		if (serverLabel)
		{
			serverLabel.SetColor(muted);
			if (m_iScope == DCO_GMMarkerScope.SERVER)
				serverLabel.SetColor(accent);
		}
		for (int kind = 0; kind < m_KindLabels.Count(); kind++)
		{
			if (m_KindLabels[kind])
			{
				m_KindLabels[kind].SetColor(muted);
				if (kind == m_iKind)
					m_KindLabels[kind].SetColor(accent);
			}
		}
		BuildTree();
		RefreshTreeRows();
		RefreshVisibilityControls();
	}

	protected void RefreshVisibilityControls()
	{
		DCO_GMVisibilityIndicator indicator = DCO_GMVisibilityIndicator.Get();
		TextWidget enabled = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerVisToggle_Label"));
		TextWidget placement = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerVisPlacement_Label"));
		TextWidget distance = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_MarkerVisDistance_Label"));
		if (enabled)
		{
			enabled.SetText("INDICATOR OFF");
			enabled.SetColor(DCO_GMTheme.Get().m_MutedColor);
			if (indicator.IsEnabled())
			{
				enabled.SetText("INDICATOR ON");
				enabled.SetColor(DCO_GMTheme.Get().m_AccentColor);
			}
		}
		if (placement)
		{
			placement.SetText("ALWAYS AT CURSOR");
			if (indicator.IsPlacementOnly())
				placement.SetText("PLACEMENT ONLY");
		}
		if (distance)
			distance.SetText("RANGE " + Math.Round(indicator.GetMaxDistance()).ToString() + " M");
	}

	protected void BuildTree()
	{
		m_TreeRows.Clear();
		array<DCO_GMMarkerRecord> records = {};
		DCO_GMMarkerService.Get().GetRecords(records);
		array<string> categories = {"POINTS", "AREAS", "INTEL & CONTROL", "COMMENTS"};
		foreach (string category : categories)
		{
			bool hasCategory;
			foreach (DCO_GMMarkerRecord testRecord : records)
			{
				if (testRecord && DCO_GMMarkerKind.Category(testRecord.m_iKind) == category)
				{
					hasCategory = true;
					break;
				}
			}
			if (!hasCategory)
				continue;
			DCO_GMMarkerTreeRow header = new DCO_GMMarkerTreeRow();
			header.m_bHeader = true;
			header.m_sHeader = category;
			m_TreeRows.Insert(header);
			foreach (DCO_GMMarkerRecord record : records)
			{
				if (!record || DCO_GMMarkerKind.Category(record.m_iKind) != category)
					continue;
				DCO_GMMarkerTreeRow row = new DCO_GMMarkerTreeRow();
				row.m_Record = record;
				m_TreeRows.Insert(row);
			}
		}
		int pages = Math.Max(1, (m_TreeRows.Count() + ROW_COUNT - 1) / ROW_COUNT);
		m_iPage = Math.ClampInt(m_iPage, 0, pages - 1);
	}

	protected void RefreshTreeRows()
	{
		int pages = Math.Max(1, (m_TreeRows.Count() + ROW_COUNT - 1) / ROW_COUNT);
		if (m_wPage)
			m_wPage.SetText((m_iPage + 1).ToString() + " / " + pages.ToString());
		for (int rowIndex = 0; rowIndex < ROW_COUNT; rowIndex++)
		{
			ButtonWidget button = m_RowButtons[rowIndex];
			TextWidget label = m_RowLabels[rowIndex];
			int treeIndex = m_iPage * ROW_COUNT + rowIndex;
			if (!button || !label || treeIndex >= m_TreeRows.Count())
			{
				if (button)
					button.SetVisible(false);
				continue;
			}
			button.SetVisible(true);
			DCO_GMMarkerTreeRow row = m_TreeRows[treeIndex];
			if (row.m_bHeader)
			{
				label.SetText(row.m_sHeader);
				label.SetColor(DCO_GMTheme.Get().m_AccentColor);
				continue;
			}
			DCO_GMMarkerRecord record = row.m_Record;
			string scope = "[L]";
			if (record.m_iScope == DCO_GMMarkerScope.SERVER)
				scope = "[S]";
			label.SetText("   " + scope + "  " + DCO_GMMarkerKind.Label(record.m_iKind) + "  ·  " + record.m_sName);
			if (record.m_iId == m_iSelectedId && record.m_iScope == m_iSelectedScope)
				label.SetColor(DCO_GMTheme.Get().m_AccentColor);
			else
				label.SetColor(DCO_GMTheme.Get().m_TextColor);
		}
	}

	protected void SelectTreeRow(int visibleRow)
	{
		int treeIndex = m_iPage * ROW_COUNT + visibleRow;
		if (treeIndex < 0 || treeIndex >= m_TreeRows.Count())
			return;
		DCO_GMMarkerTreeRow row = m_TreeRows[treeIndex];
		if (!row || row.m_bHeader || !row.m_Record)
			return;
		DCO_GMMarkerRecord record = row.m_Record;
		m_iSelectedId = record.m_iId;
		m_iSelectedScope = record.m_iScope;
		m_iKind = record.m_iKind;
		m_iScope = record.m_iScope;
		m_vPosition = record.m_vPosition;
		if (m_wName)
			m_wName.SetText(record.m_sName);
		if (m_wSizeX)
			m_wSizeX.SetText(record.m_vSizeRotation[0].ToString(1, 0));
		if (m_wSizeZ)
			m_wSizeZ.SetText(record.m_vSizeRotation[1].ToString(1, 0));
		if (m_wRotation)
			m_wRotation.SetText(record.m_vSizeRotation[2].ToString(1, 0));
		RefreshState();
	}

	protected void OnRender(DCO_GMRenderManager render)
	{
		if (!render || DCO_GMTheme.Get().IsMasterHidden())
		{
			HideWorldLabels();
			return;
		}
		array<DCO_GMMarkerRecord> records = {};
		DCO_GMMarkerService.Get().GetRecords(records);
		int labelIndex;
		foreach (DCO_GMMarkerRecord record : records)
		{
			if (!record)
				continue;
			int color = COLOR_LOCAL;
			if (record.m_iScope == DCO_GMMarkerScope.SERVER)
				color = COLOR_SERVER;
			if (record.m_iKind == DCO_GMMarkerKind.TARGET)
				color = COLOR_TARGET;
			DrawRecord(render, record, color);
			if (labelIndex < m_WorldLabels.Count() && UpdateWorldLabel(m_WorldLabels[labelIndex], record, color))
				labelIndex++;
		}
		for (int hide = labelIndex; hide < m_WorldLabels.Count(); hide++)
			m_WorldLabels[hide].SetVisible(false);
	}

	protected void DrawRecord(DCO_GMRenderManager render, DCO_GMMarkerRecord record, int color)
	{
		vector center = record.m_vPosition + Vector(0, 0.06, 0);
		if (record.m_iKind == DCO_GMMarkerKind.CIRCLE)
		{
			render.DrawRing(center, Vector(1, 0, 0), Vector(0, 0, 1), record.m_vSizeRotation[0], color);
			return;
		}
		if (record.m_iKind == DCO_GMMarkerKind.RECTANGLE)
		{
			float angle = record.m_vSizeRotation[2] * DEG_TO_RAD;
			vector axisX = Vector(Math.Cos(angle), 0, Math.Sin(angle));
			vector axisZ = Vector(-Math.Sin(angle), 0, Math.Cos(angle));
			float halfX = record.m_vSizeRotation[0] * 0.5;
			float halfZ = record.m_vSizeRotation[1] * 0.5;
			render.DrawQuad(center - axisX * halfX - axisZ * halfZ, center + axisX * halfX - axisZ * halfZ,
				center + axisX * halfX + axisZ * halfZ, center - axisX * halfX + axisZ * halfZ, color);
			return;
		}
		float radius = 0.55;
		if (record.m_iKind == DCO_GMMarkerKind.LANDING_ZONE)
			radius = 1.2;
		render.DrawRing(center, Vector(1, 0, 0), Vector(0, 0, 1), radius, color);
		render.DrawLine(center + Vector(-radius, 0, 0), center + Vector(radius, 0, 0), color, 3.0);
		render.DrawLine(center + Vector(0, 0, -radius), center + Vector(0, 0, radius), color, 3.0);
		if (record.m_iKind == DCO_GMMarkerKind.COMMENT)
			render.DrawStick(center, 1.4, color);
	}

	protected bool UpdateWorldLabel(TextWidget label, DCO_GMMarkerRecord record, int color)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		if (!label || !workspace || !world)
			return false;
		vector screen = workspace.ProjWorldToScreen(record.m_vPosition + Vector(0, 1.6, 0), world);
		if (screen[2] < 0 || screen[0] < -300 || screen[0] > workspace.GetWidth() + 300 || screen[1] < -40 || screen[1] > workspace.GetHeight() + 40)
		{
			label.SetVisible(false);
			return false;
		}
		string scope = "L";
		if (record.m_iScope == DCO_GMMarkerScope.SERVER)
			scope = "S";
		label.SetText("[" + scope + "] " + DCO_GMMarkerKind.Label(record.m_iKind) + "  ·  " + record.m_sName);
		label.SetColor(Color.FromInt(color));
		FrameSlot.SetPos(label, screen[0], screen[1]);
		label.SetVisible(true);
		return true;
	}

	protected void HideWorldLabels()
	{
		foreach (TextWidget label : m_WorldLabels)
		{
			if (label)
				label.SetVisible(false);
		}
	}
}
