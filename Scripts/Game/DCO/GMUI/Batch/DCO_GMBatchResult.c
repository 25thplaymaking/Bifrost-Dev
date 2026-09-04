class DCO_GMBatchActionResult
{
	protected string m_sActionName;
	protected int m_iAttempted;
	protected int m_iSucceeded;
	protected int m_iSkipped;
	protected int m_iFailed;

	void DCO_GMBatchActionResult(string actionName)
	{
		m_sActionName = actionName;
	}

	void RecordSucceeded(int count = 1)
	{
		if (count <= 0)
			return;
		m_iAttempted += count;
		m_iSucceeded += count;
	}

	void RecordSkipped(int count = 1)
	{
		if (count <= 0)
			return;
		m_iAttempted += count;
		m_iSkipped += count;
	}

	void RecordFailed(int count = 1)
	{
		if (count <= 0)
			return;
		m_iAttempted += count;
		m_iFailed += count;
	}

	int GetAttempted()
	{
		return m_iAttempted;
	}

	int GetSucceeded()
	{
		return m_iSucceeded;
	}

	int GetSkipped()
	{
		return m_iSkipped;
	}

	int GetFailed()
	{
		return m_iFailed;
	}

	bool HasFailures()
	{
		return m_iFailed > 0;
	}

	bool IsCompleteSuccess()
	{
		return m_iAttempted > 0 && m_iSucceeded == m_iAttempted;
	}

	string BuildMessage()
	{
		string actionName = m_sActionName;
		if (actionName.IsEmpty())
			actionName = "Action";

		if (m_iAttempted <= 0)
			return actionName + ": no targets.";

		if (IsCompleteSuccess())
			return string.Format("%1: %2 succeeded.", actionName, m_iSucceeded);

		return string.Format("%1: %2 succeeded, %3 skipped, %4 failed.", actionName, m_iSucceeded, m_iSkipped, m_iFailed);
	}
}
