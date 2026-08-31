class DCO_App6Roles
{
	protected static ref map<string, ResourceName> s_Map;

	static ResourceName Get(string key)
	{
		if (!s_Map)
			Build();
		ResourceName r;
		if (s_Map.Find(key, r))
			return r;
		return ResourceName.Empty;
	}

	protected static void Build()
	{
		s_Map = new map<string, ResourceName>();
		// Native editor glyphs keep every role aligned with the GM interface without bundled duplicate art.
		s_Map.Set("RIFLE", "{AE53796BC5D21A08}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_Rifleman.edds");
		s_Map.Set("MG", "{EF1F445746A3391A}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_MachineGunner.edds");
		s_Map.Set("AT", "{00B30C29FAF85E1C}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_AntiTank.edds");
		s_Map.Set("GREN", "{DDAEEF112BEBCF94}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_Grenadier.edds");
		s_Map.Set("MED", "{F3FCC3B9732551D9}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_Medic.edds");
		s_Map.Set("RADIO", "{B9F0BD39FF1881A3}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_RadioOperator.edds");
		s_Map.Set("LEAD", "{A26C465A6AE2AA17}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_Leader.edds");
		s_Map.Set("SNIP", "{0A78405E73C36477}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_Sharpshooter.edds");
		s_Map.Set("ENG", "{203F5DF31C1A45FB}UI/Textures/Editor/ContentBrowser/ContentBrowser_Trait_Repairing.edds");
		s_Map.Set("AMMO", "{FB48FAD32DA8BC91}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_AmmoBearer.edds");
		s_Map.Set("SCOUT", "{9A61AD7EADB131FD}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Character_Spotter.edds");
	}
}
