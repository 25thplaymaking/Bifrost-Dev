class DCO_GMAttachLink
{
	IEntity               m_Child;
	IEntity               m_Parent;
	int                   m_GMPlayerId;
	vector                m_Local[4];	// child's transform in the parent's local frame, captured at attach.
	ref map<IEntity, int> m_SavedLayers = new map<IEntity, int>();	// per-body original interaction masks.
}

class DCO_GMAttachState
{
	ref array<ref DCO_GMAttachLink> m_Links = {};
	ref array<ref Shape> m_HoverShapes = {};
}

class DCO_GMAttach
{
	protected static bool s_bArmed;	// ATTACH pressed: the next world click picks the parent.
	protected static ref DCO_GMAttachState s_State;
	protected static bool s_bTicking;
	static const int HOVER_COLOR = 0xFF3FB6E6;	// player cyan - shared with nametags / overlays.

	protected static DCO_GMAttachState State()
	{
		if (!s_State)
			s_State = new DCO_GMAttachState();
		return s_State;
	}

	static bool IsArmed()          { return s_bArmed; }
	static void SetArmed(bool on)  { s_bArmed = on; if (!on) ClearHighlight(); }

	static void TryAttach(IEntity child, vector ro, vector rd)
	{
		s_bArmed = false;
		ClearHighlight();
		if (!child)
			return;
		IEntity parent = RayPickEntity(child, ro, rd);
		if (!parent || parent == child)
		{
			Print("[DCO-ATTACH] no target under cursor - nothing attached", LogLevel.NORMAL);
			return;
		}
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		int gmPlayerId = -1;
		if (pc)
			gmPlayerId = pc.GetPlayerId();
		if (Replication.IsServer() && !DCO_GMRights.Allow(gmPlayerId, "GM attach"))
			return;
		if (!Replication.IsServer())
		{
			RplComponent childRpl = RplComponent.Cast(child.FindComponent(RplComponent));
			RplComponent parentRpl = RplComponent.Cast(parent.FindComponent(RplComponent));
			if (!pc || !childRpl || !parentRpl || !childRpl.Id().IsValid() || !parentRpl.Id().IsValid())
			{
				Print("[DCO-ATTACH] both objects must be replicated for a dedicated-server attachment", LogLevel.WARNING);
				return;
			}
			pc.DCO_SendGMAttach(childRpl.Id(), parentRpl.Id(), true);
			PopUp("ATTACH SENT");
			return;
		}
		AttachOnAuthority(child, parent, gmPlayerId);
	}

	static void ApplyRelayed(RplId childId, RplId parentId, bool attach, int gmPlayerId)
	{
		if (!Replication.IsServer())
			return;
		RplComponent childRpl = RplComponent.Cast(Replication.FindItem(childId));
		if (!childRpl || !childRpl.GetEntity())
			return;
		if (!attach)
		{
			DetachOnAuthority(childRpl.GetEntity(), gmPlayerId);
			return;
		}
		RplComponent parentRpl = RplComponent.Cast(Replication.FindItem(parentId));
		if (parentRpl && parentRpl.GetEntity())
			AttachOnAuthority(childRpl.GetEntity(), parentRpl.GetEntity(), gmPlayerId);
	}

	protected static void AttachOnAuthority(IEntity child, IEntity parent, int gmPlayerId)
	{
		if (!Replication.IsServer() || !child || !parent)
			return;
		if (FindLink(child))
			DetachOnAuthority(child, gmPlayerId);

		DCO_GMAttachLink link = new DCO_GMAttachLink();
		link.m_Child = child;
		link.m_Parent = parent;
		link.m_GMPlayerId = gmPlayerId;

		vector pw[4], cw[4];
		parent.GetWorldTransform(pw);
		child.GetWorldTransform(cw);
		Math3D.MatrixInvMultiply4(pw, cw, link.m_Local);

		FreezeRecursive(child, link);
		SCR_GarbageSystem gs = SCR_GarbageSystem.GetByEntityWorld(child);
		if (gs)
			gs.UpdateBlacklist(child, true);

		// Reparent into the vehicle's hierarchy and pin the captured pose.
		parent.AddChild(child, -1, EAddChildFlags.AUTO_TRANSFORM);
		child.SetLocalTransform(link.m_Local);

		State().m_Links.Insert(link);
		EnsureTick();
		PopUp("ATTACHED");
	}

