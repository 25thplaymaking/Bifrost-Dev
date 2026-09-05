class DCO_FieldResourceValidationRequest : JsonApiStruct {}
class DCO_FieldResourceValidationResponse : JsonApiStruct
{
	int requested;
	void DCO_FieldResourceValidationResponse() { RegV("requested"); }
}
class DCO_FieldResourceValidation : NetApiHandler
{
	override JsonApiStruct GetRequest() { return new DCO_FieldResourceValidationRequest(); }
	override JsonApiStruct GetResponse(JsonApiStruct request)
	{
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		array<string> paths = {
			"$BifrostDev:Sounds/Bifrost/crowd.wav", "$BifrostDev:Sounds/Bifrost/talking.wav",
			"$BifrostDev:Sounds/Bifrost/barking.wav", "$BifrostDev:Sounds/Bifrost/shouting.wav",
			"$BifrostDev:Sounds/Bifrost/battle.wav", "$BifrostDev:Sounds/Bifrost/rifle.wav",
			"$BifrostDev:Sounds/Bifrost/Controls.sig", "$BifrostDev:Sounds/Bifrost/Ambience.acp",
			"$BifrostDev:UI/layouts/DCO_GMHint.layout", "$BifrostDev:Prefabs/E_DCO_Teleporter.et",
			"$BifrostDev:Prefabs/E_DCO_FxAudio.et", "$BifrostDev:Configs/Editor/AttributeLists/DCO_Attributes.conf"
		};
		manager.RebuildResourceFiles(paths, "PC");
		DCO_FieldResourceValidationResponse response = new DCO_FieldResourceValidationResponse();
		response.requested = paths.Count();
		return response;
	}
}
