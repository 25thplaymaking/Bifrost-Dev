class GRSA_ItemRowQuantityHandler : ScriptedWidgetEventHandler
{
	protected GRSA_ItemRowComponent m_Owner;
	protected int m_iDelta;

	void GRSA_ItemRowQuantityHandler(GRSA_ItemRowComponent owner, int delta = 0)
	{
		m_Owner = owner;
		m_iDelta = delta;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Owner || button != 0 || m_iDelta == 0)
			return false;
		m_Owner.ApplyQuantityDelta(m_iDelta);
		return true;
	}

	override bool OnChange(Widget w, bool finished)
	{
		if (!m_Owner)
			return false;
		return m_Owner.OnQuantityEntryChanged(EditBoxWidget.Cast(w), finished);
	}

	void Clear()
	{
		m_Owner = null;
	}
}

class GRSA_ItemRowComponent : SCR_ButtonBaseComponent
{
	protected ref GRSA_ItemEntry m_Entry;
	protected TextWidget m_wName;
	protected TextWidget m_wCost;
	protected Widget m_wCostSize;
	protected Widget m_wContextHint;
	protected TextWidget m_wState;
	protected ItemPreviewWidget m_wThumb;
	protected ImageWidget m_wIcon;
	protected Widget m_wQtyChip;
	protected TextWidget m_wQtyValue;
	protected EditBoxWidget m_wQtyEntry;
	protected ref GRSA_ItemRowQuantityHandler m_QtyEntryHandler;
	protected ref GRSA_ItemRowQuantityHandler m_MinusHandler;
	protected ref GRSA_ItemRowQuantityHandler m_PlusHandler;
	protected Widget m_wMinusButton;
	protected Widget m_wPlusButton;
	protected int m_iCount;
	protected bool m_bRowFocused;
	protected bool m_bRowHovered;
	protected bool m_bQuantityControlsEnabled;
	protected bool m_bDirectQuantityEntryEnabled;
	protected bool m_bSyncingQuantityEntry;
	protected bool m_bSecondaryClickEnabled;

	ref ScriptInvoker m_OnEntryFocused = new ScriptInvoker();
	ref ScriptInvoker m_OnEntryClicked = new ScriptInvoker();
	ref ScriptInvoker m_OnEntrySecondaryClicked = new ScriptInvoker();

	//! (GRSA_ItemRowComponent row, int delta)
	ref ScriptInvoker m_OnQtyDelta = new ScriptInvoker();
	//! (GRSA_ItemRowComponent row, int value)
	ref ScriptInvoker m_OnQtySet = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		m_BackgroundDefault = GRSA_Theme.MapButtonAccent(m_BackgroundDefault);
		m_BackgroundHovered = GRSA_Theme.MapButtonAccent(m_BackgroundHovered);
		m_BackgroundSelected = GRSA_Theme.MapButtonAccent(m_BackgroundSelected);
		m_BackgroundSelectedHovered = GRSA_Theme.MapButtonAccent(m_BackgroundSelectedHovered);
		m_BackgroundClicked = GRSA_Theme.MapButtonAccent(m_BackgroundClicked);
		super.HandlerAttached(w);
		GRSA_Theme.ApplyAccentWidgets(w);
		m_wName = TextWidget.Cast(w.FindAnyWidget("RowName"));
		m_wCost = TextWidget.Cast(w.FindAnyWidget("RowCost"));
		m_wCostSize = w.FindAnyWidget("CostSize");
		m_wContextHint = w.FindAnyWidget("RowContextHintSize");
		Widget contextKeyBackground = w.FindAnyWidget("m_KeyBG");
		if (contextKeyBackground)
			contextKeyBackground.SetOpacity(0);
		Widget contextKeyGlow = w.FindAnyWidget("m_Glow");
		if (contextKeyGlow)
			contextKeyGlow.SetOpacity(0);
		m_wState = TextWidget.Cast(w.FindAnyWidget("RowState"));
		if (m_wState)
			m_wState.SetColor(GRSA_Theme.Accent());
		m_wThumb = ItemPreviewWidget.Cast(w.FindAnyWidget("RowThumb"));
		m_wIcon = ImageWidget.Cast(w.FindAnyWidget("RowIcon"));

