class DCO_AIAnimationFxTool
{
	protected static ref DCO_AIAnimationFxTool s_Instance;
	protected static const int ACTION_ANIMATION_BASE = 2000;
	protected static const int ACTION_TOGGLE_THREAT = 2100;
	protected static const int ACTION_PAGE_PREV = 2101;
	protected static const int ACTION_PAGE_NEXT = 2102;
	protected static const int PAGE_SIZE = 15;

	protected DCO_GMContextMenu m_Menu;
	protected SCR_EditableCharacterComponent m_Target;
	protected RplId m_TargetId;
	protected bool m_bTargeting;
	protected bool m_bLeaveWhenThreatened = true;
	protected int m_iPage;
	protected ref ScriptInvoker m_MenuCallback = new ScriptInvoker();

	static DCO_AIAnimationFxTool Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_AIAnimationFxTool();
		return s_Instance;
	}

	void Init(DCO_GMContextMenu menu)
	{
		Shutdown();
		m_Menu = menu;
		m_MenuCallback.Insert(OnMenuAction);
	}

	void Shutdown()
	{
		Cancel();
		m_MenuCallback.Remove(OnMenuAction);
		m_Menu = null;
		m_Target = null;
		m_TargetId = RplId.Invalid();
	}

	void BeginTargeting()
	{
		m_Target = null;
		m_TargetId = RplId.Invalid();
		m_bLeaveWhenThreatened = true;
		m_iPage = 0;
		m_bTargeting = true;
		if (m_Menu)
			m_Menu.Hide();
		DCO_GMUIController.RefreshAnimationFxIndicator();
		Print("[DCO-ANIMATION] Animations FX armed: select one AI unit", LogLevel.NORMAL);
	}

	bool IsTargeting()
	{
		return m_bTargeting;
	}

	bool SelectFromFocused(SCR_BaseEditableEntityFilter focusedFilter)
	{
		if (!m_bTargeting)
			return false;
		if (!focusedFilter || DCO_GMUIController.IsNativePropertiesOpen())
		{
			Cancel();
			return true;
		}

		set<SCR_EditableEntityComponent> focused = new set<SCR_EditableEntityComponent>();
		focusedFilter.GetEntities(focused);
		if (focused.Count() != 1)
		{
			OnAuthorityResult(false, "Select one AI-controlled unit.");
			return true;
		}
		SCR_EditableCharacterComponent target = SCR_EditableCharacterComponent.Cast(focused[0]);
		if (!target || !target.GetOwner() || DCO_PlayerUtil.IsPlayer(target.GetOwner()))
		{
			OnAuthorityResult(false, "Animations FX only accepts an AI-controlled unit.");
			return true;
		}
		RplComponent rpl = RplComponent.Cast(target.GetOwner().FindComponent(RplComponent));
		if (!rpl || !rpl.Id().IsValid())
		{
			OnAuthorityResult(false, "The selected unit is not replicated.");
			return true;
		}

		m_Target = target;
		m_TargetId = rpl.Id();
		m_bTargeting = false;
		DCO_GMUIController.RefreshAnimationFxIndicator();
		ShowMenu();
		return true;
	}

	bool Cancel()
	{
		bool wasTargeting = m_bTargeting;
		m_bTargeting = false;
		if (wasTargeting)
			DCO_GMUIController.RefreshAnimationFxIndicator();
		return wasTargeting;
	}

	protected void ShowMenu()
	{
		if (!m_Menu || !m_Target || !m_TargetId.IsValid())
			return;
		array<string> labels = {};
		array<int> ids = {};
		int catalogCount = DCO_AIAnimationService.CatalogCount();
		int pageCount = Math.Max(1, (catalogCount + PAGE_SIZE - 1) / PAGE_SIZE);
		m_iPage = Math.Clamp(m_iPage, 0, pageCount - 1);
		if (m_iPage > 0)
		{
			labels.Insert("< Previous animations");
			ids.Insert(ACTION_PAGE_PREV);
		}
		int first = m_iPage * PAGE_SIZE;
		int last = Math.Min(first + PAGE_SIZE, catalogCount);
		for (int i = first; i < last; i++)
		{
			int animation = DCO_AIAnimationService.CatalogIdAt(i);
			labels.Insert(DCO_AIAnimationService.CatalogLabelAt(i));
			ids.Insert(ACTION_ANIMATION_BASE + animation);
		}
		if (m_iPage < pageCount - 1)
		{
			labels.Insert("Next animations >");
			ids.Insert(ACTION_PAGE_NEXT);
		}
		string threatLabel = "[ ] Leave when threatened";
		if (m_bLeaveWhenThreatened)
			threatLabel = "[X] Leave when threatened";
		labels.Insert(threatLabel);
		ids.Insert(ACTION_TOGGLE_THREAT);
		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		m_Menu.ShowTitledDetailed(labels, ids, mouseX, mouseY, "ANIMATIONS FX", string.Format("SELECT A VANILLA AI POSE  ·  PAGE %1/%2", m_iPage + 1, pageCount), -1, m_MenuCallback, m_Target);
	}

	protected void OnMenuAction(int actionId, SCR_EditableEntityComponent editable)
	{
		if (actionId == ACTION_PAGE_PREV)
		{
			m_iPage--;
			ShowMenu();
			return;
		}
		if (actionId == ACTION_PAGE_NEXT)
		{
			m_iPage++;
			ShowMenu();
			return;
		}
		if (actionId == ACTION_TOGGLE_THREAT)
		{
			m_bLeaveWhenThreatened = !m_bLeaveWhenThreatened;
			ShowMenu();
			return;
		}
		int animation = actionId - ACTION_ANIMATION_BASE;
		if (!DCO_AIAnimationService.IsValid(animation) || !m_TargetId.IsValid())
			return;

		if (Replication.IsServer())
		{
			SCR_PlayerController localController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (!localController || !DCO_GMRights.Allow(localController.GetPlayerId(), "animation FX"))
			{
				OnAuthorityResult(false, "Animation FX refused: Game Master rights required.");
				return;
			}
			string result;
			bool success = DCO_AIAnimationServer.Apply(m_TargetId, animation, m_bLeaveWhenThreatened, result);
			OnAuthorityResult(success, result);
			return;
		}

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.DCO_SendAnimationFx(m_TargetId, animation, m_bLeaveWhenThreatened);
		else
			OnAuthorityResult(false, "Animation FX failed: no local player controller.");
	}

	void OnAuthorityResult(bool success, string result)
	{
		LogLevel level = LogLevel.WARNING;
		if (success)
			level = LogLevel.NORMAL;
		Print("[DCO-ANIMATION] " + result, level);
	}
}
