// Bifrost GM CREATE-panel UI binder.

class DCO_CreatePanelButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMCreatePanelComponent m_Owner;

	void DCO_CreatePanelButtonHandler(DCO_GMCreatePanelComponent owner)
	{
		m_Owner = owner;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnButton(w);
		return false;
	}

// Delays item previews until hover settles.
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_Owner)
			m_Owner.OnRowEnter(w);
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (m_Owner)
			m_Owner.OnRowLeave(w);
		return false;
	}

	// Wheel scrolling for the CREATE list.
	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!m_Owner)
			return false;
		if (wheel > 0)
			m_Owner.ScrollBy(-3);	// wheel up = toward the top of the list.
		else
			m_Owner.ScrollBy(3);
		return true;
	}

	// Scrollbar thumb DRAG.
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnBarMouseDown(w, button);
		return false;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnBarMouseUp(w, button);
		return false;
	}
}

// A bare EditBoxWidget does not perform the engine widget-library focus handoff on its own.
class DCO_CreateSearchHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMCreatePanelComponent m_Owner;

	void DCO_CreateSearchHandler(DCO_GMCreatePanelComponent owner)
	{
		m_Owner = owner;
	}

	override bool OnChange(Widget w, bool finished)
	{
		if (m_Owner)
			m_Owner.OnSearchChanged(finished);
		return false;
	}

	override bool OnFocus(Widget w, int x, int y)
	{
		if (m_Owner)
			m_Owner.OnSearchFocus(true);
		return false;
	}

	override bool OnFocusLost(Widget w, int x, int y)
	{
		if (m_Owner)
			m_Owner.OnSearchFocus(false);
		return false;
	}
}

class DCO_GMCreatePanelComponent
{
	static const int ROWS = 22;
	static const int FAC_SLOTS = 7;	// index 0 = ALL, 1..6 = faction pool.
	static const ResourceName FOLD_DOWN  = "{A3EE9DF5A7573679}img/icons/fold-down.edds";	// expanded section header.
	static const ResourceName FOLD_RIGHT = "{12D85C56D7B4F8AA}img/icons/fold-right.edds";	// collapsed section header.

	// Level-2 (type) folder glyph.
	static const ResourceName FOLDER_ICON = "{654E5E2DEB8E91DD}img/icons/folder.edds";

	static const ResourceName CAT_ICON_MEN = "{77FA6B0CE121AC03}img/icons/tabs/tab_MEN.edds";
	static const ResourceName CAT_ICON_GRP = "{A4728F10D4A4FE05}img/icons/tabs/tab_GRP.edds";
	static const ResourceName CAT_ICON_OBJ = "{6ED61BE4DFDE2F74}img/icons/tabs/tab_OBJ.edds";
	static const ResourceName CAT_ICON_SYS = "{D14C8FED5DF8FFDE}img/icons/tabs/tab_SYS.edds";
	static const ResourceName CAT_ICON_FX = "{D6B46B6655BC3FD5}UI/Textures/Editor/ContextMenu/ContextAction_LightningStrike.edds";

	static const int HOVER_DELAY_MS = 450;
	static const float HOVER_W = 210;
	static const float HOVER_H = 234;

	protected Widget m_wRoot;
	protected Widget m_wBrowser;
	protected ref DCO_PlacementCatalog m_Catalog;
	protected ref DCO_CreatePanelButtonHandler m_Handler;
	protected ref DCO_CreateSearchHandler m_SearchHandler;
	protected ref DCO_GMBudgetReadout m_Budget;

	protected EditBoxWidget m_wSearch;
	protected TextWidget m_wBudgetLine;

	protected ref array<ButtonWidget> m_CatBtns = {};
	protected ref array<int> m_CatValues = {};
	protected ref array<ImageWidget> m_CatIcons = {};	// aligned with m_CatBtns; null where the layout icon is missing.
	protected ref array<ButtonWidget> m_FacBtns = {};
	protected ref array<TextWidget> m_FacLabels = {};
	protected ref array<ImageWidget> m_FacIcons = {};	// aligned with m_FacBtns; null for the ALL slot.

	protected ref array<ButtonWidget> m_RowBtns = {};
	protected ref array<TextWidget> m_RowLabels = {};
	protected ref array<TextWidget> m_RowBudgets = {};
	protected ref array<ImageWidget> m_RowIcons = {};
	protected ref array<ResourceName> m_RowLoadedIcons = {};
	protected ref array<ImageWidget> m_RowFold = {};

	protected Widget m_wHover;
	protected ImageWidget m_wHoverImg;
	protected TextWidget m_wHoverName;
	protected int m_HoverRow = -1;	// row index the pending/showing preview belongs to.

	protected ref array<ref DCO_CatalogRow> m_QueryRows = {};
	protected ref array<FactionKey> m_FactionKeys = {};
	protected int m_FactionPage;

