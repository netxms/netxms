/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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

/**
 * Default empty Sensor class constructior
 */
Sensor::Sensor() : super()
{
   m_proxyNodeId = 0;
	m_flags = 0;
	m_deviceClass = SENSOR_CLASS_UNKNOWN;
	m_vendor = NULL;
	m_commProtocol = SENSOR_PROTO_UNKNOWN;
	m_xmlRegConfig = NULL;
	m_xmlConfig = NULL;
	m_serialNumber = NULL;
	m_deviceAddress = NULL;
	m_metaType = NULL;
	m_description = NULL;
	m_lastConnectionTime = 0;
	m_frameCount = 0; //zero when no info
   m_signalStrenght = 1; //+1 when no information(cannot be +)
   m_signalNoise = MAX_INT32; //*10 from origin number
   m_frequency = 0; //*10 from origin number
   m_lastStatusPoll = 0;
   m_lastConfigurationPoll = 0;
   m_status = STATUS_UNKNOWN;
}

/**
 * Constructor with all fields for Sensor class
 */
Sensor::Sensor(TCHAR *name, UINT32 flags, MacAddress macAddress, UINT32 deviceClass, TCHAR *vendor,
               UINT32 commProtocol, TCHAR *xmlRegConfig, TCHAR *xmlConfig, TCHAR *serialNumber, TCHAR *deviceAddress,
               TCHAR *metaType, TCHAR *description, UINT32 proxyNode) : super(name)
{
   m_runtimeFlags |= DCDF_CONFIGURATION_POLL_PENDING;
   m_flags = flags;
   m_macAddress = macAddress;
	m_deviceClass = deviceClass;
	m_vendor = vendor;
	m_commProtocol = commProtocol;
	m_xmlRegConfig = xmlRegConfig;
	m_xmlConfig = xmlConfig;
	m_serialNumber = serialNumber;
	m_deviceAddress = deviceAddress;
	m_metaType = metaType;
	m_description = description;
	m_proxyNodeId = proxyNode;
   m_lastConnectionTime = 0;
	m_frameCount = 0; //zero when no info
   m_signalStrenght = 1; //+1 when no information(cannot be +)
   m_signalNoise = MAX_INT32; //*10 from origin number
   m_frequency = 0; //*10 from origin number
   m_lastStatusPoll = 0;
   m_lastConfigurationPoll = 0;
   m_status = STATUS_UNKNOWN;
}

Sensor *Sensor::createSensor(TCHAR *name, NXCPMessage *request)
{
   Sensor *sensor = new Sensor(name,
                          request->getFieldAsUInt32(VID_SENSOR_FLAGS),
                          request->getFieldAsMacAddress(VID_MAC_ADDR),
                          request->getFieldAsUInt32(VID_DEVICE_CLASS),
                          request->getFieldAsString(VID_VENDOR),
                          request->getFieldAsUInt32(VID_COMM_PROTOCOL),
                          request->getFieldAsString(VID_XML_REG_CONFIG),
                          request->getFieldAsString(VID_XML_CONFIG),
                          request->getFieldAsString(VID_SERIAL_NUMBER),
                          request->getFieldAsString(VID_DEVICE_ADDRESS),
                          request->getFieldAsString(VID_META_TYPE),
                          request->getFieldAsString(VID_DESCRIPTION),
                          request->getFieldAsUInt32(VID_SENSOR_PROXY));

   switch(request->getFieldAsUInt32(VID_COMM_PROTOCOL))
   {
      case COMM_LORAWAN:
         sensor->generateGuid();
         if (registerLoraDevice(sensor) == NULL)
         {
            delete sensor;
            return NULL;
         }
         break;
      case COMM_DLMS:
         sensor->m_state = SSF_PROVISIONED | SSF_REGISTERED | SSF_ACTIVE;
         break;
   }
   return sensor;
}

/**
 * Create agent connection
 */
