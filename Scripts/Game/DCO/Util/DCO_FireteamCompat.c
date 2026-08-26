// Fireteam accessor compatibility shim.
class DCO_FireteamCompat
{
	static void GetAllFireteams(SCR_AIGroupFireteamManager mgr, SCR_AIGroup group, out array<ref SCR_AIGroupFireteam> outFireteams)
	{
		if (!mgr || !group)
			return;

		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			SCR_AIGroupFireteam ft = mgr.FindFireteam(a);
			if (ft && outFireteams.Find(ft) < 0)
				outFireteams.Insert(ft);
		}
	}
}
