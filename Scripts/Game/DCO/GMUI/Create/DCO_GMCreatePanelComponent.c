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
	static const int FAC_SLOTS = 7;
	static const int CUSTOM_FAC_SLOTS = 6;
	static const int CUSTOM_FAC_TAB = FAC_SLOTS - 1;
	static const ResourceName WORKSHOP_ICONS = "{3262679C50EF4F01}UI/Textures/Icons/icons_wrapperUI.imageset";
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
	protected ref array<FactionKey> m_FacSlotKeys = {};
	protected Widget m_wCustomFactionDrop;
	protected ref array<Widget> m_CustomFacHosts = {};
	protected ref array<ButtonWidget> m_CustomFacBtns = {};
	protected ref array<TextWidget> m_CustomFacLabels = {};
	protected ref array<ImageWidget> m_CustomFacIcons = {};

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
	protected ref array<FactionKey> m_CustomFactionKeys = {};
	protected int m_CustomFactionPage;
	protected int m_iCustomFactionTab = -1;
	protected bool m_bCustomFactionOpen;

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
	protected bool m_bCatalogSubscribed;
	protected bool m_bBrowserSubscribed;
	protected SCR_ContentBrowserEditorComponent m_Browser;
	protected bool m_bShown;
	protected bool m_bSearchFocused;
	protected bool m_bAnimationFxTargeting;
	protected bool m_bArsenalAccessTargeting;

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
		SCR_EntityCatalogManagerComponent.GetOnEntityCatalogInitialized().Insert(OnEntityCatalogInitialized);
		m_bCatalogSubscribed = true;

		GetGame().GetCallqueue().CallLater(PollSearch, 400, true);
	}

	void Shutdown()
	{
		GetGame().GetCallqueue().Remove(PollSearch);
		GetGame().GetCallqueue().Remove(ShowHoverPreview);
		GetGame().GetCallqueue().Remove(BarDragTick);
		if (m_bCatalogSubscribed)
		{
			SCR_EntityCatalogManagerComponent.GetOnEntityCatalogInitialized().Remove(OnEntityCatalogInitialized);
			m_bCatalogSubscribed = false;
		}
		if (m_bBrowserSubscribed && m_Browser)
		{
			ScriptInvoker browserInvoker = m_Browser.GetOnBrowserEntriesFiltered();
			if (browserInvoker)
				browserInvoker.Remove(OnBrowserEntriesFiltered);
			m_bBrowserSubscribed = false;
		}
		m_Browser = null;
		m_bAnimationFxTargeting = false;
		m_bArsenalAccessTargeting = false;
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
				m_FacSlotKeys.Insert("");
			}
		}

		m_wCustomFactionDrop = m_wBrowser.FindAnyWidget("DCO_CustomFactionDrop");
		for (int customIndex = 0; customIndex < CUSTOM_FAC_SLOTS; customIndex++)
		{
			m_CustomFacHosts.Insert(m_wBrowser.FindAnyWidget(string.Format("DCO_CustomFac_%1_Host", customIndex)));
			ButtonWidget customButton = BindButton(string.Format("DCO_CustomFac_%1", customIndex));
			m_CustomFacBtns.Insert(customButton);
			m_CustomFacLabels.Insert(TextWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_CustomFac_%1_Label", customIndex))));
			m_CustomFacIcons.Insert(ImageWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_CustomFac_%1_Icon", customIndex))));
		}
		if (m_wCustomFactionDrop)
			m_wCustomFactionDrop.SetVisible(false);
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
		{
			SyncAnimationFxTargeting();
			SyncArsenalAccessTargeting();
			Refresh();
		}
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
		EnsureBrowserSubscription();
		if (!m_Catalog)
			m_Catalog = new DCO_PlacementCatalog();
		m_bCatalogReady = m_Catalog.Build();

		if (!m_Budget)
		{
			m_Budget = new DCO_GMBudgetReadout();
			m_Budget.Init(m_wBrowser);
		}

		m_Catalog.GetFactionKeys(m_FactionKeys);
		BuildFactionTabs();

		HighlightCategory();
		HighlightFaction();
	}

	protected void EnsureBrowserSubscription()
	{
		SCR_ContentBrowserEditorComponent current = SCR_ContentBrowserEditorComponent.Cast(
			SCR_ContentBrowserEditorComponent.GetInstance(SCR_ContentBrowserEditorComponent, false));
		if (current == m_Browser)
			return;
		if (m_bBrowserSubscribed && m_Browser)
		{
			ScriptInvoker oldInvoker = m_Browser.GetOnBrowserEntriesFiltered();
			if (oldInvoker)
				oldInvoker.Remove(OnBrowserEntriesFiltered);
		}
		m_Browser = current;
		m_bBrowserSubscribed = false;
		if (m_Browser)
		{
			ScriptInvoker newInvoker = m_Browser.GetOnBrowserEntriesFiltered();
			if (newInvoker)
			{
				newInvoker.Insert(OnBrowserEntriesFiltered);
				m_bBrowserSubscribed = true;
			}
		}
	}

	protected void OnBrowserEntriesFiltered()
	{
		RefreshCatalogIfChanged();
	}

	protected void OnEntityCatalogInitialized()
	{
		DCO_FactionCatalog.Invalidate();
		if (!m_wBrowser)
			return;
		BuildCatalog();
		if (m_bShown)
			Refresh();
	}

	protected void RefreshCatalogIfChanged()
	{
		if (!m_Catalog || !m_Catalog.RefreshIfChanged())
			return;
		m_bCatalogReady = m_Catalog.IsBuilt();
		m_Catalog.GetFactionKeys(m_FactionKeys);
		BuildFactionTabs();
		Refresh();
	}

	protected void BuildFactionTabs()
	{
		m_CustomFactionKeys.Clear();
		m_iCustomFactionTab = CUSTOM_FAC_TAB;
		for (int resetIndex = 0; resetIndex < m_FacSlotKeys.Count(); resetIndex++)
			m_FacSlotKeys[resetIndex] = "";

		array<FactionKey> canonical = {};
		foreach (FactionKey key : m_FactionKeys)
		{
			if (DCO_FactionCatalog.IsCanonical(key))
				canonical.Insert(key);
			else
				m_CustomFactionKeys.Insert(key);
		}

		for (int i = 0; i < m_FacBtns.Count(); i++)
		{
			ButtonWidget button = m_FacBtns[i];
			TextWidget label = m_FacLabels[i];
			ImageWidget icon = ImageWidget.Cast(m_wBrowser.FindAnyWidget(string.Format("DCO_Fac_%1_Icon", i)));
			if (i < m_FacIcons.Count())
				m_FacIcons[i] = null;

			if (i == 0)
			{
				m_FacSlotKeys[i] = "";
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

			if (i == CUSTOM_FAC_TAB)
			{
				bool iconLoaded = icon && icon.LoadImageFromSet(0, WORKSHOP_ICONS, "modIcon");
				if (icon)
				{
					icon.SetVisible(iconLoaded);
					icon.SetColor(Color.FromRGBA(255, 255, 255, 255));
					icon.SetSize(22, 22);
				}
				if (i < m_FacIcons.Count())
				{
					if (iconLoaded)
						m_FacIcons[i] = icon;
					else
						m_FacIcons[i] = null;
				}
				if (label)
				{
					label.SetText(string.Format("CUSTOM (%1)", m_CustomFactionKeys.Count()));
					label.SetColor(DCO_GMTheme.Get().m_AccentColor);
					label.SetVisible(!iconLoaded);
				}
				if (button)
					button.SetVisible(!m_CustomFactionKeys.IsEmpty());
				continue;
			}

			int canonicalIndex = i - 1;
			if (canonicalIndex < canonical.Count())
			{
				FactionKey canonicalKey = canonical[canonicalIndex];
				m_FacSlotKeys[i] = canonicalKey;
				if (label)
				{
					label.SetText(BoundDisplay(m_Catalog.GetFactionTabLabel(canonicalKey), 12));
					label.SetColor(m_Catalog.GetFactionColor(canonicalKey));
					label.SetVisible(true);
				}
				BindFactionIcon(i, canonicalKey, icon, label);
				if (button)
					button.SetVisible(true);
				continue;
			}

			if (button)
				button.SetVisible(false);
			if (icon)
				icon.SetVisible(false);
		}

		if (m_CustomFactionKeys.IsEmpty())
			SetCustomFactionOpen(false);
		BindCustomFactionPage();
	}

	protected void BindFactionIcon(int slot, FactionKey key, ImageWidget icon, TextWidget label)
	{
		if (!icon)
			return;
		string iconSource;
		if (!DCO_App6Icons.SetFactionIcon(icon, key, iconSource))
		{
			icon.SetVisible(false);
			return;
		}
		icon.SetColor(Color.FromRGBA(255, 255, 255, 255));
		icon.SetSize(22, 22);
		icon.SetVisible(true);
		if (label)
			label.SetVisible(false);
		if (slot >= 0 && slot < m_FacIcons.Count())
			m_FacIcons[slot] = icon;
	}

	protected int CustomFactionPageSize()
	{
		if (m_CustomFactionKeys.Count() > CUSTOM_FAC_SLOTS)
			return CUSTOM_FAC_SLOTS - 1;
		return CUSTOM_FAC_SLOTS;
	}

	protected int CustomFactionPageCount()
	{
		return Math.Max(1, (m_CustomFactionKeys.Count() + CustomFactionPageSize() - 1) / CustomFactionPageSize());
	}

	protected bool IsCustomFactionMoreSlot(int slot)
	{
		return m_CustomFactionKeys.Count() > CUSTOM_FAC_SLOTS && slot == CUSTOM_FAC_SLOTS - 1;
	}

	protected void BindCustomFactionPage()
	{
		int pageCount = CustomFactionPageCount();
		if (m_CustomFactionPage >= pageCount)
			m_CustomFactionPage = 0;
		int offset = m_CustomFactionPage * CustomFactionPageSize();
		for (int i = 0; i < m_CustomFacBtns.Count(); i++)
		{
			ButtonWidget button = m_CustomFacBtns[i];
			TextWidget label = m_CustomFacLabels[i];
			ImageWidget icon = m_CustomFacIcons[i];
			if (IsCustomFactionMoreSlot(i))
			{
				if (m_CustomFacHosts[i])
					m_CustomFacHosts[i].SetVisible(true);
				if (label)
					label.SetText(BoundDisplay(string.Format("MORE FACTIONS  ·  %1/%2", m_CustomFactionPage + 1, pageCount), 34));
				if (icon)
					icon.SetVisible(false);
				if (button)
					button.SetVisible(true);
				continue;
			}

			int factionIndex = offset + i;
			if (factionIndex >= m_CustomFactionKeys.Count())
			{
				if (m_CustomFacHosts[i])
					m_CustomFacHosts[i].SetVisible(false);
				if (button)
					button.SetVisible(false);
				if (icon)
					icon.SetVisible(false);
				continue;
			}
			FactionKey key = m_CustomFactionKeys[factionIndex];
			if (m_CustomFacHosts[i])
				m_CustomFacHosts[i].SetVisible(true);
			if (label)
			{
				label.SetText(BoundDisplay(string.Format("%1  ·  %2", DCO_FactionCatalog.NameFor(key), key), 48));
				label.SetColor(m_Catalog.GetFactionColor(key));
				label.SetVisible(true);
			}
			if (icon)
			{
				string source;
				if (DCO_App6Icons.SetFactionIcon(icon, key, source))
				{
					icon.SetColor(Color.FromRGBA(255, 255, 255, 255));
					icon.SetVisible(true);
				}
				else
					icon.SetVisible(false);
			}
			if (button)
				button.SetVisible(true);
		}
	}

	protected void SetCustomFactionOpen(bool open)
	{
		m_bCustomFactionOpen = open && !m_CustomFactionKeys.IsEmpty();
		if (m_wCustomFactionDrop)
			m_wCustomFactionDrop.SetVisible(m_bCustomFactionOpen);
	}

	// Re-run the query and repaint from the top.
	void Refresh()
	{
		if (!m_bCatalogReady || !m_Catalog)
			return;
		m_QueryRows = m_Catalog.Query(m_Category, m_Faction, m_Search);
		if (m_QueryRows.IsEmpty() && !m_Faction.IsEmpty() && !m_FactionKeys.Contains(m_Faction) && m_wBudgetLine)
			m_wBudgetLine.SetText("FACTION CONTENT UNAVAILABLE  ·  CHOOSE ANOTHER FACTION");
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
		head.SetText(BoundDisplay(crumb, 34));
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
			bool animationFxActive = m_bAnimationFxTargeting && DCO_PlacementCatalog.IsAnimationFxResource(row.m_Prefab);
			bool arsenalAccessActive = m_bArsenalAccessTargeting && DCO_PlacementCatalog.IsArsenalAccessResource(row.m_Prefab);
			bool targetingActive = animationFxActive || arsenalAccessActive;
			if (btn) btn.SetVisible(true);
			if (row.m_bHeader)
			{
				if (lbl)
				{
					lbl.SetText(BoundDisplay(FolderLabel(row), 48));
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
					lbl.SetText(BoundDisplay(Indent(row.m_Depth) + row.m_Label, 48));	// items line up under their folder.
					if (targetingActive)
						lbl.SetColor(theme.m_AccentColor);
					else
						lbl.SetColor(theme.m_TextColor);
				}
				if (bud)
				{
					if (animationFxActive)
						bud.SetText("SELECT AI");
					else if (arsenalAccessActive)
						bud.SetText("TARGET");
					else
						bud.SetText(row.m_BudgetText);
				}
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
						if (targetingActive)
							ico.SetColor(theme.m_AccentColor);
						else
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
				{
					fold.SetRotation(0);
					fold.SetColor(theme.m_AccentColor);
					if (targetingActive)
						fold.SetOpacity(1.0);
					else
						fold.SetOpacity(0.0);
				}
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
				m_RowBudgets[0].SetText("");
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
				if (i == m_iCustomFactionTab)
				{
					SetCustomFactionOpen(!m_bCustomFactionOpen);
					HighlightFaction();
					return true;
				}
				SelectFaction(m_FacSlotKeys[i]);
				Refresh();
				return true;
			}
		}
		for (int customIndex = 0; customIndex < m_CustomFacBtns.Count(); customIndex++)
		{
			if (w != m_CustomFacBtns[customIndex])
				continue;
			if (IsCustomFactionMoreSlot(customIndex))
			{
				m_CustomFactionPage = (m_CustomFactionPage + 1) % CustomFactionPageCount();
				BindCustomFactionPage();
				return true;
			}
			int factionIndex = m_CustomFactionPage * CustomFactionPageSize() + customIndex;
			if (factionIndex >= 0 && factionIndex < m_CustomFactionKeys.Count())
			{
				SelectFaction(m_CustomFactionKeys[factionIndex]);
				SetCustomFactionOpen(false);
				Refresh();
			}
			return true;
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
		if (DCO_PlacementCatalog.IsAnimationFxResource(row.m_Prefab))
		{
			m_Catalog.CancelPlacement();
			DCO_ArsenalAccessPlacement.Get().Cancel();
			DCO_AIAnimationFxTool.Get().BeginTargeting();
			ShowPlacementStatus("SELECT AI", row.m_Label, "");
			return;
		}
		if (DCO_PlacementCatalog.IsArsenalAccessResource(row.m_Prefab))
		{
			m_Catalog.CancelPlacement();
			DCO_AIAnimationFxTool.Get().Cancel();
			DCO_ArsenalAccessPlacement.Get().BeginTargeting();
			ShowPlacementStatus("SELECT OBJECT", row.m_Label, "");
			return;
		}
		DCO_AIAnimationFxTool.Get().Cancel();
		DCO_ArsenalAccessPlacement.Get().Cancel();
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
			m_wBudgetLine.SetText(BoundDisplay(verb + ": " + label, 56));
		else
			m_wBudgetLine.SetText(BoundDisplay(string.Format("%1: %2  (cost %3)", verb, label, budgetText), 56));
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
			m_wHoverName.SetText(BoundDisplay(row.m_Label, 34));

		WorkspaceWidget ws = GetGame().GetWorkspace();
		ButtonWidget btn = m_RowBtns[r];
		if (!ws || !btn)
			return;
		float sx, sy;
		btn.GetScreenPos(sx, sy);
		float px = ws.DPIUnscale(sx) - HOVER_W - 10;
		float py = ws.DPIUnscale(sy) - HOVER_H * 0.35;
		float rootX, rootY, rootW, rootH;
		m_wRoot.GetScreenPos(rootX, rootY);
		m_wRoot.GetScreenSize(rootW, rootH);
		rootX = ws.DPIUnscale(rootX);
		rootY = ws.DPIUnscale(rootY);
		rootW = ws.DPIUnscale(rootW);
		rootH = ws.DPIUnscale(rootH);
		px = Math.Clamp(px, rootX, Math.Max(rootX, rootX + rootW - HOVER_W));
		py = Math.Clamp(py, rootY, Math.Max(rootY, rootY + rootH - HOVER_H));
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

	protected void SelectFaction(FactionKey key)
	{
		m_Faction = key;
		HighlightFaction();
	}

	protected string BoundDisplay(string value, int maxChars)
	{
		if (maxChars < 4 || value.Length() <= maxChars)
			return value;
		return value.Substring(0, maxChars - 3) + "...";
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
	protected void HighlightFaction()
	{
		DCO_GMTheme theme = DCO_GMTheme.Get();
		for (int i = 0; i < m_FacBtns.Count(); i++)
		{
			bool selected = m_FacSlotKeys[i] == m_Faction;
			if (i == m_iCustomFactionTab)
				selected = !m_Faction.IsEmpty() && m_CustomFactionKeys.Contains(m_Faction);
			ImageWidget ic = null;
			if (i < m_FacIcons.Count())
				ic = m_FacIcons[i];
			if (ic)
			{
				if (selected)
					ic.SetOpacity(1.0);
				else
					ic.SetOpacity(0.45);
				continue;
			}
			TextWidget lbl = m_FacLabels[i];
			if (!lbl)
				continue;
			if (selected)
			{
				lbl.SetColor(theme.m_AccentColor);
				continue;
			}
			if (i == 0)
			{
				lbl.SetColor(theme.m_TextColor);
				continue;
			}
			if (i == m_iCustomFactionTab)
				lbl.SetColor(theme.m_AccentColor);
			else if (!m_FacSlotKeys[i].IsEmpty())
				lbl.SetColor(m_Catalog.GetFactionColor(m_FacSlotKeys[i]));
		}
	}

	protected void PollSearch()
	{
		if (!m_bShown || !m_wSearch)
			return;
		bool targetingChanged = SyncAnimationFxTargeting();
		targetingChanged = SyncArsenalAccessTargeting() || targetingChanged;
		EnsureBrowserSubscription();
		string cur = m_wSearch.GetText();
		if (cur == m_LastSearch)
		{
			if (targetingChanged)
				Repaint();
			return;
		}
		m_LastSearch = cur;
		m_Search = cur;
		Refresh();
	}

	// Keeps the catalog row aligned with the targeting tool even when targeting
	// ends through world selection, Escape, or a modal handoff.
	protected bool SyncAnimationFxTargeting()
	{
		bool targeting = DCO_AIAnimationFxTool.Get().IsTargeting();
		if (targeting == m_bAnimationFxTargeting)
			return false;
		m_bAnimationFxTargeting = targeting;
		if (!targeting && m_wBudgetLine)
			m_wBudgetLine.SetText("READY  ·  SELECT ASSET");
		return true;
	}

	void RefreshAnimationFxIndicator()
	{
		if (SyncAnimationFxTargeting())
			Repaint();
	}

	// Arsenal Access uses the same persistent row treatment as Animations FX,
	// with object-specific copy so the GM can see what the next world click does.
	protected bool SyncArsenalAccessTargeting()
	{
		bool targeting = DCO_ArsenalAccessPlacement.Get().IsTargeting();
		if (targeting == m_bArsenalAccessTargeting)
			return false;
		m_bArsenalAccessTargeting = targeting;
		if (!targeting && m_wBudgetLine)
			m_wBudgetLine.SetText("READY  ·  SELECT ASSET");
		return true;
	}

	void RefreshArsenalAccessIndicator()
	{
		if (SyncArsenalAccessTargeting())
			Repaint();
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
