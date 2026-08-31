// Routes GM pause and clock changes through authority.
class DCO_GMPauseServer
{
	static void RoutePause(int scope, int aspectMask, bool on)
	{
		if (Replication.IsServer())
		{
			ApplyPause(scope, aspectMask, on, RplId.Invalid());
			return;
		}
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
		{
			Print("[DCO-GM] pause relay: no local player controller (cannot reach the server)", LogLevel.WARNING);
			return;
		}
		if (scope == EDCO_PauseScope.SELECTED && on)
		{
			int sent = 0;
			set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
			SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
			foreach (SCR_EditableEntityComponent editable : selected)
			{
				if (!editable || !editable.GetOwner())
					continue;
				RplComponent rpl = RplComponent.Cast(editable.GetOwner().FindComponent(RplComponent));
				if (rpl && rpl.Id().IsValid())
				{
					pc.DCO_SendGMPause(scope, aspectMask, true, rpl.Id());
					sent++;
				}
			}
			if (sent == 0)
				pc.DCO_SendGMPause(scope, aspectMask, true, RplId.Invalid());
			return;
		}
		pc.DCO_SendGMPause(scope, aspectMask, on, RplId.Invalid());
	}

	static int ApplyPause(int scope, int aspectMask, bool on, RplId selectedTargetId)
	{
		if (!Replication.IsServer())
			return 0;
		if (scope == EDCO_PauseScope.SELECTED && on)
		{
			if (selectedTargetId.IsValid())
			{
				RplComponent rpl = RplComponent.Cast(Replication.FindItem(selectedTargetId));
				if (rpl)
					DCO_GMPauseCore.Get().ApplySelected(rpl.GetEntity(), aspectMask);
			}
		}
		else
		{
			DCO_GMPauseCore.Get().Apply(scope, aspectMask, on);
		}
		PushPauseState(DCO_GMPauseCore.Get().IsActive());
		return DCO_GMPauseCore.Get().GetFrozenCount();
	}

	// Replicate the state on each controller.
	protected static void PushPauseState(bool paused)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;
		array<int> playerIds = {};
		players.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			SCR_PlayerController controller = SCR_PlayerController.Cast(players.GetPlayerController(playerId));
			if (controller)
				controller.DCO_SetPauseState(paused);
		}
		if (!Replication.IsRunning())
			DCO_GMPausePresentationState.SetPaused(paused);
	}

	static void RouteClock(float mult)
	{
		if (Replication.IsServer())
		{
			DCO_GMClock.SetMultiplier(mult);	// listen/SP: proven direct path.
			return;
		}
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return;
		pc.DCO_SendGMClock(mult);
	}

	static void ApplyClock(float mult)
	{
		if (!Replication.IsServer())
			return;
		DCO_GMClock.SetMultiplier(mult);
	}
}

class DCO_GMPausePresentationState
{
	protected static bool s_bPaused;
	protected static bool s_bPending;
	protected static bool s_bHasFrozenCount;
	protected static int s_iFrozenCount;
	static void SetPaused(bool paused)
	{
		s_bPaused = paused;
		s_bPending = false;
		s_bHasFrozenCount = false;
	}
	static void BeginRequest() { s_bPending = true; }
	static void SetConfirmed(bool paused, int frozenCount)
	{
		s_bPaused = paused;
		s_iFrozenCount = frozenCount;
		s_bHasFrozenCount = true;
		s_bPending = false;
	}
	static bool IsPaused() { return s_bPaused; }
	static bool IsPending() { return s_bPending; }
	static int GetFrozenCount()
	{
		if (!s_bHasFrozenCount)
			return -1;
		return s_iFrozenCount;
	}
}
