class DCO_VehicleCargoEntry
{
	ResourceName m_Prefab;
	string m_sName;
	int m_iCount;
}

class DCO_VehicleDamageEntry
{
	string m_sName;
	string m_sDetail;
	ResourceName m_IconTexture;
	int m_iHealthPercent;
	int m_iHitZoneIndex = -1;
	int m_iPartCount;
	bool m_bOnFire;
}

class DCO_VehicleAmmoEntry
{
	int m_iCurrent;
	int m_iMaximum;
}

class DCO_VehicleServiceButtonHandler : ScriptedWidgetEventHandler
{
	protected DCO_VehicleServiceMenu m_Menu;
	protected Widget m_Widget;
	protected int m_iAction;

	void DCO_VehicleServiceButtonHandler(DCO_VehicleServiceMenu menu, int action)
	{
		m_Menu = menu;
		m_iAction = action;
	}

	void Attach(Widget widget)
	{
		m_Widget = widget;
		if (m_Widget)
			m_Widget.AddHandler(this);
	}

	void Destroy()
	{
		if (m_Widget)
			m_Widget.RemoveHandler(this);
		m_Widget = null;
		m_Menu = null;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		return m_Menu && m_Menu.OnAction(m_iAction);
	}
}

class DCO_VehicleDamageCalloutLayer
{
	protected static const float DOT_SIZE = 7;
	protected static const float DOT_SIZE_SELECTED = 12;
	protected static const float LINE_WIDTH = 2;

	protected Widget m_Root;
	protected RenderTargetWidget m_Preview;
	protected DCO_VehiclePreviewStage m_Stage;
	protected ref array<Widget> m_Rows = {};
	protected ref array<ImageWidget> m_Dots = {};
	protected ref array<ImageWidget> m_Lines = {};
	protected ref array<int> m_HitZoneIndices = {};
	protected int m_iSelectedHitZone = -1;
	protected int m_iPage;
	protected int m_iPageSize;
	protected bool m_bVisible;

	void DCO_VehicleDamageCalloutLayer(Widget root, RenderTargetWidget preview,
		DCO_VehiclePreviewStage stage, notnull array<Widget> rows)
	{
		m_Root = root;
		m_Preview = preview;
		m_Stage = stage;
		foreach (Widget row : rows)
			m_Rows.Insert(row);

	}

	void Refresh(notnull array<ref DCO_VehicleDamageEntry> entries, int page, int pageSize,
		int selectedHitZone, bool visible)
	{
		m_iSelectedHitZone = selectedHitZone;
		m_iPage = page;
		m_iPageSize = pageSize;
		m_bVisible = visible;
		EnsureVisualCount(entries.Count());
		m_HitZoneIndices.Clear();
		foreach (DCO_VehicleDamageEntry entry : entries)
		{
			if (entry)
				m_HitZoneIndices.Insert(entry.m_iHitZoneIndex);
			else
				m_HitZoneIndices.Insert(-1);
		}
		Reposition();
	}

	void SetRows(notnull array<Widget> rows)
	{
		m_Rows.Clear();
		foreach (Widget row : rows)
			m_Rows.Insert(row);
	}

	void Reposition()
	{
		if (!m_bVisible || !m_Root || !m_Preview || !m_Stage)
		{
			HideAll();
			return;
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		float rootX, rootY, previewX, previewY, previewW, previewH;
		m_Root.GetScreenPos(rootX, rootY);
		m_Preview.GetScreenPos(previewX, previewY);
		m_Preview.GetScreenSize(previewW, previewH);
		float previewOriginX = workspace.DPIUnscale(previewX - rootX);
		float previewOriginY = workspace.DPIUnscale(previewY - rootY);
		float previewWidth = workspace.DPIUnscale(previewW);
		float previewHeight = workspace.DPIUnscale(previewH);

		for (int rowIndex = 0; rowIndex < m_HitZoneIndices.Count(); rowIndex++)
		{
			ImageWidget dot = m_Dots[rowIndex];
			ImageWidget line = m_Lines[rowIndex];
			int visibleRow = rowIndex - m_iPage * m_iPageSize;
			Widget row;
			if (visibleRow >= 0 && visibleRow < m_Rows.Count())
				row = m_Rows[visibleRow];
			int hitZoneIndex = m_HitZoneIndices[rowIndex];
			vector point;
			bool projected = hitZoneIndex >= 0 && m_Stage.ProjectDamagePoint(hitZoneIndex, point)
				&& point[0] >= 0 && point[0] <= previewWidth
				&& point[1] >= 0 && point[1] <= previewHeight;
			if (!projected)
			{
				if (dot)
					dot.SetVisible(false);
				if (line)
					line.SetVisible(false);
				continue;
			}

			float anchorX = previewOriginX + point[0];
			float anchorY = previewOriginY + point[1];
			bool selected = hitZoneIndex == m_iSelectedHitZone;
			if (dot)
			{
				float dotSize = DOT_SIZE;
				Color dotColor = new Color(0.95, 0.95, 0.97, 1);
				if (selected)
				{
					dotSize = DOT_SIZE_SELECTED;
					dotColor = new Color(0.851, 0.537, 0.169, 1);
				}
				dot.SetVisible(true);
				FrameSlot.SetPos(dot, anchorX, anchorY);
				FrameSlot.SetSize(dot, dotSize, dotSize);
				dot.SetColor(dotColor);
			}

			if (!line || !selected || !row || !row.IsVisibleInHierarchy())
			{
				if (line)
					line.SetVisible(false);
				continue;
			}

			float rowX, rowY, rowW, rowH;
			row.GetScreenPos(rowX, rowY);
			row.GetScreenSize(rowW, rowH);
			float chipX = workspace.DPIUnscale(rowX - rootX) - 5;
			float chipY = workspace.DPIUnscale(rowY - rootY + rowH * 0.5);
			float dx = anchorX - chipX;
			float dy = anchorY - chipY;
			float length = Math.Sqrt(dx * dx + dy * dy);
			if (length < 1)
			{
				line.SetVisible(false);
				continue;
			}
			line.SetVisible(true);
			FrameSlot.SetSize(line, length, LINE_WIDTH);
			FrameSlot.SetPos(line, (chipX + anchorX) * 0.5, (chipY + anchorY) * 0.5);
			line.SetRotation(Math.Atan2(dy, dx) * Math.RAD2DEG);
		}
	}

	protected void EnsureVisualCount(int count)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		while (m_Dots.Count() < count)
		{
			m_Dots.Insert(CreateSolidImage(workspace, new Color(0.95, 0.95, 0.97, 1)));
			m_Lines.Insert(CreateSolidImage(workspace, new Color(1, 1, 1, 0.72)));
		}
		while (m_Dots.Count() > count)
		{
			int last = m_Dots.Count() - 1;
			if (m_Dots[last])
				m_Dots[last].RemoveFromHierarchy();
			if (m_Lines[last])
				m_Lines[last].RemoveFromHierarchy();
			m_Dots.Remove(last);
			m_Lines.Remove(last);
		}
	}

	void Destroy()
	{
		foreach (ImageWidget dot : m_Dots)
		{
			if (dot)
				dot.RemoveFromHierarchy();
		}
		foreach (ImageWidget line : m_Lines)
		{
			if (line)
				line.RemoveFromHierarchy();
		}
		m_Dots.Clear();
		m_Lines.Clear();
		m_Rows.Clear();
		m_HitZoneIndices.Clear();
		m_Root = null;
		m_Preview = null;
		m_Stage = null;
	}

	protected void HideAll()
	{
		foreach (ImageWidget dot : m_Dots)
		{
			if (dot)
				dot.SetVisible(false);
		}
		foreach (ImageWidget line : m_Lines)
		{
			if (line)
				line.SetVisible(false);
		}
	}

	protected ImageWidget CreateSolidImage(WorkspaceWidget workspace, Color color)
	{
		if (!workspace || !m_Root)
			return null;
		int flags = WidgetFlags.VISIBLE | WidgetFlags.NOFOCUS | WidgetFlags.IGNORE_CURSOR;
		ImageWidget image = ImageWidget.Cast(workspace.CreateWidget(
			WidgetType.ImageWidgetTypeID, flags, null, 0, m_Root));
		if (!image)
			return null;
		FrameSlot.SetAlignment(image, 0.5, 0.5);
		image.SetColor(color);
		image.SetVisible(false);
		return image;
	}
}

class DCO_VehicleServiceMenu : ChimeraMenuBase
{
	protected static const ResourceName GRS_GUNSMITH_SCREEN = "{AB205AD4E2000004}UI/layouts/Menus/ArmoryV2/GRSA_ScreenGunsmith.layout";
	protected static const ResourceName GRS_ITEM_LIST_PANEL = "{6A4338ABF0020001}UI/layouts/Menus/ArmoryV2/GRSA_ItemListPanel.layout";
	protected static const ResourceName GRS_TAB = "{AB205AD4E2000002}UI/layouts/Menus/ArmoryV2/GRSA_ShellTab.layout";
	protected static const ResourceName GRS_ROW = "{4A47972BDCB8148E}UI/layouts/Menus/Armory/GRSA_ItemRow.layout";
	protected static const ResourceName GRS_TILE = "{AC7DEE9615659D4F}UI/layouts/Menus/ArmoryV2/GRSA_AttachmentTile.layout";
	protected static const ResourceName GRS_CHIP = "{ABEECB4C3A6F5958}UI/layouts/Menus/Armory/GRSA_CategoryButton.layout";
	protected static const ResourceName GRS_SERVICE_PROGRESS = "{B1F05E4C09010001}UI/layouts/Menus/ArmoryV2/GRSA_ServiceProgress.layout";
	protected static const ResourceName ICON_REPAIR = "{5C14C357485DEC46}img/icons/ars-reset.edds";
	protected static const ResourceName ICON_REARM = "{3BEAB520D1630ED6}img/icons/ars-load.edds";
	protected static const ResourceName ICON_CARGO = "{24C2C142CE0F1758}img/icons/ars-crate.edds";
	protected static const ResourceName ICON_REFUEL = "{AAD445F8B7A13462}UI/Textures/Editor/EditableEntities/Systems/EditableEntity_Refuel_Station.edds";
	protected static const ResourceName ICON_SYSTEM_TRANSMISSION = "{D49249938B61BE1C}img/icons/vehicle-service/vehicle-transmission.edds";
	protected static const ResourceName ICON_SYSTEM_HULL = "{D8F13B5474777691}img/icons/vehicle-service/vehicle-hull.edds";
	protected static const ResourceName ICON_SYSTEM_ENGINE = "{CEB2A057402B80E7}img/icons/vehicle-service/vehicle-engine.edds";
	protected static const ResourceName ICON_SYSTEM_LIGHTING = "{45790620BB67F331}img/icons/vehicle-service/vehicle-lighting.edds";
	protected static const ResourceName ICON_SYSTEM_ELECTRICAL = "{21712F0B491AC600}img/icons/vehicle-service/vehicle-electrical.edds";
	protected static const ResourceName ICON_SYSTEM_FUEL = "{9A05C069C402FB1F}img/icons/vehicle-service/vehicle-fuel-system.edds";
	protected static const ResourceName ICON_SYSTEM_ARMAMENT = "{F0D196AFA136AF40}img/icons/vehicle-service/vehicle-armament.edds";
	protected static const ResourceName ICON_SYSTEM_WHEELS = "{C975C3F1F665C44B}img/icons/vehicle-service/vehicle-wheels.edds";
	protected static const ResourceName ICON_SYSTEM_OTHER = "{68F327B9C168186E}UI/Textures/FieldManual/Gameplay/Damage/Tiles/vehicle-damage_ui.edds";
	protected static const ResourceName SUPPORT_ACP = "{9DD9C6279F4489B4}Sounds/SupportStations/SupportStations_Vehicles.acp";
	protected static const string SOUND_REPAIR = "SOUND_VEHICLE_REPAIR_PARTIAL";
	protected static const string SOUND_REPAIR_DONE = "SOUND_VEHICLE_REPAIR_DONE";
	protected static const string SOUND_REFUEL = "SOUND_VEHICLE_REFUEL";
	protected static const string SOUND_REFUEL_DONE = "SOUND_VEHICLE_REFUEL_DONE";
	protected static const string SOUND_REARM = "SOUND_SUPPLIES_PARTIAL_LOAD";
	protected static const float AUTHORITY_TIMEOUT_SECONDS = 12;
	protected static const int VEHICLE_ROWS = 5;
	protected static const int DATA_ROWS = 7;
	protected static const int CARGO_ROWS = 6;
	protected static const int CATALOG_ROWS = 6;
	protected static const int MODE_SERVICE = 0;
	protected static const int MODE_CARGO = 1;
	protected static const int ACTION_CLOSE = 1;
	protected static const int ACTION_REPAIR = 2;
	protected static const int ACTION_REFUEL = 3;
	protected static const int ACTION_REARM = 4;
	protected static const int ACTION_FULL = 5;
	protected static const int ACTION_PREVIEW_RESET = 6;
	protected static const int ACTION_TAB_SERVICE = 7;
	protected static const int ACTION_TAB_CARGO = 9;
	protected static const int ACTION_VEHICLE_PREV = 10;
	protected static const int ACTION_VEHICLE_NEXT = 11;
	protected static const int ACTION_DATA_PREV = 12;
	protected static const int ACTION_DATA_NEXT = 13;
	protected static const int ACTION_CARGO_PREV = 20;
	protected static const int ACTION_CARGO_NEXT = 21;
	protected static const int ACTION_CATALOG_PREV = 30;
	protected static const int ACTION_CATALOG_NEXT = 31;
	protected static const int ACTION_VEHICLE_ROW = 100;
	protected static const int ACTION_CARGO_ROW = 200;
	protected static const int ACTION_CATALOG_ROW = 300;
	protected static const int ACTION_DAMAGE_ROW = 500;

