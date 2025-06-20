/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: sensor.cpp
**/

#include "nxcore.h"
#include <asset_management.h>
#include <netxms_maps.h>

/**
 * Default empty Sensor class constructior
 */
Sensor::Sensor() : super(Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_gatewayNodeId = 0;
	m_deviceClass = SENSOR_OTHER;
	m_modbusUnitId = 255;
	m_capabilities = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Constructor with all fields for Sensor class
 */
Sensor::Sensor(const wchar_t *name, const NXCPMessage& request) : super(name, Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PENDING;
   m_flags = request.getFieldAsUInt32(VID_SENSOR_FLAGS);
   m_macAddress = request.getFieldAsMacAddress(VID_MAC_ADDR);
	m_deviceClass = static_cast<SensorDeviceClass>(request.getFieldAsInt16(VID_DEVICE_CLASS));
	m_vendor = request.getFieldAsSharedString(VID_VENDOR);
   m_model = request.getFieldAsSharedString(VID_MODEL);
	m_serialNumber = request.getFieldAsSharedString(VID_SERIAL_NUMBER);
	m_deviceAddress = request.getFieldAsSharedString(VID_DEVICE_ADDRESS);
	m_gatewayNodeId = request.getFieldAsUInt32(VID_GATEWAY_NODE);
   m_modbusUnitId = request.getFieldAsUInt16(VID_MODBUS_UNIT_ID);
   m_capabilities = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Constructor for creating sensor object from scratch
 */
Sensor::Sensor(const wchar_t *name, SensorDeviceClass deviceClass, uint32_t gatewayNodeId, uint16_t modbusUnitId) : super(name, Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_gatewayNodeId = gatewayNodeId;
   m_deviceClass = deviceClass;
   m_modbusUnitId = modbusUnitId;
   m_capabilities = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Sensor class destructor
 */
Sensor::~Sensor()
{
}

/**
 * Load from database SensorDevice class
 */
bool Sensor::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (Pollable::loadFromDatabase(hdb, m_id))
   {
      if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) < getCustomAttributeAsUInt32(_T("SysConfig:Objects.ConfigurationPollingInterval"), g_configurationPollingInterval))
         m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   }

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT mac_address,device_class,vendor,model,serial_number,device_address,modbus_unit_id,gateway_node,capabilities FROM sensors WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
   DBFreeStatement(hStmt);
	if (hResult == nullptr)
	{
		return false;
	}

   m_macAddress = DBGetFieldMacAddr(hResult, 0, 0);
	m_deviceClass = static_cast<SensorDeviceClass>(DBGetFieldLong(hResult, 0, 1));
	m_vendor = DBGetFieldAsSharedString(hResult, 0, 2);
   m_model = DBGetFieldAsSharedString(hResult, 0, 3);
	m_serialNumber = DBGetFieldAsSharedString(hResult, 0, 4);
	m_deviceAddress = DBGetFieldAsSharedString(hResult, 0, 5);
   m_modbusUnitId = static_cast<uint16_t>(DBGetFieldULong(hResult, 0, 6));
   m_gatewayNodeId = DBGetFieldULong(hResult, 0, 7);
   m_capabilities = DBGetFieldULong(hResult, 0, 8);
   DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB(hdb, preparedStatements);
   loadItemsFromDB(hdb, preparedStatements);
   for(int i = 0; i < m_dcObjects.size(); i++)
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb, preparedStatements))
         return false;
   loadDCIListForCleanup(hdb);

   return true;
}

/**
 * Save to database Sensor class
 */
bool Sensor::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_SENSOR_PROPERTIES))
   {
      static const TCHAR *columns[] = {
         _T("mac_address"), _T("device_class"), _T("vendor"), _T("model"), _T("serial_number"), _T("device_address"), _T("modbus_unit_id"), _T("gateway_node"), _T("capabilities"), nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("sensors"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_macAddress);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_deviceClass);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_TRANSIENT);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_model, DB_BIND_TRANSIENT);
         DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_serialNumber, DB_BIND_TRANSIENT);
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_deviceAddress, DB_BIND_TRANSIENT);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_modbusUnitId));
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_gatewayNodeId);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_capabilities);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_id);
         unlockProperties();

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
	}
   return success;
}

/**
 * Delete from database
 */
