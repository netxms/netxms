import csv
import sys

w = csv.writer(sys.stdout, dialect='excel')
w.writerow(['node_id', 'interface_id', 'name', 'ip', 'mask']) # Header

for node in filter(lambda x: isinstance(x, objects.Node), s.getAllObjects()):
    allInterfaces = node.getAllChilds(objects.GenericObject.OBJECT_INTERFACE)
    # iftype=6 - ethernetCsmacd, http://www.net-snmp.org/docs/mibs/interfaces.html#IANAifType
    #            or check IFTYPE_* constants in src/nms_common.h
    interfaces = filter(lambda i: i.getIfType==6 and i.getOperState() == objects.Interface.ADMIN_STATE_DOWN, allInterfaces)
    for interface in interfaces:
        w.writerow([
            node.getObjectId(),
            interface.getObjectId(),
            node.getObjectName(),
            interface.getPrimaryIP().getHostAddress(),
            interface.getSubnetMask().getHostAddress()
        ])
