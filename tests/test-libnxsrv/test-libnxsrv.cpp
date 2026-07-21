#include <nms_common.h>
#include <nms_util.h>
#include <math.h>
#include <nxsrvapi.h>
#include <nddrv.h>
#include <entity_mib.h>
#include <xml_to_json.h>
#include <testtools.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(test-libnxsrv)

/**
 * NObject subclass exposing protected members for testing
 */
class TestObject : public NObject
{
public:
   TestObject(uint32_t id, const TCHAR *name) : NObject()
   {
      m_id = id;
      _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   }

   void linkChild(const shared_ptr<NObject>& object) { addChildReference(object); }
   void linkParent(const shared_ptr<NObject>& object) { addParentReference(object); }
   void unlinkChild(uint32_t objectId) { deleteChildReference(objectId); }
   void unlinkParent(uint32_t objectId) { deleteParentReference(objectId); }
};

/**
 * Create bi-directional parent-child link between objects
 */
static void LinkObjects(const shared_ptr<TestObject>& parent, const shared_ptr<TestObject>& child)
{
   parent->linkChild(child);
   child->linkParent(parent);
}

/**
 * Remove bi-directional parent-child link between objects
 */
static void UnlinkObjects(const shared_ptr<TestObject>& parent, const shared_ptr<TestObject>& child)
{
   parent->unlinkChild(child->getId());
   child->unlinkParent(parent->getId());
}

/**
 * Get custom attribute flags (returns 0xFFFFFFFF if attribute does not exist)
 */
static uint32_t GetCustomAttributeFlags(const shared_ptr<TestObject>& object, const TCHAR *name)
{
   uint32_t flags = 0xFFFFFFFF;
   object->forEachCustomAttribute(
      [name, &flags] (const TCHAR *n, const CustomAttribute *a) -> EnumerationCallbackResult
      {
         if (!_tcscmp(n, name))
         {
            flags = a->flags;
            return _STOP;
         }
         return _CONTINUE;
      });
   return flags;
}

/**
 * Test NObject parent/child hierarchy
 */
static void TestNObjectHierarchy()
{
   StartTest(_T("NObject - direct parent/child references"));
   auto o1 = make_shared<TestObject>(1, _T("o1"));
   auto o2 = make_shared<TestObject>(2, _T("o2"));
   auto o3 = make_shared<TestObject>(3, _T("o3"));
   LinkObjects(o1, o2);
   LinkObjects(o2, o3);
   AssertEquals(o1->getChildCount(), 1);
   AssertEquals(o2->getChildCount(), 1);
   AssertEquals(o2->getParentCount(), 1);
   AssertEquals(o3->getParentCount(), 1);
   AssertTrue(o1->isDirectChild(2));
   AssertFalse(o1->isDirectChild(3));
   AssertTrue(o3->isDirectParent(2));
   AssertFalse(o3->isDirectParent(1));
   EndTest();

   StartTest(_T("NObject - transitive isChild/isParent"));
   AssertTrue(o1->isChild(2));
   AssertTrue(o1->isChild(3));
   AssertFalse(o1->isChild(4));
   AssertTrue(o3->isParent(2));
   AssertTrue(o3->isParent(1));
   AssertFalse(o3->isParent(4));
   AssertFalse(o1->isParent(3));
   EndTest();

   StartTest(_T("NObject - duplicate reference ignored"));
   o1->linkChild(o2);
   AssertEquals(o1->getChildCount(), 1);
   EndTest();

   StartTest(_T("NObject - self reference ignored"));
   o1->linkChild(o1);
   o1->linkParent(o1);
   AssertEquals(o1->getChildCount(), 1);
   AssertEquals(o1->getParentCount(), 0);
   EndTest();

   StartTest(_T("NObject - reference removal"));
   UnlinkObjects(o2, o3);
   AssertFalse(o2->isDirectChild(3));
   AssertFalse(o1->isChild(3));
   AssertEquals(o3->getParentCount(), 0);
   UnlinkObjects(o1, o2);
   AssertEquals(o1->getChildCount(), 0);
   AssertEquals(o2->getParentCount(), 0);
   EndTest();
}

/**
 * Test NObject custom attributes - basic operations
 */
