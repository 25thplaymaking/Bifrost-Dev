enum DCO_EGearRackSlot
{
	NONE,
	VEST,
	HELMET,
	BELT,
	BACK_PANEL,
	PRIMARY
}

[ComponentEditorProps(category: "Bifrost/Arsenal", description: "Placeable Arsenal with equipment display hooks.")]
class DCO_GearRackComponentClass : DCO_ArsenalAccessComponentClass {}

class DCO_GearRackComponent : DCO_ArsenalAccessComponent
{
	[Attribute("0", UIWidgets.CheckBox, "XL cross with a dedicated waist belt support.")]
	protected bool m_bXL;
	protected DCO_GearRackStorageComponent m_Storage;
	protected ScriptedInventoryStorageManagerComponent m_Inventory;
	protected IEntity m_VestDisplay;
	protected IEntity m_HelmetDisplay;
	protected IEntity m_BeltDisplay;
	protected IEntity m_BackPanelDisplay;
	protected IEntity m_PrimaryDisplay;
	protected ref DCO_GearRackTransfer m_Transfer;
	[RplProp()]
	protected bool m_bBusy;
	[RplProp(onRplName: "QueueDisplayRefresh")]
	protected RplId m_VestId;
	[RplProp(onRplName: "QueueDisplayRefresh")]
	protected RplId m_HelmetId;
	[RplProp(onRplName: "QueueDisplayRefresh")]
	protected RplId m_BeltId;
	[RplProp(onRplName: "QueueDisplayRefresh")]
	protected RplId m_BackPanelId;
	[RplProp(onRplName: "QueueDisplayRefresh")]
	protected RplId m_PrimaryId;
	[RplProp()]
	protected string m_sRackName;
	[RplProp()]
	protected string m_sOperatorName;
	[RplProp()]
	protected bool m_bPersistent;
	[RplProp(onRplName: "InvalidateDisplays")]
	protected int m_iContentsRevision;
	protected IEntity m_DisplayedVest;
	protected IEntity m_DisplayedHelmet;
	protected IEntity m_DisplayedBelt;
	protected IEntity m_DisplayedBackPanel;
	protected IEntity m_DisplayedPrimary;

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		GetGame().GetCallqueue().Remove(AuditTarget);
		m_Storage = DCO_GearRackStorageComponent.Cast(owner.FindComponent(DCO_GearRackStorageComponent));
		m_Inventory = ScriptedInventoryStorageManagerComponent.Cast(owner.FindComponent(ScriptedInventoryStorageManagerComponent));
		if (m_Inventory)
		{
			m_Inventory.m_OnItemAddedInvoker.Insert(OnContentsChanged);
			m_Inventory.m_OnItemRemovedInvoker.Insert(OnContentsChanged);
		}
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(PublishContents, 0, false);
			GetGame().GetCallqueue().CallLater(PreparePersistence, 0, false);
		}
		QueueDisplayRefresh();
	}

	string GetRackName() { return m_sRackName; }
	string GetOperatorName() { return m_sOperatorName; }
	bool IsPersistent() { return m_bPersistent; }
	string GetRackLabel()
	{
		if (m_sOperatorName.IsEmpty()) return m_sRackName;
		if (m_sRackName.IsEmpty()) return m_sOperatorName;
		return m_sRackName + " (" + m_sOperatorName + ")";
	}

	bool ConfigureRack(string name, string operatorName, bool persistent, out string result)
	{
		result = "Rack settings could not be applied.";
		if (!Replication.IsServer() || m_bBusy || name.Length() > 64 || operatorName.Length() > 64) return false;
		name = DCO_UIText.Plain(name);
		operatorName = DCO_UIText.Plain(operatorName);
		name.Replace("\n", " ");
		name.Replace("\r", " ");
		operatorName.Replace("\n", " ");
		operatorName.Replace("\r", " ");
		name.TrimInPlace();
		operatorName.TrimInPlace();
		if (persistent != m_bPersistent && !ApplyPersistence(persistent))
		{
			result = "Mission saving is unavailable or still loading. Try again after the mission has loaded.";
			return false;
		}
		RestoreRackSettings(name, operatorName, persistent);
		result = "Cross settings updated. Gear remains available to everyone.";
		if (persistent) result += " The cross and its contents will restore with the saved mission.";
		return true;
	}

	void RestoreRackSettings(string name, string operatorName, bool persistent)
	{
		if (!Replication.IsServer()) return;
		m_sRackName = name;
		m_sOperatorName = operatorName;
		m_bPersistent = persistent;
		Replication.BumpMe();
	}

	protected void PreparePersistence()
	{
		SCR_PersistenceSystem system = SCR_PersistenceSystem.GetByEntityWorld(GetOwner());
		if (!Replication.IsServer() || !system) return;
		if (system.GetState() < EPersistenceSystemState.ACTIVE)
		{
			GetGame().GetCallqueue().CallLater(PreparePersistence, 250, false);
			return;
		}
		if (ApplyPersistence(m_bPersistent)) PublishContents();
	}

	protected bool ApplyPersistence(bool enabled)
	{
		SCR_PersistenceSystem system = SCR_PersistenceSystem.GetByEntityWorld(GetOwner());
		if (!system || system.GetState() != EPersistenceSystemState.ACTIVE) return false;
		// Keep a tracked inventory root even when saving is off, so its gear cannot save separately.
		if (m_bPersistent && !enabled && system.IsTracked(GetOwner())) system.StopTracking(GetOwner(), true);
		if (!system.IsTracked(GetOwner()) && !system.StartTracking(GetOwner(), false)) return false;
		if (!system.ReloadConfig(GetOwner())) return false;
		EntityPersistenceConfig config = EntityPersistenceConfig.Cast(system.GetConfig(GetOwner()));
		if (!config) return false;
		config.m_bSelfSpawn = enabled;
		if (!enabled) config.m_eSaveMask = 0;
		return system.SetConfig(GetOwner(), config);
	}

	protected void TrackInventory(IEntity entity, PersistenceSystem system)
	{
		if (!entity || !system) return;
		if (entity.FindComponent(InventoryItemComponent) && !system.IsTracked(entity)) system.StartTracking(entity, false);
		IEntity child = entity.GetChildren();
		while (child)
		{
			TrackInventory(child, system);
			child = child.GetSibling();
		}
	}

	override IEntity GetTarget() { return GetOwner(); }
	override RplId GetTargetId()
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (rpl) return rpl.Id();
		return RplId.Invalid();
	}
	override vector GetAnchorWorld() { return GetOwner().CoordToParent("0 0.55 0"); }
	bool IsBusy() { return m_bBusy; }
	bool SupportsBelt() { return m_bXL; }
	bool SupportsKind(DCO_EGearRackSlot kind)
	{
		return kind == DCO_EGearRackSlot.VEST || kind == DCO_EGearRackSlot.HELMET || kind == DCO_EGearRackSlot.BACK_PANEL
			|| (m_bXL && (kind == DCO_EGearRackSlot.BELT || kind == DCO_EGearRackSlot.PRIMARY));
	}

	static DCO_EGearRackSlot GearKind(IEntity item)
	{
		if (!item) return DCO_EGearRackSlot.NONE;
		BaseWeaponComponent weapon = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
		if (weapon && weapon.GetWeaponSlotType() == "primary") return DCO_EGearRackSlot.PRIMARY;
		BaseLoadoutClothComponent cloth = BaseLoadoutClothComponent.Cast(item.FindComponent(BaseLoadoutClothComponent));
		if (!cloth || !cloth.GetAreaType()) return DCO_EGearRackSlot.NONE;
		typename area = cloth.GetAreaType().Type();
		if (BIA_ItemIntel.IsBackPanel(item)) return DCO_EGearRackSlot.BACK_PANEL;
		if (BIA_ItemIntel.IsWaistArea(area)) return DCO_EGearRackSlot.BELT;
		if (area.IsInherited(LoadoutVestArea) || area.IsInherited(LoadoutArmoredVestSlotArea)) return DCO_EGearRackSlot.VEST;
		if (area.IsInherited(LoadoutHeadCoverArea)) return DCO_EGearRackSlot.HELMET;
		return DCO_EGearRackSlot.NONE;
	}

	static IEntity WornGear(IEntity user, DCO_EGearRackSlot kind)
	{
		if (!user) return null;
		if (kind == DCO_EGearRackSlot.PRIMARY)
		{
			EquipedWeaponStorageComponent weapons = EquipedWeaponStorageComponent.Cast(user.FindComponent(EquipedWeaponStorageComponent));
			if (weapons && weapons.GetSlotsCount() > 0 && GearKind(weapons.Get(0)) == kind) return weapons.Get(0);
			return null;
		}
		EquipedLoadoutStorageComponent worn = EquipedLoadoutStorageComponent.Cast(user.FindComponent(EquipedLoadoutStorageComponent));
		if (!worn) return null;
		for (int i = 0; i < worn.GetSlotsCount(); i++)
		{
			IEntity item = worn.Get(i);
			if (item && GearKind(item) == kind) return item;
		}
		return null;
	}

	IEntity StoredGear(DCO_EGearRackSlot kind)
	{
		if (!Replication.IsServer())
		{
			RplComponent rpl = RplComponent.Cast(Replication.FindItem(StoredId(kind)));
			if (rpl) return rpl.GetEntity();
			return null;
		}
		if (!m_Storage || kind == DCO_EGearRackSlot.NONE) return null;
		for (int i = 0; i < m_Storage.GetSlotsCount(); i++)
		{
			IEntity item = m_Storage.Get(i);
			if (item && GearKind(item) == kind) return item;
		}
		return null;
	}

	protected RplId StoredId(DCO_EGearRackSlot kind)
	{
		if (kind == DCO_EGearRackSlot.VEST) return m_VestId;
		if (kind == DCO_EGearRackSlot.HELMET) return m_HelmetId;
		if (kind == DCO_EGearRackSlot.BELT) return m_BeltId;
		if (kind == DCO_EGearRackSlot.BACK_PANEL) return m_BackPanelId;
		if (kind == DCO_EGearRackSlot.PRIMARY) return m_PrimaryId;
		return RplId.Invalid();
	}

	bool HasStoredGear(DCO_EGearRackSlot kind)
	{
		if (Replication.IsServer()) return StoredGear(kind) != null;
		return StoredId(kind).IsValid();
	}

	bool HasActionGear(DCO_EGearRackSlot kind)
	{
		return HasStoredGear(kind) || (kind == DCO_EGearRackSlot.VEST && HasStoredGear(DCO_EGearRackSlot.BACK_PANEL));
	}

	bool CanInteract(IEntity user)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(user);
		if (!character || !character.GetCharacterController()) return false;
		return !m_bBusy && !character.GetCharacterController().IsDead()
			&& !character.GetCharacterController().IsUnconscious() && IsUsableBy(GetOwner(), user);
	}

	bool CanHang(IEntity user, DCO_EGearRackSlot kind)
	{
		if (kind == DCO_EGearRackSlot.BACK_PANEL) return false;
		if (kind == DCO_EGearRackSlot.VEST && HasStoredGear(DCO_EGearRackSlot.BACK_PANEL)) return false;
		return SupportsKind(kind) && CanInteract(user) && !HasStoredGear(kind) && WornGear(user, kind);
	}

	bool CanTake(IEntity user, DCO_EGearRackSlot kind)
	{
		if (kind == DCO_EGearRackSlot.BACK_PANEL) return false;
		if (kind == DCO_EGearRackSlot.VEST && HasStoredGear(DCO_EGearRackSlot.BACK_PANEL)
			&& FindReturnSlot(user, StoredGear(DCO_EGearRackSlot.BACK_PANEL)) < 0) return false;
		IEntity item = StoredGear(kind);
		if (!item && kind == DCO_EGearRackSlot.VEST) item = StoredGear(DCO_EGearRackSlot.BACK_PANEL);
		return SupportsKind(kind) && CanInteract(user) && FindReturnSlot(user, item) >= 0;
	}

	protected BaseInventoryStorageComponent ReturnStorage(IEntity user, IEntity item)
	{
		if (!user) return null;
		if (GearKind(item) == DCO_EGearRackSlot.PRIMARY)
			return BaseInventoryStorageComponent.Cast(user.FindComponent(EquipedWeaponStorageComponent));
		return BaseInventoryStorageComponent.Cast(user.FindComponent(EquipedLoadoutStorageComponent));
	}

	protected int FindReturnSlot(IEntity user, IEntity item)
	{
		if (!user || !item) return -1;
		BaseInventoryStorageComponent worn = ReturnStorage(user, item);
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(user.FindComponent(InventoryStorageManagerComponent));
		if (!worn || !manager) return -1;
		for (int i = 0; i < worn.GetSlotsCount(); i++)
		{
			if (GearKind(item) == DCO_EGearRackSlot.PRIMARY && i != 0) continue;
			if (!worn.Get(i) && manager.CanMoveItemToStorage(item, worn, i)) return i;
		}
		return -1;
	}

	bool TransferGear(IEntity user, DCO_EGearRackSlot kind, bool take, SCR_ResourcePlayerControllerInventoryComponent requester)
	{
		if (!Replication.IsServer() || !requester || !CanInteract(user) || !m_Storage || !m_Inventory)
			return false;
		if (!SupportsKind(kind) || kind == DCO_EGearRackSlot.BACK_PANEL) return false;
		if (take && !CanTake(user, kind)) return false;
		if (!take && !CanHang(user, kind)) return false;
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(user.FindComponent(InventoryStorageManagerComponent));
		if (!manager) return false;
		IEntity item;
		BaseInventoryStorageComponent destination;
		int slot = -1;
		if (take)
		{
			item = StoredGear(kind);
			// An interrupted paired return must still allow the remaining panel to be recovered.
			if (!item && kind == DCO_EGearRackSlot.VEST) item = StoredGear(DCO_EGearRackSlot.BACK_PANEL);
			slot = FindReturnSlot(user, item);
			if (slot < 0) return false;
			destination = ReturnStorage(user, item);
		}
		else
		{
			if (StoredGear(kind)) return false;
			item = WornGear(user, kind);
			destination = m_Storage;
		}
		if (!item || !destination || !manager || !manager.CanMoveItemToStorage(item, destination, slot)) return false;
		DCO_GearRackTransfer transfer = new DCO_GearRackTransfer();
		transfer.m_Rack = this;
		transfer.m_Requester = requester;
		transfer.m_Manager = manager;
		if (!transfer.Add(item, destination, slot)) return false;
		if (kind == DCO_EGearRackSlot.VEST)
		{
			IEntity panel;
			if (take) panel = StoredGear(DCO_EGearRackSlot.BACK_PANEL);
			else panel = WornGear(user, DCO_EGearRackSlot.BACK_PANEL);
			if (panel && panel != item)
			{
				BaseInventoryStorageComponent panelDestination = m_Storage;
				int panelSlot = -1;
				if (take)
				{
					panelDestination = ReturnStorage(user, panel);
					panelSlot = FindReturnSlot(user, panel);
					if (panelSlot < 0) return false;
				}
				if (!transfer.Add(panel, panelDestination, panelSlot)) return false;
			}
		}
		m_bBusy = true;
		m_Transfer = transfer;
		Replication.BumpMe();
		transfer.MoveNext();
		return true;
	}

	void FinishTransfer()
	{
		m_bBusy = false;
		Replication.BumpMe();
		m_Transfer = null;
		PublishContents();
	}

	protected void OnContentsChanged(IEntity item, BaseInventoryStorageComponent storage)
	{
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().Remove(PublishContents);
			GetGame().GetCallqueue().CallLater(PublishContents, 0, false);
		}
		InvalidateDisplays();
	}

	protected RplId GearId(DCO_EGearRackSlot kind, inout bool pending)
	{
		IEntity item = StoredGear(kind);
		if (!item) return RplId.Invalid();
		RplComponent rpl = RplComponent.Cast(item.FindComponent(RplComponent));
		if (rpl && rpl.Id().IsValid()) return rpl.Id();
		pending = true;
		return RplId.Invalid();
	}

	protected void PublishContents()
	{
		if (!Replication.IsServer() || !GetOwner()) return;
		GetGame().GetCallqueue().Remove(PublishContents);
		PersistenceSystem persistence = SCR_PersistenceSystem.GetByEntityWorld(GetOwner());
		if (persistence && persistence.GetState() == EPersistenceSystemState.ACTIVE && m_Storage)
		{
			for (int slot = 0; slot < m_Storage.GetSlotsCount(); slot++) TrackInventory(m_Storage.Get(slot), persistence);
		}
		bool pending;
		m_VestId = GearId(DCO_EGearRackSlot.VEST, pending);
		m_HelmetId = GearId(DCO_EGearRackSlot.HELMET, pending);
		m_BeltId = GearId(DCO_EGearRackSlot.BELT, pending);
		m_BackPanelId = GearId(DCO_EGearRackSlot.BACK_PANEL, pending);
		m_PrimaryId = GearId(DCO_EGearRackSlot.PRIMARY, pending);
		m_iContentsRevision++;
		Replication.BumpMe();
		InvalidateDisplays();
		if (pending) GetGame().GetCallqueue().CallLater(PublishContents, 250, false);
	}

	override bool RplSave(ScriptBitWriter writer)
	{
		if (!super.RplSave(writer)) return false;
		writer.WriteRplId(m_VestId);
		writer.WriteRplId(m_HelmetId);
		writer.WriteRplId(m_BeltId);
		writer.WriteRplId(m_BackPanelId);
		writer.WriteRplId(m_PrimaryId);
		writer.WriteString(m_sRackName);
		writer.WriteString(m_sOperatorName);
		writer.WriteBool(m_bPersistent);
		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		if (!super.RplLoad(reader)) return false;
		if (!reader.ReadRplId(m_VestId) || !reader.ReadRplId(m_HelmetId) || !reader.ReadRplId(m_BeltId)) return false;
		if (!reader.ReadRplId(m_BackPanelId) || !reader.ReadRplId(m_PrimaryId)) return false;
		if (!reader.ReadString(m_sRackName) || !reader.ReadString(m_sOperatorName) || !reader.ReadBool(m_bPersistent)) return false;
		InvalidateDisplays();
		return true;
	}

	protected void InvalidateDisplays()
	{
		m_DisplayedVest = null;
		m_DisplayedHelmet = null;
		m_DisplayedBelt = null;
		m_DisplayedBackPanel = null;
		m_DisplayedPrimary = null;
		QueueDisplayRefresh();
	}

	protected void QueueDisplayRefresh()
	{
		if (System.IsConsoleApp()) return;
		GetGame().GetCallqueue().Remove(RefreshDisplays);
		GetGame().GetCallqueue().CallLater(RefreshDisplays, 0, false);
	}

	protected void RefreshDisplays()
	{
		if (System.IsConsoleApp() || !GetOwner()) return;
		bool pending;
		IEntity previousVest = m_VestDisplay;
		RefreshDisplay(DCO_EGearRackSlot.VEST, m_VestDisplay, m_DisplayedVest, pending);
		// A panel can arrive before its vest when the rack streams onto a client.
		if (previousVest != m_VestDisplay) m_DisplayedBackPanel = null;
		RefreshDisplay(DCO_EGearRackSlot.HELMET, m_HelmetDisplay, m_DisplayedHelmet, pending);
		RefreshDisplay(DCO_EGearRackSlot.BELT, m_BeltDisplay, m_DisplayedBelt, pending);
		RefreshDisplay(DCO_EGearRackSlot.BACK_PANEL, m_BackPanelDisplay, m_DisplayedBackPanel, pending);
		RefreshDisplay(DCO_EGearRackSlot.PRIMARY, m_PrimaryDisplay, m_DisplayedPrimary, pending);
		// A snapshot can precede its inventory entities when joining or streaming back in.
		if (pending) GetGame().GetCallqueue().CallLater(RefreshDisplays, 250, false);
	}

	protected void RefreshDisplay(DCO_EGearRackSlot kind, inout IEntity display, inout IEntity previous, inout bool pending)
	{
		IEntity source = StoredGear(kind);
		if (source && source == previous && display && !display.IsDeleted()) return;
		BIA_PreviewDress.DeleteLocalHierarchy(display);
		display = CreateDisplay(source, kind);
		previous = source;
		if (HasStoredGear(kind) && !display) pending = true;
	}

	protected IEntity CreateDisplay(IEntity source, DCO_EGearRackSlot kind)
	{
		if (!source || !GetOwner()) return null;
		InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(source.FindComponent(InventoryItemComponent));
		if (!inventoryItem) return null;
		IEntity display = inventoryItem.CreatePreviewEntity(GetOwner().GetWorld(), 0);
		if (!display) return null;
		ShowDisplayHierarchy(display);
		if (kind == DCO_EGearRackSlot.BACK_PANEL && m_VestDisplay)
			BIA_WeaponStage.PositionBackPanel(display, m_VestDisplay);
		else if (kind == DCO_EGearRackSlot.BACK_PANEL)
			BIA_WeaponStage.PositionGearOnStand(display, GetOwner(), DCO_EGearRackSlot.VEST, display.GetScale() * GetOwner().GetScale(), GetOwner().GetYawPitchRoll(), m_bXL);
		else
			BIA_WeaponStage.PositionGearOnStand(display, GetOwner(), kind, display.GetScale() * GetOwner().GetScale(), GetOwner().GetYawPitchRoll(), m_bXL);
		// Preserve the world pose when the display starts following the rack.
		GetOwner().AddChild(display, -1, EAddChildFlags.AUTO_TRANSFORM | EAddChildFlags.RECALC_LOCAL_TRANSFORM);
		return display;
	}

	protected static void ShowDisplayHierarchy(IEntity display)
	{
		display.SetCameraMask(-1);
		display.SetFlags(EntityFlags.VISIBLE, false);
		IEntity child = display.GetChildren();
		while (child)
		{
			ShowDisplayHierarchy(child);
			child = child.GetSibling();
		}
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(PreparePersistence);
		if (m_Transfer) m_Transfer.DetachRack();
		m_Transfer = null;
		GetGame().GetCallqueue().Remove(RefreshDisplays);
		GetGame().GetCallqueue().Remove(PublishContents);
		if (m_Inventory)
		{
			m_Inventory.m_OnItemAddedInvoker.Remove(OnContentsChanged);
			m_Inventory.m_OnItemRemovedInvoker.Remove(OnContentsChanged);
		}
		BIA_PreviewDress.DeleteLocalHierarchy(m_VestDisplay);
		BIA_PreviewDress.DeleteLocalHierarchy(m_HelmetDisplay);
		BIA_PreviewDress.DeleteLocalHierarchy(m_BeltDisplay);
		BIA_PreviewDress.DeleteLocalHierarchy(m_BackPanelDisplay);
		BIA_PreviewDress.DeleteLocalHierarchy(m_PrimaryDisplay);
		m_VestDisplay = null;
		m_HelmetDisplay = null;
		m_BeltDisplay = null;
		m_BackPanelDisplay = null;
		m_PrimaryDisplay = null;
		super.OnDelete(owner);
	}
}

