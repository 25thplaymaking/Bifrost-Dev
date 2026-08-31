//! BaseContainerProps is load-bearing: without it the settings system cannot create
//! sub-containers for rows, and ReadFromInstance silently drops the whole array.
[BaseContainerProps()]
class GRSA_ArmorySavedKit
{
	[Attribute()]
	string m_sFactionKey;

	[Attribute()]
	int m_iSlot;

	[Attribute()]
	string m_sJson;
}

//! Only console builds register this data as a shared user-settings module. PC and dedicated
//! server builds keep it as a plain data holder and use profile files instead.
#ifdef PLATFORM_CONSOLE
class GRSA_ArmorySettings : ModuleGameSettings
#else
class GRSA_ArmorySettings
#endif
{
	[Attribute()]
	ref array<ref GRSA_ArmorySavedKit> m_aKits;

	[Attribute(defvalue: "1")]
	bool m_bApplyOnClose;

	[Attribute(defvalue: "100")]
	int m_iOrbitSensitivity;

	[Attribute(defvalue: "1")]
	bool m_bStudioLighting;

	[Attribute(defvalue: "125")]
	int m_iStudioBrightness;

	[Attribute(defvalue: "75")]
	int m_iStudioRearFill;

}