static void TestNObjectCustomAttributes()
{
   auto object = make_shared<TestObject>(100, _T("test"));

   StartTest(_T("NObject - custom attribute set/get"));
   object->setCustomAttribute(_T("name1"), _T("value1"));
   AssertEquals(object->getCustomAttribute(_T("name1")).cstr(), _T("value1"));
   AssertTrue(object->getCustomAttribute(_T("nonexistent")).isNull());
   AssertEquals(object->getCustomAttributeSize(), 1);

   TCHAR buffer[64];
   AssertNotNull(object->getCustomAttribute(_T("name1"), buffer, 64));
   AssertEquals(buffer, _T("value1"));
   AssertNull(object->getCustomAttribute(_T("nonexistent"), buffer, 64));

   TCHAR *copy = object->getCustomAttributeCopy(_T("name1"));
   AssertNotNull(copy);
   AssertEquals(copy, _T("value1"));
   MemFree(copy);
   AssertNull(object->getCustomAttributeCopy(_T("nonexistent")));

   object->setCustomAttribute(_T("name1"), _T("value2"));
   AssertEquals(object->getCustomAttribute(_T("name1")).cstr(), _T("value2"));
   AssertEquals(object->getCustomAttributeSize(), 1);
   EndTest();

   StartTest(_T("NObject - typed custom attribute getters"));
   object->setCustomAttribute(_T("int32"), static_cast<int32_t>(-42));
   AssertEquals(object->getCustomAttributeAsInt32(_T("int32"), 0), -42);
   AssertEquals(object->getCustomAttributeAsInt32(_T("nonexistent"), 7), 7);

   object->setCustomAttribute(_T("uint32"), static_cast<uint32_t>(4000000000U));
   AssertEquals(object->getCustomAttributeAsUInt32(_T("uint32"), 0), 4000000000U);

   object->setCustomAttribute(_T("int64"), static_cast<int64_t>(-9000000000LL));
   AssertEquals(object->getCustomAttributeAsInt64(_T("int64"), 0), static_cast<int64_t>(-9000000000LL));

   object->setCustomAttribute(_T("uint64"), static_cast<uint64_t>(18000000000000000000ULL));
   AssertEquals(object->getCustomAttributeAsUInt64(_T("uint64"), 0), static_cast<uint64_t>(18000000000000000000ULL));
   AssertEquals(object->getCustomAttributeAsUInt64(_T("nonexistent"), 99), static_cast<uint64_t>(99));

   object->setCustomAttribute(_T("double"), _T("2.5"));
   AssertTrue(object->getCustomAttributeAsDouble(_T("double"), 0) == 2.5);
   AssertTrue(object->getCustomAttributeAsDouble(_T("nonexistent"), 1.5) == 1.5);

   object->setCustomAttribute(_T("boolTrue"), _T("true"));
   object->setCustomAttribute(_T("boolFalse"), _T("FALSE"));
   object->setCustomAttribute(_T("boolNumeric"), _T("1"));
   object->setCustomAttribute(_T("boolZero"), _T("0"));
   AssertTrue(object->getCustomAttributeAsBoolean(_T("boolTrue"), false));
   AssertFalse(object->getCustomAttributeAsBoolean(_T("boolFalse"), true));
   AssertTrue(object->getCustomAttributeAsBoolean(_T("boolNumeric"), false));
   AssertFalse(object->getCustomAttributeAsBoolean(_T("boolZero"), true));
   AssertTrue(object->getCustomAttributeAsBoolean(_T("nonexistent"), true));
   EndTest();

   StartTest(_T("NObject - custom attribute deletion"));
   int sizeBefore = object->getCustomAttributeSize();
   object->deleteCustomAttribute(_T("int32"));
   AssertTrue(object->getCustomAttribute(_T("int32")).isNull());
   AssertEquals(object->getCustomAttributeSize(), sizeBefore - 1);
   EndTest();

   StartTest(_T("NObject - custom attribute regexp filtering"));
   auto filtered = make_shared<TestObject>(101, _T("filtered"));
   filtered->setCustomAttribute(_T("net.ip"), _T("10.0.0.1"));
   filtered->setCustomAttribute(_T("net.mask"), _T("255.255.255.0"));
   filtered->setCustomAttribute(_T("sys.descr"), _T("test system"));

   StringMap *attributes = filtered->getCustomAttributes(_T("^net\\..*"));
   AssertEquals(attributes->size(), 2);
   AssertEquals(attributes->get(_T("net.ip")), _T("10.0.0.1"));
   AssertEquals(attributes->get(_T("net.mask")), _T("255.255.255.0"));
   AssertNull(attributes->get(_T("sys.descr")));
   delete attributes;

   attributes = filtered->getCustomAttributes(static_cast<const TCHAR*>(nullptr));
   AssertEquals(attributes->size(), 3);
   delete attributes;
   EndTest();
}

/**
 * Test NObject custom attribute inheritance
 */
