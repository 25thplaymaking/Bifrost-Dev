
class DCO_ContextMenuHandler : ScriptedWidgetEventHandler
{
	protected DCO_GMContextMenu m_Owner;

	void DCO_ContextMenuHandler(DCO_GMContextMenu owner)
	{
		m_Owner = owner;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner)
			return m_Owner.OnMenuButton(w);
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_Owner)
			m_Owner.SetRowHovered(w, true);
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (m_Owner)
			m_Owner.SetRowHovered(w, false);
		return false;
	}
}

class DCO_GMContextMenu
{
	static const int SLOTS = 18;
	protected static const int PAGED_ACTION_SLOTS = SLOTS - 2;
	protected static const int PAGE_PREV_ID = -1001;
	protected static const int PAGE_NEXT_ID = -1002;
	static const float EDGE_GAP = 8;
	static const float FALLBACK_WIDTH = 404;
	static const float FALLBACK_ROW_HEIGHT = 36;
	static const int MAX_TITLE_CHARS = 34;
	static const int MAX_LABEL_CHARS = 54;

	protected Widget m_wRoot;
	protected Widget m_wMenu;
	protected Widget m_wBackdrop;	// full-screen catcher behind the menu; click outside -> close.
	protected ref DCO_ContextMenuHandler m_Handler;
	protected ref array<ButtonWidget> m_Btns = {};
	protected ref array<TextWidget> m_Labels = {};
	protected ref array<int> m_ActionIds = {};
	protected ref array<string> m_AllLabels = {};
	protected ref array<int> m_AllIds = {};
	protected ref array<bool> m_AllEnabled = {};
	protected TextWidget m_wTitle;
	protected TextWidget m_wSubtitle;
	protected TextWidget m_wFooter;
	protected int m_iVisibleItems;
	protected int m_iPage;
	protected int m_iSelectedId = -1;
	protected float m_fAnchorX;
	protected float m_fAnchorY;
	protected float m_fAnchorW;
	protected float m_fAnchorH;
	protected bool m_bAdjacent;
	protected Widget m_wAvoidPanel;

	protected ScriptInvoker m_OnAction;
	protected SCR_EditableEntityComponent m_Entity;

	void Init(Widget shellRoot)
	{
		if (!shellRoot)
			return;
		m_wRoot = shellRoot;
		m_wMenu = shellRoot.FindAnyWidget("DCO_ContextMenu");
		if (!m_wMenu)
		{
			Print("[DCO-GM] context menu: DCO_ContextMenu not found (layout not reloaded yet?)", LogLevel.WARNING);
			return;
		}
		m_Handler = new DCO_ContextMenuHandler(this);
		m_wTitle = TextWidget.Cast(m_wMenu.FindAnyWidget("DCO_MenuTitle"));
		m_wSubtitle = TextWidget.Cast(m_wMenu.FindAnyWidget("DCO_MenuSubtitle"));
		m_wFooter = TextWidget.Cast(m_wMenu.FindAnyWidget("DCO_MenuFooter"));
		for (int i = 0; i < SLOTS; i++)
		{
			ButtonWidget b = ButtonWidget.Cast(m_wMenu.FindAnyWidget(string.Format("DCO_Menu_%1", i)));
			if (b)
				b.AddHandler(m_Handler);
			m_Btns.Insert(b);
			m_Labels.Insert(TextWidget.Cast(m_wMenu.FindAnyWidget(string.Format("DCO_Menu_%1_Label", i))));
		}
		m_wMenu.SetVisible(false);

		m_wBackdrop = shellRoot.FindAnyWidget("DCO_MenuBackdrop");
		if (m_wBackdrop)
		{
			ButtonWidget bd = ButtonWidget.Cast(m_wBackdrop);
			if (bd)
				bd.AddHandler(m_Handler);
			m_wBackdrop.SetVisible(false);
		}
	}

	void Show(notnull array<string> labels, notnull array<int> ids, int x, int y, ScriptInvoker onAction, SCR_EditableEntityComponent e)
	{
		ShowInternal(labels, ids, null, x, y, 0, 0, false, null, "ACTIONS", "Choose an action", -1, onAction, e);
	}

	void ShowWithAvailability(notnull array<string> labels, notnull array<int> ids, notnull array<bool> enabled, int x, int y, ScriptInvoker onAction, SCR_EditableEntityComponent e)
	{
		ShowInternal(labels, ids, enabled, x, y, 0, 0, false, null, "ACTIONS", "Choose an action", -1, onAction, e);
	}

