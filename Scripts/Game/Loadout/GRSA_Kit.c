class GRSA_KitClothing
{
	int m_iSlotIdx;
	ResourceName m_Prefab;
}

class GRSA_KitWeapon
{
	int m_iSlotIdx;
	ResourceName m_Prefab;
	ref array<ResourceName> m_aAttachments = {};

	//! Parallel to m_aAttachments: authored hardpoint slot index on the weapon's own attachment
	//! storage the item is pinned to, -1 = automatic first-fit. Additive wire key ("slot"), so
	//! legacy kits read as all-automatic.
	ref array<int> m_aAttachmentSlots = {};

	//! One-shot: the player re-picked this weapon while already holding it, apply spawns a fresh
	//! instance instead of keeping the possibly spent or emptied one.
	bool m_bForceReplace;

	//------------------------------------------------------------------------------------------------
	//! Keeps the pin array in lockstep with the attachment list, -1 fills any gap.
	void EnsurePins()
	{
		while (m_aAttachmentSlots.Count() < m_aAttachments.Count())
			m_aAttachmentSlots.Insert(-1);
		while (m_aAttachmentSlots.Count() > m_aAttachments.Count())
			m_aAttachmentSlots.Remove(m_aAttachmentSlots.Count() - 1);
	}

	//------------------------------------------------------------------------------------------------
	int GetPinAt(int index)
	{
		if (index < 0 || index >= m_aAttachmentSlots.Count())
			return -1;
		return m_aAttachmentSlots[index];
	}
}

class GRSA_KitExtra
{
	ResourceName m_Prefab;
	int m_iCount;
	ResourceName m_Container;
}

class GRSA_Kit
{
	static const int CURRENT_VERSION = 1;

	int m_iVersion;
	string m_sName;
	string m_sFactionKey;
	int m_iSavedAtUnix;
	string m_sSummaryWeapons;
	string m_sSummaryClothing;
	float m_fSuppliesCost;
	int m_iRequiredRank;

	ref array<ref GRSA_KitClothing> m_aClothings = {};
	ref array<ref GRSA_KitWeapon> m_aWeapons = {};
	ref array<ref GRSA_KitExtra> m_aExtras = {};

	//------------------------------------------------------------------------------------------------
	void GRSA_Kit()
	{
		m_iVersion = CURRENT_VERSION;
	}

	//------------------------------------------------------------------------------------------------
	void StampSavedNow()
	{
		m_iSavedAtUnix = System.GetUnixTime();
	}

