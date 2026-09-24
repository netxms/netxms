/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: node_ip_change.cpp
**
** Tests for Node::changeIPAddress(): primary IP change must not return
** unmanaged node or unmanaged child objects to managed state.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Create fake interface with given index and IP address on node
 */
static shared_ptr<Interface> CreateFakeInterface(const shared_ptr<Node>& node, uint32_t index, const TCHAR *ipAddr)
{
   InterfaceInfo info(index);
   info.ipAddrList.add(InetAddress::parse(ipAddr));
   return node->createNewInterface(&info, false, true);
}

/**
 * Test that changing primary IP address keeps management status of node and its children
 */
void TestNodeIPChange()
{
   StartTest(_T("Node::changeIPAddress() keeps management status"));

   NewNodeData data(InetAddress::parse(_T("10.255.255.1")));
   shared_ptr<Node> node = make_shared<Node>(&data, 0);
   node->unpublish();   // Keep poll manager away from this node
   NetObjInsert(node, true, false);

   shared_ptr<Interface> iface1 = CreateFakeInterface(node, 1, _T("10.255.255.1"));
   shared_ptr<Interface> iface2 = CreateFakeInterface(node, 2, _T("10.255.255.2"));
   AssertEquals(node->getStatus(), STATUS_UNKNOWN);

   // Managed node: IP address and primary host name follow the change
   InetAddress ip2 = InetAddress::parse(_T("10.255.255.11"));
   node->changeIPAddress(ip2);
   AssertTrue(node->getIpAddress().equals(ip2));
   AssertTrue(!_tcscmp(node->getPrimaryHostName().cstr(), _T("10.255.255.11")));

   // Individually unmanaged interface of managed node stays unmanaged
   iface2->setMgmtStatus(false);
   AssertEquals(iface2->getStatus(), STATUS_UNMANAGED);
   InetAddress ip3 = InetAddress::parse(_T("10.255.255.12"));
   node->changeIPAddress(ip3);
   AssertTrue(node->getIpAddress().equals(ip3));
   AssertEquals(node->getStatus(), STATUS_UNKNOWN);
   AssertEquals(iface1->getStatus(), STATUS_UNKNOWN);
   AssertEquals(iface2->getStatus(), STATUS_UNMANAGED);

   // Unmanaged node stays unmanaged together with all its children
   node->setMgmtStatus(false);
   AssertEquals(node->getStatus(), STATUS_UNMANAGED);
   AssertEquals(iface1->getStatus(), STATUS_UNMANAGED);
   InetAddress ip4 = InetAddress::parse(_T("10.255.255.13"));
   node->changeIPAddress(ip4);
   AssertTrue(node->getIpAddress().equals(ip4));
   AssertEquals(node->getStatus(), STATUS_UNMANAGED);
   AssertEquals(iface1->getStatus(), STATUS_UNMANAGED);
   AssertEquals(iface2->getStatus(), STATUS_UNMANAGED);

   EndTest();
}