	void ShowTitled(notnull array<string> labels, notnull array<int> ids, int x, int y, string title, ScriptInvoker onAction, SCR_EditableEntityComponent e)
	{
		ShowInternal(labels, ids, null, x, y, 0, 0, false, null, title, "Choose an option", -1, onAction, e);
	}

	void ShowTitledDetailed(notnull array<string> labels, notnull array<int> ids, int x, int y, string title, string subtitle, int selectedId, ScriptInvoker onAction, SCR_EditableEntityComponent e)
	{
		ShowInternal(labels, ids, null, x, y, 0, 0, false, null, title, subtitle, selectedId, onAction, e);
	}

	// Open a dropdown beside a source panel.
	void ShowAdjacent(notnull array<string> labels, notnull array<int> ids, Widget anchor, Widget avoidPanel, string title, ScriptInvoker onAction, SCR_EditableEntityComponent e)
	{
		if (!anchor)
			return;
		float x, y, w, h;
		anchor.GetScreenPos(x, y);
		anchor.GetScreenSize(w, h);
		ShowInternal(labels, ids, null, x, y, w, h, true, avoidPanel, title, "Choose an option", -1, onAction, e);
	}

	void ShowAdjacentWithAvailability(notnull array<string> labels, notnull array<int> ids, notnull array<bool> enabled, Widget anchor, Widget avoidPanel, string title, ScriptInvoker onAction, SCR_EditableEntityComponent e)
	{
		if (!anchor)
			return;
		float x, y, w, h;
		anchor.GetScreenPos(x, y);
		anchor.GetScreenSize(w, h);
		ShowInternal(labels, ids, enabled, x, y, w, h, true, avoidPanel, title, "Choose an option", -1, onAction, e);
	}

	// Populate first, then measure and place.
	protected void ShowInternal(notnull array<string> labels, notnull array<int> ids, array<bool> enabled, float x, float y, float anchorW, float anchorH, bool adjacent, Widget avoidPanel, string title, string subtitle, int selectedId, ScriptInvoker onAction, SCR_EditableEntityComponent e)
	{
		if (!m_wMenu)
			return;
		m_OnAction = onAction;
		m_Entity = e;
		m_AllLabels.Clear();
		m_AllIds.Clear();
		m_AllEnabled.Clear();
		int itemCount = Math.Min(labels.Count(), ids.Count());
		for (int itemIndex = 0; itemIndex < itemCount; itemIndex++)
		{
			m_AllLabels.Insert(labels[itemIndex]);
			m_AllIds.Insert(ids[itemIndex]);
			if (enabled && itemIndex < enabled.Count())
				m_AllEnabled.Insert(enabled[itemIndex]);
			else
				m_AllEnabled.Insert(true);
		}
		m_iPage = 0;
		m_iSelectedId = selectedId;
		m_bAdjacent = adjacent;
		m_wAvoidPanel = avoidPanel;
		if (m_wTitle)
			m_wTitle.SetText(BoundText(title, MAX_TITLE_CHARS));
		if (m_wSubtitle)
			m_wSubtitle.SetText(BoundText(subtitle, MAX_LABEL_CHARS));
		RenderPage();

		WorkspaceWidget ws = GetGame().GetWorkspace();
		m_fAnchorX = x;
		m_fAnchorY = y;
		m_fAnchorW = anchorW;
		m_fAnchorH = anchorH;
		if (ws)
		{
			m_fAnchorX = ws.DPIUnscale(x);
			m_fAnchorY = ws.DPIUnscale(y);
			m_fAnchorW = ws.DPIUnscale(anchorW);
			m_fAnchorH = ws.DPIUnscale(anchorH);
		}
		if (m_wBackdrop)
			m_wBackdrop.SetVisible(true);
		m_wMenu.SetVisible(true);
		PositionMenu();

		GetGame().GetCallqueue().Remove(PositionMenu);
		GetGame().GetCallqueue().CallLater(PositionMenu, 0, false);
	}

