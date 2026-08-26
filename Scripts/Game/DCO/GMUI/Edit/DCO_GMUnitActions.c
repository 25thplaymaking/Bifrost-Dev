
class DCO_GMUnitActions
{
	static const int ACT_ORIENT  = 0;
	static const int ACT_FOLLOW  = 1;
	static const int ACT_CONTROL = 2;
	static const int ACT_EDIT    = 3;
	static const int ACT_ENTER_LAYER  = 4;
	static const int ACT_EXIT_LAYER   = 5;
	static const int ACT_CREATE_LAYER = 6;
	static const int ACT_ARSENAL = 7;	// Open the existing Bifrost Arsenal for a character.
	static const int ACT_RESTOCK = 8;

	protected SCR_EditableEntityComponent m_FollowTarget;

	void Perform(int actionId, SCR_EditableEntityComponent e)
	{
		if (!e)
			return;
		switch (actionId)
		{
			case ACT_ORIENT:  OrientCamera(e); break;
			case ACT_FOLLOW:  ToggleFollow(e); break;
			case ACT_CONTROL: InvokeAction(new SCR_TakeControlContextAction(), e); break;
			case ACT_EDIT:    InvokeAction(new SCR_OpenAttributeWindowContextAction(), e); break;
			case ACT_ARSENAL:
			{
				if (e.GetOwner() && !e.IsDestroyed())
					DCO_GMArsenalPanel.Get().OpenFor(e);
				break;
			}
			case ACT_RESTOCK:
			{
				if (e.GetOwner() && !e.IsDestroyed())
					DCO_ArsenalServer.Route(DCO_ArsenalServer.VERB_RESTOCK, e.GetOwner(), "");
				break;
			}
		}
	}

	void EditAttributes(SCR_EditableEntityComponent e)
	{
		InvokeAction(new SCR_OpenAttributeWindowContextAction(), e);
	}

	bool CanPerform(int actionId, SCR_EditableEntityComponent e)
	{
		if (!e)
			return false;
		switch (actionId)
		{
			case ACT_ORIENT:  return true;
			case ACT_FOLLOW:  return true;
			case ACT_CONTROL: return CanInvoke(new SCR_TakeControlContextAction(), e);
			case ACT_EDIT:    return CanInvoke(new SCR_OpenAttributeWindowContextAction(), e);
		}
		return false;
	}

	protected bool CanInvoke(SCR_BaseEditorAction action, SCR_EditableEntityComponent e)
	{
		if (!action || !e)
			return false;
		set<SCR_EditableEntityComponent> sel = new set<SCR_EditableEntityComponent>();
		sel.Insert(e);
		vector pos;
		GetEntityPos(e, pos);
		return action.CanBeShown(e, sel, pos, 0);
	}

	void Shutdown()
	{
		StopFollow();
	}

	void OrientCamera(SCR_EditableEntityComponent e)
	{
		vector pos;
		if (GetEntityPos(e, pos))
			TeleportCameraTo(pos);
	}

	void ToggleFollow(SCR_EditableEntityComponent e)
	{
		if (m_FollowTarget == e)
		{
			StopFollow();
			return;
		}
		m_FollowTarget = e;
		GetGame().GetCallqueue().Remove(FollowTick);
		GetGame().GetCallqueue().CallLater(FollowTick, 200, true);
		Print("[DCO-GM] camera following unit", LogLevel.NORMAL);
	}

	void StopFollow()
	{
		m_FollowTarget = null;
		GetGame().GetCallqueue().Remove(FollowTick);
	}

	protected void FollowTick()
	{
		vector pos;
		if (m_FollowTarget && GetEntityPos(m_FollowTarget, pos))
			TeleportCameraTo(pos);
		else
			StopFollow();
	}

	protected void TeleportCameraTo(vector pos)
	{
		SCR_CameraEditorComponent cm = SCR_CameraEditorComponent.Cast(SCR_CameraEditorComponent.GetInstance(SCR_CameraEditorComponent));
		if (!cm)
			return;
		SCR_ManualCamera cam = cm.GetCamera();
		if (!cam)
			return;
		SCR_TeleportToCursorManualCameraComponent tp = SCR_TeleportToCursorManualCameraComponent.Cast(cam.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
		if (tp)
			tp.TeleportCamera(pos);
	}

	protected void InvokeAction(SCR_BaseEditorAction action, SCR_EditableEntityComponent e)
	{
		if (!action || !e)
			return;
		set<SCR_EditableEntityComponent> sel = new set<SCR_EditableEntityComponent>();
		sel.Insert(e);
		vector pos;
		GetEntityPos(e, pos);
		if (action.CanBePerformed(e, sel, pos, 0))
			action.Perform(e, sel, pos, 0);
		else
			Print("[DCO-GM] unit action not available for this entity", LogLevel.NORMAL);
	}

	protected bool GetEntityPos(SCR_EditableEntityComponent e, out vector pos)
	{
		if (!e)
			return false;
		IEntity owner = e.GetOwner();
		if (!owner)
			return false;
		pos = owner.GetOrigin();
		return true;
	}
}
