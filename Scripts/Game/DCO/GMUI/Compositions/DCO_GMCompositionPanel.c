class DCO_GMCompositionPanelButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMCompositionPanel m_Owner;
	protected int m_iAction;

	void DCO_GMCompositionPanelButtonHandler(DCO_GMCompositionPanel owner, int action)
	{
		m_Owner = owner;
		m_iAction = action;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		return m_Owner && m_Owner.OnAction(m_iAction);
	}
}

class DCO_GMCompositionPanel
{
	protected static ref DCO_GMCompositionPanel s_Instance;
	protected static const ResourceName LAYOUT = "{C4B9E6237A18D50F}UI/layouts/DCO_GMCompositions.layout";
	protected static const int ROW_COUNT = 8;
	protected static const int ACTION_CLOSE = 1;
	protected static const int ACTION_CAPTURE = 2;
	protected static const int ACTION_PLACE = 3;
	protected static const int ACTION_DELETE = 4;
	protected static const int ACTION_UNDO = 5;
	protected static const int ACTION_PREV = 6;
	protected static const int ACTION_NEXT = 7;
	protected static const int ACTION_ROW_BASE = 100;

	protected Widget m_wRoot;
	protected Widget m_wPanel;
	protected EditBoxWidget m_wName;
	protected EditBoxWidget m_wCategory;
	protected EditBoxWidget m_wAuthor;
	protected TextWidget m_wPosition;
	protected TextWidget m_wStatus;
	protected TextWidget m_wPage;
	protected TextWidget m_wLibrarySummary;
	protected TextWidget m_wSelection;
	protected Widget m_wEmpty;
	protected Widget m_wScroll;
	protected Widget m_wPagination;
	protected ref array<ButtonWidget> m_aRowButtons = {};
	protected ref array<TextWidget> m_aRowLabels = {};
	protected ref array<TextWidget> m_aRowMetadata = {};
	protected ref array<ImageWidget> m_aRowBackgrounds = {};
	protected ref array<ref DCO_GMCompositionPanelButtonHandler> m_aHandlers = {};
	protected ref array<DCO_GMCompositionCatalogEntry> m_aEntries = {};
	protected bool m_bOpen;
	protected int m_iPage;
	protected int m_iSelectedId;
	protected int m_iPlacementId;

