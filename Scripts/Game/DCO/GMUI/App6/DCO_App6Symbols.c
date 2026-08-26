class DCO_App6Symbols
{
	protected static ref map<string, ResourceName> s_Map;

// Resolves a symbol key to its resource while returning an empty resource for unknown combinations.
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
		AddFriendlySymbols();
		AddHostileSymbols();
		AddNeutralSymbols();
		AddUnknownSymbols();
		AddIndependentSymbols();
	}

	protected static void AddFriendlySymbols()
	{
		s_Map.Set("F_INF_X", "{102428C15D74ADFA}img/icons/app6/app6_F_INF_X.edds");
		s_Map.Set("F_INF_TM", "{B32B0FD975865E8C}img/icons/app6/app6_F_INF_TM.edds");
		s_Map.Set("F_INF_SQ", "{043D8A498E021DB5}img/icons/app6/app6_F_INF_SQ.edds");
		s_Map.Set("F_INF_SE", "{4FCD476676660E44}img/icons/app6/app6_F_INF_SE.edds");
		s_Map.Set("F_INF_PL", "{9F00280A573C617D}img/icons/app6/app6_F_INF_PL.edds");
		s_Map.Set("F_AT_X", "{B50782C3661BA14B}img/icons/app6/app6_F_AT_X.edds");
		s_Map.Set("F_AT_TM", "{A1C29E73E1D34AD3}img/icons/app6/app6_F_AT_TM.edds");
		s_Map.Set("F_AT_SQ", "{16D41BE31A5709EA}img/icons/app6/app6_F_AT_SQ.edds");
		s_Map.Set("F_AT_SE", "{5D24D6CCE2331A1B}img/icons/app6/app6_F_AT_SE.edds");
		s_Map.Set("F_AT_PL", "{8DE9B9A0C3697522}img/icons/app6/app6_F_AT_PL.edds");
		s_Map.Set("F_ARM_X", "{0E8ED1D74FFBF095}img/icons/app6/app6_F_ARM_X.edds");
		s_Map.Set("F_ARM_TM", "{215B082C2B29EC24}img/icons/app6/app6_F_ARM_TM.edds");
		s_Map.Set("F_ARM_SQ", "{964D8DBCD0ADAF1D}img/icons/app6/app6_F_ARM_SQ.edds");
		s_Map.Set("F_ARM_SE", "{DDBD409328C9BCEC}img/icons/app6/app6_F_ARM_SE.edds");
		s_Map.Set("F_ARM_PL", "{0D702FFF0993D3D5}img/icons/app6/app6_F_ARM_PL.edds");
		s_Map.Set("F_REC_X", "{668DBFBF2634E8F2}img/icons/app6/app6_F_REC_X.edds");
		s_Map.Set("F_REC_TM", "{A62C5F25331B786B}img/icons/app6/app6_F_REC_TM.edds");
		s_Map.Set("F_REC_SQ", "{113ADAB5C89F3B52}img/icons/app6/app6_F_REC_SQ.edds");
		s_Map.Set("F_REC_SE", "{5ACA179A30FB28A3}img/icons/app6/app6_F_REC_SE.edds");
		s_Map.Set("F_REC_PL", "{8A0778F611A1479A}img/icons/app6/app6_F_REC_PL.edds");
		s_Map.Set("F_ENG_X", "{3F5973E5198D7CFD}img/icons/app6/app6_F_ENG_X.edds");
		s_Map.Set("F_ENG_TM", "{1E4E5291456D3C86}img/icons/app6/app6_F_ENG_TM.edds");
		s_Map.Set("F_ENG_SQ", "{A958D701BEE97FBF}img/icons/app6/app6_F_ENG_SQ.edds");
		s_Map.Set("F_ENG_SE", "{E2A81A2E468D6C4E}img/icons/app6/app6_F_ENG_SE.edds");
		s_Map.Set("F_ENG_PL", "{3265754267D70377}img/icons/app6/app6_F_ENG_PL.edds");
		s_Map.Set("F_MED_X", "{D21E2D2F7FA04005}img/icons/app6/app6_F_MED_X.edds");
		s_Map.Set("F_MED_TM", "{2030C5F965E19948}img/icons/app6/app6_F_MED_TM.edds");
		s_Map.Set("F_MED_SQ", "{972640699E65DA71}img/icons/app6/app6_F_MED_SQ.edds");
		s_Map.Set("F_MED_SE", "{DCD68D466601C980}img/icons/app6/app6_F_MED_SE.edds");
		s_Map.Set("F_MED_PL", "{0C1BE22A475BA6B9}img/icons/app6/app6_F_MED_PL.edds");
		s_Map.Set("F_SUP_X", "{5D07C16E3263753F}img/icons/app6/app6_F_SUP_X.edds");
		s_Map.Set("F_SUP_TM", "{D362E987331FB951}img/icons/app6/app6_F_SUP_TM.edds");
		s_Map.Set("F_SUP_SQ", "{64746C17C89BFA68}img/icons/app6/app6_F_SUP_SQ.edds");
		s_Map.Set("F_SUP_SE", "{2F84A13830FFE999}img/icons/app6/app6_F_SUP_SE.edds");
		s_Map.Set("F_SUP_PL", "{FF49CE5411A586A0}img/icons/app6/app6_F_SUP_PL.edds");
		s_Map.Set("F_ART_X", "{FDBA7F7A20235891}img/icons/app6/app6_F_ART_X.edds");
		s_Map.Set("F_ART_TM", "{545CE9AA2FC36842}img/icons/app6/app6_F_ART_TM.edds");
		s_Map.Set("F_ART_SQ", "{E34A6C3AD4472B7B}img/icons/app6/app6_F_ART_SQ.edds");
		s_Map.Set("F_ART_SE", "{A8BAA1152C23388A}img/icons/app6/app6_F_ART_SE.edds");
		s_Map.Set("F_ART_PL", "{7877CE790D7957B3}img/icons/app6/app6_F_ART_PL.edds");
		s_Map.Set("F_MOR_X", "{F90490A3EEB45803}img/icons/app6/app6_F_MOR_X.edds");
		s_Map.Set("F_MOR_TM", "{A3805621B681169D}img/icons/app6/app6_F_MOR_TM.edds");
		s_Map.Set("F_MOR_SQ", "{1496D3B14D0555A4}img/icons/app6/app6_F_MOR_SQ.edds");
		s_Map.Set("F_MOR_SE", "{5F661E9EB5614655}img/icons/app6/app6_F_MOR_SE.edds");
		s_Map.Set("F_MOR_PL", "{8FAB71F2943B296C}img/icons/app6/app6_F_MOR_PL.edds");
		s_Map.Set("F_SOF_X", "{2941409C1658C659}img/icons/app6/app6_F_SOF_X.edds");
		s_Map.Set("F_SOF_TM", "{AC92F6F35DA09C90}img/icons/app6/app6_F_SOF_TM.edds");
		s_Map.Set("F_SOF_SQ", "{1B847363A624DFA9}img/icons/app6/app6_F_SOF_SQ.edds");
		s_Map.Set("F_SOF_SE", "{5074BE4C5E40CC58}img/icons/app6/app6_F_SOF_SE.edds");
		s_Map.Set("F_SOF_PL", "{80B9D1207F1AA361}img/icons/app6/app6_F_SOF_PL.edds");
	}

	protected static void AddHostileSymbols()
	{
		s_Map.Set("H_INF_X", "{4360496E3C99BAA2}img/icons/app6/app6_H_INF_X.edds");
		s_Map.Set("H_INF_TM", "{8C63F86E1822F9F9}img/icons/app6/app6_H_INF_TM.edds");
		s_Map.Set("H_INF_SQ", "{3B757DFEE3A6BAC0}img/icons/app6/app6_H_INF_SQ.edds");
		s_Map.Set("H_INF_SE", "{7085B0D11BC2A931}img/icons/app6/app6_H_INF_SE.edds");
		s_Map.Set("H_INF_PL", "{A048DFBD3A98C608}img/icons/app6/app6_H_INF_PL.edds");
		s_Map.Set("H_AT_X", "{EFFF79D77E4F904C}img/icons/app6/app6_H_AT_X.edds");
		s_Map.Set("H_AT_TM", "{F286FFDC803E5D8B}img/icons/app6/app6_H_AT_TM.edds");
		s_Map.Set("H_AT_SQ", "{45907A4C7BBA1EB2}img/icons/app6/app6_H_AT_SQ.edds");
		s_Map.Set("H_AT_SE", "{0E60B76383DE0D43}img/icons/app6/app6_H_AT_SE.edds");
		s_Map.Set("H_AT_PL", "{DEADD80FA284627A}img/icons/app6/app6_H_AT_PL.edds");
		s_Map.Set("H_ARM_X", "{5DCAB0782E16E7CD}img/icons/app6/app6_H_ARM_X.edds");
		s_Map.Set("H_ARM_TM", "{1E13FF9B468D4B51}img/icons/app6/app6_H_ARM_TM.edds");
		s_Map.Set("H_ARM_SQ", "{A9057A0BBD090868}img/icons/app6/app6_H_ARM_SQ.edds");
		s_Map.Set("H_ARM_SE", "{E2F5B724456D1B99}img/icons/app6/app6_H_ARM_SE.edds");
		s_Map.Set("H_ARM_PL", "{3238D848643774A0}img/icons/app6/app6_H_ARM_PL.edds");
		s_Map.Set("H_REC_X", "{35C9DE1047D9FFAA}img/icons/app6/app6_H_REC_X.edds");
		s_Map.Set("H_REC_TM", "{9964A8925EBFDF1E}img/icons/app6/app6_H_REC_TM.edds");
		s_Map.Set("H_REC_SQ", "{2E722D02A53B9C27}img/icons/app6/app6_H_REC_SQ.edds");
		s_Map.Set("H_REC_SE", "{6582E02D5D5F8FD6}img/icons/app6/app6_H_REC_SE.edds");
		s_Map.Set("H_REC_PL", "{B54F8F417C05E0EF}img/icons/app6/app6_H_REC_PL.edds");
		s_Map.Set("H_ENG_X", "{6C1D124A78606BA5}img/icons/app6/app6_H_ENG_X.edds");
		s_Map.Set("H_ENG_TM", "{2106A52628C99BF3}img/icons/app6/app6_H_ENG_TM.edds");
		s_Map.Set("H_ENG_SQ", "{961020B6D34DD8CA}img/icons/app6/app6_H_ENG_SQ.edds");
		s_Map.Set("H_ENG_SE", "{DDE0ED992B29CB3B}img/icons/app6/app6_H_ENG_SE.edds");
		s_Map.Set("H_ENG_PL", "{0D2D82F50A73A402}img/icons/app6/app6_H_ENG_PL.edds");
		s_Map.Set("H_MED_X", "{815A4C801E4D575D}img/icons/app6/app6_H_MED_X.edds");
		s_Map.Set("H_MED_TM", "{1F78324E08453E3D}img/icons/app6/app6_H_MED_TM.edds");
		s_Map.Set("H_MED_SQ", "{A86EB7DEF3C17D04}img/icons/app6/app6_H_MED_SQ.edds");
		s_Map.Set("H_MED_SE", "{E39E7AF10BA56EF5}img/icons/app6/app6_H_MED_SE.edds");
		s_Map.Set("H_MED_PL", "{3353159D2AFF01CC}img/icons/app6/app6_H_MED_PL.edds");
		s_Map.Set("H_SUP_X", "{0E43A0C1538E6267}img/icons/app6/app6_H_SUP_X.edds");
		s_Map.Set("H_SUP_TM", "{EC2A1E305EBB1E24}img/icons/app6/app6_H_SUP_TM.edds");
		s_Map.Set("H_SUP_SQ", "{5B3C9BA0A53F5D1D}img/icons/app6/app6_H_SUP_SQ.edds");
		s_Map.Set("H_SUP_SE", "{10CC568F5D5B4EEC}img/icons/app6/app6_H_SUP_SE.edds");
		s_Map.Set("H_SUP_PL", "{C00139E37C0121D5}img/icons/app6/app6_H_SUP_PL.edds");
		s_Map.Set("H_ART_X", "{AEFE1ED541CE4FC9}img/icons/app6/app6_H_ART_X.edds");
		s_Map.Set("H_ART_TM", "{6B141E1D4267CF37}img/icons/app6/app6_H_ART_TM.edds");
		s_Map.Set("H_ART_SQ", "{DC029B8DB9E38C0E}img/icons/app6/app6_H_ART_SQ.edds");
		s_Map.Set("H_ART_SE", "{97F256A241879FFF}img/icons/app6/app6_H_ART_SE.edds");
		s_Map.Set("H_ART_PL", "{473F39CE60DDF0C6}img/icons/app6/app6_H_ART_PL.edds");
		s_Map.Set("H_MOR_X", "{AA40F10C8F594F5B}img/icons/app6/app6_H_MOR_X.edds");
		s_Map.Set("H_MOR_TM", "{9CC8A196DB25B1E8}img/icons/app6/app6_H_MOR_TM.edds");
		s_Map.Set("H_MOR_SQ", "{2BDE240620A1F2D1}img/icons/app6/app6_H_MOR_SQ.edds");
		s_Map.Set("H_MOR_SE", "{602EE929D8C5E120}img/icons/app6/app6_H_MOR_SE.edds");
		s_Map.Set("H_MOR_PL", "{B0E38645F99F8E19}img/icons/app6/app6_H_MOR_PL.edds");
		s_Map.Set("H_SOF_X", "{7A05213377B5D101}img/icons/app6/app6_H_SOF_X.edds");
		s_Map.Set("H_SOF_TM", "{93DA014430043BE5}img/icons/app6/app6_H_SOF_TM.edds");
		s_Map.Set("H_SOF_SQ", "{24CC84D4CB8078DC}img/icons/app6/app6_H_SOF_SQ.edds");
		s_Map.Set("H_SOF_SE", "{6F3C49FB33E46B2D}img/icons/app6/app6_H_SOF_SE.edds");
		s_Map.Set("H_SOF_PL", "{BFF1269712BE0414}img/icons/app6/app6_H_SOF_PL.edds");
	}

	protected static void AddNeutralSymbols()
	{
		s_Map.Set("N_INF_X", "{FE24122C949AACBD}img/icons/app6/app6_N_INF_X.edds");
		s_Map.Set("N_INF_TM", "{0D209D226C091FF7}img/icons/app6/app6_N_INF_TM.edds");
		s_Map.Set("N_INF_SQ", "{BA3618B2978D5CCE}img/icons/app6/app6_N_INF_SQ.edds");
		s_Map.Set("N_INF_SE", "{F1C6D59D6FE94F3F}img/icons/app6/app6_N_INF_SE.edds");
		s_Map.Set("N_INF_PL", "{210BBAF14EB32006}img/icons/app6/app6_N_INF_PL.edds");
		s_Map.Set("N_AT_X", "{BBB7369093935C12}img/icons/app6/app6_N_AT_X.edds");
		s_Map.Set("N_AT_TM", "{4FC2A49E283D4B94}img/icons/app6/app6_N_AT_TM.edds");
		s_Map.Set("N_AT_SQ", "{F8D4210ED3B908AD}img/icons/app6/app6_N_AT_SQ.edds");
		s_Map.Set("N_AT_SE", "{B324EC212BDD1B5C}img/icons/app6/app6_N_AT_SE.edds");
		s_Map.Set("N_AT_PL", "{63E9834D0A877465}img/icons/app6/app6_N_AT_PL.edds");
		s_Map.Set("N_ARM_X", "{E08EEB3A8615F1D2}img/icons/app6/app6_N_ARM_X.edds");
		s_Map.Set("N_ARM_TM", "{9F509AD732A6AD5F}img/icons/app6/app6_N_ARM_TM.edds");
		s_Map.Set("N_ARM_SQ", "{28461F47C922EE66}img/icons/app6/app6_N_ARM_SQ.edds");
		s_Map.Set("N_ARM_SE", "{63B6D2683146FD97}img/icons/app6/app6_N_ARM_SE.edds");
		s_Map.Set("N_ARM_PL", "{B37BBD04101C92AE}img/icons/app6/app6_N_ARM_PL.edds");
		s_Map.Set("N_REC_X", "{888D8552EFDAE9B5}img/icons/app6/app6_N_REC_X.edds");
		s_Map.Set("N_REC_TM", "{1827CDDE2A943910}img/icons/app6/app6_N_REC_TM.edds");
		s_Map.Set("N_REC_SQ", "{AF31484ED1107A29}img/icons/app6/app6_N_REC_SQ.edds");
		s_Map.Set("N_REC_SE", "{E4C18561297469D8}img/icons/app6/app6_N_REC_SE.edds");
		s_Map.Set("N_REC_PL", "{340CEA0D082E06E1}img/icons/app6/app6_N_REC_PL.edds");
		s_Map.Set("N_ENG_X", "{D1594908D0637DBA}img/icons/app6/app6_N_ENG_X.edds");
		s_Map.Set("N_ENG_TM", "{A045C06A5CE27DFD}img/icons/app6/app6_N_ENG_TM.edds");
		s_Map.Set("N_ENG_SQ", "{175345FAA7663EC4}img/icons/app6/app6_N_ENG_SQ.edds");
		s_Map.Set("N_ENG_SE", "{5CA388D55F022D35}img/icons/app6/app6_N_ENG_SE.edds");
		s_Map.Set("N_ENG_PL", "{8C6EE7B97E58420C}img/icons/app6/app6_N_ENG_PL.edds");
		s_Map.Set("N_MED_X", "{3C1E17C2B64E4142}img/icons/app6/app6_N_MED_X.edds");
		s_Map.Set("N_MED_TM", "{9E3B57027C6ED833}img/icons/app6/app6_N_MED_TM.edds");
		s_Map.Set("N_MED_SQ", "{292DD29287EA9B0A}img/icons/app6/app6_N_MED_SQ.edds");
		s_Map.Set("N_MED_SE", "{62DD1FBD7F8E88FB}img/icons/app6/app6_N_MED_SE.edds");
		s_Map.Set("N_MED_PL", "{B21070D15ED4E7C2}img/icons/app6/app6_N_MED_PL.edds");
		s_Map.Set("N_SUP_X", "{B307FB83FB8D7478}img/icons/app6/app6_N_SUP_X.edds");
		s_Map.Set("N_SUP_TM", "{6D697B7C2A90F82A}img/icons/app6/app6_N_SUP_TM.edds");
		s_Map.Set("N_SUP_SQ", "{DA7FFEECD114BB13}img/icons/app6/app6_N_SUP_SQ.edds");
		s_Map.Set("N_SUP_SE", "{918F33C32970A8E2}img/icons/app6/app6_N_SUP_SE.edds");
		s_Map.Set("N_SUP_PL", "{41425CAF082AC7DB}img/icons/app6/app6_N_SUP_PL.edds");
		s_Map.Set("N_ART_X", "{13BA4597E9CD59D6}img/icons/app6/app6_N_ART_X.edds");
		s_Map.Set("N_ART_TM", "{EA577B51364C2939}img/icons/app6/app6_N_ART_TM.edds");
		s_Map.Set("N_ART_SQ", "{5D41FEC1CDC86A00}img/icons/app6/app6_N_ART_SQ.edds");
		s_Map.Set("N_ART_SE", "{16B133EE35AC79F1}img/icons/app6/app6_N_ART_SE.edds");
		s_Map.Set("N_ART_PL", "{C67C5C8214F616C8}img/icons/app6/app6_N_ART_PL.edds");
		s_Map.Set("N_MOR_X", "{1704AA4E275A5944}img/icons/app6/app6_N_MOR_X.edds");
		s_Map.Set("N_MOR_TM", "{1D8BC4DAAF0E57E6}img/icons/app6/app6_N_MOR_TM.edds");
		s_Map.Set("N_MOR_SQ", "{AA9D414A548A14DF}img/icons/app6/app6_N_MOR_SQ.edds");
		s_Map.Set("N_MOR_SE", "{E16D8C65ACEE072E}img/icons/app6/app6_N_MOR_SE.edds");
		s_Map.Set("N_MOR_PL", "{31A0E3098DB46817}img/icons/app6/app6_N_MOR_PL.edds");
		s_Map.Set("N_SOF_X", "{C7417A71DFB6C71E}img/icons/app6/app6_N_SOF_X.edds");
		s_Map.Set("N_SOF_TM", "{12996408442FDDEB}img/icons/app6/app6_N_SOF_TM.edds");
		s_Map.Set("N_SOF_SQ", "{A58FE198BFAB9ED2}img/icons/app6/app6_N_SOF_SQ.edds");
		s_Map.Set("N_SOF_SE", "{EE7F2CB747CF8D23}img/icons/app6/app6_N_SOF_SE.edds");
		s_Map.Set("N_SOF_PL", "{3EB243DB6695E21A}img/icons/app6/app6_N_SOF_PL.edds");
	}

	protected static void AddUnknownSymbols()
	{
		s_Map.Set("U_INF_X", "{710EE1A5E7B609A1}img/icons/app6/app6_U_INF_X.edds");
		s_Map.Set("U_INF_TM", "{CD6D7962D56719EE}img/icons/app6/app6_U_INF_TM.edds");
		s_Map.Set("U_INF_SQ", "{7A7BFCF22EE35AD7}img/icons/app6/app6_U_INF_SQ.edds");
		s_Map.Set("U_INF_SE", "{318B31DDD6874926}img/icons/app6/app6_U_INF_SE.edds");
		s_Map.Set("U_INF_PL", "{E1465EB1F7DD261F}img/icons/app6/app6_U_INF_PL.edds");
		s_Map.Set("U_AT_X", "{8242CDC77BE43DD6}img/icons/app6/app6_U_AT_X.edds");
		s_Map.Set("U_AT_TM", "{C0E857175B11EE88}img/icons/app6/app6_U_AT_TM.edds");
		s_Map.Set("U_AT_SQ", "{77FED287A095ADB1}img/icons/app6/app6_U_AT_SQ.edds");
		s_Map.Set("U_AT_SE", "{3C0E1FA858F1BE40}img/icons/app6/app6_U_AT_SE.edds");
		s_Map.Set("U_AT_PL", "{ECC370C479ABD179}img/icons/app6/app6_U_AT_PL.edds");
		s_Map.Set("U_ARM_X", "{6FA418B3F53954CE}img/icons/app6/app6_U_ARM_X.edds");
		s_Map.Set("U_ARM_TM", "{5F1D7E978BC8AB46}img/icons/app6/app6_U_ARM_TM.edds");
		s_Map.Set("U_ARM_SQ", "{E80BFB07704CE87F}img/icons/app6/app6_U_ARM_SQ.edds");
		s_Map.Set("U_ARM_SE", "{A3FB36288828FB8E}img/icons/app6/app6_U_ARM_SE.edds");
		s_Map.Set("U_ARM_PL", "{73365944A97294B7}img/icons/app6/app6_U_ARM_PL.edds");
		s_Map.Set("U_REC_X", "{07A776DB9CF64CA9}img/icons/app6/app6_U_REC_X.edds");
		s_Map.Set("U_REC_TM", "{D86A299E93FA3F09}img/icons/app6/app6_U_REC_TM.edds");
		s_Map.Set("U_REC_SQ", "{6F7CAC0E687E7C30}img/icons/app6/app6_U_REC_SQ.edds");
		s_Map.Set("U_REC_SE", "{248C6121901A6FC1}img/icons/app6/app6_U_REC_SE.edds");
		s_Map.Set("U_REC_PL", "{F4410E4DB14000F8}img/icons/app6/app6_U_REC_PL.edds");
		s_Map.Set("U_ENG_X", "{5E73BA81A34FD8A6}img/icons/app6/app6_U_ENG_X.edds");
		s_Map.Set("U_ENG_TM", "{6008242AE58C7BE4}img/icons/app6/app6_U_ENG_TM.edds");
		s_Map.Set("U_ENG_SQ", "{D71EA1BA1E0838DD}img/icons/app6/app6_U_ENG_SQ.edds");
		s_Map.Set("U_ENG_SE", "{9CEE6C95E66C2B2C}img/icons/app6/app6_U_ENG_SE.edds");
		s_Map.Set("U_ENG_PL", "{4C2303F9C7364415}img/icons/app6/app6_U_ENG_PL.edds");
		s_Map.Set("U_MED_X", "{B334E44BC562E45E}img/icons/app6/app6_U_MED_X.edds");
		s_Map.Set("U_MED_TM", "{5E76B342C500DE2A}img/icons/app6/app6_U_MED_TM.edds");
		s_Map.Set("U_MED_SQ", "{E96036D23E849D13}img/icons/app6/app6_U_MED_SQ.edds");
		s_Map.Set("U_MED_SE", "{A290FBFDC6E08EE2}img/icons/app6/app6_U_MED_SE.edds");
		s_Map.Set("U_MED_PL", "{725D9491E7BAE1DB}img/icons/app6/app6_U_MED_PL.edds");
		s_Map.Set("U_SUP_X", "{3C2D080A88A1D164}img/icons/app6/app6_U_SUP_X.edds");
		s_Map.Set("U_SUP_TM", "{AD249F3C93FEFE33}img/icons/app6/app6_U_SUP_TM.edds");
		s_Map.Set("U_SUP_SQ", "{1A321AAC687ABD0A}img/icons/app6/app6_U_SUP_SQ.edds");
		s_Map.Set("U_SUP_SE", "{51C2D783901EAEFB}img/icons/app6/app6_U_SUP_SE.edds");
		s_Map.Set("U_SUP_PL", "{810FB8EFB144C1C2}img/icons/app6/app6_U_SUP_PL.edds");
		s_Map.Set("U_ART_X", "{9C90B61E9AE1FCCA}img/icons/app6/app6_U_ART_X.edds");
		s_Map.Set("U_ART_TM", "{2A1A9F118F222F20}img/icons/app6/app6_U_ART_TM.edds");
		s_Map.Set("U_ART_SQ", "{9D0C1A8174A66C19}img/icons/app6/app6_U_ART_SQ.edds");
		s_Map.Set("U_ART_SE", "{D6FCD7AE8CC27FE8}img/icons/app6/app6_U_ART_SE.edds");
		s_Map.Set("U_ART_PL", "{0631B8C2AD9810D1}img/icons/app6/app6_U_ART_PL.edds");
		s_Map.Set("U_MOR_X", "{982E59C75476FC58}img/icons/app6/app6_U_MOR_X.edds");
		s_Map.Set("U_MOR_TM", "{DDC6209A166051FF}img/icons/app6/app6_U_MOR_TM.edds");
		s_Map.Set("U_MOR_SQ", "{6AD0A50AEDE412C6}img/icons/app6/app6_U_MOR_SQ.edds");
		s_Map.Set("U_MOR_SE", "{2120682515800137}img/icons/app6/app6_U_MOR_SE.edds");
		s_Map.Set("U_MOR_PL", "{F1ED074934DA6E0E}img/icons/app6/app6_U_MOR_PL.edds");
		s_Map.Set("U_SOF_X", "{486B89F8AC9A6202}img/icons/app6/app6_U_SOF_X.edds");
		s_Map.Set("U_SOF_TM", "{D2D48048FD41DBF2}img/icons/app6/app6_U_SOF_TM.edds");
		s_Map.Set("U_SOF_SQ", "{65C205D806C598CB}img/icons/app6/app6_U_SOF_SQ.edds");
		s_Map.Set("U_SOF_SE", "{2E32C8F7FEA18B3A}img/icons/app6/app6_U_SOF_SE.edds");
		s_Map.Set("U_SOF_PL", "{FEFFA79BDFFBE403}img/icons/app6/app6_U_SOF_PL.edds");
	}

	protected static void AddIndependentSymbols()
	{
		s_Map.Set("F_INF_IND", "{B335B98ACB1689B8}img/icons/app6/app6_F_INF_IND.edds");
		s_Map.Set("F_AT_IND", "{B978A8365D373E71}img/icons/app6/app6_F_AT_IND.edds");
		s_Map.Set("F_MED_IND", "{FF292699BB3CE72F}img/icons/app6/app6_F_MED_IND.edds");
		s_Map.Set("F_ENG_IND", "{754DAEA1C7A8FF59}img/icons/app6/app6_F_ENG_IND.edds");
		s_Map.Set("F_REC_IND", "{D99BBA9D6179311B}img/icons/app6/app6_F_REC_IND.edds");
		s_Map.Set("F_ART_IND", "{AA749883CCC1AEEE}img/icons/app6/app6_F_ART_IND.edds");
		s_Map.Set("F_MOR_IND", "{7E5139B65CBF1D57}img/icons/app6/app6_F_MOR_IND.edds");
		s_Map.Set("H_INF_IND", "{4DC10F14366A1C5D}img/icons/app6/app6_H_INF_IND.edds");
		s_Map.Set("H_AT_IND", "{86305F8130939904}img/icons/app6/app6_H_AT_IND.edds");
		s_Map.Set("H_MED_IND", "{01DD9007464072CA}img/icons/app6/app6_H_MED_IND.edds");
		s_Map.Set("H_ENG_IND", "{8BB9183F3AD46ABC}img/icons/app6/app6_H_ENG_IND.edds");
		s_Map.Set("H_REC_IND", "{276F0C039C05A4FE}img/icons/app6/app6_H_REC_IND.edds");
		s_Map.Set("H_ART_IND", "{54802E1D31BD3B0B}img/icons/app6/app6_H_ART_IND.edds");
		s_Map.Set("H_MOR_IND", "{80A58F28A1C388B2}img/icons/app6/app6_H_MOR_IND.edds");
		s_Map.Set("N_INF_IND", "{BAAE474BC9545003}img/icons/app6/app6_N_INF_IND.edds");
		s_Map.Set("N_AT_IND", "{07733ACD44B87F0A}img/icons/app6/app6_N_AT_IND.edds");
		s_Map.Set("N_MED_IND", "{F6B2D858B97E3E94}img/icons/app6/app6_N_MED_IND.edds");
		s_Map.Set("N_ENG_IND", "{7CD65060C5EA26E2}img/icons/app6/app6_N_ENG_IND.edds");
		s_Map.Set("N_REC_IND", "{D000445C633BE8A0}img/icons/app6/app6_N_REC_IND.edds");
		s_Map.Set("N_ART_IND", "{A3EF6642CE837755}img/icons/app6/app6_N_ART_IND.edds");
		s_Map.Set("N_MOR_IND", "{77CAC7775EFDC4EC}img/icons/app6/app6_N_MOR_IND.edds");
		s_Map.Set("U_INF_IND", "{DBB5E027300C1CE1}img/icons/app6/app6_U_INF_IND.edds");
		s_Map.Set("U_AT_IND", "{C73EDE8DFDD67913}img/icons/app6/app6_U_AT_IND.edds");
		s_Map.Set("U_MED_IND", "{97A97F3440267276}img/icons/app6/app6_U_MED_IND.edds");
		s_Map.Set("U_ENG_IND", "{1DCDF70C3CB26A00}img/icons/app6/app6_U_ENG_IND.edds");
		s_Map.Set("U_REC_IND", "{B11BE3309A63A442}img/icons/app6/app6_U_REC_IND.edds");
		s_Map.Set("U_ART_IND", "{C2F4C12E37DB3BB7}img/icons/app6/app6_U_ART_IND.edds");
		s_Map.Set("U_MOR_IND", "{16D1601BA7A5880E}img/icons/app6/app6_U_MOR_IND.edds");
		s_Map.Set("F_FACTION", "{55CFB57AFF8DF0DE}img/icons/app6/app6_F_FACTION.edds");
		s_Map.Set("H_FACTION", "{AB3B03E402F1653B}img/icons/app6/app6_H_FACTION.edds");
		s_Map.Set("N_FACTION", "{5C544BBBFDCF2965}img/icons/app6/app6_N_FACTION.edds");
		s_Map.Set("U_FACTION", "{3D4FECD704976587}img/icons/app6/app6_U_FACTION.edds");
	}
}
