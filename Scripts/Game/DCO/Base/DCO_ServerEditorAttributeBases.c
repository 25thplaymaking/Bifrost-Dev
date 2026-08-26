// Dedicated-server settings: thin server-authoritative editor-attribute bases.

[BaseContainerProps()]
class DCO_ServerEditorAttribute : SCR_BaseEditorAttribute
{
	override bool IsServer()
	{
		return true;
	}
}

[BaseContainerProps()]
class DCO_ServerValueListEditorAttribute : SCR_BaseValueListEditorAttribute
{
	override bool IsServer()
	{
		return true;
	}
}

[BaseContainerProps()]
class DCO_ServerFloatHolderEditorAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override bool IsServer()
	{
		return true;
	}
}
