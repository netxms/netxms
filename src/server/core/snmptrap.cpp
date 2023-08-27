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
** File: snmptrap.cpp
**
**/

#include "nxcore.h"
#include <nxcore_discovery.h>

#define DEBUG_TAG _T("snmp.trap")

#define BY_OBJECT_ID 0
#define BY_POSITION 1

/**
 * Total number of received SNMP traps
 */
VolatileCounter64 g_snmpTrapsReceived = 0;

/**
 * Max SNMP packet length
 */
#define MAX_PACKET_LENGTH     65536

/**
 * Static data
 */
static Mutex s_trapCfgLock;
static ObjectArray<SNMPTrapConfiguration> m_trapCfgList(16, 4, Ownership::True);
static VolatileCounter64 s_trapId = 0; // Last used trap ID
static uint16_t s_trapListenerPort = 162;

/**
 * Collects information about all SNMPTraps that are using specified event
 */
void GetSNMPTrapsEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences)
{
   s_trapCfgLock.lock();
   for (int i = 0; i < m_trapCfgList.size(); i++)
   {
      SNMPTrapConfiguration *tc = m_trapCfgList.get(i);
      if (tc->getEventCode() == eventCode)
      {
         eventReferences->add(new EventReference(EventReferenceType::SNMP_TRAP, tc->getId(), tc->getGuid(), tc->getOid().toString(), tc->getDescription()));
      }
   }
   s_trapCfgLock.unlock();
}

/**
 * Create new SNMP trap configuration object
 */
SNMPTrapConfiguration::SNMPTrapConfiguration() : m_objectId(), m_mappings(8, 8, Ownership::True)
{
   m_guid = uuid::generate();
   m_id = CreateUniqueId(IDG_SNMP_TRAP);
   m_eventCode = EVENT_SNMP_UNMATCHED_TRAP;
   m_eventTag = nullptr;
   m_description = nullptr;
   m_scriptSource = nullptr;
   m_script = nullptr;
}

/**
 * Create SNMP trap configuration object from database
 */
SNMPTrapConfiguration::SNMPTrapConfiguration(DB_RESULT trapResult, DB_HANDLE hdb, DB_STATEMENT stmt, int row) : m_mappings(8, 8, Ownership::True)
{
   m_id = DBGetFieldULong(trapResult, row, 0);
   TCHAR buffer[MAX_OID_LENGTH];
   m_objectId = SNMP_ObjectId::parse(DBGetField(trapResult, row, 1, buffer, MAX_OID_LENGTH));
   m_eventCode = DBGetFieldULong(trapResult, row, 2);
   m_description = DBGetField(trapResult, row, 3, nullptr, 0);
   m_eventTag = DBGetField(trapResult, row, 4, nullptr, 0);
   m_guid = DBGetFieldGUID(trapResult, row, 5);
   m_scriptSource = DBGetField(trapResult, row, 6, nullptr, 0);
   m_script = nullptr;

   DB_RESULT mapResult;
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_INTEGER, m_id);
      mapResult = DBSelectPrepared(stmt);
   }
   else
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("SELECT snmp_oid,description,flags FROM snmp_trap_pmap WHERE trap_id=%u ORDER BY parameter"), m_id);
      mapResult = DBSelect(hdb, query);
   }
   if (mapResult != nullptr)
   {
      int mapCount = DBGetNumRows(mapResult);
      for(int i = 0; i < mapCount; i++)
      {
         SNMPTrapParameterMapping *param = new SNMPTrapParameterMapping(mapResult, i);
         if (!param->isPositional() && !param->getOid()->isValid())
            nxlog_write(NXLOG_WARNING, _T("Invalid trap parameter OID %s for trap %u in trap configuration table"), (const TCHAR *)param->getOid()->toString(), m_id);
         m_mappings.add(param);
      }
      DBFreeResult(mapResult);
   }

   compileScript();
}

/**
 * Create SNMP trap configuration object from config entry
 */
SNMPTrapConfiguration::SNMPTrapConfiguration(const ConfigEntry& entry, const uuid& guid, uint32_t id, uint32_t eventCode) : m_mappings(8, 8, Ownership::True)
{
   if (id == 0)
      m_id = CreateUniqueId(IDG_SNMP_TRAP);
   else
      m_id = id;

   m_guid = guid;
   m_objectId = SNMP_ObjectId::parse(entry.getSubEntryValue(_T("oid"), 0, _T("")));
   m_eventCode = eventCode;
   m_description = MemCopyString(entry.getSubEntryValue(_T("description")));
   m_eventTag = MemCopyString(entry.getSubEntryValue(_T("eventTag"), 0, entry.getSubEntryValue(_T("userTag"))));
   m_scriptSource = MemCopyString(entry.getSubEntryValue(_T("transformationScript")));
   m_script = nullptr;

   const ConfigEntry *parametersRoot = entry.findEntry(_T("parameters"));
   if (parametersRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> parameters = parametersRoot->getOrderedSubEntries(_T("parameter#*"));
      if (parameters->size() > 0)
      {
         for(int i = 0; i < parameters->size(); i++)
         {
            SNMPTrapParameterMapping *param = new SNMPTrapParameterMapping(parameters->get(i));
            if (!param->isPositional() && !param->getOid()->isValid())
               nxlog_write(NXLOG_WARNING, _T("Invalid trap parameter OID %s for trap %u in trap configuration table"), param->getOid()->toString().cstr(), m_id);
            m_mappings.add(param);
         }
      }
   }

   compileScript();
}

/**
 * Create SNMP trap configuration object from NXCPMessage
 */
