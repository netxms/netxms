from org.netxms.client.objects import GenericObject, Node, Interface
 
for name in open("nodes.txt").readlines():
    node = session.findObjectByName(name.strip())
    if node:
        for interface in node.getAllChilds(GenericObject.OBJECT_INTERFACE): # 3 == interface
            name = interface.getObjectName()
            if name.startswith('lo'):
                session.setObjectManaged(interface.getObjectId(), True)
            else:
                session.setObjectManaged(interface.getObjectId(), False)