	static void DetachSelected(IEntity child)
	{
		if (!child)
			return;
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		int gmPlayerId = -1;
		if (pc)
			gmPlayerId = pc.GetPlayerId();
		if (!Replication.IsServer())
		{
			RplComponent rpl = RplComponent.Cast(child.FindComponent(RplComponent));
			if (pc && rpl && rpl.Id().IsValid())
				pc.DCO_SendGMAttach(rpl.Id(), RplId.Invalid(), false);
			return;
		}
		DetachOnAuthority(child, gmPlayerId);
	}

	protected static void DetachOnAuthority(IEntity child, int gmPlayerId)
	{
		if (!Replication.IsServer() || !child)
			return;
		int idx = -1;
		for (int i = 0; i < State().m_Links.Count(); i++)
		{
			if (State().m_Links[i] && State().m_Links[i].m_Child == child && (gmPlayerId < 0 || State().m_Links[i].m_GMPlayerId == gmPlayerId))
			{
				idx = i;
				break;
			}
		}
		if (idx == -1)
		{
			Print("[DCO-ATTACH] selected object is not attached", LogLevel.NORMAL);
			return;
		}
		DCO_GMAttachLink link = State().m_Links[idx];
		State().m_Links.Remove(idx);
		StopTickIfIdle();
		ReverseLink(link);
		PopUp("DETACHED");
	}

	// Reverse EVERY open bond.
	static void DetachAll()
	{
		SetArmed(false);
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		int gmPlayerId = -1;
		if (pc)
			gmPlayerId = pc.GetPlayerId();
		if (!Replication.IsServer())
		{
			if (pc)
				pc.DCO_SendGMDetachAll();
			return;
		}
		DetachAllOnAuthority(gmPlayerId);
	}

	static void DetachAllOnAuthority(int gmPlayerId)
	{
		if (!Replication.IsServer())
			return;
		if (State().m_Links.IsEmpty())
			return;
		array<ref DCO_GMAttachLink> copy = {};
		for (int i = State().m_Links.Count() - 1; i >= 0; i--)
		{
			DCO_GMAttachLink link = State().m_Links[i];
			if (link && (gmPlayerId < 0 || link.m_GMPlayerId == gmPlayerId))
			{
				copy.Insert(link);
				State().m_Links.Remove(i);
			}
		}
		StopTickIfIdle();
		foreach (DCO_GMAttachLink l : copy)
			ReverseLink(l);
	}

	// Unparent one bond in place, restore its saved layers + live simulation, and re-enable garbage collection.
	protected static void ReverseLink(DCO_GMAttachLink link)
	{
		if (!link || !link.m_Child)
			return;
		IEntity child = link.m_Child;
		IEntity parent = link.m_Parent;
		if (!parent)
			parent = child.GetParent();
		if (parent)
			parent.RemoveChild(child, true);
		UnfreezeRecursive(child, link);
		SCR_GarbageSystem gs = SCR_GarbageSystem.GetByEntityWorld(child);
		if (gs)
			gs.UpdateBlacklist(child, false);
	}

	protected static void PopUp(string msg)
	{
		SCR_PopUpNotification n = SCR_PopUpNotification.GetInstance();
		if (n)
			n.PopupMsg(msg, duration: 2);
	}

	protected static DCO_GMAttachLink FindLink(IEntity child)
	{
		foreach (DCO_GMAttachLink l : State().m_Links)
		{
			if (l && l.m_Child == child)
				return l;
		}
		return null;
	}

	protected static void FreezeRecursive(IEntity e, DCO_GMAttachLink link)
	{
		if (!e)
			return;
		Physics ph = e.GetPhysics();
		if (ph)
		{
			link.m_SavedLayers.Insert(e, ph.GetInteractionLayer());
			// Interaction ONLY - NOT FireGeometry.
			ph.SetInteractionLayer(EPhysicsLayerDefs.Interaction);
			ph.SetActive(ActiveState.INACTIVE);
		}
		IEntity ch = e.GetChildren();
		while (ch)
		{
			FreezeRecursive(ch, link);
			ch = ch.GetSibling();
		}
	}

