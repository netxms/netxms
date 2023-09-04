/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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

/**
 * Default empty Sensor class constructior
 */
Sensor::Sensor() : super(Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_proxyNodeId = 0;
	m_flags = 0;
	m_deviceClass = SENSOR_CLASS_UNKNOWN;
	m_vendor = nullptr;
	m_commProtocol = SENSOR_PROTO_UNKNOWN;
	m_xmlRegConfig = nullptr;
	m_xmlConfig = nullptr;
	m_serialNumber = nullptr;
	m_deviceAddress = nullptr;
	m_metaType = nullptr;
	m_description = nullptr;
	m_lastConnectionTime = 0;
	m_frameCount = 0; //zero when no info
   m_signalStrenght = 1; //+1 when no information(cannot be +)
   m_signalNoise = MAX_INT32; //*10 from origin number
   m_frequency = 0; //*10 from origin number
   m_status = STATUS_UNKNOWN;
}

/**
 * Constructor with all fields for Sensor class
 */
Sensor::Sensor(const TCHAR *name, const NXCPMessage& request) : super(name, Pollable::STATUS | Pollable::CONFIGURATION)
{
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PENDING;
   m_flags = request.getFieldAsUInt32(VID_SENSOR_FLAGS);
   m_macAddress = request.getFieldAsMacAddress(VID_MAC_ADDR);
	m_deviceClass = request.getFieldAsUInt32(VID_DEVICE_CLASS);
	m_vendor = request.getFieldAsString(VID_VENDOR);
	m_commProtocol = request.getFieldAsUInt32(VID_COMM_PROTOCOL);
	m_xmlRegConfig = request.getFieldAsString(VID_XML_REG_CONFIG);
	m_xmlConfig = request.getFieldAsString(VID_XML_CONFIG);
	m_serialNumber = request.getFieldAsString(VID_SERIAL_NUMBER);
	m_deviceAddress = request.getFieldAsString(VID_DEVICE_ADDRESS);
	m_metaType = request.getFieldAsString(VID_META_TYPE);
	m_description = request.getFieldAsString(VID_DESCRIPTION);
	m_proxyNodeId = request.getFieldAsUInt32(VID_SENSOR_PROXY);
   m_lastConnectionTime = 0;
	m_frameCount = 0; //zero when no info
   m_signalStrenght = 1; //+1 when no information(cannot be +)
   m_signalNoise = MAX_INT32; //*10 from origin number
   m_frequency = 0; //*10 from origin number
   m_status = STATUS_UNKNOWN;
}

/**
 * Create new sensor
 */
shared_ptr<Sensor> Sensor::create(const TCHAR *name, const NXCPMessage& request)
{
   shared_ptr<Sensor> sensor = make_shared<Sensor>(name, request);
   sensor->generateGuid();
   switch(request.getFieldAsUInt32(VID_COMM_PROTOCOL))
   {
      case SENSOR_PROTO_LORAWAN:
         if (!registerLoraDevice(sensor.get()))
            return shared_ptr<Sensor>();
         break;
      case SENSOR_PROTO_DLMS:
         sensor->m_state = SSF_PROVISIONED | SSF_REGISTERED | SSF_ACTIVE;
         break;
   }
   return sensor;
}

/**
 * Create agent connection
 */
shared_ptr<AgentConnectionEx> Sensor::getAgentConnection()
{
   if (IsShutdownInProgress())
      return shared_ptr<AgentConnectionEx>();

   shared_ptr<NetObj> proxy = FindObjectById(m_proxyNodeId, OBJECT_NODE);
   return (proxy != nullptr) ? static_cast<Node&>(*proxy).acquireProxyConnection(SENSOR_PROXY) : shared_ptr<AgentConnectionEx>();
}

/**
 * Register LoRa WAN device
 */