static void TestNObjectCustomAttributeInheritance()
{
   StartTest(_T("NObject - custom attribute propagation to children"));
   auto parent = make_shared<TestObject>(10, _T("parent"));
   auto child = make_shared<TestObject>(11, _T("child"));
   LinkObjects(parent, child);

   parent->setCustomAttribute(_T("site"), SharedString(_T("HQ")), StateChange::SET);
   AssertEquals(child->getCustomAttribute(_T("site")).cstr(), _T("HQ"));
   AssertEquals(child->getInheritableCustomAttributeParent(_T("site")), 10u);

   parent->setCustomAttribute(_T("local"), _T("value"));   // not inheritable
   AssertTrue(child->getCustomAttribute(_T("local")).isNull());
   EndTest();

   StartTest(_T("NObject - custom attribute inheritance on new child"));
   auto lateChild = make_shared<TestObject>(12, _T("lateChild"));
   LinkObjects(parent, lateChild);
   AssertEquals(lateChild->getCustomAttribute(_T("site")).cstr(), _T("HQ"));
   AssertTrue(lateChild->getCustomAttribute(_T("local")).isNull());
   EndTest();

   StartTest(_T("NObject - custom attribute propagation to grandchildren"));
   auto grandchild = make_shared<TestObject>(13, _T("grandchild"));
   LinkObjects(child, grandchild);
   AssertEquals(grandchild->getCustomAttribute(_T("site")).cstr(), _T("HQ"));
   parent->setCustomAttribute(_T("site"), SharedString(_T("DC-1")), StateChange::SET);
   AssertEquals(child->getCustomAttribute(_T("site")).cstr(), _T("DC-1"));
   AssertEquals(grandchild->getCustomAttribute(_T("site")).cstr(), _T("DC-1"));
   EndTest();

   StartTest(_T("NObject - inherited custom attribute redefinition"));
   child->setCustomAttribute(_T("site"), SharedString(_T("Branch")), StateChange::IGNORE);
   AssertEquals(child->getCustomAttribute(_T("site")).cstr(), _T("Branch"));
   AssertEquals(parent->getCustomAttribute(_T("site")).cstr(), _T("DC-1"));
   AssertEquals(grandchild->getCustomAttribute(_T("site")).cstr(), _T("Branch"));
   AssertEquals(GetCustomAttributeFlags(child, _T("site")), static_cast<uint32_t>(CAF_INHERITABLE | CAF_REDEFINED));
   EndTest();

   StartTest(_T("NObject - redefined custom attribute deletion restores inherited value"));
   child->deleteCustomAttribute(_T("site"));
   AssertEquals(child->getCustomAttribute(_T("site")).cstr(), _T("DC-1"));
   AssertEquals(GetCustomAttributeFlags(child, _T("site")), static_cast<uint32_t>(CAF_INHERITABLE));
   AssertEquals(grandchild->getCustomAttribute(_T("site")).cstr(), _T("DC-1"));
   EndTest();

   StartTest(_T("NObject - inherited custom attribute removal on parent detach"));
   UnlinkObjects(parent, lateChild);
   AssertTrue(lateChild->getCustomAttribute(_T("site")).isNull());
   AssertEquals(lateChild->getCustomAttributeSize(), 0);
   EndTest();

   StartTest(_T("NObject - inherited custom attribute removal on parent delete"));
   parent->deleteCustomAttribute(_T("site"));
   AssertTrue(child->getCustomAttribute(_T("site")).isNull());
   AssertTrue(grandchild->getCustomAttribute(_T("site")).isNull());
   EndTest();
}

/**
 * Test NObject custom attribute conflicts between multiple parents
 */
static void TestNObjectCustomAttributeConflicts()
{
   StartTest(_T("NObject - custom attribute conflict detection"));
   auto parent1 = make_shared<TestObject>(20, _T("parent1"));
   auto parent2 = make_shared<TestObject>(21, _T("parent2"));
   auto child = make_shared<TestObject>(22, _T("child"));

   parent1->setCustomAttribute(_T("zone"), SharedString(_T("A")), StateChange::SET);
   parent2->setCustomAttribute(_T("zone"), SharedString(_T("B")), StateChange::SET);

   LinkObjects(parent1, child);
   AssertEquals(GetCustomAttributeFlags(child, _T("zone")), static_cast<uint32_t>(CAF_INHERITABLE));

   LinkObjects(parent2, child);
   AssertTrue((GetCustomAttributeFlags(child, _T("zone")) & CAF_CONFLICT) != 0);
   EndTest();

   StartTest(_T("NObject - custom attribute conflict resolution on parent detach"));
   UnlinkObjects(parent2, child);
   AssertEquals(GetCustomAttributeFlags(child, _T("zone")), static_cast<uint32_t>(CAF_INHERITABLE));
   AssertEquals(child->getCustomAttribute(_T("zone")).cstr(), _T("A"));
   EndTest();
}

/**
 * Test NObject custom attribute update from NXCP message
 */
static void TestNObjectCustomAttributesFromMessage()
{
   StartTest(_T("NObject - custom attributes from NXCP message"));
   auto object = make_shared<TestObject>(30, _T("object"));
   auto child = make_shared<TestObject>(31, _T("child"));
   LinkObjects(object, child);

   NXCPMessage msg(CMD_MODIFY_OBJECT, 0);
   msg.setField(VID_NUM_CUSTOM_ATTRIBUTES, static_cast<uint32_t>(3));
   uint32_t fieldId = VID_CUSTOM_ATTRIBUTES_BASE;
   msg.setField(fieldId++, _T("plain"));
   msg.setField(fieldId++, _T("value1"));
   msg.setField(fieldId++, static_cast<uint32_t>(0));
   msg.setField(fieldId++, _T("inheritable"));
   msg.setField(fieldId++, _T("value2"));
   msg.setField(fieldId++, static_cast<uint32_t>(CAF_INHERITABLE));
   msg.setField(fieldId++, _T("$hidden"));
   msg.setField(fieldId++, _T("value3"));
   msg.setField(fieldId++, static_cast<uint32_t>(0));
   object->setCustomAttributesFromMessage(msg);

   AssertEquals(object->getCustomAttributeSize(), 2);
   AssertEquals(object->getCustomAttribute(_T("plain")).cstr(), _T("value1"));
   AssertEquals(GetCustomAttributeFlags(object, _T("plain")), 0u);
   AssertEquals(object->getCustomAttribute(_T("inheritable")).cstr(), _T("value2"));
   AssertEquals(GetCustomAttributeFlags(object, _T("inheritable")), static_cast<uint32_t>(CAF_INHERITABLE));
   AssertTrue(object->getCustomAttribute(_T("$hidden")).isNull());
   AssertEquals(child->getCustomAttribute(_T("inheritable")).cstr(), _T("value2"));
   EndTest();

   StartTest(_T("NObject - custom attribute removal via NXCP message"));
   NXCPMessage update(CMD_MODIFY_OBJECT, 0);
   update.setField(VID_NUM_CUSTOM_ATTRIBUTES, static_cast<uint32_t>(1));
   fieldId = VID_CUSTOM_ATTRIBUTES_BASE;
   update.setField(fieldId++, _T("plain"));
   update.setField(fieldId++, _T("updated"));
   update.setField(fieldId++, static_cast<uint32_t>(0));
   object->setCustomAttributesFromMessage(update);

   AssertEquals(object->getCustomAttributeSize(), 1);
   AssertEquals(object->getCustomAttribute(_T("plain")).cstr(), _T("updated"));
   AssertTrue(object->getCustomAttribute(_T("inheritable")).isNull());
   AssertTrue(child->getCustomAttribute(_T("inheritable")).isNull());
   EndTest();
}

