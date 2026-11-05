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
** File: correlation.cpp
**
** Tests for beacon-based event correlation (NetXMS server network connectivity loss)
** of down and recovery events.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Correlated event identifiers
 */
struct CorrelationResult
{
   uint64_t id;
   uint64_t rootId;
};

/**
 * Build event, run it through correlation engine (without event processing policy) and return its ID and root ID
 */
static CorrelationResult Correlate(EventBuilder& builder)
{
   ObjectQueue<Event> queue(16, Ownership::False);
   AssertTrue(builder.post(&queue));
   Event *event = queue.get();
   AssertNotNull(event);
   CorrelateEvent(event);
   CorrelationResult result = { event->getId(), event->getRootId() };
   delete event;
   return result;
}

/**
 * Overload for builder without parameters
 */
static CorrelationResult Correlate(EventBuilder&& builder)
{
   return Correlate(builder);
}

/**
 * Correlate SYS_NODE_UNREACHABLE event
 */
static CorrelationResult NodeUnreachable(const Node& node, NetworkPathFailureReason reason, uint32_t rootCauseNodeId)
{
   return Correlate(EventBuilder(EVENT_NODE_UNREACHABLE, node)
      .param(L"reasonCode", static_cast<int32_t>(reason))
      .param(L"reason", L"test")
      .param(L"rootCauseNodeId", rootCauseNodeId, EventBuilder::OBJECT_ID_FORMAT));
}

/**
 * Create unpublished node (keeps poll manager away)
 */
static shared_ptr<Node> CreateTestNode(const TCHAR *name)
{
   NewNodeData data((InetAddress()));
   shared_ptr<Node> node = make_shared<Node>(&data, 0);
   node->setName(name);
   node->unpublish();
   NetObjInsert(node, true, false);
   return node;
}

/**
 * Test correlation of down and recovery events to server network connectivity loss and restore
 */
