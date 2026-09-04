//! One slot card of the soldier overview: a weapon-slot category or a character clothing slot,
//! its rail row, and what the draft currently holds there.
class GRSA_SoldierCard
{
	string m_sLabel;
	GRSA_ArmoryCategory m_Category;
	int m_iWeaponSlot = -1;
	int m_iClothingSlot = -1;
	string m_sArea;
	GRSA_ItemRowComponent m_Row;

	//------------------------------------------------------------------------------------------------
	bool IsWeapon()
	{
		return m_iWeaponSlot >= 0;
	}
}

enum GRSA_ESoldierActionKind
{
	CONTENTS,
	ATTACHMENTS
}

class GRSA_SoldierAction
{
	GRSA_ESoldierActionKind m_eKind;
	string m_sLabel;
	GRSA_ItemRowComponent m_Row;
}

//! SOLDIER tab: the OUTFIT side of the draft — gear slot cards right (one per character
//! clothing slot the station can stock), the draft mannequin center-stage; weapons belong to
//! the GUNSMITH tab. Selecting a card opens its item list over the left side — search on top,
//! click equips into the card's slot, clicking the equipped item takes it off. Card focus
//! reframes the mannequin on the matching body region.
class GRSA_SoldierScreen : SCR_SubMenuBase
{
	protected static const ResourceName CARD_ROW_LAYOUT = "{4A47972BDCB8148E}UI/layouts/Menus/Armory/GRSA_ItemRow.layout";

	protected Widget m_wGearCardList;
	protected Widget m_wCustomizeRail;
	protected TextWidget m_wCustomizeHeader;
	protected Widget m_wCustomizeList;
	protected RenderTargetWidget m_wStageWorld;

	protected ref GRSA_SoldierStage m_Stage;
	protected ref GRSA_ItemListPanel m_ItemList;
	protected SCR_InputButtonComponent m_WearChip;
	protected ref array<ref GRSA_SoldierCard> m_aCards = {};
	protected ref array<ref GRSA_SoldierAction> m_aActions = {};
	protected GRSA_SoldierCard m_ActiveCard;
	protected GRSA_SoldierCard m_FocusedCard;
	protected GRSA_SoldierAction m_ActiveAction;
	protected GRSA_SoldierCard m_PendingFocusCard;
	protected ref array<ref GRSA_ItemEntry> m_aActiveItems;
	protected bool m_bSuppressFocusFrame;

	protected static GRSA_SoldierScreen s_ActiveInstance;

