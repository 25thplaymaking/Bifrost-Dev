[BaseContainerProps(configRoot: true)]
class GRSA_ArmoryConfig
{
	[Attribute(defvalue: "0", desc: "Charge supplies when enabled by authored configuration or Scenario Settings", category: "Economy")]
	bool m_bUseSupplies;

	[Attribute(defvalue: "0", desc: "Enforce rank locks when enabled by authored configuration or Scenario Settings", category: "Economy")]
	bool m_bUseRankLocks;

	[Attribute(defvalue: "1", desc: "Refund supplies for items removed by an applied kit", category: "Economy")]
	bool m_bRefundRemovedItems;

	[Attribute(defvalue: "0", desc: "Use only the station arsenal's active inventory; the default combines every loaded faction catalog", category: "Economy")]
	bool m_bRestrictToArsenalConfig;

	[Attribute(defvalue: "1", desc: "Closing the Armory with unapplied changes applies them automatically", category: "Kits")]
	bool m_bApplyOnClose;

	[Attribute(defvalue: "25", uiwidget: UIWidgets.Slider, desc: "Saved kit slots in the shared client bank", params: "1 50 1", category: "Kits")]
	int m_iKitSlotCount;

	[Attribute(defvalue: "2", uiwidget: UIWidgets.Slider, desc: "Magazines granted per equipped weapon on apply when the kit lists none", params: "0 10 1", category: "Kits")]
	int m_iDefaultMagazineCount;

	[Attribute(desc: "Item categories shown in the Armory", category: "Categories")]
	ref array<ref GRSA_ArmoryCategory> m_aCategories;