bool Sensor::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM sensors WHERE id=?"));
   return success;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Sensor::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslSensorClass, new shared_ptr<Sensor>(self())));
}

/**
 * Sensor class serialization to JSON
 */
json_t *Sensor::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "macAddress", json_string_t(m_macAddress.toString()));
   json_object_set_new(root, "deviceClass", json_integer(m_deviceClass));
   json_object_set_new(root, "vendor", json_string_t(m_vendor));
   json_object_set_new(root, "model", json_string_t(m_model));
   json_object_set_new(root, "serialNumber", json_string_t(m_serialNumber));
   json_object_set_new(root, "deviceAddress", json_string_t(m_deviceAddress));
   json_object_set_new(root, "gatewayNode", json_integer(m_gatewayNodeId));
   json_object_set_new(root, "modbusUnitId", json_integer(m_modbusUnitId));
   json_object_set_new(root, "capabilityFlags", json_integer(m_capabilities));
   unlockProperties();

   return root;
}

/**
 * Fill NXCP message
 */
void Sensor::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
	msg->setField(VID_MAC_ADDR, m_macAddress);
   msg->setField(VID_DEVICE_CLASS, static_cast<int16_t>(m_deviceClass));
	msg->setField(VID_VENDOR, CHECK_NULL_EX(m_vendor));
   msg->setField(VID_MODEL, CHECK_NULL_EX(m_model));
	msg->setField(VID_SERIAL_NUMBER, CHECK_NULL_EX(m_serialNumber));
	msg->setField(VID_DEVICE_ADDRESS, CHECK_NULL_EX(m_deviceAddress));
	msg->setField(VID_GATEWAY_NODE, m_gatewayNodeId);
   msg->setField(VID_MODBUS_UNIT_ID, m_modbusUnitId);
   msg->setField(VID_CAPABILITIES, m_capabilities);
}

/**
 * Modify object from NXCP message
 */
uint32_t Sensor::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_MAC_ADDR))
      m_macAddress = msg.getFieldAsMacAddress(VID_MAC_ADDR);
   if (msg.isFieldExist(VID_DEVICE_CLASS))
      m_deviceClass = static_cast<SensorDeviceClass>(msg.getFieldAsInt16(VID_DEVICE_CLASS));
   if (msg.isFieldExist(VID_VENDOR))
      m_vendor = msg.getFieldAsSharedString(VID_VENDOR);
   if (msg.isFieldExist(VID_MODEL))
      m_model = msg.getFieldAsSharedString(VID_MODEL);
   if (msg.isFieldExist(VID_SERIAL_NUMBER))
      m_serialNumber = msg.getFieldAsSharedString(VID_SERIAL_NUMBER);
   if (msg.isFieldExist(VID_DEVICE_ADDRESS))
      m_deviceAddress = msg.getFieldAsSharedString(VID_DEVICE_ADDRESS);
   if (msg.isFieldExist(VID_GATEWAY_NODE))
      m_gatewayNodeId = msg.getFieldAsUInt32(VID_GATEWAY_NODE);
   if (msg.isFieldExist(VID_MODBUS_UNIT_ID))
      m_modbusUnitId = msg.getFieldAsUInt16(VID_MODBUS_UNIT_ID);

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Calculate sensor status
 */
void Sensor::calculateCompoundStatus(bool forcedRecalc)
{
   int oldStatus = m_status;
   super::calculateCompoundStatus(forcedRecalc);
   if (oldStatus != m_status)
      setModified(MODIFY_RUNTIME);
}

/**
 * Get effective source node for given data collection object
 */
uint32_t Sensor::getEffectiveSourceNode(DCObject *dco)
{
   if (dco->getSourceNode() != 0)
      return dco->getSourceNode();
   if ((dco->getDataSource() != DS_INTERNAL) && (dco->getDataSource() != DS_PUSH_AGENT) && (dco->getDataSource() != DS_SCRIPT))
      return m_gatewayNodeId;
   return 0;
}

/**
 * Get instances for instance discovery DCO
 */