	//------------------------------------------------------------------------------------------------
	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);

		m_wGearCardList = m_wRoot.FindAnyWidget("GearCardList");
		m_wCustomizeRail = m_wRoot.FindAnyWidget("LeftRail");
		m_wCustomizeHeader = TextWidget.Cast(m_wRoot.FindAnyWidget("WeaponsHeader"));
		m_wCustomizeList = m_wRoot.FindAnyWidget("WeaponCardList");

		//! The legacy pooled preview node stays hidden so the studio render can never double-draw.
		Widget legacyPreview = m_wRoot.FindAnyWidget("SoldierStage");
		if (legacyPreview)
			legacyPreview.SetVisible(false);

		m_wStageWorld = RenderTargetWidget.Cast(m_wRoot.FindAnyWidget("SoldierStageWorld"));
		if (!m_wStageWorld && legacyPreview)
			m_wStageWorld = GRSA_WeaponStage.CreateFallbackRender(legacyPreview.GetParent());
		if (!m_wStageWorld)
			GRSA_Log.Error("Soldier screen: no render widget, mannequin cannot draw");

		m_Stage = GRSA_StageHub.Get().GetSoldier();
		m_ItemList = new GRSA_ItemListPanel(m_wRoot, "ItemListPanel", "ItemListTitle", "SoldierItemList", "SoldierItemScroll", "SoldierSearchBox", "ItemListBackControls", "ItemListFilters");
		m_ItemList.m_OnItemClicked.Insert(OnListRowClicked);
		m_ItemList.m_OnQtyDelta.Insert(OnListQtyDelta);
		m_ItemList.m_OnDone.Insert(OnContentsDone);

		m_WearChip = CreateNavigationButton("MenuSave", "Wear", true, true);
		if (m_WearChip)
			m_WearChip.m_OnActivated.Insert(OnWearChip);
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabShow()
	{
		super.OnTabShow();

		s_ActiveInstance = this;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Insert(OnDraftChanged);
		GetGame().GetInputManager().AddActionListener("MapContextualMenu", EActionTrigger.DOWN, OnContextInput);

		BuildCards();
		RefreshCards();
		GRSA_Theme.Apply(m_wRoot);
		if (m_Stage && service)
		{
			m_Stage.SetVisible(true);
			if (m_wStageWorld)
				m_Stage.Bind(m_wStageWorld);
			m_Stage.RefreshFromDraft(service);
			m_Stage.FrameStation();
		}

		if (!m_aCards.IsEmpty() && m_aCards[0].m_Row && m_aCards[0].m_Row.GetRootWidget())
		{
			//! Seeding pad focus must not hijack the opening shot — the station home frames the
			//! whole character; the first card would zoom the camera into his hat.
			m_bSuppressFocusFrame = true;
			GetGame().GetWorkspace().SetFocusedWidget(m_aCards[0].m_Row.GetRootWidget());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! An open item list folds before the shell may close the editor.
	static bool ConsumeBack()
	{
		GRSA_SoldierScreen screen = s_ActiveInstance;
		if (!screen || !screen.m_ItemList || !screen.m_ItemList.IsOpen())
			return false;

		if (screen.m_ActiveAction && screen.m_ActiveAction.m_eKind == GRSA_ESoldierActionKind.CONTENTS)
			screen.ExitContents();
		else
			screen.CloseItemList();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabHide()
	{
		super.OnTabHide();
		GetGame().GetCallqueue().Remove(ExitContents);
		GetGame().GetCallqueue().Remove(RestoreGearBrowserAfterContents);
		m_PendingFocusCard = null;

		if (s_ActiveInstance == this)
			s_ActiveInstance = null;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Remove(OnDraftChanged);
		GetGame().GetInputManager().RemoveActionListener("MapContextualMenu", EActionTrigger.DOWN, OnContextInput);
		if (m_Stage)
			m_Stage.SetVisible(false);

		CloseItemList(false);
		ClearActions();
		ClearCards();
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabRemove()
	{
		if (m_ItemList)
			m_ItemList.Destroy();
		super.OnTabRemove();
	}

	//! Gear cards mirror the character's actual clothing slots in the configured order.
	protected void BuildCards()
	{
		ClearCards();

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.m_Config)
			return;

		//! Weapons belong to the Gunsmith tab, so this screen builds gear cards only.
		GameEntity character = service.GetLocalCharacter();
		if (!character)
			return;

		array<GRSA_ArmoryCategory> outfitCategories = {};
		service.m_Config.GetCategoriesForTab(GRSA_EArmoryTab.OUTFIT, outfitCategories);
		GRSA_CatalogService.BuildCharacterSlotCategories(outfitCategories, character, service.m_Arsenal, service.GetBrowseFaction());

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
		if (!loadoutStorage)
			return;

		array<string> slotAreas = {};
		int slotsCount = loadoutStorage.GetSlotsCount();
		for (int i = 0; i < slotsCount; ++i)
		{
			string area;
			LoadoutSlotInfo slotInfo = LoadoutSlotInfo.Cast(loadoutStorage.GetSlot(i));
			if (slotInfo && slotInfo.GetAreaType())
				area = slotInfo.GetAreaType().Type().ToString();
			slotAreas.Insert(area);
		}

		foreach (GRSA_ArmoryCategory outfitCategory : outfitCategories)
		{
			if (!outfitCategory || outfitCategory.m_sClothArea.IsEmpty())
				continue;

			int clothingSlot = slotAreas.Find(outfitCategory.m_sClothArea);
			if (clothingSlot < 0)
				continue;

			GRSA_SoldierCard card = new GRSA_SoldierCard();
			card.m_sLabel = outfitCategory.m_sDisplayName;
			card.m_Category = outfitCategory;
			card.m_iClothingSlot = clothingSlot;
			card.m_sArea = outfitCategory.m_sClothArea;
			SpawnCard(card, m_wGearCardList);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnCard(notnull GRSA_SoldierCard card, Widget listParent)
	{
		if (!listParent)
			return;

		Widget rowRoot = GetGame().GetWorkspace().CreateWidgets(CARD_ROW_LAYOUT, listParent);
		if (!rowRoot)
			return;

		GRSA_ItemRowComponent row = GRSA_ItemRowComponent.Cast(rowRoot.FindHandler(GRSA_ItemRowComponent));
		if (!row)
		{
			rowRoot.RemoveFromHierarchy();
			return;
		}

		card.m_Row = row;
		row.EnableSecondaryClick(true);
		row.m_OnEntryClicked.Insert(OnCardClicked);
		row.m_OnEntrySecondaryClicked.Insert(OnCardSecondaryClicked);
		row.m_OnEntryFocused.Insert(OnCardFocused);
		m_aCards.Insert(card);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshCards()
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.m_Draft)
			return;

		SCR_Faction faction = service.GetLocalFaction();
		foreach (GRSA_SoldierCard card : m_aCards)
		{
			if (!card.m_Row)
				continue;

			ResourceName current = CurrentPrefabFor(card, service);
			string label = card.m_sLabel;
			label.ToUpper();

			string state = "EMPTY";
			if (!current.IsEmpty())
				state = GRSA_CatalogService.GetDisplayName(current, faction);

			card.m_Row.SetSlotDisplay(label, state, current);
			bool hasContext = !current.IsEmpty()
				&& (GRSA_ItemIntel.HasContainerStorage(current) || GRSA_ItemIntel.HasVisibleAttachmentSlots(current));
			card.m_Row.SetContextHintVisible(hasContext);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName CurrentPrefabFor(notnull GRSA_SoldierCard card, notnull GRSA_DraftService service)
	{
		if (card.IsWeapon())
		{
			GRSA_KitWeapon weapon = service.m_Draft.FindWeapon(card.m_iWeaponSlot);
			if (weapon)
				return weapon.m_Prefab;
			return ResourceName.Empty;
		}

		GRSA_KitClothing clothing = service.m_Draft.FindClothing(card.m_iClothingSlot);
		if (clothing)
			return clothing.m_Prefab;
		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected GRSA_SoldierCard CardForRow(GRSA_ItemRowComponent row)
	{
		foreach (GRSA_SoldierCard card : m_aCards)
		{
			if (card.m_Row == row)
				return card;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected GRSA_SoldierAction ActionForRow(GRSA_ItemRowComponent row)
	{
		foreach (GRSA_SoldierAction action : m_aActions)
		{
			if (action.m_Row == row)
				return action;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected void RebuildActions(GRSA_SoldierCard card)
	{
		ClearActions();
		if (!card || card.IsWeapon() || !m_wCustomizeList)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.m_Draft)
			return;

		GRSA_KitClothing clothing = service.m_Draft.FindClothing(card.m_iClothingSlot);
		if (!clothing || clothing.m_Prefab.IsEmpty())
			return;

		if (GRSA_ItemIntel.HasContainerStorage(clothing.m_Prefab))
		{
			GRSA_SoldierAction contents = new GRSA_SoldierAction();
			contents.m_eKind = GRSA_ESoldierActionKind.CONTENTS;
			contents.m_sLabel = "CONTENTS";
			SpawnAction(contents, ContainerUsageState(service, clothing.m_Prefab), clothing.m_Prefab);
		}

		if (GRSA_ItemIntel.HasVisibleAttachmentSlots(clothing.m_Prefab))
		{
			GRSA_SoldierAction attachments = new GRSA_SoldierAction();
			attachments.m_eKind = GRSA_ESoldierActionKind.ATTACHMENTS;
			attachments.m_sLabel = "ATTACHMENTS";
			SpawnAction(attachments, "INSPECT", clothing.m_Prefab);
		}

		if (!m_aActions.IsEmpty())
		{
			if (m_wCustomizeHeader)
				m_wCustomizeHeader.SetText("CUSTOMIZE");
			if (m_wCustomizeRail)
				m_wCustomizeRail.SetVisible(true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnAction(notnull GRSA_SoldierAction action, string state, ResourceName thumbnail)
	{
		Widget rowRoot = GetGame().GetWorkspace().CreateWidgets(CARD_ROW_LAYOUT, m_wCustomizeList);
		if (!rowRoot)
			return;

		GRSA_ItemRowComponent row = GRSA_ItemRowComponent.Cast(rowRoot.FindHandler(GRSA_ItemRowComponent));
		if (!row)
		{
			rowRoot.RemoveFromHierarchy();
			return;
		}

		action.m_Row = row;
		row.SetSlotDisplay(action.m_sLabel, state, thumbnail);
		row.m_OnEntryClicked.Insert(OnActionClicked);
		m_aActions.Insert(action);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearActions()
	{
		foreach (GRSA_SoldierAction action : m_aActions)
		{
			if (action.m_Row && action.m_Row.GetRootWidget())
				action.m_Row.GetRootWidget().RemoveFromHierarchy();
		}
		m_aActions.Clear();
		m_ActiveAction = null;
		if (m_wCustomizeRail)
			m_wCustomizeRail.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCardFocused(GRSA_ItemRowComponent row)
	{
		GRSA_SoldierCard card = CardForRow(row);
		if (card && m_PendingFocusCard && card != m_PendingFocusCard)
		{
			GetGame().GetCallqueue().Remove(RestoreGearBrowserAfterContents);
			m_PendingFocusCard = null;
		}

		if (m_ItemList && m_ItemList.IsOpen())
		{
			if (!m_ActiveAction && card && card != m_ActiveCard)
				OnCardClicked(row);
			return;
		}

		if (card != m_FocusedCard)
			ClearActions();
		m_FocusedCard = card;

		if (m_bSuppressFocusFrame)
		{
			m_bSuppressFocusFrame = false;
			return;
		}

		if (card && m_Stage)
			m_Stage.FocusCard(card.IsWeapon(), card.m_iWeaponSlot, card.m_iClothingSlot, card.m_sArea);
	}

	//------------------------------------------------------------------------------------------------
	//! Opens the contextual operations for the worn item under the pointer or pad focus.
	protected void OnCardSecondaryClicked(GRSA_ItemRowComponent row)
	{
		GRSA_SoldierCard card = CardForRow(row);
		if (!card || card.IsWeapon())
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.m_Draft)
			return;

		GRSA_KitClothing clothing = service.m_Draft.FindClothing(card.m_iClothingSlot);
		if (!clothing || clothing.m_Prefab.IsEmpty())
			return;

		CloseItemList(false);
		ClearActions();
		m_FocusedCard = card;
		RebuildActions(card);
		if (m_aActions.Count() == 1)
			OpenAction(card, m_aActions[0]);
		else if (!m_aActions.IsEmpty() && m_aActions[0].m_Row && m_aActions[0].m_Row.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(m_aActions[0].m_Row.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnContextInput(float value, EActionTrigger reason)
	{
		InputManager input = GetGame().GetInputManager();
		if (!input || input.IsUsingMouseAndKeyboard() || !m_FocusedCard || !m_FocusedCard.m_Row)
			return;

		OnCardSecondaryClicked(m_FocusedCard.m_Row);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCardClicked(GRSA_ItemRowComponent row)
	{
		GRSA_SoldierCard card = CardForRow(row);
		if (!card || !m_ItemList)
			return;
		GetGame().GetCallqueue().Remove(RestoreGearBrowserAfterContents);
		m_PendingFocusCard = null;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
			return;

		CloseItemList(false);
		ClearActions();
		m_FocusedCard = card;
		m_ActiveCard = card;
		m_ActiveAction = null;
		m_aActiveItems = GRSA_CatalogService.GetItems(card.m_Category, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType());
		if (!m_aActiveItems)
			return;

		string title = card.m_sLabel;
		title.ToUpper();
		if (m_wCustomizeRail)
			m_wCustomizeRail.SetVisible(false);
		m_ItemList.Open(title, m_aActiveItems, CurrentPrefabFor(card, service), "EQUIPPED", service.UsesSupplies(), false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnActionClicked(GRSA_ItemRowComponent row)
	{
		GRSA_SoldierAction action = ActionForRow(row);
		if (!action || !m_FocusedCard || !m_ItemList)
			return;

		OpenAction(m_FocusedCard, action);
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenAction(notnull GRSA_SoldierCard card, notnull GRSA_SoldierAction action)
	{
		if (!m_ItemList)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.m_Draft)
			return;

		GRSA_KitClothing clothing = service.m_Draft.FindClothing(card.m_iClothingSlot);
		if (!clothing || clothing.m_Prefab.IsEmpty())
			return;

		m_FocusedCard = card;
		m_ActiveCard = null;
		m_ActiveAction = action;

		if (action.m_eKind == GRSA_ESoldierActionKind.ATTACHMENTS)
		{
			if (!GRSA_ShellMenu.OpenGunsmithForClothing(card.m_iClothingSlot))
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK_FAIL);
			return;
		}

		if (!m_aActiveItems)
			m_aActiveItems = new array<ref GRSA_ItemEntry>();
		m_aActiveItems.Clear();

		if (action.m_eKind == GRSA_ESoldierActionKind.CONTENTS)
		{
			CollectContentsItems(service, m_aActiveItems);
			service.SetTargetContainer(clothing.m_Prefab);
			if (m_wCustomizeRail)
				m_wCustomizeRail.SetVisible(false);
			m_ItemList.Open(ContentsTitle(service, clothing.m_Prefab), m_aActiveItems, ResourceName.Empty, string.Empty, service.UsesSupplies(), true);
			RefreshContentsCounts(service, clothing.m_Prefab);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectContentsItems(notnull GRSA_DraftService service, notnull array<ref GRSA_ItemEntry> outItems)
	{
		outItems.Clear();
		map<ResourceName, bool> seen = new map<ResourceName, bool>();
		array<ref GRSA_ItemEntry> tabItems = {};
		GRSA_CatalogService.CollectTabItems(service.m_Config, GRSA_EArmoryTab.GEAR, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType(), tabItems);
		AppendLooseItems(tabItems, outItems, seen);

		tabItems.Clear();
		GRSA_CatalogService.CollectTabItems(service.m_Config, GRSA_EArmoryTab.WEAPONS, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType(), tabItems);
		AppendLooseItems(tabItems, outItems, seen);
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendLooseItems(notnull array<ref GRSA_ItemEntry> source, notnull array<ref GRSA_ItemEntry> destination, notnull map<ResourceName, bool> seen)
	{
		foreach (GRSA_ItemEntry item : source)
		{
			if (!item || seen.Contains(item.m_Prefab) || !GRSA_ItemIntel.GetClothAreaType(item.m_Prefab).IsEmpty())
				continue;

			seen.Insert(item.m_Prefab, true);
			destination.Insert(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseItemList(bool restoreFocus = true)
	{
		if (m_ItemList)
			m_ItemList.Close();

		GRSA_SoldierCard closedCard = m_ActiveCard;
		GRSA_SoldierAction closedAction = m_ActiveAction;
		m_ActiveCard = null;
		m_ActiveAction = null;
		m_aActiveItems = null;

		if (!restoreFocus)
			return;

		if (closedAction && !m_aActions.IsEmpty() && m_wCustomizeRail)
			m_wCustomizeRail.SetVisible(true);

		if (closedAction && closedAction.m_Row && closedAction.m_Row.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(closedAction.m_Row.GetRootWidget());
		else if (closedCard && closedCard.m_Row && closedCard.m_Row.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(closedCard.m_Row.GetRootWidget());
		else if (m_FocusedCard && m_FocusedCard.m_Row && m_FocusedCard.m_Row.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(m_FocusedCard.m_Row.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnContentsDone()
	{
		// Let the Back button finish its click before its owning panel is hidden.
		GetGame().GetCallqueue().Remove(ExitContents);
		GetGame().GetCallqueue().CallLater(ExitContents, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void ExitContents()
	{
		GRSA_SoldierCard origin = m_FocusedCard;
		CloseItemList(false);
		ClearActions();
		m_FocusedCard = origin;
		m_PendingFocusCard = origin;
		GetGame().GetCallqueue().Remove(RestoreGearBrowserAfterContents);
		GetGame().GetCallqueue().CallLater(RestoreGearBrowserAfterContents, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void RestoreGearBrowserAfterContents()
	{
		GRSA_SoldierCard card = m_PendingFocusCard;
		m_PendingFocusCard = null;
		if (!card || m_aCards.Find(card) == -1 || !card.m_Row || !card.m_Row.GetRootWidget())
			return;

		OnCardClicked(card.m_Row);
	}

	//------------------------------------------------------------------------------------------------
	//! Click equips into the card's slot; clicking the equipped item takes it off.
	protected void OnListRowClicked(GRSA_ItemRowComponent row)
	{
		if (!row || !row.GetEntry())
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
			return;

		if (m_ActiveAction)
		{
			if (m_ActiveAction.m_eKind == GRSA_ESoldierActionKind.CONTENTS)
				ChangeContents(row, 1);
			return;
		}

		if (!m_ActiveCard)
			return;

		ResourceName clicked = row.GetEntry().m_Prefab;
		ResourceName wanted = clicked;
		if (clicked == CurrentPrefabFor(m_ActiveCard, service))
			wanted = ResourceName.Empty;

		if (m_ActiveCard.IsWeapon())
			service.SetDraftWeapon(m_ActiveCard.m_iWeaponSlot, wanted);
		else
			service.SetDraftClothing(m_ActiveCard.m_iClothingSlot, wanted);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnListQtyDelta(GRSA_ItemRowComponent row, int delta)
	{
		if (m_ActiveAction && m_ActiveAction.m_eKind == GRSA_ESoldierActionKind.CONTENTS)
			ChangeContents(row, delta);
	}

	//------------------------------------------------------------------------------------------------
	protected void ChangeContents(GRSA_ItemRowComponent row, int delta)
	{
		if (!row || !row.GetEntry() || !m_FocusedCard || !m_ActiveAction)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service || !service.m_Draft)
			return;

		GRSA_KitClothing clothing = service.m_Draft.FindClothing(m_FocusedCard.m_iClothingSlot);
		if (!clothing || clothing.m_Prefab.IsEmpty())
			return;

		GRSA_EExtraChangeResult result = service.ChangeDraftExtraTo(row.GetEntry().m_Prefab, delta, clothing.m_Prefab);
		if (result == GRSA_EExtraChangeResult.ADDED)
		{
			GRSA_ShellMenu.ShowStatus("ITEM ADDED", true);
			return;
		}

		if (result != GRSA_EExtraChangeResult.REMOVED)
		{
			if (m_ItemList)
				m_ItemList.SetTitle(ContentsTitle(service, clothing.m_Prefab, ExtraRejectLabel(result)));
			if (delta > 0)
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.CLICK_FAIL);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected string ContentsTitle(notnull GRSA_DraftService service, ResourceName container, string notice = "")
	{
		string name = service.GetContainerDisplayName(container);
		name.ToUpper();
		string title = string.Format("CONTENTS - %1 - %2", name, ContainerUsageState(service, container));
		if (!notice.IsEmpty())
			title += " - " + notice;
		return title;
	}

	//------------------------------------------------------------------------------------------------
	protected string ContainerUsageState(notnull GRSA_DraftService service, ResourceName container)
	{
		float usedWeight;
		float usedVolume;
		service.GetContainerUsage(container, usedWeight, usedVolume);

		float ratio;
		float maxLoad = GRSA_ItemIntel.GetStorageMaxLoad(container);
		if (maxLoad > 0)
			ratio = usedWeight / maxLoad;

		float maxVolume = GRSA_ItemIntel.GetStorageMaxVolume(container);
		if (maxVolume > 0)
			ratio = Math.Max(ratio, usedVolume / maxVolume);

		ratio = Math.Clamp(ratio, 0, 1);
		return (ratio * 100).ToString(-1, 0) + "% USED";
	}

	//------------------------------------------------------------------------------------------------
	protected string ExtraRejectLabel(GRSA_EExtraChangeResult result)
	{
		switch (result)
		{
			case GRSA_EExtraChangeResult.VOLUME_LIMIT:
				return "NO SPACE";
			case GRSA_EExtraChangeResult.WEIGHT_LIMIT:
				return "TOO HEAVY";
			case GRSA_EExtraChangeResult.INCOMPATIBLE:
				return "DOES NOT FIT";
			case GRSA_EExtraChangeResult.EMPTY:
				return "EMPTY";
		}

		return "UNAVAILABLE";
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshContentsCounts(notnull GRSA_DraftService service, ResourceName container)
	{
		if (!m_ItemList || !service.m_Draft)
			return;

		map<ResourceName, int> counts = new map<ResourceName, int>();
		foreach (GRSA_KitExtra extra : service.m_Draft.m_aExtras)
		{
			if (extra.m_Container == container)
				counts.Set(extra.m_Prefab, extra.m_iCount);
		}
		m_ItemList.SetCounts(counts);
		m_ItemList.SetTitle(ContentsTitle(service, container));
	}

	//------------------------------------------------------------------------------------------------
	//! The one character-mutation verb: WEAR applies the current draft through the server pipeline;
	//! the shell's shared status line reports the verdict.
	protected void OnWearChip(SCR_InputButtonComponent chip, string action)
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.RequestApplyDraft();
	}

	//------------------------------------------------------------------------------------------------
	//! Draft edits refresh cards and preserve the one open interaction without rebuilding it underneath focus.
	protected void OnDraftChanged()
	{
		RefreshCards();

		GRSA_DraftService service = GRSA_DraftService.Get();

		//! Re-frame the open card after an equip so the auto-zoom tracks the item that is
		//! actually worn now — a helmet and a boonie deserve different distances.
		if (m_Stage && m_ActiveCard)
			m_Stage.FocusCard(m_ActiveCard.IsWeapon(), m_ActiveCard.m_iWeaponSlot, m_ActiveCard.m_iClothingSlot, m_ActiveCard.m_sArea);

		if (m_ActiveCard && m_ItemList && service)
			m_ItemList.SetMarked(CurrentPrefabFor(m_ActiveCard, service));

		if (!m_ActiveAction || !service || !m_ItemList || !m_ItemList.IsOpen() || !m_FocusedCard)
			return;

		GRSA_KitClothing clothing = service.m_Draft.FindClothing(m_FocusedCard.m_iClothingSlot);
		if (!clothing || clothing.m_Prefab.IsEmpty())
		{
			CloseItemList(false);
			ClearActions();
			return;
		}

		RefreshContentsCounts(service, clothing.m_Prefab);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearCards()
	{
		foreach (GRSA_SoldierCard card : m_aCards)
		{
			if (card.m_Row && card.m_Row.GetRootWidget())
				card.m_Row.GetRootWidget().RemoveFromHierarchy();
		}
		m_aCards.Clear();
		m_FocusedCard = null;
		m_PendingFocusCard = null;
	}
}
