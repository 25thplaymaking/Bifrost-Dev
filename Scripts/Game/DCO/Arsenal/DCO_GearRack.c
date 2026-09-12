enum DCO_EGearRackSlot
{
	NONE,
	VEST,
	HELMET,
	BELT
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
	protected ref DCO_GearRackTransfer m_Transfer;
	[RplProp()]
	protected bool m_bBusy;
	[RplProp(onRplName: "QueueDisplayRefresh")]
	protected RplId m_VestId;
	[RplProp(onRplName: "QueueDisplayRefresh")]
	protected RplId m_HelmetId;
	[RplProp(onRplName: "QueueDisplayRefresh")]
	protected RplId m_BeltId;
	[RplProp(onRplName: "InvalidateDisplays")]
	protected int m_iContentsRevision;
	protected IEntity m_DisplayedVest;
	protected IEntity m_DisplayedHelmet;
	protected IEntity m_DisplayedBelt;

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
			GetGame().GetCallqueue().CallLater(PublishContents, 0, false);
		QueueDisplayRefresh();
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
		return kind == DCO_EGearRackSlot.VEST || kind == DCO_EGearRackSlot.HELMET || (m_bXL && kind == DCO_EGearRackSlot.BELT);
	}

	static DCO_EGearRackSlot GearKind(IEntity item)
	{
		if (!item) return DCO_EGearRackSlot.NONE;
		BaseLoadoutClothComponent cloth = BaseLoadoutClothComponent.Cast(item.FindComponent(BaseLoadoutClothComponent));
		if (!cloth || !cloth.GetAreaType()) return DCO_EGearRackSlot.NONE;
		typename area = cloth.GetAreaType().Type();
		if (BIA_ItemIntel.IsWaistArea(area)) return DCO_EGearRackSlot.BELT;
		if (area.IsInherited(LoadoutVestArea) || area.IsInherited(LoadoutArmoredVestSlotArea)) return DCO_EGearRackSlot.VEST;
		if (area.IsInherited(LoadoutHeadCoverArea)) return DCO_EGearRackSlot.HELMET;
		return DCO_EGearRackSlot.NONE;
	}

	static IEntity WornGear(IEntity user, DCO_EGearRackSlot kind)
	{
		if (!user) return null;
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
		return RplId.Invalid();
	}

	bool HasStoredGear(DCO_EGearRackSlot kind)
	{
		if (Replication.IsServer()) return StoredGear(kind) != null;
		return StoredId(kind).IsValid();
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
		return SupportsKind(kind) && CanInteract(user) && !HasStoredGear(kind) && WornGear(user, kind);
	}

	bool CanTake(IEntity user, DCO_EGearRackSlot kind)
	{
		return SupportsKind(kind) && CanInteract(user) && FindReturnSlot(user, StoredGear(kind)) >= 0;
	}

	protected int FindReturnSlot(IEntity user, IEntity item)
	{
		if (!user || !item) return -1;
		EquipedLoadoutStorageComponent worn = EquipedLoadoutStorageComponent.Cast(user.FindComponent(EquipedLoadoutStorageComponent));
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(user.FindComponent(InventoryStorageManagerComponent));
		if (!worn || !manager) return -1;
		for (int i = 0; i < worn.GetSlotsCount(); i++)
		{
			if (!worn.Get(i) && manager.CanMoveItemToStorage(item, worn, i)) return i;
		}
		return -1;
	}

	bool TransferGear(IEntity user, DCO_EGearRackSlot kind, bool take, SCR_ResourcePlayerControllerInventoryComponent requester)
	{
		if (!Replication.IsServer() || !requester || !CanInteract(user) || !m_Storage || !m_Inventory)
			return false;
		if (!SupportsKind(kind)) return false;
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(user.FindComponent(InventoryStorageManagerComponent));
		if (!manager) return false;
		IEntity item;
		BaseInventoryStorageComponent destination;
		int slot = -1;
		if (take)
		{
			item = StoredGear(kind);
			slot = FindReturnSlot(user, item);
			if (slot < 0) return false;
			destination = EquipedLoadoutStorageComponent.Cast(user.FindComponent(EquipedLoadoutStorageComponent));
		}
		else
		{
			if (StoredGear(kind)) return false;
			item = WornGear(user, kind);
			destination = m_Storage;
		}
		if (!item || !destination || !manager || !manager.CanMoveItemToStorage(item, destination, slot)) return false;
		m_bBusy = true;
		Replication.BumpMe();
		m_Transfer = new DCO_GearRackTransfer();
		m_Transfer.m_Rack = this;
		m_Transfer.m_Requester = requester;
		// Moving the original entity preserves its nested inventory, damage and attachments.
		if (manager.TryMoveItemToStorage(item, destination, slot, m_Transfer)) return true;
		FinishTransfer();
		return false;
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
		bool pending;
		m_VestId = GearId(DCO_EGearRackSlot.VEST, pending);
		m_HelmetId = GearId(DCO_EGearRackSlot.HELMET, pending);
		m_BeltId = GearId(DCO_EGearRackSlot.BELT, pending);
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
		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		if (!super.RplLoad(reader)) return false;
		if (!reader.ReadRplId(m_VestId) || !reader.ReadRplId(m_HelmetId) || !reader.ReadRplId(m_BeltId)) return false;
		InvalidateDisplays();
		return true;
	}

	protected void InvalidateDisplays()
	{
		m_DisplayedVest = null;
		m_DisplayedHelmet = null;
		m_DisplayedBelt = null;
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
		RefreshDisplay(DCO_EGearRackSlot.VEST, m_VestDisplay, m_DisplayedVest, pending);
		RefreshDisplay(DCO_EGearRackSlot.HELMET, m_HelmetDisplay, m_DisplayedHelmet, pending);
		RefreshDisplay(DCO_EGearRackSlot.BELT, m_BeltDisplay, m_DisplayedBelt, pending);
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
		m_VestDisplay = null;
		m_HelmetDisplay = null;
		m_BeltDisplay = null;
		super.OnDelete(owner);
	}
}

class DCO_GearRackTransfer : ScriptedInventoryOperationCallback
{
	DCO_GearRackComponent m_Rack;
	SCR_ResourcePlayerControllerInventoryComponent m_Requester;
	protected bool m_bFinished;
	void DetachRack()
	{
		// Deleting a rack must not leave its requesting player's inventory locked.
		if (m_Requester) m_Requester.BIA_FinishRackTransfer();
		m_Requester = null;
		m_Rack = null;
		m_bFinished = true;
	}
	override protected void OnComplete() { Complete(); }
	override protected void OnFailed() { Complete(); }
	protected void Complete()
	{
		if (m_bFinished) return;
		m_bFinished = true;
		if (m_Requester) m_Requester.BIA_FinishRackTransfer();
		if (m_Rack) m_Rack.FinishTransfer();
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
	override bool GetActionNameScript(out string outName) { outName = "Arsenal"; return true; }
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
		if (rack && rack.HasStoredGear(Kind())) verb = "Take ";
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
		if (rack.HasStoredGear(Kind()))
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
		if (inventory && rpl && rack) inventory.BIA_RequestRackTransfer(rpl.Id(), Kind(), rack.HasStoredGear(Kind()));
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