	protected void RenderPage()
	{
		m_ActionIds.Clear();
		bool paged = m_AllLabels.Count() > SLOTS;
		int pageSize = SLOTS;
		if (paged)
			pageSize = PAGED_ACTION_SLOTS;
		int pageCount = 1;
		if (paged)
			pageCount = (m_AllLabels.Count() + pageSize - 1) / pageSize;
		if (m_iPage >= pageCount)
			m_iPage = pageCount - 1;

		int first = m_iPage * pageSize;
		int last = Math.Min(first + pageSize, m_AllLabels.Count());
		int slot;
		for (int itemIndex = first; itemIndex < last; itemIndex++)
		{
			ButtonWidget b = m_Btns[slot];
			TextWidget lbl = m_Labels[slot];
			bool canPerform = m_AllEnabled[itemIndex];
			if (lbl)
			{
				string rowLabel = string.Format("%1  ·  %2", itemIndex + 1, m_AllLabels[itemIndex]);
				if (m_AllIds[itemIndex] == m_iSelectedId)
					rowLabel = rowLabel + "  ·  SELECTED";
				if (!canPerform)
					rowLabel = rowLabel + "  ·  UNAVAILABLE";
				lbl.SetText(BoundText(rowLabel, MAX_LABEL_CHARS));
				if (!canPerform)
					lbl.SetColor(DCO_GMTheme.Get().m_MutedColor);
				else if (m_AllIds[itemIndex] == m_iSelectedId)
					lbl.SetColor(DCO_GMTheme.Get().m_AccentColor);
				else
					lbl.SetColor(ColorForId(m_AllIds[itemIndex]));
			}
			if (b)
			{
				b.SetVisible(true);
				b.SetEnabled(canPerform);
			}
			m_ActionIds.Insert(m_AllIds[itemIndex]);
			slot++;
		}

		if (paged && m_iPage > 0)
		{
			PopulatePageControl(slot, "< PREVIOUS PAGE", PAGE_PREV_ID);
			slot++;
		}
		if (paged && m_iPage + 1 < pageCount)
		{
			PopulatePageControl(slot, "NEXT PAGE >", PAGE_NEXT_ID);
			slot++;
		}

		m_iVisibleItems = slot;
		for (int hideIndex = slot; hideIndex < SLOTS; hideIndex++)
		{
			ButtonWidget hiddenButton = m_Btns[hideIndex];
			if (hiddenButton)
			{
				hiddenButton.SetEnabled(true);
				hiddenButton.SetVisible(false);
			}
		}

		if (m_wFooter)
		{
			if (paged)
				m_wFooter.SetText(string.Format("%1 OPTIONS  ·  PAGE %2/%3  ·  SELECT  ·  ESC BACK", m_AllLabels.Count(), m_iPage + 1, pageCount));
			else
				m_wFooter.SetText(string.Format("%1 OPTIONS  ·  SELECT  ·  ESC BACK", m_AllLabels.Count()));
		}
	}

	protected void PopulatePageControl(int slot, string label, int actionId)
	{
		if (slot < 0 || slot >= SLOTS)
			return;
		ButtonWidget button = m_Btns[slot];
		TextWidget text = m_Labels[slot];
		if (text)
		{
			text.SetText(label);
			text.SetColor(DCO_GMTheme.Get().m_AccentColor);
		}
		if (button)
		{
			button.SetEnabled(true);
			button.SetVisible(true);
		}
		m_ActionIds.Insert(actionId);
	}

	protected string BoundText(string value, int maxChars)
	{
		if (maxChars < 4 || value.Length() <= maxChars)
			return value;
		return value.Substring(0, maxChars - 3) + "...";
	}

