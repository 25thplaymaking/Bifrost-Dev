class DCO_ATSettings
{
	protected static ref DCO_ATSettings s_Instance;

	// Master switch for launcher discipline.
	bool	m_bEnableLauncherDiscipline	= true;

	bool	m_bLauncherStowWhenDry		= true;

	float	m_fLauncherInfantryRange	= 250.0;

	bool	m_bLauncherVsBuildings		= true;

	float	m_fLauncherBuildingScan		= 12.0;

	float	m_fLauncherVsInfantryChance	= 0.30;

	float	m_fLauncherDisciplineCheckSec	= 1.0;

	static DCO_ATSettings Get()
	{
		if (!s_Instance)
		{
			s_Instance = new DCO_ATSettings();
			DCO_JsonConfig.LoadATInto(s_Instance);	// server JSON = startup baseline.
		}
		return s_Instance;
	}
}
