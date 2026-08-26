// APP-6 NATO symbol classifier.
class DCO_App6Icons
{
	static ResourceName GetIcon(notnull array<EEditableEntityLabel> labels, string name, FactionKey faction, int type)
	{
		string affil = AffilFromLabels(labels);
		if (affil.IsEmpty())
			affil = AffilOf(faction);	// fallback: no faction label -> derive from the faction key.

		if (type == EEditableEntityType.CHARACTER)
		{
			string roleTok = RoleTokenFromLabels(labels, name);
			ResourceName r = DCO_App6Roles.Get(affil + "_" + roleTok);
			if (r.IsEmpty())
				r = DCO_App6Roles.Get(affil + "_RIFLE");	// fallback to the generic rifleman pictogram.
			return r;
		}

		if (type == EEditableEntityType.GROUP)
		{
			string fn = FuncFromGroup(labels, name);
			string ech = EchelonFromLabels(labels);
			if (ech.IsEmpty())
				ech = EchelonOf(name);	// fallback: no GROUPSIZE label -> derive from the name.
			ResourceName r = DCO_App6Symbols.Get(affil + "_" + fn + "_" + ech);
			if (r.IsEmpty())
				r = DCO_App6Symbols.Get(affil + "_" + fn + "_X");	// fall back to the no-echelon framed symbol.
			return r;
		}

		if (type == EEditableEntityType.FACTION)
			return DCO_App6Symbols.Get(affil + "_FACTION");	// the bare APP-6 affiliation frame for the whole faction.

		return ResourceName.Empty;	// vehicle / object / system -> caller keeps the engine editor icon.
	}

	// Convenience: the APP-6 / role icon for a LIVE editable entity, extracting labels/name/faction/type from its UIInfo.
	static ResourceName ForEntity(SCR_EditableEntityComponent e)
	{
		if (!e)
			return ResourceName.Empty;
		SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.Cast(e.GetInfo());
		if (!info)
			return ResourceName.Empty;
		array<EEditableEntityLabel> labels = {};
		info.GetEntityLabels(labels);
		return GetIcon(labels, info.GetName(), info.GetFactionKey(), e.GetEntityType());
	}

	static ResourceName FactionIcon(FactionKey faction)
	{
		return DCO_App6Symbols.Get(AffilOf(faction) + "_FACTION");
	}


	protected static string AffilFromLabels(notnull array<EEditableEntityLabel> labels)
	{
		if (labels.Contains(EEditableEntityLabel.FACTION_US))
			return "F";
		if (labels.Contains(EEditableEntityLabel.FACTION_USSR))
			return "H";
		if (labels.Contains(EEditableEntityLabel.FACTION_FIA))
			return "N";
		if (labels.Contains(EEditableEntityLabel.FACTION_CIV))
			return "U";
		return "";
	}

	protected static string RoleTokenFromLabels(notnull array<EEditableEntityLabel> labels, string name)
	{
		if (labels.Contains(EEditableEntityLabel.ROLE_MACHINEGUNNER))
			return "MG";
		if (labels.Contains(EEditableEntityLabel.ROLE_ANTITANK))
			return "AT";
		if (labels.Contains(EEditableEntityLabel.ROLE_GRENADIER))
			return "GREN";
		if (labels.Contains(EEditableEntityLabel.ROLE_MEDIC))
			return "MED";
		if (labels.Contains(EEditableEntityLabel.ROLE_RADIOOPERATOR))
			return "RADIO";
		if (labels.Contains(EEditableEntityLabel.ROLE_LEADER))
			return "LEAD";
		if (labels.Contains(EEditableEntityLabel.ROLE_SHARPSHOOTER))
			return "SNIP";
		if (labels.Contains(EEditableEntityLabel.ROLE_SAPPER))
			return "ENG";
		if (labels.Contains(EEditableEntityLabel.ROLE_AMMOBEARER))
			return "AMMO";
		if (labels.Contains(EEditableEntityLabel.ROLE_SCOUT))
			return "SCOUT";
		if (labels.Contains(EEditableEntityLabel.ROLE_RIFLEMAN))
			return "RIFLE";
		return RoleTokenFromName(name);	// no role label -> name keyword.
	}

