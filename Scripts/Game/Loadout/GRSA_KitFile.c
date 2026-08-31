//! Kit wire format. Version 2 slotData is byte-compatible with the established save-mod format so
//! players' existing kits import directly. Armory extensions (per-node bare attachments, extras,
//! display sidecars) are additive keys that legacy readers simply never read.
class GRSA_KitFile
{
	static const int CURRENT_VERSION = 2;
	static const int FACTION_STAMP_LEGACY = 0;
	static const int FACTION_STAMP_PLAYER = 1;

	int m_iVersion;
	string m_sName;
	string m_sKit;
	int m_iSavedAtUnix;
	int m_iCreatedAtUnix;
	float m_fTotalWeight;
	string m_sServerName;
	string m_sFactionKey;
	int m_iFactionStampVersion;
	int m_iRequiredRank;
	float m_fSupplyCost;

	protected ref array<string> m_aSlotKeys = {};
	protected ref array<string> m_aSlotPrefabs = {};

	protected ref array<string> m_aCaptureKeys = {};
	protected ref array<IEntity> m_aCaptureEntities = {};

	protected ref array<string> m_aOverrideKeys = {};
	protected ref array<string> m_aOverridePrefabs = {};
	protected ref array<ref array<ResourceName>> m_aOverrideAttachments = {};
	protected ref array<ref array<int>> m_aOverrideAttachmentSlots = {};
	protected ref array<bool> m_aOverrideFresh = {};

	ref array<ref GRSA_KitExtra> m_aExtras = {};

	protected string m_sRawJson;

	//------------------------------------------------------------------------------------------------
	void GRSA_KitFile()
	{
		m_iVersion = CURRENT_VERSION;
	}

	//------------------------------------------------------------------------------------------------
	void StampSavedNow()
	{
		m_iSavedAtUnix = System.GetUnixTime();
		if (m_iCreatedAtUnix <= 0)
			m_iCreatedAtUnix = m_iSavedAtUnix;
	}

	//------------------------------------------------------------------------------------------------
	void StampFaction(string factionKey)
	{
		m_sFactionKey = factionKey;
		m_iFactionStampVersion = FACTION_STAMP_PLAYER;
	}

	//------------------------------------------------------------------------------------------------
	string GetRawJson()
	{
		return m_sRawJson;
	}

