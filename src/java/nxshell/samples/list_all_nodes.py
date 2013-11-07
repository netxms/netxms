import csv
import sys

w = csv.writer(sys.stdout, dialect='excel')
w.writerow(['node_id', 'interface_id', 'name', 'ip', 'mask']) # Header

for node in [o for o in s.getAllObjects() if isinstance(o, objects.Node)]: # filter all objects for objects.Node
    for interface in node.getAllChilds(objects.GenericObject.OBJECT_INTERFACE):
        w.writerow([
            node.getObjectId(),
            interface.getObjectId(),
            node.getObjectName(),
            interface.getPrimaryIP().getHostAddress(),
            interface.getSubnetMask().getHostAddress()
        ])