AgentConnectionEx *Sensor::getAgentConnection()
{
   UINT32 rcc = ERR_CONNECT_FAILED;
   if (IsShutdownInProgress())
      return NULL;

   NetObj *proxy = FindObjectById(m_proxyNodeId, OBJECT_NODE);
   if(proxy == NULL)
      return NULL;

   return ((Node *)proxy)->acquireProxyConnection(SENSOR_PROXY);
}

/**
 * Register LoRa WAN device
 */
Sensor *Sensor::registerLoraDevice(Sensor *sensor)
{
   NetObj *proxy = FindObjectById(sensor->m_proxyNodeId, OBJECT_NODE);
   if(proxy == NULL)
      return NULL;

   AgentConnectionEx *conn = sensor->getAgentConnection();
   if (conn == NULL)
   {
      return sensor; //Unprovisoned sensor - will try to provison it on next connect
   }

   Config regConfig;
   Config config;
#ifdef UNICODE
   char *regXml = UTF8StringFromWideString(sensor->getXmlRegConfig());
   regConfig.loadXmlConfigFromMemory(regXml, (UINT32)strlen(regXml), NULL, "config", false);
   free(regXml);

   char *xml = UTF8StringFromWideString(sensor->getXmlConfig());
   config.loadXmlConfigFromMemory(xml, (UINT32)strlen(xml), NULL, "config", false);
   free(xml);
#else
   regConfig.loadXmlConfigFromMemory(sensor->getXmlRegConfig(), (UINT32)strlen(sensor->getXmlRegConfig()), NULL, "config", false);
   config.loadXmlConfigFromMemory(sensor->getXmlConfig(), (UINT32)strlen(sensor->getXmlConfig()), NULL, "config", false);
#endif



   NXCPMessage msg(conn->getProtocolVersion());
   msg.setCode(CMD_REGISTER_LORAWAN_SENSOR);
   msg.setId(conn->generateRequestId());
   msg.setField(VID_DEVICE_ADDRESS, sensor->getDeviceAddress());
   msg.setField(VID_MAC_ADDR, sensor->getMacAddress());
   msg.setField(VID_GUID, sensor->getGuid());
   msg.setField(VID_DECODER, config.getValueAsInt(_T("/decoder"), 0));
   msg.setField(VID_REG_TYPE, regConfig.getValueAsInt(_T("/registrationType"), 0));
   if(regConfig.getValueAsInt(_T("/registrationType"), 0) == 0)
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
   if (response != NULL)
   {
      if(response->getFieldAsUInt32(VID_RCC) == RCC_SUCCESS)
      {
         sensor->lockProperties();
         sensor->setProvisoned();
         sensor->unlockProperties();
      }
      delete response;
   }

   return sensor;
}

//set correct status calculation function
//set correct configuration poll - provision if possible, for lora get device name, get all possible DCI's, try to do provisionning
//set status poll - check if connection is on if not generate alarm, check, that proxy is up and running

/**
 * Sensor class destructor
 */
Sensor::~Sensor()
{
   free(m_vendor);
   free(m_xmlRegConfig);
   free(m_xmlConfig);
   free(m_serialNumber);
   free(m_deviceAddress);
   free(m_metaType);
   free(m_description);
}

/**
 * Load from database SensorDevice class
 */
