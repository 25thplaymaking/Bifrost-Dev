class BIA_StandPlacementRequest : JsonApiStruct {}
class BIA_StandPlacementResponse : JsonApiStruct
{
 int passed;
 ref array<string> failures = {};
 ref array<string> details = {};
 void BIA_StandPlacementResponse() { RegV("passed"); RegV("failures"); RegV("details"); }
}
class BIA_StandPlacementRegression : NetApiHandler
{
 override JsonApiStruct GetRequest() { return new BIA_StandPlacementRequest(); }
 override JsonApiStruct GetResponse(JsonApiStruct request)
 {
  BIA_StandPlacementProbe probe = new BIA_StandPlacementProbe();
  probe.Run();
  BIA_StandPlacementResponse result = new BIA_StandPlacementResponse();
  result.passed = probe.passed;
  result.failures.Copy(probe.failures);
  result.details.Copy(probe.details);
  return result;
 }
}