bool Sensor::registerLoraDevice(Sensor *sensor)
{
   shared_ptr<NetObj> proxy = FindObjectById(sensor->m_proxyNodeId, OBJECT_NODE);
   if (proxy == nullptr)
      return false;

   shared_ptr<AgentConnectionEx> conn = sensor->getAgentConnection();
   if (conn == nullptr)
      return true; // Unprovisioned sensor - will try to provision it on next connect

   Config regConfig;
   Config config;
#ifdef UNICODE
   char *regXml = UTF8StringFromWideString(sensor->getXmlRegConfig());
   regConfig.loadXmlConfigFromMemory(regXml, (UINT32)strlen(regXml), nullptr, "config", false);
   MemFree(regXml);

   char *xml = UTF8StringFromWideString(sensor->getXmlConfig());
   config.loadXmlConfigFromMemory(xml, (UINT32)strlen(xml), nullptr, "config", false);
   MemFree(xml);
#else
   regConfig.loadXmlConfigFromMemory(sensor->getXmlRegConfig(), (UINT32)strlen(sensor->getXmlRegConfig()), nullptr, "config", false);
   config.loadXmlConfigFromMemory(sensor->getXmlConfig(), (UINT32)strlen(sensor->getXmlConfig()), nullptr, "config", false);
#endif

   NXCPMessage msg(conn->getProtocolVersion());
   msg.setCode(CMD_REGISTER_LORAWAN_SENSOR);
   msg.setId(conn->generateRequestId());
   msg.setField(VID_DEVICE_ADDRESS, sensor->getDeviceAddress());
   msg.setField(VID_MAC_ADDR, sensor->getMacAddress());
   msg.setField(VID_GUID, sensor->getGuid());
   msg.setField(VID_DECODER, config.getValueAsInt(_T("/decoder"), 0));
   msg.setField(VID_REG_TYPE, regConfig.getValueAsInt(_T("/registrationType"), 0));
   if (regConfig.getValueAsInt(_T("/registrationType"), 0) == 0)
   {
      msg.setField(VID_LORA_APP_EUI, regConfig.getValue(_T("/appEUI")));
      msg.setField(VID_LORA_APP_KEY, regConfig.getValue(_T("/appKey")));
   }
   else
   {
      msg.setField(VID_LORA_APP_S_KEY, regConfig.getValue(_T("/appSKey")));
      msg.setField(VID_LORA_NWK_S_KWY, regConfig.getValue(_T("/nwkSKey")));
   }

   NXCPMessage *response = conn->customRequest(&msg);
   if (response != nullptr)
   {
      if (response->getFieldAsUInt32(VID_RCC) == RCC_SUCCESS)
      {
         sensor->lockProperties();
         sensor->setProvisoned();
         sensor->unlockProperties();
      }
      delete response;
   }

   return true;
}

//set correct status calculation function
//set correct configuration poll - provision if possible, for lora get device name, get all possible DCI's, try to do provisionning
//set status poll - check if connection is on if not generate alarm, check, that proxy is up and running

/**
 * Sensor class destructor
 */
Sensor::~Sensor()
{
   MemFree(m_vendor);
   MemFree(m_xmlRegConfig);
   MemFree(m_xmlConfig);
   MemFree(m_serialNumber);
   MemFree(m_deviceAddress);
   MemFree(m_metaType);
   MemFree(m_description);
}

/**
 * Load from database SensorDevice class
 */