SNMPTrapConfiguration::SNMPTrapConfiguration(const NXCPMessage& msg) : m_mappings(8, 8, Ownership::True)
{
   uint32_t buffer[MAX_OID_LENGTH];

   m_id = msg.getFieldAsUInt32(VID_TRAP_ID);
   msg.getFieldAsInt32Array(VID_TRAP_OID, msg.getFieldAsUInt32(VID_TRAP_OID_LEN), buffer);
   m_objectId = SNMP_ObjectId(buffer, msg.getFieldAsUInt32(VID_TRAP_OID_LEN));
   m_eventCode = msg.getFieldAsUInt32(VID_EVENT_CODE);
   m_description = msg.getFieldAsString(VID_DESCRIPTION);
   m_eventTag = msg.getFieldAsString(VID_USER_TAG);
   m_scriptSource = msg.getFieldAsString(VID_TRANSFORMATION_SCRIPT);
   m_script = nullptr;

   // Read new mappings from message
   int count = msg.getFieldAsInt32(VID_TRAP_NUM_MAPS);
   uint32_t base = VID_TRAP_PBASE;
   for(int i = 0; i < count; i++, base += 10)
   {
      m_mappings.add(new SNMPTrapParameterMapping(msg, base));
   }

   compileScript();
}

/**
 * Destructor for SNMP trap configuration object
 */
SNMPTrapConfiguration::~SNMPTrapConfiguration()
{
   MemFree(m_description);
   MemFree(m_eventTag);
   MemFree(m_scriptSource);
   delete m_script;
}

/**
 * Compile transformation script
 */
void SNMPTrapConfiguration::compileScript()
{
   delete m_script;
   if ((m_scriptSource != nullptr) && (*m_scriptSource != 0))
   {
      m_script = CompileServerScript(m_scriptSource, SCRIPT_CONTEXT_SNMP_TRAP, nullptr, 0, _T("SNMPTrap::%u"), m_id);
      if (m_script == nullptr)
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Failed to compile SNMP trap transformation script for trap mapping [%u]"), m_id);
      }
   }
   else
   {
      m_script = nullptr;
   }
}

/**
 * Fill NXCP message with trap configuration data
 */
void SNMPTrapConfiguration::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_TRAP_ID, m_id);
   msg->setField(VID_TRAP_OID_LEN, (UINT32)m_objectId.length());
   msg->setFieldFromInt32Array(VID_TRAP_OID, m_objectId.length(), m_objectId.value());
   msg->setField(VID_EVENT_CODE, m_eventCode);
   msg->setField(VID_DESCRIPTION, CHECK_NULL_EX(m_description));
   msg->setField(VID_USER_TAG, CHECK_NULL_EX(m_eventTag));
   msg->setField(VID_TRANSFORMATION_SCRIPT, CHECK_NULL_EX(m_scriptSource));

   msg->setField(VID_TRAP_NUM_MAPS, (UINT32)m_mappings.size());
   UINT32 base = VID_TRAP_PBASE;
   for(int i = 0; i < m_mappings.size(); i++, base += 10)
   {
      m_mappings.get(i)->fillMessage(msg, base);
   }
}

/**
 * Fill NXCP message with trap configuration for list
 */
void SNMPTrapConfiguration::fillMessage(NXCPMessage *msg, UINT32 base) const
{
   msg->setField(base, m_id);
   msg->setField(base + 1, CHECK_NULL_EX(m_description));
   msg->setFieldFromInt32Array(base + 2, m_objectId.length(), m_objectId.value());
   msg->setField(base + 3, m_eventCode);
}

/**
 * Notify clients about trap configuration change
 */
void NotifyOnTrapCfgChangeCB(ClientSession *session, NXCPMessage *msg)
{
   if (session->isAuthenticated())
      session->postMessage(msg);
}

/**
 * Notify clients of trap cfg change
 */
void SNMPTrapConfiguration::notifyOnTrapCfgChange(UINT32 code)
{
   NXCPMessage msg;
   msg.setCode(CMD_TRAP_CFG_UPDATE);
   msg.setField(VID_NOTIFICATION_CODE, code);
   fillMessage(&msg);
   EnumerateClientSessions(NotifyOnTrapCfgChangeCB, &msg);
}

/**
 * SNMP trap parameter map default constructor
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping()
{
   m_objectId = new SNMP_ObjectId();
   m_position = 0;
   m_flags = 0;
   m_description = nullptr;
}

/**
 * Create SNMP trap parameter map object from database
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping(DB_RESULT mapResult, int row)
{
   TCHAR oid[MAX_DB_STRING];
   DBGetField(mapResult, row, 0, oid, MAX_DB_STRING);

   if (!_tcsncmp(oid, _T("POS:"), 4))
   {
      m_objectId = nullptr;
      m_position = _tcstoul(&oid[4], nullptr, 10);
   }
   else
   {
      m_objectId = new SNMP_ObjectId(SNMP_ObjectId::parse(oid));
      m_position = 0;
   }

   m_description = DBGetField(mapResult, row, 1, nullptr, 0);
   m_flags = DBGetFieldULong(mapResult, row, 2);
}

/**
 * Create SNMP trap parameter map object from config entry
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping(ConfigEntry *entry)
{
   int position = entry->getSubEntryValueAsInt(_T("position"), 0, -1);
   if (position > 0)
   {
      m_objectId = nullptr;
      m_position = position; // Positional parameter
   }
   else
   {
      m_objectId = new SNMP_ObjectId(SNMP_ObjectId::parse(entry->getSubEntryValue(_T("oid"), 0, _T("")))); // OID parameter
      m_position = 0;
   }

   m_description = MemCopyString(entry->getSubEntryValue(_T("description")));
   m_flags = entry->getSubEntryValueAsUInt(_T("flags"), 0, 0);
}

/**
 * Create SNMP trap parameter map object from NXCPMessage
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping(const NXCPMessage& msg, uint32_t base)
{
   m_flags = msg.getFieldAsUInt32(base);
   m_description = msg.getFieldAsString(base + 1);
   if (msg.getFieldAsUInt32(base + 2) == BY_POSITION)
   {
      m_objectId = nullptr;
      m_position = msg.getFieldAsUInt32(base + 3);
   }
   else
   {
      uint32_t buffer[MAX_OID_LENGTH];
      msg.getFieldAsInt32Array(base + 3, msg.getFieldAsUInt32(base + 4), buffer);
      m_objectId = new SNMP_ObjectId(buffer, msg.getFieldAsUInt32(base + 4));
   }
}

/**
 * Destructor for SNMP trap parameter map object
 */
