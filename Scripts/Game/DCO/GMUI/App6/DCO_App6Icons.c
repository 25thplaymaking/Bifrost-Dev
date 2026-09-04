// APP-6 NATO symbol classifier.
class DCO_App6Icons
{
	static ResourceName GetIcon(notnull array<EEditableEntityLabel> labels, string name, FactionKey faction, int type)
	{
		string affil = AffilFromLabels(labels);
		if (affil.IsEmpty())
			affil = AffilForFaction(faction);

		if (type == EEditableEntityType.CHARACTER)
		{
			string roleTok = RoleTokenFromLabels(labels, name);
			ResourceName r = DCO_App6Roles.Get(roleTok);
			if (r.IsEmpty())
				r = DCO_App6Roles.Get("RIFLE");
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
			return FactionIcon(faction);

		return ResourceName.Empty;	// vehicle / object / system -> caller keeps the engine editor icon.
	}

	// Returns the best symbol for an editable entity.
	static ResourceName ForEntity(SCR_EditableEntityComponent e)
	{
		if (!e)
			return ResourceName.Empty;
		SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.Cast(e.GetInfo());
		if (!info)
			return ResourceName.Empty;
		array<EEditableEntityLabel> labels = {};
		info.GetEntityLabels(labels);
		if (e.GetEntityType() == EEditableEntityType.GROUP)
		{
			ResourceName liveGroupIcon = LiveGroupIcon(e, labels, info.GetFactionKey());
			if (!liveGroupIcon.IsEmpty())
				return liveGroupIcon;
		}
		return GetIcon(labels, info.GetName(), info.GetFactionKey(), e.GetEntityType());
	}

	protected static ResourceName LiveGroupIcon(SCR_EditableEntityComponent editable, notnull array<EEditableEntityLabel> labels, FactionKey faction)
	{
		IEntity owner = editable.GetOwner();
		if (!owner)
			return ResourceName.Empty;
		SCR_GroupIdentityComponent identity = SCR_GroupIdentityComponent.Cast(owner.FindComponent(SCR_GroupIdentityComponent));
		if (!identity)
			return ResourceName.Empty;
		SCR_MilitarySymbol symbol = identity.GetMilitarySymbol();
		if (!symbol || symbol.GetIcons() == 0)
			return ResourceName.Empty;

		string affil = AffilFromMilitarySymbol(symbol.GetIdentity());
		if (affil.IsEmpty())
			affil = AffilFromLabels(labels);
		if (affil.IsEmpty())
			affil = AffilForFaction(faction);
		string functionToken = FuncFromMilitarySymbol(symbol);
		string echelonToken = EchelonFromMilitarySymbol(symbol.GetAmplifier());
		ResourceName result = DCO_App6Symbols.Get(affil + "_" + functionToken + "_" + echelonToken);
		if (result.IsEmpty())
			result = DCO_App6Symbols.Get(affil + "_" + functionToken + "_X");
		return result;
	}

	protected static string AffilFromMilitarySymbol(EMilitarySymbolIdentity identity)
	{
		if (identity == EMilitarySymbolIdentity.BLUFOR || identity == EMilitarySymbolIdentity.ASSUMED_BLUFOR)
			return "F";
		if (identity == EMilitarySymbolIdentity.OPFOR || identity == EMilitarySymbolIdentity.ASSUMED_OPFOR)
			return "H";
		if (identity == EMilitarySymbolIdentity.INDFOR || identity == EMilitarySymbolIdentity.ASSUMED_INDFOR)
			return "N";
		if (identity == EMilitarySymbolIdentity.CIVILIAN || identity == EMilitarySymbolIdentity.ASSUMED_CIVILIAN)
			return "U";
		return "";
	}

	protected static string FuncFromMilitarySymbol(notnull SCR_MilitarySymbol symbol)
	{
		if (symbol.HasIcon(EMilitarySymbolIcon.MORTAR))
			return "MOR";
		if (symbol.HasIcon(EMilitarySymbolIcon.ARTILLERY))
			return "ART";
		if (symbol.HasIcon(EMilitarySymbolIcon.ARMOR))
			return "ARM";
		if (symbol.HasIcon(EMilitarySymbolIcon.ANTITANK))
			return "AT";
		if (symbol.HasIcon(EMilitarySymbolIcon.RECON) || symbol.HasIcon(EMilitarySymbolIcon.SNIPER))
			return "REC";
		if (symbol.HasIcon(EMilitarySymbolIcon.MEDICAL))
			return "MED";
		if (symbol.HasIcon(EMilitarySymbolIcon.MAINTENANCE))
			return "ENG";
		if (symbol.HasIcon(EMilitarySymbolIcon.SUPPLY) || symbol.HasIcon(EMilitarySymbolIcon.SIGNAL) || symbol.HasIcon(EMilitarySymbolIcon.RELAY))
			return "SUP";
		return "INF";
	}

	protected static string EchelonFromMilitarySymbol(EMilitarySymbolAmplifier amplifier)
	{
		if (amplifier == EMilitarySymbolAmplifier.TEAM)
			return "TM";
		if (amplifier == EMilitarySymbolAmplifier.SQUAD)
			return "SQ";
		if (amplifier == EMilitarySymbolAmplifier.SECTION)
			return "SE";
		if (amplifier == EMilitarySymbolAmplifier.NONE)
			return "X";
		return "PL";
	}

	static ResourceName FactionIcon(FactionKey faction)
	{
		SCR_Faction authoredFaction = ResolveFaction(faction);
		if (authoredFaction)
		{
			SCR_UIInfo authoredInfo = FactionUIInfo(authoredFaction);
			if (IsUniqueFactionIcon(faction, authoredInfo))
			{
				if (!authoredInfo.GetIconPath().IsEmpty())
					return authoredInfo.GetIconPath();
				return authoredInfo.GetImageSetPath();
			}
			ResourceName flag = authoredFaction.GetFactionFlag();
			if (IsUniqueFactionFlag(faction, flag))
				return flag;
		}
		return DCO_App6Symbols.Get(AffilForFaction(faction) + "_FACTION");
	}

	// Preserves faction-specific art while replacing shared base-game art with APP-6.
	static bool SetFactionIcon(ImageWidget image, FactionKey faction, out string source)
	{
		source = "none";
		if (!image)
			return false;

		SCR_Faction authoredFaction = ResolveFaction(faction);
		if (authoredFaction)
		{
			SCR_UIInfo authoredInfo = FactionUIInfo(authoredFaction);
			if (IsUniqueFactionIcon(faction, authoredInfo))
			{
				if (authoredInfo && authoredInfo.HasIcon() && authoredInfo.SetIconTo(image))
				{
					source = "custom faction icon";
					return true;
				}
			}

			ResourceName flag = authoredFaction.GetFactionFlag();
			if (IsUniqueFactionFlag(faction, flag) && image.LoadImageTexture(0, flag))
			{
				source = "custom faction flag";
				return true;
			}
		}

		ResourceName app6 = DCO_App6Symbols.Get(AffilForFaction(faction) + "_FACTION");
		if (!app6.IsEmpty() && image.LoadImageTexture(0, app6))
		{
			source = "APP-6 faction";
			return true;
		}
		return false;
	}

	static bool IsPackageIcon(ResourceName icon)
	{
		if (icon.IsEmpty())
			return false;
		string path = icon.GetPath();
		path.ToLower();
		return path.Contains("img/icons/app6/");
	}

	protected static SCR_Faction ResolveFaction(FactionKey faction)
	{
		FactionManager manager = GetGame().GetFactionManager();
		if (!manager)
			return null;
		return SCR_Faction.Cast(manager.GetFactionByKey(faction));
	}

	protected static SCR_UIInfo FactionUIInfo(SCR_Faction faction)
	{
		if (!faction)
			return null;
		UIInfo rawInfo = faction.GetUIInfo();
		if (!rawInfo)
			return null;
		return SCR_UIInfo.CreateInfo(rawInfo);
	}

	protected static string AffilForFaction(FactionKey faction)
	{
		SCR_Faction authoredFaction = ResolveFaction(faction);
		SCR_Faction current = authoredFaction;
		for (int depth = 0; current && depth < 8; depth++)
		{
			string inheritedAffil = KnownAffil(current.GetFactionKey());
			if (!inheritedAffil.IsEmpty())
				return inheritedAffil;
			current = SCR_Faction.Cast(current.GetParent());
		}

		if (authoredFaction)
		{
			EEditableEntityLabel label = authoredFaction.GetFactionLabel();
			if (label == EEditableEntityLabel.FACTION_US || authoredFaction.IsInherited("US"))
				return "F";
			if (label == EEditableEntityLabel.FACTION_USSR || authoredFaction.IsInherited("USSR"))
				return "H";
			if (label == EEditableEntityLabel.FACTION_FIA || authoredFaction.IsInherited("FIA"))
				return "N";
			if (label == EEditableEntityLabel.FACTION_CIV || label == EEditableEntityLabel.FACTION_NONE)
				return "U";
		}
		return AffilOf(faction);
	}

	protected static bool IsCanonicalFaction(FactionKey faction)
	{
		string key = faction;
		key.ToLower();
		return key == "us" || key == "ussr" || key == "fia" || key == "civ" || key == "civilian";
	}

	protected static bool IsUniqueFactionIcon(FactionKey faction, SCR_UIInfo candidate)
	{
		if (IsCanonicalFaction(faction) || !candidate || !candidate.HasIcon())
			return false;
		SCR_Faction current = ResolveFaction(faction);
		for (int depth = 0; current && depth < 8; depth++)
		{
			SCR_Faction parentFaction = SCR_Faction.Cast(current.GetParent());
			if (!parentFaction)
				return true;
			SCR_UIInfo parentInfo = FactionUIInfo(parentFaction);
			if (!parentInfo || !parentInfo.HasIcon() || !SameFactionIcon(candidate, parentInfo))
				return true;
			if (IsCanonicalFaction(parentFaction.GetFactionKey()))
				return false;
			current = parentFaction;
		}
		return true;
	}

	protected static bool SameFactionIcon(SCR_UIInfo left, SCR_UIInfo right)
	{
		return SameResource(left.GetIconPath(), right.GetIconPath())
			&& SameResource(left.GetImageSetPath(), right.GetImageSetPath())
			&& left.GetIconSetName() == right.GetIconSetName();
	}

	protected static bool IsUniqueFactionFlag(FactionKey faction, ResourceName flag)
	{
		if (IsCanonicalFaction(faction) || flag.IsEmpty())
			return false;
		SCR_Faction current = ResolveFaction(faction);
		for (int depth = 0; current && depth < 8; depth++)
		{
			SCR_Faction parentFaction = SCR_Faction.Cast(current.GetParent());
			if (!parentFaction)
				return true;
			ResourceName parentFlag = parentFaction.GetFactionFlag();
			if (parentFlag.IsEmpty() || !SameResource(flag, parentFlag))
				return true;
			if (IsCanonicalFaction(parentFaction.GetFactionKey()))
				return false;
			current = parentFaction;
		}
		return true;
	}

	protected static bool SameResource(ResourceName left, ResourceName right)
	{
		if (left.IsEmpty() || right.IsEmpty())
			return left.IsEmpty() && right.IsEmpty();
		string leftPath = left.GetPath();
		string rightPath = right.GetPath();
		leftPath.ToLower();
		rightPath.ToLower();
		return leftPath == rightPath;
	}

	protected static string KnownAffil(FactionKey faction)
	{
		string key = faction;
		key.ToLower();
		if (key == "us")
			return "F";
		if (key == "ussr")
			return "H";
		if (key == "fia")
			return "N";
		if (key == "civ" || key == "civilian")
			return "U";
		return "";
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
