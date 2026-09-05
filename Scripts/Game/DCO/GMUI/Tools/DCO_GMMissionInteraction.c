[ComponentEditorProps(category: "Bifrost/Mission", description: "Player-usable intel or teleporter bound to an editable object")]
class DCO_GMMissionInteractionComponentClass : ScriptComponentClass {}

class DCO_GMMissionInteractionComponent : ScriptComponent
{
	static const int INTEL = 1;
	static const int TELEPORTER = 2;
	protected static ref array<DCO_GMMissionInteractionComponent> s_Instances;
	[RplProp()] int m_iKind;
	[RplProp()] RplId m_TargetId;
	[RplProp()] string m_sTitle;
	[RplProp()] string m_sPair;
	[RplProp()] bool m_bReady;
	string m_sBody;
	int m_iScope;
	bool m_bRemoveClue;
	protected float m_fLastArrival = -10000;
	protected ref set<string> m_UsedBy = new set<string>();

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_Instances)
			s_Instances = {};
		s_Instances.Insert(this);
		GetGame().GetCallqueue().CallLater(FollowTarget, 500, true);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(FollowTarget);
		if (s_Instances)
			s_Instances.RemoveItem(this);
		super.OnDelete(owner);
	}

	static int Count()
	{
		if (!s_Instances)
			return 0;
		return s_Instances.Count();
	}

	static bool CanBind(IEntity entity)
	{
		if (!entity || entity.GetParent() || ChimeraCharacter.Cast(entity) || Vehicle.Cast(entity) || Building.Cast(entity)) return false;
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		if (!editable || SCR_EditableSystemComponent.Cast(editable)) return false;
		return entity.GetVObject() || entity.GetChildren();
	}

	IEntity Target()
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(m_TargetId));
		if (!editable)
			return null;
		return editable.GetOwner();
	}

	static DCO_GMMissionInteractionComponent FindTarget(RplId id)
	{
		if (!s_Instances)
			return null;
		foreach (DCO_GMMissionInteractionComponent point : s_Instances)
		{
			if (point && point.m_TargetId == id)
				return point;
		}
		return null;
	}

	protected void FollowTarget()
	{
		if (!m_bReady || !Replication.IsServer())
			return;
		IEntity target = Target();
		if (!target)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(GetOwner());
			return;
		}
		GetOwner().SetOrigin(target.GetOrigin());
	}

	void Configure(int kind, RplId targetId, string title, string body, int scope, bool removeClue)
	{
		if (!Replication.IsServer())
			return;
		m_iKind = kind;
		m_TargetId = targetId;
		m_sTitle = title;
		if (m_sBody != body || m_iScope != scope) m_UsedBy.Clear();
		m_sBody = body;
		m_sPair = "";
		if (kind == TELEPORTER)
			m_sPair = body;
		m_iScope = scope;
		m_bRemoveClue = removeClue;
		m_bReady = true;
		Replication.BumpMe();
		FollowTarget();
	}

	bool IsNear(IEntity user)
	{
		IEntity target = Target();
		if (!m_bReady || !target || !user || vector.DistanceSq(target.GetOrigin(), user.GetOrigin()) > 20.25)
			return false;
		TraceParam trace = new TraceParam();
		trace.Start = user.GetOrigin() + "0 1.4 0";
		trace.End = target.GetOrigin() + "0 0.3 0";
		trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		trace.TargetLayers = EPhysicsLayerDefs.FireGeometry;
		trace.Exclude = user;
		float fraction = GetGame().GetWorld().TraceMove(trace, null);
		return fraction >= 0.98 || trace.TraceEnt == target;
	}

	static DCO_GMMissionInteractionComponent FindNearby(IEntity user)
	{
		if (!s_Instances || !user)
			return null;
		DCO_GMMissionInteractionComponent best;
		float distance = 20.25;
		foreach (DCO_GMMissionInteractionComponent point : s_Instances)
		{
			if (!point || !point.IsNear(user))
				continue;
			float candidate = vector.DistanceSq(point.Target().GetOrigin(), user.GetOrigin());
			if (candidate < distance)
			{
				distance = candidate;
				best = point;
			}
		}
		return best;
	}

	static int PairCount(string link, DCO_GMMissionInteractionComponent exclude)
	{
		int count;
		if (!s_Instances) return count;
		foreach (DCO_GMMissionInteractionComponent point : s_Instances)
		{
			if (point && point != exclude && point.m_bReady && point.m_iKind == TELEPORTER && point.m_sPair == link && point.Target()) count++;
		}
		return count;
	}

	static void DrawCues(DCO_GMRenderManager render)
	{
		if (!s_Instances || !DCO_GMRights.IsLocalGameMaster()) return;
		foreach (DCO_GMMissionInteractionComponent point : s_Instances)
		{
			if (!point || !point.m_bReady || !point.Target()) continue;
			vector centre = point.Target().GetOrigin() + "0 0.3 0";
			render.DrawRing(centre, vector.Right, vector.Forward, 4.5, 0xFFD9892B);
			DCO_GMMissionInteractionComponent paired = point.PairedEndpoint();
			if (paired) render.DrawArrow(centre, paired.Target().GetOrigin() + "0 0.3 0", 0.2, 0xFF68B7CC);
		}
	}

	DCO_GMMissionInteractionComponent PairedEndpoint()
	{
		if (!s_Instances || m_sPair.IsEmpty())
			return null;
		DCO_GMMissionInteractionComponent result;
		foreach (DCO_GMMissionInteractionComponent other : s_Instances)
		{
			if (!other || other == this || !other.m_bReady || other.m_iKind != TELEPORTER || other.m_sPair != m_sPair || !other.Target())
				continue;
			if (result)
				return null;
			result = other;
		}
		return result;
	}

	bool Use(SCR_PlayerController controller, out string result)
	{
		result = "Move closer to the interaction object.";
		if (!Replication.IsServer() || !controller)
			return false;
		IEntity user = controller.GetControlledEntity();
		ChimeraCharacter character = ChimeraCharacter.Cast(user);
		if (!character || !IsNear(user) || (character.GetCharacterController().IsDead() || character.GetCharacterController().IsUnconscious()))
			return false;
		if (m_iKind == TELEPORTER)
		{
			result = "A teleporter needs exactly two endpoints with the same link name.";
			DCO_GMMissionInteractionComponent destination = PairedEndpoint();
			if (!destination)
				return false;
			float now = GetGame().GetWorld().GetWorldTime();
			if (now - destination.m_fLastArrival < 1500)
			{
				result = "Another player is arriving. Try again in a moment.";
				return false;
			}
			if (!DCO_GMMissionServer.Teleport(controller, destination.Target(), result)) return false;
			destination.m_fLastArrival = now;
			return true;
		}
		string identity = SCR_PlayerIdentityUtils.GetPlayerIdentityId(controller.GetPlayerId());
		result = "This intel has already been collected.";
		if ((identity.IsEmpty() || identity == UUID.NULL_UUID) || m_UsedBy.Contains(identity))
			return false;
		if (!DCO_GMMissionJournal.Award(controller, m_sTitle, m_sBody, m_iScope))
		{
			result = "Intel could not be recorded for this audience.";
			return false;
		}
		m_UsedBy.Insert(identity);
		result = "Intel recorded in the map journal.";
		if (m_bRemoveClue && CanBind(Target()))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(Target());
			SCR_EntityHelper.DeleteEntityAndChildren(GetOwner());
		}
		return true;
	}
}