SNMPTrapParameterMapping::~SNMPTrapParameterMapping()
{
   delete m_objectId;
   MemFree(m_description);
}

/**
 * Fill NXCP message with trap parameter map configuration data
 */
void SNMPTrapParameterMapping::fillMessage(NXCPMessage *msg, uint32_t base) const
{
   msg->setField(base, isPositional());
   if (isPositional())
      msg->setField(base + 1, m_position);
   else
      msg->setFieldFromInt32Array(base + 1, m_objectId->length(), m_objectId->value());
   msg->setField(base + 2, CHECK_NULL_EX(m_description));
   msg->setField(base + 3, m_flags);

}

/**
 * Load trap configuration from database
 */
void LoadTrapCfg()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Load traps
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT trap_id,snmp_oid,event_code,description,user_tag,guid,transformation_script FROM snmp_trap_cfg"));
   if (hResult != nullptr)
   {
      DB_STATEMENT hStmt = (g_dbSyntax == DB_SYNTAX_ORACLE) ?
               DBPrepare(hdb, _T("SELECT snmp_oid,description,flags FROM snmp_trap_pmap WHERE trap_id=? ORDER BY parameter"), true)
               : nullptr;
      if ((g_dbSyntax != DB_SYNTAX_ORACLE) || (hStmt != nullptr))
      {
         int numRows = DBGetNumRows(hResult);
         for(int i = 0; i < numRows; i++)
         {
            SNMPTrapConfiguration *trapCfg = new SNMPTrapConfiguration(hResult, hdb, hStmt, i);
            if (!trapCfg->getOid().isValid())
            {
               TCHAR buffer[MAX_DB_STRING];
               nxlog_write(NXLOG_ERROR, _T("Invalid trap enterprise ID %s in trap configuration table"),
                        DBGetField(hResult, i, 1, buffer, MAX_DB_STRING));
            }
            m_trapCfgList.add(trapCfg);
         }
         if (hStmt != nullptr)
            DBFreeStatement(hStmt);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get last SNMP Trap id
 */
int64_t GetLastSnmpTrapId()
{
   return s_trapId;
}

/**
 * Initialize trap handling
 */
void InitTraps()
{
   LoadTrapCfg();

   int64_t id = ConfigReadInt64(_T("LastSNMPTrapId"), 0);
   if (id > s_trapId)
      s_trapId = id;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(trap_id) FROM snmp_trap_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_trapId = std::max(DBGetFieldInt64(hResult, 0, 0), static_cast<int64_t>(s_trapId));
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);

   s_trapListenerPort = static_cast<uint16_t>(ConfigReadULong(_T("SNMP.Traps.ListenerPort"), s_trapListenerPort)); // 162 by default;
}

/**
 * Generate event for matched trap
 */
static void GenerateTrapEvent(const shared_ptr<Node>& node, UINT32 dwIndex, SNMP_PDU *pdu, int sourcePort)
{
   SNMPTrapConfiguration *trapCfg = m_trapCfgList.get(dwIndex);

   EventBuilder event(trapCfg->getEventCode(), *node);
   event.origin(EventOrigin::SNMP);
   event.tag(trapCfg->getEventTag());
   event.param(_T("oid"), pdu->getTrapId().toString());

	// Extract varbinds from trap and add them as event's parameters
   int numMaps = trapCfg->getParameterMappingCount();
   for(int i = 0; i < numMaps; i++)
   {
      const SNMPTrapParameterMapping *pm = trapCfg->getParameterMapping(i);
      if (pm->isPositional())
      {
			// Extract by varbind position
         // Position numbering in mapping starts from 1,
         // SNMP v2/v3 trap contains uptime and trap OID at positions 0 and 1,
         // so map first mapping position to index 2 and so on
         int index = (pdu->getVersion() == SNMP_VERSION_1) ? pm->getPosition() - 1 : pm->getPosition() + 1;
         SNMP_Variable *varbind = pdu->getVariable(index);
         if (varbind != nullptr)
         {
				bool convertToHex = true;
            TCHAR name[64], buffer[3072];
            _sntprintf(name, 64, _T("%d"), pm->getPosition());
            event.param(name,
               ((g_flags & AF_ALLOW_TRAP_VARBIND_CONVERSION) && !(pm->getFlags() & TRAP_VARBIND_FORCE_TEXT)) ?
                  varbind->getValueAsPrintableString(buffer, 3072, &convertToHex) :
                  varbind->getValueAsString(buffer, 3072));
         }
      }
      else
      {
			// Extract by varbind OID
         for(int j = 0; j < pdu->getNumVariables(); j++)
         {
            SNMP_Variable *varbind = pdu->getVariable(j);
            int result = varbind->getName().compare(*(pm->getOid()));
            if ((result == OID_EQUAL) || (result == OID_LONGER))
            {
					bool convertToHex = true;
					TCHAR buffer[3072];
					event.param(varbind->getName().toString(),
                  ((g_flags & AF_ALLOW_TRAP_VARBIND_CONVERSION) && !(pm->getFlags() & TRAP_VARBIND_FORCE_TEXT)) ?
                     varbind->getValueAsPrintableString(buffer, 3072, &convertToHex) :
                     varbind->getValueAsString(buffer, 3072));
               break;
            }
         }
      }
   }

   event.param(_T("sourcePort"), sourcePort);

   NXSL_VM *vm;
   if ((trapCfg->getScript() != nullptr) && !trapCfg->getScript()->isEmpty())
   {
      vm = CreateServerScriptVM(trapCfg->getScript(), node);
      if (vm != nullptr)
      {
         vm->setGlobalVariable("$trap", vm->createValue(pdu->getTrapId().toString()));
         NXSL_Array *varbinds = new NXSL_Array(vm);
         for(int i = (pdu->getVersion() == SNMP_VERSION_1) ? 0 : 2; i < pdu->getNumVariables(); i++)
         {
            varbinds->append(vm->createValue(vm->createObject(&g_nxslSnmpVarBindClass, new SNMP_Variable(pdu->getVariable(i)))));
         }
         vm->setGlobalVariable("$varbinds", vm->createValue(varbinds));
         event.vm(vm);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("GenerateTrapEvent: cannot load transformation script for trap mapping [%u]"), trapCfg->getId());
      }
   }
   else
   {
      vm = nullptr;
   }
   event.post();
   delete vm;
}

