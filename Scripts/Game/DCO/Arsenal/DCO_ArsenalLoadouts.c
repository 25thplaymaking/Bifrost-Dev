
class DCO_ArsenalLoadoutRec
{
	string m_sName;
	string m_sJson;
	ref array<string> m_aPrefabs = {};
}

class DCO_ArsenalLoadoutStore
{
	int m_iVersion = 1;
	ref array<ref DCO_ArsenalLoadoutRec> m_aLoadouts = {};
}

class DCO_ArsenalLoadouts
{
	protected static const string STORE_FILE = "$profile:BifrostArsenal_loadouts.json";

	protected static ref DCO_ArsenalLoadouts s_Inst;
	static DCO_ArsenalLoadouts Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_ArsenalLoadouts();
		return s_Inst;
	}

	protected ref DCO_ArsenalLoadoutStore m_Store;
	// Pending save context while a remote-GM snapshot round-trip is in flight.
	protected string m_sPendingName;
	protected ref array<string> m_aPendingManifest;
	protected int m_iPendingSeq;
	protected ref ScriptInvoker m_OnChanged = new ScriptInvoker();	// panel refresh hook.

	ScriptInvoker GetOnChanged()
	{
		return m_OnChanged;
	}

	protected void EnsureLoaded()
	{
		if (m_Store)
			return;
		m_Store = new DCO_ArsenalLoadoutStore();
		JsonLoadContext ctx = new JsonLoadContext();
		if (ctx.LoadFromFile(STORE_FILE))
		{
			if (!ctx.ReadValue("", m_Store) || !m_Store)
			{
				Print("[DCO-ARS] loadout store unreadable - starting empty (old file left on disk)", LogLevel.WARNING);
				m_Store = new DCO_ArsenalLoadoutStore();
			}
		}
		if (!m_Store.m_aLoadouts)
			m_Store.m_aLoadouts = {};
	}

	protected void Persist()
	{
		if (!m_Store)
			return;
		JsonSaveContext ctx = new JsonSaveContext();
		if (!ctx.WriteValue("", m_Store) || !ctx.SaveToFile(STORE_FILE))
			Print("[DCO-ARS] loadout store SAVE FAILED", LogLevel.WARNING);
		m_OnChanged.Invoke();
	}

	int GetLoadouts(notnull array<DCO_ArsenalLoadoutRec> outRecs)
	{
		EnsureLoaded();
		outRecs.Clear();
		foreach (DCO_ArsenalLoadoutRec r : m_Store.m_aLoadouts)
			outRecs.Insert(r);
		return outRecs.Count();
	}

	// Availability on THIS server: every manifest prefab must resolve in the local arsenal catalog.
	bool IsAvailable(notnull DCO_ArsenalLoadoutRec rec)
	{
		DCO_ArsenalCatalog cat = DCO_ArsenalCatalog.Get();
		cat.Build();
		foreach (string p : rec.m_aPrefabs)
		{
			if (!cat.FindByPrefab(p))
				return false;
		}
		return true;
	}

	// SAVE the character's current kit under a name.
	void SaveFrom(IEntity character, string name, notnull array<string> manifest)
	{
		if (!character || name.IsEmpty() || !GRSA_ArsenalScenarioSettings.Get().m_bAllowKitChanges)
			return;
		EnsureLoaded();

		if (Replication.IsServer())
		{
			string json = DCO_ArsenalServer.SnapshotJson(character);
			if (json.IsEmpty())
			{
				Print("[DCO-ARS] loadout save: snapshot failed", LogLevel.WARNING);
				return;
			}
			StoreRecord(name, json, manifest);
			return;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		RplComponent rpl = RplComponent.Cast(character.FindComponent(RplComponent));
		if (!pc || !rpl || !rpl.Id().IsValid())
			return;
		m_iPendingSeq++;
		m_sPendingName = name;
		m_aPendingManifest = {};
		foreach (string p : manifest)
			m_aPendingManifest.Insert(p);
		pc.DCO_SendGMArsenalSnapshot(rpl.Id(), m_iPendingSeq);
	}

	void OnSnapshotReply(int seq, string json)
	{
		if (seq != m_iPendingSeq)
			return;
		if (m_sPendingName.IsEmpty() || json.IsEmpty())
			return;
		array<string> manifest = m_aPendingManifest;
		if (!manifest)
			manifest = {};
		StoreRecord(m_sPendingName, json, manifest);
		m_sPendingName = "";
		m_aPendingManifest = null;
	}

	protected void StoreRecord(string name, string json, notnull array<string> manifest)
	{
		EnsureLoaded();
		DCO_ArsenalLoadoutRec rec;
		foreach (DCO_ArsenalLoadoutRec r : m_Store.m_aLoadouts)
		{
			if (r.m_sName == name)
			{
				rec = r;
				break;
			}
		}
		if (!rec)
		{
			rec = new DCO_ArsenalLoadoutRec();
			rec.m_sName = name;
			m_Store.m_aLoadouts.Insert(rec);
		}
		rec.m_sJson = json;
		rec.m_aPrefabs.Clear();
		foreach (string p : manifest)
		{
			if (!p.IsEmpty() && !rec.m_aPrefabs.Contains(p))
				rec.m_aPrefabs.Insert(p);
		}
		Persist();
	}

	// APPLY a stored loadout onto a character.
	void ApplyTo(IEntity character, notnull DCO_ArsenalLoadoutRec rec)
	{
		if (!character || rec.m_sJson.IsEmpty() || !GRSA_ArsenalScenarioSettings.Get().m_bAllowKitChanges)
			return;
		if (!IsAvailable(rec))
		{
			Print(string.Format("[DCO-ARS] loadout '%1' is UNAVAILABLE here (missing content) - not applied", rec.m_sName), LogLevel.WARNING);
			return;
		}

		if (Replication.IsServer())
		{
			DCO_ArsenalServer.ApplyLoadoutJson(character, rec.m_sJson);
			return;
		}
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		RplComponent rpl = RplComponent.Cast(character.FindComponent(RplComponent));
		if (!pc || !rpl || !rpl.Id().IsValid())
			return;
		pc.DCO_SendGMArsenalApply(rpl.Id(), rec.m_sJson);
	}

	void Delete(notnull DCO_ArsenalLoadoutRec rec)
	{
		if (!GRSA_ArsenalScenarioSettings.Get().m_bAllowKitChanges)
			return;
		EnsureLoaded();
		int idx = m_Store.m_aLoadouts.Find(rec);
		if (idx < 0)
			return;
		m_Store.m_aLoadouts.Remove(idx);
		Persist();
	}
}