/**
 * Test VlanList and VlanInfo
 */
static void TestVlanList()
{
   StartTest(_T("VlanList - add and find"));
   VlanList list;
   list.add(new VlanInfo(1, VLAN_PRM_IFINDEX, _T("default")));
   list.add(new VlanInfo(100, VLAN_PRM_IFINDEX, _T("management")));
   list.add(new VlanInfo(200, VLAN_PRM_PHYLOC, _T("users")));
   AssertEquals(list.size(), 3);

   VlanInfo *vlan = list.findById(100);
   AssertNotNull(vlan);
   AssertEquals(vlan->getName(), _T("management"));
   AssertEquals(vlan->getVlanId(), 100);
   AssertEquals(vlan->getPortReferenceMode(), VLAN_PRM_IFINDEX);
   AssertNull(list.findById(999));

   vlan = list.findByName(_T("USERS"));   // case-insensitive
   AssertNotNull(vlan);
   AssertEquals(vlan->getVlanId(), 200);
   AssertNull(list.findByName(_T("nonexistent")));

   AssertNotNull(list.get(0));
   AssertEquals(list.get(0)->getVlanId(), 1);
   AssertNull(list.get(3));
   AssertNull(list.get(-1));
   EndTest();

   StartTest(_T("VlanList - member ports by port ID"));
   list.addMemberPort(100, 5);
   list.addMemberPort(100, 6, true);
   list.addMemberPort(999, 7);   // nonexistent VLAN, should be ignored

   vlan = list.findById(100);
   AssertEquals(vlan->getNumPorts(), 2);
   AssertEquals(vlan->getPort(0)->portId, 5u);
   AssertFalse(vlan->getPort(0)->tagged);
   AssertEquals(vlan->getPort(1)->portId, 6u);
   AssertTrue(vlan->getPort(1)->tagged);

   list.addMemberPort(100, 5, true);   // duplicate port, updates tagged flag only
   AssertEquals(vlan->getNumPorts(), 2);
   AssertTrue(vlan->getPort(0)->tagged);
   EndTest();

   StartTest(_T("VlanList - member ports by physical location"));
   list.addMemberPort(200, InterfacePhysicalLocation(1, 2, 0, 3));
   list.addMemberPort(200, InterfacePhysicalLocation(1, 2, 0, 4), true);

   vlan = list.findById(200);
   AssertEquals(vlan->getNumPorts(), 2);
   AssertTrue(vlan->getPort(0)->location.equals(InterfacePhysicalLocation(1, 2, 0, 3)));
   AssertFalse(vlan->getPort(0)->tagged);
   AssertTrue(vlan->getPort(1)->tagged);

   list.addMemberPort(200, InterfacePhysicalLocation(1, 2, 0, 3), true);   // duplicate location
   AssertEquals(vlan->getNumPorts(), 2);
   AssertTrue(vlan->getPort(0)->tagged);
   EndTest();

   StartTest(_T("VlanInfo - port resolution"));
   vlan = list.findById(100);
   vlan->resolvePort(0, InterfacePhysicalLocation(1, 0, 0, 5), 105, 1005);
   const VlanPortInfo *port = vlan->getPort(0);
   AssertEquals(port->ifIndex, 105u);
   AssertEquals(port->objectId, 1005u);
   AssertTrue(port->location.equals(InterfacePhysicalLocation(1, 0, 0, 5)));
   AssertNull(vlan->getPort(100));
   EndTest();

   StartTest(_T("VlanInfo - setName"));
   vlan = list.findById(1);
   vlan->setName(_T("renamed"));
   AssertEquals(vlan->getName(), _T("renamed"));
   AssertNotNull(list.findByName(_T("renamed")));
   EndTest();

   StartTest(_T("VlanList - fillMessage"));
   NXCPMessage msg(CMD_REQUEST_COMPLETED, 0);
   list.fillMessage(&msg);
   AssertEquals(msg.getFieldAsUInt32(VID_NUM_VLANS), 3u);

   uint32_t fieldId = VID_VLAN_LIST_BASE;
   AssertEquals(msg.getFieldAsUInt16(fieldId++), 1);
   TCHAR name[64];
   msg.getFieldAsString(fieldId++, name, 64);
   AssertEquals(name, _T("renamed"));
   AssertEquals(msg.getFieldAsUInt32(fieldId++), 0u);   // no ports in VLAN 1

   AssertEquals(msg.getFieldAsUInt16(fieldId++), 100);
   msg.getFieldAsString(fieldId++, name, 64);
   AssertEquals(name, _T("management"));
   AssertEquals(msg.getFieldAsUInt32(fieldId++), 2u);
   AssertEquals(msg.getFieldAsUInt32(fieldId), 105u);        // port 0 ifIndex
   AssertEquals(msg.getFieldAsUInt32(fieldId + 1), 1005u);   // port 0 object ID
   AssertEquals(msg.getFieldAsUInt32(fieldId + 2), 1u);      // port 0 chassis
   AssertEquals(msg.getFieldAsUInt32(fieldId + 5), 5u);      // port 0 port number
   EndTest();

   StartTest(_T("VlanInfo - toJson"));
   json_t *json = list.findById(100)->toJson();
   AssertNotNull(json);
   AssertEquals(json_integer_value(json_object_get(json, "id")), static_cast<int64_t>(100));
   AssertEquals(json_string_value(json_object_get(json, "name")), "management");
   json_t *ports = json_object_get(json, "ports");
   AssertNotNull(ports);
   AssertEquals(json_array_size(ports), 2);
   json_t *jsonPort = json_array_get(ports, 0);
   AssertEquals(json_integer_value(json_object_get(jsonPort, "ifIndex")), static_cast<int64_t>(105));
   AssertEquals(json_integer_value(json_object_get(jsonPort, "objectId")), static_cast<int64_t>(1005));
   json_t *location = json_object_get(jsonPort, "physicalLocation");
   AssertNotNull(location);
   AssertEquals(json_integer_value(json_object_get(location, "chassis")), static_cast<int64_t>(1));
   AssertEquals(json_integer_value(json_object_get(location, "port")), static_cast<int64_t>(5));
   json_decref(json);
   EndTest();
}

