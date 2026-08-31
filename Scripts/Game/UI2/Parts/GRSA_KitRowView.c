//! One saved-kit row: binds the registered kit-row layout, fills the two-line meta, runs the
//! inline rename, and lights the LOADED/FAILED result chip. The row layout's inline action strip
//! stays hidden because row verbs live in the screen footer.
class GRSA_KitRowView
{
	protected static const ResourceName ROW_LAYOUT = "{8478E1B6D5E2427D}UI/layouts/Menus/Armory/GRSA_KitRow.layout";

	int m_iSlot;
	ref GRSA_KitFile m_Kit;
	Widget m_wRoot;
	SCR_ModularButtonComponent m_Button;
	protected TextWidget m_wName;
	protected TextWidget m_wRole;
	protected TextWidget m_wDot;
	protected TextWidget m_wAge;
	protected TextWidget m_wUnavail;
	protected Widget m_wLoadedChip;
	protected Widget m_wRenameEdit;
	protected SCR_EditBoxComponent m_RenameEditComp;
	protected bool m_bRenaming;

	//! (GRSA_KitRowView row, string newName) rename left the edit box with a changed, non-empty name.
	ref ScriptInvoker m_OnRenameCommitted = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	static GRSA_KitRowView Create(Widget listParent, int slot, notnull GRSA_KitFile kit)
	{
		if (!listParent)
			return null;

		Widget w = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, listParent);
		if (!w)
			return null;

		GRSA_KitRowView row = new GRSA_KitRowView();
		row.m_iSlot = slot;
		row.m_Kit = kit;
		row.m_wRoot = w;
		row.m_Button = SCR_ModularButtonComponent.FindComponent(w);
		row.m_wName = TextWidget.Cast(w.FindAnyWidget("KitName"));
		row.m_wRole = TextWidget.Cast(w.FindAnyWidget("KitRole"));
		row.m_wDot = TextWidget.Cast(w.FindAnyWidget("MetaDot"));
		row.m_wAge = TextWidget.Cast(w.FindAnyWidget("KitAge"));
		row.m_wUnavail = TextWidget.Cast(w.FindAnyWidget("KitUnavail"));
		row.m_wLoadedChip = w.FindAnyWidget("LoadedChip");
		row.m_wRenameEdit = w.FindAnyWidget("RenameEdit");
		if (row.m_wRenameEdit)
			row.m_RenameEditComp = SCR_EditBoxComponent.Cast(row.m_wRenameEdit.FindHandler(SCR_EditBoxComponent));

		Widget actions = w.FindAnyWidget("Actions");
		if (actions)
			actions.SetVisible(false);

		Widget stripe = w.FindAnyWidget("Stripe");
		if (stripe)
			stripe.SetVisible(false);

		if (row.m_wName)
		{
			string kitName = kit.m_sName;
			if (kitName.IsEmpty())
				kitName = string.Format("Kit %1", slot + 1);
			row.m_wName.SetText(kitName);
		}