bool Sensor::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   m_id = id;

   if (!loadCommonProperties(hdb) || !super::loadFromDatabase(hdb, id))
      return false;

   if (Pollable::loadFromDatabase(hdb, m_id))
   {
      if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) < getCustomAttributeAsUInt32(_T("SysConfig:Objects.ConfigurationPollingInterval"), g_configurationPollingInterval))
         m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   }

	TCHAR query[512];
	_sntprintf(query, 512, _T("SELECT mac_address,device_class,vendor,communication_protocol,xml_config,serial_number,device_address,")
                          _T("meta_type,description,last_connection_time,frame_count,signal_strenght,signal_noise,frequency,proxy_node,xml_reg_config FROM sensors WHERE id=%d"), m_id);
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult == nullptr)
		return false;

   m_macAddress = DBGetFieldMacAddr(hResult, 0, 0);
	m_deviceClass = DBGetFieldULong(hResult, 0, 1);
	m_vendor = DBGetField(hResult, 0, 2, nullptr, 0);
	m_commProtocol = DBGetFieldULong(hResult, 0, 3);
	m_xmlConfig = DBGetField(hResult, 0, 4, nullptr, 0);
	m_serialNumber = DBGetField(hResult, 0, 5, nullptr, 0);
	m_deviceAddress = DBGetField(hResult, 0, 6, nullptr, 0);
	m_metaType = DBGetField(hResult, 0, 7, nullptr, 0);
	m_description = DBGetField(hResult, 0, 8, nullptr, 0);
   m_lastConnectionTime = DBGetFieldULong(hResult, 0, 9);
   m_frameCount = DBGetFieldULong(hResult, 0, 10);
   m_signalStrenght = DBGetFieldLong(hResult, 0, 11);
   m_signalNoise = DBGetFieldLong(hResult, 0, 12);
   m_frequency = DBGetFieldLong(hResult, 0, 13);
   m_proxyNodeId = DBGetFieldLong(hResult, 0, 14);
   m_xmlRegConfig = DBGetField(hResult, 0, 15, nullptr, 0);
   DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB(hdb);
   loadItemsFromDB(hdb);
   for(int i = 0; i < m_dcObjects.size(); i++)
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb))
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
      DB_STATEMENT hStmt;
      bool isNew = !(IsDatabaseRecordExist(hdb, _T("sensors"), _T("id"), m_id));
      if (isNew)
         hStmt = DBPrepare(hdb, _T("INSERT INTO sensors (mac_address,device_class,vendor,communication_protocol,xml_config,serial_number,device_address,meta_type,description,last_connection_time,frame_count,signal_strenght,signal_noise,frequency,proxy_node,id,xml_reg_config) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
      else
         hStmt = DBPrepare(hdb, _T("UPDATE sensors SET mac_address=?,device_class=?,vendor=?,communication_protocol=?,xml_config=?,serial_number=?,device_address=?,meta_type=?,description=?,last_connection_time=?,frame_count=?,signal_strenght=?,signal_noise=?,frequency=?,proxy_node=? WHERE id=?"));
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_macAddress);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_deviceClass);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_commProtocol);
         DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_xmlConfig, DB_BIND_STATIC);
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_serialNumber, DB_BIND_STATIC);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_deviceAddress, DB_BIND_STATIC);
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_metaType, DB_BIND_STATIC);
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (UINT32)m_lastConnectionTime);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_frameCount);
         DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_signalStrenght);
         DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_signalNoise);
         DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_frequency);
         DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_proxyNodeId);
         DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_id);
         if (isNew)
            DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, m_xmlRegConfig, DB_BIND_STATIC);

         success = DBExecute(hStmt);

         DBFreeStatement(hStmt);
         unlockProperties();
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
   json_object_set_new(root, "macAddr", json_string_t(m_macAddress.toString(MacAddressNotation::FLAT_STRING)));
   json_object_set_new(root, "deviceClass", json_integer(m_deviceClass));
   json_object_set_new(root, "vendor", json_string_t(m_vendor));
   json_object_set_new(root, "commProtocol", json_integer(m_commProtocol));
   json_object_set_new(root, "xmlConfig", json_string_t(m_xmlConfig));
   json_object_set_new(root, "serialNumber", json_string_t(m_serialNumber));
   json_object_set_new(root, "deviceAddress", json_string_t(m_deviceAddress));
   json_object_set_new(root, "metaType", json_string_t(m_metaType));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "proxyNode", json_integer(m_proxyNodeId));
   unlockProperties();

   return root;
}

/**
 * Fill NXCP message
 */
void Sensor::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageInternal(msg, userId);
	msg->setField(VID_MAC_ADDR, m_macAddress);
   msg->setField(VID_DEVICE_CLASS, m_deviceClass);
	msg->setField(VID_VENDOR, CHECK_NULL_EX(m_vendor));
   msg->setField(VID_COMM_PROTOCOL, m_commProtocol);
	msg->setField(VID_XML_CONFIG, CHECK_NULL_EX(m_xmlConfig));
	msg->setField(VID_XML_REG_CONFIG, CHECK_NULL_EX(m_xmlRegConfig));
	msg->setField(VID_SERIAL_NUMBER, CHECK_NULL_EX(m_serialNumber));
	msg->setField(VID_DEVICE_ADDRESS, CHECK_NULL_EX(m_deviceAddress));
	msg->setField(VID_META_TYPE, CHECK_NULL_EX(m_metaType));
	msg->setField(VID_DESCRIPTION, CHECK_NULL_EX(m_description));
	msg->setFieldFromTime(VID_LAST_CONN_TIME, m_lastConnectionTime);
	msg->setField(VID_FRAME_COUNT, m_frameCount);
	msg->setField(VID_SIGNAL_STRENGHT, m_signalStrenght);
	msg->setField(VID_SIGNAL_NOISE, m_signalNoise);
	msg->setField(VID_FREQUENCY, m_frequency);
	msg->setField(VID_SENSOR_PROXY, m_proxyNodeId);
}

/**
 * Modify object from NXCP message
 */