bool Sensor::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   m_id = id;

   if (!loadCommonProperties(hdb))
   {
      nxlog_debug(2, _T("Cannot load common properties for sensor object %d"), id);
      return false;
   }

	TCHAR query[512];
	_sntprintf(query, 512, _T("SELECT mac_address,device_class,vendor,communication_protocol,xml_config,serial_number,device_address,")
                          _T("meta_type,description,last_connection_time,frame_count,signal_strenght,signal_noise,frequency,proxy_node,xml_reg_config FROM sensors WHERE id=%d"), m_id);
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult == NULL)
		return false;

   m_macAddress = DBGetFieldMacAddr(hResult, 0, 0);
	m_deviceClass = DBGetFieldULong(hResult, 0, 1);
	m_vendor = DBGetField(hResult, 0, 2, NULL, 0);
	m_commProtocol = DBGetFieldULong(hResult, 0, 3);
	m_xmlConfig = DBGetField(hResult, 0, 4, NULL, 0);
	m_serialNumber = DBGetField(hResult, 0, 5, NULL, 0);
	m_deviceAddress = DBGetField(hResult, 0, 6, NULL, 0);
	m_metaType = DBGetField(hResult, 0, 7, NULL, 0);
	m_description = DBGetField(hResult, 0, 8, NULL, 0);
   m_lastConnectionTime = DBGetFieldULong(hResult, 0, 9);
   m_frameCount = DBGetFieldULong(hResult, 0, 10);
   m_signalStrenght = DBGetFieldLong(hResult, 0, 11);
   m_signalNoise = DBGetFieldLong(hResult, 0, 12);
   m_frequency = DBGetFieldLong(hResult, 0, 13);
   m_proxyNodeId = DBGetFieldLong(hResult, 0, 14);
   m_xmlRegConfig = DBGetField(hResult, 0, 15, NULL, 0);
   DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB(hdb);
   loadItemsFromDB(hdb);
   for(int i = 0; i < m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB(hdb))
         return false;

   return true;
}

/**
 * Save to database Sensor class
 */
bool Sensor::saveToDatabase(DB_HANDLE hdb)
{
   lockProperties();

   bool success = saveCommonProperties(hdb);

   if (success && (m_modified & MODIFY_SENSOR_PROPERTIES))
   {
      DB_STATEMENT hStmt;
      bool isNew = !(IsDatabaseRecordExist(hdb, _T("sensors"), _T("id"), m_id));
      if (isNew)
         hStmt = DBPrepare(hdb, _T("INSERT INTO sensors (mac_address,device_class,vendor,communication_protocol,xml_config,serial_number,device_address,meta_type,description,last_connection_time,frame_count,signal_strenght,signal_noise,frequency,proxy_node,id,xml_reg_config) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
      else
         hStmt = DBPrepare(hdb, _T("UPDATE sensors SET mac_address=?,device_class=?,vendor=?,communication_protocol=?,xml_config=?,serial_number=?,device_address=?,meta_type=?,description=?,last_connection_time=?,frame_count=?,signal_strenght=?,signal_noise=?,frequency=?,proxy_node=? WHERE id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_macAddress);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)m_deviceClass);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (INT32)m_commProtocol);
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
      }
      else
      {
         success = false;
      }
	}

   // Save data collection items
   if (success && (m_modified & MODIFY_DATA_COLLECTION))
   {
		lockDciAccess(false);
      for(int i = 0; i < m_dcObjects->size(); i++)
         m_dcObjects->get(i)->saveToDatabase(hdb);
		unlockDciAccess();
   }

   // Save access list
   if (success)
      saveACLToDB(hdb);

   // Clear modifications flag and unlock object
	if (success)
	    m_modified = 0;
   unlockProperties();

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
   return vm->createValue(new NXSL_Object(vm, &g_nxslSensorClass, this));
}

/**
 * Sensor class serialization to json
 */
json_t *Sensor::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "macAddr", json_string_t(m_macAddress.toString(MAC_ADDR_FLAT_STRING)));
   json_object_set_new(root, "deviceClass", json_integer(m_deviceClass));
   json_object_set_new(root, "vendor", json_string_t(m_vendor));
   json_object_set_new(root, "commProtocol", json_integer(m_commProtocol));
   json_object_set_new(root, "xmlConfig", json_string_t(m_xmlConfig));
   json_object_set_new(root, "serialNumber", json_string_t(m_serialNumber));
   json_object_set_new(root, "deviceAddress", json_string_t(m_deviceAddress));
   json_object_set_new(root, "metaType", json_string_t(m_metaType));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "proxyNode", json_integer(m_proxyNodeId));
   return root;
}