class DCO_GearRackMove
{
	IEntity m_Item;
	BaseInventoryStorageComponent m_Source;
	BaseInventoryStorageComponent m_Destination;
	int m_SourceSlot;
	int m_DestinationSlot;
}

class DCO_GearRackMoveCallback : ScriptedInventoryOperationCallback
{
	DCO_GearRackTransfer m_Transfer;
	protected bool m_bDone;
	override protected void OnComplete() { Complete(true); }
	override protected void OnFailed() { Complete(false); }
	void Complete(bool success)
	{
		if (m_bDone) return;
		m_bDone = true;
		if (m_Transfer) m_Transfer.OnMoveDone(success);
		m_Transfer = null;
	}
}

class DCO_GearRackTransfer
{
	DCO_GearRackComponent m_Rack;
	SCR_ResourcePlayerControllerInventoryComponent m_Requester;
	InventoryStorageManagerComponent m_Manager;
	protected ref array<ref DCO_GearRackMove> m_Moves = {};
	protected ref DCO_GearRackMoveCallback m_Callback;
	protected int m_iNext;
	protected bool m_bRollback;
	protected bool m_bFinished;

	bool Add(IEntity item, BaseInventoryStorageComponent destination, int slot)
	{
		InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		InventoryStorageSlot source;
		if (inventoryItem) source = inventoryItem.GetParentSlot();
		if (!source || !m_Manager.CanMoveItemToStorage(item, destination, slot)) return false;
		DCO_GearRackMove move = new DCO_GearRackMove();
		move.m_Item = item;
		move.m_Source = source.GetStorage();
		move.m_SourceSlot = source.GetID();
		move.m_Destination = destination;
		move.m_DestinationSlot = slot;
		m_Moves.Insert(move);
		return true;
	}