	//------------------------------------------------------------------------------------------------
	static GRSA_ArmoryConfig Load(ResourceName path)
	{
		if (path.IsEmpty())
			return null;

		Resource holder = BaseContainerTools.LoadContainer(path);
		if (!holder || !holder.IsValid())
			return null;

		return GRSA_ArmoryConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(holder.GetResource().ToBaseContainer()));
	}

	//------------------------------------------------------------------------------------------------
	//! Code defaults used when no .conf is assigned, attribute defvalues only apply to container instances.
	static GRSA_ArmoryConfig CreateDefault()
	{
		GRSA_ArmoryConfig config = new GRSA_ArmoryConfig();
		config.m_bUseSupplies = false;
		config.m_bUseRankLocks = false;
		config.m_bRefundRemovedItems = true;
		config.m_bRestrictToArsenalConfig = false;
		config.m_bApplyOnClose = true;
		config.m_iKitSlotCount = 25;
		config.m_iDefaultMagazineCount = 2;
		config.m_aCategories = {};

		config.AddCategory("Primary", SCR_EArsenalItemType.RIFLE | SCR_EArsenalItemType.SNIPER_RIFLE | SCR_EArsenalItemType.MACHINE_GUN | SCR_EArsenalItemType.ROCKET_LAUNCHER, SCR_EArsenalItemMode.WEAPON | SCR_EArsenalItemMode.WEAPON_VARIANTS, GRSA_EArmoryTab.WEAPONS, "rifle", 0);
		config.AddCategory("Secondary", SCR_EArsenalItemType.RIFLE | SCR_EArsenalItemType.SNIPER_RIFLE | SCR_EArsenalItemType.MACHINE_GUN | SCR_EArsenalItemType.ROCKET_LAUNCHER, SCR_EArsenalItemMode.WEAPON | SCR_EArsenalItemMode.WEAPON_VARIANTS, GRSA_EArmoryTab.WEAPONS, "launcher", 1);
		config.AddCategory("Sidearm", SCR_EArsenalItemType.PISTOL, SCR_EArsenalItemMode.WEAPON | SCR_EArsenalItemMode.WEAPON_VARIANTS, GRSA_EArmoryTab.WEAPONS, "pistol", 2);
		config.AddCategory("Throwables", SCR_EArsenalItemType.LETHAL_THROWABLE, SCR_EArsenalItemMode.WEAPON | SCR_EArsenalItemMode.WEAPON_VARIANTS | SCR_EArsenalItemMode.DEFAULT | SCR_EArsenalItemMode.CONSUMABLE, GRSA_EArmoryTab.WEAPONS, "throwable", 3);
		config.AddCategory("Smoke & Signals", SCR_EArsenalItemType.NON_LETHAL_THROWABLE, SCR_EArsenalItemMode.WEAPON | SCR_EArsenalItemMode.WEAPON_VARIANTS | SCR_EArsenalItemMode.DEFAULT | SCR_EArsenalItemMode.CONSUMABLE, GRSA_EArmoryTab.WEAPONS, "smoke", 4);
		config.AddCategory("Optics & Attachments", SCR_EArsenalItemType.WEAPON_ATTACHMENT, SCR_EArsenalItemMode.ATTACHMENT, GRSA_EArmoryTab.WEAPONS, "attachment");
		config.AddCategory("Ammunition", SCR_EArsenalItemType.RIFLE | SCR_EArsenalItemType.SNIPER_RIFLE | SCR_EArsenalItemType.MACHINE_GUN | SCR_EArsenalItemType.PISTOL | SCR_EArsenalItemType.ROCKET_LAUNCHER, SCR_EArsenalItemMode.AMMUNITION, GRSA_EArmoryTab.WEAPONS, "ammo");

		config.AddCategory("Equipment", SCR_EArsenalItemType.EQUIPMENT, SCR_EArsenalItemMode.DEFAULT | SCR_EArsenalItemMode.CONSUMABLE, GRSA_EArmoryTab.GEAR, "equipment");
		config.AddCategory("Medical", SCR_EArsenalItemType.HEAL, SCR_EArsenalItemMode.DEFAULT | SCR_EArsenalItemMode.CONSUMABLE, GRSA_EArmoryTab.GEAR, "medical");
		config.AddCategory("Throwables", SCR_EArsenalItemType.LETHAL_THROWABLE | SCR_EArsenalItemType.NON_LETHAL_THROWABLE, SCR_EArsenalItemMode.DEFAULT | SCR_EArsenalItemMode.CONSUMABLE, GRSA_EArmoryTab.GEAR, "throwable");
		config.AddCategory("Explosives", SCR_EArsenalItemType.EXPLOSIVES | SCR_EArsenalItemType.MORTARS, SCR_EArsenalItemMode.DEFAULT | SCR_EArsenalItemMode.CONSUMABLE, GRSA_EArmoryTab.GEAR, "explosive");

		SCR_EArsenalItemType clothingTypes = SCR_EArsenalItemType.HEADWEAR | SCR_EArsenalItemType.TORSO | SCR_EArsenalItemType.LEGS | SCR_EArsenalItemType.FOOTWEAR | SCR_EArsenalItemType.VEST_AND_WAIST | SCR_EArsenalItemType.HANDWEAR | SCR_EArsenalItemType.BACKPACK | SCR_EArsenalItemType.RADIO_BACKPACK | SCR_EArsenalItemType.EQUIPMENT;
		config.AddCategory("Headgear", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "headwear", -1, "LoadoutHeadCoverArea");
		config.AddCategory("Shirts & Jackets", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "torso", -1, "LoadoutJacketArea");
		config.AddCategory("Pants", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "legs", -1, "LoadoutPantsArea");
		config.AddCategory("Footwear", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "footwear", -1, "LoadoutBootsArea");
		config.AddCategory("Goggles", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "goggles", -1, "LoadoutGooglesArea");
		config.AddCategory("Vests", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "vest", -1, "LoadoutVestArea");
		config.AddCategory("Armored Vests", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "vest", -1, "LoadoutArmoredVestSlotArea");
		config.AddCategory("Handwear", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "handwear", -1, "LoadoutHandwearSlotArea");
		config.AddCategory("Watches", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "watch", -1, "LoadoutWatchArea");
		config.AddCategory("Binoculars", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "binoculars", -1, "LoadoutBinocularsArea");
		config.AddCategory("Backpacks", clothingTypes, SCR_EArsenalItemMode.DEFAULT, GRSA_EArmoryTab.OUTFIT, "backpack", -1, "LoadoutBackpackArea");

		return config;
	}

	//------------------------------------------------------------------------------------------------
	protected void AddCategory(string displayName, SCR_EArsenalItemType types, SCR_EArsenalItemMode modes, GRSA_EArmoryTab tab, string icon, int weaponSlot = -1, string clothArea = "")
	{
		GRSA_ArmoryCategory category = new GRSA_ArmoryCategory();
		category.m_sDisplayName = displayName;
		category.m_eItemTypes = types;
		category.m_eItemModes = modes;
		category.m_eTab = tab;
		category.m_sIconName = icon;
		category.m_iWeaponSlot = weaponSlot;
		category.m_sClothArea = clothArea;
		m_aCategories.Insert(category);
	}

	//------------------------------------------------------------------------------------------------
	int GetCategoriesForTab(GRSA_EArmoryTab tab, notnull out array<GRSA_ArmoryCategory> categories)
	{
		categories.Clear();
		if (!m_aCategories)
			return 0;

		foreach (GRSA_ArmoryCategory category : m_aCategories)
		{
			if (category && category.m_eTab == tab)
				categories.Insert(category);
		}

		return categories.Count();
	}
}

class GRSA_ConfigHolder
{
	protected static ref GRSA_ArmoryConfig s_Default;

	static GRSA_ArmoryConfig GetDefault()
	{
		if (!s_Default)
			s_Default = GRSA_ArmoryConfig.CreateDefault();
		return s_Default;
	}
}

[BaseContainerProps()]
class GRSA_ArmoryCategory
{
	[Attribute(desc: "Category label shown on the rail")]
	string m_sDisplayName;

	[Attribute("", desc: "Arsenal item types in this category", uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType))]
	SCR_EArsenalItemType m_eItemTypes;

	[Attribute("", desc: "Arsenal item modes in this category, empty means DEFAULT and WEAPON", uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemMode))]
	SCR_EArsenalItemMode m_eItemModes;

	[Attribute("0", desc: "Armory tab hosting this category", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(GRSA_EArmoryTab))]
	GRSA_EArmoryTab m_eTab;

	[Attribute(defvalue: "-1", uiwidget: UIWidgets.Slider, desc: "Weapon slot this category equips into, -1 picks automatically", params: "-1 4 1")]
	int m_iWeaponSlot;

	[Attribute(desc: "Icon sprite name inside the Armory imageset")]
	string m_sIconName;

	[Attribute(desc: "Loadout area class name this category binds to, e.g. LoadoutJacketArea. Clothing is binned by the area it actually equips to, not by its catalog type flags")]
	string m_sClothArea;
}