uint32_t Sensor::modifyFromMessageInternal(const NXCPMessage& msg)
{
   if (msg.isFieldExist(VID_MAC_ADDR))
      m_macAddress = msg.getFieldAsMacAddress(VID_MAC_ADDR);
   if (msg.isFieldExist(VID_VENDOR))
   {
      free(m_vendor);
      m_vendor = msg.getFieldAsString(VID_VENDOR);
   }
   if (msg.isFieldExist(VID_DEVICE_CLASS))
      m_deviceClass = msg.getFieldAsUInt32(VID_DEVICE_CLASS);
   if (msg.isFieldExist(VID_SERIAL_NUMBER))
   {
      free(m_serialNumber);
      m_serialNumber = msg.getFieldAsString(VID_SERIAL_NUMBER);
   }
   if (msg.isFieldExist(VID_DEVICE_ADDRESS))
   {
      free(m_deviceAddress);
      m_deviceAddress = msg.getFieldAsString(VID_DEVICE_ADDRESS);
   }
   if (msg.isFieldExist(VID_META_TYPE))
   {
      free(m_metaType);
      m_metaType = msg.getFieldAsString(VID_META_TYPE);
   }
   if (msg.isFieldExist(VID_DESCRIPTION))
   {
      free(m_description);
      m_description = msg.getFieldAsString(VID_DESCRIPTION);
   }
   if (msg.isFieldExist(VID_SENSOR_PROXY))
      m_proxyNodeId = msg.getFieldAsUInt32(VID_SENSOR_PROXY);
   if (msg.isFieldExist(VID_XML_CONFIG))
   {
      free(m_xmlConfig);
      m_xmlConfig = msg.getFieldAsString(VID_XML_CONFIG);
   }

   return super::modifyFromMessageInternal(msg);
}

/**
 * Calculate sensor status
 */
void Sensor::calculateCompoundStatus(bool forcedRecalc)
{
   int oldStatus = m_status;
   calculateStatus(forcedRecalc);
   if (oldStatus != m_status)
      setModified(MODIFY_RUNTIME);
}

/**
 * Calculate sensor status
 */
void Sensor::calculateStatus(BOOL bForcedRecalc)
{
   shared_ptr<AgentConnectionEx> conn = getAgentConnection();
   if (conn == nullptr)
   {
      m_status = STATUS_UNKNOWN;
      return;
   }
   super::calculateCompoundStatus(bForcedRecalc);
   lockProperties();
   int status = 0;
   if (m_state == 0 || m_state == SSF_PROVISIONED)
      status = STATUS_UNKNOWN;
   else if (m_state & SSF_ACTIVE)
      status = STATUS_NORMAL;
   else
      status = STATUS_CRITICAL;

   m_status = m_status != STATUS_UNKNOWN ? std::max(m_status, status) : status;
   unlockProperties();
}

/**
 * Get instances for instance discovery DCO
 */
StringMap *Sensor::getInstanceList(DCObject *dco)
{
   if (dco->getInstanceDiscoveryData() == nullptr)
      return nullptr;

   shared_ptr<NetObj> proxyNode;
   DataCollectionTarget *dataSourceObject;
   if (dco->getSourceNode() != 0)
   {
      proxyNode = FindObjectById(dco->getSourceNode(), OBJECT_NODE);
      if (proxyNode == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 6, _T("Sensor::getInstanceList(%s [%d]): source node [%d] not found"), dco->getName().cstr(), dco->getId(), dco->getSourceNode());
         return nullptr;
      }
      dataSourceObject = static_cast<DataCollectionTarget*>(proxyNode.get());
      if (!dataSourceObject->isTrustedObject(m_id))
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 6, _T("Sensor::getInstanceList(%s [%d]): this node (%s [%d]) is not trusted by source sensor %s [%d]"),
                  dco->getName().cstr(), dco->getId(), m_name, m_id, dataSourceObject->getName(), dataSourceObject->getId());
         return nullptr;
      }
   }
   else
   {
      dataSourceObject = this;
   }

   StringList *instances = nullptr;
   StringMap *instanceMap = nullptr;
   shared_ptr<Table> instanceTable;
   switch(dco->getInstanceDiscoveryMethod())
   {
      case IDM_AGENT_LIST:
         if (dataSourceObject->getObjectClass() == OBJECT_NODE)
            static_cast<Node*>(dataSourceObject)->getListFromAgent(dco->getInstanceDiscoveryData(), &instances);
         else if (dataSourceObject->getObjectClass() == OBJECT_SENSOR)
            static_cast<Sensor*>(dataSourceObject)->getListFromAgent(dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_AGENT_TABLE:
      case IDM_INTERNAL_TABLE:
         if (dataSourceObject->getObjectClass() == OBJECT_NODE)
         {
            if (dco->getInstanceDiscoveryMethod() == IDM_AGENT_TABLE)
               static_cast<Node*>(dataSourceObject)->getTableFromAgent(dco->getInstanceDiscoveryData(), &instanceTable);
            else
               static_cast<Node*>(dataSourceObject)->getInternalTable(dco->getInstanceDiscoveryData(), &instanceTable);
         }
         else if (dataSourceObject->getObjectClass() == OBJECT_SENSOR)
         {
            if (dco->getInstanceDiscoveryMethod() == IDM_AGENT_TABLE)
               static_cast<Sensor*>(dataSourceObject)->getTableFromAgent(dco->getInstanceDiscoveryData(), &instanceTable);
            else
               static_cast<Sensor*>(dataSourceObject)->getInternalTable(dco->getInstanceDiscoveryData(), &instanceTable);
         }
         if (instanceTable != nullptr)
         {
            TCHAR buffer[1024];
            instances = new StringList();
            for(int i = 0; i < instanceTable->getNumRows(); i++)
            {
               instanceTable->buildInstanceString(i, buffer, 1024);
               instances->add(buffer);
            }
         }
         break;
      case IDM_SCRIPT:
         dataSourceObject->getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         break;
      default:
         instances = nullptr;
         break;
   }
   if ((instances == nullptr) && (instanceMap == nullptr))
      return nullptr;

   if (instanceMap == nullptr)
   {
      instanceMap = new StringMap;
      for(int i = 0; i < instances->size(); i++)
         instanceMap->set(instances->get(i), instances->get(i));
   }
   delete instances;
   return instanceMap;
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
   nxlog_debug(5, _T("Starting configuration poll of sensor %s (ID: %d), m_flags: %d"), m_name, m_id, m_flags);

   bool hasChanges = false;

   if (m_commProtocol == SENSOR_PROTO_LORAWAN)
   {
      poller->setStatus(_T("lorawan"));
      if (!(m_state & SSF_PROVISIONED))
      {
         if (registerLoraDevice(this) && (m_state & SSF_PROVISIONED))
         {
            nxlog_debug(6, _T("ConfPoll(%s [%d}): sensor successfully registered"), m_name, m_id);
            hasChanges = true;
         }
      }
      if ((m_state & SSF_PROVISIONED) && (m_deviceAddress == nullptr))
      {
         TCHAR buffer[MAX_RESULT_LENGTH];
         if (getMetricFromAgent(_T("LoraWAN.DevAddr(*)"), buffer, MAX_RESULT_LENGTH) == DCE_SUCCESS)
         {
            m_deviceAddress = MemCopyString(buffer);
            nxlog_debug(6, _T("ConfPoll(%s [%d}): sensor DevAddr[%s] successfully obtained"), m_name, m_id, m_deviceAddress);
            hasChanges = true;
         }
      }
   }

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
   nxlog_debug(5, _T("Finished configuration poll of sensor %s (ID: %d)"), m_name, m_id);

   if (hasChanges)
   {
      lockProperties();
      setModified(MODIFY_SENSOR_PROPERTIES);
      unlockProperties();
   }
}

