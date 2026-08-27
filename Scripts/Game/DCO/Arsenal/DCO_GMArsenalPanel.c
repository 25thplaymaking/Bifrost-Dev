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

class DCO_GMArsenalPanel
{
	protected static ref DCO_GMArsenalPanel s_Inst;
	static DCO_GMArsenalPanel Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_GMArsenalPanel();
		return s_Inst;
	}

	protected static const int CATS = 11;
	protected static const int CTX_SLOTS = 7;	// equipped buckets 0-6 (PRI..HEAD) drive the right-column context.
	protected static const int LO_ROWS = 8;
	protected static const int POLL_MS = 500;
	protected static const float CAM_DRIFT_M = 9.0;
	protected static const int MAX_DYN_ROWS = 250;	// dynamic-list creation cap; the title reports the cut, never silent.
	protected static const float BAR_WIDTH = 8.0;
	protected static const float BAR_MIN_THUMB = 36.0;
	protected static const float ROW_HEIGHT = 42.0;
	protected static const float WHEEL_STEP_NORMALIZED = 0.08;

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

	protected static const ResourceName ROW_LAYOUT = "{B8F95C96D9349F98}UI/layouts/DCO_ArsenalRow.layout";

	protected static const string CAT_NAMES[11] = {
		"PRIMARY WEAPONS", "PISTOLS", "LAUNCHERS", "UNIFORMS", "VESTS", "BACKPACKS", "HEADGEAR",
		"ITEMS", "GRENADES", "MAGAZINES", "ATTACHMENTS"
	};

	protected static const ResourceName CAT_ICONS[11] = {
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

	protected Widget m_wRoot;
	protected Widget m_wScreen;
	protected TextWidget m_wTarget;
	protected EditBoxWidget m_wSearch;
	protected TextWidget m_wWeight;
	protected TextWidget m_wRightTitle;
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
	protected ref array<ImageWidget> m_CatIcons = {};
	protected ref array<ref DCO_ArsDynRow> m_DynL = {};	// dynamic rows, created on demand from ROW_LAYOUT.
	protected ref array<ref DCO_ArsDynRow> m_DynR = {};
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
	protected int m_iBarStartMouseY;
	protected float m_fBarStartPos;

	protected bool m_bOpen;
	protected SCR_EditableEntityComponent m_Target;
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

	void Init(Widget shellRoot)
	{
		if (!shellRoot)
			return;
		m_wRoot = shellRoot;
		s_Inst = this;

		m_wScreen     = shellRoot.FindAnyWidget("DCO_ArsenalScreen");
		m_wTarget     = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsTarget"));
		m_wSearch     = EditBoxWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsSearch"));
		m_wWeight     = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsWeight"));
		m_wRightTitle = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsRightTitle"));
		m_wCtxBack    = shellRoot.FindAnyWidget("DCO_ArsCtxBack");
		m_wCtxBackTxt = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsCtxBackTxt"));
		m_wHintBox    = shellRoot.FindAnyWidget("DCO_ArsHintBox");
		m_wLoPanel    = shellRoot.FindAnyWidget("DCO_ArsLoadoutPanel");
		m_wLoName     = EditBoxWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsLoName"));
		m_wLoPage     = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsLoPage"));
		m_wScrollL    = ScrollLayoutWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsScrollL"));
		m_wScrollR    = ScrollLayoutWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsScrollR"));
		m_wListSizeL  = SizeLayoutWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsListSizeL"));
		m_wListSizeR  = SizeLayoutWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsListSizeR"));
		m_wListL      = shellRoot.FindAnyWidget("DCO_ArsListL");
		m_wListR      = shellRoot.FindAnyWidget("DCO_ArsListR");
		m_wBarL       = shellRoot.FindAnyWidget("DCO_ArsBarL");
		m_wBarR       = shellRoot.FindAnyWidget("DCO_ArsBarR");
		m_wBarSpacerL = shellRoot.FindAnyWidget("DCO_ArsBarSpacerL");
		m_wBarSpacerR = shellRoot.FindAnyWidget("DCO_ArsBarSpacerR");
		m_wBarThumbL  = shellRoot.FindAnyWidget("DCO_ArsBarThumbL");
		m_wBarThumbR  = shellRoot.FindAnyWidget("DCO_ArsBarThumbR");
		m_wHover      = shellRoot.FindAnyWidget("DCO_ArsHover");
		m_wHoverTxt   = TextWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsHoverTxt"));
		m_WheelLeft = new DCO_ArsWheelHandler(this, false);
		m_WheelRight = new DCO_ArsWheelHandler(this, true);
		BindWheelTree(m_wScrollL, false);
		BindWheelTree(m_wScrollR, true);
		BindWheelTree(m_wBarL, false);
		BindWheelTree(m_wBarR, true);
		if (m_wHover)
			m_wHover.SetVisible(false);

		int cats = 0;
		int icons = 0;
		for (int i = 0; i < CATS; i++)
		{
			ButtonWidget b = ButtonWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsCat" + i.ToString()));
			ImageWidget ico = ImageWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsCatIco" + i.ToString()));
			m_CatIcons.Insert(ico);
			if (b)
			{
				AddHandler(b, BTN_CAT_BASE + i);
				cats++;
			}
			if (ico)
			{
				ico.LoadImageTexture(0, CAT_ICONS[i]);
				icons++;
			}
		}
		int icons2 = 0;
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_Loadouts", ICO_LOADOUTS);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_Clear",    ICO_CLEAR);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_Reset",    ICO_RESET);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_Done",     ICO_DONE);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_Undo",     ICO_UNDO);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_Redo",     ICO_REDO);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_Hint",     ICO_HINT);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_LoSave",   ICO_SAVE);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_LoLoad",   ICO_LOAD);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_LoDelete", ICO_TRASH);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_LoPrev",   ICO_NAV_L);
		icons2 += LoadIcon(shellRoot, "DCO_ArsBI_LoNext",   ICO_NAV_R);
		icons2 += LoadIcon(shellRoot, "DCO_ArsInvTabIco",   ICO_INV_TAB);
		for (int i = 0; i < CTX_SLOTS; i++)
			m_KitItems.Insert(null);
		int loRows = 0;
		for (int i = 0; i < LO_ROWS; i++)
		{
			ButtonWidget lb = ButtonWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsLoRow" + i.ToString()));
			m_LoRows.Insert(lb);
			m_LoTxts.Insert(TextWidget.Cast(shellRoot.FindAnyWidget("DCO_ArsLoTxt" + i.ToString())));
			if (lb)
			{
				AddHandler(lb, BTN_LO_BASE + i);
				loRows++;
			}
		}

		int buttons = 0;
		buttons += BindNamed("DCO_ArsClear", BTN_CLEAR);
		buttons += BindNamed("DCO_ArsReset", BTN_RESET);
		buttons += BindNamed("DCO_ArsClose", BTN_CLOSE);
		buttons += BindNamed("DCO_ArsLoadouts", BTN_LOADOUTS);
		buttons += BindNamed("DCO_ArsLoSave", BTN_LO_SAVE);
		buttons += BindNamed("DCO_ArsLoLoad", BTN_LO_LOAD);
		buttons += BindNamed("DCO_ArsLoDelete", BTN_LO_DELETE);
		buttons += BindNamed("DCO_ArsLoPrev", BTN_LO_PREV);
		buttons += BindNamed("DCO_ArsLoNext", BTN_LO_NEXT);
		buttons += BindNamed("DCO_ArsCtxBack", BTN_CTX_BACK);
		buttons += BindNamed("DCO_ArsHintBtn", BTN_HINT);
		buttons += BindNamed("DCO_ArsUndo", BTN_UNDO);
		buttons += BindNamed("DCO_ArsRedo", BTN_REDO);
		buttons += BindNamed("DCO_ArsInvTab", BTN_INV_TAB);

		PinFrame("DCO_ArsenalScreen",   0,    0,     1,    1);
		PinFrame("DCO_ArsLeftCol",      0,    0,     0.22, 1);
		PinFrame("DCO_ArsRightCol",     0.78, 0,     1,    1);
		PinFrame("DCO_ArsHeader",       0.30, 0,     0.70, 0.042);
		PinFrame("DCO_ArsLoadoutPanel", 0.59, 0.03,  0.77, 0.62);
		PinFrame("DCO_ArsHintBox",      0.52, 0.55,  0.78, 0.935);
		PinFrame("DCO_ArsBottom",       0.22, 0.945, 0.78, 1);

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

		Print(string.Format("[DCO-GM] arsenal screen bound (screen=%1 search=%2 cats=%3/11 icons=%4/11 scrollL=%5 scrollR=%6 listL=%7 listR=%8)",
			m_wScreen != null, m_wSearch != null, cats, icons, m_wScrollL != null, m_wScrollR != null, m_wListL != null, m_wListR != null), LogLevel.NORMAL);
		Print(string.Format("[DCO-GM] arsenal extras bound (header=%1 hover=%2 icons=%3/13 loRows=%4/8 buttons=%5/14 hint=%6)",
			m_wTarget != null, m_wHover != null && m_wHoverTxt != null, icons2, loRows, buttons, m_wHintBox != null), LogLevel.NORMAL);
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
		scroll.SetSliderPos(x, Math.Clamp(y - wheel * WHEEL_STEP_NORMALIZED, 0.0, 1.0));
		UpdateScrollBar(right);
		return true;
	}

	protected void ScheduleScrollBarUpdate()
	{
		GetGame().GetCallqueue().Remove(UpdateScrollBars);
		GetGame().GetCallqueue().CallLater(UpdateScrollBars, 0, false);
	}

	protected void UpdateScrollBars()
	{
		UpdateScrollBar(false);
		UpdateScrollBar(true);
	}

	protected void SetListContentHeight(bool right, int rowCount)
	{
		SizeLayoutWidget sizeHost = m_wListSizeL;
		if (right)
			sizeHost = m_wListSizeR;
		if (!sizeHost)
			return;
		sizeHost.SetHeightOverride(Math.Max(1.0, rowCount * ROW_HEIGHT));
	}

	protected void UpdateScrollBar(bool right)
	{
		ScrollLayoutWidget scroll = m_wScrollL;
		Widget list = m_wListL;
		Widget bar = m_wBarL;
		ImageWidget spacer = ImageWidget.Cast(m_wBarSpacerL);
		ImageWidget thumb = ImageWidget.Cast(m_wBarThumbL);
		if (right)
		{
			scroll = m_wScrollR;
			list = m_wListR;
			bar = m_wBarR;
			spacer = ImageWidget.Cast(m_wBarSpacerR);
			thumb = ImageWidget.Cast(m_wBarThumbR);
		}
		if (!scroll || !list || !bar || !spacer || !thumb)
			return;

		float viewportW;
		float viewportH;
		float contentW;
		float contentH;
		scroll.GetScreenSize(viewportW, viewportH);
		list.GetScreenSize(contentW, contentH);
		bool needed = contentH > viewportH + 1;
		bar.SetVisible(needed);
		if (!needed)
			return;

		float trackW;
		float trackH;
		bar.GetScreenSize(trackW, trackH);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			trackH = workspace.DPIUnscale(trackH);
		if (trackH < 1 || contentH < 1)
			return;

		float thumbH = Math.Max(BAR_MIN_THUMB, trackH * Math.Min(1.0, viewportH / contentH));
		float sliderX;
		float sliderY;
		scroll.GetSliderPos(sliderX, sliderY);
		spacer.SetSize(BAR_WIDTH, (trackH - thumbH) * sliderY);
		thumb.SetSize(BAR_WIDTH, thumbH);
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
		float sliderX;
		scroll.GetSliderPos(sliderX, m_fBarStartPos);
		m_iBarStartMouseY = mouseY;
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
		float range = trackH - thumbH;
		if (range < 1)
		{
			StopScrollBarDrag();
			return;
		}

		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		float position = Math.Max(0.0, Math.Min(1.0, m_fBarStartPos + (mouseY - m_iBarStartMouseY) / range));
		float sliderX;
		float sliderY;
		scroll.GetSliderPos(sliderX, sliderY);
		scroll.SetSliderPos(sliderX, position);
		UpdateScrollBar(m_bBarDragRight);
	}

	protected void StopScrollBarDrag()
	{
		m_bBarDragging = false;
		GetGame().GetCallqueue().Remove(ScrollBarDragTick);
	}

	protected int LoadIcon(Widget root, string name, ResourceName tex)
	{
		ImageWidget w = ImageWidget.Cast(root.FindAnyWidget(name));
		if (!w)
			return 0;
		w.LoadImageTexture(0, tex);
		return 1;
	}

	protected int BindNamed(string name, int id)
	{
		if (!m_wRoot)
			return 0;
		ButtonWidget b = ButtonWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!b)
			return 0;
		AddHandler(b, id);
		return 1;
	}

	bool IsOpen()
	{
		return m_bOpen;
	}

	void OpenFor(SCR_EditableEntityComponent unit)
	{
		if (!m_wScreen || !unit)
			return;
		IEntity owner = unit.GetOwner();
		if (!owner)
			return;

		m_Target = unit;
		m_eCatL = EDCO_ArsenalCategory.PRIMARY;
		m_eCatR = EDCO_ArsenalCategory.MAGAZINES;
		m_bLastRight = false;
		m_sLastSearch = "";
		m_iDetailIdx = -1;
		m_bPoolOverride = false;
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

		if (m_wTarget)
			m_wTarget.SetText("EDITING:  " + TargetName(unit));
		if (m_wHintBox)
			m_wHintBox.SetVisible(false);
		if (m_wHover)
			m_wHover.SetVisible(false);
		RefreshContextItems();	// fills m_KitItems FIRST - the right-column context derives from them.
		RefreshLists();
		RefreshWeight();
		FocusCameraOn(owner);
		SetOpen(true);
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
			if (wasOpen)
				DCO_GMUIController.ReleaseMenuFocus();
			StopScrollBarDrag();
			if (m_Target && m_Target.GetOwner())
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
		if (!m_wRoot)
			return;
		if (hide)
		{
			m_HiddenSiblings.Clear();
			Widget c = m_wRoot.GetChildren();
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
		if (DCO_GMTheme.Get().IsMasterHidden())
			return;
		if (!m_bOpen)
			return;
		if (!m_Target || !m_Target.GetOwner())
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
			ResetScrolls();
			RefreshContextItems();
			RefreshLists();
			return true;
		}
		if (id >= BTN_ROWR_BASE && id < BTN_ROWR_BASE + MAX_DYN_ROWS)
		{
			int idxR = id - BTN_ROWR_BASE;
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
				m_bPoolOverride = !m_bPoolOverride;
				ResetScrolls();
				RefreshContextItems();
				RefreshLists();
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
				ResetScrolls();
				RefreshContextItems();
				RefreshLists();
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
			if (idxL < m_FilteredL.Count())
			{
				m_bLastRight = false;
				m_bPoolOverride = false;
				m_bInvMode = false;
				RouteEntry(m_FilteredL[idxL]);
				RefreshContextItems();
				RefreshLists();
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
		IEntity owner = m_Target.GetOwner();
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
	}

	// Inventory mode rendering: aggregate everything the character carries into the right list.
	protected void RenderInventory()
	{
		BuildInvAggregation();

		int shown = Math.Min(m_InvPrefabs.Count(), MAX_DYN_ROWS);
		if (m_wRightTitle)
		{
			if (shown < m_InvPrefabs.Count())
				m_wRightTitle.SetText(string.Format("INVENTORY - EVERYTHING CARRIED (%1 of %2 - refine search)", shown, m_InvPrefabs.Count()));
			else
				m_wRightTitle.SetText(string.Format("INVENTORY - EVERYTHING CARRIED (%1)", m_InvPrefabs.Count()));
		}
		if (m_wCtxBack)
			m_wCtxBack.SetVisible(false);

		EnsureRows(m_DynR, m_wListR, shown, true);
		SetListContentHeight(true, shown);

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
				row.m_Txt.SetText(string.Format("%1x  %2", m_InvCounts[i], m_InvNames[i]));
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
		IEntity owner = m_Target.GetOwner();
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
				m_wRightTitle.SetText("AMMO + ITEMS" + suffix);
			IEntity candidate = ContextCandidate();
			if (m_wCtxBack)
				m_wCtxBack.SetVisible(candidate != null);
			if (m_wCtxBackTxt && candidate)
				m_wCtxBackTxt.SetText("< BACK TO " + ContextName(candidate));
			return;
		}
		if (m_wCtxBack)
			m_wCtxBack.SetVisible(true);
		if (m_wCtxBackTxt)
			m_wCtxBackTxt.SetText("< ALL ITEMS");
		if (!m_wRightTitle)
			return;
		string nm = ContextName(ctxItem);
		if (m_iDetailIdx <= 2)
			m_wRightTitle.SetText(nm + " - COMPATIBLE" + suffix);
		else
			m_wRightTitle.SetText(nm + " - INSERT ITEMS" + suffix);
	}

	// List size readout for titles; the creation cap is never silent.
	protected string CountSuffix(int total)
	{
		if (total > MAX_DYN_ROWS)
			return string.Format("  (%1 of %2 - refine search)", MAX_DYN_ROWS, total);
		return string.Format("  (%1)", total);
	}

	protected void RefreshSide(array<DCO_ArsenalEntry> filtered, array<ref DCO_ArsDynRow> pool, Widget list, bool right)
	{
		int shown = Math.Min(filtered.Count(), MAX_DYN_ROWS);
		if (shown < filtered.Count())
			Print(string.Format("[DCO-ARS] list capped at %1 of %2 - refine the search", shown, filtered.Count()), LogLevel.NORMAL);
		EnsureRows(pool, list, shown, right);
		SetListContentHeight(right, shown);

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
				row.m_Txt.SetText(string.Format("%1  [%2]", e.m_sName, e.m_sFactionKey));
			if (row.m_Img && pm)
				pm.SetPreviewItemFromPrefab(row.m_Img, e.m_Prefab);
		}
		ScheduleScrollBarUpdate();
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
		m_wHoverTxt.SetText(label);
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (w && ws)
		{
			float x, y, sw, sh;
			w.GetScreenPos(x, y);
			w.GetScreenSize(sw, sh);
			float ux = ws.DPIUnscale(x + sw * 0.5) - 90;
			float uy = ws.DPIUnscale(y) - 30;
			if (ux < 0)
				ux = 0;
			if (uy < 0)
				uy = ws.DPIUnscale(y + sh) + 4;	// no room above -> flip below.
			FrameSlot.SetPos(m_wHover, ux, uy);
			FrameSlot.SetSize(m_wHover, 180, 24);
		}
		m_wHover.SetVisible(true);
	}

	protected string HoverTextFor(int id)
	{
		if (id >= BTN_CAT_BASE && id < BTN_CAT_BASE + CATS)
			return CAT_NAMES[id - BTN_CAT_BASE];
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
		}
		return "";
	}

	// Resolve the equipped item per bucket 0-6 (PRI..HEAD).
	protected void RefreshContextItems()
	{
		if (!m_Target)
			return;
		IEntity owner = m_Target.GetOwner();
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
	}

	protected void RefreshWeight()
	{
		if (!m_wWeight || !m_Target)
			return;
		IEntity owner = m_Target.GetOwner();
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
			string label = rec.m_sName;
			if (!avail)
				label = label + "  [MISSING CONTENT]";
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
		{
			Print(string.Format("[DCO-ARS] loadout '%1' is unavailable here - not selectable", rec.m_sName), LogLevel.NORMAL);
			return;
		}
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
		IEntity owner = m_Target.GetOwner();
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
		IEntity owner = m_Target.GetOwner();
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
		if (!m_Target)
			return;
		IEntity owner = m_Target.GetOwner();
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
		if (m_Target && m_Target.GetOwner())
			RouteVerb(DCO_ArsenalServer.VERB_RELEASE, "");
		HideShell(false);
		GetGame().GetCallqueue().Remove(Poll);
		GetGame().GetCallqueue().Remove(UpdateScrollBars);
		StopScrollBarDrag();
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
		m_wRoot = null;
	}
}