	protected int m_Category = DCO_PlacementCatalog.CAT_ALL;
	protected FactionKey m_Faction = "";
	protected string m_Search = "";
	protected string m_LastSearch = "";
	protected ResourceName m_LastPlacedPrefab;	// last placement successfully handed to the native placing component.
	protected string m_LastPlacedLabel;
	protected string m_LastPlacedBudgetText;
	protected int m_ScrollOffset;	// index of the first visible row — the row pool is a window onto m_QueryRows.
	protected int m_TotalRows;	// last Repaint's row count; clamps the offset and sizes the scrollbar.

	protected Widget m_wBar;
	protected Widget m_wBarSpacer;
	protected Widget m_wBarThumb;

	// Thumb DRAG state.
	protected bool m_bBarDragging;
	protected int m_BarStartMouseY;	// cursor Y at drag start, native px.
	protected int m_BarStartOffset;	// m_ScrollOffset at drag start.

	static const float BAR_W = 5;	// bar width in reference px — must match the layout's Size.
	static const float BAR_MIN_THUMB = 24;	// keep the thumb grabbable/visible on very long lists.
	protected bool m_bCatalogReady;
	protected bool m_bShown;
	protected bool m_bSearchFocused;

	void Init(Widget shellRoot)
	{
		if (!shellRoot)
			return;
		m_wRoot = shellRoot;
		m_wBrowser = shellRoot.FindAnyWidget("DCO_CreateBrowser");
		if (!m_wBrowser)
		{
			Print("[DCO-GM] CREATE panel: DCO_CreateBrowser not found (layout not reloaded yet?)", LogLevel.WARNING);
			return;	// new layout not loaded this session; nothing to bind.
		}

		m_Handler = new DCO_CreatePanelButtonHandler(this);
		BindControls();
		BindRows();
		BindHover(shellRoot);
		m_wBrowser.SetVisible(false);	// controller shows it on the CREATE tab.

		GetGame().GetCallqueue().CallLater(PollSearch, 400, true);

		int boundRows = 0;
		foreach (ButtonWidget rb : m_RowBtns)
		{
			if (rb)
				boundRows++;
		}
		Print(string.Format("[DCO-GM] CREATE panel bound (rows=%1/%2 cat=%3 fac=%4 search=%5 hover=%6)",
			boundRows, m_RowBtns.Count(), m_CatBtns.Count(), m_FacBtns.Count(), m_wSearch != null, m_wHover != null), LogLevel.NORMAL);
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(PollSearch);
		GetGame().GetCallqueue().Remove(ShowHoverPreview);
		GetGame().GetCallqueue().Remove(BarDragTick);
		GetGame().GetInputManager().RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.PRESSED, OnSearchCancel);
		ReleaseSearchFocus();
		HideHover();
		if (m_Budget)
		{
			m_Budget.Shutdown();
			m_Budget = null;
		}
		m_SearchHandler = null;
	}

	protected void BindControls()
	{
		m_wSearch     = EditBoxWidget.Cast(m_wBrowser.FindAnyWidget("DCO_Search"));
		m_wBar        = m_wBrowser.FindAnyWidget("DCO_ListBar");
		m_wBarSpacer  = m_wBrowser.FindAnyWidget("DCO_ListBar_Spacer");
		m_wBarThumb   = m_wBrowser.FindAnyWidget("DCO_ListBar_Thumb");
		m_wBudgetLine = TextWidget.Cast(m_wBrowser.FindAnyWidget("DCO_BudgetLine"));
		if (m_wSearch)
		{
			m_SearchHandler = new DCO_CreateSearchHandler(this);
			m_wSearch.AddHandler(m_SearchHandler);
		}
		if (m_wBudgetLine)
			m_wBudgetLine.SetText("READY  ·  SELECT ASSET");
		if (m_wBarThumb)
			m_wBarThumb.AddHandler(m_Handler);

		// Stable category buttons.
		array<string> catNames = {"DCO_Cat_ALL", "DCO_Cat_MEN", "DCO_Cat_GRP", "DCO_Cat_OBJ", "DCO_Cat_SYS", "DCO_Cat_FX"};
		array<string> catIconNames = {"DCO_Cat_ALL_Icon", "DCO_Cat_MEN_Icon", "DCO_Cat_GRP_Icon", "DCO_Cat_OBJ_Icon", "DCO_Cat_SYS_Icon", "DCO_Cat_FX_Icon"};
		array<int> catVals = {DCO_PlacementCatalog.CAT_ALL, DCO_PlacementCatalog.CAT_MAN, DCO_PlacementCatalog.CAT_GROUP, DCO_PlacementCatalog.CAT_OBJECT, DCO_PlacementCatalog.CAT_MODULE, DCO_PlacementCatalog.CAT_EFFECTS};
		for (int i = 0; i < catNames.Count(); i++)
		{
			ButtonWidget b = BindButton(catNames[i]);
			if (!b)
				continue;
			m_CatBtns.Insert(b);
			m_CatValues.Insert(catVals[i]);

			ImageWidget ic = ImageWidget.Cast(m_wBrowser.FindAnyWidget(catIconNames[i]));
			TextWidget lbl = TextWidget.Cast(b.FindAnyWidget(catNames[i] + "_Label"));
			bool loaded = false;
			ResourceName tabTex = CategoryIconTexture(catVals[i]);
			if (ic && !tabTex.IsEmpty())
				loaded = ic.LoadImageTexture(0, tabTex);
			if (ic && loaded)
			{
				ic.SetVisible(true);
				if (lbl)
					lbl.SetVisible(false);
			}
			else
			{
				if (ic)
					ic.SetVisible(false);
				ic = null;	// no icon for this tab -> highlighting falls back to the label.
				if (lbl)
					lbl.SetVisible(true);
				if (!tabTex.IsEmpty())
					Print("[DCO-GM] CREATE: category glyph failed to load for " + catNames[i] + " (text fallback shown)", LogLevel.WARNING);
			}
			m_CatIcons.Insert(ic);
		}

		for (int i = 0; i < FAC_SLOTS; i++)
		{
			ButtonWidget b = BindButton(string.Format("DCO_Fac_%1", i));
			TextWidget lbl = TextWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_Fac_%1_Label", i)));
			if (b)
			{
				m_FacBtns.Insert(b);
				m_FacLabels.Insert(lbl);
				m_FacIcons.Insert(null);	// filled in BuildCatalog once the faction list exists.
			}
		}
	}

	protected ResourceName CategoryIconTexture(int category)
	{
		switch (category)
		{
			case DCO_PlacementCatalog.CAT_ALL: return FOLDER_ICON;
			case DCO_PlacementCatalog.CAT_MAN: return CAT_ICON_MEN;
			case DCO_PlacementCatalog.CAT_GROUP: return CAT_ICON_GRP;
			case DCO_PlacementCatalog.CAT_OBJECT: return CAT_ICON_OBJ;
			case DCO_PlacementCatalog.CAT_MODULE: return CAT_ICON_SYS;
			case DCO_PlacementCatalog.CAT_EFFECTS: return CAT_ICON_FX;
		}
		return ResourceName.Empty;
	}

	protected void BindRows()
	{
		for (int i = 0; i < ROWS; i++)
		{
			ButtonWidget b = BindButton(string.Format("DCO_Row_%1", i));
			TextWidget lbl = TextWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_Row_%1_Label", i)));
			TextWidget bud = TextWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_Row_%1_Budget", i)));
			ImageWidget ico = ImageWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_Row_%1_Icon", i)));
			ImageWidget fold = ImageWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_Row_%1_Fold", i)));
			m_RowBtns.Insert(b);
			m_RowLabels.Insert(lbl);
			m_RowBudgets.Insert(bud);
			m_RowIcons.Insert(ico);
			m_RowLoadedIcons.Insert(ResourceName.Empty);
			m_RowFold.Insert(fold);
		}
	}

	protected void BindHover(Widget shellRoot)
	{
		m_wHover = shellRoot.FindAnyWidget("DCO_HoverPreview");
		if (!m_wHover)
		{
			Print("[DCO-GM] CREATE: DCO_HoverPreview not found (layout not reloaded yet?)", LogLevel.WARNING);
			return;
		}
		m_wHoverImg = ImageWidget.Cast(m_wHover.FindAnyWidget("DCO_HoverPreview_Img"));
		m_wHoverName = TextWidget.Cast(m_wHover.FindAnyWidget("DCO_HoverPreview_Name"));
		m_wHover.SetVisible(false);
	}

	protected ButtonWidget BindButton(string name)
	{
		ButtonWidget b = ButtonWidget.Cast(m_wBrowser.FindAnyWidget(name));
		if (b)
			b.AddHandler(m_Handler);
		return b;
	}

	// Show/hide the whole CREATE browser; builds the catalog lazily on first show.
	void Show(bool show)
	{
		m_bShown = show;
		if (!show)
		{
			ReleaseSearchFocus();
			GetGame().GetCallqueue().Remove(ShowHoverPreview);
			HideHover();
		}
		if (!m_wBrowser)
			return;
		m_wBrowser.SetVisible(show);
		if (show && !m_bCatalogReady)
			BuildCatalog();
		if (show)
			Refresh();
	}

	// Repeats the last placement through the native placement path.
	bool RepeatLastPlacement()
	{
		if (!m_Catalog || m_LastPlacedPrefab.IsEmpty())
			return false;
		if (!m_Catalog.Place(m_LastPlacedPrefab))
			return false;
		ShowPlacementStatus("REDEPLOYING", m_LastPlacedLabel, m_LastPlacedBudgetText);
		return true;
	}

	void ShowRedeployPrompt()
	{
		if (m_wBudgetLine)
			m_wBudgetLine.SetText("SELECT ASSET  ·  THEN USE REDEPLOY");
	}

	protected void BuildCatalog()
	{
		m_Catalog = new DCO_PlacementCatalog();
		m_Catalog.Build();
		m_bCatalogReady = true;

		m_Budget = new DCO_GMBudgetReadout();
		m_Budget.Init(m_wBrowser);

		m_Catalog.GetFactionKeys(m_FactionKeys);
		m_FactionPage = 0;
		BindFactionPage();
		if (m_FactionKeys.Count() > FAC_SLOTS - 1)
			Print(string.Format("[DCO-GM] CREATE: %1 factions across %2 switchable pages", m_FactionKeys.Count(), FactionPageCount()), LogLevel.NORMAL);

		HighlightCategory();
		HighlightFaction(0);	// default selection = ALL; dims the unselected faction logos.
	}

	protected int FactionPageSize()
	{
		if (m_FactionKeys.Count() > FAC_SLOTS - 1)
			return FAC_SLOTS - 2;	// ALL + five factions + MORE.
		return FAC_SLOTS - 1;
	}

	protected int FactionPageCount()
	{
		int size = FactionPageSize();
		if (size <= 0)
			return 1;
		return Math.Max(1, (m_FactionKeys.Count() + size - 1) / size);
	}

	protected bool IsFactionMoreSlot(int slot)
	{
		return m_FactionKeys.Count() > FAC_SLOTS - 1 && slot == FAC_SLOTS - 1;
	}

	protected void BindFactionPage()
	{
		int pageSize = FactionPageSize();
		int offset = m_FactionPage * pageSize;
		for (int i = 0; i < m_FacBtns.Count(); i++)
		{
			ButtonWidget button = m_FacBtns[i];
			TextWidget label = m_FacLabels[i];
			ImageWidget icon = ImageWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_Fac_%1_Icon", i)));
			if (i < m_FacIcons.Count())
				m_FacIcons[i] = null;

			if (i == 0)
			{
				if (label)
				{
					label.SetText("ALL");
					label.SetVisible(true);
				}
				if (icon)
					icon.SetVisible(false);
				if (button)
					button.SetVisible(true);
				continue;
			}

			if (IsFactionMoreSlot(i))
			{
				if (icon)
					icon.SetVisible(false);
				if (label)
				{
					label.SetText(string.Format("%1/%2  >", m_FactionPage + 1, FactionPageCount()));
					label.SetColor(DCO_GMTheme.Get().m_AccentColor);
					label.SetVisible(true);
				}
				if (button)
					button.SetVisible(true);
				continue;
			}

			int factionIndex = offset + i - 1;
			if (factionIndex < 0 || factionIndex >= m_FactionKeys.Count())
			{
				if (button)
					button.SetVisible(false);
				if (icon)
					icon.SetVisible(false);
				continue;
			}

			FactionKey key = m_FactionKeys[factionIndex];
			if (label)
			{
				label.SetText(m_Catalog.GetFactionTabLabel(key));
				label.SetColor(m_Catalog.GetFactionColor(key));
				label.SetVisible(true);
			}
			if (icon)
			{
				string iconSource;
				if (DCO_App6Icons.SetFactionIcon(icon, key, iconSource))
				{
					icon.SetColor(Color.FromRGBA(255, 255, 255, 255));
					icon.SetSize(22, 22);
					icon.SetVisible(true);
					Print(string.Format("[DCO-GM] faction tab icon: key=%1 source=%2", key, iconSource), LogLevel.NORMAL);
					if (label)
						label.SetVisible(false);
					if (i < m_FacIcons.Count())
						m_FacIcons[i] = icon;
				}
				else
					icon.SetVisible(false);
			}
			if (button)
				button.SetVisible(true);
		}
	}

	// Re-run the query and repaint from the top.
	void Refresh()
	{
		if (!m_bCatalogReady || !m_Catalog)
			return;
		m_QueryRows = m_Catalog.Query(m_Category, m_Faction, m_Search);
// Allows empty categories when the active faction provides no matching assets.
		if (m_QueryRows.IsEmpty() && !m_Faction.IsEmpty() && m_Search.IsEmpty())
		{
			m_Faction = "";
			HighlightFaction(0);
			m_QueryRows = m_Catalog.Query(m_Category, m_Faction, m_Search);
		}
		m_ScrollOffset = 0;
		UpdateHeaderCrumb();
		Repaint();
	}

	// Shows the active category and faction.
	protected void UpdateHeaderCrumb()
	{
		TextWidget head = TextWidget.Cast(m_wBrowser.FindAnyWidget("DCO_CreateHeader"));
		if (!head)
			return;
		string crumb = "CREATE";
		if (m_Category != DCO_PlacementCatalog.CAT_ALL)
		{
			string cl = DCO_PlacementCatalog.CategoryLabel(m_Category);
			cl.ToUpper();
			crumb = crumb + " · " + cl;
		}
		if (!m_Faction.IsEmpty())
		{
			string fk = m_Faction;
			fk.ToUpper();
			crumb = crumb + " · " + fk;
		}
		head.SetText(crumb);
	}

	void RevealByName(string name)
	{
		if (!m_wBrowser)
			return;
		if (!m_bCatalogReady)
			BuildCatalog();
		m_Category = DCO_PlacementCatalog.CAT_ALL;
		m_Search = name;
		m_LastSearch = name;	// keep PollSearch in sync so it doesn't fight this on the next tick.
		if (m_wSearch)
			m_wSearch.SetText(name);
		HighlightCategory();	// clear any active tab tint - reveal drops the category filter to ALL.
		Refresh();
	}

	// Depth indent for the folder tree.
	protected string Indent(int depth)
	{
		string s = "";
		for (int d = 0; d < depth; d++)
		{
			s = s + "   ";
		}
		return s;
	}

	// Folder label = indent + "Name · count".
	protected string FolderLabel(DCO_CatalogRow row)
	{
		return Indent(row.m_Depth) + row.m_Label;
	}

	protected ResourceName FolderIcon(DCO_CatalogRow row)
	{
		if (row.m_Level == 0)
			return CategoryIconTexture(row.m_Category);
		if (row.m_Level == 1)
			return DCO_App6Icons.FactionIcon(row.m_Faction);
		return FOLDER_ICON;
	}

	// Size the scrollbar from the scroll state.
	protected void UpdateScrollBar()
	{
		if (!m_wBar)
			return;
		bool needed = m_TotalRows > ROWS;
		m_wBar.SetVisible(needed);
		if (!needed)
			return;

		float trackH = BarHeightRef();
		if (trackH < 1)
			return;	// not laid out yet — the next repaint sizes it.

		float frac = ROWS / (float)m_TotalRows;
		float thumbH = Math.Max(BAR_MIN_THUMB, trackH * frac);
		int maxOff = m_TotalRows - ROWS;
		float t = 0;
		if (maxOff > 0)
			t = m_ScrollOffset / (float)maxOff;

		ImageWidget sp = ImageWidget.Cast(m_wBarSpacer);
		if (sp)
			sp.SetSize(BAR_W, (trackH - thumbH) * t);
		ImageWidget th = ImageWidget.Cast(m_wBarThumb);
		if (th)
			th.SetSize(BAR_W, thumbH);
	}

	// Bar height in REFERENCE px.
	protected float BarHeightRef()
	{
		if (!m_wBar)
			return 0;
		float w, h;
		m_wBar.GetScreenSize(w, h);
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (ws)
			return ws.DPIUnscale(h);
		return h;
	}

	// Keep the visible window inside the row array.
	protected void ClampScroll()
	{
		int maxOff = m_TotalRows - ROWS;
		if (maxOff < 0)
			maxOff = 0;
		if (m_ScrollOffset > maxOff)
			m_ScrollOffset = maxOff;
		if (m_ScrollOffset < 0)
			m_ScrollOffset = 0;
	}

	void ScrollBy(int rows)
	{
		int before = m_ScrollOffset;
		m_ScrollOffset += rows;
		ClampScroll();
		if (m_ScrollOffset != before)
			Repaint();	// nothing re-queries; only the window moved.
	}

	bool OnBarMouseDown(Widget w, int button)
	{
		if (button != 0 || !m_wBarThumb || w != m_wBarThumb || m_TotalRows <= ROWS)
			return false;
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		m_BarStartMouseY = my;
		m_BarStartOffset = m_ScrollOffset;
		m_bBarDragging = true;
		GetGame().GetCallqueue().Remove(BarDragTick);
		GetGame().GetCallqueue().CallLater(BarDragTick, 0, true);
		return true;
	}

	bool OnBarMouseUp(Widget w, int button)
	{
		if (!m_bBarDragging)
			return false;
		StopBarDrag();
		return true;
	}

	protected void BarDragTick()
	{
		if (!m_bBarDragging || !m_wBar || m_TotalRows <= ROWS)
		{
			StopBarDrag();
			return;
		}
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		int rawDy = my - m_BarStartMouseY;
		float dy = rawDy;
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (ws)
			dy = ws.DPIUnscale(rawDy);

		float trackH = BarHeightRef();
		int maxOff = m_TotalRows - ROWS;
		float frac = ROWS / (float)m_TotalRows;
		float thumbH = Math.Max(BAR_MIN_THUMB, trackH * frac);
		float range = trackH - thumbH;
		if (range < 1 || maxOff <= 0)
		{
			StopBarDrag();
			return;
		}
		int newOffset = m_BarStartOffset + Math.Round((dy / range) * maxOff);
		if (newOffset != m_ScrollOffset)
		{
			m_ScrollOffset = newOffset;
			ClampScroll();
			Repaint();
		}
	}

	protected void StopBarDrag()
	{
		m_bBarDragging = false;
		GetGame().GetCallqueue().Remove(BarDragTick);
	}

	// Paint the current scroll window into the row pool.
	protected void Repaint()
	{
		GetGame().GetCallqueue().Remove(ShowHoverPreview);	// rows are about to change under the mouse.
		HideHover();

		int total = m_QueryRows.Count();
		m_TotalRows = total;
		ClampScroll();

		DCO_GMTheme theme = DCO_GMTheme.Get();
		for (int r = 0; r < ROWS; r++)
		{
			int idx = m_ScrollOffset + r;
			ButtonWidget btn = m_RowBtns[r];
			TextWidget lbl = m_RowLabels[r];
			TextWidget bud = m_RowBudgets[r];
			if (idx >= total)
			{
				if (btn) btn.SetVisible(false);
				continue;
			}
			DCO_CatalogRow row = m_QueryRows[idx];
			ImageWidget ico = m_RowIcons[r];
			ImageWidget fold = m_RowFold[r];
			if (btn) btn.SetVisible(true);
			if (row.m_bHeader)
			{
				if (lbl)
				{
					lbl.SetText(FolderLabel(row));
					lbl.SetColor(theme.m_AccentColor);
				}
				if (bud) bud.SetText("");
				if (ico)
				{
					bool factionIconLoaded;
					if (row.m_Level == 1)
					{
						string factionIconSource;
						factionIconLoaded = DCO_App6Icons.SetFactionIcon(ico, row.m_Faction, factionIconSource);
					}
					ResourceName fico;
					if (!factionIconLoaded)
						fico = FolderIcon(row);
					if (factionIconLoaded || !fico.IsEmpty())
					{
						if (!factionIconLoaded && m_RowLoadedIcons[r] != fico)
						{
							ico.LoadImageTexture(0, fico);
							m_RowLoadedIcons[r] = fico;
						}
						if (factionIconLoaded)
							m_RowLoadedIcons[r] = ResourceName.Empty;
						if (row.m_Level == 1)
							ico.SetColor(Color.FromRGBA(255, 255, 255, 255));
						else
							ico.SetColor(theme.m_AccentColor);
						ico.SetVisible(true);
					}
					else
					{
						m_RowLoadedIcons[r] = ResourceName.Empty;
						ico.SetVisible(false);
					}
				}
				if (fold)
				{
					float foldRot = 0;
					if (!row.m_bCollapsed)
						foldRot = 90;
					fold.SetRotation(foldRot);
					fold.SetColor(theme.m_AccentColor);
					fold.SetOpacity(1.0);
				}
			}
			else
			{
				if (lbl)
				{
					lbl.SetText(Indent(row.m_Depth) + row.m_Label);	// items line up under their folder.
					lbl.SetColor(theme.m_TextColor);
				}
				if (bud) bud.SetText(row.m_BudgetText);
				if (ico)
				{
					ResourceName app6 = row.m_App6Icon;
					ResourceName useIcon = row.m_Icon;
					if (!app6.IsEmpty())
						useIcon = app6;
					if (!useIcon.IsEmpty())
					{
						if (m_RowLoadedIcons[r] != useIcon)
						{
							ico.LoadImageTexture(0, useIcon);
							m_RowLoadedIcons[r] = useIcon;
						}
						ico.SetColor(Color.FromRGBA(255, 255, 255, 255));
						ico.SetVisible(true);
					}
					else
					{
						m_RowLoadedIcons[r] = ResourceName.Empty;
						ico.SetVisible(false);
					}
				}
				if (fold)
					fold.SetOpacity(0.0);
			}
		}

		// Empty is a state, not an invisible list.
		if (total == 0 && !m_RowBtns.IsEmpty())
		{
			ButtonWidget emptyRow = m_RowBtns[0];
			if (emptyRow)
				emptyRow.SetVisible(true);
			if (!m_RowLabels.IsEmpty() && m_RowLabels[0])
			{
				m_RowLabels[0].SetText("NO ASSETS MATCH THESE FILTERS");
				m_RowLabels[0].SetColor(theme.m_MutedColor);
			}
			if (!m_RowBudgets.IsEmpty() && m_RowBudgets[0])
				m_RowBudgets[0].SetText("CLEAR SEARCH OR CHOOSE ALL");
			if (!m_RowIcons.IsEmpty() && m_RowIcons[0])
				m_RowIcons[0].SetVisible(false);
			if (!m_RowFold.IsEmpty() && m_RowFold[0])
				m_RowFold[0].SetOpacity(0.0);
		}

		UpdateScrollBar();
	}

	bool OnButton(Widget w)
	{
		// A click elsewhere in CREATE is an explicit handoff away from text entry.
		ReleaseSearchFocus();

		// Category?
		for (int i = 0; i < m_CatBtns.Count(); i++)
		{
			if (w == m_CatBtns[i])
			{
				m_Category = m_CatValues[i];
				HighlightCategory();
				Refresh();
				return true;
			}
		}
		// Faction?
		for (int i = 0; i < m_FacBtns.Count(); i++)
		{
			if (w == m_FacBtns[i])
			{
				if (IsFactionMoreSlot(i))
				{
					m_FactionPage = (m_FactionPage + 1) % FactionPageCount();
					BindFactionPage();
					HighlightFaction(-1);
					return true;
				}
				SelectFaction(i);
				Refresh();
				return true;
			}
		}
		for (int r = 0; r < m_RowBtns.Count(); r++)
		{
			if (w == m_RowBtns[r])
			{
				OnRowClicked(r);
				return true;
			}
		}
		return false;
	}

	protected void OnRowClicked(int r)
	{
		GetGame().GetCallqueue().Remove(ShowHoverPreview);
		HideHover();
		int idx = m_ScrollOffset + r;
		if (idx < 0 || idx >= m_QueryRows.Count())
			return;
		DCO_CatalogRow row = m_QueryRows[idx];
		if (row.m_bHeader)
		{
			m_Catalog.ToggleSection(row.m_SectionKey);
			int keep = m_ScrollOffset;
			m_QueryRows = m_Catalog.Query(m_Category, m_Faction, m_Search);
			m_ScrollOffset = keep;
			Repaint();
			return;
		}
		if (m_Catalog.Place(row.m_Prefab))
		{
			m_LastPlacedPrefab = row.m_Prefab;
			m_LastPlacedLabel = row.m_Label;
			m_LastPlacedBudgetText = row.m_BudgetText;
			ShowPlacementStatus("PLACING", row.m_Label, row.m_BudgetText);
		}
	}

	protected void ShowPlacementStatus(string verb, string label, string budgetText)
	{
		if (!m_wBudgetLine)
			return;
		if (budgetText.IsEmpty())
			m_wBudgetLine.SetText(verb + ": " + label);
		else
			m_wBudgetLine.SetText(string.Format("%1: %2  (cost %3)", verb, label, budgetText));
		m_wBudgetLine.SetOpacity(0.25);
		AnimateWidget.Opacity(m_wBudgetLine, 1.0, 8.0, true);
	}

	void OnRowEnter(Widget w)
	{
		if (!m_bShown || !m_wHover)
			return;
		for (int r = 0; r < m_RowBtns.Count(); r++)
		{
			if (w == m_RowBtns[r])
			{
				m_HoverRow = r;
				GetGame().GetCallqueue().Remove(ShowHoverPreview);
				GetGame().GetCallqueue().CallLater(ShowHoverPreview, HOVER_DELAY_MS, false);
				return;
			}
		}
	}

	void OnRowLeave(Widget w)
	{
		if (m_HoverRow < 0)
			return;
		if (m_HoverRow < m_RowBtns.Count() && w == m_RowBtns[m_HoverRow])
		{
			m_HoverRow = -1;
			GetGame().GetCallqueue().Remove(ShowHoverPreview);
			HideHover();
		}
	}

	// Shows the hovered entity preview.
	protected void ShowHoverPreview()
	{
		int r = m_HoverRow;
		if (r < 0 || !m_wHover || !m_wHoverImg)
			return;
		int idx = m_ScrollOffset + r;
		if (idx < 0 || idx >= m_QueryRows.Count())
			return;
		DCO_CatalogRow row = m_QueryRows[idx];
		if (row.m_bHeader)
			return;

		ResourceName img = row.m_Icon;
		string ext;
		FilePath.StripExtension(img, ext);
		if (img.IsEmpty() || ext == "imageset")	// no quad name available for a set - use the APP-6 symbol instead.
			img = row.m_App6Icon;
		if (img.IsEmpty())
			return;
		if (!m_wHoverImg.LoadImageTexture(0, img))
			return;
		if (m_wHoverName)
			m_wHoverName.SetText(row.m_Label);

		WorkspaceWidget ws = GetGame().GetWorkspace();
		ButtonWidget btn = m_RowBtns[r];
		if (!ws || !btn)
			return;
		float sx, sy;
		btn.GetScreenPos(sx, sy);
		float px = ws.DPIUnscale(sx) - HOVER_W - 10;
		float py = ws.DPIUnscale(sy) - HOVER_H * 0.35;
		if (px < 0)
			px = 0;
		if (py < 0)
			py = 0;
		FrameSlot.SetAnchor(m_wHover, 0, 0);
		FrameSlot.SetAlignment(m_wHover, 0, 0);
		FrameSlot.SetSize(m_wHover, HOVER_W, HOVER_H);
		FrameSlot.SetPos(m_wHover, px, py);
		m_wHover.SetVisible(true);
	}

	protected void HideHover()
	{
		if (m_wHover)
			m_wHover.SetVisible(false);
	}

	protected void SelectFaction(int facIdx)
	{
		if (facIdx == 0)
		{
			m_Faction = "";	// ALL.
		}
		else
		{
			int fidx = m_FactionPage * FactionPageSize() + facIdx - 1;
			if (fidx >= 0 && fidx < m_FactionKeys.Count())
				m_Faction = m_FactionKeys[fidx];
			else
				return;
		}
		HighlightFaction(facIdx);
	}

	// Selected category glyph -> accent tint; others grey.
	protected void HighlightCategory()
	{
		DCO_GMTheme theme = DCO_GMTheme.Get();
		for (int i = 0; i < m_CatBtns.Count(); i++)
		{
			Color c = DCO_GMTheme.Get().m_MutedColor;
			if (m_CatValues[i] == m_Category)
				c = theme.m_AccentColor;
			ImageWidget ic = null;
			if (i < m_CatIcons.Count())
				ic = m_CatIcons[i];
			if (ic)
				ic.SetColor(c);
			TextWidget lbl = TextWidget.Cast(m_CatBtns[i].FindAnyWidget(GetCatLabelName(i)));
			if (lbl)
				lbl.SetColor(c);
		}
	}

	protected string GetCatLabelName(int i)
	{
		array<string> names = {"DCO_Cat_ALL_Label", "DCO_Cat_MEN_Label", "DCO_Cat_GRP_Label", "DCO_Cat_OBJ_Label", "DCO_Cat_SYS_Label", "DCO_Cat_FX_Label"};
		if (i >= 0 && i < names.Count())
			return names[i];
		return "";
	}

	// Uses opacity so faction artwork keeps its authored colour.
	protected void HighlightFaction(int selIdx)
	{
		DCO_GMTheme theme = DCO_GMTheme.Get();
		for (int i = 0; i < m_FacBtns.Count(); i++)
		{
			ImageWidget ic = null;
			if (i < m_FacIcons.Count())
				ic = m_FacIcons[i];
			if (ic)
			{
				if (i == selIdx)
					ic.SetOpacity(1.0);
				else
					ic.SetOpacity(0.45);
				continue;
			}
			TextWidget lbl = m_FacLabels[i];
			if (!lbl)
				continue;
			if (i == selIdx)
			{
				lbl.SetColor(theme.m_AccentColor);
				continue;
			}
			if (i == 0)
			{
				lbl.SetColor(theme.m_TextColor);
				continue;
			}
			int fidx = m_FactionPage * FactionPageSize() + i - 1;
			if (fidx >= 0 && fidx < m_FactionKeys.Count())
				lbl.SetColor(m_Catalog.GetFactionColor(m_FactionKeys[fidx]));
		}
	}

	protected void PollSearch()
	{
		if (!m_bShown || !m_wSearch)
			return;
		string cur = m_wSearch.GetText();
		if (cur == m_LastSearch)
			return;
		m_LastSearch = cur;
		m_Search = cur;
		Refresh();
	}

	// EditBox final-change is Enter on keyboard.
	void OnSearchChanged(bool finished)
	{
		PollSearch();
		if (finished)
			ReleaseSearchFocus();
	}

	void OnSearchFocus(bool focused)
	{
		if (m_bSearchFocused == focused)
			return;
		m_bSearchFocused = focused;
		if (focused)
		{
			GetGame().GetInputManager().AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.PRESSED, OnSearchCancel);
		}
		else
		{
			GetGame().GetInputManager().RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.PRESSED, OnSearchCancel);
		}
	}

	protected void OnSearchCancel()
	{
		// Clear the Back action before focus is handed to the GM shell.
		InputManager input = GetGame().GetInputManager();
		if (input)
			input.SetActionValue(UIConstants.MENU_ACTION_BACK, 0);
		ReleaseSearchFocus();
	}

	protected void ReleaseSearchFocus()
	{
		if (!m_wSearch || (!m_bSearchFocused && !m_wSearch.IsInWriteMode()))
			return;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.SetFocusedWidget(null, true);
		m_bSearchFocused = false;
		GetGame().GetInputManager().RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.PRESSED, OnSearchCancel);
	}
}