	protected static string RoleTokenFromName(string name)
	{
		string n = name;
		n.ToLower();
		if (n.Contains("machine") || n.Contains("automatic rifle") || n.Contains("gunner"))
			return "MG";
		if (n.Contains("anti-tank") || n.Contains("anti tank") || n.Contains("antitank") || n.Contains(" at ") || n.Contains("rocket") || n.Contains("rpg") || n.Contains("launcher"))
			return "AT";
		if (n.Contains("grenadier"))
			return "GREN";
		if (n.Contains("medic"))
			return "MED";
		if (n.Contains("signal") || n.Contains("radio"))
			return "RADIO";
		if (n.Contains("leader") || n.Contains("officer") || n.Contains("sergeant") || n.Contains("commander"))
			return "LEAD";
		if (n.Contains("sniper") || n.Contains("marksman") || n.Contains("sharpshooter"))
			return "SNIP";
		if (n.Contains("engineer") || n.Contains("sapper"))
			return "ENG";
		if (n.Contains("ammuni") || n.Contains("ammo") || n.Contains("bearer"))
			return "AMMO";
		if (n.Contains("scout") || n.Contains("recon"))
			return "SCOUT";
		return "RIFLE";	// default soldier.
	}

	protected static string FuncFromGroup(notnull array<EEditableEntityLabel> labels, string name)
	{
		if (labels.Contains(EEditableEntityLabel.TRAIT_MORTAR))
			return "MOR";
		if (labels.Contains(EEditableEntityLabel.TRAIT_ARMOR))
			return "ARM";
		if (labels.Contains(EEditableEntityLabel.TRAIT_MEDICAL))
			return "MED";
		if (labels.Contains(EEditableEntityLabel.TRAIT_LOGISTICS) || labels.Contains(EEditableEntityLabel.TRAIT_ARSENAL))
			return "SUP";
		if (labels.Contains(EEditableEntityLabel.ROLE_ANTITANK))
			return "AT";
		return FuncOf(name);
	}

	protected static string EchelonFromLabels(notnull array<EEditableEntityLabel> labels)
	{
		if (labels.Contains(EEditableEntityLabel.GROUPSIZE_LARGE))
			return "PL";
		if (labels.Contains(EEditableEntityLabel.GROUPSIZE_MEDIUM))
			return "SQ";
		if (labels.Contains(EEditableEntityLabel.GROUPSIZE_SMALL))
			return "TM";
		return "";
	}


	// Faction key -> affiliation token.
	protected static string AffilOf(FactionKey faction)
	{
		string f = faction;
		f.ToLower();
		if (f == "us")
			return "F";
		if (f == "ussr")
			return "H";
		if (f == "fia")
			return "N";
		if (f == "civ" || f == "civilian")
			return "U";
		return "U";
	}

	protected static string FuncOf(string name)
	{
		string n = name;
		n.ToLower();
		if (n.Contains("anti-tank") || n.Contains("anti tank") || n.Contains("antitank") || n.Contains("anti-armor") || n.Contains("antiarmor") || n.Contains("anti-armour"))
			return "AT";
		if (n.Contains("mortar"))
			return "MOR";
		if (n.Contains("artillery") || n.Contains("howitzer") || n.Contains("battery") || n.Contains("gun line"))
			return "ART";
		if (n.Contains("recon") || n.Contains("scout") || n.Contains("sniper") || n.Contains("marksman") || n.Contains("observ"))
			return "REC";
		if (n.Contains("engineer") || n.Contains("sapper") || n.Contains("demolition") || n.Contains("eod"))
			return "ENG";
		if (n.Contains("medic") || n.Contains("medical") || n.Contains("aid") || n.Contains("casualty"))
			return "MED";
		if (n.Contains("ammuni") || n.Contains("supply") || n.Contains("logistic") || n.Contains("resupply") || n.Contains("quartermaster"))
			return "SUP";
		if (n.Contains("armor") || n.Contains("armour") || n.Contains("tank") || n.Contains("mechaniz") || n.Contains("mechanis"))
			return "ARM";
		if (n.Contains("special forces") || n.Contains("special force") || n.Contains("spetsnaz") || n.Contains("commando"))
			return "SOF";
		return "INF";
	}

	// Unit name -> echelon token.
	protected static string EchelonOf(string name)
	{
		string n = name;
		n.ToLower();
		if (n.Contains("platoon") || n.Contains("company") || n.Contains("battalion"))
			return "PL";
		if (n.Contains("section"))
			return "SE";
		if (n.Contains("squad"))
			return "SQ";
		if (n.Contains("fire team") || n.Contains("team") || n.Contains("crew") || n.Contains("pair"))
			return "TM";
		return "SQ";
	}
}