class DCO_UseMissionInteractionAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.Cast(pOwnerEntity.FindComponent(DCO_GMMissionInteractionComponent));
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		RplComponent rpl = RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent));
		if (controller && rpl && point && point.IsNear(pUserEntity))
			controller.DCO_UseMissionPoint(rpl.Id());
	}

	override bool HasLocalEffectOnlyScript() { return true; }
	override bool CanBeShownScript(IEntity user)
	{
		DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.Cast(GetOwner().FindComponent(DCO_GMMissionInteractionComponent));
		return point && point.IsNear(user);
	}
	override bool CanBePerformedScript(IEntity user) { return CanBeShownScript(user); }
	override bool GetActionNameScript(out string outName)
	{
		DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.Cast(GetOwner().FindComponent(DCO_GMMissionInteractionComponent));
		if (!point)
			return false;
		outName = "Read Intel: " + point.m_sTitle;
		if (point.m_iKind == DCO_GMMissionInteractionComponent.TELEPORTER)
		{
			outName = "Teleporter: " + point.m_sTitle + " (unlinked)";
			DCO_GMMissionInteractionComponent other = point.PairedEndpoint();
			if (other)
				outName = "Travel to " + other.m_sTitle;
		}
		return true;
	}
}

modded class SCR_InteractionHandlerComponent
{
	protected DCO_GMMissionInteractionComponent m_DCO_MissionPoint;
	override protected void HandleOverride(notnull ChimeraCharacter character)
	{
		m_DCO_MissionPoint = null;
		super.HandleOverride(character);
		DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.FindNearby(character);
		if (!point || !point.GetOwner())
			return;
		m_DCO_MissionPoint = point;
		point.GetOwner().SetOrigin(point.Target().GetOrigin());
		DCO_ArsenalAccessComponent.AppendActionOwners(point.GetOwner(), m_aCollectedEntities);
		DCO_ArsenalAccessComponent.AppendActionOwners(point.GetOwner(), m_aCollectedNearbyEntities);
		SetManualNearbyCollectionOverride(true);
	}
	override array<IEntity> GetManualNearbyOverrideList(IEntity owner, out vector referencePoint)
	{
		if (!m_DCO_MissionPoint || !m_DCO_MissionPoint.Target())
			return super.GetManualNearbyOverrideList(owner, referencePoint);
		referencePoint = m_DCO_MissionPoint.Target().GetOrigin();
		return m_aCollectedNearbyEntities;
	}
}

