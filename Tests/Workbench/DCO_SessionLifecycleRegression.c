class DCO_SessionLifecycleRequest : JsonApiStruct {}
class DCO_SessionLifecycleResponse : JsonApiStruct
{
	int passed;
	bool studioLights;
	bool audioGraph;
	int audioEvents;
	ref array<string> failures = {};
	void DCO_SessionLifecycleResponse() { RegV("passed"); RegV("studioLights"); RegV("audioGraph"); RegV("failures"); }
}
class DCO_SessionLifecycleRegression : NetApiHandler
{
	override JsonApiStruct GetRequest() { return new DCO_SessionLifecycleRequest(); }
	override JsonApiStruct GetResponse(JsonApiStruct request)
	{
		DCO_GMScenarioPanel probe = new DCO_GMScenarioPanel();
		probe.DCO_RunSessionLifecycleProbe();
		DCO_SessionLifecycleResponse response = new DCO_SessionLifecycleResponse();
		response.passed = probe.DCO_TestPassed;
		foreach (string failure : probe.DCO_TestFailures) response.failures.Insert(failure);
		BIA_StageCore stage = new BIA_StageCore();
		response.studioLights = stage.DCO_TestStudioLights();
		response.audioGraph = AudioSystem.PlayEventInitialize("{DCA6090590000001}Sounds/Bifrost/Ambience.acp");
		return response;
	}
}