/**
 * Check if DLMS converter used for sensor access is accessible
 */
void Sensor::checkDlmsConverterAccessibility()
{
   //Create connectivity test DCI and try to get it's result
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
   nxlog_debug(5, _T("Starting status poll of sensor %s (ID: %d)"), m_name, m_id);

   UINT32 prevState = m_state;

   nxlog_debug(6, _T("StatusPoll(%s): checking agent"), m_name);
   poller->setStatus(_T("check agent"));
   sendPollerMsg(_T("Checking NetXMS agent connectivity\r\n"));

   shared_ptr<AgentConnectionEx> conn = getAgentConnection();
   lockProperties();
   if (conn != nullptr)
   {
      nxlog_debug(6, _T("StatusPoll(%s): connected to agent"), m_name);
      if (m_state & DCSF_UNREACHABLE)
      {
         m_state &= ~DCSF_UNREACHABLE;
         sendPollerMsg(POLLER_INFO _T("Connectivity with NetXMS agent restored\r\n"));
      }
   }
   else
   {
      nxlog_debug(6, _T("StatusPoll(%s): agent unreachable"), m_name);
      sendPollerMsg(POLLER_ERROR _T("NetXMS agent unreachable\r\n"));
      if (!(m_state & DCSF_UNREACHABLE))
         m_state |= DCSF_UNREACHABLE;
   }
   unlockProperties();
   nxlog_debug(6, _T("StatusPoll(%s): agent check finished"), m_name);

   switch(m_commProtocol)
   {
      case SENSOR_PROTO_LORAWAN:
         if (m_runtimeFlags & SSF_PROVISIONED)
         {
            lockProperties();
            TCHAR lastValue[MAX_DCI_STRING_VALUE] = { 0 };
            time_t now;
            getMetricFromAgent(_T("LoraWAN.LastContact(*)"), lastValue, MAX_DCI_STRING_VALUE);
            time_t lastConnectionTime = _tcstol(lastValue, nullptr, 0);
            if (lastConnectionTime != 0)
            {
               m_lastConnectionTime = lastConnectionTime;
               nxlog_debug(6, _T("StatusPoll(%s [%d}): Last connection time updated - %d"), m_name, m_id, m_lastConnectionTime);
            }

            now = time(nullptr);

            if (!(m_state & SSF_REGISTERED))
            {
               if (m_lastConnectionTime > 0)
               {
                  m_state |= SSF_REGISTERED;
                  nxlog_debug(6, _T("StatusPoll(%s [%d}): Status set to REGISTERED"), m_name, m_id);
               }
            }
            if (m_state & SSF_REGISTERED)
            {
               if (now - m_lastConnectionTime > 3600) // Last contact > 1h
               {
                  m_state &= ~SSF_ACTIVE;
                  nxlog_debug(6, _T("StatusPoll(%s [%d}): Inactive for over 1h, status set to INACTIVE"), m_name, m_id);
               }
               else
               {
                  // FIXME: modify runtime if needed
                  m_state |= SSF_ACTIVE;
                  nxlog_debug(6, _T("StatusPoll(%s [%d]): Status set to ACTIVE"), m_name, m_id);
                  getMetricFromAgent(_T("LoraWAN.RSSI"), lastValue, MAX_DCI_STRING_VALUE);
                  m_signalStrenght = _tcstol(lastValue, nullptr, 10);
                  getMetricFromAgent(_T("LoraWAN.SNR"), lastValue, MAX_DCI_STRING_VALUE);
                  m_signalNoise = static_cast<INT32>(_tcstod(lastValue, nullptr) * 10);
                  getMetricFromAgent(_T("LoraWAN.Frequency"), lastValue, MAX_DCI_STRING_VALUE);
                  m_frequency = static_cast<UINT32>(_tcstod(lastValue, nullptr) * 10);
               }
            }

            unlockProperties();
         }
         break;
      case SENSOR_PROTO_DLMS:
         checkDlmsConverterAccessibility();
         break;
      default:
         break;
   }
   calculateStatus(TRUE);

   poller->setStatus(_T("hook"));
   executeHookScript(_T("StatusPoll"));

   lockProperties();
   if (prevState != m_state)
      setModified(MODIFY_SENSOR_PROPERTIES);
   unlockProperties();

   sendPollerMsg(_T("Finished status poll of sensor %s\r\n"), m_name);
   sendPollerMsg(_T("Sensor status after poll is %s\r\n"), GetStatusAsText(m_status, true));

   pollerUnlock();
   nxlog_debug(5, _T("Finished status poll of sensor %s (ID: %d)"), m_name, m_id);
}

