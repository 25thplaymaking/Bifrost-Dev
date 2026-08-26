class DCO_QRFRangeVisual
{
	protected static ref Shape s_Shape;

	protected static const int		DCO_QRF_CIRCLE_COLOR	= 0xFF3FA9F5;	// ARGB - bright blue.

	static void Show(vector origin, float radius)
	{
		if (!DCO_GMRights.IsLocalGameMaster())
		{
			Hide();
			return;
		}

		if (radius < 1.0)
		{
			Hide();
			return;
		}

		s_Shape = DCO_ZoneShape.FlatCircle(origin, radius, DCO_QRF_CIRCLE_COLOR);
	}

	static void Hide()
	{
		s_Shape = null;
	}
}