	protected static void UnfreezeRecursive(IEntity e, DCO_GMAttachLink link)
	{
		if (!e)
			return;
		Physics ph = e.GetPhysics();
		if (ph)
		{
			int saved;
			if (link.m_SavedLayers.Find(e, saved))
				ph.SetInteractionLayer(saved);
			ph.SetActive(ActiveState.ACTIVE);
		}
		IEntity ch = e.GetChildren();
		while (ch)
		{
			UnfreezeRecursive(ch, link);
			ch = ch.GetSibling();
		}
	}

	protected static void Tick()
	{
		for (int i = State().m_Links.Count() - 1; i >= 0; i--)
		{
			DCO_GMAttachLink l = State().m_Links[i];
			if (!l || !l.m_Child || !l.m_Parent || !DCO_GMRights.IsGameMaster(l.m_GMPlayerId))
			{
				if (l && l.m_Child)
					ReverseLink(l);
				State().m_Links.Remove(i);
			}
		}
		StopTickIfIdle();
	}

	protected static void EnsureTick()
	{
		if (s_bTicking)
			return;
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().CallLater(Tick, 250, true);
			s_bTicking = true;
		}
	}

	protected static void StopTickIfIdle()
	{
		if (s_bTicking && State().m_Links.IsEmpty() && GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(Tick);
			s_bTicking = false;
		}
	}

	protected static IEntity RayPickEntity(IEntity exclude, vector ro, vector rd)
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		TraceParam tp = new TraceParam();
		tp.Start = ro;
		tp.End = ro + rd * 2000;
		tp.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		tp.TargetLayers = EPhysicsLayerDefs.FireGeometry;	// solid, shootable geometry - vehicles/props/characters.
		tp.Exclude = exclude;
		world.TraceMove(tp, null);
		return tp.TraceEnt;
	}

	static void UpdateHoverHighlight(IEntity child, vector ro, vector rd)
	{
		State().m_HoverShapes.Clear();
		IEntity hover = RayPickEntity(child, ro, rd);
		if (!hover)
			return;
		vector mins, maxs;
		hover.GetBounds(mins, maxs);
		array<vector> c = {};
		c.Insert(Vector(mins[0], mins[1], mins[2]));
		c.Insert(Vector(maxs[0], mins[1], mins[2]));
		c.Insert(Vector(maxs[0], mins[1], maxs[2]));
		c.Insert(Vector(mins[0], mins[1], maxs[2]));
		c.Insert(Vector(mins[0], maxs[1], mins[2]));
		c.Insert(Vector(maxs[0], maxs[1], mins[2]));
		c.Insert(Vector(maxs[0], maxs[1], maxs[2]));
		c.Insert(Vector(mins[0], maxs[1], maxs[2]));
		for (int i = 0; i < 8; i++)
			c[i] = hover.CoordToParent(c[i]);
		ShapeFlags fl = ShapeFlags.VISIBLE | ShapeFlags.NOZBUFFER;
		Edge(c, 0, 1, fl); Edge(c, 1, 2, fl); Edge(c, 2, 3, fl); Edge(c, 3, 0, fl);
		Edge(c, 4, 5, fl); Edge(c, 5, 6, fl); Edge(c, 6, 7, fl); Edge(c, 7, 4, fl);
		Edge(c, 0, 4, fl); Edge(c, 1, 5, fl); Edge(c, 2, 6, fl); Edge(c, 3, 7, fl);
	}

	protected static void Edge(array<vector> c, int a, int b, ShapeFlags fl)
	{
		Shape s = Shape.Create(ShapeType.LINE, HOVER_COLOR, fl, c[a], c[b]);
		if (s)
			State().m_HoverShapes.Insert(s);
	}

	static void ClearHighlight()
	{
		State().m_HoverShapes.Clear();
	}
}