		m_wQtyChip = w.FindAnyWidget("QtyChip");
		m_wQtyValue = TextWidget.Cast(w.FindAnyWidget("QtyValue"));
		m_wQtyEntry = EditBoxWidget.Cast(w.FindAnyWidget("QtyEntry"));
		if (m_wQtyEntry)
		{
			m_QtyEntryHandler = new GRSA_ItemRowQuantityHandler(this);
			m_wQtyEntry.AddHandler(m_QtyEntryHandler);
		}
		m_wMinusButton = w.FindAnyWidget("QtyMinus");
		if (m_wMinusButton)
		{
			m_MinusHandler = new GRSA_ItemRowQuantityHandler(this, -1);
			m_wMinusButton.AddHandler(m_MinusHandler);
		}
		m_wPlusButton = w.FindAnyWidget("QtyPlus");
		if (m_wPlusButton)
		{
			m_PlusHandler = new GRSA_ItemRowQuantityHandler(this, 1);
			m_wPlusButton.AddHandler(m_PlusHandler);
		}
	}

	//------------------------------------------------------------------------------------------------
	void ApplyQuantityDelta(int delta)
	{
		m_OnQtyDelta.Invoke(this, delta);
	}

	//------------------------------------------------------------------------------------------------
	bool OnQuantityEntryChanged(EditBoxWidget editBox, bool finished)
	{
		if (!editBox || editBox != m_wQtyEntry || !finished)
			return false;
		if (!m_bSyncingQuantityEntry && m_bDirectQuantityEntryEnabled)
			m_OnQtySet.Invoke(this, editBox.GetText().ToInt());
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void SetEntry(GRSA_ItemEntry entry, bool usesSupplies)
	{
		m_Entry = entry;
		SetContextHintVisible(false);
		if (!entry)
			return;

		SetThumbnail(entry.m_Prefab);

		if (m_wName)
			m_wName.SetText(entry.m_sDisplayName);

		if (usesSupplies && entry.m_iSupplyCost > 0)
			SetCostText(entry.m_iSupplyCost.ToString());
		else
			SetCostText(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	GRSA_ItemEntry GetEntry()
	{
		return m_Entry;
	}

	//------------------------------------------------------------------------------------------------
	//! Repurposes the row as a hardpoint entry: label, attached item as state, thumb of the item.
	void SetSlotDisplay(string label, string stateText, ResourceName thumbPrefab)
	{
		m_Entry = null;
		m_iCount = 0;
		SetContextHintVisible(false);

		if (m_wName)
			m_wName.SetText(label);

		SetCostText(string.Empty);

		SetStateText(stateText);

		SetThumbnail(thumbPrefab);
	}

	//------------------------------------------------------------------------------------------------
	void SetThumbnail(ResourceName prefab)
	{
		if (m_wIcon)
			m_wIcon.SetVisible(false);
		if (m_wThumb)
			m_wThumb.SetVisible(true);
		GRSA_ItemIntel.SetThumbnail(m_wThumb, prefab);
	}

	//------------------------------------------------------------------------------------------------
	void SetIconTexture(ResourceName texture)
	{
		if (m_wThumb)
		{
			ItemPreviewManagerEntity previewManager = GRSA_ItemIntel.GetPreviewManager();
			if (previewManager)
				previewManager.SetPreviewItem(m_wThumb, null);
			m_wThumb.SetVisible(false);
		}
		if (!m_wIcon)
			return;
		m_wIcon.LoadImageTexture(0, texture);
		m_wIcon.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	void SetStateText(string state)
	{
		if (m_wState)
			m_wState.SetText(state);
	}

	//------------------------------------------------------------------------------------------------
	void UseOpaqueBackground()
	{
		m_BackgroundDefault = Color.FromRGBA(8, 9, 10, 248);
		m_BackgroundHovered = Color.FromRGBA(25, 26, 28, 252);
		Widget root = GetRootWidget();
		if (!root)
			return;
		Widget background = root.FindAnyWidget("Background");
		if (background)
			background.SetColor(m_BackgroundDefault);
	}

	//------------------------------------------------------------------------------------------------
	//! Plain text in the cost column for rows that carry no supply price, e.g. per-item weight.
	void SetMetaText(string text)
	{
		SetCostText(text);
	}

	//------------------------------------------------------------------------------------------------
	//! Empty metadata must release its fixed-width column so the item name can use that space.
	protected void SetCostText(string text)
	{
		if (m_wCost)
			m_wCost.SetText(text);
		if (m_wCostSize)
			m_wCostSize.SetVisible(!text.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	//! Contents rows carry a count; the < N > stepper stays reachable at zero on focus or hover.
	void SetCount(int count)
	{
		m_iCount = count;
		if (m_wQtyValue)
			m_wQtyValue.SetTextFormat("%1", count);
		if (m_wQtyEntry)
		{
			m_bSyncingQuantityEntry = true;
			m_wQtyEntry.SetText(count.ToString());
			m_bSyncingQuantityEntry = false;
		}
		UpdateQtyChip();
	}

	//------------------------------------------------------------------------------------------------
	int GetCount()
	{
		return m_iCount;
	}

	//------------------------------------------------------------------------------------------------
	void SetQuantityControlsEnabled(bool enabled)
	{
		m_bQuantityControlsEnabled = enabled;
		UpdateQtyChip();
	}

	//------------------------------------------------------------------------------------------------
	void SetDirectQuantityEntryEnabled(bool enabled)
	{
		m_bDirectQuantityEntryEnabled = enabled;
		if (m_wQtyValue)
			m_wQtyValue.SetVisible(!enabled);
		if (m_wQtyEntry)
			m_wQtyEntry.SetVisible(enabled);
		UpdateQtyChip();
	}

	//------------------------------------------------------------------------------------------------
	void EnableSecondaryClick(bool enabled)
	{
		m_bSecondaryClickEnabled = enabled;
	}

	//------------------------------------------------------------------------------------------------
	void SetContextHintVisible(bool visible)
	{
		if (m_wContextHint)
			m_wContextHint.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateQtyChip()
	{
		bool show = m_bQuantityControlsEnabled && (m_bRowFocused || m_bRowHovered);
		if (m_wQtyChip)
			m_wQtyChip.SetVisible(show);
		if (m_wState)
			m_wState.SetVisible(!show);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnFocus(Widget w, int x, int y)
	{
		bool result = super.OnFocus(w, x, y);
		m_bRowFocused = true;
		UpdateQtyChip();
		m_OnEntryFocused.Invoke(this);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnFocusLost(Widget w, int x, int y)
	{
		bool result = super.OnFocusLost(w, x, y);
		m_bRowFocused = false;
		UpdateQtyChip();
		return result;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		bool result = super.OnMouseEnter(w, x, y);
		m_bRowHovered = true;
		UpdateQtyChip();
		return result;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		Widget cursor = enterW;
		while (cursor)
		{
			if (cursor == w)
				return false;
			cursor = cursor.GetParent();
		}

		bool result = super.OnMouseLeave(w, enterW, x, y);
		m_bRowHovered = false;
		UpdateQtyChip();
		return result;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button == 1 && m_bSecondaryClickEnabled)
		{
			m_OnEntrySecondaryClicked.Invoke(this);
			return true;
		}

		return super.OnMouseButtonDown(w, x, y, button);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (button == 1 && m_bSecondaryClickEnabled)
			return true;

		return super.OnMouseButtonUp(w, x, y, button);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 1 && m_bSecondaryClickEnabled)
			return true;
		if (m_wMinusButton && (IsWidgetWithin(w, m_wMinusButton) || IsPointWithin(m_wMinusButton, x, y)))
			return false;
		if (m_wPlusButton && (IsWidgetWithin(w, m_wPlusButton) || IsPointWithin(m_wPlusButton, x, y)))
			return false;
		if (m_wQtyEntry && (IsWidgetWithin(w, m_wQtyEntry) || IsPointWithin(m_wQtyEntry, x, y)))
			return false;

		bool result = super.OnClick(w, x, y, button);
		m_OnEntryClicked.Invoke(this);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsWidgetWithin(Widget widget, Widget ancestor)
	{
		while (widget)
		{
			if (widget == ancestor)
				return true;
			widget = widget.GetParent();
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsPointWithin(Widget widget, int x, int y)
	{
		if (!widget || !widget.IsVisible())
			return false;

		float left, top, width, height;
		widget.GetScreenPos(left, top);
		widget.GetScreenSize(width, height);
		return x >= left && x <= left + width && y >= top && y <= top + height;
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		if (m_wThumb)
		{
			ItemPreviewManagerEntity previewManager = GRSA_ItemIntel.GetPreviewManager();
			if (previewManager)
				previewManager.SetPreviewItem(m_wThumb, null);
		}

		if (m_wMinusButton && m_MinusHandler)
			m_wMinusButton.RemoveHandler(m_MinusHandler);
		if (m_wPlusButton && m_PlusHandler)
			m_wPlusButton.RemoveHandler(m_PlusHandler);
		if (m_wQtyEntry && m_QtyEntryHandler)
			m_wQtyEntry.RemoveHandler(m_QtyEntryHandler);
		if (m_QtyEntryHandler)
			m_QtyEntryHandler.Clear();
		m_QtyEntryHandler = null;
		m_MinusHandler = null;
		m_PlusHandler = null;
		super.HandlerDeattached(w);
	}
}
