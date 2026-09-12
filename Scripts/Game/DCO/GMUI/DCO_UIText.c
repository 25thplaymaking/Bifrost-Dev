class DCO_UIText
{
	static string Plain(string text)
	{
		// Localized strings can introduce markup, so expand them before cleaning.
		text = WidgetManager.Translate(text);
		string result;
		for (int i = 0; i < text.Length(); i++)
		{
			if (text[i] == "<")
			{
				int end = i + 1;
				while (end < text.Length() && text[end] != ">")
					end++;
				if (end < text.Length())
				{
					string tag = text.Substring(i + 1, end - i - 1);
					tag.ToLower();
					if (tag == "br" || tag == "br/")
					{
						result += "\n";
						i = end;
						continue;
					}
					if (tag == "color" || tag.StartsWith("color ") || tag == "/color"
						|| tag == "b" || tag == "/b" || tag == "i" || tag == "/i"
						|| tag == "u" || tag == "/u" || tag.StartsWith("image "))
					{
						i = end;
						continue;
					}
				}
			}
			result += text[i];
		}
		return result;
	}
}