	protected static DCO_VehicleServiceMenu s_Current;
	protected static DCO_VehicleServiceZoneComponent s_QueuedZone;
	protected static IEntity s_QueuedUser;
	protected static bool s_bOpenQueued;
	protected static int s_iNextRequestId = 1;
	protected DCO_VehicleServiceZoneComponent m_Zone;
	protected IEntity m_User;
	protected IEntity m_SelectedVehicle;
	protected ResourceName m_PreviewVehiclePrefab;
	protected IEntity m_ServiceVehicle;
	protected Widget m_Root;
	protected RenderTargetWidget m_PreviewRender;
	protected Widget m_DamagePanel;
	protected Widget m_CargoPanel;
	protected Widget m_DataPager;
	protected Widget m_ProgressPanel;
	protected Widget m_ServiceProgressModal;
	protected Widget m_ServiceProgressBorder;
	protected TextWidget m_VehiclePage;
	protected TextWidget m_DataPage;
	protected TextWidget m_CargoPage;
	protected TextWidget m_CatalogPage;
	protected TextWidget m_DamageTitle;
	protected TextWidget m_CargoTitle;
	protected TextWidget m_DamageDetail;
	protected TextWidget m_Status;
	protected TextWidget m_ProgressText;
	protected TextWidget m_ServiceProgressOperation;
	protected TextWidget m_ServiceProgressPercent;
	protected TextWidget m_ServiceProgressRemaining;
	protected TextWidget m_TabDamageLabel;
	protected TextWidget m_TabCargoLabel;
	protected ProgressBarWidget m_Progress;
	protected ProgressBarWidget m_ServiceProgressBar;
	protected Widget m_ServiceProgressIconHost;
	protected ImageWidget m_ServiceProgressIcon;
	protected EditBoxWidget m_Search;
	protected ref DCO_VehiclePreviewStage m_PreviewStage;
	protected ref DCO_VehicleDamageCalloutLayer m_DamageCallouts;
	protected ref array<IEntity> m_Vehicles = {};
	protected ref array<ref DCO_VehicleCargoEntry> m_Cargo = {};
	protected ref array<ref DCO_VehicleDamageEntry> m_Damage = {};
	protected ref array<ref DCO_VehicleAmmoEntry> m_Ammo = {};
	protected ref array<DCO_ArsenalEntry> m_Catalog = {};
	protected ref array<ref GRSA_ItemEntry> m_CargoBrowseItems = {};
	protected ref array<ButtonWidget> m_VehicleButtons = {};
	protected ref array<TextWidget> m_VehicleLabels = {};
	protected ref array<TextWidget> m_DamageLabels = {};
	protected ref array<TextWidget> m_DamageValues = {};
	protected ref array<Widget> m_DamageRows = {};
	protected ref array<TextWidget> m_AmmoLabels = {};
	protected ref array<TextWidget> m_AmmoValues = {};
	protected ref array<ButtonWidget> m_CargoButtons = {};
	protected ref array<TextWidget> m_CargoLabels = {};
	protected ref array<ButtonWidget> m_CatalogButtons = {};
	protected ref array<TextWidget> m_CatalogLabels = {};
	protected ref array<ref DCO_VehicleServiceButtonHandler> m_Handlers = {};
	protected ref array<ref DCO_VehicleServiceButtonHandler> m_FooterHandlers = {};
	protected int m_iMode = MODE_SERVICE;
	protected int m_iVehiclePage;
	protected int m_iDamagePage;
	protected int m_iCargoPage;
	protected int m_iCatalogPage;
	protected int m_iSelectedDamageHitZone = -1;
	protected int m_iLastRefresh;
	protected int m_iPendingRequestId;
	protected int m_iPendingVerb;
	protected int m_iActiveVerb;
	protected int m_iServiceCapabilities = -1;
	protected int m_iActiveServiceCapabilities;
	protected IEntity m_PendingServiceVehicle;
	protected float m_fServiceElapsed;
	protected float m_fServiceDuration;
	protected float m_fAuthorityWaitElapsed;
	protected float m_fPendingResponseElapsed;
	protected float m_fNextServiceAudioAttempt;
	protected int m_iServiceAudioPhase;
	protected int m_iServiceIconPhase;
	protected bool m_bServiceTimerComplete;
	protected string m_sLastSearch;
	protected string m_sRenderSignature;
	protected bool m_bCloseInputHeld;
	protected ResourceName m_CargoContainerPrefab;
	protected bool m_bClosed;
	protected bool m_bInitialized;
	protected AudioHandle m_ServiceAudio = AudioHandle.Invalid;
	protected Widget m_GRSAScreen;
	protected Widget m_GRSADataList;
	protected Widget m_GRSACandidateList;
	protected Widget m_GRSACandidatePanel;
	protected Widget m_GRSACandidateCarousel;
	protected Widget m_GRSAReceiverCard;
	protected Widget m_GRSATabs;
	protected Widget m_GRSACandidateClasses;
	protected Widget m_GRSAFooter;
	protected TextWidget m_GRSAHeaderTitle;
	protected TextWidget m_GRSAHeaderStatus;
	protected TextWidget m_GRSAHardpointCounter;
	protected TextWidget m_GRSAStatsTitle;
	protected RichTextWidget m_GRSAStatsText;
	protected Widget m_GRSAStatsIconRow;
	protected TextWidget m_GRSAStatsDamageValue;
	protected TextWidget m_GRSAStatsAmmoValue;
	protected TextWidget m_GRSAStatsCargoValue;
	protected TextWidget m_GRSACandidateTitle;
	protected TextWidget m_GRSAProgressValue;
	protected SCR_SliderComponent m_GRSAProgressSlider;
	protected GRSA_CarouselComponent m_GRSACarousel;
	protected Widget m_CargoBrowserRoot;
	protected ref GRSA_ItemListPanel m_CargoBrowser;
	protected ref array<GRSA_ItemRowComponent> m_GRSADataRows = {};
	protected ref array<int> m_GRSADataActions = {};
	protected ref array<GRSA_ItemRowComponent> m_GRSACandidateRows = {};
	protected ref array<int> m_GRSACandidateActions = {};
	protected ref array<SCR_ButtonTextComponent> m_GRSAModeChips = {};

	static bool IsServiceOpen()
	{
		return s_Current != null;
	}

	static void ShutdownForGameEnd()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(CompleteQueuedOpen);
		s_QueuedZone = null;
		s_QueuedUser = null;
		s_bOpenQueued = false;

