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
		s_Map.Set("F_RIFLE", "{40027F7DDA6CE549}img/icons/app6/role_F_RIFLE.edds");
		s_Map.Set("F_MG", "{C16F65F08BA7015C}img/icons/app6/role_F_MG.edds");
		s_Map.Set("F_AT", "{E9D2FCF6D946A8DB}img/icons/app6/role_F_AT.edds");
		s_Map.Set("F_GREN", "{727C68B3BBA8B9E2}img/icons/app6/role_F_GREN.edds");
		s_Map.Set("F_MED", "{CDC0FE090430FC3B}img/icons/app6/role_F_MED.edds");
		s_Map.Set("F_RADIO", "{2859FB4418CEEBD1}img/icons/app6/role_F_RADIO.edds");
		s_Map.Set("F_LEAD", "{6E7D5E88F32FF0EE}img/icons/app6/role_F_LEAD.edds");
		s_Map.Set("F_SNIP", "{B9AF49D28DF8FD39}img/icons/app6/role_F_SNIP.edds");
		s_Map.Set("F_ENG", "{DE43768FABB837DF}img/icons/app6/role_F_ENG.edds");
		s_Map.Set("F_AMMO", "{BC944C5643C9AE1C}img/icons/app6/role_F_AMMO.edds");
		s_Map.Set("F_SCOUT", "{7B5CEC3FBA7F3670}img/icons/app6/role_F_SCOUT.edds");
		s_Map.Set("H_RIFLE", "{13461ED2BB81F211}img/icons/app6/role_H_RIFLE.edds");
		s_Map.Set("H_MG", "{F2FDF35FAADECB58}img/icons/app6/role_H_MG.edds");
		s_Map.Set("H_AT", "{DA406A59F83F62DF}img/icons/app6/role_H_AT.edds");
		s_Map.Set("H_GREN", "{288493A7A3FC88E5}img/icons/app6/role_H_GREN.edds");
		s_Map.Set("H_MED", "{3200FA1436E62DBF}img/icons/app6/role_H_MED.edds");
		s_Map.Set("H_RADIO", "{7B1D9AEB7923FC89}img/icons/app6/role_H_RADIO.edds");
		s_Map.Set("H_LEAD", "{3485A59CEB7BC1E9}img/icons/app6/role_H_LEAD.edds");
		s_Map.Set("H_SNIP", "{E357B2C695ACCC3E}img/icons/app6/role_H_SNIP.edds");
		s_Map.Set("H_ENG", "{21837292996EE65B}img/icons/app6/role_H_ENG.edds");
		s_Map.Set("H_AMMO", "{E66CB7425B9D9F1B}img/icons/app6/role_H_AMMO.edds");
		s_Map.Set("H_SCOUT", "{28188D90DB922128}img/icons/app6/role_H_SCOUT.edds");
		s_Map.Set("N_RIFLE", "{AE0245901382E40E}img/icons/app6/role_N_RIFLE.edds");
		s_Map.Set("N_MG", "{EEC17BCFD130D4EC}img/icons/app6/role_N_MG.edds");
		s_Map.Set("N_AT", "{C67CE2C983D17D6B}img/icons/app6/role_N_AT.edds");
		s_Map.Set("N_GREN", "{7CCCDCE04E2044BB}img/icons/app6/role_N_GREN.edds");
		s_Map.Set("N_MED", "{B3E0B85F5ADF2931}img/icons/app6/role_N_MED.edds");
		s_Map.Set("N_RADIO", "{C659C1A9D120EA96}img/icons/app6/role_N_RADIO.edds");
		s_Map.Set("N_LEAD", "{60CDEADB06A70DB7}img/icons/app6/role_N_LEAD.edds");
		s_Map.Set("N_SNIP", "{B71FFD8178700060}img/icons/app6/role_N_SNIP.edds");
		s_Map.Set("N_ENG", "{A06330D9F557E2D5}img/icons/app6/role_N_ENG.edds");
		s_Map.Set("N_AMMO", "{B224F805B6415345}img/icons/app6/role_N_AMMO.edds");
		s_Map.Set("N_SCOUT", "{955CD6D273913737}img/icons/app6/role_N_SCOUT.edds");
		s_Map.Set("U_RIFLE", "{2128B61960AE4112}img/icons/app6/role_U_RIFLE.edds");
		s_Map.Set("U_MG", "{902D1DC6037FA5E6}img/icons/app6/role_U_MG.edds");
		s_Map.Set("U_AT", "{B89084C0519E0C61}img/icons/app6/role_U_AT.edds");
		s_Map.Set("U_GREN", "{453927B7A657257F}img/icons/app6/role_U_GREN.edds");
		s_Map.Set("U_MED", "{717053800FF3D468}img/icons/app6/role_U_MED.edds");
		s_Map.Set("U_RADIO", "{49733220A20C4F8A}img/icons/app6/role_U_RADIO.edds");
		s_Map.Set("U_LEAD", "{5938118CEED06C73}img/icons/app6/role_U_LEAD.edds");
		s_Map.Set("U_SNIP", "{8EEA06D6900761A4}img/icons/app6/role_U_SNIP.edds");
		s_Map.Set("U_ENG", "{62F3DB06A07B1F8C}img/icons/app6/role_U_ENG.edds");
		s_Map.Set("U_AMMO", "{8BD103525E363281}img/icons/app6/role_U_AMMO.edds");
		s_Map.Set("U_SCOUT", "{1A76255B00BD922B}img/icons/app6/role_U_SCOUT.edds");
	}
}
