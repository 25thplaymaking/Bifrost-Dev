//! Bottom tile strip of the gunsmith: one panel, two contents. Attachment candidates for a
//! selected hardpoint, or the weapon browser with its class chip row. Owns only widgets and
//! tile lifecycle; what a tile means and what clicking it does stays with the screen.
class GRSA_TileStrip
{
	protected static const ResourceName TILE_LAYOUT = "{AC7DEE9615659D4F}UI/layouts/Menus/ArmoryV2/GRSA_AttachmentTile.layout";
	protected static const ResourceName CLASS_CHIP_LAYOUT = "{ABEECB4C3A6F5958}UI/layouts/Menus/Armory/GRSA_CategoryButton.layout";
	protected static const float PANEL_HEIGHT = 188;
	protected static const float PANEL_HEIGHT_WITH_CLASSES = 228;
	protected static const float TILE_SPAN = 134;

	protected Widget m_wPanel;
	protected TextWidget m_wTitle;
	protected Widget m_wList;
	protected SizeLayoutWidget m_wContentSize;
	protected Widget m_wClasses;
	protected FrameWidget m_wCarouselViewport;
	protected GRSA_CarouselComponent m_Carousel;
	protected SCR_ButtonTextComponent m_PreviousButton;
	protected SCR_ButtonTextComponent m_NextButton;
	protected string m_sContentKey;

	protected ref array<GRSA_ItemRowComponent> m_aRows = {};
	protected ref array<SCR_ButtonTextComponent> m_aClassChips = {};

	//! (GRSA_ItemRowComponent row) tile activated.
	ref ScriptInvoker m_OnTileClicked = new ScriptInvoker();

	//! (int index) class chip activated.
	ref ScriptInvoker m_OnClassClicked = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	void GRSA_TileStrip(Widget screenRoot)
	{
		if (!screenRoot)
			return;

		m_wPanel = screenRoot.FindAnyWidget("CandidatesPanel");
		m_wTitle = TextWidget.Cast(screenRoot.FindAnyWidget("CandidatesTitle"));
		m_wList = screenRoot.FindAnyWidget("CandidatesList");
		m_wContentSize = SizeLayoutWidget.Cast(screenRoot.FindAnyWidget("CandidatesContentSize"));
		m_wClasses = screenRoot.FindAnyWidget("BrowserClasses");
		m_wCarouselViewport = FrameWidget.Cast(screenRoot.FindAnyWidget("CandidatesScroll"));
		if (m_wCarouselViewport)
			m_Carousel = GRSA_CarouselComponent.Cast(m_wCarouselViewport.FindHandler(GRSA_CarouselComponent));

		m_PreviousButton = SCR_ButtonTextComponent.GetButtonText("CarouselStartIndicator", screenRoot);
		if (m_PreviousButton)
			m_PreviousButton.m_OnClicked.Insert(OnPreviousClicked);
		m_NextButton = SCR_ButtonTextComponent.GetButtonText("CarouselEndIndicator", screenRoot);
		if (m_NextButton)
			m_NextButton.m_OnClicked.Insert(OnNextClicked);
	}

