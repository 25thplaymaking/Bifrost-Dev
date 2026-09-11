class DCO_VestMountRequest : JsonApiStruct {}
class DCO_VestMountResponse : JsonApiStruct
{
	int passed;
	ref array<string> failures = {};
	ref array<string> details = {};
	void DCO_VestMountResponse() { RegV("passed"); RegV("failures"); RegV("details"); }
}
class DCO_VestMountRegression : NetApiHandler
{
	override JsonApiStruct GetRequest() { return new DCO_VestMountRequest(); }
	override JsonApiStruct GetResponse(JsonApiStruct request)
	{
		DCO_VestMountProbe probe = new DCO_VestMountProbe();
		probe.Run();
		DCO_VestMountResponse result = new DCO_VestMountResponse();
		result.passed = probe.passed;
		result.failures.Copy(probe.failures);
		result.details.Copy(probe.details);
		return result;
	}
}