/**
 * Set all required parameters for LoRaWAN request
 */
void Sensor::prepareLoraDciParameters(StringBuffer &parameter)
{
   if (parameter.find(_T(")")) > 0)
   {
      parameter.replace(_T(")"), m_guid.toString());
      parameter.append(_T(")"));
   }
   else
   {
      parameter.append(_T("("));
      parameter.append(m_guid.toString());
      parameter.append(_T(")"));
   }
}

/**
 * Set all required parameters for DLMS request
 */
void Sensor::prepareDlmsDciParameters(StringBuffer &parameter)
{
   Config config;
#ifdef UNICODE
   char *xml = UTF8StringFromWideString(m_xmlConfig);
   config.loadXmlConfigFromMemory(xml, strlen(xml), nullptr, "config", false);
   free(xml);
#else
   config.loadXmlConfigFromMemory(m_xmlConfig, strlen(m_xmlConfig), nullptr, "config", false);
#endif
   ConfigEntry *configRoot = config.getEntry(_T("/connections"));
	if (configRoot != nullptr)
	{
	   if (parameter.find(_T(")")) > 0)
	   {
	      parameter.replace(_T(")"), _T(""));
	   }
	   else
	   {
	      parameter.append(_T("("));
	   }
      unique_ptr<ObjectArray<ConfigEntry>> credentials = configRoot->getSubEntries(_T("/cred"));
      for (int i = 0; i < credentials->size(); i++)
      {
			ConfigEntry *cred = credentials->get(i);
         parameter.append(_T(","));
         parameter.append(cred->getSubEntryValueAsInt(_T("/lineType")));
         parameter.append(_T(","));
         parameter.append(cred->getSubEntryValueAsInt(_T("/port")));
         parameter.append(_T(","));
         parameter.append(cred->getSubEntryValueAsInt(_T("/password")));
         parameter.append(_T(","));
         parameter.append(cred->getSubEntryValue(_T("/inetAddress")));
         parameter.append(_T(","));
         parameter.append(cred->getSubEntryValueAsInt(_T("/linkNumber")));
         parameter.append(_T(","));
         parameter.append(cred->getSubEntryValueAsInt(_T("/lineNumber")));
         parameter.append(_T(","));
         parameter.append(cred->getSubEntryValueAsInt(_T("/linkParams")));
		}
      parameter.append(_T(")"));
   }

   /*
   config.
   //set number of configurations
   //set all parameters
<config>
   <connections class="java.util.ArrayList">
      <cred>
         <lineType>32</lineType>
         <port>3001</port>
         <password>ABCD</password>
         <inetAddress>127.0.0.1</inetAddress>
         <linkNumber>54</linkNumber>
         <lineNumber>31</lineNumber>
         <linkParams>1231</linkParams>
      </cred>
  </connections>
</config>
   */
}