	void MoveNext()
	{
		if (m_bFinished) return;
		if (m_iNext < 0 || m_iNext >= m_Moves.Count())
		{
			Complete();
			return;
		}
		DCO_GearRackMove move = m_Moves[m_iNext];
		BaseInventoryStorageComponent destination = move.m_Destination;
		int slot = move.m_DestinationSlot;
		if (m_bRollback)
		{
			destination = move.m_Source;
			slot = move.m_SourceSlot;
		}
		DCO_GearRackMoveCallback callback = new DCO_GearRackMoveCallback();
		callback.m_Transfer = this;
		m_Callback = callback;
		// Keep original entities, including every attachment and nested container.
		if (!move.m_Item || !destination || !m_Manager
			|| !m_Manager.TryMoveItemToStorage(move.m_Item, destination, slot, callback)) callback.Complete(false);
	}

	void OnMoveDone(bool success)
	{
		if (m_bFinished) return;
		if (m_bRollback)
		{
			if (!success)
				Print("[Bifrost] Rack transfer rollback could not restore a slot; original gear remains in its current inventory.", LogLevel.WARNING);
			m_iNext--;
		}
		else if (success) m_iNext++;
		else
		{
			m_bRollback = true;
			m_iNext--;
		}
		MoveNext();
	}

	void DetachRack()
	{
		// Deleting a rack must not leave its requesting player's inventory locked.
		if (m_Requester) m_Requester.BIA_FinishRackTransfer();
		m_Requester = null;
		m_Rack = null;
		m_bFinished = true;
		if (m_Callback) m_Callback.m_Transfer = null;
		m_Callback = null;
	}
	protected void Complete()
	{
		if (m_bFinished) return;
		m_bFinished = true;
		if (m_Requester) m_Requester.BIA_FinishRackTransfer();
		DCO_GearRackComponent rack = m_Rack;
		m_Requester = null;
		m_Rack = null;
		m_Callback = null;
		if (rack) rack.FinishTransfer();
	}
}

