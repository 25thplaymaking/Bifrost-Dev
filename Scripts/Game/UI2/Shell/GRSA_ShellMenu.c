//! Intercepts a tab switch before SCR_TabViewComponent hides the active Gunsmith content.
class GRSA_ArsenalTabViewComponent : SCR_TabViewComponent
{
	protected GRSA_ShellMenu m_TransitionGuard;

	void SetTransitionGuard(GRSA_ShellMenu guard)
	{
		m_TransitionGuard = guard;
	}

	override bool OnUpdate(Widget w)
	{
		if (!m_TransitionGuard)
			return true;

		return super.OnUpdate(w);
	}

	override void ShowTab(int i, bool callAction = true, bool playSound = true)
	{
		if (m_TransitionGuard && i != GetShownTab()
			&& !m_TransitionGuard.RequestTabChange(i, callAction, playSound))
			return;

		super.ShowTab(i, callAction, playSound);
	}

	void ShowConfirmedTab(int i, bool callAction, bool playSound)
	{
		super.ShowTab(i, callAction, playSound);
	}

	void ReassertCurrentButton()
	{
		array<ref SCR_TabViewContent> contents = GetContents();
		int shown = GetShownTab();
		foreach (int i, SCR_TabViewContent content : contents)
		{
			if (content && content.m_ButtonComponent)
				content.m_ButtonComponent.SetToggled(i == shown, false, false);
		}
	}
}

//! Editor v2 shell: thin dispatcher over the native super-menu machinery. Owns only the header
//! readouts, back handling, and the session lifecycle; every screen is its own SCR_SubMenuBase
//! and all state lives in GRSA_DraftService.
class GRSA_ShellMenu : SCR_SuperMenuBase
{
	protected static bool s_bOpen;
	protected static GRSA_ShellMenu s_ActiveInstance;
	protected TextWidget m_wHeaderSupply;
	protected TextWidget m_wHeaderWeight;
	protected TextWidget m_wStatus;
	protected SCR_ButtonTextComponent m_ExitButton;
	protected Widget m_wChrome;
	protected Widget m_wLeavePrompt;
	protected SCR_ButtonTextComponent m_LeaveYesButton;
	protected SCR_ButtonTextComponent m_LeaveNoButton;
	protected GRSA_ArsenalTabViewComponent m_TabView;
	protected int m_iPendingTab = -1;
	protected bool m_bPendingCallAction;
	protected bool m_bPendingPlaySound;

	//! The menu instance owns the studio hub: a static ref owning it would survive into game
	//! teardown on a script reload with the menu open and trip the engine resource-leak assert.
	protected ref GRSA_StageHub m_Hub;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		s_bOpen = true;
		//! Installed before super so the tab view's OnTabCreate lookups find the hub.
		m_Hub = new GRSA_StageHub();
		GRSA_StageHub.Install(m_Hub);

		super.OnMenuOpen();

		GRSA_KitStore.TouchConsoleModule();
		GRSA_KitStore.FlushIfPending();

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
		{
			GRSA_Log.Warn("Shell opened without a draft session, closing");
			Close();
			return;
		}
		s_ActiveInstance = this;

		Widget root = GetRootWidget();
		GRSA_Theme.Apply(root);
		m_wHeaderSupply = TextWidget.Cast(root.FindAnyWidget("HeaderSupply"));
		m_wHeaderWeight = TextWidget.Cast(root.FindAnyWidget("HeaderWeight"));
		m_wStatus = TextWidget.Cast(root.FindAnyWidget("ShellStatus"));
		m_wChrome = root.FindAnyWidget("Chrome");
		m_wLeavePrompt = root.FindAnyWidget("GunsmithLeavePrompt");
		m_ExitButton = SCR_ButtonTextComponent.GetButtonText("ExitButton", root);
		if (m_ExitButton)
			m_ExitButton.m_OnClicked.Insert(OnExitPressed);

		if (m_SuperMenuComponent)
			m_TabView = GRSA_ArsenalTabViewComponent.Cast(m_SuperMenuComponent.GetTabView());
		if (m_TabView)
			m_TabView.SetTransitionGuard(this);
		else
			GRSA_Log.Error("Shell: guarded Arsenal tab view is missing");

		m_LeaveYesButton = SCR_ButtonTextComponent.GetButtonText("GunsmithLeaveYes", root);
		m_LeaveNoButton = SCR_ButtonTextComponent.GetButtonText("GunsmithLeaveNo", root);
		if (!m_wLeavePrompt || !m_LeaveYesButton || !m_LeaveNoButton)
			GRSA_Log.Error("Shell: Gunsmith leave prompt is incomplete");
		if (m_LeaveYesButton)
		{
			m_LeaveYesButton.SetText("YES");
			m_LeaveYesButton.m_OnClicked.Insert(OnLeaveYes);
		}
		if (m_LeaveNoButton)
		{
			m_LeaveNoButton.SetText("NO");
			m_LeaveNoButton.m_OnClicked.Insert(OnLeaveNo);
		}
		HideLeavePrompt();

