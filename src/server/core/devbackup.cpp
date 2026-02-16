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
** File: devbackup.cpp
**
**/

#include "nxcore.h"
#include <nddrv.h>
#include <device-backup.h>

#define DEBUG_TAG _T("backup")

/**
 * Device backup interface
 */
static DeviceBackupInterface *s_provider;

/**
 * Get error message for given status
 */
const TCHAR *GetDeviceBackupApiErrorMessage(DeviceBackupApiStatus status)
{
   static const TCHAR *messages[] =
   {
      _T("No errors"),
      _T("Service provider unavailable"),
      _T("Function not implemented"),
      _T("External API error"),
      _T("Device not registered for backups")
   };
   return messages[static_cast<int>(status)];
}

/**
 * Register device for backup
 */
DeviceBackupApiStatus DevBackupRegisterDevice(Node *node)
{
   return (s_provider != nullptr) ? s_provider->RegisterDevice(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Delete device from backup service
 */
DeviceBackupApiStatus DevBackupDeleteDevice(Node *node)
{
   return (s_provider != nullptr) ? s_provider->DeleteDevice(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Check if device is registered for backup
 */
bool DevBackupIsDeviceRegistered(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->IsDeviceRegistered(node) : false;
}

/**
 * Validate device registration
 */
DeviceBackupApiStatus DevBackupValidateDeviceRegistration(Node *node)
{
   return (s_provider != nullptr) ? s_provider->ValidateDeviceRegistration(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Initiate immediate device backup
 */
DeviceBackupApiStatus DevBackupStartJob(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->StartJob(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Get status of last backup job
 */
std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo> DevBackupGetLastJobStatus(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->GetLastJobStatus(node) : std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, DeviceBackupJobInfo());
}

/**
 * Get latest device backup
 */
std::pair<DeviceBackupApiStatus, BackupData> DevBackupGetLatestBackup(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->GetLatestBackup(node) : std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, BackupData());
}

/**
 * Get list of device backups (metadata only)
 */
std::pair<DeviceBackupApiStatus, std::vector<BackupData>> DevBackupGetBackupList(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->GetBackupList(node) : std::pair<DeviceBackupApiStatus, std::vector<BackupData>>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, std::vector<BackupData>());
}

/**
 * Get device backup by ID (with full config content)
 */
std::pair<DeviceBackupApiStatus, BackupData> DevBackupGetBackupById(const Node& node, int64_t id)
{
   return (s_provider != nullptr) ? s_provider->GetBackupById(node, id) : std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, BackupData());
}

/**
 * Initialize device backup interface
 */
void InitializeDeviceBackupInterface()
{
   const TCHAR *providerName;
   ENUMERATE_MODULES(deviceBackupInterface)
   {
      DeviceBackupApiStatus status = CURRENT_MODULE.deviceBackupInterface->Initialize();
      if (status == DeviceBackupApiStatus::SUCCESS)
      {
         s_provider = CURRENT_MODULE.deviceBackupInterface;
         providerName = CURRENT_MODULE.name;
         break;
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Network device backup interface provided by module %s cannot be initialized (%s)"),
            CURRENT_MODULE.name, GetDeviceBackupApiErrorMessage(status));
      }
   }

   if (s_provider != nullptr)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Network device backup interface is provided by module %s"), providerName);
      RegisterComponent(_T("DEVBACKUP"));
      InterlockedOr64(&g_flags, AF_DEVICE_BACKUP_API_ENABLED);
   }
   else
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Network device backup interface is not available"));
   }
}

/**
 * NodeBackupContext constructor
 */
NodeBackupContext::NodeBackupContext(const shared_ptr<Node>& node) : m_node(node), m_snmpTransport(nullptr)
{
}

/**
 * NodeBackupContext destructor
 */
NodeBackupContext::~NodeBackupContext()
{
   if (m_sshChannel != nullptr)
      m_sshChannel->close();
   delete m_snmpTransport;
}

/**
 * Check if SSH command channel is available
 */
bool NodeBackupContext::isSSHCommandChannelAvailable()
{
   return (m_node->getCapabilities() & NC_SSH_COMMAND_CHANNEL) != 0;
}

/**
 * Check if SSH interactive channel is available
 */
bool NodeBackupContext::isSSHInteractiveChannelAvailable()
{
   return (m_node->getCapabilities() & NC_SSH_INTERACTIVE_CHANNEL) != 0;
}

/**
 * Check if SNMP is available
 */
bool NodeBackupContext::isSNMPAvailable()
{
   return (m_node->getCapabilities() & NC_IS_SNMP) != 0;
}

/**
 * Get interactive SSH channel (lazy init with privilege escalation)
 */
SSHInteractiveChannel *NodeBackupContext::getInteractiveSSH()
{
   if (m_sshChannel != nullptr)
      return m_sshChannel.get();

   m_sshChannel = m_node->openInteractiveSSHChannel(nullptr, nullptr, 0);
   if (m_sshChannel == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeBackupContext: cannot open SSH channel to %s [%u]", m_node->getName(), m_node->getId());
      return nullptr;
   }

   if (!m_sshChannel->waitForInitialPrompt())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeBackupContext: timeout waiting for initial prompt from %s [%u]", m_node->getName(), m_node->getId());
      m_sshChannel->close();
      m_sshChannel.reset();
      return nullptr;
   }

   SSHDriverHints hints;
   m_node->getDriver()->getSSHDriverHints(&hints);
   if (hints.enableCommand != nullptr)
   {
      m_sshChannel->escalatePrivilege(m_node->getSshPassword());
   }

   m_sshChannel->disablePagination();
   return m_sshChannel.get();
}

/**
 * Execute SSH command via command channel
 */
bool NodeBackupContext::executeSSHCommand(const char *command, ByteStream *output)
{
   uint32_t proxyId = m_node->getEffectiveSshProxy();
   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxyNode == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeBackupContext::executeSSHCommand(%s [%u]): SSH proxy node [%u] not found", m_node->getName(), m_node->getId(), proxyId);
      return false;
   }

   shared_ptr<AgentConnectionEx> agentConn = proxyNode->createAgentConnection();
   if (agentConn == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeBackupContext::executeSSHCommand(%s [%u]): cannot connect to SSH proxy node %s [%u]",
               m_node->getName(), m_node->getId(), proxyNode->getName(), proxyId);
      return false;
   }

   uint32_t rcc = agentConn->executeSSHCommand(m_node->getIpAddress(), m_node->getSshPort(),
            m_node->getSshLogin(), m_node->getSshPassword(), m_node->getSshKeyId(), command, output);
   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NodeBackupContext::executeSSHCommand(%s [%u]): command execution failed (%u: %s)",
               m_node->getName(), m_node->getId(), rcc, AgentErrorCodeToText(rcc));
      return false;
   }
   return true;
}

/**
 * Get SNMP transport (lazy init)
 */
SNMP_Transport *NodeBackupContext::getSNMPTransport()
{
   if (m_snmpTransport != nullptr)
      return m_snmpTransport;

   m_snmpTransport = m_node->createSnmpTransport();
   return m_snmpTransport;
}