void Sensor::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
   msg->setField(VID_SENSOR_FLAGS, m_flags);
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

UINT32 Sensor::modifyFromMessageInternal(NXCPMessage *request)
{
   if (request->isFieldExist(VID_FLAGS))
      m_flags = request->getFieldAsUInt32(VID_FLAGS);

   if (request->isFieldExist(VID_MAC_ADDR))
      m_macAddress = request->getFieldAsMacAddress(VID_MAC_ADDR);
   if (request->isFieldExist(VID_VENDOR))
   {
      free(m_vendor);
      m_vendor = request->getFieldAsString(VID_VENDOR);
   }
   if (request->isFieldExist(VID_DEVICE_CLASS))
      m_deviceClass = request->getFieldAsUInt32(VID_DEVICE_CLASS);
   if (request->isFieldExist(VID_SERIAL_NUMBER))
   {
      free(m_serialNumber);
      m_serialNumber = request->getFieldAsString(VID_SERIAL_NUMBER);
   }
   if (request->isFieldExist(VID_DEVICE_ADDRESS))
   {
      free(m_deviceAddress);
      m_deviceAddress = request->getFieldAsString(VID_DEVICE_ADDRESS);
   }
   if (request->isFieldExist(VID_META_TYPE))
   {
      free(m_metaType);
      m_metaType = request->getFieldAsString(VID_META_TYPE);
   }
   if (request->isFieldExist(VID_DESCRIPTION))
   {
      free(m_description);
      m_description = request->getFieldAsString(VID_DESCRIPTION);
   }
   if (request->isFieldExist(VID_SENSOR_PROXY))
      m_proxyNodeId = request->getFieldAsUInt32(VID_SENSOR_PROXY);
   if (request->isFieldExist(VID_XML_CONFIG))
   {
      free(m_xmlConfig);
      m_xmlConfig = request->getFieldAsString(VID_XML_CONFIG);
   }

   return super::modifyFromMessageInternal(request);
}

/**
 * Calculate sensor status
 */
void Sensor::calculateCompoundStatus(BOOL bForcedRecalc)
{
   UINT32 oldStatus = m_status;
   calculateStatus(bForcedRecalc);
   lockProperties();
   if (oldStatus != m_status)
      setModified(MODIFY_RUNTIME);
   unlockProperties();
}