	//------------------------------------------------------------------------------------------------
	bool IsEmpty()
	{
		return m_aClothings.IsEmpty() && m_aWeapons.IsEmpty() && m_aExtras.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	GRSA_KitClothing FindClothing(int slotIdx)
	{
		foreach (GRSA_KitClothing clothing : m_aClothings)
		{
			if (clothing.m_iSlotIdx == slotIdx)
				return clothing;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	GRSA_KitWeapon FindWeapon(int slotIdx)
	{
		foreach (GRSA_KitWeapon weapon : m_aWeapons)
		{
			if (weapon.m_iSlotIdx == slotIdx)
				return weapon;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	void SetClothing(int slotIdx, ResourceName prefab)
	{
		GRSA_KitClothing clothing = FindClothing(slotIdx);
		if (prefab.IsEmpty())
		{
			if (clothing)
				m_aClothings.RemoveItem(clothing);
			return;
		}

		if (!clothing)
		{
			clothing = new GRSA_KitClothing();
			clothing.m_iSlotIdx = slotIdx;
			m_aClothings.Insert(clothing);
		}
		clothing.m_Prefab = prefab;
	}

	//------------------------------------------------------------------------------------------------
	void SetWeapon(int slotIdx, ResourceName prefab)
	{
		GRSA_KitWeapon weapon = FindWeapon(slotIdx);
		if (prefab.IsEmpty())
		{
			if (weapon)
				m_aWeapons.RemoveItem(weapon);
			return;
		}

		if (!weapon)
		{
			weapon = new GRSA_KitWeapon();
			weapon.m_iSlotIdx = slotIdx;
			m_aWeapons.Insert(weapon);
		}

		if (weapon.m_Prefab != prefab)
		{
			weapon.m_aAttachments.Clear();
			weapon.m_aAttachmentSlots.Clear();
		}
		weapon.m_Prefab = prefab;
	}

	//------------------------------------------------------------------------------------------------
	//! Extras are keyed by (prefab, container): the same item type can ride in several containers,
	//! an empty container is the automatic-placement bucket.
	GRSA_KitExtra FindExtra(ResourceName prefab, ResourceName container)
	{
		foreach (GRSA_KitExtra extra : m_aExtras)
		{
			if (extra.m_Prefab == prefab && extra.m_Container == container)
				return extra;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	void SetExtra(ResourceName prefab, int count, ResourceName container = ResourceName.Empty)
	{
		GRSA_KitExtra extra = FindExtra(prefab, container);
		if (extra)
		{
			if (count <= 0)
				m_aExtras.RemoveItem(extra);
			else
				extra.m_iCount = count;
			return;
		}

		if (count <= 0)
			return;

		GRSA_KitExtra newExtra = new GRSA_KitExtra();
		newExtra.m_Prefab = prefab;
		newExtra.m_iCount = count;
		newExtra.m_Container = container;
		m_aExtras.Insert(newExtra);
	}

	//------------------------------------------------------------------------------------------------
	int GetExtraCount(ResourceName prefab, ResourceName container)
	{
		GRSA_KitExtra extra = FindExtra(prefab, container);
		if (extra)
			return extra.m_iCount;
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	int GetExtraTotal(ResourceName prefab)
	{
		int total = 0;
		foreach (GRSA_KitExtra extra : m_aExtras)
		{
			if (extra.m_Prefab == prefab)
				total += extra.m_iCount;
		}
		return total;
	}

	//------------------------------------------------------------------------------------------------
	GRSA_Kit DeepCopy()
	{
		GRSA_Kit copy = new GRSA_Kit();
		string json = ToJson();
		if (!json.IsEmpty() && copy.FromJson(json))
			return copy;
		return new GRSA_Kit();
	}

	//------------------------------------------------------------------------------------------------
	string ToJson()
	{
		JsonSaveContext ctx = new JsonSaveContext();
		if (!WriteTo(ctx))
			return string.Empty;
		return ctx.SaveToString();
	}

	//------------------------------------------------------------------------------------------------
	bool FromJson(string json)
	{
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(json))
			return false;
		return ReadFrom(ctx);
	}

	//------------------------------------------------------------------------------------------------
	bool WriteTo(notnull JsonSaveContext ctx)
	{
		ctx.WriteValue("version", m_iVersion);
		ctx.WriteValue("name", m_sName);
		ctx.WriteValue("factionKey", m_sFactionKey);
		ctx.WriteValue("savedAt", m_iSavedAtUnix);
		ctx.WriteValue("summaryWeapons", m_sSummaryWeapons);
		ctx.WriteValue("summaryClothing", m_sSummaryClothing);
		ctx.WriteValue("suppliesCost", m_fSuppliesCost);
		ctx.WriteValue("requiredRank", m_iRequiredRank);

		int clothingCount = m_aClothings.Count();
		ctx.StartArray("clothings", clothingCount);
		foreach (GRSA_KitClothing clothing : m_aClothings)
		{
			ctx.StartObject();
			ctx.WriteValue("slot", clothing.m_iSlotIdx);
			ctx.WriteValue("prefab", clothing.m_Prefab);
			ctx.EndObject();
		}
		ctx.EndArray();

		int weaponCount = m_aWeapons.Count();
		ctx.StartArray("weapons", weaponCount);
		foreach (GRSA_KitWeapon weapon : m_aWeapons)
		{
			ctx.StartObject();
			ctx.WriteValue("slot", weapon.m_iSlotIdx);
			ctx.WriteValue("prefab", weapon.m_Prefab);
			weapon.EnsurePins();
			int attachmentCount = weapon.m_aAttachments.Count();
			ctx.StartArray("attachments", attachmentCount);
			foreach (int aIdx, ResourceName attachment : weapon.m_aAttachments)
			{
				ctx.StartObject();
				ctx.WriteValue("prefab", attachment);
				if (weapon.m_aAttachmentSlots[aIdx] >= 0)
					ctx.WriteValue("slot", weapon.m_aAttachmentSlots[aIdx]);
				ctx.EndObject();
			}
			ctx.EndArray();
			ctx.EndObject();
		}
		ctx.EndArray();

		int extraCount = m_aExtras.Count();
		ctx.StartArray("extras", extraCount);
		foreach (GRSA_KitExtra extra : m_aExtras)
		{
			ctx.StartObject();
			ctx.WriteValue("prefab", extra.m_Prefab);
			ctx.WriteValue("count", extra.m_iCount);
			if (!extra.m_Container.IsEmpty())
				ctx.WriteValue("container", extra.m_Container);
			ctx.EndObject();
		}
		ctx.EndArray();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool ReadFrom(notnull JsonLoadContext ctx)
	{
		int version = 0;
		if (!ctx.ReadValue("version", version) || version <= 0 || version > CURRENT_VERSION)
			return false;

		m_iVersion = version;
		ctx.ReadValue("name", m_sName);
		ctx.ReadValue("factionKey", m_sFactionKey);
		ctx.ReadValue("savedAt", m_iSavedAtUnix);
		ctx.ReadValue("summaryWeapons", m_sSummaryWeapons);
		ctx.ReadValue("summaryClothing", m_sSummaryClothing);
		ctx.ReadValue("suppliesCost", m_fSuppliesCost);
		ctx.ReadValue("requiredRank", m_iRequiredRank);

		m_aClothings.Clear();
		int clothingCount = 0;
		if (ctx.StartArray("clothings", clothingCount))
		{
			for (int i = 0; i < clothingCount; ++i)
			{
				if (!ctx.StartObject())
					continue;

				GRSA_KitClothing clothing = new GRSA_KitClothing();
				ctx.ReadValue("slot", clothing.m_iSlotIdx);
				ctx.ReadValue("prefab", clothing.m_Prefab);
				ctx.EndObject();

				if (!clothing.m_Prefab.IsEmpty())
					m_aClothings.Insert(clothing);
			}
			ctx.EndArray();
		}

		m_aWeapons.Clear();
		int weaponCount = 0;
		if (ctx.StartArray("weapons", weaponCount))
		{
			for (int i = 0; i < weaponCount; ++i)
			{
				if (!ctx.StartObject())
					continue;

				GRSA_KitWeapon weapon = new GRSA_KitWeapon();
				ctx.ReadValue("slot", weapon.m_iSlotIdx);
				ctx.ReadValue("prefab", weapon.m_Prefab);

				int attachmentCount = 0;
				if (ctx.StartArray("attachments", attachmentCount))
				{
					for (int a = 0; a < attachmentCount; ++a)
					{
						if (!ctx.StartObject())
							continue;

						ResourceName attachment;
						ctx.ReadValue("prefab", attachment);
						int pinnedSlot = -1;
						ctx.ReadValue("slot", pinnedSlot);
						ctx.EndObject();

						if (!attachment.IsEmpty())
						{
							weapon.m_aAttachments.Insert(attachment);
							weapon.m_aAttachmentSlots.Insert(pinnedSlot);
						}
					}
					ctx.EndArray();
				}
				ctx.EndObject();

				if (!weapon.m_Prefab.IsEmpty())
					m_aWeapons.Insert(weapon);
			}
			ctx.EndArray();
		}

		m_aExtras.Clear();
		int extraCount = 0;
		if (ctx.StartArray("extras", extraCount))
		{
			for (int i = 0; i < extraCount; ++i)
			{
				if (!ctx.StartObject())
					continue;

				GRSA_KitExtra extra = new GRSA_KitExtra();
				ctx.ReadValue("prefab", extra.m_Prefab);
				ctx.ReadValue("count", extra.m_iCount);
				ctx.ReadValue("container", extra.m_Container);
				ctx.EndObject();

				if (!extra.m_Prefab.IsEmpty() && extra.m_iCount > 0)
					m_aExtras.Insert(extra);
			}
			ctx.EndArray();
		}

		return true;
	}
}