/**
 * Handler for EnumerateSessions()
 */
static void BroadcastNewTrap(ClientSession *pSession, NXCPMessage *msg)
{
   pSession->onNewSNMPTrap(msg);
}

/**
 * Build trap varbind list
 */
static StringBuffer BuildVarbindList(SNMP_PDU *pdu)
{
   StringBuffer out;
   TCHAR oidText[1024], data[4096];

   for(int i = (pdu->getVersion() == SNMP_VERSION_1) ? 0 : 2; i < pdu->getNumVariables(); i++)
   {
      SNMP_Variable *v = pdu->getVariable(i);
      if (!out.isEmpty())
         out.append(_T("; "));

      v->getName().toString(oidText, 1024);

      bool convertToHex = true;
      if (g_flags & AF_ALLOW_TRAP_VARBIND_CONVERSION)
         v->getValueAsPrintableString(data, 4096, &convertToHex);
      else
         v->getValueAsString(data, 4096);

      nxlog_debug_tag(DEBUG_TAG, 5, _T("   %s == '%s'"), oidText, data);

      out.append(oidText);
      out.append(_T(" == '"));
      out.append(data);
      out.append(_T('\''));
   }

   return out;
}

/**
 * Process trap
 */
void ProcessTrap(SNMP_PDU *pdu, const InetAddress& srcAddr, int32_t zoneUIN, int srcPort, SNMP_Transport *snmpTransport, SNMP_Engine *localEngine, bool isInformRq)
{
   StringBuffer varbinds;
   TCHAR buffer[4096];
	bool processedByModule = false;
   int iResult;

   InterlockedIncrement64(&g_snmpTrapsReceived);
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Received SNMP %s %s from %s"), isInformRq ? _T("INFORM-REQUEST") : _T("TRAP"),
             pdu->getTrapId().toString(&buffer[96], 4000), srcAddr.toString(buffer));

	if (isInformRq)
	{
		SNMP_PDU response(SNMP_RESPONSE, pdu->getRequestId(), pdu->getVersion());
		if (snmpTransport->getSecurityContext() == nullptr)
		{
		   snmpTransport->setSecurityContext(new SNMP_SecurityContext(pdu->getCommunity()));
		}
		response.setMessageId(pdu->getMessageId());
		response.setContextEngineId(localEngine->getId(), localEngine->getIdLen());
		snmpTransport->sendMessage(&response, 0);
	}

   // Match IP address to object
   shared_ptr<Node> node = FindNodeByIP(zoneUIN, (g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) != 0, srcAddr);

   // Write trap to log if required
   if ((node != nullptr) || (g_flags & AF_LOG_ALL_SNMP_TRAPS))
   {
      time_t timestamp = time(nullptr);

      nxlog_debug_tag(DEBUG_TAG, 5, _T("Varbinds for %s %s from %s:"), isInformRq ? _T("INFORM-REQUEST") : _T("TRAP"), &buffer[96], buffer);
      if ((node != nullptr) && (node->getSnmpCodepage()[0] != 0))
      {
         pdu->setCodepage(node->getSnmpCodepage());
      }
      else if (g_snmpCodepage[0] != 0)
      {
         pdu->setCodepage(g_snmpCodepage);
      }
      varbinds = BuildVarbindList(pdu);

      // Write new trap to database
		uint64_t trapId = InterlockedIncrement64(&s_trapId);
      TCHAR query[8192], oidText[1024];
      _sntprintf(query, 8192, _T("INSERT INTO snmp_trap_log (trap_id,trap_timestamp,")
                                _T("ip_addr,object_id,zone_uin,trap_oid,trap_varlist) VALUES ")
                                _T("(") UINT64_FMT _T(",%s") INT64_FMT _T("%s,'%s',%d,%d,'%s',%s)"),
                 trapId, (g_dbSyntax == DB_SYNTAX_TSDB) ? _T("to_timestamp(") : _T(""), static_cast<int64_t>(timestamp),
                 (g_dbSyntax == DB_SYNTAX_TSDB) ? _T(")") : _T(""), srcAddr.toString(buffer),
                 (node != nullptr) ? node->getId() : (UINT32)0, (node != nullptr) ? node->getZoneUIN() : zoneUIN,
                 pdu->getTrapId().toString(oidText, 1024),
                 DBPrepareString(g_dbDriver, varbinds).cstr());
      QueueSQLRequest(query);

      // Notify connected clients
      NXCPMessage msg;
      msg.setCode(CMD_TRAP_LOG_RECORDS);
      msg.setField(VID_NUM_RECORDS, (UINT32)1);
      msg.setField(VID_RECORDS_ORDER, (WORD)RECORD_ORDER_NORMAL);
      msg.setField(VID_TRAP_LOG_MSG_BASE, trapId);
      msg.setFieldFromTime(VID_TRAP_LOG_MSG_BASE + 1, timestamp);
      msg.setField(VID_TRAP_LOG_MSG_BASE + 2, srcAddr);
      msg.setField(VID_TRAP_LOG_MSG_BASE + 3, (node != nullptr) ? node->getId() : (UINT32)0);
      msg.setField(VID_TRAP_LOG_MSG_BASE + 4, pdu->getTrapId().toString(oidText, 1024));
      msg.setField(VID_TRAP_LOG_MSG_BASE + 5, varbinds);
      EnumerateClientSessions(BroadcastNewTrap, &msg);
   }
   else if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 5)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Varbinds for %s %s:"), isInformRq ? _T("INFORM-REQUEST") : _T("TRAP"), &buffer[96]);
      BuildVarbindList(pdu);
   }

   // Process trap if it is coming from host registered in database
   if (node != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ProcessTrap: trap matched to node %s [%d]"), node->getName(), node->getId());
      node->incSnmpTrapCount();
      if (node->checkTrapShouldBeProcessed())
      {
         if ((node->getStatus() != STATUS_UNMANAGED) || (g_flags & AF_TRAPS_FROM_UNMANAGED_NODES))
         {
            // Pass trap to loaded modules
            ENUMERATE_MODULES(pfTrapHandler)
            {
               if (CURRENT_MODULE.pfTrapHandler(pdu, node))
               {
                  processedByModule = true;
                  break;   // Trap was processed by the module
               }
            }

            // Find if we have this trap in our list
            s_trapCfgLock.lock();

            // Try to find closest match
            size_t matchLen = 0;
            int matchIndex;
            for(int i = 0; i < m_trapCfgList.size(); i++)
            {
               const SNMPTrapConfiguration *trapCfg = m_trapCfgList.get(i);
               if (trapCfg->getOid().length() > 0)
               {
                  iResult = pdu->getTrapId().compare(trapCfg->getOid());
                  if (iResult == OID_EQUAL)
                  {
                     matchLen = trapCfg->getOid().length();
                     matchIndex = i;
                     break;   // Find exact match
                  }
                  else if (iResult == OID_LONGER)
                  {
                     if (trapCfg->getOid().length() > matchLen)
                     {
                        matchLen = trapCfg->getOid().length();
                        matchIndex = i;
                     }
                  }
               }
            }

            if (matchLen > 0)
            {
               GenerateTrapEvent(node, matchIndex, pdu, srcPort);
            }
            else if (!processedByModule)    // Process unmatched traps not processed by module
            {
               // Generate default event for unmatched traps
               TCHAR oidText[1024];
               EventBuilder(EVENT_SNMP_UNMATCHED_TRAP, *node)
                  .origin(EventOrigin::SNMP)
                  .param(_T("oid"), pdu->getTrapId().toString(oidText, 1024))
                  .param(_T("varbinds"), varbinds)
                  .param(_T("sourcePort"), srcPort)
                  .post();
            }
            s_trapCfgLock.unlock();
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("ProcessTrap: Node %s [%d] is in UNMANAGED state, trap ignored"), node->getName(), node->getId());
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ProcessTrap: Node %s [%d] is in trap flood state, trap is dropped"), node->getName(), node->getId());
      }
   }
   else if (g_flags & AF_SNMP_TRAP_DISCOVERY)  // unknown node, discovery enabled
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ProcessTrap: trap not matched to node, adding new IP address %s for discovery"), srcAddr.toString(buffer));
      CheckPotentialNode(srcAddr, zoneUIN, DA_SRC_SNMP_TRAP, 0);
   }
   else  // unknown node, discovery disabled
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ProcessTrap: trap not matched to any node"));
   }
}

