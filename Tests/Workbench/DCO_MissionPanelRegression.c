class DCO_MissionPanelRegressionRequest : JsonApiStruct {}

class DCO_MissionPanelRegressionResponse : JsonApiStruct
{
	int passed;
	ref array<string> failures = {};
	void DCO_MissionPanelRegressionResponse()
	{
		RegV("passed");
		RegV("failures");
	}
}

class DCO_MissionPanelRegression : NetApiHandler
{
	override JsonApiStruct GetRequest() { return new DCO_MissionPanelRegressionRequest(); }
	override JsonApiStruct GetResponse(JsonApiStruct request)
	{
		DCO_MissionPanelRegressionProbe probe = new DCO_MissionPanelRegressionProbe();
		probe.Run();
		DCO_MissionPanelRegressionResponse response = new DCO_MissionPanelRegressionResponse();
		response.passed = probe.m_Passed;
		foreach (string failure : probe.m_Failures) response.failures.Insert(failure);
		return response;
	}
}
