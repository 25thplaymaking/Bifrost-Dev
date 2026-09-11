class DCO_RplFixRegressionRequest : JsonApiStruct
{
	bool start;
	void DCO_RplFixRegressionRequest() { RegV("start"); }
}
class DCO_RplFixRegressionResponse : JsonApiStruct
{
	int passed;
	bool done;
	ref array<string> failures = {};
	ref array<string> measurements = {};
	void DCO_RplFixRegressionResponse() { RegV("passed"); RegV("done"); RegV("failures"); RegV("measurements"); }
}
class DCO_RplFixRegression : NetApiHandler
{
	override JsonApiStruct GetRequest() { return new DCO_RplFixRegressionRequest(); }
	override JsonApiStruct GetResponse(JsonApiStruct request)
	{
		DCO_RplFixRegressionRequest args = DCO_RplFixRegressionRequest.Cast(request);
		if (args.start && (!DCO_RplFixProbe.s_Probe || DCO_RplFixProbe.s_Probe.done))
		{
			DCO_RplFixProbe.s_Probe = new DCO_RplFixProbe();
			DCO_RplFixProbe.s_Probe.Start();
		}
		DCO_RplFixRegressionResponse result = new DCO_RplFixRegressionResponse();
		DCO_RplFixProbe probe = DCO_RplFixProbe.s_Probe;
		if (!probe) { result.failures.Insert("Probe not started"); return result; }
		result.passed = probe.passed; result.done = probe.done;
		foreach (string failure : probe.failures) result.failures.Insert(failure);
		foreach (string measurement : probe.measurements) result.measurements.Insert(measurement);
		return result;
	}
}