/**
 * Context finder - tries to find SNMPv3 security context by IP address
 */
static SNMP_SecurityContext *ContextFinder(struct sockaddr *addr, socklen_t addrLen)
{
   InetAddress ipAddr = InetAddress::createFromSockaddr(addr);
	shared_ptr<Node> node = FindNodeByIP((g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) ? ALL_ZONES : 0, ipAddr);
	TCHAR buffer[64];
	nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMPTrapReceiver: looking for SNMP security context for node %s %s"),
      ipAddr.toString(buffer), (node != nullptr) ? node->getName() : _T("<unknown>"));
	return (node != nullptr) ? node->getSnmpSecurityContext() : nullptr;
}

/**
 * Create SNMP transport for receiver
 */
static SNMP_Transport *CreateTransport(SOCKET hSocket)
{
   if (hSocket == INVALID_SOCKET)
      return nullptr;

   SNMP_Transport *t = new SNMP_UDPTransport(hSocket);
	t->enableEngineIdAutoupdate(true);
	t->setPeerUpdatedOnRecv(true);
   return t;
}

/**
 * SNMP trap receiver thread
 */
void SNMPTrapReceiver()
{
   ThreadSetName("SNMPTrapRecv");

   static BYTE defaultEngineId[] = { 0x80, 0x00, 0xDF, 0x4B, 0x05, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 };
   BYTE engineId[32];
   size_t engineIdLen;
   TCHAR engineIdText[96];
   if (ConfigReadStr(_T("SNMP.EngineId"), engineIdText, 96, _T("")))
   {
      TranslateStr(engineIdText, _T(":"), _T(""));
      engineIdLen = StrToBin(engineIdText, engineId, sizeof(engineId));
      if (engineIdLen < 1)
      {
         memcpy(engineId, defaultEngineId, 12);
         engineIdLen = 12;
      }
   }
   else
   {
      memcpy(engineId, defaultEngineId, 12);
      engineIdLen = 12;
   }
   SNMP_Engine localEngine(engineId, engineIdLen);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Local SNMP engine ID set to %s"), localEngine.toString().cstr());

   SOCKET hSocket = CreateSocket(AF_INET, SOCK_DGRAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = CreateSocket(AF_INET6, SOCK_DGRAM, 0);
#endif

#ifdef WITH_IPV6
   if ((hSocket == INVALID_SOCKET) && (hSocket6 == INVALID_SOCKET))
#else
   if (hSocket == INVALID_SOCKET)
#endif
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to create socket for SNMP trap receiver (%s)"), GetLastSocketErrorText(buffer, 1024));
      return;
   }

   SetSocketExclusiveAddrUse(hSocket);
   SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

#ifdef WITH_IPV6
   SetSocketExclusiveAddrUse(hSocket6);
   SetSocketReuseFlag(hSocket6);
#ifndef _WIN32
   fcntl(hSocket6, F_SETFD, fcntl(hSocket6, F_GETFD) | FD_CLOEXEC);
#endif
#ifdef IPV6_V6ONLY
   int on = 1;
   setsockopt(hSocket6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
#endif
#endif

   // Fill in local address structure
   struct sockaddr_in servAddr;
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;

#ifdef WITH_IPV6
   struct sockaddr_in6 servAddr6;
   memset(&servAddr6, 0, sizeof(struct sockaddr_in6));
   servAddr6.sin6_family = AF_INET6;
#endif
   if (!_tcscmp(g_szListenAddress, _T("*")))
	{
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef WITH_IPV6
		memset(servAddr6.sin6_addr.s6_addr, 0, 16);
#endif
	}
	else
	{
      InetAddress bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET))
      {
		   servAddr.sin_addr.s_addr = htonl(bindAddress.getAddressV4());
      }
      else
      {
   		servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      }
#ifdef WITH_IPV6
      bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET6);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET6))
      {
		   memcpy(servAddr6.sin6_addr.s6_addr, bindAddress.getAddressV6(), 16);
      }
      else
      {
   		memset(servAddr6.sin6_addr.s6_addr, 0, 15);
         servAddr6.sin6_addr.s6_addr[15] = 1;
      }
