// Full-screen Bifrost loadout editor.
class DCO_ArsBtnHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMArsenalPanel m_Owner;
	protected int m_Id;

	void DCO_ArsBtnHandler(DCO_GMArsenalPanel owner, int id)
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

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_Owner)
			m_Owner.OnHoverBtn(m_Id, w, true);
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (m_Owner)
			m_Owner.OnHoverBtn(m_Id, w, false);
		return false;
	}

}

class DCO_ArsWheelHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMArsenalPanel m_Owner;
	protected bool m_bRight;

	void DCO_ArsWheelHandler(DCO_GMArsenalPanel owner, bool right)
	{
		m_Owner = owner;
		m_bRight = right;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		return m_Owner && m_Owner.ScrollColumn(m_bRight, wheel);
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return m_Owner && m_Owner.OnScrollBarMouseDown(m_bRight, w, button);
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		return m_Owner && m_Owner.OnScrollBarMouseUp(button);
	}
}

class DCO_ArsDynRow
{
	Widget m_Root;
	ButtonWidget m_Btn;
	ItemPreviewWidget m_Img;
	TextWidget m_Txt;
	ButtonWidget m_Bag;
	ImageWidget m_BagIco;
}

class DCO_ArsSlotCard
{
	Widget m_Root;
	ButtonWidget m_Button;
	ItemPreviewWidget m_Preview;
	TextWidget m_Label;
	TextWidget m_Name;
	IEntity m_BoundItem;
	bool m_bHasBoundItem;
}

class DCO_GMArsenalPanelStaticData
{
	ref array<string> m_CategoryNames = {
		"PRIMARY WEAPONS", "PISTOLS", "LAUNCHERS", "UNIFORMS", "VESTS", "BACKPACKS", "HEADGEAR",
		"ITEMS", "GRENADES", "MAGAZINES", "ATTACHMENTS"
	};

	ref array<ResourceName> m_CategoryIcons = {
		"{71648F15B3984B87}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_AssaultRifles.edds",
		"{2EEBBBCA36DD775F}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Pistols.edds",
		"{10840233D666C940}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Launcher.edds",
		"{92245C15E122EDB2}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Jackets.edds",
		"{2DCA69EEB8628C06}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Vests.edds",
		"{769B709DF200BF84}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Backpacks.edds",
		"{F349167C49E996FB}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Headwear.edds",
		"{CDF94F179A33CFB9}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Accessories.edds",
		"{233A8BC0520B1B8B}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Grenades.edds",
		"{A02EB3B80E276400}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_MachineGuns.edds",
		"{CB055708E982C0A5}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Optics.edds"
	};
}