	static DCO_GMCompositionPanel Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMCompositionPanel();
		return s_Instance;
	}

	void Init(Widget shellRoot)
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
			Print("[DCO-GM] composition panel layout failed to instantiate", LogLevel.ERROR);
			return;
		}
		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);
		m_wRoot.SetZOrder(9800);
		m_wRoot.SetVisible(false);
		m_wPanel = m_wRoot.FindAnyWidget("DCO_CompositionPanel");
		if (m_wPanel)
		{
			FrameSlot.SetAnchor(m_wPanel, 0.5, 0.5);
			FrameSlot.SetAlignment(m_wPanel, 0.5, 0.5);
			FrameSlot.SetSize(m_wPanel, 840, 620);
			FrameSlot.SetPos(m_wPanel, 0, 0);
			m_wPanel.SetVisible(false);
		}
		m_wName = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionName"));
		m_wCategory = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionCategory"));
		m_wAuthor = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionAuthor"));
		m_wPosition = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionPosition"));
		m_wStatus = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionStatus"));
		m_wPage = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionPage"));
		m_wLibrarySummary = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionLibrarySummary"));
		m_wSelection = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionSelection"));
		m_wEmpty = m_wRoot.FindAnyWidget("DCO_CompositionEmpty");
		m_wScroll = m_wRoot.FindAnyWidget("DCO_CompositionScroll");
		m_wPagination = m_wRoot.FindAnyWidget("DCO_CompositionPagination");
		Bind("DCO_CompositionClose", ACTION_CLOSE);
		Bind("DCO_CompositionCapture", ACTION_CAPTURE);
		Bind("DCO_CompositionPlace", ACTION_PLACE);
		Bind("DCO_CompositionDelete", ACTION_DELETE);
		Bind("DCO_CompositionUndo", ACTION_UNDO);
		Bind("DCO_CompositionPrev", ACTION_PREV);
		Bind("DCO_CompositionNext", ACTION_NEXT);
		for (int rowIndex = 0; rowIndex < ROW_COUNT; rowIndex++)
		{
			ButtonWidget rowButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionRow" + rowIndex.ToString()));
			TextWidget rowLabel = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionRow" + rowIndex.ToString() + "_Label"));
			ImageWidget rowBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionRow" + rowIndex.ToString() + "_Bg"));
			m_aRowButtons.Insert(rowButton);
			m_aRowLabels.Insert(rowLabel);
			m_aRowMetadata.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_CompositionRow" + rowIndex.ToString() + "_Metadata")));
			m_aRowBackgrounds.Insert(rowBackground);
			Bind("DCO_CompositionRow" + rowIndex.ToString(), ACTION_ROW_BASE + rowIndex);
		}
		DCO_GMCompositionService.Get().GetOnChanged().Insert(OnLibraryChanged);
		DCO_GMCompositionService.Get().GetOnResult().Insert(OnResult);
		DCO_GMCompositionService.Get().Initialize();
		DCO_GMTheme.Get().ApplyAccent(m_wRoot);
		DCO_GMTheme.Get().ApplyOpacity(m_wRoot);
		DCO_GMTheme.Get().ApplyDisplayFont(m_wRoot);
		SetStatus(true, "Persistent server library ready. Captures are saved to this server profile.");
		RefreshLibrary();
	}

	void Shutdown()
	{
		DCO_GMCompositionService.Get().GetOnChanged().Remove(OnLibraryChanged);
		DCO_GMCompositionService.Get().GetOnResult().Remove(OnResult);
		m_aHandlers.Clear();
		m_aRowButtons.Clear();
		m_aRowLabels.Clear();
		m_aRowMetadata.Clear();
		m_aRowBackgrounds.Clear();
		m_aEntries.Clear();
		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
		m_wPanel = null;
		m_wName = null;
		m_wCategory = null;
		m_wAuthor = null;
		m_wPosition = null;
		m_wStatus = null;
		m_wPage = null;
		m_wLibrarySummary = null;
		m_wSelection = null;
		m_wEmpty = null;
		m_wScroll = null;
		m_wPagination = null;
		m_bOpen = false;
	}

	void ApplyLayout(bool compactViewport)
	{
		if (!m_wPanel || !m_wRoot)
			return;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;
		float width = workspace.DPIUnscale(workspace.GetWidth());
		float height = workspace.DPIUnscale(workspace.GetHeight());
		float desiredHeight = 620;
		if (!m_aEntries.IsEmpty())
			desiredHeight = 760;
		FrameSlot.SetSize(m_wPanel, Math.Min(840, Math.Max(320, width - 40)), Math.Min(desiredHeight, Math.Max(400, height - 40)));
	}

	protected void Bind(string widgetName, int action)
	{
		ButtonWidget button = ButtonWidget.Cast(m_wRoot.FindAnyWidget(widgetName));
		if (!button)
			return;
		DCO_GMCompositionPanelButtonHandler handler = new DCO_GMCompositionPanelButtonHandler(this, action);
		button.AddHandler(handler);
		m_aHandlers.Insert(handler);
	}

	void Open(vector position)
	{
		OpenInternal(position, false);
	}

	void OpenForCapture(vector position)
	{
		OpenInternal(position, true);
	}

	protected void OpenInternal(vector position, bool captureFocus)
	{
		if (!m_wPanel)
			return;
		DCO_GMCompositionService.Get().RequestSnapshot();
		m_iPage = 0;
		m_iSelectedId = 0;
		if (m_wName)
			m_wName.SetText("");
		if (m_wCategory)
			m_wCategory.SetText("Custom");
		if (m_wAuthor)
			m_wAuthor.SetText("Game Master");
		RefreshLibrary();
		RefreshPosition();
		RefreshCaptureSelection();
		ApplyLayout(false);
		m_wRoot.SetVisible(true);
		m_wPanel.SetVisible(true);
		m_bOpen = true;
		if (!captureFocus)
		{
			SetStatus(true, "Select an entry, choose PLACE IN WORLD, then click terrain. Escape cancels placement.");
			return;
		}

		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
		if (selected.IsEmpty())
			SetStatus(false, "No editable objects are selected. Close this panel, select the base objects, then try again.");
		else
			SetStatus(true, string.Format("%1 objects selected. Enter a name and choose SAVE SELECTED. Category and author are optional.", selected.Count()));

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace && m_wName)
			workspace.SetFocusedWidget(m_wName);
	}

	bool CloseForBack()
	{
		if (m_iPlacementId > 0)
		{
			m_iPlacementId = 0;
			return true;
		}
		if (!m_bOpen)
			return false;
		Close();
		return true;
	}

	protected void Close()
	{
		m_bOpen = false;
		if (m_wRoot)
			m_wRoot.SetVisible(false);
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
		if (action == ACTION_CAPTURE)
		{
			string name;
			string category;
			string author;
			if (m_wName)
				name = m_wName.GetText();
			if (m_wCategory)
				category = m_wCategory.GetText();
			if (m_wAuthor)
				author = m_wAuthor.GetText();
			name.TrimInPlace();
			if (name.Length() > 48 || category.Length() > 32 || author.Length() > 32)
			{
				SetStatus(false, "Use up to 48 characters for the name, and 32 each for category and author.");
				return true;
			}
			if (name.IsEmpty())
			{
				SetStatus(false, "Enter a name for this composition before saving.");
				if (m_wName)
					GetGame().GetWorkspace().SetFocusedWidget(m_wName);
				return true;
			}
			DCO_GMCompositionService.Get().CaptureSelected(name, category, author);
			return true;
		}
		if (action == ACTION_PLACE)
		{
			if (!RequireSelection("place"))
				return true;
			m_iPlacementId = m_iSelectedId;
			Close();
			SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
			if (popup)
				popup.PopupMsg("Click terrain to place the selected composition. Escape cancels.", duration: 5);
			return true;
		}
		if (action == ACTION_DELETE)
		{
			if (!RequireSelection("delete"))
				return true;
			DCO_GMCompositionService.Get().DeleteLibraryEntry(m_iSelectedId);
			return true;
		}
		if (action == ACTION_UNDO)
		{
			DCO_GMCompositionService.Get().UndoLastPlacement();
			return true;
		}
		if (action == ACTION_PREV || action == ACTION_NEXT)
		{
			int pageCount = Math.Max(1, (m_aEntries.Count() + ROW_COUNT - 1) / ROW_COUNT);
			if (action == ACTION_PREV)
				m_iPage = Math.Max(0, m_iPage - 1);
			else
				m_iPage = Math.Min(pageCount - 1, m_iPage + 1);
			RefreshRows();
			return true;
		}
		if (action >= ACTION_ROW_BASE && action < ACTION_ROW_BASE + ROW_COUNT)
		{
			SelectRow(action - ACTION_ROW_BASE);
			return true;
		}
		return false;
	}

	bool IsTargetingPlacement()
	{
		return m_iPlacementId > 0;
	}
	bool IsOpen() { return m_bOpen; }

	bool PlaceAtWorldCursor(vector position)
	{
		if (m_iPlacementId <= 0)
			return false;
		int compositionId = m_iPlacementId;
		m_iPlacementId = 0;
		if (!SCR_Global.IsPositionWithinTerrainBounds(position))
		{
			DCO_GMCompositionService.Get().OnResult(false, "Composition placement requires valid terrain.");
			return true;
		}
		DCO_GMCompositionService.Get().Place(compositionId, position);
		return true;
	}

	protected bool RequireSelection(string verb)
	{
		if (m_iSelectedId > 0 && DCO_GMCompositionService.Get().Find(m_iSelectedId))
			return true;
		SetStatus(false, "Select a library entry before choosing " + verb + ".");
		return false;
	}

	protected void OnLibraryChanged()
	{
		if (m_iSelectedId > 0 && !DCO_GMCompositionService.Get().Find(m_iSelectedId))
			m_iSelectedId = 0;
		RefreshLibrary();
	}

	protected void OnResult(bool success, string message)
	{
		SetStatus(success, message);
	}

	protected void SetStatus(bool success, string message)
	{
		if (!m_wStatus)
			return;
		if (message.Length() > 120)
			message = message.Substring(0, 120);
		m_wStatus.SetText(message);
		m_wStatus.SetColor(DCO_GMTheme.Get().m_TextColor);
		if (!success)
			m_wStatus.SetColor(Color.FromRGBA(224, 82, 82, 255));
	}

	protected void RefreshPosition()
	{
		if (!m_wPosition)
			return;
		DCO_GMCompositionCatalogEntry selected = DCO_GMCompositionService.Get().Find(m_iSelectedId);
		if (selected)
			m_wPosition.SetText("Selected: " + selected.m_sName);
		else
			m_wPosition.SetText("Select a saved composition to place or delete.");
	}

	protected void RefreshCaptureSelection()
	{
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
		if (m_wSelection)
		{
			if (selected.IsEmpty())
				m_wSelection.SetText("No objects selected");
			else
				m_wSelection.SetText(selected.Count().ToString() + " objects selected");
		}
		SetActionEnabled("DCO_CompositionCapture", !selected.IsEmpty());
	}

	protected void SetActionEnabled(string name, bool enabled)
	{
		Widget button = m_wRoot.FindAnyWidget(name);
		if (!button)
			return;
		button.SetEnabled(enabled);
		if (enabled)
			button.SetOpacity(1);
		else
			button.SetOpacity(0.4);
	}

	protected void RefreshLibrary()
	{
		DCO_GMCompositionService.Get().GetEntries(m_aEntries);
		int pageCount = Math.Max(1, (m_aEntries.Count() + ROW_COUNT - 1) / ROW_COUNT);
		m_iPage = Math.ClampInt(m_iPage, 0, pageCount - 1);
		RefreshRows();
		ApplyLayout(false);
	}

	protected void RefreshRows()
	{
		int pageCount = Math.Max(1, (m_aEntries.Count() + ROW_COUNT - 1) / ROW_COUNT);
		bool isEmpty = m_aEntries.IsEmpty();
		if (m_wPage)
			m_wPage.SetText((m_iPage + 1).ToString() + " / " + pageCount.ToString());
		if (m_wLibrarySummary)
		{
			if (m_aEntries.Count() == 1)
				m_wLibrarySummary.SetText("1 saved");
			else
				m_wLibrarySummary.SetText(m_aEntries.Count().ToString() + " saved");
		}
		if (m_wEmpty)
			m_wEmpty.SetVisible(isEmpty);
		if (m_wScroll)
			m_wScroll.SetVisible(!isEmpty);
		if (m_wPagination)
			m_wPagination.SetVisible(pageCount > 1);
		SetActionEnabled("DCO_CompositionPrev", m_iPage > 0);
		SetActionEnabled("DCO_CompositionNext", m_iPage < pageCount - 1);
		bool hasSelection = m_iSelectedId > 0 && DCO_GMCompositionService.Get().Find(m_iSelectedId);
		SetActionEnabled("DCO_CompositionPlace", hasSelection);
		SetActionEnabled("DCO_CompositionDelete", hasSelection);
		RefreshPosition();
		for (int rowIndex = 0; rowIndex < ROW_COUNT; rowIndex++)
		{
			ButtonWidget button = m_aRowButtons[rowIndex];
			TextWidget label = m_aRowLabels[rowIndex];
			ImageWidget background = m_aRowBackgrounds[rowIndex];
			int entryIndex = m_iPage * ROW_COUNT + rowIndex;
			if (!button || !label || entryIndex >= m_aEntries.Count())
			{
				if (button)
					button.SetVisible(false);
				continue;
			}
			button.SetVisible(true);
			DCO_GMCompositionCatalogEntry entry = m_aEntries[entryIndex];
			label.SetText(entry.m_sName);
			if (m_aRowMetadata[rowIndex])
				m_aRowMetadata[rowIndex].SetText(string.Format("%1  |  %2  |  %3 objects", entry.m_sCategory, entry.m_sAuthor, entry.m_iItemCount));
			label.SetColor(DCO_GMTheme.Get().m_TextColor);
			if (background)
				background.SetColor(Color.FromRGBA(28, 31, 36, 250));
			if (entry.m_iId == m_iSelectedId)
			{
				label.SetColor(DCO_GMTheme.Get().m_AccentColor);
				if (background)
					background.SetColor(Color.FromRGBA(42, 53, 62, 255));
			}
		}
	}

	protected void SelectRow(int visibleRow)
	{
		int entryIndex = m_iPage * ROW_COUNT + visibleRow;
		if (entryIndex < 0 || entryIndex >= m_aEntries.Count())
			return;
		DCO_GMCompositionCatalogEntry entry = m_aEntries[entryIndex];
		if (!entry)
			return;
		m_iSelectedId = entry.m_iId;
		SetStatus(true, "Selected '" + entry.m_sName + "'. Choose PLACE IN WORLD, then click terrain.");
		RefreshRows();
	}
}