		service.m_OnDraftChanged.Insert(RefreshHeader);
		SCR_ResourcePlayerControllerInventoryComponent.GRSA_GetOnApplyResult().Insert(OnApplyResult);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		#ifdef WORKBENCH
		GetGame().GetInputManager().AddActionListener("MenuBackWB", EActionTrigger.DOWN, OnBack);
		#endif
		RefreshHeader();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		//! The polled preview-orbit actions live in this context; without a per-frame activation
		//! they read zero inside menus.
		GetGame().GetInputManager().ActivateContext("GRSA_ArmoryContext");
		//! Advance queued wheel travel once for every vertical list and the Gunsmith candidate strip.
		GRSA_SmoothScrollComponent.TickAll(tDelta);
		GRSA_CarouselComponent.TickAll(tDelta);

		//! The shared studio is shell-owned: one camera write per frame regardless of which tab
		//! is open, so station glides keep moving across tab changes.
		GRSA_StageHub.Tick(tDelta);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service && service.m_bDraftDirty && service.m_Config && service.m_Config.m_bApplyOnClose
			&& GRSA_ClientPrefs.Get().m_bApplyOnClose)
			service.RequestApplyDraft();

		if (m_ExitButton)
		{
			m_ExitButton.m_OnClicked.Remove(OnExitPressed);
			m_ExitButton = null;
		}
		if (m_LeaveYesButton)
		{
			m_LeaveYesButton.m_OnClicked.Remove(OnLeaveYes);
			m_LeaveYesButton = null;
		}
		if (m_LeaveNoButton)
		{
			m_LeaveNoButton.m_OnClicked.Remove(OnLeaveNo);
			m_LeaveNoButton = null;
		}
		if (m_TabView)
			m_TabView.SetTransitionGuard(null);
		m_TabView = null;
		m_wLeavePrompt = null;
		m_wChrome = null;

		super.OnMenuClose();
		s_bOpen = false;
		if (s_ActiveInstance == this)
			s_ActiveInstance = null;

		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		#ifdef WORKBENCH
		GetGame().GetInputManager().RemoveActionListener("MenuBackWB", EActionTrigger.DOWN, OnBack);
		#endif
		SCR_ResourcePlayerControllerInventoryComponent.GRSA_GetOnApplyResult().Remove(OnApplyResult);
		GetGame().GetCallqueue().Remove(ClearStatus);

		if (service)
			service.m_OnDraftChanged.Remove(RefreshHeader);

		//! Hosts unsubscribe from the draft service inside Shutdown — it must run before the
		//! service singleton is cleared.
		GRSA_StageHub.Shutdown();
		m_Hub = null;
		GRSA_DraftService.Clear();
		GRSA_Theme.EndSession();
		GRSA_KitStore.FlushIfPending();
	}

	//------------------------------------------------------------------------------------------------
	static bool IsArmoryOpen()
	{
		return s_bOpen;
	}

	//------------------------------------------------------------------------------------------------
	static bool OpenGunsmithForClothing(int clothingSlot)
	{
		if (!s_ActiveInstance || !s_ActiveInstance.m_TabView || clothingSlot < 0)
			return false;

		GRSA_GunsmithScreen.QueueClothingInspection(clothingSlot);
		s_ActiveInstance.m_TabView.ShowTab(1);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Short transient feedback shared by local Arsenal interactions.
	static void ShowStatus(string text, bool success)
	{
		if (!s_ActiveInstance)
			return;

		Color color = GRSA_Theme.TextPrimary();
		if (!success)
			color = GRSA_Theme.Separator();
		s_ActiveInstance.PresentStatus(text, color, 2200);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBack()
	{
		if (m_iPendingTab >= 0)
		{
			CancelPendingTabChange();
			return;
		}

		if (GRSA_SoldierScreen.ConsumeBack())
			return;

		if (GRSA_GunsmithScreen.ConsumeBack())
			return;

		if (GRSA_KitsScreen.ConsumeBack())
			return;

		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnExitPressed(SCR_ButtonBaseComponent button)
	{
		Close();
	}

	//------------------------------------------------------------------------------------------------
	bool RequestTabChange(int targetTab, bool callAction, bool playSound)
	{
		if (m_iPendingTab >= 0)
			return false;
		if (!GRSA_GunsmithScreen.HasActiveAttachmentEdit())
			return true;
		if (!m_wLeavePrompt || !m_LeaveYesButton || !m_LeaveNoButton)
		{
			GRSA_Log.Error("Shell: leave prompt unavailable; preserving the attachment draft and switching tabs");
			GRSA_GunsmithScreen.SaveAndCloseAttachmentEdit();
			return true;
		}

		m_iPendingTab = targetTab;
		m_bPendingCallAction = callAction;
		m_bPendingPlaySound = playSound;
		if (m_wLeavePrompt)
			m_wLeavePrompt.SetVisible(true);
		if (m_wChrome)
			m_wChrome.SetEnabled(false);
		if (m_TabView)
			m_TabView.ReassertCurrentButton();
		if (m_LeaveNoButton)
			GetGame().GetWorkspace().SetFocusedWidget(m_LeaveNoButton.GetRootWidget());
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLeaveYes(SCR_ButtonBaseComponent button)
	{
		if (button)
			button.SetToggled(false, true, false);
		if (m_iPendingTab < 0)
			return;

		int target = m_iPendingTab;
		bool callAction = m_bPendingCallAction;
		bool playSound = m_bPendingPlaySound;
		GRSA_GunsmithScreen.SaveAndCloseAttachmentEdit();
		HideLeavePrompt();
		if (m_TabView)
			m_TabView.ShowConfirmedTab(target, callAction, playSound);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLeaveNo(SCR_ButtonBaseComponent button)
	{
		if (button)
			button.SetToggled(false, true, false);
		CancelPendingTabChange();
	}

	//------------------------------------------------------------------------------------------------
	protected void CancelPendingTabChange()
	{
		HideLeavePrompt();
		if (m_TabView)
			m_TabView.ReassertCurrentButton();
		GRSA_GunsmithScreen.RestoreAttachmentEditFocus();
	}

	//------------------------------------------------------------------------------------------------
	protected void HideLeavePrompt()
	{
		m_iPendingTab = -1;
		m_bPendingCallAction = false;
		m_bPendingPlaySound = false;
		if (m_wLeavePrompt)
			m_wLeavePrompt.SetVisible(false);
		if (m_wChrome)
			m_wChrome.SetEnabled(true);
	}

	//------------------------------------------------------------------------------------------------
	//! One shared WEAR verdict presenter for every screen: the header status line states what the
	//! server actually did, in the same words everywhere.
	protected void OnApplyResult(GRSA_EApplyStatus status, int applied, int skipped, float suppliesCharged, string skippedSample)
	{
		if (!m_wStatus)
			return;

		string text;
		Color statusColor = GRSA_Theme.TextPrimary();
		switch (status)
		{
			case GRSA_EApplyStatus.SUCCESS:
			{
				text = "WORN";
				break;
			}
			case GRSA_EApplyStatus.PARTIAL:
			{
				text = string.Format("WORN: %1 ITEMS UNAVAILABLE", skipped);
				statusColor = GRSA_Theme.Separator();
				break;
			}
			case GRSA_EApplyStatus.FAILED_RANK:
			{
				text = "HIGHER RANK REQUIRED";
				statusColor = GRSA_Theme.Separator();
				break;
			}
			case GRSA_EApplyStatus.FAILED_SUPPLIES:
			{
				text = "NOT ENOUGH SUPPLIES";
				statusColor = GRSA_Theme.Separator();
				break;
			}
			default:
			{
				text = "WEAR FAILED";
				statusColor = GRSA_Theme.Separator();
				break;
			}
		}
		if ((status == GRSA_EApplyStatus.SUCCESS || status == GRSA_EApplyStatus.PARTIAL) && GRSA_DraftService.Get())
			GRSA_DraftService.Get().m_bDraftDirty = false;

		PresentStatus(text, statusColor, 4000);
	}

	//------------------------------------------------------------------------------------------------
	protected void PresentStatus(string text, Color color, int durationMs)
	{
		if (!m_wStatus)
			return;

		m_wStatus.SetText(text);
		m_wStatus.SetColor(color);
		GetGame().GetCallqueue().Remove(ClearStatus);
		GetGame().GetCallqueue().CallLater(ClearStatus, durationMs, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearStatus()
	{
		if (m_wStatus)
			m_wStatus.SetText(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshHeader()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.m_Draft)
			return;

		if (m_wHeaderSupply)
		{
			if (service.UsesSupplies())
			{
				int cost = Math.Round(service.m_Draft.m_fSuppliesCost);
				m_wHeaderSupply.SetTextFormat("SUPPLIES  %1", cost);
			}
			else
			{
				m_wHeaderSupply.SetText(string.Empty);
			}
		}

		if (m_wHeaderWeight)
			m_wHeaderWeight.SetTextFormat("%1 KG", service.GetDraftWeight().ToString(-1, 1));
	}
}
