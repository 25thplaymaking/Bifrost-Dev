class BIA_ArsenalRackRequest : JsonApiStruct { bool poll; void BIA_ArsenalRackRequest() { RegV("poll"); } }
class BIA_ArsenalRackResponse : JsonApiStruct
{
	int passed;
	bool running;
	ref array<string> failures = {};
	ref array<string> details = {};
	void BIA_ArsenalRackResponse() { RegV("running"); RegV("passed"); RegV("failures"); RegV("details"); }
}
class BIA_ArsenalRackRegression : NetApiHandler
{
	protected static ref BIA_ArsenalRackProbe s_Probe;
	override JsonApiStruct GetRequest() { return new BIA_ArsenalRackRequest(); }
	override JsonApiStruct GetResponse(JsonApiStruct request)
	{
		BIA_ArsenalRackRequest req = BIA_ArsenalRackRequest.Cast(request);
		if (!s_Probe || (!req.poll && !s_Probe.running))
		{
			s_Probe = new BIA_ArsenalRackProbe();
			s_Probe.Run();
		}
		BIA_ArsenalRackProbe probe = s_Probe;
		BIA_ArsenalRackResponse response = new BIA_ArsenalRackResponse();
		response.running = probe.running;
		response.passed = probe.passed;
		response.failures.Copy(probe.failures);
		response.details.Copy(probe.details);
		return response;
	}
}