#endif
	}
   servAddr.sin_port = htons(s_trapListenerPort);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(s_trapListenerPort);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv4 socket for SNMP trap receiver (%s)"), GetLastSocketErrorText(buffer, 1024));
      bindFailures++;
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }

#ifdef WITH_IPV6
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv6 socket for SNMP trap receiver (%s)"), GetLastSocketErrorText(buffer, 1024));
      bindFailures++;
      closesocket(hSocket6);
      hSocket6 = INVALID_SOCKET;
   }
#else
   bindFailures++;
#endif

   // Abort if cannot bind to at least one socket
   if (bindFailures == 2)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP trap receiver aborted - cannot bind at least one socket"));
      return;
   }

   if (hSocket != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
	   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for SNMP traps on UDP socket %s:%u"), InetAddress(ntohl(servAddr.sin_addr.s_addr)).toString(ipAddrText), s_trapListenerPort);
   }
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for SNMP traps on UDP socket %s:%u"), InetAddress(servAddr6.sin6_addr.s6_addr).toString(ipAddrText), s_trapListenerPort);
   }
#endif

   SNMP_Transport *snmp = CreateTransport(hSocket);
#ifdef WITH_IPV6
   SNMP_Transport *snmp6 = CreateTransport(hSocket6);
#endif

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("SNMP Trap Receiver started on port %u"), s_trapListenerPort);

   // Wait for packets
   SocketPoller sp;
   while(!IsShutdownInProgress())
   {
      sp.reset();
      if (hSocket != INVALID_SOCKET)
         sp.add(hSocket);
#ifdef WITH_IPV6
      if (hSocket6 != INVALID_SOCKET)
         sp.add(hSocket6);
#endif

      int rc = sp.poll(1000);
      if ((rc > 0) && !IsShutdownInProgress())
      {
         SockAddrBuffer addr;
         socklen_t addrLen = sizeof(SockAddrBuffer);
         SNMP_PDU *pdu;
#ifdef WITH_IPV6
         SNMP_Transport *transport = sp.isSet(hSocket) ? snmp : snmp6;
#else
         SNMP_Transport *transport = snmp;
#endif
         int bytes = transport->readMessage(&pdu, 2000, (struct sockaddr *)&addr, &addrLen, ContextFinder);
         if ((bytes > 0) && (pdu != nullptr))
         {
            InetAddress sourceAddr = InetAddress::createFromSockaddr((struct sockaddr *)&addr);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMPTrapReceiver: received PDU of type %d from %s"), pdu->getCommand(), (const TCHAR *)sourceAddr.toString());
			   if ((pdu->getCommand() == SNMP_TRAP) || (pdu->getCommand() == SNMP_INFORM_REQUEST))
			   {
				   if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_INFORM_REQUEST))
				   {
					   SNMP_SecurityContext *context = transport->getSecurityContext();
					   context->setAuthoritativeEngine(localEngine);
				   }
               ProcessTrap(pdu, sourceAddr, 0, ntohs(SA_PORT(&addr)), transport, &localEngine, pdu->getCommand() == SNMP_INFORM_REQUEST);
			   }
			   else if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_GET_REQUEST) && (pdu->getAuthoritativeEngine().getIdLen() == 0))
			   {
				   // Engine ID discovery
				   nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMPTrapReceiver: EngineId discovery"));

				   SNMP_PDU *response = new SNMP_PDU(SNMP_REPORT, pdu->getRequestId(), pdu->getVersion());
				   response->setReportable(false);
				   response->setMessageId(pdu->getMessageId());
				   response->setContextEngineId(localEngine.getId(), localEngine.getIdLen());

				   SNMP_Variable *var = new SNMP_Variable(_T(".1.3.6.1.6.3.15.1.1.4.0"));
				   var->setValueFromUInt32(ASN_INTEGER, 2);
				   response->bindVariable(var);

				   SNMP_SecurityContext *context = new SNMP_SecurityContext();
				   localEngine.setTime((int)time(nullptr));
				   context->setAuthoritativeEngine(localEngine);
				   context->setSecurityModel(SNMP_SECURITY_MODEL_USM);
				   context->setAuthMethod(SNMP_AUTH_NONE);
				   context->setPrivMethod(SNMP_ENCRYPT_NONE);
				   transport->setSecurityContext(context);

				   transport->sendMessage(response, 0);
				   delete response;
			   }
			   else if (pdu->getCommand() == SNMP_REPORT)
			   {
				   nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMPTrapReceiver: REPORT PDU with error %s"), (const TCHAR *)pdu->getVariable(0)->getName().toString());
			   }
            delete pdu;
         }
         else
         {
            // Sleep on error
            ThreadSleepMs(100);
         }
      }
   }

   delete snmp;
