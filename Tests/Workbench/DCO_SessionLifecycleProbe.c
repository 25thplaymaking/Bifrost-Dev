modded class DCO_GMScenarioPanel
{
	int DCO_TestPassed;
	ref array<string> DCO_TestFailures = {};

	protected void DCO_TestExpect(bool condition, string label)
	{
		if (condition) DCO_TestPassed++;
		else DCO_TestFailures.Insert(label);
	}

	void DCO_RunSessionLifecycleProbe()
	{
		if (GetGame().InPlayMode()) { DCO_TestFailures.Insert("Edit mode required"); return; }
		ref ScriptInvoker confirmed = new ScriptInvoker();
		ref ScriptInvoker cancelled = new ScriptInvoker();
		confirmed.Insert(OnAttributesEnded);
		cancelled.Insert(OnAttributesEnded);
		ScriptCallQueue queue = GetGame().GetCallqueue();
		int notQueued = queue.GetRemainingTime(RefreshConditionalRows);
		for (int cycle = 0; cycle < 3; cycle++)
		{
			m_bOpen = true;
			m_bEditing = true;
			m_bTriggerSession = true;
			m_aSessionAttributes = {};
			QueueCategoryRefresh();
			OnAttributeChanged();
			m_TimeDateOpenAttempts = 0;
			SelectTimeAndDateCategory();
			DCO_TestExpect(queue.GetRemainingTime(RenderSelectedCategory) != notQueued, "Category refresh queued before end");
			DCO_TestExpect(queue.GetRemainingTime(RefreshConditionalRows) != notQueued, "Conditional refresh queued before end");
			if (cycle == 1) cancelled.Invoke(m_aSessionAttributes);
			else confirmed.Invoke(m_aSessionAttributes);
			DCO_TestExpect(!m_bOpen && !m_bEditing && !m_aSessionAttributes, "End releases the old editing session");
			DCO_TestExpect(!m_bTriggerSession && !m_bConditionalRefreshQueued && !m_bCategoryRefreshQueued, "End clears modal and refresh state");
			// Reopen before the old callbacks are due; cancelled work must not touch this session.
			m_bOpen = true;
			m_bEditing = true;
			m_aSessionAttributes = {};
			m_bCategoryRefreshQueued = true;
			m_bConditionalRefreshQueued = true;
			int attempts = m_TimeDateOpenAttempts;
			queue.Tick(0.2);
			DCO_TestExpect(m_bCategoryRefreshQueued && m_bConditionalRefreshQueued && m_TimeDateOpenAttempts == attempts, "Old callbacks cannot modify a reopened session");
			DCO_TestExpect(queue.GetRemainingTime(RenderSelectedCategory) == notQueued, "Old category callback drained");
			DCO_TestExpect(queue.GetRemainingTime(RefreshConditionalRows) == notQueued, "Old conditional callback drained");
			DCO_TestExpect(queue.GetRemainingTime(SelectTimeAndDateCategory) == notQueued, "Old deferred selection drained");
			confirmed.Invoke(m_aSessionAttributes);
		}
		m_bOpen = true;
		m_bEditing = false;
		m_bTriggerSession = true;
		m_aSessionAttributes = {};
		QueueCategoryRefresh();
		OnAttributeChanged();
		ref DCO_ScenarioBackdropHandler backdrop = new DCO_ScenarioBackdropHandler(this);
		DCO_TestExpect(backdrop.OnMouseButtonDown(null, 0, 0, 0) && m_bOpen, "Outside press is consumed before dismissal");
		DCO_TestExpect(backdrop.OnMouseButtonUp(null, 0, 0, 0) && m_bOpen, "Release without a backdrop target is consumed");
		DCO_TestExpect(backdrop.OnClick(null, 0, 0, 1) && m_bOpen, "Other mouse buttons do not dismiss properties");
		DCO_TestExpect(backdrop.OnClick(null, 0, 0, 0), "Outside left click is consumed");
		DCO_TestExpect(!m_bOpen && !m_bEditing && !m_aSessionAttributes, "Outside click closes without a manager end event");
		DCO_TestExpect(!m_bTriggerSession && !m_bConditionalRefreshQueued && !m_bCategoryRefreshQueued, "Outside click releases modal and queued state");
		m_bOpen = true;
		m_bEditing = true;
		m_aSessionAttributes = {};
		m_bCategoryRefreshQueued = true;
		m_bConditionalRefreshQueued = true;
		queue.Tick(0.2);
		DCO_TestExpect(m_bCategoryRefreshQueued && m_bConditionalRefreshQueued, "Outside-close callbacks cannot touch a reopened session");
		confirmed.Invoke(m_aSessionAttributes);
		DCO_TestExpect(backdrop.OnClick(null, 0, 0, 0) && !m_bOpen, "Repeated outside close remains safe");
		ref DCO_ScenarioBackdropHandler detached = new DCO_ScenarioBackdropHandler(null);
		DCO_TestExpect(detached.OnClick(null, 0, 0, 0), "Detached backdrop still consumes the click");
		confirmed.Remove(OnAttributesEnded);
		cancelled.Remove(OnAttributesEnded);
		OnAttributeChanged();
		Shutdown();
		queue.Tick(0.2);
		DCO_TestExpect(queue.GetRemainingTime(RefreshConditionalRows) == notQueued, "Shutdown cancels pending conditional refresh");
	}
}

modded class BIA_StageCore
{
	bool DCO_TestStudioLights()
	{
		if (!EnsureWorld("BifrostSessionTest")) return false;
		bool valid = m_StudioKey && m_StudioFillLeft && m_StudioFillRight;
		if (valid) valid = m_StudioKey.GetWorld() == m_World && m_StudioFillLeft.GetWorld() == m_World && m_StudioFillRight.GetWorld() == m_World;
		Release();
		return valid && !m_World && !m_StudioKey && !m_StudioFillLeft && !m_StudioFillRight;
	}
}