		DCO_VehicleServiceMenu current = s_Current;
		if (current)
		{
			current.CleanupOwnedResources(false);
			DCO_GMUIController.ReleaseMenuFocus();
		}
	}

	static bool Open(DCO_VehicleServiceZoneComponent zone, IEntity user)
	{
		DCO_VehicleServiceAccessComponent access;
		if (zone)
			access = zone.GetAccess();
		if (!zone || !access || !DCO_VehicleServiceAccessComponent.IsUsableBy(access.GetOwner(), user))
			return false;
		if (s_Current)
			return true;
		SCR_ConfigurableDialogUi current = SCR_ConfigurableDialogUi.GetCurrentDialog();
		if (current)
		{
			current.Close();
			if (!s_bOpenQueued)
			{
				s_QueuedZone = zone;
				s_QueuedUser = user;
				s_bOpenQueued = true;
				GetGame().GetCallqueue().CallLater(CompleteQueuedOpen, 50, false);
			}
			return true;
		}
		return CreateMenu(zone, user);
	}

	protected static void CompleteQueuedOpen()
	{
		DCO_VehicleServiceZoneComponent zone = s_QueuedZone;
		IEntity user = s_QueuedUser;
		s_QueuedZone = null;
		s_QueuedUser = null;
		s_bOpenQueued = false;
		if (!zone || !user || s_Current)
			return;
		CreateMenu(zone, user);
	}

	protected static bool CreateMenu(DCO_VehicleServiceZoneComponent zone, IEntity user)
	{
		DCO_VehicleServiceAccessComponent access;
		if (zone)
			access = zone.GetAccess();
		if (!zone || !user || !access || !DCO_VehicleServiceAccessComponent.IsUsableBy(access.GetOwner(), user))
			return false;
		MenuManager manager = GetGame().GetMenuManager();
		if (!manager)
			return false;

		s_QueuedZone = zone;
		s_QueuedUser = user;
		MenuBase menu = manager.OpenMenu(ChimeraMenuPreset.DCO_VehicleService);
		if (menu)
			return true;

		s_QueuedZone = null;
		s_QueuedUser = null;
		return false;
	}

	static void OnAuthorityResult(int requestId, bool success, string result)
	{
		if (!s_Current || requestId != s_Current.m_iPendingRequestId)
			return;
		int completedVerb = s_Current.m_iPendingVerb;
		if (s_Current.m_iActiveVerb)
			s_Current.FinishServiceProgress();
		s_Current.m_iPendingRequestId = 0;
		s_Current.m_iPendingVerb = 0;
		s_Current.m_PendingServiceVehicle = null;
		s_Current.m_fPendingResponseElapsed = 0;
		s_Current.SetStatus(success, result);
		if (success)
			s_Current.PlayCompletionSound(completedVerb);
		GetGame().GetCallqueue().Remove(s_Current.RefreshAfterAuthority);
		GetGame().GetCallqueue().CallLater(s_Current.RefreshAfterAuthority, 120, false);
	}

	static void OnAuthorityStarted(int requestId, int verb, int durationMs, int capabilities)
	{
		if (!s_Current || requestId != s_Current.m_iPendingRequestId
			|| verb != s_Current.m_iPendingVerb)
			return;
		if (durationMs <= 0 || !DCO_VehicleServiceServer.SupportsVerb(capabilities, verb))
		{
			s_Current.m_iPendingRequestId = 0;
			s_Current.m_iPendingVerb = 0;
			s_Current.m_PendingServiceVehicle = null;
			s_Current.SetStatus(false, "The server returned an invalid service authorization.");
			return;
		}

		s_Current.m_iActiveVerb = verb;
		s_Current.m_iActiveServiceCapabilities = capabilities;
		s_Current.m_ServiceVehicle = s_Current.m_PendingServiceVehicle;
		s_Current.m_fServiceElapsed = 0;
		s_Current.m_fServiceDuration = durationMs * 0.001;
		s_Current.m_fAuthorityWaitElapsed = 0;
		s_Current.m_fPendingResponseElapsed = 0;
		s_Current.m_fNextServiceAudioAttempt = 0;
		s_Current.m_iServiceAudioPhase = 0;
		s_Current.m_iServiceIconPhase = 0;
		s_Current.m_bServiceTimerComplete = false;
		if (s_Current.m_ServiceProgressModal)
			s_Current.m_ServiceProgressModal.SetVisible(true);
		s_Current.UpdateProgressText();
		s_Current.UpdateServiceAudio(true);
		s_Current.SetStatus(true, s_Current.VerbName(verb)
			+ " authorized by server. Stay outside, nearby, and keep the vehicle stopped.");
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_Zone = s_QueuedZone;
		m_User = s_QueuedUser;
		s_QueuedZone = null;
		s_QueuedUser = null;
		m_bClosed = false;
		m_bInitialized = false;
		s_Current = this;
		m_Root = GetRootWidget();
		if (!m_Root || !m_Zone || !m_User)
		{
			Close();
			return;
		}
		PrepareShellHost();
		m_Root.SetVisible(true);
		GetGame().GetCallqueue().CallLater(CompleteMenuOpen, 1, false);
	}

	protected void PrepareShellHost()
	{
		ScriptedWidgetEventHandler shellController = m_Root.FindHandler(SCR_SuperMenuComponent);
		if (shellController)
			m_Root.RemoveHandler(shellController);

		Widget tabView = m_Root.FindAnyWidget("TabView");
		if (!tabView)
			return;

		ScriptedWidgetEventHandler tabController = tabView.FindHandler(GRSA_ArsenalTabViewComponent);
		if (tabController)
			tabView.RemoveHandler(tabController);
	}

	protected void CompleteMenuOpen()
	{
		if (m_bClosed || !m_Root || !m_Zone || !m_User)
			return;

		DCO_VehicleServiceAccessComponent access = m_Zone.GetAccess();
		if (!access || !DCO_VehicleServiceAccessComponent.IsUsableBy(access.GetOwner(), m_User))
		{
			Close();
			return;
		}

		GRSA_Theme.Apply(m_Root);
		m_GRSAHeaderTitle = TextWidget.Cast(m_Root.FindAnyWidget("HeaderTitle"));
		m_GRSAHeaderStatus = TextWidget.Cast(m_Root.FindAnyWidget("ShellStatus"));
		m_Status = m_GRSAHeaderStatus;
		if (m_GRSAHeaderTitle)
			m_GRSAHeaderTitle.SetText("SERVICE BAY");
		if (m_GRSAHeaderStatus)
			m_GRSAHeaderStatus.SetVisible(true);
		TextWidget supply = TextWidget.Cast(m_Root.FindAnyWidget("HeaderSupply"));
		TextWidget weight = TextWidget.Cast(m_Root.FindAnyWidget("HeaderWeight"));
		if (supply)
			supply.SetVisible(false);
		if (weight)
			weight.SetVisible(false);

		Bind("ExitButton", ACTION_CLOSE);
		m_GRSATabs = m_Root.FindAnyWidget("Tabs");
		ClearChildren(m_GRSATabs);
		CreateModeChip("REPAIR", ACTION_TAB_SERVICE);
		CreateModeChip("CARGO", ACTION_TAB_CARGO);
		Widget pagingLeft = m_Root.FindAnyWidget("PagingLeft");
		Widget pagingRight = m_Root.FindAnyWidget("PagingRight");
		if (pagingLeft)
			pagingLeft.SetVisible(false);
		if (pagingRight)
			pagingRight.SetVisible(false);

		Widget content = m_Root.FindAnyWidget("ContentOverlay");
		ClearChildren(content);
		if (content)
			m_GRSAScreen = GetGame().GetWorkspace().CreateWidgets(GRS_GUNSMITH_SCREEN, content);
		if (!m_GRSAScreen)
		{
			Close();
			return;
		}
		AlignableSlot.SetHorizontalAlign(m_GRSAScreen, LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetVerticalAlign(m_GRSAScreen, LayoutVerticalAlign.Stretch);
		AlignableSlot.SetPadding(m_GRSAScreen, 0, 0, 0, 0);
		m_GRSAScreen.Update();
		m_PreviewRender = RenderTargetWidget.Cast(m_GRSAScreen.FindAnyWidget("StageWorld"));
		m_GRSADataList = m_GRSAScreen.FindAnyWidget("HardpointRailList");
		m_GRSAHardpointCounter = TextWidget.Cast(m_GRSAScreen.FindAnyWidget("HardpointCounter"));
		m_GRSACandidatePanel = m_GRSAScreen.FindAnyWidget("CandidatesPanel");
		m_GRSACandidateList = m_GRSAScreen.FindAnyWidget("CandidatesList");
		m_GRSACandidateClasses = m_GRSAScreen.FindAnyWidget("BrowserClasses");
		m_GRSACandidateTitle = TextWidget.Cast(m_GRSAScreen.FindAnyWidget("CandidatesTitle"));
		m_GRSACandidateCarousel = m_GRSAScreen.FindAnyWidget("CandidatesCarousel");
		Widget candidateScroll = m_GRSAScreen.FindAnyWidget("CandidatesScroll");
		if (candidateScroll)
			m_GRSACarousel = GRSA_CarouselComponent.Cast(candidateScroll.FindHandler(GRSA_CarouselComponent));
		m_GRSAReceiverCard = m_GRSAScreen.FindAnyWidget("ReceiverCard");
		m_GRSAStatsTitle = TextWidget.Cast(m_GRSAScreen.FindAnyWidget("StatsTitle"));
		m_GRSAStatsText = RichTextWidget.Cast(m_GRSAScreen.FindAnyWidget("StatsText"));
		m_GRSAStatsIconRow = m_GRSAScreen.FindAnyWidget("StatsIconRow");
		m_GRSAStatsDamageValue = TextWidget.Cast(m_GRSAScreen.FindAnyWidget("StatsDamageValue"));
		m_GRSAStatsAmmoValue = TextWidget.Cast(m_GRSAScreen.FindAnyWidget("StatsAmmoValue"));
		m_GRSAStatsCargoValue = TextWidget.Cast(m_GRSAScreen.FindAnyWidget("StatsCargoValue"));
		BindStatsIcon("StatsDamageIcon", ICON_REPAIR);
		BindStatsIcon("StatsAmmoIcon", ICON_REARM);
		BindStatsIcon("StatsCargoIcon", ICON_CARGO);
		DarkenPanel("StatsBg");
		DarkenPanel("ReceiverBg");
		DarkenPanel("CandidatesBg");
		SizeLayoutWidget hardpointRail = SizeLayoutWidget.Cast(m_GRSAScreen.FindAnyWidget("HardpointRail"));
		if (hardpointRail)
		{
			hardpointRail.SetWidthOverride(500);
			AlignableSlot.SetPadding(hardpointRail, 0, 82, 24, 178);
		}
		Widget hardpointCounter = m_GRSAScreen.FindAnyWidget("HardpointCounter");
		if (hardpointCounter)
			AlignableSlot.SetPadding(hardpointCounter, 0, 16, 32, 0);
		if (m_GRSACandidatePanel)
			AlignableSlot.SetPadding(m_GRSACandidatePanel, 48, 0, 408, 64);
		CreateCargoBrowseControls();
		CreateCargoBrowser();
		m_ProgressPanel = m_GRSAScreen.FindAnyWidget("PositionRow");
		m_ProgressText = TextWidget.Cast(m_GRSAScreen.FindAnyWidget("PositionLabel"));
		m_GRSAProgressValue = TextWidget.Cast(m_GRSAScreen.FindAnyWidget("PositionValue"));
		Widget progressSlider = m_GRSAScreen.FindAnyWidget("PositionSlider");
		if (progressSlider)
		{
			m_GRSAProgressSlider = SCR_SliderComponent.Cast(progressSlider.FindHandler(SCR_SliderComponent));
			progressSlider.SetEnabled(false);
		}
		if (m_GRSAReceiverCard)
			m_GRSAReceiverCard.SetVisible(false);

		m_GRSAFooter = m_Root.FindAnyWidget("Footer");
		ClearChildren(m_GRSAFooter);

		if (m_ProgressPanel)
			m_ProgressPanel.SetVisible(false);
		CreateServiceProgressModal();
		m_PreviewStage = new DCO_VehiclePreviewStage();
		DCO_ArsenalCatalog.Get().Build();
		BuildCatalog();
		RefreshAll();
		RefreshMode();
		if (m_Vehicles.IsEmpty())
			SetStatus(false, "NO VEHICLE IN BAY");
		else
			SetStatus(true, "READY - SELECT A SERVICE TAB");
		m_bInitialized = true;
	}

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (m_bClosed)
			return;
		if (!m_bInitialized)
			return;

		InputManager input = GetGame().GetInputManager();
		if (input)
		{
			input.ActivateContext("MenuContext");
			input.ActivateContext("InventoryMenuContext");
			input.ActivateContext("GRSA_ArmoryContext");
			bool closeHeld = input.GetActionValue("MenuBack") > 0 || input.GetActionValue("MenuOpen") > 0;
#ifdef WORKBENCH
			closeHeld = closeHeld || input.GetActionValue("MenuBackWB") > 0 || input.GetActionValue("MenuOpenWB") > 0;
#endif
			if (closeHeld && !m_bCloseInputHeld)
			{
				if (m_iActiveVerb || m_iPendingRequestId)
				{
					CancelServiceOperation("Cancelled by operator.");
					Close();
					return;
				}
				if (m_iMode == MODE_CARGO)
				{
					m_iMode = MODE_SERVICE;
					RefreshMode();
					m_sRenderSignature = BuildRenderSignature();
					m_bCloseInputHeld = closeHeld;
					return;
				}
				Close();
				return;
			}
			m_bCloseInputHeld = closeHeld;
		}

		UpdateServiceOperation(tDelta);
		if (m_iActiveVerb)
			return;
		UpdatePendingRequestTimeout(tDelta);

		GRSA_SmoothScrollComponent.TickAll(tDelta);
		GRSA_CarouselComponent.TickAll(tDelta);
		if (m_PreviewStage)
			m_PreviewStage.Tick(tDelta);
		if (m_DamageCallouts)
			m_DamageCallouts.Reposition();
		DCO_VehicleServiceAccessComponent access;
		if (m_Zone)
			access = m_Zone.GetAccess();
		if (!m_Zone || !m_Zone.GetOwner() || !m_User || !access
			|| !DCO_VehicleServiceAccessComponent.IsUsableBy(access.GetOwner(), m_User))
		{
			Close();
			return;
		}

		string search;
		if (m_Search)
			search = m_Search.GetText();
		if (search != m_sLastSearch)
		{
			m_sLastSearch = search;
			m_iCatalogPage = 0;
			BuildCatalog();
			RefreshCatalogRows();
		}

		int now = System.GetTickCount();
		if (now - m_iLastRefresh >= 500)
		{
			m_iLastRefresh = now;
			RefreshAll();
		}
	}

	protected void CleanupOwnedResources(bool cancelAuthority = true)
	{
		if (m_bClosed)
			return;
		m_bClosed = true;
		m_bInitialized = false;
		if (s_Current == this)
			s_Current = null;
		GetGame().GetCallqueue().Remove(CompleteQueuedOpen);
		GetGame().GetCallqueue().Remove(CompleteMenuOpen);
		GetGame().GetCallqueue().Remove(RefreshAfterAuthority);
		if (cancelAuthority)
			CancelServiceOperation(string.Empty);
		else
		{
			StopLoopSound();
			m_iPendingRequestId = 0;
			m_iPendingVerb = 0;
			m_iActiveVerb = 0;
			m_iActiveServiceCapabilities = 0;
			m_PendingServiceVehicle = null;
			m_ServiceVehicle = null;
		}
		if (m_DamageCallouts)
			m_DamageCallouts.Destroy();
		m_DamageCallouts = null;
		if (m_PreviewStage)
			m_PreviewStage.Destroy();
		m_PreviewStage = null;
		if (m_GRSACarousel)
			m_GRSACarousel.Clear();
		if (m_CargoBrowser)
		{
			m_CargoBrowser.m_OnItemClicked.Remove(OnCargoBrowserClicked);
			m_CargoBrowser.m_OnQtyDelta.Remove(OnCargoBrowserQuantity);
			m_CargoBrowser.m_OnDone.Remove(OnCargoBrowserDone);
			m_CargoBrowser.Destroy();
		}
		ClearGRSADataRows();
		ClearGRSACandidateRows();
		foreach (DCO_VehicleServiceButtonHandler handler : m_Handlers)
		{
			if (handler)
				handler.Destroy();
		}
		m_Handlers.Clear();
		foreach (DCO_VehicleServiceButtonHandler footerHandler : m_FooterHandlers)
		{
			if (footerHandler)
				footerHandler.Destroy();
		}
		m_FooterHandlers.Clear();
		if (m_ServiceProgressModal)
			m_ServiceProgressModal.RemoveFromHierarchy();
		m_ServiceProgressModal = null;
		m_ServiceProgressBorder = null;
		m_ServiceProgressOperation = null;
		m_ServiceProgressPercent = null;
		m_ServiceProgressRemaining = null;
		m_ServiceProgressBar = null;
		m_ServiceProgressIconHost = null;
		m_ServiceProgressIcon = null;
		ClearChildren(m_GRSATabs);
		ClearChildren(m_GRSAFooter);
		if (m_GRSAScreen)
			m_GRSAScreen.RemoveFromHierarchy();
		m_VehicleButtons.Clear();
		m_VehicleLabels.Clear();
		m_DamageLabels.Clear();
		m_DamageValues.Clear();
		m_DamageRows.Clear();
		m_AmmoLabels.Clear();
		m_AmmoValues.Clear();
		m_CargoButtons.Clear();
		m_CargoLabels.Clear();
		m_CatalogButtons.Clear();
		m_CatalogLabels.Clear();
		m_Vehicles.Clear();
		m_Damage.Clear();
		m_Ammo.Clear();
		m_Cargo.Clear();
		m_Catalog.Clear();
		m_CargoBrowseItems.Clear();
		m_GRSAModeChips.Clear();
		m_GRSAScreen = null;
		m_GRSADataList = null;
		m_GRSACandidateList = null;
		m_GRSACandidatePanel = null;
		m_GRSACandidateCarousel = null;
		m_GRSAReceiverCard = null;
		m_GRSATabs = null;
		m_GRSACandidateClasses = null;
		m_GRSAFooter = null;
		m_GRSAHeaderTitle = null;
		m_GRSAHeaderStatus = null;
		m_GRSAHardpointCounter = null;
		m_GRSAStatsTitle = null;
		m_GRSAStatsText = null;
		m_GRSAStatsIconRow = null;
		m_GRSAStatsDamageValue = null;
		m_GRSAStatsAmmoValue = null;
		m_GRSAStatsCargoValue = null;
		m_GRSACandidateTitle = null;
		m_GRSAProgressValue = null;
		m_GRSAProgressSlider = null;
		m_GRSACarousel = null;
		m_CargoBrowser = null;
		m_CargoBrowserRoot = null;
		m_PreviewRender = null;
		m_Root = null;
		m_Zone = null;
		m_User = null;
		m_SelectedVehicle = null;
		m_PreviewVehiclePrefab = ResourceName.Empty;
		m_ServiceVehicle = null;
		m_bCloseInputHeld = false;
		m_iSelectedDamageHitZone = -1;
	}

	override void OnMenuClose()
	{
		if (m_bClosed)
			return;
		CleanupOwnedResources();
		DCO_GMUIController.ReleaseMenuFocus();
		super.OnMenuClose();
	}

	protected void ClearChildren(Widget parent)
	{
		if (!parent)
			return;
		Widget child = parent.GetChildren();
		while (child)
		{
			Widget next = child.GetSibling();
			child.RemoveFromHierarchy();
			child = next;
		}
	}

	protected void DarkenPanel(string widgetName)
	{
		if (!m_GRSAScreen)
			return;
		Widget background = m_GRSAScreen.FindAnyWidget(widgetName);
		if (background)
			background.SetColor(Color.FromRGBA(3, 3, 4, 245));
	}

	protected void CreateServiceProgressModal()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace || !m_Root)
			return;
		m_ServiceProgressModal = workspace.CreateWidgets(GRS_SERVICE_PROGRESS, m_Root);
		if (!m_ServiceProgressModal)
			return;
		FrameSlot.SetAnchorMin(m_ServiceProgressModal, 0, 0);
		FrameSlot.SetAnchorMax(m_ServiceProgressModal, 1, 1);
		FrameSlot.SetOffsets(m_ServiceProgressModal, 0, 0, 0, 0);
		m_ServiceProgressBorder = m_ServiceProgressModal.FindAnyWidget("ServiceProgressBorder");
		m_ServiceProgressOperation = TextWidget.Cast(m_ServiceProgressModal.FindAnyWidget("ServiceProgressOperation"));
		m_ServiceProgressPercent = TextWidget.Cast(m_ServiceProgressModal.FindAnyWidget("ServiceProgressPercent"));
		m_ServiceProgressRemaining = TextWidget.Cast(m_ServiceProgressModal.FindAnyWidget("ServiceProgressRemaining"));
		m_ServiceProgressBar = ProgressBarWidget.Cast(m_ServiceProgressModal.FindAnyWidget("ServiceProgressBar"));
		m_ServiceProgressIconHost = m_ServiceProgressModal.FindAnyWidget("ServiceProgressIconHost");
		m_ServiceProgressIcon = ImageWidget.Cast(m_ServiceProgressModal.FindAnyWidget("ServiceProgressIcon"));
		Color accent = GRSA_Theme.Accent();
		if (m_ServiceProgressBorder)
			m_ServiceProgressBorder.SetColor(accent);
		if (m_ServiceProgressPercent)
			m_ServiceProgressPercent.SetColor(accent);
		if (m_ServiceProgressBar)
		{
			m_ServiceProgressBar.SetMin(0);
			m_ServiceProgressBar.SetMax(100);
			m_ServiceProgressBar.SetCurrent(0);
			m_ServiceProgressBar.SetDrawBackground(false);
			m_ServiceProgressBar.SetColor(accent);
		}
		if (m_ServiceProgressIcon)
			m_ServiceProgressIcon.SetColor(accent);
		m_ServiceProgressModal.SetVisible(false);
	}

	protected void BindStatsIcon(string widgetName, ResourceName texture)
	{
		if (!m_GRSAScreen)
			return;
		ImageWidget icon = ImageWidget.Cast(m_GRSAScreen.FindAnyWidget(widgetName));
		if (icon)
			icon.LoadImageTexture(0, texture);
	}

	protected Widget CreateGRSAChip(Widget parent, string label, int action)
	{
		if (!parent)
			return null;
		Widget root = GetGame().GetWorkspace().CreateWidgets(GRS_CHIP, parent);
		if (!root)
			return null;
		SCR_ButtonTextComponent chip = SCR_ButtonTextComponent.Cast(root.FindHandler(SCR_ButtonTextComponent));
		if (chip)
			chip.SetText(label);
		DCO_VehicleServiceButtonHandler handler = new DCO_VehicleServiceButtonHandler(this, action);
		handler.Attach(root);
		m_Handlers.Insert(handler);
		return root;
	}

	protected void CreateCargoBrowseControls()
	{
		ClearChildren(m_GRSACandidateClasses);
		if (m_GRSACandidateClasses)
			m_GRSACandidateClasses.SetVisible(false);
	}

	protected void CreateCargoBrowser()
	{
		if (!m_GRSAScreen)
			return;

		m_CargoBrowserRoot = GetGame().GetWorkspace().CreateWidgets(GRS_ITEM_LIST_PANEL, m_GRSAScreen);
		if (!m_CargoBrowserRoot)
			return;

		m_CargoBrowser = new GRSA_ItemListPanel(m_CargoBrowserRoot, "ItemListPanel", "ItemListTitle",
			"ItemList", "ItemScroll", "ItemSearchBox", "ItemListBackControls", "ItemListFilters");
		m_CargoBrowser.m_OnItemClicked.Insert(OnCargoBrowserClicked);
		m_CargoBrowser.m_OnQtyDelta.Insert(OnCargoBrowserQuantity);
		m_CargoBrowser.m_OnDone.Insert(OnCargoBrowserDone);
	}

	protected void CreateModeChip(string label, int action)
	{
		if (!m_GRSATabs)
			return;
		Widget root = GetGame().GetWorkspace().CreateWidgets(GRS_TAB, m_GRSATabs);
		if (!root)
			return;
		SCR_ButtonTextComponent chip = SCR_ButtonTextComponent.Cast(root.FindHandler(SCR_ButtonTextComponent));
		if (chip)
		{
			chip.SetText(label);
			m_GRSAModeChips.Insert(chip);
		}
		DCO_VehicleServiceButtonHandler handler = new DCO_VehicleServiceButtonHandler(this, action);
		handler.Attach(root);
		m_Handlers.Insert(handler);
	}

	protected void CreateFooterChip(string label, int action)
	{
		if (!m_GRSAFooter)
			return;
		Widget root = GetGame().GetWorkspace().CreateWidgets(GRS_CHIP, m_GRSAFooter);
		if (!root)
			return;
		SCR_ButtonTextComponent chip = SCR_ButtonTextComponent.Cast(root.FindHandler(SCR_ButtonTextComponent));
		if (chip)
			chip.SetText(label);
		DCO_VehicleServiceButtonHandler handler = new DCO_VehicleServiceButtonHandler(this, action);
		handler.Attach(root);
		m_FooterHandlers.Insert(handler);
	}

	protected void RebuildServiceFooter()
	{
		foreach (DCO_VehicleServiceButtonHandler handler : m_FooterHandlers)
		{
			if (handler)
				handler.Destroy();
		}
		m_FooterHandlers.Clear();
		ClearChildren(m_GRSAFooter);
		if (m_iServiceCapabilities & DCO_VehicleServiceServer.CAPABILITY_REPAIR)
			CreateFooterChip("REPAIR", ACTION_REPAIR);
		if (m_iServiceCapabilities & DCO_VehicleServiceServer.CAPABILITY_REFUEL)
			CreateFooterChip("REFUEL", ACTION_REFUEL);
		if (m_iServiceCapabilities & DCO_VehicleServiceServer.CAPABILITY_REARM)
			CreateFooterChip("REARM", ACTION_REARM);
		if (m_iServiceCapabilities != 0)
			CreateFooterChip("FULL SERVICE", ACTION_FULL);
		CreateFooterChip("RESET VIEW", ACTION_PREVIEW_RESET);
	}

	protected void OnGRSADataClicked(GRSA_ItemRowComponent row)
	{
		int index = m_GRSADataRows.Find(row);
		if (index < 0 || index >= m_GRSADataActions.Count())
			return;
		int dataIndex = m_GRSADataActions[index];
		if (m_iMode == MODE_SERVICE)
		{
			m_iDamagePage = dataIndex / DATA_ROWS;
			SelectDamage(dataIndex % DATA_ROWS);
		}
		else if (m_iMode == MODE_CARGO)
		{
			m_iCargoPage = dataIndex / CARGO_ROWS;
			RemoveCargo(dataIndex % CARGO_ROWS);
		}
	}

	protected void OnGRSACandidateClicked(GRSA_ItemRowComponent row)
	{
		int index = m_GRSACandidateRows.Find(row);
		if (index < 0 || index >= m_GRSACandidateActions.Count())
			return;
		int catalogIndex = m_GRSACandidateActions[index];
		if (catalogIndex >= 0 && catalogIndex < m_Catalog.Count())
			AddCargoEntry(m_Catalog[catalogIndex]);
	}

	protected void OnCargoBrowserClicked(GRSA_ItemRowComponent row)
	{
		if (!row || !row.GetEntry())
			return;
		ResourceName prefab = row.GetEntry().m_Prefab;
		if (m_CargoContainerPrefab.IsEmpty() && OpenCargoContainer(prefab))
			return;
		ChangeCargo(row, 1);
	}

	protected void OnCargoBrowserQuantity(GRSA_ItemRowComponent row, int delta)
	{
		ChangeCargo(row, delta);
	}

	protected void OnCargoBrowserDone()
	{
		if (!m_CargoContainerPrefab.IsEmpty())
		{
			m_CargoContainerPrefab = ResourceName.Empty;
			RefreshCargoBrowser();
			return;
		}
		m_iMode = MODE_SERVICE;
		RefreshMode();
		m_sRenderSignature = BuildRenderSignature();
	}

	protected void ChangeCargo(GRSA_ItemRowComponent row, int delta)
	{
		if (!row || !row.GetEntry() || delta == 0)
			return;
		DCO_ArsenalEntry entry = DCO_ArsenalCatalog.Get().FindByPrefab(row.GetEntry().m_Prefab);
		if (!entry)
			return;
		if (delta > 0)
			AddCargoEntry(entry);
		else
			RemoveCargoPrefab(entry.m_Prefab);
	}

	protected bool OpenCargoContainer(ResourceName prefab)
	{
		IEntity container = FindLoadedCargoEntity(prefab);
		if (!container)
			return false;
		InventoryStorageManagerComponent storage = InventoryStorageManagerComponent.Cast(
			container.FindComponent(InventoryStorageManagerComponent));
		if (!storage)
			return false;
		m_CargoContainerPrefab = prefab;
		RefreshCargoBrowser();
		return true;
	}

	protected IEntity FindLoadedCargoEntity(ResourceName prefab)
	{
		InventoryStorageManagerComponent inventory = DCO_VehicleServiceServer.GetVehicleInventory(m_SelectedVehicle);
		if (!inventory)
			return null;
		array<IEntity> items = {};
		inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!DCO_VehicleServiceServer.IsCargoItem(item) || !item.GetPrefabData()
				|| item.GetPrefabData().GetPrefabName() != prefab)
				continue;
			if (item.FindComponent(InventoryStorageManagerComponent))
				return item;
		}
		return null;
	}

	protected InventoryStorageManagerComponent GetCargoTargetInventory()
	{
		if (m_CargoContainerPrefab.IsEmpty())
			return DCO_VehicleServiceServer.GetVehicleInventory(m_SelectedVehicle);
		IEntity container = FindLoadedCargoEntity(m_CargoContainerPrefab);
		if (!container)
			return null;
		return InventoryStorageManagerComponent.Cast(container.FindComponent(InventoryStorageManagerComponent));
	}

	protected string CargoPayload(ResourceName itemPrefab)
	{
		if (m_CargoContainerPrefab.IsEmpty())
			return itemPrefab;
		return m_CargoContainerPrefab + "^" + itemPrefab;
	}

	protected void ClearGRSADataRows()
	{
		foreach (GRSA_ItemRowComponent row : m_GRSADataRows)
		{
			if (row)
				row.m_OnEntryClicked.Remove(OnGRSADataClicked);
			if (row && row.GetRootWidget())
				row.GetRootWidget().RemoveFromHierarchy();
		}
		m_GRSADataRows.Clear();
		m_GRSADataActions.Clear();
		m_DamageRows.Clear();
	}

	protected void ClearGRSACandidateRows()
	{
		foreach (GRSA_ItemRowComponent row : m_GRSACandidateRows)
		{
			if (row)
				row.m_OnEntryClicked.Remove(OnGRSACandidateClicked);
			if (row && row.GetRootWidget())
				row.GetRootWidget().RemoveFromHierarchy();
		}
		m_GRSACandidateRows.Clear();
		m_GRSACandidateActions.Clear();
	}

	protected GRSA_ItemRowComponent CreateGRSADataRow(string label, string state, ResourceName thumbnail, int action)
	{
		if (!m_GRSADataList)
			return null;
		Widget root = GetGame().GetWorkspace().CreateWidgets(GRS_ROW, m_GRSADataList);
		if (!root)
			return null;
		GRSA_ItemRowComponent row = GRSA_ItemRowComponent.Cast(root.FindHandler(GRSA_ItemRowComponent));
		if (!row)
		{
			root.RemoveFromHierarchy();
			return null;
		}
		SizeLayoutWidget rowSize = SizeLayoutWidget.Cast(root.FindAnyWidget("SizeLayout"));
		if (rowSize)
			rowSize.SetHeightOverride(62);
		row.SetSlotDisplay(label, state, thumbnail);
		row.UseOpaqueBackground();
		row.m_OnEntryClicked.Insert(OnGRSADataClicked);
		m_GRSADataRows.Insert(row);
		m_GRSADataActions.Insert(action);
		return row;
	}

	protected GRSA_ItemRowComponent CreateGRSACandidate(DCO_ArsenalEntry source, string state, int action)
	{
		if (!m_GRSACandidateList || !source)
			return null;
		GRSA_ItemEntry entry = new GRSA_ItemEntry();
		entry.m_Prefab = source.m_Prefab;
		entry.m_sDisplayName = source.m_sName;
		entry.m_eType = source.m_eType;
		Widget root = GetGame().GetWorkspace().CreateWidgets(GRS_TILE, m_GRSACandidateList);
		if (!root)
			return null;
		GRSA_ItemRowComponent row = GRSA_ItemRowComponent.Cast(root.FindHandler(GRSA_ItemRowComponent));
		if (!row)
		{
			root.RemoveFromHierarchy();
			return null;
		}
		row.SetEntry(entry, false);
		row.SetStateText(state);
		row.UseOpaqueBackground();
		row.m_OnEntryClicked.Insert(OnGRSACandidateClicked);
		m_GRSACandidateRows.Insert(row);
		m_GRSACandidateActions.Insert(action);
		return row;
	}

	protected void Bind(string widgetName, int action)
	{
		ButtonWidget button = ButtonWidget.Cast(m_Root.FindAnyWidget(widgetName));
		if (!button)
			return;
		DCO_VehicleServiceButtonHandler handler = new DCO_VehicleServiceButtonHandler(this, action);
		handler.Attach(button);
		m_Handlers.Insert(handler);
	}

	protected void BindWidget(string widgetName, int action)
	{
		Widget widget = m_Root.FindAnyWidget(widgetName);
		if (!widget)
			return;
		DCO_VehicleServiceButtonHandler handler = new DCO_VehicleServiceButtonHandler(this, action);
		handler.Attach(widget);
		m_Handlers.Insert(handler);
	}

	bool OnAction(int action)
	{
		if (action == ACTION_CLOSE)
		{
			Close();
			return true;
		}
		if (m_iActiveVerb)
			return true;
		if (action == ACTION_PREVIEW_RESET)
		{
			if (m_PreviewStage)
				m_PreviewStage.GoHome();
			return true;
		}
		if (action == ACTION_TAB_SERVICE || action == ACTION_TAB_CARGO)
		{
			if (action == ACTION_TAB_SERVICE)
				m_iMode = MODE_SERVICE;
			else
				m_iMode = MODE_CARGO;
			if (m_iMode != MODE_CARGO)
				m_CargoContainerPrefab = ResourceName.Empty;
			if (m_PreviewStage)
				m_PreviewStage.GoHome();
			RefreshMode();
			m_sRenderSignature = BuildRenderSignature();
			RefreshDamageCallouts();
			return true;
		}
		if (action >= ACTION_VEHICLE_ROW && action < ACTION_VEHICLE_ROW + VEHICLE_ROWS)
		{
			SelectVehicle(action - ACTION_VEHICLE_ROW);
			return true;
		}
		if (action >= ACTION_CARGO_ROW && action < ACTION_CARGO_ROW + CARGO_ROWS)
		{
			RemoveCargo(action - ACTION_CARGO_ROW);
			return true;
		}
		if (action >= ACTION_CATALOG_ROW && action < ACTION_CATALOG_ROW + CATALOG_ROWS)
		{
			AddCargo(action - ACTION_CATALOG_ROW);
			return true;
		}
		if (action >= ACTION_DAMAGE_ROW && action < ACTION_DAMAGE_ROW + DATA_ROWS)
		{
			SelectDamage(action - ACTION_DAMAGE_ROW);
			return true;
		}
		if (action == ACTION_VEHICLE_PREV || action == ACTION_VEHICLE_NEXT)
		{
			Page(action == ACTION_VEHICLE_NEXT, m_iVehiclePage, m_Vehicles.Count(), VEHICLE_ROWS);
			RefreshVehicleRows();
			return true;
		}
		if (action == ACTION_DATA_PREV || action == ACTION_DATA_NEXT)
		{
			if (m_iMode == MODE_SERVICE)
				Page(action == ACTION_DATA_NEXT, m_iDamagePage, m_Damage.Count(), DATA_ROWS);
			if (m_iMode == MODE_SERVICE)
				SelectFirstDamageOnPage();
			RefreshDiagnosticRows();
			return true;
		}
		if (action == ACTION_CARGO_PREV || action == ACTION_CARGO_NEXT)
		{
			Page(action == ACTION_CARGO_NEXT, m_iCargoPage, m_Cargo.Count(), CARGO_ROWS);
			RefreshCargoRows();
			return true;
		}
		if (action == ACTION_CATALOG_PREV || action == ACTION_CATALOG_NEXT)
		{
			Page(action == ACTION_CATALOG_NEXT, m_iCatalogPage, m_Catalog.Count(), CATALOG_ROWS);
			RefreshCatalogRows();
			return true;
		}
		if (action == ACTION_REPAIR)
			BeginServiceOperation(DCO_VehicleServiceServer.VERB_REPAIR);
		else if (action == ACTION_REFUEL)
			BeginServiceOperation(DCO_VehicleServiceServer.VERB_REFUEL);
		else if (action == ACTION_REARM)
			BeginServiceOperation(DCO_VehicleServiceServer.VERB_REARM);
		else if (action == ACTION_FULL)
			BeginServiceOperation(DCO_VehicleServiceServer.VERB_FULL_SERVICE);
		else
			return false;
		return true;
	}

	protected void BeginServiceOperation(int verb, string payload = "")
	{
		if (m_iActiveVerb || m_iPendingRequestId)
		{
			SetStatus(false, "Finish the current vehicle service request before starting another.");
			return;
		}
		if (!m_ServiceProgressModal || !m_ServiceProgressBar || !m_ServiceProgressOperation
			|| !m_ServiceProgressPercent || !m_ServiceProgressRemaining)
		{
			SetStatus(false, "The service progress interface is unavailable. Close and reopen Vehicle Service.");
			return;
		}
		string reason;
		if (!CanContinueService(reason))
		{
			SetStatus(false, reason);
			return;
		}
		if (!DCO_VehicleServiceServer.SupportsVerb(m_iServiceCapabilities, verb))
		{
			SetStatus(false, "The selected vehicle does not expose that service capability.");
			return;
		}

		RplId zoneId = m_Zone.GetReplicationId();
		RplComponent vehicleReplication = RplComponent.Cast(m_SelectedVehicle.FindComponent(RplComponent));
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!zoneId.IsValid() || !vehicleReplication || !vehicleReplication.Id().IsValid() || !controller)
		{
			SetStatus(false, "The service zone or vehicle is not replicated to this client.");
			return;
		}

		m_PendingServiceVehicle = m_SelectedVehicle;
		m_fPendingResponseElapsed = 0;
		m_iPendingRequestId = s_iNextRequestId++;
		m_iPendingVerb = verb;
		if (s_iNextRequestId <= 0)
			s_iNextRequestId = 1;
		SetStatus(true, "Requesting " + VerbName(verb) + " authorization from server...");
		controller.DCO_SendVehicleService(m_iPendingRequestId, verb, zoneId, vehicleReplication.Id(), payload);
	}

	protected void UpdateServiceOperation(float timeSlice)
	{
		if (!m_iActiveVerb)
			return;

		string reason;
		if (!CanContinueService(reason) || m_SelectedVehicle != m_ServiceVehicle)
		{
			CancelServiceOperation(reason);
			return;
		}

		if (m_bServiceTimerComplete)
		{
			m_fAuthorityWaitElapsed += timeSlice;
			UpdateProgressText();
			if (m_fAuthorityWaitElapsed < AUTHORITY_TIMEOUT_SECONDS)
				return;
			CancelServiceOperation("Server confirmation timed out. Verify the vehicle before retrying.");
			return;
		}

		m_fServiceElapsed = Math.Min(m_fServiceDuration, m_fServiceElapsed + timeSlice);
		float progress = 1;
		if (m_fServiceDuration > 0)
			progress = m_fServiceElapsed / m_fServiceDuration;
		UpdateProgressText();
		UpdateServiceAudio(false);
		if (m_fServiceElapsed < m_fServiceDuration)
			return;

		m_bServiceTimerComplete = true;
		m_fAuthorityWaitElapsed = 0;
		StopLoopSound();
		SetStatus(true, "Service cycle complete. Waiting for server confirmation...");
	}

	protected bool CanContinueService(out string reason)
	{
		if (!m_Zone || !m_Zone.GetOwner() || !m_User || m_User.IsDeleted()
			|| !m_SelectedVehicle || m_SelectedVehicle.IsDeleted())
		{
			reason = "Select a vehicle inside the service circle first.";
			return false;
		}
		DCO_VehicleServiceAccessComponent access = m_Zone.GetAccess();
		if (!access || !DCO_VehicleServiceAccessComponent.IsUsableBy(access.GetOwner(), m_User))
		{
			reason = "Exit the vehicle and remain beside the service access point.";
			return false;
		}
		if (!m_Zone.ContainsVehicle(m_SelectedVehicle))
		{
			reason = "Keep the selected vehicle inside the service circle.";
			return false;
		}
		if (!m_Zone.IsVehicleStationary(m_SelectedVehicle))
		{
			reason = "Stop the selected vehicle before servicing it.";
			return false;
		}
		return true;
	}

	protected void UpdatePendingRequestTimeout(float timeSlice)
	{
		if (!m_iPendingRequestId)
			return;
		m_fPendingResponseElapsed += timeSlice;
		if (m_fPendingResponseElapsed < AUTHORITY_TIMEOUT_SECONDS)
			return;
		if (DCO_VehicleServiceServer.IsTimedServiceVerb(m_iPendingVerb))
		{
			SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (controller)
				controller.DCO_CancelVehicleService(m_iPendingRequestId);
		}
		m_iPendingRequestId = 0;
		m_iPendingVerb = 0;
		m_PendingServiceVehicle = null;
		m_fPendingResponseElapsed = 0;
		SetStatus(false, "The server did not confirm the request. The interface was unlocked; verify state before retrying.");
	}

	protected void UpdateProgressText()
	{
		if (!m_iActiveVerb)
			return;
		float overallProgress = Math.Clamp(m_fServiceElapsed / Math.Max(0.01, m_fServiceDuration), 0, 1);
		int percent = Math.Round(overallProgress * 100);
		int phase = GetActiveServicePhase();
		string operation = VerbName(m_iActiveVerb);
		if (m_iActiveVerb == DCO_VehicleServiceServer.VERB_FULL_SERVICE)
			operation = "FULL SERVICE - " + VerbName(phase);
		if (m_ServiceProgressOperation)
			m_ServiceProgressOperation.SetText(operation);
		if (m_ServiceProgressPercent)
			m_ServiceProgressPercent.SetText(percent.ToString() + "%");
		if (m_ServiceProgressRemaining)
		{
			if (m_bServiceTimerComplete)
				m_ServiceProgressRemaining.SetText("VERIFYING");
			else
			{
				int remaining = Math.Ceil(Math.Max(0, m_fServiceDuration - m_fServiceElapsed));
				m_ServiceProgressRemaining.SetText(remaining.ToString() + " SEC");
			}
		}
		if (m_ServiceProgressBar)
			m_ServiceProgressBar.SetCurrent(percent);
		UpdateServiceIconPosition(overallProgress);
		UpdateServiceIcon(phase);
	}

	protected void UpdateServiceIconPosition(float progress)
	{
		if (!m_ServiceProgressIconHost)
			return;
		float anchorX = 0.045 + Math.Clamp(progress, 0, 1) * 0.91;
		FrameSlot.SetAnchorMin(m_ServiceProgressIconHost, anchorX, 0.39);
		FrameSlot.SetAnchorMax(m_ServiceProgressIconHost, anchorX, 0.39);
		FrameSlot.SetOffsets(m_ServiceProgressIconHost, -12, 0, 12, 24);
	}

	protected int GetActiveServicePhase()
	{
		return DCO_VehicleServiceServer.GetServicePhase(m_iActiveVerb,
			Math.Round(m_fServiceElapsed * 1000), m_iActiveServiceCapabilities);
	}

	protected void UpdateServiceIcon(int phase)
	{
		if (phase == m_iServiceIconPhase)
			return;
		ResourceName icon = ICON_REPAIR;
		if (phase == DCO_VehicleServiceServer.VERB_REFUEL)
			icon = ICON_REFUEL;
		else if (phase == DCO_VehicleServiceServer.VERB_REARM)
			icon = ICON_REARM;
		if (m_ServiceProgressIcon)
			m_ServiceProgressIcon.LoadImageTexture(0, icon);
		m_iServiceIconPhase = phase;
	}

	protected void FinishServiceProgress()
	{
		StopLoopSound();
		m_iActiveVerb = 0;
		m_iActiveServiceCapabilities = 0;
		m_iServiceAudioPhase = 0;
		m_iServiceIconPhase = 0;
		m_ServiceVehicle = null;
		m_fServiceElapsed = 0;
		m_fServiceDuration = 0;
		m_fAuthorityWaitElapsed = 0;
		m_fPendingResponseElapsed = 0;
		m_fNextServiceAudioAttempt = 0;
		m_bServiceTimerComplete = false;
		if (m_ServiceProgressModal)
			m_ServiceProgressModal.SetVisible(false);
		if (m_ServiceProgressBar)
			m_ServiceProgressBar.SetCurrent(0);
		UpdateServiceIconPosition(0);
		RefreshVehicleRows();
	}

	protected void CancelServiceOperation(string reason)
	{
		bool hasProgress = m_iActiveVerb != 0;
		bool hasTimedRequest = m_iPendingRequestId != 0
			&& DCO_VehicleServiceServer.IsTimedServiceVerb(m_iPendingVerb);
		if (!hasProgress && !hasTimedRequest)
		{
			StopLoopSound();
			return;
		}

		if (hasTimedRequest)
		{
			SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (controller)
				controller.DCO_CancelVehicleService(m_iPendingRequestId);
			m_iPendingRequestId = 0;
			m_iPendingVerb = 0;
			m_PendingServiceVehicle = null;
			m_fPendingResponseElapsed = 0;
		}
		if (hasProgress)
			FinishServiceProgress();
		else
			StopLoopSound();
		if (!reason.IsEmpty())
			SetStatus(false, "Service cancelled: " + reason);
	}

	protected string VerbName(int verb)
	{
		switch (verb)
		{
			case DCO_VehicleServiceServer.VERB_REPAIR: return "REPAIR";
			case DCO_VehicleServiceServer.VERB_REFUEL: return "REFUEL";
			case DCO_VehicleServiceServer.VERB_REARM: return "REARM";
			case DCO_VehicleServiceServer.VERB_FULL_SERVICE: return "FULL SERVICE";
		}
		return "SERVICE";
	}

	protected void UpdateServiceAudio(bool force)
	{
		if (!m_iActiveVerb || m_bServiceTimerComplete)
			return;
		int phase = GetActiveServicePhase();
		if (!force && phase == m_iServiceAudioPhase)
		{
			if (m_ServiceAudio != AudioHandle.Invalid && AudioSystem.IsSoundPlayed(m_ServiceAudio))
				return;
			if (m_fServiceElapsed < m_fNextServiceAudioAttempt)
				return;
		}
		PlayOperationSound(phase);
		m_fNextServiceAudioAttempt = m_fServiceElapsed + 0.25;
	}

	protected void PlayOperationSound(int phase)
	{
		StopLoopSound();
		string eventName = SOUND_REPAIR;
		if (phase == DCO_VehicleServiceServer.VERB_REFUEL)
			eventName = SOUND_REFUEL;
		else if (phase == DCO_VehicleServiceServer.VERB_REARM)
			eventName = SOUND_REARM;
		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		if (m_Zone)
			transform[3] = m_Zone.GetAccessPoint();
		m_ServiceAudio = AudioSystem.PlayEvent(SUPPORT_ACP, eventName, transform);
		m_iServiceAudioPhase = phase;
	}

	protected void StopLoopSound()
	{
		if (m_ServiceAudio == AudioHandle.Invalid)
			return;
		AudioSystem.TerminateSoundFadeOut(m_ServiceAudio, true, 0.2);
		m_ServiceAudio = AudioHandle.Invalid;
	}

	protected void PlayCompletionSound(int verb)
	{
		if (verb == DCO_VehicleServiceServer.VERB_REARM
			|| verb == DCO_VehicleServiceServer.VERB_ADD_CARGO
			|| verb == DCO_VehicleServiceServer.VERB_REMOVE_CARGO)
		{
			SCR_UISoundEntity.SoundEvent("SOUND_LOADSUPPLIES");
			return;
		}

		string eventName = SOUND_REPAIR_DONE;
		if (verb == DCO_VehicleServiceServer.VERB_REFUEL || verb == DCO_VehicleServiceServer.VERB_FULL_SERVICE)
			eventName = SOUND_REFUEL_DONE;
		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		if (m_Zone)
			transform[3] = m_Zone.GetAccessPoint();
		AudioSystem.PlayEvent(SUPPORT_ACP, eventName, transform);
	}

	protected void Page(bool forward, inout int page, int count, int pageSize)
	{
		int pages = Math.Max(1, (count + pageSize - 1) / pageSize);
		if (forward)
			page = Math.Min(pages - 1, page + 1);
		else
			page = Math.Max(0, page - 1);
	}

	protected void SelectVehicle(int visibleRow)
	{
		int index = m_iVehiclePage * VEHICLE_ROWS + visibleRow;
		if (index < 0 || index >= m_Vehicles.Count())
			return;
		if (m_SelectedVehicle != m_Vehicles[index])
			CancelServiceOperation("a different vehicle was selected.");
		m_SelectedVehicle = m_Vehicles[index];
		m_iSelectedDamageHitZone = -1;
		m_iDamagePage = 0;
		m_iCargoPage = 0;
		m_CargoContainerPrefab = ResourceName.Empty;
		RefreshVehicleData();
		RefreshVehicleRows();
		SetStatus(true, "Selected " + EntityName(m_SelectedVehicle) + ".");
	}

	protected void SelectDamage(int visibleRow)
	{
		int index = m_iDamagePage * DATA_ROWS + visibleRow;
		if (index < 0 || index >= m_Damage.Count())
			return;
		m_iSelectedDamageHitZone = m_Damage[index].m_iHitZoneIndex;
		if (m_PreviewStage)
			m_PreviewStage.FocusDamagePoint(m_iSelectedDamageHitZone);
		RefreshDiagnosticRows();
		SetStatus(true, m_Damage[index].m_sName + "  |  " + m_Damage[index].m_sDetail);
	}

	protected void SelectFirstDamageOnPage()
	{
		int index = m_iDamagePage * DATA_ROWS;
		if (index >= 0 && index < m_Damage.Count())
			m_iSelectedDamageHitZone = m_Damage[index].m_iHitZoneIndex;
		else
			m_iSelectedDamageHitZone = -1;
	}

	protected void AddCargo(int visibleRow)
	{
		int index = m_iCatalogPage * CATALOG_ROWS + visibleRow;
		if (index < 0 || index >= m_Catalog.Count())
			return;
		DCO_ArsenalEntry entry = m_Catalog[index];
		AddCargoEntry(entry);
	}

	protected void AddCargoEntry(DCO_ArsenalEntry entry)
	{
		if (!entry)
			return;
		InventoryStorageManagerComponent inventory = GetCargoTargetInventory();
		if (!inventory || !inventory.CanInsertResource(entry.m_Prefab, EStoragePurpose.PURPOSE_DEPOSIT))
		{
			SetStatus(false, "The selected item does not fit in this vehicle's cargo storage.");
			return;
		}
		Send(DCO_VehicleServiceServer.VERB_ADD_CARGO, CargoPayload(entry.m_Prefab));
	}

	protected void RemoveCargo(int visibleRow)
	{
		int index = m_iCargoPage * CARGO_ROWS + visibleRow;
		if (index < 0 || index >= m_Cargo.Count())
			return;
		DCO_VehicleCargoEntry entry = m_Cargo[index];
		if (entry)
			RemoveCargoPrefab(entry.m_Prefab);
	}

	protected void RemoveCargoPrefab(ResourceName prefab)
	{
		if (CargoTargetCount(prefab) <= 0)
		{
			SetStatus(false, "That item is not currently loaded in the vehicle.");
			return;
		}
		Send(DCO_VehicleServiceServer.VERB_REMOVE_CARGO, CargoPayload(prefab));
	}

	protected int CargoTargetCount(ResourceName prefab)
	{
		InventoryStorageManagerComponent inventory = GetCargoTargetInventory();
		if (!inventory)
			return 0;
		int count;
		array<IEntity> items = {};
		inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (DCO_VehicleServiceServer.IsCargoItem(item) && item.GetPrefabData()
				&& item.GetPrefabData().GetPrefabName() == prefab)
				count++;
		}
		return count;
	}

	protected void Send(int verb, string payload)
	{
		if (m_iPendingRequestId)
		{
			SetStatus(false, "Wait for the current server response before sending another request.");
			return;
		}
		if (!m_Zone || !m_SelectedVehicle)
		{
			SetStatus(false, "Select a vehicle inside the service circle first.");
			return;
		}
		RplId zoneId = m_Zone.GetReplicationId();
		RplComponent vehicleReplication = RplComponent.Cast(m_SelectedVehicle.FindComponent(RplComponent));
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!zoneId.IsValid() || !vehicleReplication || !vehicleReplication.Id().IsValid() || !controller)
		{
			SetStatus(false, "The service zone or vehicle is not replicated to this client.");
			return;
		}
		if (verb == DCO_VehicleServiceServer.VERB_ADD_CARGO || verb == DCO_VehicleServiceServer.VERB_REMOVE_CARGO)
			SetStatus(true, "Applying the cargo change on the server...");
		else
			SetStatus(true, "Applying the completed service on the server...");
		m_iPendingRequestId = s_iNextRequestId++;
		m_iPendingVerb = verb;
		m_fPendingResponseElapsed = 0;
		if (s_iNextRequestId <= 0)
			s_iNextRequestId = 1;
		controller.DCO_SendVehicleService(m_iPendingRequestId, verb, zoneId, vehicleReplication.Id(), payload);
	}

	protected void RefreshAfterAuthority()
	{
		RefreshAll();
	}

	protected void RefreshAll()
	{
		RefreshVehicles();
		RefreshVehicleData();
		string signature = BuildRenderSignature();
		if (signature != m_sRenderSignature)
		{
			m_sRenderSignature = signature;
			RefreshDiagnosticRows();
			RefreshCandidates();
		}
		RefreshVehicleRows();
	}

	protected void RefreshVehicleData()
	{
		RefreshPreview();
		RefreshDamage();
		RefreshAmmo();
		RefreshCargo();
		RefreshServiceCapabilities();
	}

	protected void RefreshServiceCapabilities()
	{
		int capabilities = DCO_VehicleServiceServer.GetServiceCapabilities(m_SelectedVehicle);
		if (capabilities == m_iServiceCapabilities)
			return;
		m_iServiceCapabilities = capabilities;
		RebuildServiceFooter();
	}

	protected string BuildRenderSignature()
	{
		string signature = m_iMode.ToString() + ":" + m_Vehicles.Find(m_SelectedVehicle).ToString();
		foreach (DCO_VehicleDamageEntry damage : m_Damage)
		{
			if (damage)
				signature += "|D:" + damage.m_sName + ":" + damage.m_sDetail + ":" + damage.m_iHealthPercent.ToString();
		}
		foreach (DCO_VehicleAmmoEntry ammo : m_Ammo)
		{
			if (ammo)
				signature += "|A:" + ammo.m_iCurrent.ToString() + ":" + ammo.m_iMaximum.ToString();
		}
		foreach (DCO_VehicleCargoEntry cargo : m_Cargo)
		{
			if (cargo)
				signature += "|C:" + cargo.m_Prefab + ":" + cargo.m_iCount.ToString();
		}
		return signature;
	}

	protected void RefreshVehicles()
	{
		IEntity prior = m_SelectedVehicle;
		m_Vehicles.Clear();
		if (m_Zone)
			m_Zone.GetVehicles(m_Vehicles);
		if (prior && m_Vehicles.Find(prior) >= 0)
			m_SelectedVehicle = prior;
		else if (!m_Vehicles.IsEmpty())
			m_SelectedVehicle = m_Vehicles[0];
		else
			m_SelectedVehicle = null;
		if (prior && prior != m_SelectedVehicle)
			CancelServiceOperation("the selected vehicle left the service circle.");
		int pages = Math.Max(1, (m_Vehicles.Count() + VEHICLE_ROWS - 1) / VEHICLE_ROWS);
		m_iVehiclePage = Math.ClampInt(m_iVehiclePage, 0, pages - 1);
	}

	protected void RefreshPreview()
	{
		if (!m_PreviewStage || !m_PreviewRender)
			return;

		ResourceName prefab;
		if (m_SelectedVehicle && !m_SelectedVehicle.IsDeleted() && m_SelectedVehicle.GetPrefabData())
			prefab = m_SelectedVehicle.GetPrefabData().GetPrefabName();
		if (prefab == m_PreviewVehiclePrefab)
			return;
		m_PreviewVehiclePrefab = prefab;
		if (prefab.IsEmpty())
		{
			m_PreviewStage.Clear();
			return;
		}
		if (!m_PreviewStage.ShowVehicle(prefab, m_PreviewRender))
			SetStatus(false, "This vehicle cannot be rendered in the private workshop preview.");
	}

	protected void RefreshDamage()
	{
		m_Damage.Clear();
		SCR_DamageManagerComponent damage;
		if (m_SelectedVehicle)
			damage = SCR_DamageManagerComponent.Cast(
				m_SelectedVehicle.FindComponent(SCR_DamageManagerComponent));
		if (!damage)
			return;

		array<HitZone> hitZones = {};
		map<string, ref DCO_VehicleDamageEntry> systems = new map<string, ref DCO_VehicleDamageEntry>();
		damage.GetAllHitZonesInHierarchy(hitZones);
		foreach (int index, HitZone hitZone : hitZones)
		{
			if (!hitZone || hitZone.GetMaxHealth() <= 0)
				continue;
			string systemName = DamageSystemName(hitZone, index);
			DCO_VehicleDamageEntry entry;
			if (!systems.Find(systemName, entry))
			{
				entry = new DCO_VehicleDamageEntry();
				entry.m_sName = systemName;
				entry.m_IconTexture = DamageSystemIcon(systemName);
				entry.m_iHealthPercent = 101;
				systems.Insert(systemName, entry);
			}
			entry.m_iPartCount++;
			int healthPercent = Math.Round(Math.Clamp(hitZone.GetHealthScaled(), 0, 1) * 100);
			if (healthPercent < entry.m_iHealthPercent)
			{
				entry.m_iHealthPercent = healthPercent;
				entry.m_iHitZoneIndex = index;
			}
			if (hitZone.GetDamageOverTime(EDamageType.FIRE) > 0)
				entry.m_bOnFire = true;
		}

		foreach (string systemName, DCO_VehicleDamageEntry entry : systems)
		{
			entry.m_sDetail = AggregatedDamageDetail(entry);
			int insertAt;
			while (insertAt < m_Damage.Count() && m_Damage[insertAt].m_iHealthPercent <= entry.m_iHealthPercent)
				insertAt++;
			m_Damage.InsertAt(entry, insertAt);
		}
		int pages = Math.Max(1, (m_Damage.Count() + DATA_ROWS - 1) / DATA_ROWS);
		m_iDamagePage = Math.ClampInt(m_iDamagePage, 0, pages - 1);
		bool selectionFound;
		foreach (DCO_VehicleDamageEntry damageEntry : m_Damage)
		{
			if (damageEntry.m_iHitZoneIndex == m_iSelectedDamageHitZone)
			{
				selectionFound = true;
				break;
			}
		}
		if (!selectionFound)
			SelectFirstDamageOnPage();
	}

	protected string DamageSystemName(HitZone hitZone, int index)
	{
		if (SCR_WheelHitZone.Cast(hitZone))
			return "WHEELS";
		if (SCR_EngineHitZone.Cast(hitZone))
			return "ENGINE";
		if (SCR_GearboxHitZone.Cast(hitZone))
			return "TRANSMISSION";
		if (SCR_FuelHitZone.Cast(hitZone))
			return "FUEL SYSTEM";
		if (SCR_BatteryHitZone.Cast(hitZone))
			return "ELECTRICAL";
		if (SCR_LightHitZone.Cast(hitZone))
			return "LIGHTING";

		string name = HitZoneName(hitZone, index);
		if (name.Contains("GEAR") || name.Contains("DIFFERENTIAL") || name.Contains("DRIVE") || name.Contains("TRANSMISSION"))
			return "TRANSMISSION";
		if (name.Contains("WHEEL") || name.Contains("TIRE") || name.Contains("TRACK"))
			return "WHEELS";
		if (name.Contains("TURRET") || name.Contains("WEAPON") || name.Contains("AMMO") || name.Contains("GUN"))
			return "ARMAMENT";
		if (name.Contains("OPTIC") || name.Contains("SIGHT") || name.Contains("VIEWPORT"))
			return "OPTICS";
		if (name.Contains("ROTOR") || name.Contains("PROPELLER"))
			return "ROTOR SYSTEM";
		if (name.Contains("CONTROL") || name.Contains("INSTRUMENT"))
			return "CONTROLS";
		if (name.Contains("HULL") || name.Contains("BODY") || name.Contains("FRAME") || name.Contains("STRUCTURE"))
			return "HULL";
		return "OTHER SYSTEMS";
	}

	protected ResourceName DamageSystemIcon(string systemName)
	{
		if (systemName == "WHEELS")
			return ICON_SYSTEM_WHEELS;
		if (systemName == "ENGINE")
			return ICON_SYSTEM_ENGINE;
		if (systemName == "TRANSMISSION")
			return ICON_SYSTEM_TRANSMISSION;
		if (systemName == "FUEL SYSTEM")
			return ICON_SYSTEM_FUEL;
		if (systemName == "LIGHTING" || systemName == "OPTICS")
			return ICON_SYSTEM_LIGHTING;
		if (systemName == "ELECTRICAL" || systemName == "CONTROLS")
			return ICON_SYSTEM_ELECTRICAL;
		if (systemName == "ARMAMENT")
			return ICON_SYSTEM_ARMAMENT;
		if (systemName == "HULL")
			return ICON_SYSTEM_HULL;
		return ICON_SYSTEM_OTHER;
	}

	protected string AggregatedDamageDetail(DCO_VehicleDamageEntry entry)
	{
		string detail = entry.m_iPartCount.ToString() + " PART";
		if (entry.m_iPartCount != 1)
			detail += "S";
		if (entry.m_iHealthPercent >= 100)
			detail += "  |  UNDAMAGED";
		else
			detail += "  |  " + (100 - entry.m_iHealthPercent).ToString() + "% DAMAGED";
		if (entry.m_bOnFire)
			detail += "  |  ON FIRE";
		return detail;
	}

	protected string HitZoneName(HitZone hitZone, int index)
	{
		SCR_WheelHitZone wheel = SCR_WheelHitZone.Cast(hitZone);
		if (wheel && wheel.GetWheelIndex() >= 0)
			return "WHEEL " + (wheel.GetWheelIndex() + 1).ToString();
		if (SCR_EngineHitZone.Cast(hitZone))
			return "ENGINE";
		if (SCR_BatteryHitZone.Cast(hitZone))
			return "BATTERY";
		if (SCR_GearboxHitZone.Cast(hitZone))
			return "GEARBOX";
		if (SCR_FuelHitZone.Cast(hitZone))
			return "FUEL SYSTEM";
		if (SCR_LightHitZone.Cast(hitZone))
			return "LIGHT";

		string nativeName = CleanHitZoneName(hitZone.GetName());
		if (!nativeName.IsEmpty())
			return nativeName;

		return "VEHICLE STRUCTURE " + (index + 1).ToString();
	}

	protected string CleanHitZoneName(string name)
	{
		name.Replace("EHitZoneGroup.", string.Empty);
		name.Replace("HITZONEGROUP_", string.Empty);
		name.Replace("HitZone", string.Empty);
		name.Replace("HITZONE", string.Empty);
		name.Replace("_", " ");
		name.Replace("-", " ");
		name.ToUpper();
		name.Replace("UNKNOWN", string.Empty);
		name.Replace("VIRTUAL", string.Empty);
		name.Replace("UBX", string.Empty);
		name.Replace("UCX", string.Empty);
		name.Replace(" FG ", " ");
		if (name.StartsWith("FG "))
			name = name.Substring(3, name.Length() - 3);
		while (name.Contains("  "))
			name.Replace("  ", " ");
		while (name.Length() > 0 && name.StartsWith(" "))
			name = name.Substring(1, name.Length() - 1);
		while (name.Length() > 0 && name.EndsWith(" "))
			name = name.Substring(0, name.Length() - 1);
		if (name == "NONE" || name == "DEFAULT")
			return string.Empty;
		if (name.Length() > 42)
			name = name.Substring(0, 42);
		return name;
	}

	protected string DamageDetail(HitZone hitZone, int healthPercent)
	{
		string state = SCR_Enum.GetEnumName(EDamageState, hitZone.GetDamageState());
		state.Replace("EDamageState.", string.Empty);
		state.ToUpper();
		if (state.IsEmpty())
			state = "CURRENT CONDITION";
		string detail = state;
		if (healthPercent < 100)
			detail = (100 - healthPercent).ToString() + "% DAMAGED  |  " + state;
		if (hitZone.GetDamageOverTime(EDamageType.FIRE) > 0)
			detail += "  |  ON FIRE";
		return detail;
	}

	protected void RefreshAmmo()
	{
		m_Ammo.Clear();
		array<WeaponSlotComponent> weaponSlots = {};
		DCO_VehicleServiceServer.GetMountedWeaponSlots(m_SelectedVehicle, weaponSlots);
		array<IEntity> visitedWeapons = {};
		foreach (WeaponSlotComponent weaponSlot : weaponSlots)
		{
			IEntity weapon;
			if (weaponSlot)
				weapon = weaponSlot.GetWeaponEntity();
			if (!weapon || visitedWeapons.Find(weapon) >= 0)
				continue;
			visitedWeapons.Insert(weapon);
			CollectWeaponAmmo(weaponSlot, weapon);
		}
	}

	protected void CollectWeaponAmmo(WeaponSlotComponent weaponSlot, IEntity weapon)
	{
		array<BaseMuzzleComponent> muzzles = {};
		weaponSlot.GetMuzzlesList(muzzles);
		foreach (BaseMuzzleComponent muzzle : muzzles)
		{
			BaseMagazineComponent magazine = muzzle.GetMagazine();
			if (!magazine || magazine.GetMaxAmmoCount() <= 0)
				continue;
			DCO_VehicleAmmoEntry entry = new DCO_VehicleAmmoEntry();
			entry.m_iCurrent = magazine.GetAmmoCount();
			entry.m_iMaximum = magazine.GetMaxAmmoCount();
			m_Ammo.Insert(entry);
		}

		array<Managed> rocketComponents = {};
		weapon.FindComponents(SCR_RocketEjectorMuzzleComponent, rocketComponents);
		foreach (Managed managedRocket : rocketComponents)
		{
			SCR_RocketEjectorMuzzleComponent rocketMuzzle = SCR_RocketEjectorMuzzleComponent.Cast(managedRocket);
			if (!rocketMuzzle || rocketMuzzle.GetBarrelsCount() <= 0)
				continue;
			array<IEntity> loadedRockets = {};
			rocketMuzzle.GetLoadedEntities(loadedRockets);
			DCO_VehicleAmmoEntry entry = new DCO_VehicleAmmoEntry();
			entry.m_iCurrent = loadedRockets.Count();
			entry.m_iMaximum = rocketMuzzle.GetBarrelsCount();
			m_Ammo.Insert(entry);
		}
	}

	protected void RefreshCargo()
	{
		m_Cargo.Clear();
		InventoryStorageManagerComponent inventory = DCO_VehicleServiceServer.GetVehicleInventory(m_SelectedVehicle);
		if (!inventory)
			return;
		array<IEntity> items = {};
		inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!DCO_VehicleServiceServer.IsCargoItem(item) || !item.GetPrefabData())
				continue;
			ResourceName prefab = item.GetPrefabData().GetPrefabName();
			DCO_VehicleCargoEntry existing = FindCargo(prefab);
			if (existing)
			{
				existing.m_iCount++;
				continue;
			}
			DCO_VehicleCargoEntry entry = new DCO_VehicleCargoEntry();
			entry.m_Prefab = prefab;
			DCO_ArsenalEntry catalogEntry = DCO_ArsenalCatalog.Get().FindByPrefab(prefab);
			if (catalogEntry)
				entry.m_sName = catalogEntry.m_sName;
			else
				entry.m_sName = NameFromResource(prefab);
			entry.m_iCount = 1;
			m_Cargo.Insert(entry);
		}
		int pages = Math.Max(1, (m_Cargo.Count() + CARGO_ROWS - 1) / CARGO_ROWS);
		m_iCargoPage = Math.ClampInt(m_iCargoPage, 0, pages - 1);
	}

	protected DCO_VehicleCargoEntry FindCargo(ResourceName prefab)
	{
		foreach (DCO_VehicleCargoEntry entry : m_Cargo)
		{
			if (entry && entry.m_Prefab == prefab)
				return entry;
		}
		return null;
	}

	protected void BuildCatalog()
	{
		m_Catalog.Clear();
		m_CargoBrowseItems.Clear();
		array<string> seen = {};
		AppendCategory(EDCO_ArsenalCategory.PRIMARY, seen);
		AppendCategory(EDCO_ArsenalCategory.PISTOL, seen);
		AppendCategory(EDCO_ArsenalCategory.LAUNCHER, seen);
		AppendCategory(EDCO_ArsenalCategory.UNIFORM, seen);
		AppendCategory(EDCO_ArsenalCategory.VEST, seen);
		AppendCategory(EDCO_ArsenalCategory.BACKPACK, seen);
		AppendCategory(EDCO_ArsenalCategory.HEADGEAR, seen);
		AppendCategory(EDCO_ArsenalCategory.ITEMS, seen);
		AppendCategory(EDCO_ArsenalCategory.GRENADES, seen);
		AppendCategory(EDCO_ArsenalCategory.MAGAZINES, seen);
		AppendCategory(EDCO_ArsenalCategory.ATTACHMENTS, seen);
		foreach (DCO_ArsenalEntry source : m_Catalog)
		{
			if (!source)
				continue;
			GRSA_ItemEntry item = new GRSA_ItemEntry();
			item.m_Prefab = source.m_Prefab;
			item.m_sDisplayName = source.m_sName;
			item.m_eType = source.m_eType;
			if (source.m_eCategory == EDCO_ArsenalCategory.MAGAZINES)
				item.m_eMode = SCR_EArsenalItemMode.AMMUNITION;
			m_CargoBrowseItems.Insert(item);
		}
		int pages = Math.Max(1, (m_Catalog.Count() + CATALOG_ROWS - 1) / CATALOG_ROWS);
		m_iCatalogPage = Math.ClampInt(m_iCatalogPage, 0, pages - 1);
	}

	protected bool IsWeaponCategory(EDCO_ArsenalCategory category)
	{
		return category == EDCO_ArsenalCategory.PRIMARY
			|| category == EDCO_ArsenalCategory.PISTOL
			|| category == EDCO_ArsenalCategory.LAUNCHER;
	}

	protected void AppendCategory(EDCO_ArsenalCategory category, notnull array<string> seen)
	{
		array<DCO_ArsenalEntry> entries = {};
		DCO_ArsenalCatalog.Get().GetEntries(category, m_sLastSearch, string.Empty, entries);
		foreach (DCO_ArsenalEntry entry : entries)
		{
			if (!entry)
				continue;
			string key = entry.m_Prefab;
			if (seen.Find(key) >= 0)
				continue;
			seen.Insert(key);
			m_Catalog.Insert(entry);
		}
	}

	protected void RefreshMode()
	{
		int selected;
		if (m_iMode == MODE_CARGO)
			selected = 1;
		foreach (int index, SCR_ButtonTextComponent chip : m_GRSAModeChips)
		{
			if (chip)
				chip.SetToggled(index == selected, true, false);
		}
		RefreshDiagnosticRows();
		RefreshCandidates();
		RefreshVehicleRows();
	}

	protected void RefreshVehicleRows()
	{
		if (m_GRSAReceiverCard)
			m_GRSAReceiverCard.SetVisible(false);
		RefreshStats();
	}

	protected void RefreshDiagnosticRows()
	{
		ClearGRSADataRows();
		Widget rail = m_GRSAScreen.FindAnyWidget("HardpointRail");
		int start;
		int end;
		if (m_iMode == MODE_SERVICE)
		{
			if (m_GRSAHardpointCounter)
				m_GRSAHardpointCounter.SetText("REPAIR AREA");
			start = 0;
			end = m_Damage.Count();
			for (int i = start; i < end; i++)
			{
				DCO_VehicleDamageEntry entry = m_Damage[i];
				string condition = entry.m_iHealthPercent.ToString() + "% HEALTH";
				if (entry.m_iHealthPercent >= 100)
					condition += "\nUNDAMAGED";
				else
					condition += "\n" + (100 - entry.m_iHealthPercent).ToString() + "% DAMAGED";
				if (entry.m_bOnFire)
					condition += " / FIRE";
				GRSA_ItemRowComponent row = CreateGRSADataRow(entry.m_sName, condition,
					ResourceName.Empty, i);
				if (row)
				{
					row.SetIconTexture(entry.m_IconTexture);
					string parts = entry.m_iPartCount.ToString() + " PART";
					if (entry.m_iPartCount != 1)
						parts += "S";
					row.SetMetaText(parts);
				}
				if (row && row.GetRootWidget())
					m_DamageRows.Insert(row.GetRootWidget());
			}
		}
		else
		{
			if (m_GRSAHardpointCounter)
				m_GRSAHardpointCounter.SetText("LOADING AREA");
			start = 0;
			end = m_Cargo.Count();
			for (int i = start; i < end; i++)
			{
				DCO_VehicleCargoEntry entry = m_Cargo[i];
				GRSA_ItemRowComponent cargoRow = CreateGRSADataRow(entry.m_sName, "LOADED", entry.m_Prefab, i);
				if (cargoRow)
					cargoRow.SetMetaText("x" + entry.m_iCount.ToString());
			}
		}
		if (rail)
			rail.SetVisible(!m_GRSADataRows.IsEmpty());
		if (m_DamageCallouts)
			m_DamageCallouts.SetRows(m_DamageRows);
		RefreshDamageCallouts();
	}

	protected void RefreshDamageCallouts()
	{
		if (m_DamageCallouts)
			m_DamageCallouts.Refresh(m_Damage, 0, Math.Max(1, m_Damage.Count()),
				m_iSelectedDamageHitZone, m_iMode == MODE_SERVICE);
	}

	protected void RefreshCargoRows()
	{
		if (m_iMode == MODE_CARGO)
			RefreshCandidates();
	}

	protected void RefreshCargoBrowser()
	{
		if (!m_CargoBrowser)
			return;
		if (m_iMode != MODE_CARGO)
		{
			m_CargoBrowser.Close();
			return;
		}

		if (!m_CargoBrowser.IsOpen())
			m_CargoBrowser.Open("VEHICLE CARGO", m_CargoBrowseItems, ResourceName.Empty, string.Empty, false, true);

		map<ResourceName, int> counts = new map<ResourceName, int>();
		InventoryStorageManagerComponent inventory = GetCargoTargetInventory();
		array<IEntity> items = {};
		if (inventory)
			inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!DCO_VehicleServiceServer.IsCargoItem(item) || !item.GetPrefabData())
				continue;
			ResourceName prefab = item.GetPrefabData().GetPrefabName();
			int count;
			counts.Find(prefab, count);
			counts.Set(prefab, count + 1);
		}
		m_CargoBrowser.SetCounts(counts);
		string title = "VEHICLE CARGO  |  " + CargoItemCount().ToString() + " ITEMS LOADED";
		if (!m_CargoContainerPrefab.IsEmpty())
		{
			string containerName = NameFromResource(m_CargoContainerPrefab);
			containerName.ToUpper();
			title = "PACK " + containerName + "  |  < BACK RETURNS TO VEHICLE";
		}
		m_CargoBrowser.SetTitle(title);
	}

	protected int CargoItemCount()
	{
		int count;
		foreach (DCO_VehicleCargoEntry entry : m_Cargo)
		{
			if (entry)
				count += entry.m_iCount;
		}
		return count;
	}

	protected void RefreshCatalogRows()
	{
		if (m_iMode == MODE_CARGO)
			RefreshCandidates();
	}

	protected void RefreshCandidates()
	{
		ClearGRSACandidateRows();
		RefreshCargoBrowser();
		if (!m_GRSACandidatePanel)
			return;
		if (m_iMode != MODE_CARGO)
		{
			if (m_GRSACandidateClasses)
				m_GRSACandidateClasses.SetVisible(false);
			if (m_GRSACandidateCarousel)
				m_GRSACandidateCarousel.SetVisible(true);
			if (!m_iActiveVerb)
				m_GRSACandidatePanel.SetVisible(false);
			return;
		}

		m_GRSACandidatePanel.SetVisible(false);
		if (m_GRSACandidateClasses)
			m_GRSACandidateClasses.SetVisible(false);
		if (m_GRSACandidateCarousel)
			m_GRSACandidateCarousel.SetVisible(false);
		return;
	}

	protected void RefreshStats()
	{
		if (!m_GRSAScreen)
			return;
		Widget root = m_GRSAScreen.FindAnyWidget("StatsBlock");
		if (!root || !m_GRSAStatsText)
			return;
		root.SetVisible(ShowSideCards());
		if (!ShowSideCards())
			return;
		if (!m_SelectedVehicle)
		{
			if (m_GRSAStatsIconRow)
				m_GRSAStatsIconRow.SetVisible(false);
			if (m_GRSAStatsTitle)
				m_GRSAStatsTitle.SetText("VEHICLE SERVICE");
			m_GRSAStatsText.SetText("Park inside the white circle, stop, exit, then open Vehicle Service.");
			return;
		}
		if (m_GRSAStatsTitle)
			m_GRSAStatsTitle.SetText(EntityName(m_SelectedVehicle));
		if (m_GRSAStatsIconRow)
			m_GRSAStatsIconRow.SetVisible(true);
		if (m_GRSAStatsDamageValue)
			m_GRSAStatsDamageValue.SetText(m_Damage.Count().ToString() + " SYSTEMS");
		if (m_GRSAStatsAmmoValue)
		{
			int totalRounds;
			int totalCapacity;
			foreach (DCO_VehicleAmmoEntry ammo : m_Ammo)
			{
				if (!ammo)
					continue;
				totalRounds += ammo.m_iCurrent;
				totalCapacity += ammo.m_iMaximum;
			}
			m_GRSAStatsAmmoValue.SetText(totalRounds.ToString() + " / " + totalCapacity.ToString() + " ROUNDS");
		}
		if (m_GRSAStatsCargoValue)
			m_GRSAStatsCargoValue.SetText(CargoItemCount().ToString() + " CARGO");
		string text;
		if (m_iMode == MODE_SERVICE && m_iSelectedDamageHitZone >= 0)
		{
			foreach (DCO_VehicleDamageEntry entry : m_Damage)
			{
				if (!entry || entry.m_iHitZoneIndex != m_iSelectedDamageHitZone)
					continue;
				text += "INSPECTING  " + entry.m_sName;
				text += "\n" + entry.m_iHealthPercent.ToString() + "% HEALTH  |  " + entry.m_sDetail;
				break;
			}
		}
		if (!text.IsEmpty())
			text += "\n\n";
		text += "Drag the open stage to orbit. Use the wheel or stick to zoom.";
		m_GRSAStatsText.SetText(text);
	}

	protected bool ShowSideCards()
	{
		return m_iMode != MODE_CARGO && m_iActiveVerb == 0;
	}

	protected void SetStatus(bool success, string text)
	{
		if (!m_Status)
			return;
		if (text.Length() > 38)
			text = text.Substring(0, 35) + "...";
		m_Status.SetText(text);
		m_Status.SetColor(Color.FromRGBA(218, 224, 230, 255));
		if (!success)
			m_Status.SetColor(Color.FromRGBA(224, 82, 82, 255));
	}

	protected string EntityName(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "VEHICLE";

		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.ExtractEditableUIInfoFromPrefab(prefab);
		LocalizedString authoredName;
		if (info)
			authoredName = info.GetName();

		return DCO_GMDisplayName.Resolve(authoredName, prefab, "VEHICLE");
	}

	protected string NameFromResource(ResourceName resource)
	{
		if (resource.IsEmpty())
			return "UNNAMED ASSET";
		string name = resource.GetPath();
		int slash = name.LastIndexOf("/");
		if (slash >= 0)
			name = name.Substring(slash + 1, name.Length() - slash - 1);
		name.Replace(".et", string.Empty);
		name.Replace("_", " ");
		return name;
	}
}