#ifdef WITH_IPV6
   delete snmp6;
#endif
   nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP Trap Receiver terminated"));
}

/**
 * Send all trap configuration records to client
 */
void SendTrapsToClient(ClientSession *pSession, UINT32 dwRqId)
{
   NXCPMessage msg;

   // Prepare message
   msg.setCode(CMD_TRAP_CFG_RECORD);
   msg.setId(dwRqId);

   s_trapCfgLock.lock();
   for(int i = 0; i < m_trapCfgList.size(); i++)
   {
      m_trapCfgList.get(i)->fillMessage(&msg);
      pSession->sendMessage(&msg);
      msg.deleteAllFields();
   }
   s_trapCfgLock.unlock();

   msg.setField(VID_TRAP_ID, (UINT32)0);
   pSession->sendMessage(&msg);
}

/**
 * Prepare single message with all trap configuration records
 */
void CreateTrapCfgMessage(NXCPMessage *msg)
{
   s_trapCfgLock.lock();
	msg->setField(VID_NUM_TRAPS, m_trapCfgList.size());
   for(int i = 0, id = VID_TRAP_INFO_BASE; i < m_trapCfgList.size(); i++, id += 10)
      m_trapCfgList.get(i)->fillMessage(msg, id);
   s_trapCfgLock.unlock();
}

static void NotifyOnTrapCfgDelete(uint32_t id)
{
	NXCPMessage msg(CMD_TRAP_CFG_UPDATE, 0);
	msg.setField(VID_NOTIFICATION_CODE, static_cast<uint32_t>(NX_NOTIFY_TRAPCFG_DELETED));
	msg.setField(VID_TRAP_ID, id);
	EnumerateClientSessions(NotifyOnTrapCfgChangeCB, &msg);
}

/**
 * Delete trap configuration record
 */
uint32_t DeleteTrap(uint32_t id)
{
   uint32_t rcc = RCC_INVALID_TRAP_ID;

   s_trapCfgLock.lock();

   for(int i = 0; i < m_trapCfgList.size(); i++)
   {
      if (m_trapCfgList.get(i)->getId() == id)
      {
         // Remove trap entry from database
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmtCfg = DBPrepare(hdb, _T("DELETE FROM snmp_trap_cfg WHERE trap_id=?"));
         DB_STATEMENT hStmtMap = DBPrepare(hdb, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=?"));

         if (hStmtCfg != nullptr && hStmtMap != nullptr)
         {
            DBBind(hStmtCfg, 1, DB_SQLTYPE_INTEGER, id);
            DBBind(hStmtMap, 1, DB_SQLTYPE_INTEGER, id);

            if (DBBegin(hdb))
            {
               if (DBExecute(hStmtCfg) && DBExecute(hStmtMap))
               {
                  m_trapCfgList.remove(i);
                  NotifyOnTrapCfgDelete(id);
                  rcc = RCC_SUCCESS;
                  DBCommit(hdb);
               }
               else
               {
                  DBRollback(hdb);
               }
               DBFreeStatement(hStmtCfg);
               DBFreeStatement(hStmtMap);
            }
         }
         DBConnectionPoolReleaseConnection(hdb);
         break;
      }
   }

   s_trapCfgLock.unlock();
   return rcc;
}

/**
 * Save parameter mapping to database
 */
bool SNMPTrapConfiguration::saveParameterMapping(DB_HANDLE hdb)
{
   if (!ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=?")))
      return false;

   if (m_mappings.isEmpty())
      return true;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description,flags) VALUES (?,?,?,?,?)"), true);
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

   bool success = true;
   TCHAR oid[1024];
   for(int i = 0; (i < m_mappings.size()) && success; i++)
   {
      const SNMPTrapParameterMapping *pm = m_mappings.get(i);
      if (!pm->isPositional())
         pm->getOid()->toString(oid, 1024);
      else
         _sntprintf(oid, 1024, _T("POS:%d"), pm->getPosition());

      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, oid, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, pm->getDescription(), DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, pm->getFlags());

      success = DBExecute(hStmt);
   }
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Create new trap configuration record
 */
uint32_t CreateNewTrap(uint32_t *trapId)
{
   uint32_t rcc = RCC_DB_FAILURE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_trap_cfg (guid,trap_id,snmp_oid,event_code,description,user_tag) VALUES (?,?,'',?,'','')"));
   if (hStmt != nullptr)
   {
      SNMPTrapConfiguration *trapCfg = new SNMPTrapConfiguration();
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, trapCfg->getGuid());
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, trapCfg->getId());
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, trapCfg->getEventCode());

      if (DBExecute(hStmt))
      {
         AddTrapCfgToList(trapCfg);
         trapCfg->notifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_CREATED);
         *trapId = trapCfg->getId();
         rcc = RCC_SUCCESS;
      }
      else
      {
         delete trapCfg;
      }

      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Update trap configuration record from message
 */