/**
 * Create component array for tree tests. Order is intentionally shuffled to test sorting by position.
 * Layout: chassis (index 1) -> module (index 2, position 1) -> ports (indexes 3, 4);
 *         chassis -> module (index 5, position 2) -> submodule (index 6) -> port (index 7)
 */
static void CreateComponents(ObjectArray<Component> *elements)
{
   elements->add(new Component(4, COMPONENT_CLASS_PORT, 2, 2, 1002, _T("port2"), _T("Port 2"), _T(""), _T(""), _T(""), _T("")));
   elements->add(new Component(1, COMPONENT_CLASS_CHASSIS, 0, 1, 0, _T("chassis"), _T("Test Chassis"), _T("CH-1000"), _T("SN12345"), _T("TestVendor"), _T("1.0.0")));
   elements->add(new Component(3, COMPONENT_CLASS_PORT, 2, 1, 1001, _T("port1"), _T("Port 1"), _T(""), _T(""), _T(""), _T("")));
   elements->add(new Component(2, COMPONENT_CLASS_MODULE, 1, 1, 0, _T("module1"), _T("Module 1"), _T("M-100"), _T("SN0001"), _T("TestVendor"), _T("2.0")));
   elements->add(new Component(5, COMPONENT_CLASS_MODULE, 1, 2, 0, _T("module2"), _T("Module 2"), _T("M-200"), _T("SN0002"), _T("TestVendor"), _T("2.1")));
   elements->add(new Component(6, COMPONENT_CLASS_MODULE, 5, 3, 0, _T("submodule"), _T("Sub-module"), _T("SM-10"), _T("SN0003"), _T("TestVendor"), _T("2.2")));
   elements->add(new Component(7, COMPONENT_CLASS_PORT, 6, 5, 1003, _T("port3"), _T("Port 3"), _T(""), _T(""), _T(""), _T("")));
}

/**
 * Build component tree from array created by CreateComponents
 */
static shared_ptr<ComponentTree> BuildTestTree(ObjectArray<Component> *elements)
{
   Component *root = nullptr;
   for(int i = 0; i < elements->size(); i++)
      if (elements->get(i)->getParentIndex() == 0)
      {
         root = elements->get(i);
         break;
      }
   AssertNotNull(root);
   root->buildTree(elements);
   return make_shared<ComponentTree>(root);
}

/**
 * Test Component and ComponentTree
 */
