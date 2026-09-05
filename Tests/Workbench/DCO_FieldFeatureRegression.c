class DCO_FieldFeatureRegressionRequest : JsonApiStruct {}
class DCO_FieldFeatureRegressionResponse : JsonApiStruct
{
	int passed;
	ref array<string> failures = {};
	void DCO_FieldFeatureRegressionResponse() { RegV("passed"); RegV("failures"); }
}
class DCO_FieldFeatureRegression : NetApiHandler
{
	override JsonApiStruct GetRequest() { return new DCO_FieldFeatureRegressionRequest(); }
	override JsonApiStruct GetResponse(JsonApiStruct request)
	{
		DCO_FieldFeatureProbe probe = new DCO_FieldFeatureProbe();
		probe.Run();
		DCO_FieldFeatureRegressionResponse result = new DCO_FieldFeatureRegressionResponse();
		result.passed = probe.m_Passed;
		foreach (string failure : probe.m_Failures) result.failures.Insert(failure);
		return result;
	}
}
