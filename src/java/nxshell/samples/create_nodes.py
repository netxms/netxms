parentId = objects.GenericObject.SERVICEROOT # Infrastructure Services root
cd = NXCObjectCreationData(objects.GenericObject.OBJECT_CONTAINER, "Sample Container", parentId);
containerId = session.createObject(cd) # createObject return ID of newly created object
print '"Sample Container" created, id=%d' % (containerId, )

flags = NXCObjectCreationData.CF_DISABLE_ICMP | \
        NXCObjectCreationData.CF_DISABLE_NXCP | \
        NXCObjectCreationData.CF_DISABLE_SNMP
for i in xrange(0, 5):
    name = "Node %d" % (i + 1, )
    cd = NXCObjectCreationData(objects.GenericObject.OBJECT_NODE, name, containerId);
    cd.setCreationFlags(flags);
    cd.setPrimaryName("0.0.0.0") # Create node without IP address
    nodeId = session.createObject(cd)
    print '"%s" created, id=%d' % (name, nodeId)