class DCO_GMIntelAward
{
	int m_iId;
	string m_sAudience;
	string m_sTitle;
	string m_sBody;
}

class DCO_GMMissionJournal
{
	protected static BaseWorld s_World;
	protected static ref array<ref DCO_GMIntelAward> s_Awards;
	protected static ref array<ref SCR_JournalEntry> s_Local;
	protected static ref ScriptInvoker s_OnChanged;

	static ScriptInvoker GetOnChanged()
	{
		if (!s_OnChanged)
			s_OnChanged = new ScriptInvoker();
		return s_OnChanged;
	}
	static void NotifyChanged() { GetOnChanged().Invoke(); }

	protected static void EnsureWorld()
	{
		if (s_World == GetGame().GetWorld() && s_Awards)
			return;
		s_World = GetGame().GetWorld();
		s_Awards = {};
		s_Local = {};
	}
	static string FactionOf(SCR_PlayerController controller)
	{
		SCR_PlayerFactionAffiliationComponent affiliation = SCR_PlayerFactionAffiliationComponent.Cast(controller.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!affiliation || !affiliation.GetAffiliatedFaction())
			return "";
		return affiliation.GetAffiliatedFaction().GetFactionKey();
	}
	static bool Award(SCR_PlayerController finder, string title, string body, int scope)
	{
		EnsureWorld();
		if (!Replication.IsServer() || s_Awards.Count() >= 256)
			return false;
		string audience = "all";
		if (scope == 0)
		{
			string uid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(finder.GetPlayerId());
			if (uid.IsEmpty() || uid == UUID.NULL_UUID)
				return false;
			audience = "player:" + uid;
		}
		else if (scope == 1)
		{
			string faction = FactionOf(finder);
			if (faction.IsEmpty())
				return false;
			audience = "faction:" + faction;
		}
		foreach (DCO_GMIntelAward existing : s_Awards)
		{
			if (existing.m_sAudience == audience && existing.m_sTitle == title && existing.m_sBody == body)
				return true;
		}
		DCO_GMIntelAward award = new DCO_GMIntelAward();
		award.m_iId = 20000 + s_Awards.Count();
		award.m_sAudience = audience;
		award.m_sTitle = title;
		award.m_sBody = body;
		s_Awards.Insert(award);
		array<int> ids = {};
		GetGame().GetPlayerManager().GetPlayers(ids);
		foreach (int id : ids)
		{
			SCR_PlayerController recipient = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(id));
			if (recipient && CanRead(recipient, award))
				recipient.DCO_DeliverIntel(award.m_iId, title, body, true);
		}
		return true;
	}
	protected static bool CanRead(SCR_PlayerController controller, DCO_GMIntelAward award)
	{
		string uid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(controller.GetPlayerId());
		return award.m_sAudience == "all" || award.m_sAudience == "player:" + uid || award.m_sAudience == "faction:" + FactionOf(controller);
	}
	static void Snapshot(SCR_PlayerController controller)
	{
		EnsureWorld();
		if (!Replication.IsServer())
			return;
		controller.DCO_ClearIntel();
		foreach (DCO_GMIntelAward award : s_Awards)
		{
			if (CanRead(controller, award))
				controller.DCO_DeliverIntel(award.m_iId, award.m_sTitle, award.m_sBody, false);
		}
		controller.DCO_FinishIntel();
	}
	static void ClearLocal()
	{
		EnsureWorld();
		s_Local.Clear();
	}
	static void Receive(int id, string title, string body, bool notify)
	{
		EnsureWorld();
		foreach (SCR_JournalEntry existing : s_Local)
		{
			if (existing.GetEntryID() == id)
				return;
		}
		DCO_GMIntelJournalEntry entry = new DCO_GMIntelJournalEntry();
		entry.DCO_ConfigureIntel(id, title, body);
		s_Local.Insert(entry);
		if (notify)
		{
			SCR_HintManagerComponent.ShowCustomHint("Intel added to your map journal.", title, 8);
			NotifyChanged();
		}
	}
	static void AppendTo(SCR_JournalConfig journal)
	{
		EnsureWorld();
		array<ref SCR_JournalEntry> entries = journal.GetEntries();
		if (!entries)
			return;
		for (int i = entries.Count() - 1; i >= 0; i--)
		{
			if (DCO_GMIntelJournalEntry.Cast(entries[i]))
				entries.Remove(i);
		}
		foreach (SCR_JournalEntry entry : s_Local)
			entries.Insert(entry);
	}
}