static void TestComponentTree()
{
   StartTest(_T("Component - buildTree"));
   ObjectArray<Component> elements(16, 16);
   CreateComponents(&elements);
   shared_ptr<ComponentTree> tree = BuildTestTree(&elements);
   AssertFalse(tree->isEmpty());

   const Component *root = tree->getRoot();
   AssertNotNull(root);
   AssertEquals(root->getIndex(), 1u);
   AssertEquals(root->getClass(), static_cast<uint32_t>(COMPONENT_CLASS_CHASSIS));
   AssertEquals(root->getName(), _T("chassis"));
   AssertEquals(root->getChildren().size(), 2);
   AssertNull(root->getParent());

   const Component *module1 = root->getChildren().get(0);
   AssertEquals(module1->getIndex(), 2u);   // sorted by position
   AssertEquals(module1->getChildren().size(), 2);
   AssertEquals(module1->getChildren().get(0)->getIndex(), 3u);
   AssertEquals(module1->getChildren().get(1)->getIndex(), 4u);
   AssertTrue(module1->getParent() == root);

   const Component *module2 = root->getChildren().get(1);
   AssertEquals(module2->getIndex(), 5u);
   AssertEquals(module2->getChildren().size(), 1);

   const Component *port3 = module2->getChildren().get(0)->getChildren().get(0);
   AssertEquals(port3->getIndex(), 7u);
   AssertEquals(port3->getIfIndex(), 1003u);
   EndTest();

   StartTest(_T("Component - orphaned element handling"));
   ObjectArray<Component> orphanElements(16, 16);
   CreateComponents(&orphanElements);
   Component *orphan = new Component(99, COMPONENT_CLASS_FAN, 98, 1, 0, _T("orphan"), _T("Orphaned fan"), _T(""), _T(""), _T(""), _T(""));
   orphanElements.add(orphan);
   shared_ptr<ComponentTree> orphanTree = BuildTestTree(&orphanElements);
   AssertNull(orphan->getParent());
   AssertEquals(orphanTree->getRoot()->getChildren().size(), 2);
   delete orphan;   // not owned by the tree
   EndTest();

   StartTest(_T("ComponentTree - equals"));
   ObjectArray<Component> elements2(16, 16);
   CreateComponents(&elements2);
   shared_ptr<ComponentTree> tree2 = BuildTestTree(&elements2);
   AssertTrue(tree->equals(tree2.get()));
   AssertTrue(tree2->equals(tree.get()));

   ObjectArray<Component> elements3(16, 16);
   CreateComponents(&elements3);
   elements3.add(new Component(8, COMPONENT_CLASS_FAN, 1, 3, 0, _T("fan"), _T("Fan tray"), _T(""), _T(""), _T(""), _T("")));
   shared_ptr<ComponentTree> tree3 = BuildTestTree(&elements3);
   AssertFalse(tree->equals(tree3.get()));
   AssertFalse(tree3->equals(tree.get()));
   EndTest();

   StartTest(_T("ComponentTree - NXCP serialization"));
   NXCPMessage msg(CMD_REQUEST_COMPLETED, 0);
   tree->fillMessage(&msg, VID_COMPONENT_LIST_BASE);
   AssertEquals(msg.getFieldAsUInt32(VID_COMPONENT_LIST_BASE), 1u);          // root index
   AssertEquals(msg.getFieldAsUInt32(VID_COMPONENT_LIST_BASE + 1), 0u);      // root parent index
   AssertEquals(msg.getFieldAsInt32(VID_COMPONENT_LIST_BASE + 2), 1);        // root position
   AssertEquals(msg.getFieldAsUInt32(VID_COMPONENT_LIST_BASE + 3), static_cast<uint32_t>(COMPONENT_CLASS_CHASSIS));
   TCHAR buffer[256];
   msg.getFieldAsString(VID_COMPONENT_LIST_BASE + 5, buffer, 256);
   AssertEquals(buffer, _T("chassis"));
   msg.getFieldAsString(VID_COMPONENT_LIST_BASE + 8, buffer, 256);
   AssertEquals(buffer, _T("SN12345"));
   AssertEquals(msg.getFieldAsUInt32(VID_COMPONENT_LIST_BASE + 19), 2u);     // number of children
   AssertEquals(msg.getFieldAsUInt32(VID_COMPONENT_LIST_BASE + 20), 2u);     // first child index
   EndTest();

   StartTest(_T("Component - physical location from ENTITY MIB"));
   const Component *port1 = tree->getRoot()->getChildren().get(0)->getChildren().get(0);
   InterfacePhysicalLocation location = InterfacePhysicalLocationFromEntityMib(port1);
   AssertEquals(location.chassis, 1u);
   AssertEquals(location.module, 1u);
   AssertEquals(location.pic, 0u);
   AssertEquals(location.port, 1u);

   // port3 is nested under two module levels (module2/submodule), second level maps to pic
   const Component *nestedPort = tree->getRoot()->getChildren().get(1)->getChildren().get(0)->getChildren().get(0);
   location = InterfacePhysicalLocationFromEntityMib(nestedPort);
   AssertEquals(location.chassis, 1u);
   AssertEquals(location.module, 2u);
   AssertEquals(location.pic, 3u);
   AssertEquals(location.port, 5u);
   EndTest();
}

/**
 * Test HostMibStorageEntry metric calculations
 */
static void TestHostMibStorageEntry()
{
   StartTest(_T("HostMibStorageEntry - metric calculations"));
   HostMibStorageEntry e;
   memset(&e, 0, sizeof(e));
   _tcscpy(e.name, _T("/data"));
   e.type = hrStorageFixedDisk;
   e.unitSize = 4096;
   e.size = 1000000;
   e.used = 250000;

   TCHAR buffer[64];
   e.getTotal(buffer, 64);
   AssertEquals(buffer, _T("4096000000"));
   e.getUsed(buffer, 64);
   AssertEquals(buffer, _T("1024000000"));
   e.getFree(buffer, 64);
   AssertEquals(buffer, _T("3072000000"));
   e.getUsedPerc(buffer, 64);
   AssertTrue(fabs(_tcstod(buffer, nullptr) - 25.0) < 0.0001);
   e.getFreePerc(buffer, 64);
   AssertTrue(fabs(_tcstod(buffer, nullptr) - 75.0) < 0.0001);
   EndTest();

   StartTest(_T("HostMibStorageEntry - 64 bit overflow handling"));
   e.unitSize = 65536;
   e.size = 0xFFFFFFFF;
   e.used = 0x80000000;

   e.getTotal(buffer, 64);
   AssertEquals(buffer, _T("281474976645120"));
   e.getUsed(buffer, 64);
   AssertEquals(buffer, _T("140737488355328"));
   e.getFree(buffer, 64);
   AssertEquals(buffer, _T("140737488289792"));
   EndTest();

   StartTest(_T("HostMibStorageEntry - getMetric"));
   e.unitSize = 1024;
   e.size = 1000;
   e.used = 400;

   AssertTrue(e.getMetric(_T("Total"), buffer, 64));
   AssertEquals(buffer, _T("1024000"));
   AssertTrue(e.getMetric(_T("used"), buffer, 64));   // case-insensitive
   AssertEquals(buffer, _T("409600"));
   AssertTrue(e.getMetric(_T("FREE"), buffer, 64));
   AssertEquals(buffer, _T("614400"));
   AssertTrue(e.getMetric(_T("UsedPerc"), buffer, 64));
   AssertTrue(fabs(_tcstod(buffer, nullptr) - 40.0) < 0.0001);
   AssertTrue(e.getMetric(_T("FreePerc"), buffer, 64));
   AssertTrue(fabs(_tcstod(buffer, nullptr) - 60.0) < 0.0001);
   AssertFalse(e.getMetric(_T("Bogus"), buffer, 64));
   EndTest();
}

