/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: devicecontext.cpp
**
**/

#include "nxcore.h"
#include <nddrv.h>
#include <nxcore_netconf.h>

#define DEBUG_TAG L"ndd"

/**
 * NodeDeviceContext constructor. Externally supplied SNMP transport is not owned
 * by the context; if none is supplied, transport is created on first use.
 */
NodeDeviceContext::NodeDeviceContext(const shared_ptr<Node>& node, SNMP_Transport *snmpTransport) : m_node(node),
      m_snmpTransport(snmpTransport), m_ownSnmpTransport(false)
{
}

/**
 * NodeDeviceContext destructor
 */
NodeDeviceContext::~NodeDeviceContext()
{
   if (m_sshChannel != nullptr)
      m_sshChannel->close();
   if (m_ownSnmpTransport)
      delete m_snmpTransport;
}

/**
 * Check if SSH command channel is available
 */
bool NodeDeviceContext::isSSHCommandChannelAvailable()
{
   return (m_node->getCapabilities() & NC_SSH_COMMAND_CHANNEL) != 0;
}

/**
 * Check if SSH interactive channel is available
 */
bool NodeDeviceContext::isSSHInteractiveChannelAvailable()
{
   return (m_node->getCapabilities() & NC_SSH_INTERACTIVE_CHANNEL) != 0;
}

/**
 * Check if SNMP is available
 */
bool NodeDeviceContext::isSNMPAvailable()
{
   return (m_node->getCapabilities() & NC_IS_SNMP) != 0;
}

/**
 * Get interactive SSH channel (lazy init with privilege escalation)
 */
SSHInteractiveChannel *NodeDeviceContext::getInteractiveSSH()
{
   if (m_sshChannel != nullptr)
      return m_sshChannel.get();

   m_sshChannel = m_node->openInteractiveSSHChannel(nullptr, nullptr, 0);
   if (m_sshChannel == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeDeviceContext: cannot open SSH channel to %s [%u]", m_node->getName(), m_node->getId());
      return nullptr;
   }

   if (!m_sshChannel->waitForInitialPrompt())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeDeviceContext: timeout waiting for initial prompt from %s [%u]", m_node->getName(), m_node->getId());
      m_sshChannel->close();
      m_sshChannel.reset();
      return nullptr;
   }

   SSHDriverHints hints;
   m_node->getDriver()->getSSHDriverHints(&hints);
   if (hints.enableCommand != nullptr)
   {
      if (!m_sshChannel->escalatePrivilege(m_node->getSshPassword()))
         nxlog_debug_tag(DEBUG_TAG, 5, L"NodeDeviceContext: privilege escalation on %s [%u] failed (%s)",
               m_node->getName(), m_node->getId(), m_sshChannel->getLastErrorMessage());
   }

   m_sshChannel->disablePagination();
   return m_sshChannel.get();
}

/**
 * Execute SSH command via command channel
 */
bool NodeDeviceContext::executeSSHCommand(const char *command, ByteStream *output)
{
   uint32_t proxyId = m_node->getEffectiveSshProxy();
   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxyNode == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeDeviceContext::executeSSHCommand(%s [%u]): SSH proxy node [%u] not found", m_node->getName(), m_node->getId(), proxyId);
      return false;
   }

   shared_ptr<AgentConnectionEx> agentConn = proxyNode->createAgentConnection();
   if (agentConn == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeDeviceContext::executeSSHCommand(%s [%u]): cannot connect to SSH proxy node %s [%u]",
               m_node->getName(), m_node->getId(), proxyNode->getName(), proxyId);
      return false;
   }

   uint32_t rcc = agentConn->executeSSHCommand(m_node->getIpAddress(), m_node->getSshPort(),
            m_node->getSshLogin(), m_node->getSshPassword(), m_node->getSshKeyId(), command, output);
   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeDeviceContext::executeSSHCommand(%s [%u]): command execution failed (%u: %s)",
               m_node->getName(), m_node->getId(), rcc, AgentErrorCodeToText(rcc));
      return false;
   }
   return true;
}

/**
 * Check if NETCONF is available
 */
bool NodeDeviceContext::isNETCONFAvailable()
{
   return ((m_node->getCapabilities() & NC_IS_NETCONF) != 0) && ((m_node->getFlags() & (NF_DISABLE_SSH | NF_DISABLE_NETCONF)) == 0);
}

/**
 * Execute sequence of NETCONF RPCs on a single device session
 */
int NodeDeviceContext::executeNETCONFRequests(int count, const char * const *requests, char **replies, uint32_t timeout)
{
   uint32_t agentRcc;
   int received = ExecuteNetconfRpcBatch(*m_node, count, requests, replies, timeout, &agentRcc);
   if (received < count)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeDeviceContext::executeNETCONFRequests(%s [%u]): %d of %d RPCs executed (agent error %u: %s)",
            m_node->getName(), m_node->getId(), (received > 0) ? received : 0, count, agentRcc, AgentErrorCodeToText(agentRcc));
   }
   return received;
}

/**
 * Get SNMP transport (lazy init if not externally supplied)
 */
SNMP_Transport *NodeDeviceContext::getSNMPTransport()
{
   if (m_snmpTransport != nullptr)
      return m_snmpTransport;

   m_snmpTransport = m_node->createSnmpTransport();
   m_ownSnmpTransport = (m_snmpTransport != nullptr);
   return m_snmpTransport;
}
