modded class SCR_AICombatMoveState
{
	protected float m_fDCO_LastStanceChangeMs;
	protected int m_iDCO_LastStance = -1;

	override void ApplyNewRequest(notnull SCR_AICombatMoveRequestBase request)
	{
		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();

		if (cfg && cfg.m_bStanceCooldownDebug && Replication.IsServer())
			DCO_LogStanceRequest(request);

		BaseWorld world = GetGame().GetWorld();
		if (world && Replication.IsServer() && cfg && cfg.m_bEnableStanceCooldown && !IsMoving() && DCO_IsThrottledStanceRequest(request))
		{
			float now = world.GetWorldTime();	// ms.
			float cdMs = cfg.m_fStanceCooldownSec * 1000.0;

			SCR_AICombatMoveRequest_ChangeStance cs = SCR_AICombatMoveRequest_ChangeStance.Cast(request);
			if (cs)
			{
				int target = cs.m_eStance;

				if (target == m_iDCO_LastStance)
				{
					super.ApplyNewRequest(request);
					return;
				}

				// A genuine change.
				float required = cdMs;
				if (m_iDCO_LastStance >= 0)
					required = cdMs * 2.0;

				if (now - m_fDCO_LastStanceChangeMs < required)
					return;	// within the settle window: drop this flip.

				m_fDCO_LastStanceChangeMs = now;
				m_iDCO_LastStance = target;
				super.ApplyNewRequest(request);
				return;
			}

			if (now - m_fDCO_LastStanceChangeMs < cdMs)
				return;
			m_fDCO_LastStanceChangeMs = now;
		}

		super.ApplyNewRequest(request);
	}

	// True for the stance flips this feature throttles.
	protected bool DCO_IsThrottledStanceRequest(SCR_AICombatMoveRequestBase request)
	{
		if (SCR_AICombatMoveRequest_ChangeStance.Cast(request) != null)
			return true;

		if (DCO_MoraleSettings.Get().m_bStanceCooldownInCover && SCR_AICombatMoveRequest_ChangeStanceInCover.Cast(request) != null)
			return true;

		return false;
	}

	// Diagnostic: print the kind of stance request arriving + whether it would be throttled.
	protected void DCO_LogStanceRequest(SCR_AICombatMoveRequestBase request)
	{
		string kind = "Other";
		string stance = "-";

		SCR_AICombatMoveRequest_ChangeStance cs = SCR_AICombatMoveRequest_ChangeStance.Cast(request);
		if (cs)
		{
			kind = "ChangeStance";
			stance = cs.m_eStance.ToString();
		}
		else if (SCR_AICombatMoveRequest_ChangeStanceInCover.Cast(request))
		{
			kind = "ChangeStanceInCover";
		}

		Print(string.Format("[DCO Stance] kind=%1 stance=%2 reason=%3 moving=%4 throttledType=%5",
			kind, stance, request.m_eReason, IsMoving(), DCO_IsThrottledStanceRequest(request)), LogLevel.NORMAL);
	}
}
