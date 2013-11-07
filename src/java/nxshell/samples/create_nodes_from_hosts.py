import re

parentId = objects.GenericObject.SERVICEROOT # Infrastructure Services root
#flags = NXCObjectCreationData.CF_DISABLE_ICMP | \
#        NXCObjectCreationData.CF_DISABLE_NXCP | \
#        NXCObjectCreationData.CF_DISABLE_SNMP
flags = 0

for line in open("/etc/hosts").readlines():
    line = line.strip()
    if line.startswith('#'):
        continue
    data = re.split(r'\s+', line)
    if len(data) < 2:
        continue
    (ip, name) = data[:2]

    cd = NXCObjectCreationData(objects.GenericObject.OBJECT_NODE, name, parentId);
    cd.setCreationFlags(flags);
    cd.setPrimaryName(ip)
    try:
        nodeId = session.createObject(cd)
        print '"%s" created, id=%d' % (name, nodeId)
    except NXCException:
        print "Can't create \"%s\" - %s" % (name, ip)
