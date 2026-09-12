class BIA_SoldierBrowserRequest : JsonApiStruct {}
class BIA_SoldierBrowserResponse : JsonApiStruct
{
 int passed;
 ref array<string> failures = {};
 void BIA_SoldierBrowserResponse() { RegV("passed"); RegV("failures"); }
}
class BIA_SoldierBrowserRegression : NetApiHandler
{
 override JsonApiStruct GetRequest() { return new BIA_SoldierBrowserRequest(); }
 override JsonApiStruct GetResponse(JsonApiStruct request)
 {
  BIA_SoldierBrowserProbe probe = new BIA_SoldierBrowserProbe();
  probe.Run();
  BIA_SoldierBrowserResponse response = new BIA_SoldierBrowserResponse();
  response.passed = probe.passed;
  response.failures = probe.failures;
  return response;
 }
}
