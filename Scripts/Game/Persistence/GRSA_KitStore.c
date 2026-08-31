//! Kit persistence in ONE shared bank: kits are visible regardless of the faction you play,
//! availability is decided at apply time by the catalog gate instead of by file bucketing.
//! Legacy per-faction slot files copy into the shared bank once and remain available to the
//! original package.
class GRSA_KitStore
{
	protected static const string DIRECTORY = "$profile:GRSArmoryKits";
	protected static const string BANK = "shared";
	protected static const string LEGACY_DIR = "$profile:.save";
	protected static const string LEGACY_PREFIX = "GRSLockerKit_kits_";
	protected static const string LEGACY_SUFFIX = ".json";
	protected static const string LEGACY_SETTINGS_MODULE = "GRS_LockerSettings";
	protected static const int MAX_NAME_LENGTH = 48;
	protected static const int MAX_KIT_JSON_LENGTH = 32768;

	//! Console blob encoding: quotes/braces/newlines swapped for #-tokens before entering the
	//! settings container ('#' cannot occur in payloads — SanitizeName strips it from names,
	//! everything else is machine-generated). Prefix marks encoded blobs; raw legacy passes through.
	protected static const string BLOB_PREFIX = "GRSAB1:";
	//! Hex blob prefix is lowercase-only so a case-folding serializer cannot hide it; the
	//! reader lowercases before comparing and the hex decoder accepts both cases.
	protected static const string HEX_PREFIX = "grsab2x";

	protected static bool s_bMigrated;
	protected static bool s_bFlushPending;

	//------------------------------------------------------------------------------------------------
	//! Consoles store kits in the user settings container because mod profile file IO is not
	//! available there.
	static bool UseConsolePath()
	{
		return GetGame().IsPlatformGameConsole();
	}

	//------------------------------------------------------------------------------------------------
	//! Console persistence commits only when SaveUserSettings() runs at a lifecycle boundary, never
	//! mid-mission (base pattern, SCR_SettingsSubMenuBase.c:15-19). Kit/pref writers mark dirty here;
	//! the Armory menu open/close boundary does the durable flush.
	static void MarkConsoleDirty()
	{
		s_bFlushPending = true;
	}

	static void TouchConsoleModule()
	{
		if (!UseConsolePath())
			return;

		BaseContainer container;
		ConsoleAcquire(container);
	}

	static void FlushIfPending()
	{
		if (!s_bFlushPending || !UseConsolePath())
			return;

		//! No script mount API or success callback exists (Game.c:43-57): try the write regardless,
		//! and keep pending unless storage reports available so it retries at the next boundary.
		GetGame().SaveUserSettings();
		if (GetGame().IsSaveStorageAvailable())
			s_bFlushPending = false;

	}

	protected static int HexVal(string c)
	{
		int code = c.ToAscii();
		if (code >= 48 && code <= 57)
			return code - 48;
		if (code >= 97 && code <= 102)
			return code - 87;
		if (code >= 65 && code <= 70)
			return code - 55;
		return -1;
	}

	protected static string HexDecodeBlob(string body)
	{
		string s;
		for (int i = 0; i + 1 < body.Length(); i = i + 2)
		{
			int hi = HexVal(body[i]);
			int lo = HexVal(body[i + 1]);
			if (hi < 0 || lo < 0)
				return string.Empty;
			int code = hi * 16 + lo;
			s += code.AsciiToString();
		}
		return s;
	}

	//! Reads any stored generation: hex (case-fold-proof), round-1 token encoding, or raw legacy.
	protected static string ReadBlob(string stored)
	{
		if (stored.Length() >= HEX_PREFIX.Length())
		{
			string head = stored.Substring(0, HEX_PREFIX.Length());
			head.ToLower();
			if (head == HEX_PREFIX)
				return HexDecodeBlob(stored.Substring(HEX_PREFIX.Length(), stored.Length() - HEX_PREFIX.Length()));
		}

		return DecodeBlob(stored);
	}