void Sensor::calculateStatus(BOOL bForcedRecalc)
{
   AgentConnectionEx *conn = getAgentConnection();
   if (conn == NULL)
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
   if (dco->getInstanceDiscoveryData() == NULL)
      return NULL;

   DataCollectionTarget *obj;
   if (dco->getSourceNode() != 0)
   {
      obj = (DataCollectionTarget *)FindObjectById(dco->getSourceNode(), OBJECT_NODE);
      if (obj == NULL)
      {
         DbgPrintf(6, _T("Sensor::getInstanceList(%s [%d]): source node [%d] not found"), dco->getName(), dco->getId(), dco->getSourceNode());
         return NULL;
      }
      if (!obj->isTrustedNode(m_id))
      {
         DbgPrintf(6, _T("Sensor::getInstanceList(%s [%d]): this node (%s [%d]) is not trusted by source sensor %s [%d]"),
                  dco->getName(), dco->getId(), m_name, m_id, obj->getName(), obj->getId());
         return NULL;
      }
   }
   else
   {
      obj = this;
   }

   StringList *instances = NULL;
   StringMap *instanceMap = NULL;
   switch(dco->getInstanceDiscoveryMethod())
   {
      case IDM_AGENT_LIST:
         if (obj->getObjectClass() == OBJECT_NODE)
            ((Node *)obj)->getListFromAgent(dco->getInstanceDiscoveryData(), &instances);
         else if (obj->getObjectClass() == OBJECT_SENSOR)
            ((Sensor *)obj)->getListFromAgent(dco->getInstanceDiscoveryData(), &instances);
         break;
      case IDM_SCRIPT:
         obj->getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         break;
      default:
         instances = NULL;
         break;
   }
   if ((instances == NULL) && (instanceMap == NULL))
      return NULL;

   if (instanceMap == NULL)
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
void Sensor::configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   if (m_runtimeFlags & DCDF_DELETE_IN_PROGRESS)
   {
      if (rqId == 0)
         m_runtimeFlags &= ~DCDF_QUEUED_FOR_CONFIGURATION_POLL;
      return;
   }

   poller->setStatus(_T("wait for lock"));
   pollerLock();

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   nxlog_debug(5, _T("Starting configuration poll for sensor %s (ID: %d), m_flags: %d"), m_name, m_id, m_flags);

   bool hasChanges = false;

   if (m_commProtocol == COMM_LORAWAN)
   {
      if (!(m_state & SSF_PROVISIONED))
      {
         if ((registerLoraDevice(this) != NULL) && (m_state & SSF_PROVISIONED))
         {
            nxlog_debug(6, _T("ConfPoll(%s [%d}): sensor successfully registered"), m_name, m_id);
            hasChanges = true;
         }
      }
      if ((m_state & SSF_PROVISIONED) && (m_deviceAddress == NULL))
      {
         getItemFromAgent(_T("LoraWAN.DevAddr(*)"), 0, m_deviceAddress);
         if (m_deviceAddress != NULL)
         {
            nxlog_debug(6, _T("ConfPoll(%s [%d}): sensor DevAddr[%s] successfully obtained"), m_name, m_id, m_deviceAddress);
            hasChanges = true;
         }
      }
   }

   applyUserTemplates();
   updateContainerMembership();

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("ConfigurationPoll"));

   sendPollerMsg(rqId, _T("Finished configuration poll for sensor %s\r\n"), m_name);
   sendPollerMsg(rqId, _T("Sensor configuration was%schanged after poll\r\n"), hasChanges ? _T(" ") : _T(" not "));

   if (rqId == 0)
      m_runtimeFlags &= ~DCDF_QUEUED_FOR_CONFIGURATION_POLL;
   m_lastConfigurationPoll = time(NULL);

   nxlog_debug(5, _T("Finished configuration poll for sensor %s (ID: %d)"), m_name, m_id);
   m_runtimeFlags &= ~DCDF_CONFIGURATION_POLL_PENDING;
   m_runtimeFlags |= DCDF_CONFIGURATION_POLL_PASSED;
   pollerUnlock();

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
   //Create conectivity test DCI and try to get it's result
}

/**
 * Perform status poll on sensor
 */