	protected void PositionMenu()
	{
		if (!m_wMenu || !m_wMenu.IsVisible())
			return;
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return;

		float menuX, menuY, menuW, menuH;
		m_wMenu.GetScreenPos(menuX, menuY);
		m_wMenu.GetScreenSize(menuW, menuH);
		menuW = ws.DPIUnscale(menuW);
		menuH = ws.DPIUnscale(menuH);
		if (menuW <= 0)
			menuW = FALLBACK_WIDTH;
		if (menuH <= 0)
			menuH = 92 + m_iVisibleItems * FALLBACK_ROW_HEIGHT;

		float safeL, safeT, safeR, safeB;
		GetSafeViewport(safeL, safeT, safeR, safeB);
		float maxX = safeR - menuW;
		float maxY = safeB - menuH;
		if (maxX < safeL)
			maxX = safeL;
		if (maxY < safeT)
			maxY = safeT;

		float rightX = m_fAnchorX + EDGE_GAP;
		float leftX = m_fAnchorX - menuW - EDGE_GAP;
		float downY = m_fAnchorY;
		float upY = m_fAnchorY - menuH;
		if (m_bAdjacent && m_wAvoidPanel)
		{
			float ox, oy, ow, oh;
			m_wAvoidPanel.GetScreenPos(ox, oy);
			m_wAvoidPanel.GetScreenSize(ow, oh);
			ox = ws.DPIUnscale(ox);
			oy = ws.DPIUnscale(oy);
			ow = ws.DPIUnscale(ow);
			oh = ws.DPIUnscale(oh);
			rightX = ox + ow + EDGE_GAP;
			leftX = ox - menuW - EDGE_GAP;
			downY = m_fAnchorY;
			upY = m_fAnchorY + m_fAnchorH - menuH;
		}

		array<vector> candidates = {};
		candidates.Insert(Vector(Math.Clamp(rightX, safeL, maxX), Math.Clamp(downY, safeT, maxY), 0));
		candidates.Insert(Vector(Math.Clamp(rightX, safeL, maxX), Math.Clamp(upY, safeT, maxY), 0));
		candidates.Insert(Vector(Math.Clamp(leftX, safeL, maxX), Math.Clamp(downY, safeT, maxY), 0));
		candidates.Insert(Vector(Math.Clamp(leftX, safeL, maxX), Math.Clamp(upY, safeT, maxY), 0));

		vector best = candidates[0];
		float bestScore = CollisionScore(best[0], best[1], menuW, menuH);
		for (int ci = 1; ci < candidates.Count(); ci++)
		{
			vector candidate = candidates[ci];
			float score = CollisionScore(candidate[0], candidate[1], menuW, menuH);
			if (score < bestScore)
			{
				best = candidate;
				bestScore = score;
			}
		}
		FrameSlot.SetPos(m_wMenu, best[0], best[1]);
	}

	// The usable popup viewport is the world workspace between the persistent shell surfaces.
	protected void GetSafeViewport(out float left, out float top, out float right, out float bottom)
	{
		WorkspaceWidget ws = GetGame().GetWorkspace();
		float rx, ry, rw, rh;
		m_wRoot.GetScreenPos(rx, ry);
		m_wRoot.GetScreenSize(rw, rh);
		left = ws.DPIUnscale(rx) + EDGE_GAP;
		top = ws.DPIUnscale(ry) + EDGE_GAP;
		right = ws.DPIUnscale(rx + rw) - EDGE_GAP;
		bottom = ws.DPIUnscale(ry + rh) - EDGE_GAP;

		Widget fixedPanel = m_wRoot.FindAnyWidget("DCO_TopBar");
		if (fixedPanel && fixedPanel.IsVisibleInHierarchy())
		{
			fixedPanel.GetScreenPos(rx, ry); fixedPanel.GetScreenSize(rw, rh);
			top = Math.Max(top, ws.DPIUnscale(ry + rh) + EDGE_GAP);
		}
		fixedPanel = m_wRoot.FindAnyWidget("DCO_BottomBar");
		if (fixedPanel && fixedPanel.IsVisibleInHierarchy())
		{
			fixedPanel.GetScreenPos(rx, ry); fixedPanel.GetScreenSize(rw, rh);
			bottom = Math.Min(bottom, ws.DPIUnscale(ry) - EDGE_GAP);
		}
		fixedPanel = m_wRoot.FindAnyWidget("DCO_EditTree");
		if (fixedPanel && fixedPanel.IsVisibleInHierarchy())
		{
			fixedPanel.GetScreenPos(rx, ry); fixedPanel.GetScreenSize(rw, rh);
			left = Math.Max(left, ws.DPIUnscale(rx + rw) + EDGE_GAP);
		}
		fixedPanel = m_wRoot.FindAnyWidget("DCO_CreateBrowser");
		if (fixedPanel && fixedPanel.IsVisibleInHierarchy())
		{
			fixedPanel.GetScreenPos(rx, ry); fixedPanel.GetScreenSize(rw, rh);
			right = Math.Min(right, ws.DPIUnscale(rx) - EDGE_GAP);
		}
	}