StringMap *Sensor::getInstanceList(DCObject *dco)
{
   shared_ptr<Node> sourceNode;
   uint32_t sourceNodeId = getEffectiveSourceNode(dco);
   if (sourceNodeId != 0)
   {
      sourceNode = static_pointer_cast<Node>(FindObjectById(dco->getSourceNode(), OBJECT_NODE));
      if (sourceNode == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 6, _T("Sensor::getInstanceList(%s [%u]): source node [%u] not found"), dco->getName().cstr(), dco->getId(), sourceNodeId);
         return nullptr;
      }
      if ((sourceNodeId != m_gatewayNodeId) && !sourceNode->isTrustedObject(m_id))
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 6, _T("Sensor::getInstanceList(%s [%u]): sensor \"%s\" [%u] is not trusted by node \"%s\" [%u]"),
                  dco->getName().cstr(), dco->getId(), m_name, m_id, sourceNode->getName(), sourceNode->getId());
         return nullptr;
      }
   }

   StringList *instances = nullptr;
   StringMap *instanceMap = nullptr;
   shared_ptr<Table> instanceTable;
   wchar_t tableName[MAX_DB_STRING], nameColumn[MAX_DB_STRING];
   switch(dco->getInstanceDiscoveryMethod())
   {
      case IDM_AGENT_LIST:
         if (sourceNode != nullptr)
            sourceNode->getListFromAgent(dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_AGENT_TABLE:
         if (sourceNode != nullptr)
         {
            parseInstanceDiscoveryTableName(dco->getInstanceDiscoveryData(), tableName, nameColumn);
            sourceNode->getTableFromAgent(tableName, &instanceTable);
         }
         break;
      case IDM_INTERNAL_TABLE:
         parseInstanceDiscoveryTableName(dco->getInstanceDiscoveryData(), tableName, nameColumn);
         if (sourceNode != nullptr)
         {
            sourceNode->getInternalTable(tableName, &instanceTable);
         }
         else
         {
            getInternalTable(tableName, &instanceTable);
         }
         break;
      case IDM_SCRIPT:
         if (sourceNode != nullptr)
         {
            sourceNode->getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         }
         else
         {
            getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         }
         break;
      case IDM_SNMP_WALK_VALUES:
         if (sourceNode != nullptr)
            sourceNode->getListFromSNMP(dco->getSnmpPort(), dco->getSnmpVersion(), dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_SNMP_WALK_OIDS:
         if (sourceNode != nullptr)
            sourceNode->getOIDSuffixListFromSNMP(dco->getSnmpPort(), dco->getSnmpVersion(), dco->getInstanceDiscoveryData(), &instanceMap);
         break;
      case IDM_WEB_SERVICE:
         if (sourceNode != nullptr)
            sourceNode->getListFromWebService(dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_WINPERF:
         if (sourceNode != nullptr)
         {
            TCHAR query[256];
            _sntprintf(query, 256, _T("PDH.ObjectInstances(\"%s\")"), EscapeStringForAgent(dco->getInstanceDiscoveryData()).cstr());
            sourceNode->getListFromAgent(query, &instances);
         }
         break;
      default:
         break;
   }
   if ((instances == nullptr) && (instanceMap == nullptr) && (instanceTable == nullptr))
      return nullptr;

   if (instanceTable != nullptr)
   {
      instanceMap = new StringMap();
      wchar_t buffer[1024];
      int nameColumnIndex = (nameColumn[0] != 0) ? instanceTable->getColumnIndex(nameColumn) : -1;
      for(int i = 0; i < instanceTable->getNumRows(); i++)
      {
         instanceTable->buildInstanceString(i, buffer, 1024);
         if (nameColumnIndex != -1)
         {
            const wchar_t *name = instanceTable->getAsString(i, nameColumnIndex, buffer);
            if (name != nullptr)
               instanceMap->set(buffer, name);
            else
               instanceMap->set(buffer, buffer);
         }
         else
         {
            instanceMap->set(buffer, buffer);
         }
      }
   }
   else if (instanceMap == nullptr)
   {
      instanceMap = new StringMap;
      for(int i = 0; i < instances->size(); i++)
         instanceMap->set(instances->get(i), instances->get(i));
   }
   delete instances;
   return instanceMap;
}

/**
 * Get metric value via Modbus protocol
 */
DataCollectionError Sensor::getMetricFromModbus(const TCHAR *metric, TCHAR *buffer, size_t size)
{
   shared_ptr<Node> gatewayNode = static_pointer_cast<Node>(FindObjectById(m_gatewayNodeId, OBJECT_NODE));
   return (gatewayNode != nullptr) ? gatewayNode->getMetricFromModbus(metric, buffer, size, m_modbusUnitId) : DCE_COMM_ERROR;
}

/**
 * Configuration poll: check for MODBUS
 */
bool Sensor::confPollModbus(uint32_t requestId)
{
   shared_ptr<Node> gatewayNode = static_pointer_cast<Node>(FindObjectById(m_gatewayNodeId, OBJECT_NODE));
   if ((gatewayNode == nullptr) || !gatewayNode->isModbusTCPSupported())
   {
      sendPollerMsg(_T("   Modbus polling is not possible\r\n"));
      return false;
   }

   bool hasChanges = false;

   sendPollerMsg(_T("   Checking Modbus...\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): checking Modbus"), m_name);

   ModbusTransport *transport = gatewayNode->createModbusTransport();
   if (transport != nullptr)
   {
      ModbusOperationStatus status = transport->checkConnection();
      if (status == MODBUS_STATUS_SUCCESS)
      {
         sendPollerMsg(_T("   Device is Modbus capable\r\n"));

         lockProperties();
         m_capabilities |= SC_IS_MODBUS;
         if (m_state & SSF_MODBUS_UNREACHABLE)
         {
            m_state &= ~SSF_MODBUS_UNREACHABLE;
            PostSystemEvent(EVENT_MODBUS_OK, m_id);
            sendPollerMsg(POLLER_INFO _T("   Modbus connectivity restored\r\n"));
         }
         unlockProperties();

         ModbusDeviceIdentification deviceIdentification;
         status = transport->readDeviceIdentification(&deviceIdentification);
         if (status == MODBUS_STATUS_SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): Modbus device identification successfully completed (vendor=\"%s\", product-code=\"%s\", product-name=\"%s\""),
               m_name, deviceIdentification.vendorName, deviceIdentification.productCode, deviceIdentification.productName);
            sendPollerMsg(_T("   Modbus device identification successfully completed\r\n"));
            sendPollerMsg(_T("      Vendor: %s\r\n"), deviceIdentification.vendorName);
            sendPollerMsg(_T("      Product code: %s\r\n"), deviceIdentification.productCode);
            sendPollerMsg(_T("      Product name: %s\r\n"), deviceIdentification.productName);
            sendPollerMsg(_T("      Revision: %s\r\n"), deviceIdentification.revision);

            lockProperties();

            if (_tcscmp(m_vendor, deviceIdentification.vendorName))
            {
               m_vendor = deviceIdentification.vendorName;
               hasChanges = true;
            }

            if (_tcscmp(m_model, deviceIdentification.productName))
            {
               m_model = deviceIdentification.productName;
               hasChanges = true;
            }

            unlockProperties();
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): cannot get device identification data via Modbus (%s)"), m_name, GetModbusStatusText(status));
            sendPollerMsg(_T("   Cannot get device identification data via Modbus (%s)\r\n"), GetModbusStatusText(status));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): device does not respond to Modbus TCP request (%s)"), m_name, GetModbusStatusText(status));
         sendPollerMsg(_T("   Device does not respond to Modbus TCP request (%s)\r\n"), GetModbusStatusText(status));
      }
      delete transport;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): cannot create transport for Modbus"), m_name);
      sendPollerMsg(_T("   Modbus connection is not possible\r\n"));
   }

   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ConfPoll(%s): check for Modbus completed"), m_name);
   return hasChanges;
}