/**
 * Test XML to JSON conversion
 */
static void TestXmlNodeToJson()
{
   StartTest(_T("XmlNodeToJson - elements and attributes"));
   static const char *xml =
      "<root version=\"2\" label=\"test\">"
      "<intValue>42</intValue>"
      "<negative>-7</negative>"
      "<strValue>hello</strValue>"
      "<nested attr=\"9\"><inner>5</inner><other>text</other></nested>"
      "<empty/>"
      "</root>";

   pugi::xml_document doc;
   AssertTrue(doc.load_string(xml));

   json_t *json = XmlNodeToJson(doc.child("root"));
   AssertNotNull(json);
   AssertEquals(json_integer_value(json_object_get(json, "intValue")), static_cast<int64_t>(42));
   AssertEquals(json_integer_value(json_object_get(json, "negative")), static_cast<int64_t>(-7));
   AssertEquals(json_string_value(json_object_get(json, "strValue")), "hello");
   AssertEquals(json_integer_value(json_object_get(json, "version")), static_cast<int64_t>(2));
   AssertEquals(json_string_value(json_object_get(json, "label")), "test");
   AssertNull(json_object_get(json, "empty"));   // empty elements are skipped

   json_t *nested = json_object_get(json, "nested");
   AssertNotNull(nested);
   AssertTrue(json_is_object(nested));
   AssertEquals(json_integer_value(json_object_get(nested, "inner")), static_cast<int64_t>(5));
   AssertEquals(json_string_value(json_object_get(nested, "other")), "text");
   AssertEquals(json_integer_value(json_object_get(nested, "attr")), static_cast<int64_t>(9));
   json_decref(json);
   EndTest();

   StartTest(_T("XmlNodeToJson - arrays"));
   static const char *arrayXml =
      "<root>"
      "<list class=\"java.util.List\"><int>1</int><int>2</int><string>three</string></list>"
      "<numbers length=\"3\">10,20,30</numbers>"
      "<complex class=\"custom\"><item><id>1</id><name>first</name></item></complex>"
      "</root>";

   pugi::xml_document doc2;
   AssertTrue(doc2.load_string(arrayXml));

   json_t *json2 = XmlNodeToJson(doc2.child("root"));
   AssertNotNull(json2);

   json_t *list = json_object_get(json2, "list");
   AssertNotNull(list);
   AssertTrue(json_is_array(list));
   AssertEquals(json_array_size(list), 3);
   AssertEquals(json_integer_value(json_array_get(list, 0)), static_cast<int64_t>(1));
   AssertEquals(json_integer_value(json_array_get(list, 1)), static_cast<int64_t>(2));
   AssertEquals(json_string_value(json_array_get(list, 2)), "three");

   json_t *numbers = json_object_get(json2, "numbers");
   AssertNotNull(numbers);
   AssertTrue(json_is_array(numbers));
   AssertEquals(json_array_size(numbers), 3);
   AssertEquals(json_integer_value(json_array_get(numbers, 0)), static_cast<int64_t>(10));
   AssertEquals(json_integer_value(json_array_get(numbers, 2)), static_cast<int64_t>(30));

   json_t *complex = json_object_get(json2, "complex");
   AssertNotNull(complex);
   AssertTrue(json_is_array(complex));
   AssertEquals(json_array_size(complex), 1);
   json_t *item = json_array_get(complex, 0);
   AssertTrue(json_is_object(item));
   AssertEquals(json_integer_value(json_object_get(item, "id")), static_cast<int64_t>(1));
   AssertEquals(json_string_value(json_object_get(item, "name")), "first");
   json_decref(json2);
   EndTest();
}

/**
 * Test InterfaceList
 */
