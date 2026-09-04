class DCO_GMBatchAuthority
{
	static bool Resolve(RplId targetId, int allowedCategories, out SCR_EditableEntityComponent editable, out int categories)
	{
		editable = null;
		categories = EDCO_GMBatchTargetCategory.NONE;
		if (!Replication.IsServer() || !targetId.IsValid())
			return false;

		editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(targetId));
		if (!editable || !editable.GetOwner())
			return false;

		categories = DCO_GMBatchTargets.Categorize(editable);
		if ((categories & allowedCategories) == 0)
		{
			editable = null;
			return false;
		}
		return true;
	}

	static bool Resolve(DCO_GMBatchTarget target, int allowedCategories, out SCR_EditableEntityComponent editable, out int categories)
	{
		if (!target)
		{
			editable = null;
			categories = EDCO_GMBatchTargetCategory.NONE;
			return false;
		}
		return Resolve(target.m_RplId, allowedCategories, editable, categories);
	}
}
