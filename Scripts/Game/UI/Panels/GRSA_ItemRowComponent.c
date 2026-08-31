class GRSA_ItemRowComponent : SCR_ButtonBaseComponent
{
	protected ref GRSA_ItemEntry m_Entry;
	protected TextWidget m_wName;
	protected TextWidget m_wCost;
	protected Widget m_wCostSize;
	protected TextWidget m_wState;
	protected ItemPreviewWidget m_wThumb;
	protected Widget m_wQtyChip;
	protected TextWidget m_wQtyValue;
	protected Widget m_wMinusButton;
	protected Widget m_wPlusButton;
	protected SCR_ButtonTextComponent m_MinusComponent;
	protected SCR_ButtonTextComponent m_PlusComponent;
	protected int m_iCount;
	protected bool m_bRowFocused;
	protected bool m_bRowHovered;

	ref ScriptInvoker m_OnEntryFocused = new ScriptInvoker();
	ref ScriptInvoker m_OnEntryClicked = new ScriptInvoker();

	//! (GRSA_ItemRowComponent row, int delta)
	ref ScriptInvoker m_OnQtyDelta = new ScriptInvoker();

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
		m_wState = TextWidget.Cast(w.FindAnyWidget("RowState"));
		if (m_wState)
			m_wState.SetColor(GRSA_Theme.Accent());
		m_wThumb = ItemPreviewWidget.Cast(w.FindAnyWidget("RowThumb"));

		m_wQtyChip = w.FindAnyWidget("QtyChip");
		m_wQtyValue = TextWidget.Cast(w.FindAnyWidget("QtyValue"));
		m_wMinusButton = w.FindAnyWidget("QtyMinus");
		if (m_wMinusButton)
		{
			m_MinusComponent = SCR_ButtonTextComponent.Cast(m_wMinusButton.FindHandler(SCR_ButtonTextComponent));
			if (m_MinusComponent)
				m_MinusComponent.m_OnClicked.Insert(OnMinusPressed);
		}
		m_wPlusButton = w.FindAnyWidget("QtyPlus");
		if (m_wPlusButton)
		{
			m_PlusComponent = SCR_ButtonTextComponent.Cast(m_wPlusButton.FindHandler(SCR_ButtonTextComponent));
			if (m_PlusComponent)
				m_PlusComponent.m_OnClicked.Insert(OnPlusPressed);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMinusPressed(SCR_ButtonBaseComponent button)
	{
		m_OnQtyDelta.Invoke(this, -1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlusPressed(SCR_ButtonBaseComponent button)
	{
		m_OnQtyDelta.Invoke(this, 1);
	}

	//------------------------------------------------------------------------------------------------
	void SetEntry(GRSA_ItemEntry entry, bool usesSupplies)
	{
		m_Entry = entry;
		if (!entry)
			return;

		if (m_wThumb)
		{
			ItemPreviewManagerEntity previewManager = GRSA_ItemIntel.GetPreviewManager();
			if (previewManager)
				previewManager.SetPreviewItemFromPrefab(m_wThumb, entry.m_Prefab);
		}

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

		if (m_wName)
			m_wName.SetText(label);

		SetCostText(string.Empty);

		SetStateText(stateText);

		if (m_wThumb)
		{
			ItemPreviewManagerEntity previewManager = GRSA_ItemIntel.GetPreviewManager();
			if (previewManager)
			{
				if (thumbPrefab.IsEmpty())
					previewManager.SetPreviewItem(m_wThumb, null);
				else
					previewManager.SetPreviewItemFromPrefab(m_wThumb, thumbPrefab);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetStateText(string state)
	{
		if (m_wState)
			m_wState.SetText(state);
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
	//! Stackable rows carry a count; the < N > stepper pops on the focused or hovered row only.
	void SetCount(int count)
	{
		m_iCount = count;
		if (m_wQtyValue)
			m_wQtyValue.SetTextFormat("%1", count);
		UpdateQtyChip();
	}

	//------------------------------------------------------------------------------------------------
	int GetCount()
	{
		return m_iCount;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateQtyChip()
	{
		bool show = (m_bRowFocused || m_bRowHovered) && m_iCount > 0;
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
		bool result = super.OnMouseLeave(w, enterW, x, y);
		m_bRowHovered = false;
		UpdateQtyChip();
		return result;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_wMinusButton && w == m_wMinusButton)
			return false;
		if (m_wPlusButton && w == m_wPlusButton)
			return false;

		bool result = super.OnClick(w, x, y, button);
		m_OnEntryClicked.Invoke(this);
		return result;
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

		if (m_MinusComponent)
			m_MinusComponent.m_OnClicked.Remove(OnMinusPressed);
		if (m_PlusComponent)
			m_PlusComponent.m_OnClicked.Remove(OnPlusPressed);
		super.HandlerDeattached(w);
	}
}