uint32_t UpdateTrapFromMsg(const NXCPMessage& msg)
{
   uint32_t rcc = RCC_INVALID_TRAP_ID;
   TCHAR oid[1024];

   uint32_t id = msg.getFieldAsUInt32(VID_TRAP_ID);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   for(int i = 0; i < m_trapCfgList.size(); i++)
   {
      if (m_trapCfgList.get(i)->getId() == id)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE snmp_trap_cfg SET snmp_oid=?,event_code=?,description=?,user_tag=?,transformation_script=? WHERE trap_id=?"));
         if (hStmt != nullptr)
         {
            SNMPTrapConfiguration *trapCfg = new SNMPTrapConfiguration(msg);
            trapCfg->getOid().toString(oid, 1024);
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, oid, DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, trapCfg->getEventCode());
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, trapCfg->getDescription(), DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, trapCfg->getEventTag(), DB_BIND_STATIC);
            DBBind(hStmt, 5, DB_SQLTYPE_TEXT, trapCfg->getScriptSource(), DB_BIND_STATIC);
            DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, trapCfg->getId());

            if (DBBegin(hdb))
            {
               if (DBExecute(hStmt) && trapCfg->saveParameterMapping(hdb))
               {
                  AddTrapCfgToList(trapCfg);
                  trapCfg->notifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_MODIFIED);
                  rcc = RCC_SUCCESS;
                  DBCommit(hdb);
               }
               else
               {
                  DBRollback(hdb);
                  rcc = RCC_DB_FAILURE;
               }
            }
            else
            {
               rcc = RCC_DB_FAILURE;
            }
            DBFreeStatement(hStmt);

            if (rcc != RCC_SUCCESS)
               delete trapCfg;
         }

			DBConnectionPoolReleaseConnection(hdb);
         break;
      }
   }

   return rcc;
}

/**
 * Create trap record in NXMP file
 */
void CreateTrapExportRecord(StringBuffer &xml, UINT32 id)
{
	TCHAR szBuffer[1024];
	SNMPTrapConfiguration *trapCfg;

	s_trapCfgLock.lock();
   for(int i = 0; i < m_trapCfgList.size(); i++)
   {
      trapCfg = m_trapCfgList.get(i);
      if (trapCfg->getId() == id)
      {
         xml.append(_T("\t\t<trap id=\""));
         xml.append(id);
         xml.append(_T("\">\n\t\t\t<guid>"));
         xml.append(trapCfg->getGuid());
         xml.append(_T("</guid>\n\t\t\t<oid>"));
         xml.append(trapCfg->getOid().toString());
         xml.append(_T("</oid>\n\t\t\t<description>"));
         xml.append(EscapeStringForXML2(trapCfg->getDescription()));
         xml.append(_T("</description>\n\t\t\t<eventTag>"));
         xml.append(EscapeStringForXML2(trapCfg->getEventTag()));
         xml.append(_T("</eventTag>\n\t\t\t<event>"));
         EventNameFromCode(trapCfg->getEventCode(), szBuffer);
         xml.append(EscapeStringForXML2(szBuffer));
         xml.append(_T("</event>\n\t\t\t<transformationScript>"));
         xml.append(EscapeStringForXML2(trapCfg->getScriptSource()));
         xml.append(_T("</transformationScript>\n"));
			if (trapCfg->getParameterMappingCount() > 0)
			{
            xml.append(_T("\t\t\t<parameters>\n"));
				for(int j = 0; j < trapCfg->getParameterMappingCount(); j++)
				{
				   const SNMPTrapParameterMapping *pm = trapCfg->getParameterMapping(j);
					xml.appendFormattedString(_T("\t\t\t\t<parameter id=\"%d\">\n")
			                             _T("\t\t\t\t\t<flags>%d</flags>\n")
					                       _T("\t\t\t\t\t<description>%s</description>\n"),
												  j + 1, pm->getFlags(),
												  (const TCHAR *)EscapeStringForXML2(pm->getDescription()));
               if (!pm->isPositional())
					{
						xml.appendFormattedString(_T("\t\t\t\t\t<oid>%s</oid>\n"), pm->getOid()->toString(szBuffer, 1024));
					}
					else
					{
						xml.appendFormattedString(_T("\t\t\t\t\t<position>%d</position>\n"), pm->getPosition());
					}
               xml.append(_T("\t\t\t\t</parameter>\n"));
				}
            xml.append(_T("\t\t\t</parameters>\n"));
			}
         xml.append(_T("\t\t</trap>\n"));
			break;
		}
	}
   s_trapCfgLock.unlock();
}

/**
 * Find if trap with guid already exists
 */
uint32_t ResolveTrapGuid(const uuid& guid)
{
   uint32_t id = 0;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT trap_id FROM snmp_trap_cfg WHERE guid=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            id = DBGetFieldULong(hResult, 0, 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return id;
}

/**
 * Add SNMP trap configuration to local list
 */
void AddTrapCfgToList(SNMPTrapConfiguration *trapCfg)
{
   s_trapCfgLock.lock();

   for(int i = 0; i < m_trapCfgList.size(); i++)
   {
      if (m_trapCfgList.get(i)->getId() == trapCfg->getId())
      {
         m_trapCfgList.remove(i);
      }
   }
   m_trapCfgList.add(trapCfg);

   s_trapCfgLock.unlock();
}
