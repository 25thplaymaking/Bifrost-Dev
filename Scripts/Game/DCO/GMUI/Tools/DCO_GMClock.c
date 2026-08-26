// DCO GM clock-speed core.
class DCO_GMClock
{
	static const float STOPPED_DAY_SECONDS = 8640000;
	static const float MIN_MULT = 86400 / STOPPED_DAY_SECONDS;	// the multiplier floor that maps to "stopped".

	protected static TimeAndWeatherManagerEntity Mgr()
	{
		ChimeraWorld cw = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!cw)
			return null;
		return cw.GetTimeAndWeatherManager();
	}

	// mult = realtime speed of the day-night clock: 0 = stopped, 1 = real time, 60 = 60x.
	static void SetMultiplier(float mult)
	{
		TimeAndWeatherManagerEntity m = Mgr();
		if (!m)
		{
			Print("[DCO-GM] clock: no time manager", LogLevel.WARNING);
			return;
		}
		if (mult <= MIN_MULT)
			m.SetDayDuration(STOPPED_DAY_SECONDS);
		else
			m.SetDayDuration(86400 / mult);
		Print(string.Format("[DCO-GM] clock multiplier set to %1x", mult), LogLevel.NORMAL);
	}

	static float GetMultiplier()
	{
		TimeAndWeatherManagerEntity m = Mgr();
		if (!m)
			return 0;
		float d = m.GetDayDuration();
		if (d <= 0)
			return 0;
		return 86400 / d;
	}
}
