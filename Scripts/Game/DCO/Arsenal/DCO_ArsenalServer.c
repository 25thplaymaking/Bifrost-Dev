// Bifrost Arsenal - server-authoritative gear verbs.

// OWNING-CLIENT core for the arsenal PLAYER leg-lock.
class DCO_ArsenalLegLock
{
	protected static bool s_bLocked;

	static void Apply(bool on)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		IEntity body = pc.GetControlledEntity();
		if (!body)
		{
			s_bLocked = false;
			return;
		}
		CharacterControllerComponent ctrl = CharacterControllerComponent.Cast(body.FindComponent(CharacterControllerComponent));
		if (!ctrl)
			return;
		ctrl.SetDisableMovementControls(on);
		s_bLocked = on;
		GetGame().GetCallqueue().Remove(Deadman);
		if (on)
			GetGame().GetCallqueue().CallLater(Deadman, 900000, false);
		if (on)
			Print("[DCO-ARS] leg-lock ON (a GM is editing your loadout)", LogLevel.NORMAL);
		else
			Print("[DCO-ARS] leg-lock OFF", LogLevel.NORMAL);
	}

	protected static void Deadman()
	{
		if (!s_bLocked)
			return;
		Print("[DCO-ARS] leg-lock dead-man release: the arsenal session never closed", LogLevel.WARNING);
		Apply(false);
	}
}

class DCO_ArsenalServer
{
	static const int VERB_EQUIP  = 1;	// payload = prefab ResourceName; target = CHARACTER.
	static const int VERB_CLEAR  = 2;
	static const int VERB_RESET  = 3;	// payload unused; target = CHARACTER.
	static const int VERB_INSERT = 4;
	static const int VERB_HOLD    = 5;	// arsenal opened on target: AI holds position / player gets the leg-lock.
	static const int VERB_RELEASE = 6;	// arsenal closed: undo exactly what HOLD did.
	static const int VERB_REMOVE  = 7;
	static const int VERB_UNDO    = 8;	// target = CHARACTER; step back one arsenal edit.
	static const int VERB_REDO    = 9;	// target = CHARACTER; step forward again.
	static const int VERB_RESTOCK = 10;	// target = CHARACTER; refill every carried/loaded magazine.

	// First-edit snapshots for RESET: character -> loadout JSON captured before the first arsenal write.
	protected static ref map<IEntity, string> s_Snapshots = new map<IEntity, string>();