		return row;
	}

	//------------------------------------------------------------------------------------------------
	void Remove()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
		m_wRoot = null;
	}

	//------------------------------------------------------------------------------------------------
	bool IsRenaming()
	{
		return m_bRenaming;
	}

	//------------------------------------------------------------------------------------------------
	void SetKit(GRSA_KitFile kit)
	{
		m_Kit = kit;
	}

	//------------------------------------------------------------------------------------------------
	//! Second line: item count, weight, supplies, rank gate; age chip; red availability warning.
	void FillMeta(GRSA_DraftService service)
	{
		if (!m_Kit)
			return;

		string info;
		array<ResourceName> prefabs = {};
		m_Kit.CollectItemPrefabs(prefabs);
		if (!prefabs.IsEmpty())
			info = string.Format("%1 ITEMS", prefabs.Count());

		if (m_Kit.m_fTotalWeight > 0)
			info = JoinMeta(info, string.Format("%1 KG", m_Kit.m_fTotalWeight.ToString(1, 1)));

		if (service && service.UsesSupplies() && m_Kit.m_fSupplyCost > 0)
		{
			int supplyInt = m_Kit.m_fSupplyCost;
			info = JoinMeta(info, string.Format("%1 SUPPLIES", supplyInt));
		}

		if (m_Kit.m_iRequiredRank > 0 && GRSA_DraftService.RanksActive())
			info = JoinMeta(info, typename.EnumToString(SCR_ECharacterRank, m_Kit.m_iRequiredRank));

		string ageText = FormatAge(m_Kit.m_iSavedAtUnix);

		bool hasInfo = !info.IsEmpty();
		if (m_wRole)
		{
			m_wRole.SetText(info);
			m_wRole.SetVisible(hasInfo);
		}
		if (m_wDot)
			m_wDot.SetVisible(hasInfo && !ageText.IsEmpty());
		if (m_wAge)
			m_wAge.SetText(ageText);

		if (m_wUnavail)
		{
			int unavailable = 0;
			if (service)
				unavailable = service.CountUnavailableItems(m_Kit);
			if (unavailable > 0)
			{
				m_wUnavail.SetTextFormat("%1 UNAVAILABLE HERE", unavailable);
				m_wUnavail.SetVisible(true);
			}
			else
			{
				m_wUnavail.SetVisible(false);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void ShowResultChip(string label, bool failed)
	{
		if (!m_wLoadedChip)
			return;

		TextWidget chipText = TextWidget.Cast(m_wLoadedChip.FindAnyWidget("LoadedChipText"));
		if (chipText)
		{
			chipText.SetText(label);
			if (failed)
				chipText.SetColor(Color.FromSRGBA(214, 64, 52, 255));
			else
				chipText.SetColor(GRSA_Theme.Accent());
		}

		Widget chipBorder = m_wLoadedChip.FindAnyWidget("LoadedChipBorder");
		if (chipBorder)
		{
			if (failed)
				chipBorder.SetColor(Color.FromSRGBA(184, 38, 30, 255));
			else
				chipBorder.SetColor(GRSA_Theme.AccentDeep());
		}

		m_wLoadedChip.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	void HideResultChip()
	{
		if (m_wLoadedChip)
			m_wLoadedChip.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void StartRename()
	{
		if (m_bRenaming || !m_RenameEditComp || !m_wRenameEdit || !m_Kit)
			return;

		m_bRenaming = true;
		if (m_wName)
			m_wName.SetVisible(false);
		m_wRenameEdit.SetVisible(true);
		m_RenameEditComp.SetValue(m_Kit.m_sName);
		m_RenameEditComp.m_OnConfirm.Insert(OnRenameConfirm);
		m_RenameEditComp.m_OnWriteModeLeave.Insert(OnRenameWriteModeLeave);

		GetGame().GetWorkspace().SetFocusedWidget(m_wRenameEdit);
		m_RenameEditComp.ActivateWriteMode();
	}

	//------------------------------------------------------------------------------------------------
	void CommitRename()
	{
		if (!m_bRenaming)
			return;

		m_bRenaming = false;

		string oldName;
		if (m_Kit)
			oldName = m_Kit.m_sName;

		string newName;
		if (m_RenameEditComp)
		{
			newName = m_RenameEditComp.GetValue();
			m_RenameEditComp.m_OnConfirm.Remove(OnRenameConfirm);
			m_RenameEditComp.m_OnWriteModeLeave.Remove(OnRenameWriteModeLeave);
		}
		newName.TrimInPlace();

		if (m_wRenameEdit)
			m_wRenameEdit.SetVisible(false);
		if (m_wName)
			m_wName.SetVisible(true);

		if (newName.IsEmpty() || newName == oldName || !m_Kit)
			return;

		m_OnRenameCommitted.Invoke(this, newName);
	}

	//------------------------------------------------------------------------------------------------
	//! The committed name was accepted by the store; reflect it on the row.
	void ApplyName(string name)
	{
		if (m_Kit)
			m_Kit.m_sName = name;
		if (m_wName)
			m_wName.SetText(name);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRenameConfirm(SCR_EditBoxComponent comp, string value)
	{
		CommitRename();
	}

	//------------------------------------------------------------------------------------------------
	//! Console OSK "Done" closes write mode without firing m_OnConfirm; commit on leave instead.
	protected void OnRenameWriteModeLeave(string text)
	{
		CommitRename();
	}

	//------------------------------------------------------------------------------------------------
	protected string JoinMeta(string info, string piece)
	{
		if (info.IsEmpty())
			return piece;
		return info + "  " + piece;
	}

	//------------------------------------------------------------------------------------------------
	protected string FormatAge(int savedAtUnix)
	{
		if (savedAtUnix <= 0)
			return string.Empty;

		int age = System.GetUnixTime() - savedAtUnix;
		if (age < 90)
			return "NOW";
		if (age < 3600)
			return string.Format("%1M", age / 60);
		if (age < 86400)
			return string.Format("%1H", age / 3600);
		if (age < 604800)
			return string.Format("%1D", age / 86400);
		return string.Format("%1W", age / 604800);
	}
}
