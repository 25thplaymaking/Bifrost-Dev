class DCO_ContactUtil
{
	static const float FRESH_CONTACT_S = 15.0;

	static bool IsLive(SCR_AITargetInfo info, float pmNow, float freshSec)
	{
		if (!info || !info.m_Entity)
			return false;
		if (info.m_eCategory < EAITargetInfoCategory.DETECTED)
			return false;
		return (pmNow - info.m_fTimestamp) <= freshSec;
	}

	// Any live target in the group's memory.
	static bool HasLiveContact(SCR_AIGroupPerception per, float freshSec)
	{
		vector unused;
		return GetLiveThreatNear(per, vector.Zero, 0, freshSec, unused);
	}

	static bool GetLiveThreatNear(SCR_AIGroupPerception per, vector nearPos, float maxRange, float freshSec, out vector outPos)
	{
		if (!per || !per.m_aTargets)
			return false;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return false;
		float now = pm.GetTime();

		bool found = false;
		float bestSq = maxRange * maxRange;
		if (maxRange <= 0)
			bestSq = 1000000000000.0;

		foreach (SCR_AITargetInfo info : per.m_aTargets)
		{
			if (!IsLive(info, now, freshSec))
				continue;
			float d = vector.DistanceSq(info.m_vWorldPos, nearPos);
			if (d > bestSq)
				continue;
			bestSq = d;
			outPos = info.m_vWorldPos;
			found = true;
		}
		return found;
	}
}
