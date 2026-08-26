enum DCO_EBaseGrade
{
	NONE,	// manual - leave sliders untouched.
	CONSCRIPT,	// 25.
	TRAINED,	// 50.
	ELITE,	// 75.
	FANATIC,	// 100.
}

enum DCO_EBaseStance
{
	NONE,	// do not force.
	STAND,
	CROUCH,
	PRONE,
}

class DCO_BaseSettings
{
	protected static ref DCO_BaseSettings s_Instance;

	bool m_bEnableBaseSettings = false;
	bool m_bDebugBaseSettings  = false;

	int  m_iAiSkill        = 50;
	int  m_iPerception     = 50;
	int  m_iReactionTime   = 50;
	int  m_iFireRate       = 50;

	DCO_EBaseGrade  m_eGlobalGrade           = DCO_EBaseGrade.NONE;
	DCO_EBaseStance m_eGlobalStance          = DCO_EBaseStance.NONE;
	int             m_eDefaultSpawnFormation = 0;

	bool  m_bFanaticHoldsGround = false;
	float m_fApplyIntervalSec   = 5.0;

	static DCO_BaseSettings Get()
	{
		if (!s_Instance)
		{
			s_Instance = new DCO_BaseSettings();
			DCO_JsonConfig.LoadBaseInto(s_Instance);	// server JSON = startup baseline.
		}
		return s_Instance;
	}
}