	//------------------------------------------------------------------------------------------------
	bool IsEmpty()
	{
		return m_aSlotKeys.IsEmpty() && m_aCaptureKeys.IsEmpty() && m_aOverrideKeys.IsEmpty() && m_aExtras.IsEmpty() && m_sRawJson.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	int GetSlotCount()
	{
		return m_aSlotKeys.Count();
	}

	//------------------------------------------------------------------------------------------------
	string GetSlotKeyAt(int i)
	{
		return m_aSlotKeys[i];
	}

	//------------------------------------------------------------------------------------------------
	string GetSlotPrefabAt(int i)
	{
		return m_aSlotPrefabs[i];
	}

	//------------------------------------------------------------------------------------------------
	string GetSlotPrefab(string key)
	{
		int idx = m_aSlotKeys.Find(key);
		if (idx < 0)
			return string.Empty;
		return m_aSlotPrefabs[idx];
	}

	//------------------------------------------------------------------------------------------------
	//! Full-fidelity capture channel, entity subtrees are serialized through the bridge at export.
	void QueueCaptureSlot(string slotKey, IEntity entity)
	{
		if (slotKey.IsEmpty() || !entity)
			return;

		m_aCaptureKeys.Insert(slotKey);
		m_aCaptureEntities.Insert(entity);
		SetSlotShadow(slotKey, SCR_ResourceNameUtils.GetPrefabName(entity));
	}

	//------------------------------------------------------------------------------------------------
	//! Draft channel, a bare prefab node with optional attachment prefabs spawned with defaults.
	//! attachmentSlots runs parallel to attachments (-1 or missing = automatic placement).
	void SetOverrideSlot(string slotKey, string prefab, array<ResourceName> attachments = null, bool forceReplace = false, array<int> attachmentSlots = null)
	{
		if (slotKey.IsEmpty())
			return;

		ref array<ResourceName> attachmentCopy = {};
		ref array<int> slotCopy = {};
		if (attachments)
		{
			foreach (int i, ResourceName attachment : attachments)
			{
				attachmentCopy.Insert(attachment);
				if (attachmentSlots && i < attachmentSlots.Count())
					slotCopy.Insert(attachmentSlots[i]);
				else
					slotCopy.Insert(-1);
			}
		}

		int idx = m_aOverrideKeys.Find(slotKey);
		if (idx >= 0)
		{
			m_aOverridePrefabs[idx] = prefab;
			m_aOverrideAttachments[idx] = attachmentCopy;
			m_aOverrideAttachmentSlots[idx] = slotCopy;
			m_aOverrideFresh[idx] = forceReplace;
		}
		else
		{
			m_aOverrideKeys.Insert(slotKey);
			m_aOverridePrefabs.Insert(prefab);
			m_aOverrideAttachments.Insert(attachmentCopy);
			m_aOverrideAttachmentSlots.Insert(slotCopy);
			m_aOverrideFresh.Insert(forceReplace);
		}
		SetSlotShadow(slotKey, prefab);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetSlotShadow(string key, string prefab)
	{
		int idx = m_aSlotKeys.Find(key);
		if (idx >= 0)
		{
			m_aSlotPrefabs[idx] = prefab;
			return;
		}
		m_aSlotKeys.Insert(key);
		m_aSlotPrefabs.Insert(prefab);
	}

	//------------------------------------------------------------------------------------------------
	//! Every prefab referenced by the kit, for cost, rank and availability checks.
	void CollectItemPrefabs(notnull out array<ResourceName> outPrefabs)
	{
		outPrefabs.Clear();

		foreach (string prefab : m_aSlotPrefabs)
		{
			if (!prefab.IsEmpty())
				outPrefabs.Insert(prefab);
		}

		foreach (array<ResourceName> attachments : m_aOverrideAttachments)
		{
			foreach (ResourceName attachment : attachments)
			{
				if (!attachment.IsEmpty())
					outPrefabs.Insert(attachment);
			}
		}

		foreach (GRSA_KitExtra extra : m_aExtras)
		{
			for (int n = 0; n < extra.m_iCount; ++n)
			{
				outPrefabs.Insert(extra.m_Prefab);
			}
		}

		if (!m_sRawJson.IsEmpty())
			CollectNestedPrefabs(outPrefabs);
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectNestedPrefabs(notnull array<ResourceName> outPrefabs)
	{
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(m_sRawJson))
			return;

		int slotCount = 0;
		if (!ctx.StartMap("slotData", slotCount))
			return;

		for (int i = 0; i < slotCount; i++)
		{
			string slotKey;
			if (!ctx.ReadMapKey(i, slotKey))
				continue;
			if (!ctx.StartObject(slotKey))
				continue;

			ResourceName prefab;
			ctx.ReadValue("prefab", prefab);
			CollectStoragePrefabs(ctx, outPrefabs);
			ctx.EndObject();
		}
		ctx.EndMap();
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectStoragePrefabs(notnull JsonLoadContext ctx, notnull array<ResourceName> outPrefabs)
	{
		int storageCount = 0;
		if (!ctx.StartArray("storages", storageCount))
			return;

		for (int s = 0; s < storageCount; s++)
		{
			if (!ctx.StartObject())
				continue;

			string id;
			ctx.ReadValue("id", id);

			int slotCount = 0;
			if (ctx.StartMap("slots", slotCount))
			{
				for (int i = 0; i < slotCount; i++)
				{
					string idxStr;
					if (!ctx.ReadMapKey(i, idxStr))
						continue;
					if (!ctx.StartObject(idxStr))
						continue;

					ResourceName prefab;
					if (ctx.ReadValue("prefab", prefab) && !prefab.IsEmpty())
						outPrefabs.Insert(prefab);

					CollectStoragePrefabs(ctx, outPrefabs);
					ctx.EndObject();
				}
				ctx.EndMap();
			}
			ctx.EndObject();
		}
		ctx.EndArray();
	}

	//------------------------------------------------------------------------------------------------
	//! Top-level attachment prefabs for one slot, from the draft channel's bare list or the capture
	//! channel's weapon attachment storage, so previews can dress variants with their parts.
	//! outSlots (optional) receives the parallel pinned hardpoint indices, -1 = automatic.
	void GetSlotAttachments(string slotKey, notnull array<ResourceName> outAttachments, array<int> outSlots = null)
	{
		outAttachments.Clear();
		if (outSlots)
			outSlots.Clear();
		if (slotKey.IsEmpty())
			return;

		int overrideIdx = m_aOverrideKeys.Find(slotKey);
		if (overrideIdx >= 0)
		{
			array<int> overrideSlots = m_aOverrideAttachmentSlots[overrideIdx];
			foreach (int i, ResourceName attachment : m_aOverrideAttachments[overrideIdx])
			{
				outAttachments.Insert(attachment);
				if (outSlots)
				{
					if (overrideSlots && i < overrideSlots.Count())
						outSlots.Insert(overrideSlots[i]);
					else
						outSlots.Insert(-1);
				}
			}
			return;
		}

		if (m_sRawJson.IsEmpty())
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(m_sRawJson))
			return;

		int slotCount = 0;
		if (!ctx.StartMap("slotData", slotCount))
			return;

		for (int i = 0; i < slotCount; i++)
		{
			string key;
			if (!ctx.ReadMapKey(i, key))
				continue;
			if (!ctx.StartObject(key))
				continue;

			if (key == slotKey)
				ReadNodeAttachments(ctx, outAttachments, outSlots);

			ctx.EndObject();
		}
		ctx.EndMap();
	}

	//------------------------------------------------------------------------------------------------
	protected void ReadNodeAttachments(notnull JsonLoadContext ctx, notnull array<ResourceName> outAttachments, array<int> outSlots = null)
	{
		int attachmentCount = 0;
		if (ctx.StartArray("attachments", attachmentCount))
		{
			for (int i = 0; i < attachmentCount; i++)
			{
				if (!ctx.StartObject())
					continue;

				ResourceName prefab;
				ctx.ReadValue("prefab", prefab);
				int pinnedSlot = -1;
				ctx.ReadValue("slot", pinnedSlot);
				ctx.EndObject();

				if (!prefab.IsEmpty())
				{
					outAttachments.Insert(prefab);
					if (outSlots)
						outSlots.Insert(pinnedSlot);
				}
			}
			ctx.EndArray();

			if (!outAttachments.IsEmpty())
				return;
		}

		int storageCount = 0;
		if (!ctx.StartArray("storages", storageCount))
			return;

		for (int s = 0; s < storageCount; s++)
		{
			if (!ctx.StartObject())
				continue;

			string id;
			ctx.ReadValue("id", id);
			bool attachmentStorage = id.IndexOf("WeaponAttachmentsStorageComponent") == 0;

			int slotsCount = 0;
			if (ctx.StartMap("slots", slotsCount))
			{
				for (int i = 0; i < slotsCount; i++)
				{
					string idxStr;
					if (!ctx.ReadMapKey(i, idxStr))
						continue;
					if (!ctx.StartObject(idxStr))
						continue;

					ResourceName prefab;
					ctx.ReadValue("prefab", prefab);
					if (attachmentStorage && !prefab.IsEmpty())
					{
						outAttachments.Insert(prefab);
						if (outSlots)
							outSlots.Insert(-1);
					}

					GRSA_LoadoutBridge.SkipStoragesArray(ctx);
					ctx.EndObject();
				}
				ctx.EndMap();
			}
			ctx.EndObject();
		}
		ctx.EndArray();
	}

	//------------------------------------------------------------------------------------------------
	string ExportToString()
	{
		JsonSaveContext ctx = new JsonSaveContext();
		if (!Serialize(ctx))
			return string.Empty;

		string result = ctx.SaveToString();
		m_sRawJson = result;
		return result;
	}

	//------------------------------------------------------------------------------------------------
	bool ImportFromString(string json)
	{
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(json))
			return false;

		if (!Deserialize(ctx))
			return false;

		m_sRawJson = json;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Serialize(notnull JsonSaveContext ctx)
	{
		ctx.WriteValue("version", CURRENT_VERSION);
		ctx.WriteValue("name", m_sName);
		ctx.WriteValue("kit", m_sKit);
		ctx.WriteValue("savedAt", m_iSavedAtUnix);
		ctx.WriteValue("createdAt", m_iCreatedAtUnix);
		ctx.WriteValue("totalWeight", m_fTotalWeight);
		ctx.WriteValue("serverName", m_sServerName);
		ctx.WriteValue("factionKey", m_sFactionKey);
		ctx.WriteValue("factionStamp", m_iFactionStampVersion);
		ctx.WriteValue("requiredRank", m_iRequiredRank);
		ctx.WriteValue("supplyCost", m_fSupplyCost);

		if (!m_aCaptureKeys.IsEmpty() || !m_aOverrideKeys.IsEmpty())
			SerializeSlotData(ctx);
		else if (!m_sRawJson.IsEmpty())
			SerializeFromRaw(ctx);
		else
		{
			int emptyCount = 0;
			ctx.StartMap("slotData", emptyCount);
			ctx.EndMap();
		}

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
	protected void SerializeSlotData(notnull JsonSaveContext ctx)
	{
		int writes = 0;
		for (int i = 0; i < m_aCaptureKeys.Count(); i++)
		{
			if (!m_aCaptureKeys[i].IsEmpty() && m_aCaptureEntities[i] && m_aOverrideKeys.Find(m_aCaptureKeys[i]) < 0)
				writes++;
		}
		for (int i = 0; i < m_aOverrideKeys.Count(); i++)
		{
			if (!m_aOverridePrefabs[i].IsEmpty())
				writes++;
		}

		ctx.StartMap("slotData", writes);
		for (int i = 0; i < m_aCaptureKeys.Count(); i++)
		{
			string slotKey = m_aCaptureKeys[i];
			IEntity entity = m_aCaptureEntities[i];
			if (slotKey.IsEmpty() || !entity)
				continue;
			if (m_aOverrideKeys.Find(slotKey) >= 0)
				continue;

			ctx.WriteMapKey(slotKey);
			ctx.StartObject();
			GRSA_LoadoutBridge.ReadEntity(entity, ctx);
			ctx.EndObject();
		}
		for (int i = 0; i < m_aOverrideKeys.Count(); i++)
		{
			if (m_aOverridePrefabs[i].IsEmpty())
				continue;

			ctx.WriteMapKey(m_aOverrideKeys[i]);
			ctx.StartObject();
			ctx.WriteValue("prefab", m_aOverridePrefabs[i]);
			if (m_aOverrideFresh[i])
				ctx.WriteValue("fresh", 1);
			array<ResourceName> attachments = m_aOverrideAttachments[i];
			array<int> attachmentSlots = m_aOverrideAttachmentSlots[i];
			int attachmentCount = attachments.Count();
			if (attachmentCount > 0)
			{
				ctx.StartArray("attachments", attachmentCount);
				foreach (int aIdx, ResourceName attachment : attachments)
				{
					ctx.StartObject();
					ctx.WriteValue("prefab", attachment);
					if (attachmentSlots && aIdx < attachmentSlots.Count() && attachmentSlots[aIdx] >= 0)
						ctx.WriteValue("slot", attachmentSlots[aIdx]);
					ctx.EndObject();
				}
				ctx.EndArray();
			}
			ctx.EndObject();
		}
		ctx.EndMap();
	}

	//------------------------------------------------------------------------------------------------
	protected void SerializeFromRaw(notnull JsonSaveContext ctx)
	{
		JsonLoadContext loadCtx = new JsonLoadContext();
		int emptyCount = 0;
		if (!loadCtx.LoadFromString(m_sRawJson))
		{
			ctx.StartMap("slotData", emptyCount);
			ctx.EndMap();
			return;
		}

		int slotCount = 0;
		if (!loadCtx.StartMap("slotData", slotCount))
		{
			ctx.StartMap("slotData", emptyCount);
			ctx.EndMap();
			return;
		}

		ctx.StartMap("slotData", slotCount);
		for (int i = 0; i < slotCount; i++)
		{
			string slotKey;
			if (!loadCtx.ReadMapKey(i, slotKey))
				continue;
			if (!loadCtx.StartObject(slotKey))
				continue;

			ctx.WriteMapKey(slotKey);
			ctx.StartObject();
			MirrorEntitySubtree(loadCtx, ctx);
			ctx.EndObject();
			loadCtx.EndObject();
		}
		ctx.EndMap();
		loadCtx.EndMap();
	}

	//------------------------------------------------------------------------------------------------
	protected void MirrorEntitySubtree(notnull JsonLoadContext loadCtx, notnull JsonSaveContext ctx)
	{
		ResourceName prefab;
		loadCtx.ReadValue("prefab", prefab);
		ctx.WriteValue("prefab", prefab);

		int fresh = 0;
		if (loadCtx.ReadValue("fresh", fresh) && fresh != 0)
			ctx.WriteValue("fresh", fresh);

		int attachmentCount = 0;
		if (loadCtx.StartArray("attachments", attachmentCount))
		{
			ctx.StartArray("attachments", attachmentCount);
			for (int a = 0; a < attachmentCount; a++)
			{
				if (!loadCtx.StartObject())
					continue;
				ctx.StartObject();

				ResourceName attachment;
				loadCtx.ReadValue("prefab", attachment);
				ctx.WriteValue("prefab", attachment);

				int pinnedSlot = -1;
				if (loadCtx.ReadValue("slot", pinnedSlot) && pinnedSlot >= 0)
					ctx.WriteValue("slot", pinnedSlot);

				ctx.EndObject();
				loadCtx.EndObject();
			}
			ctx.EndArray();
			loadCtx.EndArray();
		}

		int storageCount = 0;
		if (!loadCtx.StartArray("storages", storageCount))
			return;

		ctx.StartArray("storages", storageCount);
		for (int s = 0; s < storageCount; s++)
		{
			if (!loadCtx.StartObject())
				continue;
			ctx.StartObject();

			string id;
			loadCtx.ReadValue("id", id);
			ctx.WriteValue("id", id);

			int slotCount = 0;
			if (loadCtx.StartMap("slots", slotCount))
			{
				ctx.StartMap("slots", slotCount);
				for (int sl = 0; sl < slotCount; sl++)
				{
					string idxStr;
					if (!loadCtx.ReadMapKey(sl, idxStr))
						continue;
					if (!loadCtx.StartObject(idxStr))
						continue;

					ctx.WriteMapKey(idxStr);
					ctx.StartObject();
					MirrorEntitySubtree(loadCtx, ctx);
					ctx.EndObject();
					loadCtx.EndObject();
				}
				ctx.EndMap();
				loadCtx.EndMap();
			}
			ctx.EndObject();
			loadCtx.EndObject();
		}
		ctx.EndArray();
		loadCtx.EndArray();
	}

	//------------------------------------------------------------------------------------------------
	protected bool Deserialize(notnull JsonLoadContext ctx)
	{
		int version = 0;
		if (!ctx.ReadValue("version", version) || version <= 0)
			return false;

		m_iVersion = version;
		ctx.ReadValue("name", m_sName);
		ctx.ReadValue("kit", m_sKit);
		m_iSavedAtUnix = 0;
		ctx.ReadValue("savedAt", m_iSavedAtUnix);
		m_iCreatedAtUnix = 0;
		ctx.ReadValue("createdAt", m_iCreatedAtUnix);
		if (m_iCreatedAtUnix <= 0)
			m_iCreatedAtUnix = m_iSavedAtUnix;
		m_fTotalWeight = 0;
		ctx.ReadValue("totalWeight", m_fTotalWeight);
		m_sServerName = string.Empty;
		ctx.ReadValue("serverName", m_sServerName);
		m_sFactionKey = string.Empty;
		ctx.ReadValue("factionKey", m_sFactionKey);
		m_iFactionStampVersion = FACTION_STAMP_LEGACY;
		ctx.ReadValue("factionStamp", m_iFactionStampVersion);
		m_iRequiredRank = 0;
		ctx.ReadValue("requiredRank", m_iRequiredRank);
		m_fSupplyCost = 0;
		ctx.ReadValue("supplyCost", m_fSupplyCost);

		m_aSlotKeys.Clear();
		m_aSlotPrefabs.Clear();
		m_aCaptureKeys.Clear();
		m_aCaptureEntities.Clear();
		m_aOverrideKeys.Clear();
		m_aOverridePrefabs.Clear();
		m_aOverrideAttachments.Clear();
		m_aOverrideAttachmentSlots.Clear();
		m_aOverrideFresh.Clear();

		int slotCount = 0;
		if (ctx.StartMap("slotData", slotCount))
		{
			for (int i = 0; i < slotCount; i++)
			{
				string slotKey;
				if (!ctx.ReadMapKey(i, slotKey))
					continue;
				if (!ctx.StartObject(slotKey))
					continue;

				ResourceName prefab;
				ctx.ReadValue("prefab", prefab);
				if (!slotKey.IsEmpty() && !prefab.IsEmpty())
					SetSlotShadow(slotKey, prefab);

				GRSA_LoadoutBridge.SkipStoragesArray(ctx);
				ctx.EndObject();
			}
			ctx.EndMap();
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