	static void Route(int verb, IEntity target, string payload)
	{
		if (!target)
			return;

		if (Replication.IsServer())
		{
			ApplyOn(target, verb, payload);
			return;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
		{
			Print("[DCO-ARS] no local player controller (cannot reach the server)", LogLevel.WARNING);
			return;
		}
		RplComponent rpl = RplComponent.Cast(target.FindComponent(RplComponent));
		if (!rpl || !rpl.Id().IsValid())
		{
			Print("[DCO-ARS] target has no valid replication id - verb skipped", LogLevel.WARNING);
			return;
		}
		pc.DCO_SendGMArsenal(verb, rpl.Id(), payload);
	}

	// Server side: resolve the wire id back to the entity and apply.
	static void Apply(int verb, RplId targetId, string payload)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(targetId));
		if (!rpl)
		{
			Print("[DCO-ARS] RplId did not resolve on the server - verb dropped", LogLevel.WARNING);
			return;
		}
		ApplyOn(rpl.GetEntity(), verb, payload);
	}

	static void ApplyOn(IEntity target, int verb, string payload)
	{
		if (!target)
			return;
		if (verb == VERB_INSERT)
		{
			InsertIntoItem(target, payload);	// target is the ITEM; root-character bookkeeping inside.
			return;
		}
		if (verb == VERB_HOLD)
		{
			HoldTarget(target);
			return;
		}
		if (verb == VERB_RELEASE)
		{
			ReleaseTarget(target);
			return;
		}
		if (verb == VERB_REMOVE)
		{
			RemoveFromScope(target, payload);
			return;
		}
		if (verb == VERB_UNDO)
		{
			UndoStep(target);
			return;
		}
		if (verb == VERB_REDO)
		{
			RedoStep(target);
			return;
		}
		if (DCO_PlayerUtil.IsPlayer(target))
			Print(string.Format("[DCO-ARS] GM gear verb %1 on a PLAYER character", verb), LogLevel.NORMAL);
		EnsureSnapshot(target);
		string preJson = SnapshotJson(target);
		switch (verb)
		{
			case VERB_EQUIP: { EquipPrefab(target, payload);           break; }
			case VERB_CLEAR: { ClearCategory(target, payload.ToInt()); break; }
			case VERB_RESET: { ResetToSnapshot(target);                break; }
			case VERB_RESTOCK: { RestockAmmo(target);                   break; }
			default:         { Print(string.Format("[DCO-ARS] unknown verb %1", verb), LogLevel.WARNING); break; }
		}
		if (!preJson.IsEmpty() && SnapshotJson(target) != preJson)
			CommitUndo(target, preJson);
	}

	// GM restock: top off every magazine the character still owns, including magazines loaded into weapons.
	protected static void RestockAmmo(IEntity target)
	{
		CharacterControllerComponent controller = CharacterControllerComponent.Cast(target.FindComponent(CharacterControllerComponent));
		if (!controller || controller.GetLifeState() == ECharacterLifeState.DEAD)
			return;

		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(target.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
		{
			Print("[DCO-ARS] restock: target has no inventory manager", LogLevel.WARNING);
			return;
		}

		array<IEntity> items = {};
		inv.GetItems(items);
		int filled;
		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
			if (magazine && magazine.GetAmmoCount() < magazine.GetMaxAmmoCount())
			{
				magazine.SetAmmoCount(magazine.GetMaxAmmoCount());
				filled++;
			}

			array<Managed> muzzles = {};
			item.FindComponents(BaseMuzzleComponent, muzzles);
			foreach (Managed managedMuzzle : muzzles)
			{
				BaseMuzzleComponent muzzle = BaseMuzzleComponent.Cast(managedMuzzle);
				if (!muzzle)
					continue;
				BaseMagazineComponent loaded = muzzle.GetMagazine();
				if (!loaded || loaded.GetAmmoCount() >= loaded.GetMaxAmmoCount())
					continue;
				loaded.SetAmmoCount(loaded.GetMaxAmmoCount());
				filled++;
			}
		}

		Print(string.Format("[DCO-ARS] restock: refilled %1 magazine(s)", filled), LogLevel.NORMAL);
	}

	// Undo/redo: a per-character history of whole-kit loadout snapshots.
	protected static ref map<IEntity, ref array<string>> s_Undo = new map<IEntity, ref array<string>>();
	protected static ref map<IEntity, ref array<string>> s_Redo = new map<IEntity, ref array<string>>();
	protected static const int UNDO_DEPTH = 20;

	protected static array<string> GetStack(map<IEntity, ref array<string>> store, IEntity target)
	{
		ref array<string> stack;
		if (!store.Find(target, stack))
		{
			stack = new array<string>();
			store.Insert(target, stack);
		}
		return stack;
	}

	protected static void CommitUndo(IEntity target, string preJson)
	{
		if (!target || preJson.IsEmpty())
			return;
		array<string> undo = GetStack(s_Undo, target);
		undo.Insert(preJson);
		if (undo.Count() > UNDO_DEPTH)
			undo.RemoveOrdered(0);
		GetStack(s_Redo, target).Clear();
	}

	static void PushUndo(IEntity target)
	{
		if (!target)
			return;
		string json = SnapshotJson(target);
		if (json.IsEmpty())
			return;
		array<string> undo = GetStack(s_Undo, target);
		undo.Insert(json);
		if (undo.Count() > UNDO_DEPTH)
			undo.RemoveOrdered(0);
		GetStack(s_Redo, target).Clear();
	}

	protected static void UndoStep(IEntity target)
	{
		array<string> undo = GetStack(s_Undo, target);
		if (undo.IsEmpty())
		{
			Print("[DCO-ARS] undo: nothing to undo", LogLevel.NORMAL);
			return;
		}
		string cur = SnapshotJson(target);
		string prev = undo[undo.Count() - 1];
		undo.RemoveOrdered(undo.Count() - 1);
		if (!cur.IsEmpty())
			GetStack(s_Redo, target).Insert(cur);
		if (ApplyJsonRaw(target, prev))
			Print(string.Format("[DCO-ARS] undo applied (%1 undo / %2 redo left)", undo.Count(), GetStack(s_Redo, target).Count()), LogLevel.NORMAL);
	}

	protected static void RedoStep(IEntity target)
	{
		array<string> redo = GetStack(s_Redo, target);
		if (redo.IsEmpty())
		{
			Print("[DCO-ARS] redo: nothing to redo", LogLevel.NORMAL);
			return;
		}
		string cur = SnapshotJson(target);
		string next = redo[redo.Count() - 1];
		redo.RemoveOrdered(redo.Count() - 1);
		if (!cur.IsEmpty())
			GetStack(s_Undo, target).Insert(cur);
		if (ApplyJsonRaw(target, next))
			Print(string.Format("[DCO-ARS] redo applied (%1 undo / %2 redo left)", GetStack(s_Undo, target).Count(), redo.Count()), LogLevel.NORMAL);
	}

	// Remove ONE carried/contained item matching the prefab.
	protected static void RemoveFromScope(IEntity target, string prefab)
	{
		if (prefab.IsEmpty())
			return;
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(target.FindComponent(SCR_InventoryStorageManagerComponent));
		array<IEntity> pool = {};
		if (inv)
		{
			EnsureSnapshot(target);
			PushUndo(target);
			inv.GetItems(pool);
		}
		else
		{
			IEntity root = target;
			int guard = 0;
			while (root.GetParent() && guard < 16)
			{
				root = root.GetParent();
				guard++;
			}
			inv = SCR_InventoryStorageManagerComponent.Cast(root.FindComponent(SCR_InventoryStorageManagerComponent));
			if (!inv)
			{
				Print("[DCO-ARS] remove: no inventory manager in reach", LogLevel.WARNING);
				return;
			}
			EnsureSnapshot(root);
			PushUndo(root);
			BaseInventoryStorageComponent st = WeaponAttachmentsStorageComponent.Cast(target.FindComponent(WeaponAttachmentsStorageComponent));
			if (!st)
				st = BaseInventoryStorageComponent.Cast(target.FindComponent(BaseInventoryStorageComponent));
			if (!st)
			{
				Print("[DCO-ARS] remove: target item has no storage", LogLevel.WARNING);
				return;
			}
			st.GetAll(pool);
		}
		foreach (IEntity it : pool)
		{
			if (!it || !it.GetPrefabData())
				continue;
			if (it.GetPrefabData().GetPrefabName() != prefab)
				continue;
			if (DCO_ArsenalCompat.IsIntegralAttachment(it))
			{
				Print(string.Format("[DCO-ARS] remove REFUSED (integral attachment): %1", prefab), LogLevel.WARNING);
				return;
			}
			if (inv.TryDeleteItem(it))
				Print(string.Format("[DCO-ARS] removed one: %1", prefab), LogLevel.NORMAL);
			else
				Print(string.Format("[DCO-ARS] remove FAILED: %1", prefab), LogLevel.WARNING);
			return;
		}
		Print(string.Format("[DCO-ARS] remove: none found in scope: %1", prefab), LogLevel.NORMAL);
	}

	protected static ref map<IEntity, int> s_Held = new map<IEntity, int>();	// entity -> hold sequence.
	protected static ref map<IEntity, bool> s_HeldPlayers = new map<IEntity, bool>();
	protected static int s_HoldSeq;

	protected static void HoldTarget(IEntity target)
	{
		if (s_Held.Contains(target))
			return;
		if (DCO_PlayerUtil.IsPlayer(target))
		{
			if (!SendPlayerLegLock(target, true))
				return;
			s_HoldSeq++;
			s_Held.Insert(target, s_HoldSeq);
			s_HeldPlayers.Insert(target, true);
			GetGame().GetCallqueue().CallLater(DeadmanRelease, 900000, false, target, s_HoldSeq);
			Print("[DCO-ARS] player leg-lock engaged (arsenal session)", LogLevel.NORMAL);
			return;
		}
		AIControlComponent ctrl = AIControlComponent.Cast(target.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.IsAIActivated())
			return;
		AIAgent agent = ctrl.GetAIAgent();
		if (agent)
			agent.SetPermanentLOD(AIAgent.GetMaxLOD());
		ctrl.DeactivateAI();
		s_HoldSeq++;
		s_Held.Insert(target, s_HoldSeq);
		GetGame().GetCallqueue().CallLater(DeadmanRelease, 900000, false, target, s_HoldSeq);
		Print("[DCO-ARS] target holding position (arsenal session)", LogLevel.NORMAL);
	}

	protected static void ReleaseTarget(IEntity target)
	{
		if (!target || !s_Held.Contains(target))
			return;
		s_Held.Remove(target);
		if (s_HeldPlayers.Contains(target))
		{
			// Player path: unlock on the owning client.
			s_HeldPlayers.Remove(target);
			SendPlayerLegLock(target, false);
			Print("[DCO-ARS] player leg-lock released (arsenal session ended)", LogLevel.NORMAL);
			return;
		}
		AIControlComponent ctrl = AIControlComponent.Cast(target.FindComponent(AIControlComponent));
		if (!ctrl)
			return;
		if (!ctrl.IsAIActivated())
			ctrl.ActivateAI();
		AIAgent agent = ctrl.GetAIAgent();
		if (agent)
			agent.SetPermanentLOD(-1);
		Print("[DCO-ARS] target released (arsenal session ended)", LogLevel.NORMAL);
	}

	// Reach the edited player's OWNING client with the lock flag.
	protected static bool SendPlayerLegLock(IEntity target, bool on)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return false;
		int pid = pm.GetPlayerIdFromControlledEntity(target);
		if (pid == 0)
			return false;	// disconnected between verbs - nothing to lock.
		SCR_PlayerController pc = SCR_PlayerController.Cast(pm.GetPlayerController(pid));
		if (!pc)
			return false;
		if (pc == GetGame().GetPlayerController())
			DCO_ArsenalLegLock.Apply(on);
		else
			pc.DCO_SendArsenalLegLock(on);
		return true;
	}

	protected static void DeadmanRelease(IEntity target, int seq)
	{
		int held;
		if (!target || !s_Held.Find(target, held) || held != seq)
			return;	// released and possibly re-held since - not ours to undo.
		Print("[DCO-ARS] dead-man release: arsenal hold outlived its session", LogLevel.WARNING);
		ReleaseTarget(target);
	}

	protected static bool PrefabKnown(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return false;
		SCR_EntityCatalogManagerComponent mgr = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!mgr)
			return false;
		return mgr.GetEntryWithPrefabFromAnyCatalog(EEntityCatalogType.ITEM, prefab) != null;
	}

	static string SnapshotJson(IEntity target)
	{
		GameEntity ge = GameEntity.Cast(target);
		if (!ge)
			return "";
		JsonSaveContext ctx = new JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(ge, ctx))
			return "";
		return ctx.SaveToString();
	}

	static void ApplyLoadoutJson(IEntity target, string json)
	{
		if (!target || json.IsEmpty())
			return;
		EnsureSnapshot(target);
		PushUndo(target);
		if (ApplyJsonRaw(target, json))
			Print("[DCO-ARS] loadout applied", LogLevel.NORMAL);
	}

	protected static bool ApplyJsonRaw(IEntity target, string json)
	{
		GameEntity ge = GameEntity.Cast(target);
		if (!ge || json.IsEmpty())
			return false;
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(json))
		{
			Print("[DCO-ARS] loadout apply: JSON failed to parse", LogLevel.WARNING);
			return false;
		}
		if (!SCR_PlayerArsenalLoadout.ApplyLoadoutString(ge, ctx))
			Print("[DCO-ARS] loadout applied with skipped items (missing/incompatible content)", LogLevel.WARNING);
		return true;
	}

	protected static void InsertIntoItem(IEntity itemEnt, string prefab)
	{
		if (!PrefabKnown(prefab))
		{
			Print(string.Format("[DCO-ARS] insert refused - prefab not in local catalogs: %1", prefab), LogLevel.WARNING);
			return;
		}
		IEntity root = itemEnt;
		int guard = 0;
		while (root.GetParent() && guard < 16)
		{
			root = root.GetParent();
			guard++;
		}
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(root.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
		{
			Print("[DCO-ARS] insert: no inventory manager on the item's root entity", LogLevel.WARNING);
			return;
		}
		EnsureSnapshot(root);
		PushUndo(root);

		WeaponAttachmentsStorageComponent weaponStorage = WeaponAttachmentsStorageComponent.Cast(itemEnt.FindComponent(WeaponAttachmentsStorageComponent));
		BaseInventoryStorageComponent storage = weaponStorage;
		if (!storage)
			storage = BaseInventoryStorageComponent.Cast(itemEnt.FindComponent(BaseInventoryStorageComponent));
		if (!storage)
		{
			Print("[DCO-ARS] insert: target item has no storage (not a weapon/container)", LogLevel.WARNING);
			return;
		}

		if (weaponStorage)
			EvictWeaponOccupant(itemEnt, inv, prefab);

		if (inv.TrySpawnPrefabToStorage(prefab, storage))
			Print(string.Format("[DCO-ARS] inserted into item: %1", prefab), LogLevel.NORMAL);
		else
			Print(string.Format("[DCO-ARS] insert FAILED (incompatible or full): %1", prefab), LogLevel.WARNING);
	}

	protected static void EvictWeaponOccupant(IEntity weapon, SCR_InventoryStorageManagerComponent inv, string prefab)
	{
		if (DCO_ArsenalCompat.IsMagazinePrefab(prefab))
		{
			array<Managed> muzzles = {};
			weapon.FindComponents(BaseMuzzleComponent, muzzles);
			foreach (Managed m : muzzles)
			{
				BaseMuzzleComponent muzzle = BaseMuzzleComponent.Cast(m);
				if (!muzzle)
					continue;
				BaseMagazineComponent loadedMag = muzzle.GetMagazine();
				if (!loadedMag)
					continue;
				IEntity loaded = loadedMag.GetOwner();
				if (loaded && inv.TryDeleteItem(loaded))
					Print("[DCO-ARS] swap: unloaded the current magazine", LogLevel.NORMAL);
			}
			return;
		}
		typename incoming = DCO_ArsenalCompat.AttachTypeOfPrefab(prefab);
		if (!incoming)
			return;
		array<Managed> slots = {};
		weapon.FindComponents(AttachmentSlotComponent, slots);
		foreach (Managed s : slots)
		{
			AttachmentSlotComponent slot = AttachmentSlotComponent.Cast(s);
			if (!slot)
				continue;
			if (!slot.ShouldShowInInspection())
				continue;
			BaseAttachmentType slotType = slot.GetAttachmentSlotType();
			if (!slotType || !incoming.IsInherited(slotType.Type()))
				continue;
			IEntity occupant = slot.GetAttachedEntity();
			if (occupant && inv.TryDeleteItem(occupant))
				Print("[DCO-ARS] swap: removed the occupying attachment", LogLevel.NORMAL);
			return;	// one matching slot handled - the insert lands in it.
		}
	}

	// Capture the character's pre-edit kit once, so RESET always means "as it was before the GM touched it".
	protected static void EnsureSnapshot(IEntity target)
	{
		if (s_Snapshots.Contains(target))
			return;
		string json = SnapshotJson(target);
		if (!json.IsEmpty())
			s_Snapshots.Set(target, json);
		else
			Print("[DCO-ARS] snapshot: ReadLoadoutString failed (RESET unavailable for this unit)", LogLevel.WARNING);
	}

	protected static void EquipPrefab(IEntity target, string prefab)
	{
		if (!PrefabKnown(prefab))
		{
			Print(string.Format("[DCO-ARS] equip refused - prefab not in local catalogs: %1", prefab), LogLevel.WARNING);
			return;
		}
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(target.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
		{
			Print("[DCO-ARS] equip: target has no inventory manager", LogLevel.WARNING);
			return;
		}

		EDCO_ArsenalCategory cat = CategoryOfPrefab(prefab);
		if (cat == EDCO_ArsenalCategory.PRIMARY || cat == EDCO_ArsenalCategory.PISTOL || cat == EDCO_ArsenalCategory.LAUNCHER)
		{
			ClearCategory(target, cat);
			EquipedWeaponStorageComponent weaponStorage = EquipedWeaponStorageComponent.Cast(target.FindComponent(EquipedWeaponStorageComponent));
			bool spawned = false;
			if (weaponStorage && inv.TrySpawnPrefabToStorage(prefab, weaponStorage))
			{
				spawned = true;
				Print(string.Format("[DCO-ARS] weapon equipped: %1", prefab), LogLevel.NORMAL);
			}
			else if (inv.TrySpawnPrefabToStorage(prefab))
			{
				spawned = true;
				Print(string.Format("[DCO-ARS] weapon equipped via auto-slot: %1", prefab), LogLevel.NORMAL);
			}
			if (spawned)
			{
				GetGame().GetCallqueue().CallLater(DrawWeapon, 150, false, target, prefab);
				return;
			}
			Print(string.Format("[DCO-ARS] weapon equip FAILED: %1", prefab), LogLevel.WARNING);
			return;
		}

		if (cat == EDCO_ArsenalCategory.UNIFORM || cat == EDCO_ArsenalCategory.VEST || cat == EDCO_ArsenalCategory.BACKPACK || cat == EDCO_ArsenalCategory.HEADGEAR)
		{
			SCR_EArsenalItemType t;
			SCR_EArsenalItemMode m;
			if (TypeOfPrefab(prefab, t, m))
				ClearByType(target, t);
		}

		if (inv.TrySpawnPrefabToStorage(prefab))
			Print(string.Format("[DCO-ARS] item equipped: %1", prefab), LogLevel.NORMAL);
		else
			Print(string.Format("[DCO-ARS] item equip FAILED (no space/slot?): %1", prefab), LogLevel.WARNING);
	}

	protected static bool TypeOfPrefab(ResourceName prefab, out SCR_EArsenalItemType t, out SCR_EArsenalItemMode m)
	{
		SCR_EntityCatalogManagerComponent mgr = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!mgr)
			return false;
		SCR_EntityCatalogEntry entry = mgr.GetEntryWithPrefabFromAnyCatalog(EEntityCatalogType.ITEM, prefab);
		if (!entry)
			return false;
		SCR_ArsenalItem data = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
		if (!data)
			return false;
		t = data.GetItemType();
		m = data.GetItemMode();
		return true;
	}

	protected static void ClearByType(IEntity target, SCR_EArsenalItemType type)
	{
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(target.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return;
		array<IEntity> items = {};
		inv.GetItems(items);
		int removed = 0;
		foreach (IEntity item : items)
		{
			if (!item || !item.GetPrefabData())
				continue;
			ResourceName p = item.GetPrefabData().GetPrefabName();
			if (p.IsEmpty())
				continue;
			SCR_EArsenalItemType it;
			SCR_EArsenalItemMode im;
			if (!TypeOfPrefab(p, it, im))
				continue;
			if (it != type || im == SCR_EArsenalItemMode.AMMUNITION)
				continue;	// never touch magazines here - only the worn piece itself.
			if (inv.TryDeleteItem(item))
				removed++;
		}
		Print(string.Format("[DCO-ARS] swap: removed %1 item(s) of type %2", removed, type), LogLevel.NORMAL);
	}

	protected static void DrawWeapon(IEntity target, string prefab)
	{
		if (!target)
			return;
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(target.FindComponent(SCR_InventoryStorageManagerComponent));
		CharacterControllerComponent controller = CharacterControllerComponent.Cast(target.FindComponent(CharacterControllerComponent));
		if (!inv || !controller)
			return;
		array<IEntity> items = {};
		inv.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!item || !item.GetPrefabData())
				continue;
			if (item.GetPrefabData().GetPrefabName() != prefab)
				continue;
			if (controller.TryEquipRightHandItem(item, EEquipItemType.EEquipTypeWeapon, false))
				Print("[DCO-ARS] weapon draw ordered", LogLevel.NORMAL);
			else
				Print("[DCO-ARS] weapon draw refused by controller (stance/state?)", LogLevel.WARNING);
			return;
		}
	}

	// Delete every carried item whose catalog category matches.
	protected static void ClearCategory(IEntity target, int cat)
	{
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(target.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return;
		array<IEntity> items = {};
		inv.GetItems(items);
		int removed = 0;
		foreach (IEntity item : items)
		{
			if (!item || !item.GetPrefabData())
				continue;
			ResourceName p = item.GetPrefabData().GetPrefabName();
			if (p.IsEmpty())
				continue;
			if (CategoryOfPrefab(p) != cat)
				continue;
			if (inv.TryDeleteItem(item))
				removed++;
		}
		Print(string.Format("[DCO-ARS] clear category %1: removed %2 item(s)", cat, removed), LogLevel.NORMAL);
	}

	protected static void ResetToSnapshot(IEntity target)
	{
		string json;
		if (!s_Snapshots.Find(target, json))
		{
			Print("[DCO-ARS] reset: no snapshot (unit was never edited)", LogLevel.NORMAL);
			return;
		}
		GameEntity ge = GameEntity.Cast(target);
		if (!ge)
			return;
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(json))
		{
			Print("[DCO-ARS] reset: snapshot JSON failed to parse", LogLevel.WARNING);
			return;
		}
		if (SCR_PlayerArsenalLoadout.ApplyLoadoutString(ge, ctx))
			Print("[DCO-ARS] reset: pre-edit kit restored", LogLevel.NORMAL);
		else
			Print("[DCO-ARS] reset: apply reported failures (partial restore)", LogLevel.WARNING);
	}

	protected static EDCO_ArsenalCategory CategoryOfPrefab(ResourceName prefab)
	{
		SCR_EntityCatalogManagerComponent mgr = SCR_EntityCatalogManagerComponent.GetInstance();
		if (mgr)
		{
			SCR_EntityCatalogEntry entry = mgr.GetEntryWithPrefabFromAnyCatalog(EEntityCatalogType.ITEM, prefab);
			if (entry)
			{
				SCR_ArsenalItem data = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
				if (data)
					return DCO_ArsenalCatalog.CategoryOf(data.GetItemType(), data.GetItemMode());
			}
		}
		return EDCO_ArsenalCategory.ITEMS;
	}
}