/**
 * Perform configuration poll on node
 */
void Sensor::configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_configurationPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(configuration);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("Starting configuration poll of sensor \"%s\" [%u] (flags = %08x)"), m_name, m_id, m_flags);

   bool hasChanges = false;

   if (confPollModbus(hasChanges))
      hasChanges = true;

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("ConfigurationPoll"));

   poller->setStatus(_T("autobind"));
   if (ConfigReadBoolean(_T("Objects.Sensors.TemplateAutoApply"), false))
      applyTemplates();
   if (ConfigReadBoolean(_T("Objects.Sensors.ContainerAutoBind"), false))
      updateContainerMembership();

   sendPollerMsg(_T("Finished configuration poll of sensor %s\r\n"), m_name);
   sendPollerMsg(_T("Sensor configuration was%schanged after poll\r\n"), hasChanges ? _T(" ") : _T(" not "));

   UpdateAssetLinkage(this);
   autoFillAssetProperties();

   lockProperties();
   m_runtimeFlags &= ~ODF_CONFIGURATION_POLL_PENDING;
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   unlockProperties();

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL,5, _T("Finished configuration poll of sensor \"%s\" [%u]"), m_name, m_id);

   if (hasChanges)
   {
      lockProperties();
      setModified(MODIFY_SENSOR_PROPERTIES);
      unlockProperties();
   }
}

