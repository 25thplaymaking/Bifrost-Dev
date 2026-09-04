modded class SCR_EditorRespawnBriefingComponent
{
	protected static const ResourceName DCO_GM_JOURNAL = "{BF050000A36E1092}Configs/Journal/BifrostGMJournal.conf";

	override bool LoadJournalConfig()
	{
		if (m_sJournalConfigPath != DCO_GM_JOURNAL)
		{
			m_sJournalConfigPath = DCO_GM_JOURNAL;
			m_JournalConfig = null;
		}
		return super.LoadJournalConfig();
	}
}

class DCO_GMBriefing
{
	static const int ENTRY_SITUATION = 100;
	static const int ENTRY_MISSION = 101;
	static const int ENTRY_EXECUTION = 102;
	static const int ENTRY_SIGNAL = 103;
	static const int ENTRY_INTEL = 104;

	static SCR_EditorRespawnBriefingComponent GetComponent()
	{
		IEntity gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
		return SCR_EditorRespawnBriefingComponent.Cast(gameMode.FindComponent(SCR_EditorRespawnBriefingComponent));
	}

	static string Read(int entryId)
	{
		SCR_EditorRespawnBriefingComponent briefing = GetComponent();
		if (!briefing)
			return string.Empty;
		SCR_JournalSetupConfig setup = briefing.GetJournalSetup();
		if (!setup)
			return string.Empty;
		SCR_JournalConfig journal = setup.GetJournalConfig(FactionKey.Empty);
		if (!journal)
			return string.Empty;
		foreach (SCR_JournalEntry entry : journal.GetEntries())
		{
			if (entry && entry.GetEntryID() == entryId)
				return entry.GetEntryText();
		}
		return string.Empty;
	}

	static bool Apply(int playerId, int entryId, string text)
	{
		if (!Replication.IsServer() || !DCO_GMRights.Allow(playerId, "GM mission briefing"))
			return false;
		if (entryId < ENTRY_SITUATION || entryId > ENTRY_INTEL || text.Length() > 4096)
			return false;
		SCR_EditorRespawnBriefingComponent briefing = GetComponent();
		if (!briefing)
			return false;
		array<string> parameters = {};
		briefing.RewriteEntry_SA(FactionKey.Empty, entryId, text, parameters);
		Print(string.Format("[DCO-GM] mission briefing entry %1 updated by GM %2", entryId, playerId), LogLevel.NORMAL);
		return true;
	}
}
