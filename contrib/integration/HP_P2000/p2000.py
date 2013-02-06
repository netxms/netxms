#!/usr/bin/env python

#
# Config
#

confHostname = '127.0.0.1'
confLogin = 'netxms'
confPassword = 'password'
ssl = True


#
# Main code
#

confUrl = ('https://' if ssl else 'http://') + confHostname + '/api/'
prefix = 'P2000.'

import hashlib
import socket
import urllib2

from lxml import etree


socket.setdefaulttimeout(15)
handler = urllib2.HTTPHandler(debuglevel=0)
opener = urllib2.build_opener(handler)


def makeCall(url):
    req = urllib2.Request(confUrl + url)
    res = opener.open(req)
    return etree.XML(res.read())


def login():
    authToken = hashlib.md5(confLogin + '_' + confPassword).hexdigest()
    root = makeCall('login/' + authToken)
    status = root.findtext(".//PROPERTY[@name='response-type-numeric']")
    token = root.findtext(".//PROPERTY[@name='response']")
    return token if status == '0' else None


def processEnclosureStatus(root):
    enclosureId = 0
    for obj in root:
        objectName = obj.get('name')
        if objectName == 'enclosure-environmental':
            enclosureId += 1
            # next enclosure found
            for prop in obj:
                print prefix + 'Enclosure[%d].%s=%s' % (enclosureId, prop.get('name'), prop.text)
        elif objectName == 'enclosure-component':
            unitNumber = int(obj.findtext('./PROPERTY[@name="enclosure-unit-number"]'))
            unitType = obj.findtext('./PROPERTY[@name="type"]')
            for prop in obj:
                name = prop.get('name')
                if name in ('enclosure-unit-number', 'type'):
                    continue
                value = prop.text
                if name == 'additional-data':
                    name = 'current-value'
                    if '=' in value:
                        value = prop.text.split('=')[1]
                print prefix + 'Enclosure[%d].%s[%d].%s=%s' % (enclosureId, unitType, unitNumber, name, value)


def processStatistics(root, objectClass, objectName, durableIdName):
    for obj in root.findall('./OBJECT[@name="%s"]' % objectName):
        durableId = obj.findtext('./PROPERTY[@name="%s"]' % durableIdName)
        for prop in obj:
            name = prop.get('name')
            if name == durableIdName:
                continue
            print prefix + '%s[%s].%s=%s' % (objectClass, durableId, name, prop.text)


def processControllerStatistics(root):
    processStatistics(root, 'Controller', 'controller-statistics', 'durable-id')


def processDiskStatistics(root):
    processStatistics(root, 'Disk', 'disk-statistics', 'durable-id')


def processVDiskStatistics(root):
    processStatistics(root, 'VDisk', 'vdisk-statistics', 'name')


def processVolumeStatistics(root):
    processStatistics(root, 'Volume', 'volume-statistics', 'volume-name')


def main():
    token = login()
    if token:
        opener.addheaders.append(('Cookie', 'wbisessionkey=' + token))
        processEnclosureStatus(makeCall('show/enclosure-status'))
        processControllerStatistics(makeCall('show/controller-statistics'))
        processDiskStatistics(makeCall('show/disk-statistics'))
        processVDiskStatistics(makeCall('show/vdisk-statistics'))
        processVolumeStatistics(makeCall('show/volume-statistics'))

if __name__ == "__main__":
    try:
        main()
    except:
        import sys
        sys.exit(1)