void Sensor::statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   if (m_runtimeFlags & DCDF_DELETE_IN_PROGRESS)
   {
      if (rqId == 0)
         m_runtimeFlags &= ~DCDF_QUEUED_FOR_STATUS_POLL;
      return;
   }

   Queue *pQueue = new Queue;     // Delayed event queue

   poller->setStatus(_T("wait for lock"));
   pollerLock();

   if (IsShutdownInProgress())
   {
      delete pQueue;
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   sendPollerMsg(rqId, _T("Starting status poll for sensor %s\r\n"), m_name);
   nxlog_debug(5, _T("Starting status poll for sensor %s (ID: %d)"), m_name, m_id);

   UINT32 prevState = m_state;

   nxlog_debug(6, _T("StatusPoll(%s): checking agent"), m_name);
   poller->setStatus(_T("check agent"));
   sendPollerMsg(rqId, _T("Checking NetXMS agent connectivity\r\n"));

   AgentConnectionEx *conn = getAgentConnection();
   lockProperties();
   if (conn != NULL)
   {
      nxlog_debug(6, _T("StatusPoll(%s): connected to agent"), m_name);
      if (m_state & DCSF_UNREACHABLE)
      {
         m_state &= ~DCSF_UNREACHABLE;
         sendPollerMsg(rqId, POLLER_INFO _T("Connectivity with NetXMS agent restored\r\n"));
      }
   }
   else
   {
      nxlog_debug(6, _T("StatusPoll(%s): agent unreachable"), m_name);
      sendPollerMsg(rqId, POLLER_ERROR _T("NetXMS agent unreachable\r\n"));
      if (!(m_state & DCSF_UNREACHABLE))
         m_state |= DCSF_UNREACHABLE;
   }
   unlockProperties();
   nxlog_debug(6, _T("StatusPoll(%s): agent check finished"), m_name);

   switch(m_commProtocol)
   {
      case COMM_LORAWAN:
         if (m_runtimeFlags & SSF_PROVISIONED)
         {
            lockProperties();
            TCHAR lastValue[MAX_DCI_STRING_VALUE] = { 0 };
            time_t now;
            getItemFromAgent(_T("LoraWAN.LastContact(*)"), MAX_DCI_STRING_VALUE, lastValue);
            time_t lastConnectionTime = _tcstol(lastValue, NULL, 0);
            if (lastConnectionTime != 0)
            {
               m_lastConnectionTime = lastConnectionTime;
               nxlog_debug(6, _T("StatusPoll(%s [%d}): Last connection time updated - %d"), m_name, m_id, m_lastConnectionTime);
            }

            now = time(NULL);

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
                  getItemFromAgent(_T("LoraWAN.RSSI(*)"), MAX_DCI_STRING_VALUE, lastValue);
                  m_signalStrenght = _tcstol(lastValue, NULL, 10);
                  getItemFromAgent(_T("LoraWAN.SNR(*)"), MAX_DCI_STRING_VALUE, lastValue);
                  m_signalNoise = static_cast<INT32>(_tcstod(lastValue, NULL) * 10);
                  getItemFromAgent(_T("LoraWAN.Frequency(*)"), MAX_DCI_STRING_VALUE, lastValue);
                  m_frequency = static_cast<UINT32>(_tcstod(lastValue, NULL) * 10);
               }
            }

            unlockProperties();
         }
         break;
      case COMM_DLMS:
         checkDlmsConverterAccessibility();
         break;
      default:
         break;
   }
   calculateStatus(TRUE);

   // Send delayed events and destroy delayed event queue
   if (pQueue != NULL)
   {
      ResendEvents(pQueue);
      delete pQueue;
   }
   poller->setStatus(_T("hook"));
   executeHookScript(_T("StatusPoll"));

   if (rqId == 0)
      m_runtimeFlags &= ~DCDF_QUEUED_FOR_STATUS_POLL;

   if (prevState != m_state)
      setModified(MODIFY_SENSOR_PROPERTIES);

   sendPollerMsg(rqId, _T("Finished status poll for sensor %s\r\n"), m_name);
   sendPollerMsg(rqId, _T("Sensor status after poll is %s\r\n"), GetStatusAsText(m_status, true));

   pollerUnlock();
   m_lastStatusPoll = time(NULL);
   nxlog_debug(5, _T("Finished status poll for sensor %s (ID: %d)"), m_name, m_id);
}