	//------------------------------------------------------------------------------------------------
	bool IsShown()
	{
		return m_wPanel && m_wPanel.IsVisible();
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds the tile row; the entry matching markedPrefab gets markedText as its state.
	void ShowTiles(string title, notnull array<GRSA_ItemEntry> items, ResourceName markedPrefab, string markedText, bool usesSupplies)
	{
		if (!m_wPanel || !m_wList)
			return;

		bool sameContent = title == m_sContentKey;
		int restoreStart;
		if (sameContent && m_Carousel)
			restoreStart = m_Carousel.GetStartIndex();
		m_sContentKey = title;

		GetGame().GetCallqueue().Remove(FinishCarouselLayout);
		ClearRows();
		m_wPanel.SetVisible(true);
		if (m_wTitle)
			m_wTitle.SetText(title);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int markedIndex = -1;
		foreach (GRSA_ItemEntry item : items)
		{
			if (!item)
				continue;

			Widget tileRoot = workspace.CreateWidgets(TILE_LAYOUT, m_wList);
			if (!tileRoot)
				continue;

			GRSA_ItemRowComponent row = GRSA_ItemRowComponent.Cast(tileRoot.FindHandler(GRSA_ItemRowComponent));
			if (!row)
			{
				tileRoot.RemoveFromHierarchy();
				continue;
			}

			row.SetEntry(item, usesSupplies);
			if (!markedPrefab.IsEmpty() && item.m_Prefab == markedPrefab)
			{
				row.SetStateText(markedText);
				markedIndex = m_aRows.Count();
			}
			row.m_OnEntryClicked.Insert(OnRowClicked);
			row.m_OnEntryFocused.Insert(OnRowFocused);
			m_aRows.Insert(row);
		}

		GetGame().GetCallqueue().CallLater(FinishCarouselLayout, 0, false, markedIndex, restoreStart);
	}

	//------------------------------------------------------------------------------------------------
	//! Title-only state, e.g. the magazine hardpoint's pointer to the soldier loadout.
	void ShowMessage(string title)
	{
		if (!m_wPanel)
			return;

		ClearRows();
		m_sContentKey = string.Empty;
		if (m_Carousel)
			m_Carousel.Clear();
		m_wPanel.SetVisible(true);
		if (m_wTitle)
			m_wTitle.SetText(title);
	}

	//------------------------------------------------------------------------------------------------
	//! Builds the class chip row and grows the panel to fit it; chips stay until HideClasses.
	void ShowClasses(notnull array<string> labels, int selected)
	{
		ClearClasses();
		if (!m_wClasses)
			return;

		m_wClasses.SetVisible(true);
		SetPanelHeight(PANEL_HEIGHT_WITH_CLASSES);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		foreach (string label : labels)
		{
			Widget chipRoot = workspace.CreateWidgets(CLASS_CHIP_LAYOUT, m_wClasses);
			if (!chipRoot)
				continue;

			SCR_ButtonTextComponent chip = SCR_ButtonTextComponent.Cast(chipRoot.FindHandler(SCR_ButtonTextComponent));
			if (!chip)
			{
				chipRoot.RemoveFromHierarchy();
				continue;
			}

			chip.SetText(label);
			chip.m_OnClicked.Insert(OnChipClicked);
			m_aClassChips.Insert(chip);
		}

		SelectClass(selected);
	}

	//------------------------------------------------------------------------------------------------
	void SelectClass(int index)
	{
		foreach (int i, SCR_ButtonTextComponent chip : m_aClassChips)
		{
			if (chip)
				chip.SetToggled(i == index, true, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	void HideClasses()
	{
		ClearClasses();
		if (m_wClasses)
			m_wClasses.SetVisible(false);
		SetPanelHeight(PANEL_HEIGHT);
	}

	//------------------------------------------------------------------------------------------------
	//! Seeds pad focus on the first tile when a drill-in opens the strip.
	void FocusFirstTile()
	{
		if (m_aRows.IsEmpty())
			return;

		GRSA_ItemRowComponent first = m_aRows[0];
		if (first && first.GetRootWidget())
		{
			RevealRow(first);
			GetGame().GetWorkspace().SetFocusedWidget(first.GetRootWidget());
		}
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		GetGame().GetCallqueue().Remove(FinishCarouselLayout);
		ClearRows();
		HideClasses();
		m_sContentKey = string.Empty;
		if (m_Carousel)
			m_Carousel.Clear();
		if (m_wPanel)
			m_wPanel.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetPanelHeight(float height)
	{
		SizeLayoutWidget panel = SizeLayoutWidget.Cast(m_wPanel);
		if (panel)
			panel.SetHeightOverride(height);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowClicked(GRSA_ItemRowComponent row)
	{
		RevealRow(row);
		m_OnTileClicked.Invoke(row);
	}

	//------------------------------------------------------------------------------------------------
	void SetCounts(notnull map<ResourceName, int> counts)
	{
		foreach (GRSA_ItemRowComponent row : m_aRows)
		{
			if (!row || !row.GetEntry())
				continue;

			int count;
			counts.Find(row.GetEntry().m_Prefab, count);
			if (count > 0)
				row.SetStateText(string.Format("PACKED x%1", count));
			else
				row.SetStateText("ADD");
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetTitle(string title)
	{
		if (m_wTitle)
			m_wTitle.SetText(title);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRowFocused(GRSA_ItemRowComponent row)
	{
		RevealRow(row);
	}

	//------------------------------------------------------------------------------------------------
	protected void RevealRow(GRSA_ItemRowComponent row)
	{
		if (!m_Carousel || !row)
			return;

		int index = m_aRows.Find(row);
		if (index >= 0)
			m_Carousel.GlideToIndex(index);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPreviousClicked(SCR_ButtonBaseComponent button)
	{
		if (m_Carousel)
			m_Carousel.GlidePage(-1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnNextClicked(SCR_ButtonBaseComponent button)
	{
		if (m_Carousel)
			m_Carousel.GlidePage(1);
	}

	//------------------------------------------------------------------------------------------------
	//! Layout must commit dynamic tile widths before screen-space centering is measured.
	protected void FinishCarouselLayout(int markedIndex, int restoreStart)
	{
		if (m_wContentSize)
		{
			m_wContentSize.SetWidthOverride(Math.Max(1, m_aRows.Count() * TILE_SPAN));
			m_wContentSize.Update();
		}

		HorizontalLayoutWidget list = HorizontalLayoutWidget.Cast(m_wList);
		if (list)
			list.Update();

		if (m_wCarouselViewport)
			m_wCarouselViewport.Update();
		if (m_Carousel)
			m_Carousel.Configure(m_aRows.Count(), TILE_SPAN, restoreStart);

		if (markedIndex >= 0 && markedIndex < m_aRows.Count())
			RevealRow(m_aRows[markedIndex]);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnChipClicked(SCR_ButtonBaseComponent button)
	{
		int index = m_aClassChips.Find(SCR_ButtonTextComponent.Cast(button));
		if (index >= 0)
			m_OnClassClicked.Invoke(index);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearRows()
	{
		foreach (GRSA_ItemRowComponent row : m_aRows)
		{
			if (row && row.GetRootWidget())
				row.GetRootWidget().RemoveFromHierarchy();
		}
		m_aRows.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearClasses()
	{
		m_aClassChips.Clear();
		if (!m_wClasses)
			return;

		Widget child = m_wClasses.GetChildren();
		while (child)
		{
			Widget next = child.GetSibling();
			child.RemoveFromHierarchy();
			child = next;
		}
	}
}
