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

//! SOLDIER tab: the OUTFIT side of the draft — gear slot cards right (one per character
//! clothing slot the station can stock), the draft mannequin center-stage; weapons belong to
//! the GUNSMITH tab. Selecting a card opens its item list over the left side — search on top,
//! click equips into the card's slot, clicking the equipped item takes it off. Card focus
//! reframes the mannequin on the matching body region.
class GRSA_SoldierScreen : SCR_SubMenuBase
{
	protected static const ResourceName CARD_ROW_LAYOUT = "{4A47972BDCB8148E}UI/layouts/Menus/Armory/GRSA_ItemRow.layout";

	protected Widget m_wGearCardList;
	protected RenderTargetWidget m_wStageWorld;

	protected ref GRSA_SoldierStage m_Stage;
	protected ref GRSA_ItemListPanel m_ItemList;
	protected SCR_InputButtonComponent m_WearChip;
	protected ref array<ref GRSA_SoldierCard> m_aCards = {};
	protected GRSA_SoldierCard m_ActiveCard;
	protected ref array<ref GRSA_ItemEntry> m_aActiveItems;
	protected bool m_bSuppressFocusFrame;

	protected static GRSA_SoldierScreen s_ActiveInstance;

	//------------------------------------------------------------------------------------------------
	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);

		m_wGearCardList = m_wRoot.FindAnyWidget("GearCardList");

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
		m_ItemList = new GRSA_ItemListPanel(m_wRoot, "ItemListPanel", "ItemListTitle", "SoldierItemList", "SoldierItemScroll", "SoldierSearchBox");
		m_ItemList.m_OnItemClicked.Insert(OnListRowClicked);

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
		if (!screen || !screen.m_ActiveCard)
			return false;

		screen.CloseItemList();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabHide()
	{
		super.OnTabHide();

		if (s_ActiveInstance == this)
			s_ActiveInstance = null;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.m_OnDraftChanged.Remove(OnDraftChanged);
		if (m_Stage)
			m_Stage.SetVisible(false);

		CloseItemList();
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
		row.m_OnEntryClicked.Insert(OnCardClicked);
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
	protected void OnCardFocused(GRSA_ItemRowComponent row)
	{
		if (m_bSuppressFocusFrame)
		{
			m_bSuppressFocusFrame = false;
			return;
		}

		GRSA_SoldierCard card = CardForRow(row);
		if (card && m_Stage)
			m_Stage.FocusCard(card.IsWeapon(), card.m_iWeaponSlot, card.m_iClothingSlot, card.m_sArea);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCardClicked(GRSA_ItemRowComponent row)
	{
		GRSA_SoldierCard card = CardForRow(row);
		if (!card || !m_ItemList)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
			return;

		m_ActiveCard = card;
		m_aActiveItems = GRSA_CatalogService.GetItems(card.m_Category, service.GetBrowseFaction(), service.m_Arsenal, service.GetCostType());
		if (!m_aActiveItems)
			return;

		string title = card.m_sLabel;
		title.ToUpper();
		m_ItemList.Open(title, m_aActiveItems, CurrentPrefabFor(card, service), "EQUIPPED", service.UsesSupplies());
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseItemList()
	{
		if (m_ItemList)
			m_ItemList.Close();

		GRSA_SoldierCard closedCard = m_ActiveCard;
		m_ActiveCard = null;
		m_aActiveItems = null;

		if (closedCard && closedCard.m_Row && closedCard.m_Row.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(closedCard.m_Row.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	//! Click equips into the card's slot; clicking the equipped item takes it off.
	protected void OnListRowClicked(GRSA_ItemRowComponent row)
	{
		if (!row || !row.GetEntry() || !m_ActiveCard)
			return;

		GRSA_DraftService service = GRSA_DraftService.Get();
		if (!service)
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
	//! The one character-mutation verb: WEAR applies the current draft through the server pipeline;
	//! the shell's shared status line reports the verdict.
	protected void OnWearChip(SCR_InputButtonComponent chip, string action)
	{
		GRSA_DraftService service = GRSA_DraftService.Get();
		if (service)
			service.RequestApplyDraft();
	}

	//------------------------------------------------------------------------------------------------
	//! Draft edits refresh the cards; the mannequin re-dresses itself through the stage host's own
	//! draft subscription. An open list only re-marks its rows.
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
	}
}
