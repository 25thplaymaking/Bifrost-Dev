class DCO_MenuLayoutRequest : JsonApiStruct {}
class DCO_MenuLayoutResponse : JsonApiStruct
{
 int passed;
 ref array<string> failures = {};
 void DCO_MenuLayoutResponse() { RegV("passed"); RegV("failures"); }
}
class DCO_MenuLayoutRegression : NetApiHandler
{
 override JsonApiStruct GetRequest() { return new DCO_MenuLayoutRequest(); }
 override JsonApiStruct GetResponse(JsonApiStruct request)
 {
  DCO_MenuLayoutProbe probe = new DCO_MenuLayoutProbe();
  probe.Run();
  DCO_MenuLayoutResponse response = new DCO_MenuLayoutResponse();
  response.passed = probe.passed;
  response.failures = probe.failures;
  return response;
 }
}