void Sensor::prepareLoraDciParameters(String &parameter)
{
   int place = parameter.find(_T(")"));
   if(place > 0)
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
void Sensor::prepareDlmsDciParameters(String &parameter)
{
   Config config;
#ifdef UNICODE
   char *xml = UTF8StringFromWideString(m_xmlConfig);
   config.loadXmlConfigFromMemory(xml, (UINT32)strlen(xml), NULL, "config", false);
   free(xml);
#else
   config.loadXmlConfigFromMemory(m_xmlConfig, (UINT32)strlen(m_xmlConfig), NULL, "config", false);
#endif
   ConfigEntry *configRoot = config.getEntry(_T("/connections"));
	if (configRoot != NULL)
	{
	   int place = parameter.find(_T(")"));
	   if(place > 0)
	   {
	      parameter.replace(_T(")"), _T(""));
	   }
	   else
	   {
	      parameter.append(_T("("));
	   }
      ObjectArray<ConfigEntry> *credentials = configRoot->getSubEntries(_T("/cred"));
		for(int i = 0; i < credentials->size(); i++)
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
      delete credentials;
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
DataCollectionError Sensor::getItemFromAgent(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer)
{
   if (m_state & DCSF_UNREACHABLE)
      return DCE_COMM_ERROR;

   UINT32 dwError = ERR_NOT_CONNECTED;
   DataCollectionError dwResult = DCE_COMM_ERROR;
   int retry = 3;

   nxlog_debug(7, _T("Sensor(%s)->GetItemFromAgent(%s)"), m_name, szParam);
   // Establish connection if needed
   AgentConnectionEx *conn = getAgentConnection();
   if (conn == NULL)
   {
      return dwResult;
   }

   String parameter(szParam);
   switch(m_commProtocol)
   {
      case COMM_LORAWAN:
         prepareLoraDciParameters(parameter);
         break;
      case COMM_DLMS:
         if(parameter.find(_T("Sensor")) != -1)
            prepareDlmsDciParameters(parameter);
         break;
   }
   nxlog_debug(3, _T("Sensor(%s)->GetItemFromAgent(%s)"), m_name, parameter.getBuffer());

   // Get parameter from agent
   while(retry-- > 0)
   {
      dwError = conn->getParameter(parameter, dwBufSize, szBuffer);
      switch(dwError)
      {
         case ERR_SUCCESS:
            dwResult = DCE_SUCCESS;
            break;
         case ERR_UNKNOWN_PARAMETER:
            dwResult = DCE_NOT_SUPPORTED;
            break;
         case ERR_NO_SUCH_INSTANCE:
            dwResult = DCE_NO_SUCH_INSTANCE;
            break;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
            break;
         case ERR_REQUEST_TIMEOUT:
            // Reset connection to agent after timeout
            nxlog_debug(7, _T("Sensor(%s)->GetItemFromAgent(%s): timeout; resetting connection to agent..."), m_name, szParam);
            if (getAgentConnection() == NULL)
               break;
            nxlog_debug(7, _T("Sensor(%s)->GetItemFromAgent(%s): connection to agent restored successfully"), m_name, szParam);
            break;
         case ERR_INTERNAL_ERROR:
            dwResult = DCE_COLLECTION_ERROR;
            break;
      }
   }

   nxlog_debug(7, _T("Sensor(%s)->GetItemFromAgent(%s): dwError=%d dwResult=%d"), m_name, szParam, dwError, dwResult);
   return dwResult;
}

/**
 * Get list from agent
 */
DataCollectionError Sensor::getListFromAgent(const TCHAR *name, StringList **list)
{
   UINT32 dwError = ERR_NOT_CONNECTED;
   DataCollectionError dwResult = DCE_COMM_ERROR;
   UINT32 dwTries = 3;

   *list = NULL;

   if (m_state & DCSF_UNREACHABLE) //removed disable agent usage for all polls
      return DCE_COMM_ERROR;

   nxlog_debug(7, _T("Sensor(%s)->GetItemFromAgent(%s)"), m_name, name);
   AgentConnectionEx *conn = getAgentConnection();
   if (conn == NULL)
   {
      return dwResult;
   }

   String parameter(name);
   switch(m_commProtocol)
   {
      case COMM_LORAWAN:
         prepareLoraDciParameters(parameter);
         break;
      case COMM_DLMS:
         if(parameter.find(_T("Sensor")) != -1)
            prepareDlmsDciParameters(parameter);
         break;
   }
   nxlog_debug(3, _T("Sensor(%s)->GetItemFromAgent(%s)"), m_name, parameter.getBuffer());

   // Get parameter from agent
   while(dwTries-- > 0)
   {
      dwError = conn->getList(parameter, list);
      switch(dwError)
      {
         case ERR_SUCCESS:
            dwResult = DCE_SUCCESS;
            break;
         case ERR_UNKNOWN_PARAMETER:
            dwResult = DCE_NOT_SUPPORTED;
            break;
         case ERR_NO_SUCH_INSTANCE:
            dwResult = DCE_NO_SUCH_INSTANCE;
            break;
         case ERR_NOT_CONNECTED:
         case ERR_CONNECTION_BROKEN:
         case ERR_REQUEST_TIMEOUT:
            // Reset connection to agent after timeout
            DbgPrintf(7, _T("Sensor(%s)->getListFromAgent(%s): timeout; resetting connection to agent..."), m_name, name);
            if (getAgentConnection() == NULL)
               break;
            DbgPrintf(7, _T("Sensor(%s)->getListFromAgent(%s): connection to agent restored successfully"), m_name, name);
            break;
         case ERR_INTERNAL_ERROR:
            dwResult = DCE_COLLECTION_ERROR;
            break;
      }
   }

   DbgPrintf(7, _T("Sensor(%s)->getListFromAgent(%s): dwError=%d dwResult=%d"), m_name, name, dwError, dwResult);
   return dwResult;
}

/**
 * Prepare sensor object for deletion
 */
void Sensor::prepareForDeletion()
{
   // Prevent sensor from being queued for polling
   lockProperties();
   m_runtimeFlags |= DCDF_DELETE_IN_PROGRESS;
   unlockProperties();

   // Wait for all pending polls
   nxlog_debug(4, _T("Sensor::PrepareForDeletion(%s [%d]): waiting for outstanding polls to finish"), m_name, m_id);
   while(1)
   {
      lockProperties();
      if ((m_runtimeFlags &
            (DCDF_QUEUED_FOR_STATUS_POLL | DCDF_QUEUED_FOR_CONFIGURATION_POLL)) == 0)
      {
         unlockProperties();
         break;
      }
      unlockProperties();
      ThreadSleepMs(100);
   }
   nxlog_debug(4, _T("Sensor::PrepareForDeletion(%s [%d]): no outstanding polls left"), m_name, m_id);

   AgentConnectionEx *conn = getAgentConnection();
   if(m_commProtocol == COMM_LORAWAN && conn != NULL)
   {
      NXCPMessage msg(conn->getProtocolVersion());
      msg.setCode(CMD_UNREGISTER_LORAWAN_SENSOR);
      msg.setId(conn->generateRequestId());
      msg.setField(VID_GUID, m_guid);
      NXCPMessage *response = conn->customRequest(&msg);
      if (response != NULL)
      {
         if(response->getFieldAsUInt32(VID_RCC) == RCC_SUCCESS)
            nxlog_debug(4, _T("Sensor::PrepareForDeletion(%s [%d]): successfully unregistered from LoRaWAN server"), m_name, m_id);

         delete response;
      }
   }

   super::prepareForDeletion();
}

/**
 * Build internal connection topology
 */
NetworkMapObjectList *Sensor::buildInternalConnectionTopology()
{
   NetworkMapObjectList *topology = new NetworkMapObjectList();
   topology->setAllowDuplicateLinks(true);
   buildInternalConnectionTopologyInternal(topology, false);

   return topology;
}

/**
 * Build internal connection topology - internal function
 */
void Sensor::buildInternalConnectionTopologyInternal(NetworkMapObjectList *topology, bool checkAllProxies)
{
   topology->addObject(m_id);

   if (m_proxyNodeId != 0)
   {
      Node *proxy = static_cast<Node *>(FindObjectById(m_proxyNodeId, OBJECT_NODE));
      if (proxy != NULL)
      {
         topology->addObject(m_proxyNodeId);
         topology->linkObjects(m_id, m_proxyNodeId, LINK_TYPE_SENSOR_PROXY, _T("Sensor proxy"));
         proxy->buildInternalConnectionTopologyInternal(topology, m_proxyNodeId, false, checkAllProxies);
      }
   }
}