[ComponentEditorProps(category: "Bifrost/Arsenal", description: "Native storage limited to the cross's equipment hooks.")]
class DCO_GearRackStorageComponentClass : UniversalInventoryStorageComponentClass {}

class DCO_GearRackStorageComponent : UniversalInventoryStorageComponent
{
	override bool OnOverrideCanStoreItem() { return true; }
	override bool OnOverrideCanStoreResource() { return true; }
	override bool OnOverrideCanReplaceItem() { return true; }
	override bool CanStoreResource(ResourceName resourceName, int slotID) { return false; }
	override bool CanReplaceItem(IEntity nextItem, int slotID) { return false; }
	override bool CanStoreItem(IEntity item, int slotID)
	{
		DCO_EGearRackSlot kind = DCO_GearRackComponent.GearKind(item);
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(GetOwner().FindComponent(DCO_GearRackComponent));
		if (!rack || !rack.SupportsKind(kind)) return false;
		for (int i = 0; i < GetSlotsCount(); i++)
		{
			IEntity stored = Get(i);
			if (stored && DCO_GearRackComponent.GearKind(stored) == kind) return false;
		}
		return true;
	}
}

class DCO_GearRackArsenalAction : DCO_OpenArsenalAction
{
	override bool GetActionNameScript(out string outName)
	{
		outName = "Arsenal";
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(GetOwner().FindComponent(DCO_GearRackComponent));
		if (rack && !rack.GetRackLabel().IsEmpty()) outName += " - " + rack.GetRackLabel();
		return true;
	}
	override bool CanBePerformedScript(IEntity user)
	{
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(GetOwner().FindComponent(DCO_GearRackComponent));
		return super.CanBePerformedScript(user) && rack && rack.CanInteract(user);
	}
}

