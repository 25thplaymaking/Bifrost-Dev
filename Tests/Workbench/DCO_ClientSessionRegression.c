class DCO_ClientSessionRequest : JsonApiStruct {}
class DCO_ClientSessionResponse : JsonApiStruct
{
 int passed;
 ref array<string> failures = {};
 void DCO_ClientSessionResponse() { RegV("passed"); RegV("failures"); }
}
class DCO_ClientSessionRegression : NetApiHandler
{
 override JsonApiStruct GetRequest() { return new DCO_ClientSessionRequest(); }
 override JsonApiStruct GetResponse(JsonApiStruct request)
 {
  DCO_ClientSessionProbe probe = new DCO_ClientSessionProbe();
  probe.Run();
  DCO_ClientSessionResponse result = new DCO_ClientSessionResponse();
  result.passed = probe.passed;
  result.failures = probe.failures;
  return result;
 }
}