	// Pick the candidate with the least overlap against every movable Bifrost surface.
	protected float CollisionScore(float x, float y, float width, float height)
	{
		array<string> panels = {"DCO_OrdersBox", "DCO_OverlayBar", "DCO_ScenarioPanel", "DCO_OptionsPanel", "DCO_SimPanel", "DCO_GizmoPanel", "DCO_NotifPanel", "DCO_ChatPanel", "DCO_TacticsPanel"};
		WorkspaceWidget ws = GetGame().GetWorkspace();
		float score = 0;
		foreach (string name : panels)
		{
			Widget panel = m_wRoot.FindAnyWidget(name);
			if (!panel || !panel.IsVisibleInHierarchy())
				continue;
			float px, py, pw, ph;
			panel.GetScreenPos(px, py);
			panel.GetScreenSize(pw, ph);
			px = ws.DPIUnscale(px); py = ws.DPIUnscale(py);
			pw = ws.DPIUnscale(pw); ph = ws.DPIUnscale(ph);
			float overlapW = Math.Min(x + width, px + pw) - Math.Max(x, px);
			float overlapH = Math.Min(y + height, py + ph) - Math.Max(y, py);
			if (overlapW > 0 && overlapH > 0)
				score += overlapW * overlapH;
		}
		return score;
	}

	protected Color ColorForId(int id)
	{
		DCO_GMTheme theme = DCO_GMTheme.Get();
		if (id == DCO_GMGroupOrders.ORD_HOLD)
			return Color.FromInt(DCO_GMTheme.SEM_HOSTILE);	// red - hold fire.
		if (id == DCO_GMGroupOrders.ORD_RESUME)
			return Color.FromInt(DCO_GMTheme.SEM_FRIENDLY);	// green - resume fire.
		if (id == DCO_GMGroupOrders.ORD_CANCEL_AMBUSH)
			return Color.FromRGBA(200, 124, 92, 255);	// muted - cancel.
		if (id == DCO_GMGroupOrders.ORD_AMBUSH || id == DCO_GMGroupOrders.ORD_QRF)
			return Color.FromRGBA(232, 172, 72, 255);	// amber - tactics verbs.
		if (id >= DCO_GMGroupOrders.ORD_FORM_WEDGE && id <= DCO_GMGroupOrders.ORD_FORM_STAGGERED)
			return Color.FromRGBA(92, 162, 232, 255);	// blue - formations.
		if (id >= DCO_GMGroupOrders.ORD_STANCE_STAND && id <= DCO_GMGroupOrders.ORD_STANCE_PRONE)
			return DCO_GMTheme.Get().m_MutedColor;	// grey - stances.
		if (DCO_GMGroupOrders.IsSubmenu(id))
			return theme.m_AccentColor;	// accent - drill-in headers + Back.
		return theme.m_TextColor;
	}

	void Hide()
	{
		bool wasOpen = IsOpen();
		GetGame().GetCallqueue().Remove(PositionMenu);
		if (m_wMenu)
			m_wMenu.SetVisible(false);
		if (m_wBackdrop)
			m_wBackdrop.SetVisible(false);
		if (wasOpen)
			DCO_GMUIController.ReleaseMenuFocus();
	}

	// Subtle row response without replacing the semantic order colours.
	void SetRowHovered(Widget w, bool hovered)
	{
		for (int i = 0; i < m_Btns.Count(); i++)
		{
			if (w != m_Btns[i] || !m_Labels[i])
				continue;
			if (hovered)
			{
				m_Labels[i].SetOpacity(1.0);
				m_Btns[i].SetOpacity(1.0);
			}
			else
			{
				m_Labels[i].SetOpacity(0.88);
				m_Btns[i].SetOpacity(0.92);
			}
			return;
		}
	}

	bool IsOpen()
	{
		return m_wMenu && m_wMenu.IsVisible();
	}

	bool OnMenuButton(Widget w)
	{
		if (w == m_wBackdrop)	// clicked outside the menu -> just close it.
		{
			Hide();
			return true;
		}
		for (int i = 0; i < m_Btns.Count(); i++)
		{
			if (w == m_Btns[i])
			{
				int actionId = -1;
				if (i < m_ActionIds.Count())
					actionId = m_ActionIds[i];
				if (actionId == PAGE_PREV_ID)
				{
					if (m_iPage > 0)
						m_iPage--;
					RenderPage();
					PositionMenu();
					GetGame().GetCallqueue().Remove(PositionMenu);
					GetGame().GetCallqueue().CallLater(PositionMenu, 0, false);
					return true;
				}
				if (actionId == PAGE_NEXT_ID)
				{
					m_iPage++;
					RenderPage();
					PositionMenu();
					GetGame().GetCallqueue().Remove(PositionMenu);
					GetGame().GetCallqueue().CallLater(PositionMenu, 0, false);
					return true;
				}

				ScriptInvoker cb = m_OnAction;
				SCR_EditableEntityComponent e = m_Entity;
				Hide();
				if (cb && actionId >= 0)
					cb.Invoke(actionId, e);
				return true;
			}
		}
		return false;
	}
}