class DCO_HangVestAction : ScriptedUserAction
{
	protected DCO_EGearRackSlot Kind() { return DCO_EGearRackSlot.VEST; }
	protected string GearName() { return "Vest"; }
	override bool HasLocalEffectOnlyScript() { return true; }
	override bool GetActionNameScript(out string outName)
	{
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(GetOwner().FindComponent(DCO_GearRackComponent));
		string verb = "Hang ";
		if (rack && rack.HasActionGear(Kind())) verb = "Take ";
		outName = verb + GearName();
		return true;
	}
	override bool CanBeShownScript(IEntity user)
	{
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(GetOwner().FindComponent(DCO_GearRackComponent));
		return rack && rack.SupportsKind(Kind()) && !DCO_ArsenalMenu.IsOpen()
			&& DCO_ArsenalAccessComponent.IsUsableBy(GetOwner(), user);
	}
	override bool CanBePerformedScript(IEntity user)
	{
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(GetOwner().FindComponent(DCO_GearRackComponent));
		if (!rack) return false;
		if (rack.HasActionGear(Kind()))
		{
			if (rack.CanTake(user, Kind())) return true;
			SetCannotPerformReason("Free the matching equipment slot and remain beside the rack.");
			return false;
		}
		if (rack.CanHang(user, Kind())) return true;
		SetCannotPerformReason("Wear the matching gear and remain beside the rack.");
		return false;
	}
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller.GetControlledEntity() != pUserEntity || !CanBePerformedScript(pUserEntity)) return;
		SCR_ResourcePlayerControllerInventoryComponent inventory = SCR_ResourcePlayerControllerInventoryComponent.Cast(controller.FindComponent(SCR_ResourcePlayerControllerInventoryComponent));
		RplComponent rpl = RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent));
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(pOwnerEntity.FindComponent(DCO_GearRackComponent));
		if (inventory && rpl && rack) inventory.BIA_RequestRackTransfer(rpl.Id(), Kind(), rack.HasActionGear(Kind()));
	}
}

class DCO_HangHelmetAction : DCO_HangVestAction
{
	override protected DCO_EGearRackSlot Kind() { return DCO_EGearRackSlot.HELMET; }
	override protected string GearName() { return "Helmet"; }
}

class DCO_HangBeltAction : DCO_HangVestAction
{
	override protected DCO_EGearRackSlot Kind() { return DCO_EGearRackSlot.BELT; }
	override protected string GearName() { return "Belt"; }
}

class DCO_HangPrimaryAction : DCO_HangVestAction
{
	override protected DCO_EGearRackSlot Kind() { return DCO_EGearRackSlot.PRIMARY; }
	override protected string GearName() { return "Primary Rifle"; }
}