/**
 * Perform status poll on sensor
 */
void Sensor::statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_statusPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(status);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   sendPollerMsg(_T("Starting status poll of sensor %s\r\n"), m_name);
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Starting status poll of sensor \"%s\" [%u]"), m_name, m_id);

   uint32_t prevState = m_state;

   // Check Modbus TCP connectivity
   if (m_capabilities & SC_IS_MODBUS)
   {
      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): checking Modbus"), m_name);
      poller->setStatus(_T("check Modbus"));
      sendPollerMsg(_T("Checking Modbus connectivity\r\n"));

      ModbusOperationStatus status = MODBUS_STATUS_COMM_FAILURE;
      shared_ptr<Node> gatewayNode = static_pointer_cast<Node>(FindObjectById(m_gatewayNodeId, OBJECT_NODE));
      ModbusTransport *transport = (gatewayNode != nullptr) ? gatewayNode->createModbusTransport() : nullptr;
      if ((transport != nullptr) && ((status = transport->checkConnection()) == MODBUS_STATUS_SUCCESS))
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): connected to device via Modbus"), m_name);
         if (m_state & SSF_MODBUS_UNREACHABLE)
         {
            m_state &= ~SSF_MODBUS_UNREACHABLE;
            PostSystemEvent(EVENT_MODBUS_OK, m_id);
            sendPollerMsg(POLLER_INFO _T("Modbus connectivity restored\r\n"));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): Modbus is unreachable (%s)"), m_name, GetModbusStatusText(status));
         sendPollerMsg(POLLER_ERROR _T("Cannot connect to device via Modbus (%s)\r\n"), GetModbusStatusText(status));
         if (!(m_state & SSF_MODBUS_UNREACHABLE))
         {
            m_state |= SSF_MODBUS_UNREACHABLE;
            PostSystemEvent(EVENT_MODBUS_UNREACHABLE, m_id);
         }
      }
      delete transport;

      nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 7, _T("StatusPoll(%s): Modbus check finished"), m_name);
   }

   calculateCompoundStatus(true);

   poller->setStatus(_T("hook"));
   executeHookScript(_T("StatusPoll"));

   lockProperties();
   if (prevState != m_state)
      setModified(MODIFY_SENSOR_PROPERTIES);
   unlockProperties();

   sendPollerMsg(_T("Finished status poll of sensor %s\r\n"), m_name);
   sendPollerMsg(_T("Sensor status after poll is %s\r\n"), GetStatusAsText(m_status, true));

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 5, _T("Finished status poll of sensor \"%s\" [%u]"), m_name, m_id);
}

/**
 * Prepare sensor object for deletion
 */
void Sensor::prepareForDeletion()
{
   // Wait for all pending polls
   nxlog_debug(4, _T("Sensor::PrepareForDeletion(%s [%u]): waiting for outstanding polls to finish"), m_name, m_id);
   while (m_statusPollState.isPending() || m_configurationPollState.isPending())
      ThreadSleepMs(100);
   nxlog_debug(4, _T("Sensor::PrepareForDeletion(%s [%u]): no outstanding polls left"), m_name, m_id);

   super::prepareForDeletion();
}

/**
 * Build internal connection topology - internal function
 */
void Sensor::populateInternalCommunicationTopologyMap(NetworkMapObjectList *map, uint32_t currentObjectId, bool agentConnectionOnly, bool checkAllProxies)
{
   map->addObject(m_id);
   if (m_gatewayNodeId != 0)
   {
      shared_ptr<NetObj> gw = FindObjectById(m_gatewayNodeId, OBJECT_NODE);
      if (gw != nullptr)
      {
         map->addObject(m_gatewayNodeId);
         map->linkObjects(m_id, m_gatewayNodeId, LINK_TYPE_SENSOR_PROXY, _T("Sensor gateway"));
         callPopulateInternalCommunicationTopologyMap(static_cast<DataCollectionTarget*>(gw.get()), map, m_gatewayNodeId, false, checkAllProxies);
      }
   }
}
