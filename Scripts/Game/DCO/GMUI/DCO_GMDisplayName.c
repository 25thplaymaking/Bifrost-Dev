class DCO_GMDisplayName
{
	static string Resolve(LocalizedString authoredName, ResourceName resourceName, string fallback = "Item")
	{
		string rawName = authoredName;
		if (!rawName.IsEmpty())
		{
			if (rawName[0] != "#")
				return rawName;

			string translatedName = WidgetManager.Translate(rawName);
			if (IsResolved(translatedName, rawName))
				return translatedName;
		}

		// Foreign content can retain a stale localization key, so use its stable prefab identity.
		string resourceLabel = SCR_StringHelper.FormatResourceNameToUserFriendly(resourceName);
		if (!resourceLabel.IsEmpty())
			return resourceLabel;

		return fallback;
	}

	protected static bool IsResolved(string translatedName, string authoredName)
	{
		if (translatedName.IsEmpty() || translatedName[0] == "#")
			return false;

		if (authoredName.Length() > 1 && translatedName == authoredName.Substring(1, authoredName.Length() - 1))
			return false;

		return true;
	}
}