static void TestInterfaceList()
{
   StartTest(_T("InterfaceList - add and find"));
   InterfaceList list;

   InterfaceInfo *eth0 = new InterfaceInfo(1);
   _tcscpy(eth0->name, _T("eth0"));
   eth0->ipAddrList.add(InetAddress::parse(_T("10.0.0.1")));
   list.add(eth0);

   InterfaceInfo *eth1 = new InterfaceInfo(2);
   _tcscpy(eth1->name, _T("eth1"));
   eth1->isPhysicalPort = true;
   eth1->location = InterfacePhysicalLocation(1, 2, 0, 4);
   list.add(eth1);

   AssertEquals(list.size(), 2);
   AssertTrue(list.get(0) == eth0);
   AssertNull(list.get(2));

   AssertTrue(list.findByIfIndex(1) == eth0);
   AssertTrue(list.findByIfIndex(2) == eth1);
   AssertNull(list.findByIfIndex(3));

   AssertTrue(list.findByPhysicalLocation(1, 2, 0, 4) == eth1);
   AssertNull(list.findByPhysicalLocation(1, 2, 0, 5));
   AssertNull(list.findByPhysicalLocation(InterfacePhysicalLocation()));   // eth0 is not a physical port

   AssertTrue(list.findByIpAddress(InetAddress::parse(_T("10.0.0.1"))) == eth0);
   AssertNull(list.findByIpAddress(InetAddress::parse(_T("192.168.1.1"))));
   EndTest();

   StartTest(_T("InterfaceList - remove"));
   list.remove(0);
   AssertEquals(list.size(), 1);
   AssertNull(list.findByIfIndex(1));
   AssertTrue(list.findByIfIndex(2) != nullptr);
   EndTest();
}

/**
 * Test ArpCache
 */
static void TestArpCache()
{
   StartTest(_T("ArpCache - add and find"));
   ArpCache cache;
   AssertEquals(cache.size(), 0);
   AssertTrue(cache.timestamp() != 0);

   cache.addEntry(InetAddress::parse(_T("10.0.0.1")), MacAddress::parse(_T("00:11:22:33:44:55")), 1);
   cache.addEntry(InetAddress::parse(_T("10.0.0.2")), MacAddress::parse(_T("00:11:22:33:44:56")), 2);
   AssertEquals(cache.size(), 2);

   const ArpEntry *entry = cache.findByIP(InetAddress::parse(_T("10.0.0.1")));
   AssertNotNull(entry);
   AssertEquals(entry->ifIndex, 1u);
   AssertTrue(entry->macAddr.equals(MacAddress::parse(_T("00:11:22:33:44:55"))));

   AssertNull(cache.findByIP(InetAddress::parse(_T("10.0.0.3"))));
   EndTest();

   StartTest(_T("ArpCache - lookup ignores mask bits"));
   InetAddress addr = InetAddress::parse(_T("10.0.0.2"));
   addr.setMaskBits(24);
   const ArpEntry *entry2 = cache.findByIP(addr);
   AssertNotNull(entry2);
   AssertEquals(entry2->ifIndex, 2u);
   EndTest();

   StartTest(_T("ArpCache - get by index"));
   AssertNotNull(cache.get(0));
   AssertEquals(cache.get(0)->ifIndex, 1u);
   AssertNull(cache.get(2));
   EndTest();
}

/**
 * Test agent error code mappings
 */
static void TestAgentErrorMappings()
{
   StartTest(_T("AgentErrorCodeToText"));
   AssertEquals(AgentErrorCodeToText(ERR_SUCCESS), _T("Success"));
   AssertEquals(AgentErrorCodeToText(ERR_ACCESS_DENIED), _T("Access denied"));
   AssertEquals(AgentErrorCodeToText(ERR_REQUEST_TIMEOUT), _T("Request timeout"));
   AssertEquals(AgentErrorCodeToText(ERR_PROXY_CONNECT_FAILED), _T("Cannot connect to proxy agent"));
   AssertEquals(AgentErrorCodeToText(0xFFFFFF), _T("Unknown error code"));
   EndTest();

   StartTest(_T("AgentErrorToRCC"));
   AssertEquals(AgentErrorToRCC(ERR_SUCCESS), RCC_SUCCESS);
   AssertEquals(AgentErrorToRCC(ERR_ACCESS_DENIED), RCC_AGENT_ACCESS_DENIED);
   AssertEquals(AgentErrorToRCC(ERR_BAD_SIGNATURE), RCC_AGENT_ACCESS_DENIED);
   AssertEquals(AgentErrorToRCC(ERR_IO_FAILURE), RCC_IO_ERROR);
   AssertEquals(AgentErrorToRCC(ERR_AUTH_FAILED), RCC_COMM_FAILURE);
   AssertEquals(AgentErrorToRCC(ERR_NO_SUCH_INSTANCE), RCC_NO_SUCH_INSTANCE);
   AssertEquals(AgentErrorToRCC(ERR_REQUEST_TIMEOUT), RCC_TIMEOUT);
   AssertEquals(AgentErrorToRCC(ERR_UNKNOWN_METRIC), RCC_UNKNOWN_METRIC);
   AssertEquals(AgentErrorToRCC(ERR_UNSUPPORTED_METRIC), RCC_UNKNOWN_METRIC);
   AssertEquals(AgentErrorToRCC(ERR_PROXY_CONNECT_FAILED), RCC_ZONE_PROXY_NOT_AVAILABLE);
   AssertEquals(AgentErrorToRCC(ERR_INTERNAL_ERROR), RCC_AGENT_ERROR);   // unmapped codes
   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   TestNObjectHierarchy();
   TestNObjectCustomAttributes();
   TestNObjectCustomAttributeInheritance();
   TestNObjectCustomAttributeConflicts();
   TestNObjectCustomAttributesFromMessage();
   TestVlanList();
   TestComponentTree();
   TestHostMibStorageEntry();
   TestXmlNodeToJson();
   TestInterfaceList();
   TestArpCache();
   TestAgentErrorMappings();
   return 0;
}
