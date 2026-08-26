[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
modded class SCR_DateEditorAttribute
{
	protected static const int DCO_FIRST_VANILLA_YEAR = 1940;
	protected static const int DCO_LAST_YEAR = 9999;

	// Preserve engine's 1940-based indices so existing Bifrost presets remain valid, then append the earlier Gregorian years.
	protected void DCO_EnsureFullYearRange()
	{
		if (m_aYearArray.Count() == DCO_LAST_YEAR)
			return;

		m_aYearArray.Clear();
		for (int year = DCO_FIRST_VANILLA_YEAR; year <= DCO_LAST_YEAR; year++)
			m_aYearArray.Insert(year);
		for (int earlyYear = 1; earlyYear < DCO_FIRST_VANILLA_YEAR; earlyYear++)
			m_aYearArray.Insert(earlyYear);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_EnsureFullYearRange();
		super.WriteVariable(item, var, manager, playerID);
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		DCO_EnsureFullYearRange();
		return super.GetEntries(outEntries);
	}

	int DCO_GetYearIndex(int year)
	{
		DCO_EnsureFullYearRange();
		return m_aYearArray.Find(year);
	}

	int DCO_GetYearAtIndex(int index)
	{
		DCO_EnsureFullYearRange();
		if (!m_aYearArray.IsIndexValid(index))
			return DCO_FIRST_VANILLA_YEAR;
		return m_aYearArray[index];
	}
}
