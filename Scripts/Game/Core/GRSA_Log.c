class GRSA_Log
{
	protected static const string PREFIX = "[GRSA] ";

	//------------------------------------------------------------------------------------------------
	static void Info(string message)
	{
		Print(PREFIX + message, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	static void Warn(string message)
	{
		Print(PREFIX + message, LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	static void Error(string message)
	{
		Print(PREFIX + message, LogLevel.ERROR);
	}
}