void TestNetworkLossCorrelation()
{
   StartTest(_T("Network loss correlation: node events"));

   shared_ptr<Node> node = CreateTestNode(_T("Correlation Test Node"));
   shared_ptr<Node> proxy = CreateTestNode(_T("Correlation Test Proxy"));
   shared_ptr<Node> nodeBehindProxy = CreateTestNode(_T("Correlation Test Node Behind Proxy"));

   // Outage 1: down event is rooted to connectivity loss
   InterlockedOr64(&g_flags, AF_NO_NETWORK_CONNECTIVITY);
   CorrelationResult lost1 = Correlate(EventBuilder(EVENT_NETWORK_CONNECTION_LOST, *node));
   CorrelationResult down = Correlate(EventBuilder(EVENT_NODE_DOWN, *node));
   AssertEquals(down.rootId, lost1.id);
   AssertEquals(node->getLastEventId(LAST_EVENT_NETWORK_LOSS), lost1.id);

   // Recovery before restore is rooted to the loss event
   CorrelationResult up = Correlate(EventBuilder(EVENT_NODE_UP, *node));
   AssertEquals(up.rootId, lost1.id);
   AssertEquals(node->getLastEventId(LAST_EVENT_NETWORK_LOSS), static_cast<uint64_t>(0));
   AssertEquals(node->getLastEventId(LAST_EVENT_NODE_DOWN), static_cast<uint64_t>(0));

   // Tunnel-only proxy goes down, node behind it inherits the loss through the path failure root cause
   CorrelationResult proxyDown = NodeUnreachable(*proxy, NetworkPathFailureReason::AGENT_TUNNEL_DOWN, 0);
   AssertEquals(proxyDown.rootId, lost1.id);
   AssertEquals(proxy->getLastEventId(LAST_EVENT_NETWORK_LOSS), lost1.id);
   CorrelationResult behindDown = NodeUnreachable(*nodeBehindProxy, NetworkPathFailureReason::PROXY_NODE_DOWN, proxy->getId());
   AssertEquals(behindDown.rootId, proxyDown.id);
   AssertEquals(nodeBehindProxy->getLastEventId(LAST_EVENT_NETWORK_LOSS), lost1.id);
   Correlate(EventBuilder(EVENT_NODE_DOWN, *node));

   // Restore: recovery is rooted to the restore event
   InterlockedAnd64(&g_flags, ~AF_NO_NETWORK_CONNECTIVITY);
   CorrelationResult restored1 = Correlate(EventBuilder(EVENT_NETWORK_CONNECTION_RESTORED, *node));
   AssertEquals(Correlate(EventBuilder(EVENT_NODE_UP, *proxy)).rootId, restored1.id);
   AssertEquals(Correlate(EventBuilder(EVENT_NODE_UP, *nodeBehindProxy)).rootId, restored1.id);
   AssertEquals(Correlate(EventBuilder(EVENT_NODE_UP, *node)).rootId, restored1.id);

   // Outage without connectivity loss is not correlated in either direction
   down = Correlate(EventBuilder(EVENT_NODE_DOWN, *node));
   AssertEquals(down.rootId, static_cast<uint64_t>(0));
   AssertEquals(node->getLastEventId(LAST_EVENT_NETWORK_LOSS), static_cast<uint64_t>(0));
   AssertEquals(Correlate(EventBuilder(EVENT_NODE_UP, *node)).rootId, static_cast<uint64_t>(0));
   AssertEquals(NodeUnreachable(*proxy, NetworkPathFailureReason::AGENT_TUNNEL_DOWN, 0).rootId, static_cast<uint64_t>(0));
   AssertEquals(proxy->getLastEventId(LAST_EVENT_NETWORK_LOSS), static_cast<uint64_t>(0));
   AssertEquals(Correlate(EventBuilder(EVENT_NODE_UP, *proxy)).rootId, static_cast<uint64_t>(0));

   // Outage 2: restore event of outage 1 must not be used for recovery during outage 2
   InterlockedOr64(&g_flags, AF_NO_NETWORK_CONNECTIVITY);
   CorrelationResult lost2 = Correlate(EventBuilder(EVENT_NETWORK_CONNECTION_LOST, *node));
   Correlate(EventBuilder(EVENT_NODE_DOWN, *node));
   AssertEquals(Correlate(EventBuilder(EVENT_NODE_UP, *node)).rootId, lost2.id);

   // Down event posted after restore replaces attribution of earlier correlated outage
   Correlate(EventBuilder(EVENT_NODE_DOWN, *node));
   InterlockedAnd64(&g_flags, ~AF_NO_NETWORK_CONNECTIVITY);
   Correlate(EventBuilder(EVENT_NETWORK_CONNECTION_RESTORED, *node));
   AssertEquals(Correlate(EventBuilder(EVENT_NODE_DOWN, *node)).rootId, static_cast<uint64_t>(0));
   AssertEquals(Correlate(EventBuilder(EVENT_NODE_UP, *node)).rootId, static_cast<uint64_t>(0));

   node->deleteObject();
   proxy->deleteObject();
   nodeBehindProxy->deleteObject();

   EndTest();

   StartTest(_T("Network loss correlation: access point events"));

   // Connectivity events are recorded by node event correlation, so they are posted with a node as source
   // (in the server it is the management node)
   shared_ptr<Node> serverNode = CreateTestNode(_T("Correlation Test Server Node"));
   shared_ptr<AccessPoint> ap = make_shared<AccessPoint>(_T("Correlation Test AP"), 1, MacAddress::parse("00:11:22:33:44:55"));
   NetObjInsert(ap, true, false);

   // Unknown state (controller unreachable) during outage, recovery after restore
   InterlockedOr64(&g_flags, AF_NO_NETWORK_CONNECTIVITY);
   CorrelationResult lost3 = Correlate(EventBuilder(EVENT_NETWORK_CONNECTION_LOST, *serverNode));
   AssertEquals(Correlate(EventBuilder(EVENT_AP_UNKNOWN, *ap)).rootId, lost3.id);
   AssertEquals(Correlate(EventBuilder(EVENT_AP_DOWN, *ap)).rootId, lost3.id);
   AssertEquals(ap->getNetworkLossEventId(), lost3.id);
   InterlockedAnd64(&g_flags, ~AF_NO_NETWORK_CONNECTIVITY);
   CorrelationResult restored3 = Correlate(EventBuilder(EVENT_NETWORK_CONNECTION_RESTORED, *serverNode));
   AssertEquals(Correlate(EventBuilder(EVENT_AP_UP, *ap)).rootId, restored3.id);
   AssertEquals(ap->getNetworkLossEventId(), static_cast<uint64_t>(0));

   // State change observed after restore ends attribution to connectivity loss
   InterlockedOr64(&g_flags, AF_NO_NETWORK_CONNECTIVITY);
   Correlate(EventBuilder(EVENT_NETWORK_CONNECTION_LOST, *serverNode));
   Correlate(EventBuilder(EVENT_AP_UNKNOWN, *ap));
   InterlockedAnd64(&g_flags, ~AF_NO_NETWORK_CONNECTIVITY);
   Correlate(EventBuilder(EVENT_NETWORK_CONNECTION_RESTORED, *serverNode));
   AssertEquals(Correlate(EventBuilder(EVENT_AP_UNPROVISIONED, *ap)).rootId, static_cast<uint64_t>(0));
   AssertEquals(Correlate(EventBuilder(EVENT_AP_UP, *ap)).rootId, static_cast<uint64_t>(0));

   ap->deleteObject();
   serverNode->deleteObject();

   EndTest();
}