	protected static string DecodeBlob(string stored)
	{
		if (stored.IndexOf(BLOB_PREFIX) != 0)
			return stored;

		string s = stored.Substring(BLOB_PREFIX.Length(), stored.Length() - BLOB_PREFIX.Length());
		s.Replace("#q#", "\"");
		s.Replace("#o#", "{");
		s.Replace("#c#", "}");
		s.Replace("#n#", "\n");
		return s;
	}

	//------------------------------------------------------------------------------------------------
	static bool SaveKit(notnull GRSA_KitFile kit, int slot)
	{
		if (slot < 0)
			return false;

		kit.m_sName = SanitizeName(kit.m_sName);
		string json = kit.ExportToString();
		if (json.IsEmpty() || json.Length() > MAX_KIT_JSON_LENGTH)
		{
			GRSA_Log.Warn(string.Format("KitStore: slot %1 export is empty or too large", slot));
			return false;
		}

		if (UseConsolePath())
			return ConsoleWrite(slot, json);

		FileIO.MakeDirectory(DIRECTORY);
		return WriteFileText(SharedPath(slot), json);
	}

	//------------------------------------------------------------------------------------------------
	static GRSA_KitFile LoadKit(int slot)
	{
		string json;
		if (UseConsolePath())
		{
			json = ConsoleRead(slot);
			if (json.IsEmpty())
				json = ReadFileText(SharedPath(slot));
		}
		else
			json = ReadFileText(SharedPath(slot));

		if (json.IsEmpty() || json.Length() > MAX_KIT_JSON_LENGTH)
			return null;

		GRSA_KitFile kit = new GRSA_KitFile();
		if (!kit.ImportFromString(json))
		{
			GRSA_Log.Warn(string.Format("Kit slot %1 failed to parse", slot));
			return null;
		}

		return kit;
	}

	//------------------------------------------------------------------------------------------------
	static bool DeleteKit(int slot)
	{
		if (UseConsolePath())
		{
			bool ok = ConsoleWrite(slot, string.Empty);
			string consoleFilePath = SharedPath(slot);
			if (FileIO.FileExists(consoleFilePath))
				FileIO.DeleteFile(consoleFilePath);
			return ok;
		}

		string path = SharedPath(slot);
		if (!FileIO.FileExists(path))
			return true;

		return FileIO.DeleteFile(path);
	}