class DCO_GMIntelJournalEntry : SCR_JournalEntry
{
	void DCO_ConfigureIntel(int id, string title, string body)
	{
		m_iEntryID = id;
		m_eJournalEntryType = SCR_EJournalEntryType.Custom;
		m_sCustomEntryName = title;
		m_sEntryText = body;
		m_sEntryButtonLayout = "{3EAF5389D3D89EE5}UI/layouts/Menus/DeployMenu/JournalButton.layout";
		m_bUseCustomLayout = false;
	}
}

modded class SCR_MapJournalUI
{
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		DCO_GMMissionJournal.GetOnChanged().Remove(DCO_RefreshIntel);
		DCO_GMMissionJournal.GetOnChanged().Insert(DCO_RefreshIntel);
		DCO_RequestJournal();
	}
	protected void DCO_RequestJournal()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.DCO_RequestIntel();
	}
	override protected void OnPlayerFactionResponse(SCR_PlayerFactionAffiliationComponent component, int factionIndex, bool response)
	{
		if (response)
			DCO_GMMissionJournal.ClearLocal();
		super.OnPlayerFactionResponse(component, factionIndex, response);
		if (response)
		{
			GetGame().GetCallqueue().Remove(DCO_RequestJournal);
			GetGame().GetCallqueue().CallLater(DCO_RequestJournal, 1100, false);
		}
	}
	protected void DCO_RefreshIntel()
	{
		if (!m_JournalConfig || !m_wJournalFrame || !m_Widgets.m_wEntriesWrapper)
			return;
		int selected = m_iCurrentEntryId;
		GetJournalForPlayer();
		if (selected >= 0 && selected < m_aEntries.Count())
			ShowEntryByID(selected);
	}
	override void HandlerDeattached(Widget w)
	{
		GetGame().GetCallqueue().Remove(DCO_RequestJournal);
		DCO_GMMissionJournal.GetOnChanged().Remove(DCO_RefreshIntel);
		super.HandlerDeattached(w);
	}
	override protected void GetJournalForPlayer()
	{
		m_LastInteractedEntry = null;
		if (m_Widgets.m_wEntriesWrapper)
		{
			Widget child = m_Widgets.m_wEntriesWrapper.GetChildren();
			while (child)
			{
				child.RemoveFromHierarchy();
				child = m_Widgets.m_wEntriesWrapper.GetChildren();
			}
		}
		if (m_Widgets.m_wEntryLayout)
			m_Widgets.m_wEntryLayout.SetVisible(false);
		if (m_JournalConfig)
		{
			FactionKey key = FactionKey.Empty;
			if (m_PlyFactionAffilComp && m_PlyFactionAffilComp.GetAffiliatedFaction())
				key = m_PlyFactionAffilComp.GetAffiliatedFaction().GetFactionKey();
			SCR_JournalConfig journal = m_JournalConfig.GetJournalConfig(key);
			if (journal)
				DCO_GMMissionJournal.AppendTo(journal);
		}
		super.GetJournalForPlayer();
	}
}
