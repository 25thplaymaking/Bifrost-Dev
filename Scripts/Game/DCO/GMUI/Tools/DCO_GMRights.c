// Server-side authorization for every GM client->server request.
class DCO_GMRights
{
	static bool IsGameMaster(int playerId)
	{
		if (playerId <= 0)
			return false;

		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity manager = core.GetEditorManager(playerId);
		if (!manager)
			return false;

		return manager.IsOpened() && !manager.IsLimited();
	}

	static bool IsLocalGameMaster()
	{
		if (System.IsConsoleApp())
			return false;

		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity manager = core.GetEditorManager();
		if (!manager)
			return false;

		return manager.IsOpened() && !manager.IsLimited();
	}

	// Gate + one warning line.
	static bool Allow(int playerId, string request)
	{
		if (IsGameMaster(playerId))
			return true;
		Print(string.Format("[DCO-GM] REFUSED %1 from player %2 - not an open Game Master", request, playerId),
			LogLevel.WARNING);
		return false;
	}
}
