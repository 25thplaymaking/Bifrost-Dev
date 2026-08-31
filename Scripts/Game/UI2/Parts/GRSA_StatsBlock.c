//! Honest weapon data block for the gunsmith stage: every line is read live off engine data and
//! lines with no engine source are omitted, never invented. Identity lines (type, caliber,
//! magazine, built weight) read off the already-staged entity; ballistic lines (fire modes,
//! rate, velocity) ride the base field manual's stats helper and its own full prefab spawn —
//! the native muzzle state behind those reads is uninitialized on preview-pipeline entities
//! and dereferencing it crashes the process.
class GRSA_StatsBlock
{
	protected static const int LABEL_WIDTH = 12;

	protected Widget m_wRoot;
	protected TextWidget m_wTitle;
	protected RichTextWidget m_wText;
	protected string m_sLabelColor;
	protected ResourceName m_HelperPrefab;
	protected ref SCR_FieldManualUI_WeaponStatsHelper m_Helper;

	//------------------------------------------------------------------------------------------------
	void GRSA_StatsBlock(Widget screenRoot)
	{
		if (!screenRoot)
			return;

		m_wRoot = screenRoot.FindAnyWidget("StatsBlock");
		if (!m_wRoot)
			return;

		m_wTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("StatsTitle"));
		m_wText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("StatsText"));
		m_sLabelColor = UIColors.FormatColor(GRSA_Theme.Separator());
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		if (m_wRoot)
			m_wRoot.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Fills the block from the staged weapon entity; components a weapon does not carry simply
	//! drop their lines. Hidden entirely when the entity offers no weapon data at all.
	void Refresh(IEntity weaponSource, ResourceName weaponPrefab, GRSA_ItemEntry entry, string displayName, IEntity character)
	{
		if (!m_wRoot || !m_wText)
			return;

		BaseWeaponComponent weapon;
		if (weaponSource)
			weapon = BaseWeaponComponent.Cast(weaponSource.FindComponent(BaseWeaponComponent));

		if (!weapon)
		{
			Hide();
			return;
		}

		if (m_wTitle)
			m_wTitle.SetText(displayName);

		string text = Line("TYPE", WeaponTypeName(weapon.GetWeaponType()));

		BaseMagazineComponent magazine = FindMagazine(weapon);
		if (magazine)
		{
			MagazineUIInfo magazineInfo = MagazineUIInfo.Cast(magazine.GetUIInfo());
			if (magazineInfo && !magazineInfo.GetAmmoCaliber().IsEmpty())
				text += Line("CALIBER", FormatCaliber(magazineInfo.GetAmmoCaliber()));

			if (magazine.GetMaxAmmoCount() > 0)
				text += Line("MAGAZINE", string.Format("%1 rounds", magazine.GetMaxAmmoCount()));
		}

		if (!m_Helper || m_HelperPrefab != weaponPrefab)
		{
			m_Helper = new SCR_FieldManualUI_WeaponStatsHelper(weaponPrefab);
			m_HelperPrefab = weaponPrefab;
		}

		string fireModes = FormatFireModes(m_Helper.GetFireModes());
		if (!fireModes.IsEmpty())
			text += Line("FIRE MODES", fireModes);

		int rateOfFire = m_Helper.GetRateOfFire();
		if (rateOfFire > 0)
			text += Line("RATE", string.Format("%1 rpm", rateOfFire));

		int velocity = m_Helper.m_iMuzzleVelocity;
		if (velocity > 0)
			text += Line("VELOCITY", string.Format("%1 m/s", velocity));

		float mass = ReadMass(weaponSource);
		if (mass > 0)
			text += Line("WEIGHT", string.Format("%1 kg", mass.ToString(1, 2)));

		if (entry && GRSA_DraftService.RanksActive() && entry.m_eRequiredRank > SCR_ECharacterRank.PRIVATE)
			text += Line("RANK", WidgetManager.Translate(SCR_CharacterRankComponent.GetRankName(character, entry.m_eRequiredRank)));

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (entry && service && service.UsesSupplies() && entry.m_iSupplyCost > 0)
			text += Line("SUPPLY", entry.m_iSupplyCost.ToString());

		m_wText.SetText(text);
		m_wRoot.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	//! Engine caliber labels vary between "5.56x45m", "5.56x45mm", and spaced forms.
	protected string FormatCaliber(string value)
	{
		string lower = value;
		lower.ToLower();

		if (lower.EndsWith("mm"))
		{
			int unitIndex = value.Length() - 2;
			if (unitIndex > 0 && value.Get(unitIndex - 1) != " ")
				return value.Substring(0, unitIndex) + " mm";
		}
		else if (lower.EndsWith("m") && value.Length() > 1)
		{
			int unitIndex = value.Length() - 1;
			string previous = value.Get(unitIndex - 1);
			if ("0123456789".IndexOf(previous) >= 0)
				return value.Substring(0, unitIndex) + " mm";
		}

		return value;
	}

	//------------------------------------------------------------------------------------------------
	//! The rendered clone can ship without its default magazine spawned; fall back to the pooled
	//! instance of the muzzle's authored default so caliber and capacity still read true.
	protected BaseMagazineComponent FindMagazine(notnull BaseWeaponComponent weapon)
	{
		BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
		if (magazine)
			return magazine;

		BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
		if (!muzzle)
			return null;

		ResourceName defaultMagazine = muzzle.GetDefaultMagazineOrProjectileName();
		if (defaultMagazine.IsEmpty())
			return null;

		ItemPreviewManagerEntity manager = GRSA_ItemIntel.GetPreviewManager();
		if (!manager)
			return null;

		IEntity pooled = manager.ResolvePreviewEntityForPrefab(defaultMagazine);
		if (!pooled)
			return null;

		return BaseMagazineComponent.Cast(pooled.FindComponent(BaseMagazineComponent));
	}

	//------------------------------------------------------------------------------------------------
	//! Helper mode names are raw UI keys; translate and join, skipping anything unnamed.
	protected string FormatFireModes(array<string> modeNames)
	{
		string fireModes;
		if (!modeNames)
			return fireModes;

		foreach (string modeName : modeNames)
		{
			string translated = WidgetManager.Translate(modeName);
			if (translated.IsEmpty())
				continue;

			if (!fireModes.IsEmpty())
				fireModes += " / ";
			fireModes += translated;
		}

		return fireModes;
	}

	//------------------------------------------------------------------------------------------------
	//! Storage total counts every mounted attachment, the built weapon's real carry weight.
	protected float ReadMass(notnull IEntity weaponSource)
	{
		WeaponAttachmentsStorageComponent storage = WeaponAttachmentsStorageComponent.Cast(weaponSource.FindComponent(WeaponAttachmentsStorageComponent));
		if (storage)
			return storage.GetTotalWeight();

		InventoryItemComponent item = InventoryItemComponent.Cast(weaponSource.FindComponent(InventoryItemComponent));
		if (item)
			return item.GetTotalWeight();

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected string WeaponTypeName(EWeaponType type)
	{
		switch (type)
		{
			case EWeaponType.WT_RIFLE: return "RIFLE";
			case EWeaponType.WT_GRENADELAUNCHER: return "GRENADE LAUNCHER";
			case EWeaponType.WT_SNIPERRIFLE: return "SNIPER RIFLE";
			case EWeaponType.WT_ROCKETLAUNCHER: return "ROCKET LAUNCHER";
			case EWeaponType.WT_MACHINEGUN: return "MACHINE GUN";
			case EWeaponType.WT_HANDGUN: return "HANDGUN";
			case EWeaponType.WT_FRAGGRENADE: return "FRAG GRENADE";
			case EWeaponType.WT_SMOKEGRENADE: return "SMOKE GRENADE";
			case EWeaponType.WT_AUTOCANNON: return "AUTOCANNON";
		}
		return "WEAPON";
	}

	//------------------------------------------------------------------------------------------------
	//! Mono font column: label padded to a fixed width in the accent color, value in body color.
	protected string Line(string label, string value)
	{
		if (value.IsEmpty())
			return string.Empty;

		string padded = label;
		while (padded.Length() < LABEL_WIDTH)
			padded += " ";

		return string.Format("<color rgba='%1'>%2</color> %3<br/>", m_sLabelColor, padded, value);
	}
}
