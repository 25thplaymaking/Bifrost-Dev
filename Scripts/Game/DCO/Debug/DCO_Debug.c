// Unified debug logger.
class DCO_Debug
{
	static bool Enabled()
	{
		DCO_MoraleSettings cfg = DCO_MoraleSettings.Get();
		return cfg && cfg.m_bDebug;
	}

	static void Log(string category, string message)
	{
		if (!Enabled())
			return;
		Print("[DCO:" + category + "] " + message, LogLevel.NORMAL);
	}

	// Log a line tagged with the group's leader ID, so lines are traceable per group.
	static void LogGroup(string category, IEntity groupLeader, string message)
	{
		if (!Enabled())
			return;
		string who = "?";
		if (groupLeader)
			who = groupLeader.GetID().ToString();
		Print("[DCO:" + category + "] (grp " + who + ") " + message, LogLevel.NORMAL);
	}
}
