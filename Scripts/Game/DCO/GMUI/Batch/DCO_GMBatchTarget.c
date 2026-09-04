enum EDCO_GMBatchTargetCategory
{
	NONE = 0,
	ENTITY = 1,
	CHARACTER = 2,
	GROUP = 4,
	VEHICLE = 8,
	PLAYER = 16
}

class DCO_GMBatchTarget
{
	RplId m_RplId;
	EEditableEntityType m_EntityType;
	int m_iCategoryMask;
	int m_iPlayerId;

	bool HasCategory(int categoryMask)
	{
		return (m_iCategoryMask & categoryMask) != 0;
	}
}

class DCO_GMBatchTargetSet
{
	protected ref array<ref DCO_GMBatchTarget> m_aTargets;
	protected int m_iSkipped;
	protected int m_iFailed;

	protected array<ref DCO_GMBatchTarget> Targets()
	{
		if (!m_aTargets)
			m_aTargets = new array<ref DCO_GMBatchTarget>();
		return m_aTargets;
	}

	void Add(DCO_GMBatchTarget target)
	{
		if (target)
			Targets().Insert(target);
	}

	void RecordSkipped()
	{
		m_iSkipped++;
	}

	void RecordFailed()
	{
		m_iFailed++;
	}

	int GetTargetCount()
	{
		return Targets().Count();
	}

	DCO_GMBatchTarget GetTarget(int index)
	{
		if (index < 0 || index >= Targets().Count())
			return null;
		return Targets()[index];
	}

	bool Contains(RplId targetId)
	{
		foreach (DCO_GMBatchTarget target : Targets())
		{
			if (target && target.m_RplId == targetId)
				return true;
		}
		return false;
	}

	DCO_GMBatchActionResult CreateResult(string actionName)
	{
		DCO_GMBatchActionResult result = new DCO_GMBatchActionResult(actionName);
		result.RecordSkipped(m_iSkipped);
		result.RecordFailed(m_iFailed);
		return result;
	}
}

class DCO_GMBatchTargets
{
	static int Categorize(SCR_EditableEntityComponent editable)
	{
		if (!editable)
			return EDCO_GMBatchTargetCategory.NONE;

		int categories = EDCO_GMBatchTargetCategory.ENTITY;
		EEditableEntityType entityType = editable.GetEntityType();
		if (entityType == EEditableEntityType.CHARACTER)
			categories |= EDCO_GMBatchTargetCategory.CHARACTER;
		else if (entityType == EEditableEntityType.GROUP)
			categories |= EDCO_GMBatchTargetCategory.GROUP;
		else if (entityType == EEditableEntityType.VEHICLE)
			categories |= EDCO_GMBatchTargetCategory.VEHICLE;

		if (editable.GetPlayerID() > 0 || SCR_EditablePlayerDelegateComponent.Cast(editable))
			categories |= EDCO_GMBatchTargetCategory.PLAYER;
		return categories;
	}

	static DCO_GMBatchTargetSet Normalize(notnull set<SCR_EditableEntityComponent> selectedEntities, int allowedCategories = EDCO_GMBatchTargetCategory.ENTITY)
	{
		DCO_GMBatchTargetSet normalized = new DCO_GMBatchTargetSet();
		foreach (SCR_EditableEntityComponent editable : selectedEntities)
		{
			if (!editable)
			{
				normalized.RecordFailed();
				continue;
			}

			int categories = Categorize(editable);
			if ((categories & allowedCategories) == 0)
			{
				normalized.RecordSkipped();
				continue;
			}

			RplId targetId;
			if (!editable.IsReplicated(targetId) || !targetId.IsValid())
			{
				normalized.RecordFailed();
				continue;
			}

			if (normalized.Contains(targetId))
			{
				normalized.RecordSkipped();
				continue;
			}

			DCO_GMBatchTarget target = new DCO_GMBatchTarget();
			target.m_RplId = targetId;
			target.m_EntityType = editable.GetEntityType();
			target.m_iCategoryMask = categories;
			target.m_iPlayerId = editable.GetPlayerID();
			normalized.Add(target);
		}
		return normalized;
	}
}