	//------------------------------------------------------------------------------------------------
	static void GetKits(int slotCount, notnull out array<ref GRSA_KitFile> kits)
	{
		EnsureMigrated(slotCount);

		kits.Clear();
		for (int slot = 0; slot < slotCount; ++slot)
		{
			kits.Insert(LoadKit(slot));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Copies old per-faction slot files into the shared bank in filename order.
	protected static void EnsureMigrated(int slotCount)
	{
		if (s_bMigrated)
			return;
		s_bMigrated = true;

		if (UseConsolePath())
		{
			MigrateConsole(slotCount);
			return;
		}

		for (int slot = 0; slot < slotCount; ++slot)
		{
			if (FileIO.FileExists(SharedPath(slot)))
				return;
		}

		array<string> paths = {};
		FileIO.FindFiles(paths.Insert, DIRECTORY + "/", ".json");
		paths.Sort();

		int target = 0;
		int migrated = 0;
		foreach (string path : paths)
		{
			if (target >= slotCount)
				break;

			string fileName = FilePath.StripPath(path);
			if (fileName.IndexOf(BANK + "_slot") == 0)
				continue;

			string json = ReadFileText(path);
			if (json.IsEmpty())
				continue;

			GRSA_KitFile kit = new GRSA_KitFile();
			if (!kit.ImportFromString(json))
				continue;

			FileIO.MakeDirectory(DIRECTORY);
			if (!WriteFileText(SharedPath(target), json))
				continue;

			target++;
			migrated++;
		}

		if (migrated > 0)
			GRSA_Log.Info(string.Format("Migrated %1 kit(s) into the shared bank", migrated));
	}

	//------------------------------------------------------------------------------------------------
	protected static void MigrateConsole(int slotCount)
	{
		BaseContainer container;
		GRSA_ArmorySettings settings = ConsoleAcquire(container);
		if (!settings || settings.m_aKits.IsEmpty())
			return;

		foreach (GRSA_ArmorySavedKit row : settings.m_aKits)
		{
			if (row && row.m_sFactionKey == BANK)
				return;
		}

		array<ref GRSA_ArmorySavedKit> ordered = {};
		foreach (GRSA_ArmorySavedKit row : settings.m_aKits)
		{
			if (row && !row.m_sJson.IsEmpty())
				ordered.Insert(row);
		}

		foreach (int slot, GRSA_ArmorySavedKit row : ordered)
		{
			if (slot >= slotCount)
				break;

			GRSA_ArmorySavedKit shared = new GRSA_ArmorySavedKit();
			shared.m_sFactionKey = BANK;
			shared.m_iSlot = slot;
			shared.m_sJson = row.m_sJson;
			settings.m_aKits.Insert(shared);
		}

		BaseContainerTools.ReadFromInstance(settings, container);
		GetGame().UserSettingsChanged();
		s_bFlushPending = true;
	}

	//------------------------------------------------------------------------------------------------
	//! One-shot migration from the legacy save-mod files. Faction stamps stay as metadata only.
	static int ImportLegacyKits(string currentFactionKey, int slotCount)
	{
		EnsureMigrated(slotCount);

		array<ref GRSA_KitFile> existing = {};
		GetKits(slotCount, existing);

		int imported = 0;
		if (UseConsolePath())
		{
			BaseContainer container = GetGame().GetGameUserSettings().GetModule(LEGACY_SETTINGS_MODULE);
			if (!container)
				return 0;

			GRSA_ArmorySettings legacySettings = new GRSA_ArmorySettings();
			BaseContainerTools.WriteToInstance(legacySettings, container);
			if (!legacySettings.m_aKits)
				return 0;

			foreach (GRSA_ArmorySavedKit row : legacySettings.m_aKits)
			{
				if (!row || row.m_sJson.IsEmpty())
					continue;

				GRSA_KitFile kit = new GRSA_KitFile();
				if (!kit.ImportFromString(row.m_sJson))
					continue;

				if (ImportOneKit(kit, currentFactionKey, slotCount, existing))
					imported++;
			}
			return imported;
		}

		array<string> legacyPaths = {};
		FileIO.FindFiles(legacyPaths.Insert, LEGACY_DIR + "/", LEGACY_SUFFIX);

		foreach (string path : legacyPaths)
		{
			string fileName = FilePath.StripPath(path);
			if (fileName.IndexOf(LEGACY_PREFIX) != 0)
				continue;

			string json = ReadFileText(path);
			if (json.IsEmpty())
				continue;

			GRSA_KitFile kit = new GRSA_KitFile();
			if (!kit.ImportFromString(json))
				continue;

			if (ImportOneKit(kit, currentFactionKey, slotCount, existing))
				imported++;
		}

		return imported;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ImportOneKit(notnull GRSA_KitFile kit, string currentFactionKey, int slotCount, notnull array<ref GRSA_KitFile> existing)
	{
		if (kit.m_sFactionKey.IsEmpty())
			kit.m_sFactionKey = currentFactionKey;

		if (AlreadyImported(existing, kit))
			return false;

		int freeSlot = FindFreeSlot(existing, slotCount);
		if (freeSlot < 0)
			return false;

		if (!SaveKit(kit, freeSlot))
			return false;

		while (existing.Count() <= freeSlot)
		{
			existing.Insert(null);
		}
		existing[freeSlot] = kit;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool AlreadyImported(notnull array<ref GRSA_KitFile> existing, notnull GRSA_KitFile kit)
	{
		foreach (GRSA_KitFile slotKit : existing)
		{
			if (slotKit && slotKit.m_sName == kit.m_sName && slotKit.m_iSavedAtUnix == kit.m_iSavedAtUnix)
				return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static int FindFreeSlot(notnull array<ref GRSA_KitFile> existing, int slotCount)
	{
		for (int slot = 0; slot < slotCount; ++slot)
		{
			if (slot >= existing.Count() || !existing[slot])
				return slot;
		}
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	static string SanitizeName(string name)
	{
		string result = name;
		result.Replace("\\", " ");
		result.Replace("/", " ");
		result.Replace(":", " ");
		result.Replace("*", " ");
		result.Replace("?", " ");
		result.Replace("\"", " ");
		result.Replace("<", " ");
		result.Replace(">", " ");
		result.Replace("|", " ");
		result.Replace("\n", " ");
		result.Replace("\t", " ");
		result.Replace("#", " ");
		result.Replace("~", " ");
		result.TrimInPlace();
		if (result.Length() > MAX_NAME_LENGTH)
			result = result.Substring(0, MAX_NAME_LENGTH);
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected static string SharedPath(int slot)
	{
		return string.Format("%1/%2_slot%3.json", DIRECTORY, BANK, slot);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool WriteFileText(string path, string content)
	{
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
			return false;

		file.Write(content);
		file.Close();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ReadFileText(string path)
	{
		if (!FileIO.FileExists(path))
			return string.Empty;

		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
			return string.Empty;

		string raw;
		string line;
		while (file.ReadLine(line) > 0)
		{
			raw += line + "\n";
			if (raw.Length() > MAX_KIT_JSON_LENGTH)
			{
				file.Close();
				return string.Empty;
			}
		}
		file.Close();
		return raw;
	}

	//------------------------------------------------------------------------------------------------
	protected static GRSA_ArmorySettings ConsoleAcquire(out BaseContainer container)
	{
		container = GetGame().GetGameUserSettings().GetModule("GRSA_ArmorySettings");
		if (!container)
		{
			GRSA_Log.Warn("GRSA_ArmorySettings module not registered in user settings");
			return null;
		}

		GRSA_ArmorySettings settings = new GRSA_ArmorySettings();
		BaseContainerTools.WriteToInstance(settings, container);
		if (!settings.m_aKits)
			settings.m_aKits = {};
		return settings;
	}

	//------------------------------------------------------------------------------------------------
	//! Console kit rows stay compact so the shared user-settings container remains reliable.
	protected static bool ConsoleWrite(int slot, string json)
	{
		BaseContainer container;
		GRSA_ArmorySettings settings = ConsoleAcquire(container);
		if (!settings)
			return false;

		for (int i = settings.m_aKits.Count() - 1; i >= 0; --i)
		{
			GRSA_ArmorySavedKit row = settings.m_aKits[i];
			if (row && row.m_sFactionKey == BANK && row.m_iSlot == slot)
				settings.m_aKits.RemoveOrdered(i);
		}

		if (!json.IsEmpty())
		{
			GRSA_ArmorySavedKit row = new GRSA_ArmorySavedKit();
			row.m_sFactionKey = BANK;
			row.m_iSlot = slot;
			row.m_sJson = json;
			settings.m_aKits.Insert(row);
		}

		BaseContainerTools.ReadFromInstance(settings, container);
		GetGame().UserSettingsChanged();
		s_bFlushPending = true;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ConsoleRead(int slot)
	{
		BaseContainer container;
		GRSA_ArmorySettings settings = ConsoleAcquire(container);
		if (!settings)
			return string.Empty;

		foreach (GRSA_ArmorySavedKit row : settings.m_aKits)
		{
			if (row && row.m_sFactionKey == BANK && row.m_iSlot == slot)
				return ReadBlob(row.m_sJson);
		}

		return string.Empty;
	}

}