/**
 * Get item's value via native agent
 */
DataCollectionError Sensor::getMetricFromAgent(const TCHAR *name, TCHAR *buffer, size_t bufferSize)
{
   if (m_state & DCSF_UNREACHABLE)
      return DCE_COMM_ERROR;

   uint32_t agentError = ERR_NOT_CONNECTED;
   DataCollectionError result = DCE_COMM_ERROR;
   int retry = 3;

   nxlog_debug(7, _T("Sensor(%s)->getMetricFromAgent(%s)"), m_name, name);
   // Establish connection if needed
   shared_ptr<AgentConnectionEx> conn = getAgentConnection();
   if (conn == nullptr)
      return result;

   StringBuffer parameter(name);
   switch(m_commProtocol)
   {
      case SENSOR_PROTO_LORAWAN:
         prepareLoraDciParameters(parameter);
         break;
      case SENSOR_PROTO_DLMS:
         if (parameter.find(_T("Sensor")) != -1)
            prepareDlmsDciParameters(parameter);
         break;
   }
   nxlog_debug(3, _T("Sensor(%s)->getMetricFromAgent(%s)"), m_name, parameter.cstr());

   // Get parameter from agent
   while(retry-- > 0)
   {
      agentError = conn->getParameter(parameter, buffer, bufferSize);
      switch(agentError)
      {
         case ERR_SUCCESS:
            result = DCE_SUCCESS;
            break;
         case ERR_UNKNOWN_METRIC:
         case ERR_UNSUPPORTED_METRIC:
            result = DCE_NOT_SUPPORTED;
            break;
         case ERR_NO_SUCH_INSTANCE:
            result = DCE_NO_SUCH_INSTANCE;
            break;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
            break;
         case ERR_REQUEST_TIMEOUT:
            // Reset connection to agent after timeout
            nxlog_debug(7, _T("Sensor(%s)->getMetricFromAgent(%s): timeout; resetting connection to agent..."), m_name, name);
            if (getAgentConnection() == nullptr)
               break;
            nxlog_debug(7, _T("Sensor(%s)->getMetricFromAgent(%s): connection to agent restored successfully"), m_name, name);
            break;
         case ERR_INTERNAL_ERROR:
            result = DCE_COLLECTION_ERROR;
            break;
      }
   }

   nxlog_debug(7, _T("Sensor(%s)->getMetricFromAgent(%s): agentError=%d result=%d"), m_name, name, agentError, result);
   return result;
}

/**
 * Get list from agent
 */
DataCollectionError Sensor::getListFromAgent(const TCHAR *name, StringList **list)
{
   *list = nullptr;

   if (m_state & DCSF_UNREACHABLE) //removed disable agent usage for all polls
      return DCE_COMM_ERROR;

   nxlog_debug(7, _T("Sensor(%s)->getListFromAgent(%s)"), m_name, name);
   shared_ptr<AgentConnectionEx> conn = getAgentConnection();
   if (conn == nullptr)
      return DCE_COMM_ERROR;

   StringBuffer parameter(name);
   switch(m_commProtocol)
   {
      case SENSOR_PROTO_LORAWAN:
         prepareLoraDciParameters(parameter);
         break;
      case SENSOR_PROTO_DLMS:
         if (parameter.find(_T("Sensor")) != -1)
            prepareDlmsDciParameters(parameter);
         break;
   }
   nxlog_debug(3, _T("Sensor(%s)->getListFromAgent(%s)"), m_name, parameter.getBuffer());

   // Get parameter from agent
   DataCollectionError result = DCE_COMM_ERROR;
   uint32_t agentError = ERR_NOT_CONNECTED;
   int retryCount = 3;
   while(retryCount-- > 0)
   {
      agentError = conn->getList(parameter, list);
      switch(agentError)
      {
         case ERR_SUCCESS:
            result = DCE_SUCCESS;
            break;
         case ERR_UNKNOWN_METRIC:
         case ERR_UNSUPPORTED_METRIC:
            result = DCE_NOT_SUPPORTED;
            break;
         case ERR_NO_SUCH_INSTANCE:
            result = DCE_NO_SUCH_INSTANCE;
            break;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
         case ERR_REQUEST_TIMEOUT:
            // Reset connection to agent after timeout
            nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Sensor(%s)->getListFromAgent(%s): timeout; resetting connection to agent..."), m_name, name);
            if (getAgentConnection() == nullptr)
               break;
            nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Sensor(%s)->getListFromAgent(%s): connection to agent restored successfully"), m_name, name);
            break;
         case ERR_INTERNAL_ERROR:
            result = DCE_COLLECTION_ERROR;
            break;
      }
   }

   nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Sensor(%s)->getListFromAgent(%s): agentError=%d result=%d"), m_name, name, agentError, result);
   return result;
}

