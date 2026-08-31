enum GRSA_EArmoryTab
{
	WEAPONS = 0,
	GEAR = 1,
	OUTFIT = 2,
	KITS = 3,
	SETTINGS = 4
}

//! Focus regions a mannequin stage can reframe onto, mapped from browse categories and slot cards.
enum GRSA_EStageFocus
{
	FULL = 0,
	HEAD = 1,
	TORSO = 2,
	LEGS = 3,
	FEET = 4
}

enum GRSA_EApplyStatus
{
	SUCCESS = 0,
	PARTIAL = 1,
	FAILED_NO_CHARACTER = 2,
	FAILED_FACTION = 3,
	FAILED_RANK = 4,
	FAILED_SUPPLIES = 5,
	FAILED_INVALID = 6,
	FAILED_SERVER = 7
}

class GRSA_ApplyReport
{
	GRSA_EApplyStatus m_eStatus;
	int m_iApplied;
	int m_iSkipped;
	float m_fSuppliesCharged;
	ref array<string> m_aSkippedNames = {};

	//------------------------------------------------------------------------------------------------
	string GetSkippedSample(int maxNames = 3)
	{
		string result;
		int count = Math.Min(maxNames, m_aSkippedNames.Count());
		for (int i = 0; i < count; ++i)
		{
			if (i > 0)
				result += ", ";
			result += m_aSkippedNames[i];
		}
		return result;
	}
}
