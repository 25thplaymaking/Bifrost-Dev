// Bifrost GM group-order backend.
class DCO_GMGroupOrders
{
	static const int ORD_HOLD           = 100;
	static const int ORD_RESUME         = 101;
	static const int ORD_AMBUSH         = 102;
	static const int ORD_CANCEL_AMBUSH  = 103;
	static const int ORD_CQB            = 104;
	static const int ORD_QRF            = 105;
	static const int ORD_FORM_WEDGE     = 110;
	static const int ORD_FORM_LINE      = 111;
	static const int ORD_FORM_COLUMN    = 112;
	static const int ORD_FORM_STAGGERED = 113;
	static const int ORD_STANCE_STAND   = 120;
	static const int ORD_STANCE_CROUCH  = 121;
	static const int ORD_STANCE_PRONE   = 122;
	static const int ORD_STANCE_AUTO    = 123;	// release the GM stance lock -> back to autonomous stance control.
	static const int SUB_FORMATION      = 200;
	static const int SUB_STANCE         = 201;
	static const int SUB_BEHAVIOR       = 202;
	static const int SUB_TACTICS        = 203;
	static const int SUB_BACK           = 299;
	static const int TAC_PLACE_AMBUSH   = 400;
	static const int TAC_PLACE_KILLZONE = 401;
	static const int TAC_PLACE_DEFEND   = 402;
	static const int TAC_PLACE_QRF      = 403;
	static const int TAC_PLACE_CLEAR    = 404;

	static bool IsOrderAction(int actionId)
	{
		return actionId >= 100;
	}

	static bool IsTacticPlacement(int actionId)
	{
		return actionId >= 400 && actionId <= 449;
	}

	static bool IsSubmenu(int actionId)
	{
		return actionId >= 200 && actionId <= 299;
	}

	static void BuildRoot(out notnull array<string> labels, out notnull array<int> ids)
	{
		labels.Clear();
		ids.Clear();
		labels.Insert("Formation  ▸"); ids.Insert(SUB_FORMATION);
		labels.Insert("Stance  ▸");    ids.Insert(SUB_STANCE);
		labels.Insert("Behavior  ▸");  ids.Insert(SUB_BEHAVIOR);
		labels.Insert("Set Ambush");        ids.Insert(ORD_AMBUSH);
		labels.Insert("Cancel Ambush");     ids.Insert(ORD_CANCEL_AMBUSH);
		labels.Insert("CQB Clear");         ids.Insert(ORD_CQB);
		labels.Insert("QRF Stage");         ids.Insert(ORD_QRF);
	}

	static void BuildSubmenu(int submenuId, out notnull array<string> labels, out notnull array<int> ids)
	{
		if (submenuId == SUB_BACK)
		{
			BuildRoot(labels, ids);
			return;
		}
		labels.Clear();
		ids.Clear();
		labels.Insert("◂ Back"); ids.Insert(SUB_BACK);
		switch (submenuId)
		{
			case SUB_FORMATION:
				labels.Insert("Wedge");   ids.Insert(ORD_FORM_WEDGE);
				labels.Insert("Line");    ids.Insert(ORD_FORM_LINE);
				labels.Insert("Column");  ids.Insert(ORD_FORM_COLUMN);
				labels.Insert("Stagger"); ids.Insert(ORD_FORM_STAGGERED);
				break;
			case SUB_STANCE:
				labels.Insert("Stand");          ids.Insert(ORD_STANCE_STAND);
				labels.Insert("Crouch");         ids.Insert(ORD_STANCE_CROUCH);
				labels.Insert("Prone");          ids.Insert(ORD_STANCE_PRONE);
				labels.Insert("Auto (release)"); ids.Insert(ORD_STANCE_AUTO);
				break;
			case SUB_BEHAVIOR:
				labels.Insert("Hold Fire");   ids.Insert(ORD_HOLD);
				labels.Insert("Resume Fire"); ids.Insert(ORD_RESUME);
				break;
		}
	}