/**
 * Get table from agent
 */
DataCollectionError Sensor::getTableFromAgent(const TCHAR *name, shared_ptr<Table> *table)
{
   uint32_t error = ERR_NOT_CONNECTED;

   if (m_state & DCSF_UNREACHABLE) //removed disable agent usage for all polls
      return DCE_COMM_ERROR;

   nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Sensor(%s)->getTableFromAgent(%s)"), m_name, name);
   shared_ptr<AgentConnectionEx> conn = getAgentConnection();
   if (conn == nullptr)
      return DCE_COMM_ERROR;

   StringBuffer parameter(name);
   switch(m_commProtocol)
   {
      case SENSOR_PROTO_LORAWAN:
         prepareLoraDciParameters(parameter);
         break;
      case SENSOR_PROTO_DLMS:
         if (parameter.find(_T("Sensor")) != -1)
            prepareDlmsDciParameters(parameter);
         break;
   }

   // Get parameter from agent
   DataCollectionError result = DCE_COMM_ERROR;
   int retries = 3;
   while(retries-- > 0)
   {
      Table *tableObject;
      error = conn->getTable(parameter, &tableObject);
      switch(error)
      {
         case ERR_SUCCESS:
            result = DCE_SUCCESS;
            *table = shared_ptr<Table>(tableObject);
            break;
         case ERR_UNKNOWN_METRIC:
         case ERR_UNSUPPORTED_METRIC:
            result = DCE_NOT_SUPPORTED;
            break;
         case ERR_NO_SUCH_INSTANCE:
            result = DCE_NO_SUCH_INSTANCE;
            break;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
         case ERR_REQUEST_TIMEOUT:
            // Reset connection to agent after timeout
            nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Sensor(%s)->getTableFromAgent(%s): timeout; resetting connection to agent..."), m_name, name);
            if (getAgentConnection() == nullptr)
               break;
            nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Sensor(%s)->getTableFromAgent(%s): connection to agent restored successfully"), m_name, name);
            break;
         case ERR_INTERNAL_ERROR:
            result = DCE_COLLECTION_ERROR;
            break;
      }
   }

   nxlog_debug_tag(DEBUG_TAG_DC_AGENT, 7, _T("Sensor(%s)->getTableFromAgent(%s): error=%d result=%d"), m_name, name, error, result);
   return result;
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

   shared_ptr<AgentConnectionEx> conn = getAgentConnection();
   if ((m_commProtocol == SENSOR_PROTO_LORAWAN) && (conn != nullptr))
   {
      NXCPMessage request(CMD_UNREGISTER_LORAWAN_SENSOR, conn->generateRequestId(), conn->getProtocolVersion());
      request.setField(VID_GUID, m_guid);
      NXCPMessage *response = conn->customRequest(&request);
      if (response != nullptr)
      {
         if (response->getFieldAsUInt32(VID_RCC) == RCC_SUCCESS)
            nxlog_debug(4, _T("Sensor::PrepareForDeletion(%s [%u]): successfully unregistered from LoRaWAN server"), m_name, m_id);
         delete response;
      }
   }

   super::prepareForDeletion();
}

/**
 * Build internal connection topology
 */
unique_ptr<NetworkMapObjectList> Sensor::buildInternalCommunicationTopology()
{
   auto topology = make_unique<NetworkMapObjectList>();
   topology->setAllowDuplicateLinks(true);
   buildInternalCommunicationTopologyInternal(topology.get(), false);
   return topology;
}

/**
 * Build internal connection topology - internal function
 */
void Sensor::buildInternalCommunicationTopologyInternal(NetworkMapObjectList *topology, bool checkAllProxies)
{
   topology->addObject(m_id);

   if (m_proxyNodeId != 0)
   {
      shared_ptr<NetObj> proxy = FindObjectById(m_proxyNodeId, OBJECT_NODE);
      if (proxy != nullptr)
      {
         topology->addObject(m_proxyNodeId);
         topology->linkObjects(m_id, m_proxyNodeId, LINK_TYPE_SENSOR_PROXY, _T("Sensor proxy"));
         static_cast<Node&>(*proxy).buildInternalCommunicationTopologyInternal(topology, m_proxyNodeId, false, checkAllProxies);
      }
   }
}