class DCO_GMArsenalPanel
{
	protected static ref DCO_GMArsenalPanel s_Inst;
	protected static ref DCO_GMArsenalPanelStaticData s_StaticData;
	static DCO_GMArsenalPanel Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_GMArsenalPanel();
		return s_Inst;
	}

	protected static DCO_GMArsenalPanelStaticData StaticData()
	{
		if (!s_StaticData)
			s_StaticData = new DCO_GMArsenalPanelStaticData();
		return s_StaticData;
	}

	protected static const int CATS = 11;
	protected static const int CTX_SLOTS = 7;	// equipped buckets 0-6 (PRI..HEAD) drive the right-column context.
	protected static const int LO_ROWS = 8;
	protected static const int POLL_MS = 500;
	protected static const int PREVIEW_TICK_MS = 16;
	protected static const float CAM_DRIFT_M = 9.0;
	protected static const int MAX_DYN_ROWS = 250;	// dynamic-list creation cap; the title reports the cut, never silent.
	protected static const float BAR_WIDTH = 10.0;
	protected static const float BAR_MIN_THUMB = 72.0;
	protected static const float CARD_WIDTH = 236.0;
	protected static const float WHEEL_STEP_NORMALIZED = 0.12;

	protected static const int BTN_ROWL_BASE = 1000;
	protected static const int BTN_ROWR_BASE = 2000;
	protected static const int BTN_ROWRBAG_BASE = 3000;
	protected static const int BTN_LO_BASE   = 400;
	protected static const int BTN_CLEAR     = 54;
	protected static const int BTN_RESET     = 55;
	protected static const int BTN_CLOSE     = 56;
	protected static const int BTN_LOADOUTS  = 57;
	protected static const int BTN_LO_SAVE   = 58;
	protected static const int BTN_LO_LOAD   = 59;
	protected static const int BTN_LO_DELETE = 60;
	protected static const int BTN_LO_PREV   = 61;
	protected static const int BTN_LO_NEXT   = 62;
	protected static const int BTN_CTX_BACK  = 63;
	protected static const int BTN_HINT     = 64;
	protected static const int BTN_UNDO     = 65;
	protected static const int BTN_REDO     = 66;
	protected static const int BTN_INV_TAB   = 70;
	protected static const int BTN_BACK_OPERATOR = 71;
	protected static const int BTN_SLOT_BASE = 300;
	protected static const int BTN_CAT_BASE  = 100;

	protected static const ResourceName ICO_LOADOUTS = "{71AD1053EC53DA8D}img/icons/ars-loadouts.edds";
	protected static const ResourceName ICO_SAVE     = "{41E5BC703AF12285}img/icons/ars-save.edds";
	protected static const ResourceName ICO_LOAD     = "{3BEAB520D1630ED6}img/icons/ars-load.edds";
	protected static const ResourceName ICO_TRASH    = "{B8BC7C9B29CD1757}img/icons/ars-trash.edds";
	protected static const ResourceName ICO_CLEAR    = "{1EA3C4B6FC4F7659}img/icons/ars-clear.edds";
	protected static const ResourceName ICO_RESET    = "{5C14C357485DEC46}img/icons/ars-reset.edds";
	protected static const ResourceName ICO_DONE     = "{C07CFDFCE87A8E89}img/icons/ars-done.edds";
	protected static const ResourceName ICO_UNDO     = "{4FC7CBD31FC30A74}img/icons/ars-undo.edds";
	protected static const ResourceName ICO_REDO     = "{51A1129A81ECF31B}img/icons/ars-redo.edds";
	protected static const ResourceName ICO_HINT     = "{77933E0A31FA61A2}img/icons/ars-hint.edds";
	protected static const ResourceName ICO_NAV_L    = "{7A3CD1BF283DE603}img/icons/ars-prev.edds";
	protected static const ResourceName ICO_NAV_R    = "{97C57D6B4CCA3FFF}img/icons/ars-next.edds";
	protected static const ResourceName ICO_INV_TAB  = "{24C2C142CE0F1758}img/icons/ars-crate.edds";
	protected static const ResourceName ICO_BAG_PLUS = "{186870059CE4FE3C}img/icons/ars-bag-plus.edds";

	protected static const ResourceName ARSENAL_LAYOUT = "{1BE5479343C2F85A}UI/layouts/DCO_ArsenalScreen.layout";
	protected static const ResourceName ROW_LAYOUT = "{86E10B187E4A61C3}UI/layouts/DCO_ArsenalCard.layout";
	protected static const ResourceName SLOT_LAYOUT = "{BEF269C956EC7B27}UI/layouts/DCO_ArsenalSlotCard.layout";
	protected static const ResourceName PREVIEW_MANAGER_PREFAB = "{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et";

	protected Widget m_wShellRoot;
	protected Widget m_wOwnedRoot;
	protected Widget m_wRoot;
	protected Widget m_wScreen;
	protected Widget m_wOverviewPanel;
	protected Widget m_wItemPanel;
	protected Widget m_wLeftCol;
	protected Widget m_wRightCol;
	protected TextWidget m_wMode;
	protected TextWidget m_wTarget;
	protected TextWidget m_wLeftTitle;
	protected EditBoxWidget m_wSearch;
	protected TextWidget m_wWeight;
	protected TextWidget m_wRightTitle;
	protected TextWidget m_wStageTitle;
	protected TextWidget m_wStageHelp;
	protected Widget m_wCtxBack;
	protected TextWidget m_wCtxBackTxt;
	protected Widget m_wHintBox;
	protected Widget m_wLoPanel;
	protected EditBoxWidget m_wLoName;
	protected TextWidget m_wLoPage;
	protected ScrollLayoutWidget m_wScrollL;
	protected ScrollLayoutWidget m_wScrollR;
	protected SizeLayoutWidget m_wListSizeL;
	protected SizeLayoutWidget m_wListSizeR;
	protected Widget m_wListL;
	protected Widget m_wListR;
	protected Widget m_wBarL;
	protected Widget m_wBarR;
	protected Widget m_wBarSpacerL;
	protected Widget m_wBarSpacerR;
	protected Widget m_wBarThumbL;
	protected Widget m_wBarThumbR;
	protected Widget m_wHover;
	protected TextWidget m_wHoverTxt;
	protected Widget m_wCharacterPreviewArea;
	protected ItemPreviewWidget m_wCharacterPreview;
	protected ItemPreviewManagerEntity m_CharacterPreviewManager;
	protected PreviewRenderAttributes m_CharacterPreviewAttributes;
	protected ref SCR_InventoryCharacterWidgetHelper m_CharacterPreviewInput;
	protected IEntity m_PreviewSubject;
	protected bool m_bPreviewingItem;
	protected ref array<ImageWidget> m_CatIcons = {};
	protected ref array<ref DCO_ArsDynRow> m_DynL = {};	// dynamic rows, created on demand from ROW_LAYOUT.
	protected ref array<ref DCO_ArsDynRow> m_DynR = {};
	protected ref array<ref DCO_ArsSlotCard> m_SlotCards = {};
	protected ref array<IEntity> m_KitItems = {};	// Equipped item per bucket 0-6.
	protected ref array<ButtonWidget> m_LoRows = {};
	protected ref array<TextWidget> m_LoTxts = {};
	protected ref array<string> m_InvPrefabs = {};
	protected ref array<string> m_InvNames = {};
	protected ref array<int> m_InvCounts = {};
	protected bool m_bInvMode;
	protected ref array<ref DCO_ArsBtnHandler> m_Handlers = {};
	protected ref DCO_ArsWheelHandler m_WheelLeft;
	protected ref DCO_ArsWheelHandler m_WheelRight;
	protected ref array<Widget> m_HiddenSiblings = {};
	protected bool m_bBarDragging;
	protected bool m_bBarDragRight;
	protected int m_iBarStartMouseX;
	protected float m_fBarStartPos;
	protected int m_iListRowsL;
	protected int m_iListRowsR;
	protected bool m_bOpen;
	protected bool m_bStandalone;
	protected bool m_bOverview;
	protected bool m_bUseEditorCamera;
	protected IEntity m_Target;
	protected EDCO_ArsenalCategory m_eCatL;
	protected EDCO_ArsenalCategory m_eCatR;
	protected bool m_bLastRight;
	protected string m_sLastSearch;
	protected int m_iDetailIdx = -1;
	protected bool m_bPoolOverride;	// right tab / ALL ITEMS pressed: keep the general pool until a left pick.
	protected IEntity m_LastCtxItem;
	protected bool m_bLoOpen;
	protected int m_iLoPage;
	protected int m_iLoSelected = -1;	// index into m_LoRecs.
	protected ref array<DCO_ArsenalLoadoutRec> m_LoRecs = {};
	protected ref array<DCO_ArsenalEntry> m_FilteredL = {};
	protected ref array<DCO_ArsenalEntry> m_FilteredR = {};

	void Init(Widget shellRoot, bool standalone = false)
	{
		if (!shellRoot)
			return;
		m_wShellRoot = shellRoot;
		m_bStandalone = standalone;
		m_wRoot = shellRoot;
		if (!standalone)
		{
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			if (!workspace)
				return;
			m_wOwnedRoot = workspace.CreateWidgets(ARSENAL_LAYOUT, shellRoot);
			if (!m_wOwnedRoot)
			{
				Print("[DCO-ARS] dedicated arsenal layout failed to instantiate", LogLevel.WARNING);
				return;
			}
			m_wRoot = m_wOwnedRoot;
			FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
			FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
			FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);
		}

		m_wScreen     = m_wRoot.FindAnyWidget("DCO_ArsenalScreen");
		if (!m_wScreen && m_wRoot.GetName() == "DCO_ArsenalScreen")
			m_wScreen = m_wRoot;
		m_wOverviewPanel = m_wRoot.FindAnyWidget("DCO_ArsOverviewPanel");
		m_wItemPanel = m_wRoot.FindAnyWidget("DCO_ArsItemPanel");
		m_wLeftCol = m_wRoot.FindAnyWidget("DCO_ArsLeftCol");
		m_wRightCol = m_wRoot.FindAnyWidget("DCO_ArsRightCol");
		m_wMode       = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsMode"));
		m_wTarget     = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsTarget"));
		m_wLeftTitle  = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsTitle"));
		m_wSearch     = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsSearch"));
		m_wWeight     = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsWeight"));
		m_wRightTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsRightTitle"));
		m_wStageTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsStageTitle"));
		m_wStageHelp  = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsStageHelp"));
		m_wCtxBack    = m_wRoot.FindAnyWidget("DCO_ArsCtxBack");
		m_wCtxBackTxt = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsCtxBackTxt"));
		m_wHintBox    = m_wRoot.FindAnyWidget("DCO_ArsHintBox");
		m_wLoPanel    = m_wRoot.FindAnyWidget("DCO_ArsLoadoutPanel");
		m_wLoName     = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsLoName"));
		m_wLoPage     = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsLoPage"));
		m_wScrollL    = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsScrollL"));
		m_wScrollR    = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsScrollR"));
		m_wListSizeL  = SizeLayoutWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsListSizeL"));
		m_wListSizeR  = SizeLayoutWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsListSizeR"));
		m_wListL      = m_wRoot.FindAnyWidget("DCO_ArsListL");
		m_wListR      = m_wRoot.FindAnyWidget("DCO_ArsListR");
		m_wBarL       = m_wRoot.FindAnyWidget("DCO_ArsBarL");
		m_wBarR       = m_wRoot.FindAnyWidget("DCO_ArsBarR");
		m_wBarSpacerL = m_wRoot.FindAnyWidget("DCO_ArsBarSpacerL");
		m_wBarSpacerR = m_wRoot.FindAnyWidget("DCO_ArsBarSpacerR");
		m_wBarThumbL  = m_wRoot.FindAnyWidget("DCO_ArsBarThumbL");
		m_wBarThumbR  = m_wRoot.FindAnyWidget("DCO_ArsBarThumbR");
		m_wHover      = m_wRoot.FindAnyWidget("DCO_ArsHover");
		m_wHoverTxt   = TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsHoverTxt"));
		m_wCharacterPreviewArea = m_wRoot.FindAnyWidget("DCO_ArsCharacterPreview");
		m_wCharacterPreview = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("playerRender"));
		m_WheelLeft = new DCO_ArsWheelHandler(this, false);
		m_WheelRight = new DCO_ArsWheelHandler(this, true);
		BindWheelTree(m_wScrollL, false);
		BindWheelTree(m_wScrollR, true);
		BindWheelTree(m_wBarL, false);
		BindWheelTree(m_wBarR, true);
		if (m_wHover)
			m_wHover.SetVisible(false);
		if (m_wCharacterPreviewArea)
			m_wCharacterPreviewArea.SetVisible(false);

		for (int i = 0; i < CATS; i++)
		{
			ButtonWidget b = ButtonWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsCat" + i.ToString()));
			ImageWidget ico = ImageWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsCatIco" + i.ToString()));
			m_CatIcons.Insert(ico);
			if (b)
				AddHandler(b, BTN_CAT_BASE + i);
			if (ico)
				ico.LoadImageTexture(0, StaticData().m_CategoryIcons[i]);
		}
		LoadIcon(m_wRoot, "DCO_ArsBI_Loadouts", ICO_LOADOUTS);
		LoadIcon(m_wRoot, "DCO_ArsBI_Clear",    ICO_CLEAR);
		LoadIcon(m_wRoot, "DCO_ArsBI_Reset",    ICO_RESET);
		LoadIcon(m_wRoot, "DCO_ArsBI_Done",     ICO_DONE);
		LoadIcon(m_wRoot, "DCO_ArsBI_Undo",     ICO_UNDO);
		LoadIcon(m_wRoot, "DCO_ArsBI_Redo",     ICO_REDO);
		LoadIcon(m_wRoot, "DCO_ArsBI_Hint",     ICO_HINT);
		LoadIcon(m_wRoot, "DCO_ArsBI_LoSave",   ICO_SAVE);
		LoadIcon(m_wRoot, "DCO_ArsBI_LoLoad",   ICO_LOAD);
		LoadIcon(m_wRoot, "DCO_ArsBI_LoDelete", ICO_TRASH);
		LoadIcon(m_wRoot, "DCO_ArsBI_LoPrev",   ICO_NAV_L);
		LoadIcon(m_wRoot, "DCO_ArsBI_LoNext",   ICO_NAV_R);
		LoadIcon(m_wRoot, "DCO_ArsInvTabIco",   ICO_INV_TAB);
		BuildSlotCards();
		for (int i = 0; i < CTX_SLOTS; i++)
			m_KitItems.Insert(null);
		for (int i = 0; i < LO_ROWS; i++)
		{
			ButtonWidget lb = ButtonWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsLoRow" + i.ToString()));
			m_LoRows.Insert(lb);
			m_LoTxts.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget("DCO_ArsLoTxt" + i.ToString())));
			if (lb)
				AddHandler(lb, BTN_LO_BASE + i);
		}

		BindNamed("DCO_ArsClear", BTN_CLEAR);
		BindNamed("DCO_ArsReset", BTN_RESET);
		BindNamed("DCO_ArsClose", BTN_CLOSE);
		BindNamed("DCO_ArsLoadouts", BTN_LOADOUTS);
		BindNamed("DCO_ArsLoSave", BTN_LO_SAVE);
		BindNamed("DCO_ArsLoLoad", BTN_LO_LOAD);
		BindNamed("DCO_ArsLoDelete", BTN_LO_DELETE);
		BindNamed("DCO_ArsLoPrev", BTN_LO_PREV);
		BindNamed("DCO_ArsLoNext", BTN_LO_NEXT);
		BindNamed("DCO_ArsCtxBack", BTN_CTX_BACK);
		BindNamed("DCO_ArsHintBtn", BTN_HINT);
		BindNamed("DCO_ArsUndo", BTN_UNDO);
		BindNamed("DCO_ArsRedo", BTN_REDO);
		BindNamed("DCO_ArsInvTab", BTN_INV_TAB);
		BindNamed("DCO_ArsBackOperator", BTN_BACK_OPERATOR);

		if (m_wScreen)
			m_wScreen.SetVisible(false);
		if (m_wLoPanel)
			m_wLoPanel.SetVisible(false);
		if (m_wCtxBack)
			m_wCtxBack.SetVisible(false);
		if (m_wHintBox)
			m_wHintBox.SetVisible(false);
		m_bOpen = false;

		DCO_ArsenalLoadouts.Get().GetOnChanged().Insert(OnLoadoutsChanged);
	}

	protected void BuildSlotCards()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace || !m_wRoot)
			return;
		Widget weapons = m_wRoot.FindAnyWidget("DCO_ArsWeaponsCards");
		Widget gear = m_wRoot.FindAnyWidget("DCO_ArsGearCards");
		Widget outfit = m_wRoot.FindAnyWidget("DCO_ArsOutfitCards");
		for (int category = 0; category < CTX_SLOTS; category++)
		{
			Widget host = weapons;
			if (category == EDCO_ArsenalCategory.VEST || category == EDCO_ArsenalCategory.BACKPACK)
				host = gear;
			else if (category == EDCO_ArsenalCategory.UNIFORM || category == EDCO_ArsenalCategory.HEADGEAR)
				host = outfit;
			Widget root;
			if (host)
				root = workspace.CreateWidgets(SLOT_LAYOUT, host);
			DCO_ArsSlotCard card = new DCO_ArsSlotCard();
			card.m_Root = root;
			if (root)
			{
				card.m_Button = ButtonWidget.Cast(root.FindAnyWidget("DCO_ArsSlotButton"));
				card.m_Preview = ItemPreviewWidget.Cast(root.FindAnyWidget("DCO_ArsSlotImg"));
				card.m_Label = TextWidget.Cast(root.FindAnyWidget("DCO_ArsSlotLabel"));
				card.m_Name = TextWidget.Cast(root.FindAnyWidget("DCO_ArsSlotName"));
				if (card.m_Button)
					AddHandler(card.m_Button, BTN_SLOT_BASE + category);
				if (card.m_Label)
					card.m_Label.SetText(StaticData().m_CategoryNames[category]);
			}
			m_SlotCards.Insert(card);
		}
	}

	protected void PinFrame(string name, float aMinX, float aMinY, float aMaxX, float aMaxY)
	{
		if (!m_wRoot)
			return;
		Widget w = m_wRoot.FindAnyWidget(name);
		if (!w)
			return;
		FrameSlot.SetAnchorMin(w, aMinX, aMinY);
		FrameSlot.SetAnchorMax(w, aMaxX, aMaxY);
		FrameSlot.SetOffsets(w, 0, 0, 0, 0);
	}

	protected void AddHandler(ButtonWidget b, int id)
	{
		DCO_ArsBtnHandler h = new DCO_ArsBtnHandler(this, id);
		b.AddHandler(h);
		m_Handlers.Insert(h);
	}

	protected void BindWheelTree(Widget widget, bool right)
	{
		if (!widget)
			return;
		DCO_ArsWheelHandler handler = m_WheelLeft;
		if (right)
			handler = m_WheelRight;
		if (!handler)
			return;
		widget.AddHandler(handler);
		Widget child = widget.GetChildren();
		while (child)
		{
			BindWheelTree(child, right);
			child = child.GetSibling();
		}
	}

	bool ScrollColumn(bool right, int wheel)
	{
		if (!m_bOpen || wheel == 0)
			return false;
		ScrollLayoutWidget scroll = m_wScrollL;
		if (right)
			scroll = m_wScrollR;
		if (!scroll)
			return false;
		float x;
		float y;
		scroll.GetSliderPos(x, y);
		scroll.SetSliderPos(Math.Clamp(x - wheel * WHEEL_STEP_NORMALIZED, 0.0, 1.0), y);
		UpdateScrollBar(right);
		return true;
	}

	// Keep selection usable when a platform or nested preview widget does not forward wheel input.
	// Centering the clicked card exposes the next cards, allowing repeated clicks to traverse the rail.
	protected void RevealClickedRow(bool right, int index)
	{
		ScrollLayoutWidget scroll = m_wScrollL;
		int rowCount = m_iListRowsL;
		if (right)
		{
			scroll = m_wScrollR;
			rowCount = m_iListRowsR;
		}
		if (!scroll || index < 0 || index >= rowCount)
			return;

		float viewportW;
		float viewportH;
		scroll.GetScreenSize(viewportW, viewportH);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			viewportW = workspace.DPIUnscale(viewportW);

		float contentW = rowCount * CARD_WIDTH;
		float scrollRange = contentW - viewportW;
		if (viewportW < 1 || scrollRange <= 1)
			return;

		float cardCenter = (index + 0.5) * CARD_WIDTH;
		float targetX = Math.Clamp((cardCenter - viewportW * 0.5) / scrollRange, 0.0, 1.0);
		float currentX;
		float currentY;
		scroll.GetSliderPos(currentX, currentY);
		scroll.SetSliderPos(targetX, currentY);
		UpdateScrollBar(right);
	}

	protected void ScheduleScrollBarUpdate()
	{
		GetGame().GetCallqueue().Remove(UpdateScrollBars);
		GetGame().GetCallqueue().CallLater(UpdateScrollBars, 0, false);
	}

	protected void UpdateScrollBars()
	{
		// Dynamic visibility changes must be committed before ScrollLayout measures overflow.
		HorizontalLayoutWidget leftLayout = HorizontalLayoutWidget.Cast(m_wListL);
		HorizontalLayoutWidget rightLayout = HorizontalLayoutWidget.Cast(m_wListR);
		if (leftLayout)
			leftLayout.Update();
		if (rightLayout)
			rightLayout.Update();
		UpdateScrollBar(false);
		UpdateScrollBar(true);
	}

	protected void SetListRowCount(bool right, int rowCount)
	{
		SizeLayoutWidget sizeHost = m_wListSizeL;
		if (right)
		{
			m_iListRowsR = rowCount;
			sizeHost = m_wListSizeR;
		}
		else
			m_iListRowsL = rowCount;

		// ScrollLayout only develops a horizontal range when its direct child reports the full card width.
		if (sizeHost)
			sizeHost.SetWidthOverride(Math.Max(1.0, rowCount * CARD_WIDTH));
	}

	protected void UpdateScrollBar(bool right)
	{
		ScrollLayoutWidget scroll = m_wScrollL;
		Widget bar = m_wBarL;
		ImageWidget spacer = ImageWidget.Cast(m_wBarSpacerL);
		ImageWidget thumb = ImageWidget.Cast(m_wBarThumbL);
		int rowCount = m_iListRowsL;
		if (right)
		{
			scroll = m_wScrollR;
			bar = m_wBarR;
			spacer = ImageWidget.Cast(m_wBarSpacerR);
			thumb = ImageWidget.Cast(m_wBarThumbR);
			rowCount = m_iListRowsR;
		}
		if (!scroll || !bar || !spacer || !thumb)
			return;

		float viewportW;
		float viewportH;
		scroll.GetScreenSize(viewportW, viewportH);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			viewportW = workspace.DPIUnscale(viewportW);
		float contentW = Math.Max(1.0, rowCount * CARD_WIDTH);
		bool needed = contentW > viewportW + 1;
		bar.SetVisible(needed);
		if (!needed)
		{
			float resetX;
			float resetY;
			scroll.GetSliderPos(resetX, resetY);
			if (resetX != 0)
				scroll.SetSliderPos(0, resetY);
			return;
		}

		float trackW;
		float trackH;
		bar.GetScreenSize(trackW, trackH);
		if (workspace)
			trackW = workspace.DPIUnscale(trackW);
		if (trackW < 1 || contentW < 1)
			return;

		float thumbW = Math.Max(BAR_MIN_THUMB, trackW * Math.Min(1.0, viewportW / contentW));
		float sliderX;
		float sliderY;
		scroll.GetSliderPos(sliderX, sliderY);
		spacer.SetSize((trackW - thumbW) * sliderX, BAR_WIDTH);
		thumb.SetSize(thumbW, BAR_WIDTH);
	}

	bool OnScrollBarMouseDown(bool right, Widget widget, int button)
	{
		Widget thumb = m_wBarThumbL;
		ScrollLayoutWidget scroll = m_wScrollL;
		if (right)
		{
			thumb = m_wBarThumbR;
			scroll = m_wScrollR;
		}
		if (button != 0 || widget != thumb || !scroll)
			return false;

		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		float sliderY;
		scroll.GetSliderPos(m_fBarStartPos, sliderY);
		m_iBarStartMouseX = mouseX;
		m_bBarDragRight = right;
		m_bBarDragging = true;
		GetGame().GetCallqueue().Remove(ScrollBarDragTick);
		GetGame().GetCallqueue().CallLater(ScrollBarDragTick, 0, true);
		return true;
	}

	bool OnScrollBarMouseUp(int button)
	{
		if (button != 0 || !m_bBarDragging)
			return false;
		StopScrollBarDrag();
		return true;
	}

	protected void ScrollBarDragTick()
	{
		if (!m_bBarDragging)
			return;
		Widget bar = m_wBarL;
		Widget thumb = m_wBarThumbL;
		ScrollLayoutWidget scroll = m_wScrollL;
		if (m_bBarDragRight)
		{
			bar = m_wBarR;
			thumb = m_wBarThumbR;
			scroll = m_wScrollR;
		}
		if (!bar || !thumb || !scroll)
		{
			StopScrollBarDrag();
			return;
		}

		float trackW;
		float trackH;
		float thumbW;
		float thumbH;
		bar.GetScreenSize(trackW, trackH);
		thumb.GetScreenSize(thumbW, thumbH);
		float range = trackW - thumbW;
		if (range < 1)
		{
			StopScrollBarDrag();
			return;
		}

		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		float position = Math.Max(0.0, Math.Min(1.0, m_fBarStartPos + (mouseX - m_iBarStartMouseX) / range));
		float sliderX;
		float sliderY;
		scroll.GetSliderPos(sliderX, sliderY);
		scroll.SetSliderPos(position, sliderY);
		UpdateScrollBar(m_bBarDragRight);
	}

	protected void StopScrollBarDrag()
	{
		m_bBarDragging = false;
		GetGame().GetCallqueue().Remove(ScrollBarDragTick);
	}

	protected void LoadIcon(Widget root, string name, ResourceName tex)
	{
		ImageWidget w = ImageWidget.Cast(root.FindAnyWidget(name));
		if (!w)
			return;
		w.LoadImageTexture(0, tex);
	}

	protected void BindNamed(string name, int id)
	{
		if (!m_wRoot)
			return;
		ButtonWidget b = ButtonWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!b)
			return;
		AddHandler(b, id);
	}

	bool IsOpen()
	{
		return m_bOpen;
	}

	void OpenFor(SCR_EditableEntityComponent unit)
	{
		if (!unit)
			return;
		IEntity owner = unit.GetOwner();
		if (!owner)
			return;
		OpenForEntity(owner, TargetName(unit), false);
	}

	void OpenForEntity(IEntity owner, string targetLabel, bool useEditorCamera)
	{
		if (!m_wScreen || !owner)
			return;

		m_Target = owner;
		m_bUseEditorCamera = useEditorCamera;
		m_eCatL = EDCO_ArsenalCategory.PRIMARY;
		m_eCatR = EDCO_ArsenalCategory.MAGAZINES;
		m_bLastRight = false;
		m_sLastSearch = "";
		m_iDetailIdx = -1;
		m_bPoolOverride = false;
		m_bOverview = true;
		m_LastCtxItem = null;
		m_bLoOpen = false;
		m_iLoSelected = -1;
		m_iLoPage = 0;
		if (m_wSearch)
			m_wSearch.SetText("");
		if (m_wLoPanel)
			m_wLoPanel.SetVisible(false);
		m_bInvMode = false;
		ResetScrolls();

		DCO_ArsenalCatalog.Get().Build();

		if (m_wMode)
			m_wMode.SetText("BIFROST ARSENAL");
		if (m_wTarget)
			m_wTarget.SetText(BoundRowPart(targetLabel, 56));
		if (m_wHintBox)
			m_wHintBox.SetVisible(false);
		if (m_wHover)
			m_wHover.SetVisible(false);
		RefreshContextItems();	// fills m_KitItems FIRST - the right-column context derives from them.
		RefreshLists();
		RefreshPresentation();
		RefreshWeight();
		if (m_bUseEditorCamera)
			FocusCameraOn(owner);
		SetOpen(true);
		StartCharacterPreview(owner);
		RouteVerb(DCO_ArsenalServer.VERB_HOLD, "");
	}

	void CloseSilent()
	{
		SetOpen(false);
	}

	protected void SetOpen(bool open)
	{
		if (open == m_bOpen && open)
			return;
		bool wasOpen = m_bOpen;
		m_bOpen = open;
		HideShell(open);
		if (m_wScreen)
			m_wScreen.SetVisible(open);
		GetGame().GetCallqueue().Remove(Poll);
		if (open)
		{
			GetGame().GetCallqueue().CallLater(Poll, POLL_MS, true);
		}
		else
		{
			if (wasOpen && !m_bStandalone)
				DCO_GMUIController.ReleaseMenuFocus();
			StopScrollBarDrag();
			StopCharacterPreview();
			if (m_Target)
				RouteVerb(DCO_ArsenalServer.VERB_RELEASE, "");
			m_Target = null;
			m_iDetailIdx = -1;
			m_bLoOpen = false;
			m_FilteredL.Clear();
			m_FilteredR.Clear();
			if (m_wHover)
				m_wHover.SetVisible(false);
		}
	}

	protected void HideShell(bool hide)
	{
		if (m_bStandalone || !m_wShellRoot)
			return;
		if (hide)
		{
			m_HiddenSiblings.Clear();
			Widget c = m_wShellRoot.GetChildren();
			while (c)
			{
				if (c != m_wScreen && c.IsVisible())
				{
					m_HiddenSiblings.Insert(c);
					c.SetVisible(false);
				}
				c = c.GetSibling();
			}
			return;
		}
		foreach (Widget w : m_HiddenSiblings)
		{
			if (!w)
				continue;
			string wName = w.GetName();
			if (wName == "DCO_TacticsPanel")
			{
				if (DCO_GMTacticsPanel.Get().IsOpen())
					w.SetVisible(true);
				continue;
			}
			if (wName == "DCO_GizmoPanel" || wName == "DCO_SimPanel" || wName == "DCO_ContextMenu" || wName == "DCO_HoverPreview")
				continue;
			w.SetVisible(true);
		}
		m_HiddenSiblings.Clear();
	}

	protected void Poll()
	{
		if (!m_bStandalone && DCO_GMTheme.Get().IsMasterHidden())
			return;
		if (!m_bOpen)
			return;
		if (!m_Target)
		{
			CloseSilent();
			return;
		}
		if (m_bStandalone && !DCO_ArsenalAccessComponent.CanUseNearby(m_Target))
		{
			CloseSilent();
			return;
		}
		if (m_wSearch)
		{
			string s = m_wSearch.GetText();
			if (s != m_sLastSearch)
			{
				m_sLastSearch = s;
				ResetScrolls();
				RefreshLists();
			}
		}
		RefreshContextItems();
		RefreshWeight();
		LockCamera();
		UpdateScrollBars();

		if (m_bInvMode)
		{
			RenderInventory();	// carried contents drift as the server applies verbs.
			return;
		}

		IEntity ctxNow;
		if (!m_bPoolOverride)
			ctxNow = ContextCandidate();
		if (ctxNow != m_LastCtxItem)
			RefreshLists();
	}

	protected string NameOfPrefab(ResourceName p)
	{
		DCO_ArsenalEntry e = DCO_ArsenalCatalog.Get().FindByPrefab(p);
		if (e)
			return e.m_sName;
		string nm = p.GetPath();
		int slash = nm.LastIndexOf("/");
		if (slash >= 0)
			nm = nm.Substring(slash + 1, nm.Length() - slash - 1);
		nm.Replace(".et", "");
		return nm;
	}

	bool OnButton(int id)
	{
		if (id == BTN_CLOSE)
		{
			CloseSilent();
			return true;
		}
		if (!m_bOpen)
			return true;

		if (id >= BTN_LO_BASE && id < BTN_LO_BASE + LO_ROWS)
		{
			OnLoadoutRow(id - BTN_LO_BASE);
			return true;
		}
		if (id >= BTN_CAT_BASE && id < BTN_CAT_BASE + CATS)
		{
			int ord = id - BTN_CAT_BASE;
			if (ord <= EDCO_ArsenalCategory.HEADGEAR)
			{
				m_eCatL = ord;
				m_bLastRight = false;
				m_bPoolOverride = false;	// Restore category-driven browsing.
			}
			else
			{
				m_eCatR = ord;
				m_bLastRight = true;
				m_bPoolOverride = true;	// a right tab is an explicit ask for the general pool.
			}
			m_bInvMode = false;
			m_bOverview = false;
			ResetScrolls();
			RefreshContextItems();
			RefreshLists();
			RefreshPresentation();
			return true;
		}
		if (id >= BTN_SLOT_BASE && id < BTN_SLOT_BASE + CTX_SLOTS)
		{
			m_eCatL = id - BTN_SLOT_BASE;
			m_bLastRight = false;
			m_bInvMode = false;
			m_bOverview = false;
			m_bPoolOverride = ContextCandidate() == null;
			ResetScrolls();
			RefreshLists();
			RefreshPresentation();
			return true;
		}
		if (id >= BTN_ROWR_BASE && id < BTN_ROWR_BASE + MAX_DYN_ROWS)
		{
			int idxR = id - BTN_ROWR_BASE;
			RevealClickedRow(true, idxR);
			if (m_bInvMode)
			{
				// inventory mode: a row click adds ONE MORE of that line's item.
				if (idxR < m_InvPrefabs.Count())
					RouteVerb(DCO_ArsenalServer.VERB_EQUIP, m_InvPrefabs[idxR]);
				return true;
			}
			if (idxR < m_FilteredR.Count())
			{
				m_bLastRight = true;
				RouteEntry(m_FilteredR[idxR]);
			}
			return true;
		}
		if (id >= BTN_ROWRBAG_BASE && id < BTN_ROWRBAG_BASE + MAX_DYN_ROWS)
		{
			int idxB = id - BTN_ROWRBAG_BASE;
			RevealClickedRow(true, idxB);
			if (m_bInvMode)
			{
				if (idxB < m_InvPrefabs.Count())
					RouteVerb(DCO_ArsenalServer.VERB_REMOVE, m_InvPrefabs[idxB]);
				return true;
			}
			if (idxB < m_FilteredR.Count())
				RouteVerb(DCO_ArsenalServer.VERB_EQUIP, m_FilteredR[idxB].m_Prefab);
			return true;
		}

		switch (id)
		{
			case BTN_CLEAR:
			{
				int cat = m_eCatL;
				if (m_bLastRight)
					cat = m_eCatR;
				RouteVerb(DCO_ArsenalServer.VERB_CLEAR, string.Format("%1", cat));
				return true;
			}
			case BTN_RESET:
			{
				RouteVerb(DCO_ArsenalServer.VERB_RESET, "");
				return true;
			}
			case BTN_LOADOUTS:
			{
				m_bLoOpen = !m_bLoOpen;
				if (m_wLoPanel)
					m_wLoPanel.SetVisible(m_bLoOpen);
				if (m_bLoOpen)
					RefreshLoadouts();
				return true;
			}
			case BTN_LO_SAVE:  { SaveLoadout();    return true; }
			case BTN_LO_LOAD:  { LoadLoadout();    return true; }
			case BTN_LO_DELETE: { DeleteLoadout(); return true; }
			case BTN_CTX_BACK:
			{
				// Toggle between replacing the selected slot and customizing its equipped item.
				m_bPoolOverride = !m_bPoolOverride;
				ResetScrolls();
				RefreshContextItems();
				RefreshLists();
				RefreshPresentation();
				return true;
			}
			case BTN_BACK_OPERATOR:
			{
				m_bOverview = true;
				m_bInvMode = false;
				m_bPoolOverride = false;
				RefreshContextItems();
				RefreshPresentation();
				return true;
			}
			case BTN_HINT:
			{
				if (m_wHintBox)
					m_wHintBox.SetVisible(!m_wHintBox.IsVisible());
				return true;
			}
			case BTN_UNDO: { RouteVerb(DCO_ArsenalServer.VERB_UNDO, ""); return true; }
			case BTN_REDO: { RouteVerb(DCO_ArsenalServer.VERB_REDO, ""); return true; }
			case BTN_INV_TAB:
			{
				m_bInvMode = !m_bInvMode;
				m_bOverview = false;
				ResetScrolls();
				RefreshContextItems();
				RefreshLists();
				RefreshPresentation();
				return true;
			}
			case BTN_LO_PREV:
			{
				if (m_iLoPage > 0)
				{
					m_iLoPage--;
					RefreshLoadouts();
				}
				return true;
			}
			case BTN_LO_NEXT:
			{
				if ((m_iLoPage + 1) * LO_ROWS < m_LoRecs.Count())
				{
					m_iLoPage++;
					RefreshLoadouts();
				}
				return true;
			}
		}

		if (id >= BTN_ROWL_BASE && id < BTN_ROWL_BASE + MAX_DYN_ROWS)
		{
			int idxL = id - BTN_ROWL_BASE;
			RevealClickedRow(false, idxL);
			if (idxL < m_FilteredL.Count())
			{
				m_bLastRight = false;
				m_bPoolOverride = false;
				m_bInvMode = false;
				RouteEntry(m_FilteredL[idxL]);
				RefreshContextItems();
				RefreshLists();
				RefreshPresentation();
			}
		}
		return true;
	}

	// A list entry was clicked.
	protected void RouteEntry(DCO_ArsenalEntry e)
	{
		if (!e)
			return;
		if (m_iDetailIdx >= 0 && m_iDetailIdx < CTX_SLOTS)
		{
			IEntity workItem = m_KitItems[m_iDetailIdx];
			if (workItem)
			{
				bool weaponTarget = m_iDetailIdx <= 2;
				bool fits;
				if (weaponTarget)
					fits = e.m_eCategory == EDCO_ArsenalCategory.ATTACHMENTS || e.m_eCategory == EDCO_ArsenalCategory.MAGAZINES;
				else
					fits = e.m_eCategory == EDCO_ArsenalCategory.MAGAZINES || e.m_eCategory == EDCO_ArsenalCategory.GRENADES || e.m_eCategory == EDCO_ArsenalCategory.ITEMS;
				if (fits)
				{
					DCO_ArsenalServer.Route(DCO_ArsenalServer.VERB_INSERT, workItem, e.m_Prefab);
					return;
				}
			}
		}
		RouteVerb(DCO_ArsenalServer.VERB_EQUIP, e.m_Prefab);
	}

	protected void RouteVerb(int verb, string payload)
	{
		if (!m_Target)
			return;
		IEntity owner = m_Target;
		if (!owner)
			return;
		DCO_ArsenalServer.Route(verb, owner, payload);
	}

	protected void RefreshLists()
	{
		DCO_ArsenalCatalog cat = DCO_ArsenalCatalog.Get();
		cat.GetEntries(m_eCatL, m_sLastSearch, "", m_FilteredL);
		RefreshSide(m_FilteredL, m_DynL, m_wListL, false);

		if (m_bInvMode)
		{
			RenderInventory();
			RefreshNavigationLabels(null);
			for (int i = 0; i < CATS; i++)
			{
				ImageWidget ico2 = m_CatIcons[i];
				if (ico2)
				{
					if (i == m_eCatL)
						ico2.SetColor(DCO_GMTheme.Get().m_AccentColor);
					else
						ico2.SetColor(DCO_GMTheme.Get().m_LabelColor);
				}
			}
			RefreshPresentation();
			return;
		}

		// The active left category drives the right column.
		m_iDetailIdx = -1;
		IEntity ctxItem;
		if (!m_bPoolOverride)
		{
			ctxItem = ContextCandidate();
			if (ctxItem)
				m_iDetailIdx = m_eCatL;
		}
		m_LastCtxItem = ctxItem;
		if (ctxItem && m_iDetailIdx <= 2)
			DCO_ArsenalCompat.FilterForWeapon(ctxItem, m_sLastSearch, m_FilteredR);
		else if (ctxItem)
			BuildInsertables(m_sLastSearch, m_FilteredR);
		else
			cat.GetEntries(m_eCatR, m_sLastSearch, "", m_FilteredR);

		RefreshSide(m_FilteredR, m_DynR, m_wListR, true);

		bool weaponCtx = ctxItem && m_iDetailIdx <= 2;
		int shownR = Math.Min(m_FilteredR.Count(), MAX_DYN_ROWS);
		for (int i = 0; i < m_DynR.Count(); i++)
		{
			DCO_ArsDynRow row = m_DynR[i];
			if (!row || !row.m_Bag)
				continue;
			bool show = weaponCtx && i < shownR && m_FilteredR[i].m_eCategory == EDCO_ArsenalCategory.MAGAZINES;
			row.m_Bag.SetVisible(show);
			if (show && row.m_BagIco)
				row.m_BagIco.LoadImageTexture(0, ICO_BAG_PLUS);
		}

		RefreshContextHeader(ctxItem);
		RefreshNavigationLabels(ctxItem);
		UpdatePreviewForContext(ctxItem);

		for (int i = 0; i < CATS; i++)
		{
			ImageWidget ico = m_CatIcons[i];
			if (!ico)
				continue;
			bool active = (i == m_eCatL) || (!ctxItem && i == m_eCatR);
			if (active)
				ico.SetColor(DCO_GMTheme.Get().m_AccentColor);
			else
				ico.SetColor(DCO_GMTheme.Get().m_LabelColor);
		}
		RefreshPresentation();
	}

	protected void RefreshPresentation()
	{
		if (!m_wRoot)
			return;
		IEntity contextItem = ContextCandidate();
		IEntity previewItem = SelectedSlotItem();
		bool showRight = !m_bOverview && (m_bInvMode || (!m_bPoolOverride && contextItem));
		if (m_wOverviewPanel)
			m_wOverviewPanel.SetVisible(m_bOverview);
		if (m_wItemPanel)
			m_wItemPanel.SetVisible(!m_bOverview);
		if (m_wLeftCol)
			m_wLeftCol.SetVisible(!m_bOverview && !showRight);
		if (m_wRightCol)
			m_wRightCol.SetVisible(!m_bOverview && showRight);
		if (m_wLeftTitle)
			m_wLeftTitle.SetVisible(!m_bOverview && !showRight);
		if (m_wRightTitle)
			m_wRightTitle.SetVisible(!m_bOverview && showRight);

		if (m_wCharacterPreviewArea)
		{
			if (m_bOverview)
			{
				FrameSlot.SetAnchorMin(m_wCharacterPreviewArea, 0.035, 0.105);
				FrameSlot.SetAnchorMax(m_wCharacterPreviewArea, 0.525, 0.89);
			}
			else
			{
				FrameSlot.SetAnchorMin(m_wCharacterPreviewArea, 0.12, 0.10);
				FrameSlot.SetAnchorMax(m_wCharacterPreviewArea, 0.88, 0.70);
			}
			FrameSlot.SetOffsets(m_wCharacterPreviewArea, 0, 0, 0, 0);
		}

		if (m_bOverview)
		{
			if (m_wStageTitle)
				m_wStageTitle.SetText("OPERATOR LOADOUT");
			if (m_wStageHelp)
				m_wStageHelp.SetText("Select an equipped slot to inspect or replace it. Drag the operator to rotate; use the wheel to zoom.");
			UpdatePreviewForContext(null);
		}
		else
		{
			if (m_wCtxBack)
				m_wCtxBack.SetVisible(contextItem != null);
			UpdatePreviewForContext(previewItem);
		}
		ScheduleScrollBarUpdate();
	}

	// Inventory mode rendering: aggregate everything the character carries into the right list.
	protected void RenderInventory()
	{
		BuildInvAggregation();

		int shown = Math.Min(m_InvPrefabs.Count(), MAX_DYN_ROWS);
		if (m_wRightTitle)
		{
			if (shown < m_InvPrefabs.Count())
				m_wRightTitle.SetText(string.Format("CARRIED INVENTORY  (%1/%2)", shown, m_InvPrefabs.Count()));
			else
				m_wRightTitle.SetText(string.Format("CARRIED INVENTORY  (%1)", m_InvPrefabs.Count()));
		}
		if (m_wCtxBack)
			m_wCtxBack.SetVisible(false);

		EnsureRows(m_DynR, m_wListR, shown, true);
		SetListRowCount(true, shown);

		ItemPreviewManagerEntity pm;
		ChimeraWorld cw = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (cw)
			pm = cw.GetItemPreviewManager();

		for (int i = 0; i < m_DynR.Count(); i++)
		{
			DCO_ArsDynRow row = m_DynR[i];
			if (!row)
				continue;
			bool has = i < shown;
			if (row.m_Root)
				row.m_Root.SetVisible(has);
			if (!has)
				continue;
			if (row.m_Txt)
				row.m_Txt.SetText(string.Format("%1x  %2", m_InvCounts[i], BoundRowPart(m_InvNames[i], 38)));
			if (row.m_Img && pm)
				pm.SetPreviewItemFromPrefab(row.m_Img, m_InvPrefabs[i]);
			if (row.m_Bag)
				row.m_Bag.SetVisible(true);
			if (row.m_BagIco)
				row.m_BagIco.LoadImageTexture(0, ICO_TRASH);
		}
		ScheduleScrollBarUpdate();
	}

	protected void BuildInvAggregation()
	{
		m_InvPrefabs.Clear();
		m_InvNames.Clear();
		m_InvCounts.Clear();
		if (!m_Target)
			return;
		IEntity owner = m_Target;
		if (!owner)
			return;
		SCR_InventoryStorageManagerComponent im = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!im)
			return;
		array<IEntity> contents = {};
		im.GetItems(contents);
		foreach (IEntity c : contents)
		{
			if (!c || !c.GetPrefabData())
				continue;
			if (DCO_ArsenalCompat.IsIntegralAttachment(c))
				continue;
			ResourceName p = c.GetPrefabData().GetPrefabName();
			int at = m_InvPrefabs.Find(p);
			if (at >= 0)
			{
				m_InvCounts[at] = m_InvCounts[at] + 1;
				continue;
			}
			m_InvPrefabs.Insert(p);
			m_InvNames.Insert(NameOfPrefab(p));
			m_InvCounts.Insert(1);
		}
	}

	// Everything that can go INSIDE a container: magazines, grenades and loose items.
	protected void BuildInsertables(string search, out notnull array<DCO_ArsenalEntry> outEntries)
	{
		outEntries.Clear();
		DCO_ArsenalCatalog cat = DCO_ArsenalCatalog.Get();
		array<DCO_ArsenalEntry> pool = {};
		cat.GetEntries(EDCO_ArsenalCategory.MAGAZINES, search, "", pool);
		foreach (DCO_ArsenalEntry m : pool)
			outEntries.Insert(m);
		cat.GetEntries(EDCO_ArsenalCategory.GRENADES, search, "", pool);
		foreach (DCO_ArsenalEntry g : pool)
			outEntries.Insert(g);
		cat.GetEntries(EDCO_ArsenalCategory.ITEMS, search, "", pool);
		foreach (DCO_ArsenalEntry it : pool)
			outEntries.Insert(it);
	}

	protected IEntity ContextCandidate()
	{
		if (m_eCatL > EDCO_ArsenalCategory.HEADGEAR)
			return null;
		IEntity slotItem = m_KitItems[m_eCatL];	// category 0-6 order matches the kit buckets.
		if (!slotItem)
			return null;
		if (m_eCatL <= EDCO_ArsenalCategory.LAUNCHER)
			return slotItem;
		if (slotItem.FindComponent(BaseInventoryStorageComponent))
			return slotItem;
		return null;
	}

	protected IEntity SelectedSlotItem()
	{
		if (m_eCatL < 0 || m_eCatL >= m_KitItems.Count())
			return null;
		return m_KitItems[m_eCatL];
	}

	protected string ContextName(IEntity item)
	{
		if (item && item.GetPrefabData())
		{
			DCO_ArsenalEntry e = DCO_ArsenalCatalog.Get().FindByPrefab(item.GetPrefabData().GetPrefabName());
			if (e)
				return e.m_sName;
		}
		return "SELECTED ITEM";
	}

	protected void RefreshContextHeader(IEntity ctxItem)
	{
		string suffix = CountSuffix(m_FilteredR.Count());
		if (!ctxItem)
		{
			if (m_wRightTitle)
				m_wRightTitle.SetText(StaticData().m_CategoryNames[m_eCatR] + suffix);
			IEntity candidate = ContextCandidate();
			if (m_wCtxBack)
				m_wCtxBack.SetVisible(candidate != null);
			if (m_wCtxBackTxt && candidate)
				m_wCtxBackTxt.SetText("CUSTOMIZE EQUIPPED ITEM >");
			return;
		}
		if (m_wCtxBack)
			m_wCtxBack.SetVisible(true);
		if (m_wCtxBackTxt)
			m_wCtxBackTxt.SetText("CHANGE ITEM  >");
		if (!m_wRightTitle)
			return;
		if (m_iDetailIdx <= 2)
			m_wRightTitle.SetText("COMPATIBLE ATTACHMENTS" + suffix);
		else
			m_wRightTitle.SetText("PACK THIS ITEM" + suffix);
	}

	protected void RefreshNavigationLabels(IEntity ctxItem)
	{
		string categoryName = StaticData().m_CategoryNames[m_eCatL];
		if (m_wLeftTitle)
			m_wLeftTitle.SetText(categoryName + CountSuffix(m_FilteredL.Count()));

		if (m_bInvMode)
		{
			if (m_wStageTitle)
				m_wStageTitle.SetText("CARRIED LOADOUT");
			if (m_wStageHelp)
				m_wStageHelp.SetText("Review everything carried. Select a row to add one; use its remove action to take one away.");
			return;
		}

		if (m_wStageTitle)
		{
			if (ctxItem)
				m_wStageTitle.SetText(BoundRowPart(ContextName(ctxItem), 48));
			else
				m_wStageTitle.SetText(CategoryGroup(m_eCatL) + "  /  " + categoryName);
		}
		if (!m_wStageHelp)
			return;
		if (ctxItem && m_iDetailIdx <= 2)
			m_wStageHelp.SetText("CUSTOMIZE ITEM  /  Drag the weapon to rotate and use the wheel to zoom. Only compatible attachments and magazines are shown.");
		else if (ctxItem)
			m_wStageHelp.SetText("Choose what this equipped item should carry. Drag the character to inspect every side.");
		else
			m_wStageHelp.SetText("Choose a replacement from the equipment rail. Select an equipped item to reveal its compatible customization options.");
	}

	protected string CategoryGroup(int category)
	{
		if (category <= EDCO_ArsenalCategory.LAUNCHER)
			return "WEAPONS KIT";
		if (category == EDCO_ArsenalCategory.VEST || category == EDCO_ArsenalCategory.BACKPACK)
			return "GEAR KIT";
		if (category == EDCO_ArsenalCategory.UNIFORM || category == EDCO_ArsenalCategory.HEADGEAR)
			return "OUTFIT";
		return "AMMO + ITEMS";
	}

	// List size readout for titles; the creation cap is never silent.
	protected string CountSuffix(int total)
	{
		if (total > MAX_DYN_ROWS)
			return string.Format("  (%1/%2)", MAX_DYN_ROWS, total);
		return string.Format("  (%1)", total);
	}

	protected void RefreshSide(array<DCO_ArsenalEntry> filtered, array<ref DCO_ArsDynRow> pool, Widget list, bool right)
	{
		int shown = Math.Min(filtered.Count(), MAX_DYN_ROWS);
		if (shown < filtered.Count())
			Print(string.Format("[DCO-ARS] list capped at %1 of %2 - refine the search", shown, filtered.Count()), LogLevel.WARNING);
		EnsureRows(pool, list, shown, right);
		SetListRowCount(right, shown);

		ItemPreviewManagerEntity pm;
		ChimeraWorld cw = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (cw)
			pm = cw.GetItemPreviewManager();

		for (int i = 0; i < pool.Count(); i++)
		{
			DCO_ArsDynRow row = pool[i];
			if (!row)
				continue;
			bool has = i < shown;
			if (row.m_Root)
				row.m_Root.SetVisible(has);
			if (row.m_Bag)
				row.m_Bag.SetVisible(false);	// callers re-show per-row where it applies.
			if (!has)
				continue;
			DCO_ArsenalEntry e = filtered[i];
			if (row.m_Txt)
			{
				string label = BoundRowPart(e.m_sName, 38);
				if (!e.m_sFactionKey.IsEmpty())
					label = label + "\n" + BoundRowPart(e.m_sFactionKey, 18);
				row.m_Txt.SetText(label);
			}
			if (row.m_Img && pm)
				pm.SetPreviewItemFromPrefab(row.m_Img, e.m_Prefab);
		}
		ScheduleScrollBarUpdate();
	}

	protected string BoundRowPart(string value, int maxLength)
	{
		if (value.Length() <= maxLength)
			return value;
		return value.Substring(0, maxLength - 3) + "...";
	}

	protected void EnsureRows(array<ref DCO_ArsDynRow> pool, Widget list, int count, bool right)
	{
		if (!list)
			return;
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return;
		while (pool.Count() < count && pool.Count() < MAX_DYN_ROWS)
		{
			int idx = pool.Count();
			Widget rowRoot = ws.CreateWidgets(ROW_LAYOUT, list);
			if (!rowRoot)
			{
				Print("[DCO-ARS] row template failed to instantiate - is DCO_ArsenalRow.layout registered?", LogLevel.WARNING);
				return;
			}
			DCO_ArsDynRow row = new DCO_ArsDynRow();
			row.m_Root = rowRoot;
			row.m_Btn = ButtonWidget.Cast(rowRoot.FindAnyWidget("DCO_ArsDynBtn"));
			row.m_Img = ItemPreviewWidget.Cast(rowRoot.FindAnyWidget("DCO_ArsDynImg"));
			row.m_Txt = TextWidget.Cast(rowRoot.FindAnyWidget("DCO_ArsDynTxt"));
			row.m_Bag = ButtonWidget.Cast(rowRoot.FindAnyWidget("DCO_ArsDynBag"));
			row.m_BagIco = ImageWidget.Cast(rowRoot.FindAnyWidget("DCO_ArsDynBagIco"));
			BindWheelTree(row.m_Root, right);
			if (row.m_Btn)
			{
				if (right)
					AddHandler(row.m_Btn, BTN_ROWR_BASE + idx);
				else
					AddHandler(row.m_Btn, BTN_ROWL_BASE + idx);
			}
			if (row.m_Bag)
			{
				row.m_Bag.SetVisible(false);
				if (right)
					AddHandler(row.m_Bag, BTN_ROWRBAG_BASE + idx);
			}
			pool.Insert(row);
		}
	}

	protected void ResetScrolls()
	{
		if (m_wScrollL)
			m_wScrollL.SetSliderPos(0, 0);
		if (m_wScrollR)
			m_wScrollR.SetSliderPos(0, 0);
		ScheduleScrollBarUpdate();
	}

	void OnHoverBtn(int id, Widget w, bool entering)
	{
		SetDynamicRowHover(id, entering);
		if (!m_wHover || !m_wHoverTxt)
			return;
		if (!entering || !m_bOpen)
		{
			m_wHover.SetVisible(false);
			return;
		}
		string label = HoverTextFor(id);
		if (label.IsEmpty())
		{
			m_wHover.SetVisible(false);
			return;
		}
		m_wHoverTxt.SetText(BoundHoverLabel(label));
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (w && ws)
		{
			float x, y, sw, sh;
			w.GetScreenPos(x, y);
			w.GetScreenSize(sw, sh);
			float ux = ws.DPIUnscale(x + sw * 0.5) - 90;
			float uy = ws.DPIUnscale(y) - 30;
			if (uy < 0)
				uy = ws.DPIUnscale(y + sh) + 4;	// no room above -> flip below.
			float rootX, rootY, rootW, rootH;
			m_wRoot.GetScreenPos(rootX, rootY);
			m_wRoot.GetScreenSize(rootW, rootH);
			rootX = ws.DPIUnscale(rootX);
			rootY = ws.DPIUnscale(rootY);
			rootW = ws.DPIUnscale(rootW);
			rootH = ws.DPIUnscale(rootH);
			ux = Math.Clamp(ux, rootX, Math.Max(rootX, rootX + rootW - 180));
			uy = Math.Clamp(uy, rootY, Math.Max(rootY, rootY + rootH - 24));
			FrameSlot.SetSize(m_wHover, 180, 24);
			FrameSlot.SetPos(m_wHover, ux, uy);
		}
		m_wHover.SetVisible(true);
	}

	protected void SetDynamicRowHover(int id, bool entering)
	{
		DCO_ArsDynRow row;
		if (id >= BTN_ROWL_BASE && id < BTN_ROWL_BASE + m_DynL.Count())
			row = m_DynL[id - BTN_ROWL_BASE];
		else if (id >= BTN_ROWR_BASE && id < BTN_ROWR_BASE + m_DynR.Count())
			row = m_DynR[id - BTN_ROWR_BASE];
		if (!row || !row.m_Txt)
			return;
		if (entering)
			row.m_Txt.SetColor(DCO_GMTheme.Get().m_AccentColor);
		else
			row.m_Txt.SetColor(DCO_GMTheme.Get().m_LabelColor);
	}

	protected string BoundHoverLabel(string value)
	{
		if (value.Length() <= 30)
			return value;
		return value.Substring(0, 27) + "...";
	}

	protected string HoverTextFor(int id)
	{
		if (id >= BTN_SLOT_BASE && id < BTN_SLOT_BASE + CTX_SLOTS)
			return "EDIT " + StaticData().m_CategoryNames[id - BTN_SLOT_BASE];
		if (id >= BTN_CAT_BASE && id < BTN_CAT_BASE + CATS)
			return StaticData().m_CategoryNames[id - BTN_CAT_BASE];
		if (id >= BTN_ROWRBAG_BASE && id < BTN_ROWRBAG_BASE + MAX_DYN_ROWS)
		{
			if (m_bInvMode)
				return "REMOVE ONE";
			return "ADD TO INVENTORY";
		}
		switch (id)
		{
			case BTN_CLEAR:     return "CLEAR CATEGORY";
			case BTN_RESET:     return "RESET KIT";
			case BTN_CLOSE:     return "DONE";
			case BTN_LOADOUTS:  return "LOADOUTS";
			case BTN_LO_SAVE:   return "SAVE LOADOUT";
			case BTN_LO_LOAD:   return "LOAD LOADOUT";
			case BTN_LO_DELETE: return "DELETE LOADOUT";
			case BTN_LO_PREV:   return "PREVIOUS PAGE";
			case BTN_LO_NEXT:   return "NEXT PAGE";
			case BTN_HINT:      return "HELP";
			case BTN_UNDO:      return "UNDO";
			case BTN_REDO:      return "REDO";
			case BTN_INV_TAB:   return "INVENTORY - EVERYTHING CARRIED";
			case BTN_BACK_OPERATOR: return "BACK TO OPERATOR";
		}
		return "";
	}

	// Resolve the equipped item per bucket 0-6 (PRI..HEAD).
	protected void RefreshContextItems()
	{
		if (!m_Target)
			return;
		IEntity owner = m_Target;
		if (!owner)
			return;
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return;

		for (int i = 0; i < CTX_SLOTS; i++)
			m_KitItems[i] = null;
		array<IEntity> items = {};
		inv.GetItems(items);
		DCO_ArsenalCatalog cat = DCO_ArsenalCatalog.Get();
		foreach (IEntity item : items)
		{
			if (!item || !item.GetPrefabData())
				continue;
			DCO_ArsenalEntry e = cat.FindByPrefab(item.GetPrefabData().GetPrefabName());
			if (!e)
				continue;
			int bucket = -1;
			switch (e.m_eCategory)
			{
				case EDCO_ArsenalCategory.PRIMARY:   { bucket = 0; break; }
				case EDCO_ArsenalCategory.PISTOL:    { bucket = 1; break; }
				case EDCO_ArsenalCategory.LAUNCHER:  { bucket = 2; break; }
				case EDCO_ArsenalCategory.UNIFORM:   { bucket = 3; break; }
				case EDCO_ArsenalCategory.VEST:      { bucket = 4; break; }
				case EDCO_ArsenalCategory.BACKPACK:  { bucket = 5; break; }
				case EDCO_ArsenalCategory.HEADGEAR:  { bucket = 6; break; }
			}
			if (bucket >= 0 && !m_KitItems[bucket])
				m_KitItems[bucket] = item;
		}
		RefreshSlotCards();
	}

	protected void RefreshSlotCards()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		ItemPreviewManagerEntity previewManager;
		if (world)
			previewManager = world.GetItemPreviewManager();
		for (int category = 0; category < m_SlotCards.Count(); category++)
		{
			DCO_ArsSlotCard card = m_SlotCards[category];
			if (!card)
				continue;
			IEntity item;
			if (category < m_KitItems.Count())
				item = m_KitItems[category];
			if (card.m_bHasBoundItem && card.m_BoundItem == item)
				continue;
			card.m_BoundItem = item;
			card.m_bHasBoundItem = true;
			if (card.m_Name)
			{
				if (item)
					card.m_Name.SetText(BoundRowPart(ContextName(item), 34));
				else
					card.m_Name.SetText("EMPTY - SELECT TO EQUIP");
			}
			if (card.m_Preview && previewManager)
				previewManager.SetPreviewItem(card.m_Preview, item);
		}
	}

	protected void RefreshWeight()
	{
		if (!m_wWeight || !m_Target)
			return;
		IEntity owner = m_Target;
		if (!owner)
			return;
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
		{
			m_wWeight.SetText("WEIGHT: -");
			return;
		}
		float kg = inv.GetTotalWeightOfAllStorages();
		float lb = kg * 2.20462;
		m_wWeight.SetText(string.Format("WEIGHT: %1 kg (%2 lb)", Math.Round(kg * 100) / 100, Math.Round(lb * 10) / 10));
	}

	// LOADOUTS panel.

	protected void OnLoadoutsChanged()
	{
		if (m_bOpen && m_bLoOpen)
			RefreshLoadouts();
	}

	protected void RefreshLoadouts()
	{
		DCO_ArsenalLoadouts store = DCO_ArsenalLoadouts.Get();
		store.GetLoadouts(m_LoRecs);
		int pages = Math.Max(1, Math.Ceil(m_LoRecs.Count() / (float)LO_ROWS));
		if (m_iLoPage >= pages)
			m_iLoPage = pages - 1;
		if (m_wLoPage)
			m_wLoPage.SetText(string.Format("%1/%2  (%3)", m_iLoPage + 1, pages, m_LoRecs.Count()));

		for (int i = 0; i < LO_ROWS; i++)
		{
			int idx = m_iLoPage * LO_ROWS + i;
			bool has = idx < m_LoRecs.Count();
			ButtonWidget b = m_LoRows[i];
			if (b)
				b.SetVisible(has);
			TextWidget t = m_LoTxts[i];
			if (!t || !has)
				continue;
			DCO_ArsenalLoadoutRec rec = m_LoRecs[idx];
			bool avail = store.IsAvailable(rec);
			string label = BoundRowPart(rec.m_sName, 38);
			if (!avail)
				label = BoundRowPart(rec.m_sName, 25) + "  [MISSING]";
			t.SetText(label);
			if (!avail)
				t.SetColor(Color.FromInt(DCO_GMTheme.SEM_HOSTILE));	// semantic red = unavailable, unselectable.
			else if (idx == m_iLoSelected)
				t.SetColor(DCO_GMTheme.Get().m_AccentColor);	// accent = selected.
			else
				t.SetColor(DCO_GMTheme.Get().m_LabelColor);
		}
	}

	protected void OnLoadoutRow(int rowIdx)
	{
		if (rowIdx < 0 || rowIdx >= LO_ROWS)
			return;
		int idx = m_iLoPage * LO_ROWS + rowIdx;
		if (idx >= m_LoRecs.Count())
			return;
		DCO_ArsenalLoadoutRec rec = m_LoRecs[idx];
		if (!DCO_ArsenalLoadouts.Get().IsAvailable(rec))
			return;
		if (m_iLoSelected == idx)
			m_iLoSelected = -1;
		else
			m_iLoSelected = idx;
		RefreshLoadouts();
	}

	protected void SaveLoadout()
	{
		if (!m_Target)
			return;
		IEntity owner = m_Target;
		if (!owner)
			return;
		string name;
		if (m_wLoName)
			name = m_wLoName.GetText();
		name.TrimInPlace();
		if (name.IsEmpty())
			name = string.Format("Loadout %1", m_LoRecs.Count() + 1);

		array<string> manifest = {};
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (inv)
		{
			array<IEntity> items = {};
			inv.GetItems(items);
			DCO_ArsenalCatalog cat = DCO_ArsenalCatalog.Get();
			foreach (IEntity item : items)
			{
				if (!item || !item.GetPrefabData())
					continue;
				string p = item.GetPrefabData().GetPrefabName();
				if (!p.IsEmpty() && cat.FindByPrefab(p) && !manifest.Contains(p))
					manifest.Insert(p);
			}
		}
		DCO_ArsenalLoadouts.Get().SaveFrom(owner, name, manifest);
	}

	protected void LoadLoadout()
	{
		if (!m_Target || m_iLoSelected < 0 || m_iLoSelected >= m_LoRecs.Count())
			return;
		IEntity owner = m_Target;
		if (!owner)
			return;
		DCO_ArsenalLoadouts.Get().ApplyTo(owner, m_LoRecs[m_iLoSelected]);
	}

	protected void DeleteLoadout()
	{
		if (m_iLoSelected < 0 || m_iLoSelected >= m_LoRecs.Count())
			return;
		DCO_ArsenalLoadouts.Get().Delete(m_LoRecs[m_iLoSelected]);
		m_iLoSelected = -1;
	}

	protected void LockCamera()
	{
		if (!m_bUseEditorCamera || !m_Target)
			return;
		IEntity owner = m_Target;
		if (!owner)
			return;
		SCR_CameraEditorComponent cm = SCR_CameraEditorComponent.Cast(SCR_CameraEditorComponent.GetInstance(SCR_CameraEditorComponent));
		if (!cm)
			return;
		SCR_ManualCamera cam = cm.GetCamera();
		if (!cam)
			return;
		if (vector.Distance(cam.GetOrigin(), owner.GetOrigin()) <= CAM_DRIFT_M)
			return;
		SCR_TeleportToCursorManualCameraComponent tp = SCR_TeleportToCursorManualCameraComponent.Cast(cam.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
		if (tp)
			tp.TeleportCamera(owner.GetOrigin());
	}

	protected void FocusCameraOn(IEntity unit)
	{
		SCR_CameraEditorComponent cm = SCR_CameraEditorComponent.Cast(SCR_CameraEditorComponent.GetInstance(SCR_CameraEditorComponent));
		if (!cm)
			return;
		SCR_ManualCamera cam = cm.GetCamera();
		if (!cam)
			return;
		SCR_TeleportToCursorManualCameraComponent tp = SCR_TeleportToCursorManualCameraComponent.Cast(cam.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
		if (tp)
			tp.TeleportCamera(unit.GetOrigin());
	}

	protected void StartCharacterPreview(IEntity target)
	{
		StopCharacterPreview();
		ChimeraCharacter character = ChimeraCharacter.Cast(target);
		ChimeraWorld world;
		if (character)
			world = ChimeraWorld.CastFrom(character.GetWorld());
		if (!character || !world || !m_wCharacterPreview || !m_wCharacterPreviewArea)
		{
			Print("[DCO-ARS] character preview unavailable: player or preview widgets are missing", LogLevel.WARNING);
			return;
		}

		m_CharacterPreviewManager = world.GetItemPreviewManager();
		if (!m_CharacterPreviewManager)
		{
			Resource resource = Resource.Load(PREVIEW_MANAGER_PREFAB);
			if (resource && resource.IsValid())
				GetGame().SpawnEntityPrefabLocal(resource, world);
			m_CharacterPreviewManager = world.GetItemPreviewManager();
		}
		if (!m_CharacterPreviewManager)
		{
			Print("[DCO-ARS] preview unavailable: native preview manager is missing", LogLevel.WARNING);
			return;
		}

		m_wCharacterPreviewArea.SetVisible(true);
		m_wCharacterPreview.SetVisible(true);
		UpdatePreviewForContext(m_LastCtxItem);
		if (!m_PreviewSubject)
			SetPreviewSubject(character, false);
		GetGame().GetCallqueue().Remove(PreviewTick);
		if (!m_bStandalone)
			GetGame().GetCallqueue().CallLater(PreviewTick, PREVIEW_TICK_MS, true);
	}

	// Ground Branch-style state transition: character overview becomes a focused item inspection
	// while the existing server-authoritative attachment route remains unchanged.
	protected void UpdatePreviewForContext(IEntity contextItem)
	{
		if (!m_CharacterPreviewManager || !m_wCharacterPreview)
			return;
		if (!m_bOverview && contextItem)
		{
			SetPreviewSubject(contextItem, true);
			return;
		}
		if (m_Target)
			SetPreviewSubject(m_Target, false);
	}

	protected PreviewRenderAttributes ResolvePreviewAttributes(IEntity subject, bool itemMode)
	{
		if (!subject)
			return null;
		ItemAttributeCollection attributes;
		if (itemMode)
		{
			InventoryItemComponent item = InventoryItemComponent.Cast(subject.FindComponent(InventoryItemComponent));
			if (item)
				attributes = item.GetAttributes();
		}
		else
		{
			SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(
				subject.FindComponent(SCR_CharacterInventoryStorageComponent));
			if (storage)
				attributes = storage.GetAttributes();
		}
		if (!attributes)
			return null;
		if (itemMode)
			return PreviewRenderAttributes.Cast(attributes.FindAttribute(PreviewRenderAttributes));
		return PreviewRenderAttributes.Cast(attributes.FindAttribute(SCR_CharacterInventoryPreviewAttributes));
	}

	protected void SetPreviewSubject(IEntity subject, bool itemMode)
	{
		if (!subject || !m_CharacterPreviewManager || !m_wCharacterPreview)
			return;
		if (m_PreviewSubject == subject && m_bPreviewingItem == itemMode)
			return;

		StopPreviewInput();
		m_PreviewSubject = subject;
		m_bPreviewingItem = itemMode;
		if (m_wMode)
			m_wMode.SetText("BIFROST ARSENAL");
		m_CharacterPreviewAttributes = ResolvePreviewAttributes(subject, itemMode);
		if (m_CharacterPreviewAttributes)
		{
			m_CharacterPreviewAttributes.ResetDeltaRotation();
			// The native helper owns its workspace event registration. Registering it on the
			// preview as well dispatches each mouse event twice and destabilizes drag state.
			m_CharacterPreviewInput = SCR_InventoryCharacterWidgetHelper(
				m_wCharacterPreview, GetGame().GetWorkspace());
		}
		m_CharacterPreviewManager.SetPreviewItem(
			m_wCharacterPreview, subject, m_CharacterPreviewAttributes, true);
	}

	protected void StopPreviewInput()
	{
		if (m_CharacterPreviewAttributes)
			m_CharacterPreviewAttributes.ResetDeltaRotation();
		if (m_CharacterPreviewInput)
			m_CharacterPreviewInput.Destroy();
		m_CharacterPreviewInput = null;
		m_CharacterPreviewAttributes = null;
	}

	void UpdateCharacterPreview(float timeSlice)
	{
		if (!m_bOpen || !m_CharacterPreviewInput || !m_CharacterPreviewAttributes)
			return;
		InputManager input = GetGame().GetInputManager();
		if (input)
			input.ActivateContext("InventoryMenuContext");
		if (m_CharacterPreviewInput.Update(timeSlice, m_CharacterPreviewAttributes))
			RefreshCharacterPreview();
	}

	protected void PreviewTick()
	{
		UpdateCharacterPreview(PREVIEW_TICK_MS * 0.001);
	}

	protected void RefreshCharacterPreview()
	{
		if (!m_PreviewSubject || !m_CharacterPreviewManager || !m_wCharacterPreview)
			return;
		m_CharacterPreviewManager.SetPreviewItem(
			m_wCharacterPreview, m_PreviewSubject, m_CharacterPreviewAttributes);
	}

	protected void StopCharacterPreview()
	{
		GetGame().GetCallqueue().Remove(PreviewTick);
		StopPreviewInput();
		if (m_CharacterPreviewManager && m_wCharacterPreview)
			m_CharacterPreviewManager.SetPreviewItem(m_wCharacterPreview, null);
		if (m_wCharacterPreview)
			m_wCharacterPreview.SetVisible(false);
		if (m_wCharacterPreviewArea)
			m_wCharacterPreviewArea.SetVisible(false);
		m_CharacterPreviewManager = null;
		m_PreviewSubject = null;
		m_bPreviewingItem = false;
	}

	protected string TargetName(SCR_EditableEntityComponent e)
	{
		if (e)
		{
			SCR_UIInfo info = e.GetInfo();
			if (info)
			{
				string nm = info.GetName();
				if (!nm.IsEmpty())
					return nm;
			}
		}
		return "selected unit";
	}

	void Shutdown()
	{
		if (m_Target)
			RouteVerb(DCO_ArsenalServer.VERB_RELEASE, "");
		HideShell(false);
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(UpdateScrollBars);
		GetGame().GetCallqueue().Remove(PreviewTick);
		StopScrollBarDrag();
		StopCharacterPreview();
		DCO_ArsenalLoadouts.Get().GetOnChanged().Remove(OnLoadoutsChanged);
		m_bOpen = false;
		m_bLoOpen = false;
		m_Target = null;
		m_iDetailIdx = -1;
		m_iLoSelected = -1;
		m_FilteredL.Clear();
		m_FilteredR.Clear();
		m_LoRecs.Clear();
		m_HiddenSiblings.Clear();
		m_Handlers.Clear();
		m_WheelLeft = null;
		m_WheelRight = null;
		m_CatIcons.Clear();
		m_DynL.Clear();
		m_DynR.Clear();
		m_SlotCards.Clear();
		m_InvPrefabs.Clear();
		m_InvNames.Clear();
		m_InvCounts.Clear();
		m_KitItems.Clear();
		m_LoRows.Clear();
		m_LoTxts.Clear();
		m_wScreen = null;
		m_wTarget = null;
		m_wSearch = null;
		m_wWeight = null;
		m_wRightTitle = null;
		m_wCtxBack = null;
		m_wCtxBackTxt = null;
		m_wHintBox = null;
		m_wLoPanel = null;
		m_wLoName = null;
		m_wLoPage = null;
		m_wScrollL = null;
		m_wScrollR = null;
		m_wListSizeL = null;
		m_wListSizeR = null;
		m_wListL = null;
		m_wListR = null;
		m_wBarL = null;
		m_wBarR = null;
		m_wBarSpacerL = null;
		m_wBarSpacerR = null;
		m_wBarThumbL = null;
		m_wBarThumbR = null;
		m_wHover = null;
		m_wHoverTxt = null;
		m_wCharacterPreviewArea = null;
		m_wCharacterPreview = null;
		if (m_wOwnedRoot)
			m_wOwnedRoot.RemoveFromHierarchy();
		m_wOwnedRoot = null;
		m_wShellRoot = null;
		m_wRoot = null;
	}
}
