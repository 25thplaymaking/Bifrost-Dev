// Base-settings mapping helpers.
class DCO_BaseSettingsUtil
{

	static EAISkill SkillFromScore(int score)
	{
		if (score <= 12)  return EAISkill.NONE;
		if (score <= 37)  return EAISkill.ROOKIE;
		if (score <= 62)  return EAISkill.REGULAR;
		if (score <= 87)  return EAISkill.VETERAN;
		return EAISkill.EXPERT;
	}

	static float PerceptionFromScore(int score)   { return 0.5 + (score / 100.0); }
	static float FireRateFromScore(int score)     { return 0.7 + (score / 100.0) * 0.5; }
	static float ReactionDelayFromScore(int score){ return 2.0 - (score / 100.0) * 1.8; }

	static EAISkill BaselineSkill()         { return SkillFromScore(DCO_BaseSettings.Get().m_iAiSkill); }
	static float    BaselineFireRateCoef()  { return FireRateFromScore(DCO_BaseSettings.Get().m_iFireRate); }

	// Apply a troop grade: write every lever to the grade's quarter value and set the hold-ground flag.
	static void ApplyGrade(DCO_BaseSettings cfg, DCO_EBaseGrade grade)
	{
		int score;
		switch (grade)
		{
			case DCO_EBaseGrade.CONSCRIPT: score = 25;  break;
			case DCO_EBaseGrade.TRAINED:   score = 50;  break;
			case DCO_EBaseGrade.ELITE:     score = 75;  break;
			case DCO_EBaseGrade.FANATIC:   score = 100; break;
			default: return;
		}
		cfg.m_iAiSkill            = score;
		cfg.m_iPerception         = score;
		cfg.m_iReactionTime       = score;
		cfg.m_iFireRate           = score;
		cfg.m_bFanaticHoldsGround = (grade == DCO_EBaseGrade.FANATIC);
	}
}
