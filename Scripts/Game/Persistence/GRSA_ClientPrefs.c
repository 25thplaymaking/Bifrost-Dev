//! Per-player UI preferences, client side only: PC persists to a $profile JSON, consoles ride
//! the GRSA_ArmorySettings user-settings module alongside the kit bank.
class GRSA_ClientPrefs
{
	protected static const string DIRECTORY = "$profile:GRSArmory";
	protected static const string FILE_PATH = "$profile:GRSArmory/prefs.json";
	protected static const string SETTINGS_MODULE = "GRSA_ArmorySettings";

	static const int SENSITIVITY_MIN = 50;
	static const int SENSITIVITY_MAX = 200;
	static const int LIGHT_MIN = 50;
	static const int LIGHT_MAX = 150;
	static const int LIGHT_STEP = 25;
	static const int FILL_MIN = 25;
	static const int FILL_MAX = 125;
	static const int FILL_STEP = 25;

	bool m_bApplyOnClose;
	int m_iOrbitSensitivity;
	bool m_bStudioLighting;
	int m_iStudioBrightness;
	int m_iStudioRearFill;

	protected static ref GRSA_ClientPrefs s_Instance;

	//------------------------------------------------------------------------------------------------
	void GRSA_ClientPrefs()
	{
		SetDefaults();
	}

	//------------------------------------------------------------------------------------------------
	void SetDefaults()
	{
		m_bApplyOnClose = true;
		m_iOrbitSensitivity = 100;
		m_bStudioLighting = true;
		m_iStudioBrightness = 125;
		m_iStudioRearFill = 75;
	}

	//------------------------------------------------------------------------------------------------
	float GetOrbitScale()
	{
		return Math.Clamp(m_iOrbitSensitivity, SENSITIVITY_MIN, SENSITIVITY_MAX) * 0.01;
	}

	//------------------------------------------------------------------------------------------------
	static GRSA_ClientPrefs Get()
	{
		if (!s_Instance)
		{
			s_Instance = new GRSA_ClientPrefs();
			s_Instance.LoadStored();
		}
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	static void Save()
	{
		GRSA_ClientPrefs prefs = Get();
		if (GRSA_KitStore.UseConsolePath())
		{
			prefs.WriteConsole();
			return;
		}

		JsonSaveContext ctx = new JsonSaveContext();
		ctx.WriteValue("applyOnClose", prefs.m_bApplyOnClose);
		ctx.WriteValue("orbitSensitivity", prefs.m_iOrbitSensitivity);
		ctx.WriteValue("studioLighting", prefs.m_bStudioLighting);
		ctx.WriteValue("studioBrightness", prefs.m_iStudioBrightness);
		ctx.WriteValue("studioRearFill", prefs.m_iStudioRearFill);
		string json = ctx.SaveToString();
		if (json.IsEmpty())
			return;

		FileIO.MakeDirectory(DIRECTORY);
		FileHandle file = FileIO.OpenFile(FILE_PATH, FileMode.WRITE);
		if (!file)
			return;

		file.Write(json);
		file.Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadStored()
	{
		if (GRSA_KitStore.UseConsolePath())
		{
			ReadConsole();
			return;
		}

		if (!FileIO.FileExists(FILE_PATH))
			return;

		FileHandle file = FileIO.OpenFile(FILE_PATH, FileMode.READ);
		if (!file)
			return;

		string json;
		string line;
		while (file.ReadLine(line) > 0)
		{
			json += line + "\n";
		}
		file.Close();

		if (json.IsEmpty())
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(json))
			return;

		ctx.ReadValue("applyOnClose", m_bApplyOnClose);
		ctx.ReadValue("orbitSensitivity", m_iOrbitSensitivity);
		ctx.ReadValue("studioLighting", m_bStudioLighting);
		ctx.ReadValue("studioBrightness", m_iStudioBrightness);
		ctx.ReadValue("studioRearFill", m_iStudioRearFill);
		ClampStored();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClampStored()
	{
		m_iOrbitSensitivity = Math.Clamp(m_iOrbitSensitivity, SENSITIVITY_MIN, SENSITIVITY_MAX);
		m_iStudioBrightness = Math.Clamp(m_iStudioBrightness, LIGHT_MIN, LIGHT_MAX);
		m_iStudioRearFill = Math.Clamp(m_iStudioRearFill, FILL_MIN, FILL_MAX);
	}

	//------------------------------------------------------------------------------------------------
	protected void WriteConsole()
	{
		BaseContainer container = GetGame().GetGameUserSettings().GetModule(SETTINGS_MODULE);
		if (!container)
			return;

		GRSA_ArmorySettings settings = new GRSA_ArmorySettings();
		BaseContainerTools.WriteToInstance(settings, container);
		settings.m_bApplyOnClose = m_bApplyOnClose;
		settings.m_iOrbitSensitivity = m_iOrbitSensitivity;
		settings.m_bStudioLighting = m_bStudioLighting;
		settings.m_iStudioBrightness = m_iStudioBrightness;
		settings.m_iStudioRearFill = m_iStudioRearFill;
		BaseContainerTools.ReadFromInstance(settings, container);
		GetGame().UserSettingsChanged();
		GRSA_KitStore.MarkConsoleDirty();
	}

	//------------------------------------------------------------------------------------------------
	protected void ReadConsole()
	{
		BaseContainer container = GetGame().GetGameUserSettings().GetModule(SETTINGS_MODULE);
		if (!container)
			return;

		GRSA_ArmorySettings settings = new GRSA_ArmorySettings();
		BaseContainerTools.WriteToInstance(settings, container);
		m_bApplyOnClose = settings.m_bApplyOnClose;
		m_iOrbitSensitivity = settings.m_iOrbitSensitivity;
		m_bStudioLighting = settings.m_bStudioLighting;
		m_iStudioBrightness = settings.m_iStudioBrightness;
		m_iStudioRearFill = settings.m_iStudioRearFill;
		ClampStored();
	}
}