	static void BuildCategoryOptions(int categoryId, out notnull array<string> labels, out notnull array<int> ids)
	{
		labels.Clear();
		ids.Clear();
		switch (categoryId)
		{
			case SUB_FORMATION:
				labels.Insert("Wedge");   ids.Insert(ORD_FORM_WEDGE);
				labels.Insert("Line");    ids.Insert(ORD_FORM_LINE);
				labels.Insert("Column");  ids.Insert(ORD_FORM_COLUMN);
				labels.Insert("Stagger"); ids.Insert(ORD_FORM_STAGGERED);
				break;
			case SUB_STANCE:
				labels.Insert("Stand");          ids.Insert(ORD_STANCE_STAND);
				labels.Insert("Crouch");         ids.Insert(ORD_STANCE_CROUCH);
				labels.Insert("Prone");          ids.Insert(ORD_STANCE_PRONE);
				labels.Insert("Auto (release)"); ids.Insert(ORD_STANCE_AUTO);
				break;
			case SUB_BEHAVIOR:
				labels.Insert("Hold Fire");   ids.Insert(ORD_HOLD);
				labels.Insert("Resume Fire"); ids.Insert(ORD_RESUME);
				break;
			case SUB_TACTICS:
				labels.Insert("Ambush Position"); ids.Insert(TAC_PLACE_AMBUSH);
				labels.Insert("Kill-Zone");       ids.Insert(TAC_PLACE_KILLZONE);
				labels.Insert("Defend Area");     ids.Insert(TAC_PLACE_DEFEND);
				labels.Insert("QRF Stage Area");  ids.Insert(TAC_PLACE_QRF);
				labels.Insert("Clear Building");  ids.Insert(TAC_PLACE_CLEAR);
				break;
		}
	}

	static void BuildMenu(out notnull array<string> labels, out notnull array<int> ids)
	{
		labels.Clear();
		ids.Clear();
		labels.Insert("Hold Fire");         ids.Insert(ORD_HOLD);
		labels.Insert("Resume Fire");       ids.Insert(ORD_RESUME);
		labels.Insert("Set Ambush");        ids.Insert(ORD_AMBUSH);
		labels.Insert("Cancel Ambush");     ids.Insert(ORD_CANCEL_AMBUSH);
		labels.Insert("CQB Clear");         ids.Insert(ORD_CQB);
		labels.Insert("QRF Stage");         ids.Insert(ORD_QRF);
		labels.Insert("Formation: Wedge");  ids.Insert(ORD_FORM_WEDGE);
		labels.Insert("Formation: Line");   ids.Insert(ORD_FORM_LINE);
		labels.Insert("Formation: Column"); ids.Insert(ORD_FORM_COLUMN);
		labels.Insert("Formation: Stagger");ids.Insert(ORD_FORM_STAGGERED);
	}

	// Apply a leaf order to the group's editable entity.
	void Apply(int actionId, SCR_EditableEntityComponent e)
	{
		if (!e)
			return;
		SCR_AIGroupUtilityComponent util = DCO_QRFAttributeHelper.GetGroupUtility(e);
		if (!util)
		{
			Print("[DCO-GM] order: entity has no AI group utility", LogLevel.NORMAL);
			return;
		}
		util.DCO_SendGMOrder(actionId);
		Print(string.Format("[DCO-GM] group order applied (id=%1)", actionId), LogLevel.NORMAL);
	}

	// Server side of the remote-GM order leg: resolve the wire id back to the group and apply.
	static void ApplyRelayed(RplId groupId, int actionId)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(groupId));
		if (!rpl)
		{
			Print("[DCO-GM] order relay: RplId did not resolve on the server - order dropped", LogLevel.WARNING);
			return;
		}
		SCR_AIGroup grp = SCR_AIGroup.Cast(rpl.GetEntity());
		if (!grp)
			return;
		ApplyToUtil(grp.GetGroupUtilityComponent(), actionId);
	}

	static void ApplyToUtil(SCR_AIGroupUtilityComponent util, int actionId)
	{
		if (!util)
			return;
		if (actionId == ORD_STANCE_AUTO)   { util.DCO_SetGMStance(-1);                    return; }
		if (actionId == ORD_STANCE_STAND)  { util.DCO_SetGMStance(ECharacterStance.STAND);  return; }
		if (actionId == ORD_STANCE_CROUCH) { util.DCO_SetGMStance(ECharacterStance.CROUCH); return; }
		if (actionId == ORD_STANCE_PRONE)  { util.DCO_SetGMStance(ECharacterStance.PRONE);  return; }
		switch (actionId)
		{
			case ORD_HOLD:           util.DCO_SetManualHold(true);  break;
			case ORD_RESUME:         util.DCO_SetManualHold(false); break;
			case ORD_AMBUSH:         util.DCO_SetAmbusher(true);    break;
			case ORD_CANCEL_AMBUSH:  util.DCO_SetAmbusher(false);   break;
			case ORD_CQB:            util.DCO_SetCqbClearer(true);  break;
			case ORD_QRF:            util.DCO_SetQRFResponder(true);break;
			case ORD_FORM_WEDGE:     util.DCO_OrderFormation(0);    break;
			case ORD_FORM_LINE:      util.DCO_OrderFormation(1);    break;
			case ORD_FORM_COLUMN:    util.DCO_OrderFormation(2);    break;
			case ORD_FORM_STAGGERED: util.DCO_OrderFormation(3);    break;
		}
	}
}
