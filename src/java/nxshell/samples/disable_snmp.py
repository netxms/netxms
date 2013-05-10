from org.netxms.client.objects import Node
 
for name in open("nodes.txt").readlines():
    node = session.findObjectByName(name.strip())
    if node:
        md = NXCObjectModificationData(node.getObjectId())
        newFlags = node.getFlags() | Node.NF_DISABLE_SNMP
        md.setObjectFlags(newFlags)
        session.modifyObject(md)
