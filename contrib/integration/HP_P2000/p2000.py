#!/usr/bin/env python

prefix = 'P2000'

import hashlib
import socket
import urllib2
import argparse

from lxml import etree


handler = urllib2.HTTPHandler(debuglevel=0)
opener = urllib2.build_opener(handler)


def makeCall(command):
    url = ('https://' if config.ssl else 'http://') + config.address + '/api/'
    req = urllib2.Request(url + command)
    res = opener.open(req)
    return etree.XML(res.read())


def login():
    authToken = hashlib.md5(config.user + '_' + config.password).hexdigest()
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


def parsecmdline():
    global config
    parser = argparse.ArgumentParser(description='Get status and performance data from HP P2000 disk arrays.')
    parser.add_argument('-a', '--address', required=True, help='Array IP address')
    parser.add_argument('-u', '--user', required=True, help='Username')
    parser.add_argument('-p', '--password', required=True, help='Password')
    parser.add_argument('-n', '--nossl', dest='ssl', action='store_false', help='Disable https')
    parser.add_argument('-t', '--tag', help='Array tag, will be added to DCI name after prefix (e.g. "P2000[tag]."). Usefull for monitoring multiple arrays')
    parser.add_argument('-d', '--debug', action='store_true', help='Enable debug output')
    parser.add_argument('-T', '--timeout', default=15, type=int, help='Network timeout in seconds, default value: 15.')
    config = parser.parse_args()


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
    parsecmdline()
    socket.setdefaulttimeout(config.timeout)
    if config.debug:
        handler.set_http_debuglevel(1)
    if config.tag is not None:
        prefix = "%s[%s]." % (prefix, config.tag, )
    else:
        prefix += '.'

    try:
        main()
    except Exception as e:
        import sys, traceback
        if config.debug:
            traceback.print_exc(e);
        sys.exit(1)
