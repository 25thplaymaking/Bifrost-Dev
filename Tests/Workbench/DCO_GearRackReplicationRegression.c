class DCO_GearRackReplicationRequest : JsonApiStruct {}
class DCO_GearRackReplicationResponse : JsonApiStruct
{
 int passed;
 ref array<string> failures = {};
 ref array<string> details = {};
 void DCO_GearRackReplicationResponse() { RegV("passed"); RegV("failures"); RegV("details"); }
 void Expect(bool value, string label)
 {
  if (value) passed++;
  else failures.Insert(label);
 }
}
class DCO_GearRackReplicationRegression : NetApiHandler
{
 override JsonApiStruct GetRequest() { return new DCO_GearRackReplicationRequest(); }
 override JsonApiStruct GetResponse(JsonApiStruct request)
 {
  DCO_GearRackReplicationResponse result = new DCO_GearRackReplicationResponse();
  array<ResourceName> prefabs = {"{D67E9B88BBDB4FE4}Prefabs/E_DCO_PlaceableArsenal.et", "{762A1A6379E24BD4}Prefabs/E_DCO_XLGearCross.et"};
  for (int rackIndex = 0; rackIndex < prefabs.Count(); rackIndex++)
  {
   ResourceName prefab = prefabs[rackIndex];
   Resource resource = Resource.Load(prefab);
   result.Expect(resource && resource.IsValid(), "Rack resource loads: " + prefab);
   if (!resource || !resource.IsValid()) continue;
   IEntitySource source = resource.GetResource().ToEntitySource();
   result.Expect(source != null, "Rack has native entity source: " + prefab);
   if (!source) continue;
   IEntityComponentSource storage;
   IEntityComponentSource rack;
   IEntityComponentSource replication;
   for (int i = 0; i < source.GetComponentCount(); i++)
   {
    IEntityComponentSource component = source.GetComponent(i);
    string className = component.GetClassName();
    if (className == "DCO_GearRackStorageComponent") storage = component;
    if (className == "DCO_GearRackComponent") rack = component;
    if (className == "RplComponent") replication = component;
   }
   result.Expect(storage != null, "Rack retains native gear storage: " + prefab);
   result.Expect(rack != null, "Rack retains Hang/Take component: " + prefab);
   result.Expect(replication != null, "Rack retains replication component: " + prefab);
   if (storage)
    {
     bool virtualInventory;
     result.Expect(storage.Get("UseVirtualInventoryReplication", virtualInventory), "Native virtual-inventory property resolves: " + prefab);
     result.Expect(!virtualInventory, "Rack keeps inventory entities available to remote clients: " + prefab);
     result.details.Insert(prefab + " UseVirtualInventoryReplication=" + virtualInventory);
    }
   if (rack)
   {
    bool xl;
    result.Expect(rack.Get("m_bXL", xl) && xl == (rackIndex == 1), "Small and XL retain their respective slot support: " + prefab);
   }
   if (replication)
   {
    bool enabled;
    result.Expect(replication.Get("Enabled", enabled) && enabled, "Rack replication remains enabled: " + prefab);
   }
  }
  Resource control = Resource.Load("{42894F191BC9EDE1}Prefabs/Props/Core/AmmoBox_Base.et");
  result.Expect(control && control.IsValid(), "Native inventory control prefab loads");
  if (control && control.IsValid())
  {
   IEntitySource controlSource = control.GetResource().ToEntitySource();
   bool foundControl;
   if (controlSource)
   {
    for (int c = 0; c < controlSource.GetComponentCount(); c++)
    {
     IEntityComponentSource controlComponent = controlSource.GetComponent(c);
     if (controlComponent.GetClassName() != "SCR_UniversalInventoryStorageComponent") continue;
     bool controlVirtual;
     foundControl = controlComponent.Get("UseVirtualInventoryReplication", controlVirtual) && controlVirtual;
    }
   }
   result.Expect(foundControl, "Native ammunition-box virtual inventory remains unchanged");
  }
  return result;
 }
}
