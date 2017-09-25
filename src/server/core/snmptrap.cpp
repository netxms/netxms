/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Raden Solutions
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

#define BY_OBJECT_ID 0
#define BY_POSITION 1

/**
 * Total number of received SNMP traps
 */
UINT64 g_snmpTrapsReceived = 0;

/**
 * Max SNMP packet length
 */
#define MAX_PACKET_LENGTH     65536

/**
 * Static data
 */
static Mutex s_trapCfgLock;
static ObjectArray<SNMPTrapConfiguration> m_trapCfgList(16, 4, true);
static BOOL m_bLogAllTraps = FALSE;
static Mutex s_trapCountersLock;
static INT64 s_trapId = 1; // Next free trap ID
static bool s_allowVarbindConversion = true;
static UINT16 m_wTrapPort = 162;

/**
 * Create new SNMP trap configuration object
 */
SNMPTrapConfiguration::SNMPTrapConfiguration() : m_objectId(), m_mappings(8, 8, true)
{
   m_guid = uuid::generate();
   m_id = CreateUniqueId(IDG_SNMP_TRAP);
   m_eventCode = EVENT_SNMP_UNMATCHED_TRAP;
   nx_strncpy(m_description, _T(""), 1);
   nx_strncpy(m_userTag, _T(""), 1);
}

/**
 * Create SNMP trap configuration object from database
 */
SNMPTrapConfiguration::SNMPTrapConfiguration(DB_RESULT trapResult, DB_STATEMENT stmt, int row) : m_mappings(8, 8, true)
{
   m_id = DBGetFieldULong(trapResult, row, 0);
   TCHAR buffer[MAX_OID_LENGTH];
   m_objectId = SNMP_ObjectId::parse(DBGetField(trapResult, row, 1, buffer, MAX_OID_LENGTH));
   m_eventCode = DBGetFieldULong(trapResult, row, 2);
   DBGetField(trapResult, row, 3, m_description, MAX_DB_STRING);
   DBGetField(trapResult, row, 4, m_userTag, MAX_USERTAG_LENGTH);
   m_guid = DBGetFieldGUID(trapResult, row, 5);

   DBBind(stmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT mapResult = DBSelectPrepared(stmt);
   if (mapResult != NULL)
   {
      int mapCount = DBGetNumRows(mapResult);
      for(int i = 0; i < mapCount; i++)
      {
         SNMPTrapParameterMapping *param = new SNMPTrapParameterMapping(mapResult, i);
         if (!param->isPositional() && !param->getOid()->isValid())
            nxlog_write(MSG_INVALID_TRAP_ARG_OID, EVENTLOG_ERROR_TYPE, "sd", (const TCHAR *)param->getOid()->toString(), m_id);
         m_mappings.add(param);
      }
      DBFreeResult(mapResult);
   }
}

/**
 * Create SNMP trap configuration object from config entry
 */
SNMPTrapConfiguration::SNMPTrapConfiguration(ConfigEntry *entry, const uuid& guid, UINT32 id, UINT32 eventCode) : m_mappings(8, 8, true)
{
   if (id == 0)
      m_id = CreateUniqueId(IDG_SNMP_TRAP);
   else
      m_id = id;

   m_guid = guid;
   m_objectId = SNMP_ObjectId::parse(entry->getSubEntryValue(_T("oid"), 0, _T("")));
   m_eventCode = eventCode;
   nx_strncpy(m_description, entry->getSubEntryValue(_T("description"), 0, _T("")), MAX_DB_STRING);
   nx_strncpy(m_userTag, entry->getSubEntryValue(_T("userTag"), 0, _T("")), MAX_USERTAG_LENGTH);

   ConfigEntry *parametersRoot = entry->findEntry(_T("parameters"));
   if (parametersRoot != NULL)
   {
      ObjectArray<ConfigEntry> *parameters = parametersRoot->getOrderedSubEntries(_T("parameter#*"));
      if (parameters->size() > 0)
      {
         for(int i = 0; i < parameters->size(); i++)
         {
            SNMPTrapParameterMapping *param = new SNMPTrapParameterMapping(parameters->get(i));
            if (!param->isPositional() && !param->getOid()->isValid())
               nxlog_write(MSG_INVALID_TRAP_ARG_OID, EVENTLOG_ERROR_TYPE, "sd", (const TCHAR *)param->getOid()->toString(), m_id);
            m_mappings.add(param);
         }
      }
      delete parameters;
   }
}

/**
 * Create SNMP trap configuration object from NXCPMessage
 */
SNMPTrapConfiguration::SNMPTrapConfiguration(NXCPMessage *msg) : m_mappings(8, 8, true)
{
   UINT32 buffer[MAX_OID_LENGTH];

   m_id = msg->getFieldAsUInt32(VID_TRAP_ID);
   msg->getFieldAsInt32Array(VID_TRAP_OID, msg->getFieldAsUInt32(VID_TRAP_OID_LEN), buffer);
   m_objectId = SNMP_ObjectId(buffer, msg->getFieldAsUInt32(VID_TRAP_OID_LEN));
   m_eventCode = msg->getFieldAsUInt32(VID_EVENT_CODE);
   msg->getFieldAsString(VID_DESCRIPTION, m_description, MAX_DB_STRING);
   msg->getFieldAsString(VID_USER_TAG, m_userTag, MAX_USERTAG_LENGTH);

   // Read new mappings from message
   int count = msg->getFieldAsInt32(VID_TRAP_NUM_MAPS);
   UINT32 base = VID_TRAP_PBASE;
   for(int i = 0; i < count; i++, base += 10)
   {
      m_mappings.add(new SNMPTrapParameterMapping(msg, base));
   }
}

/**
 * Destructor for SNMP trap configuration object
 */
SNMPTrapConfiguration::~SNMPTrapConfiguration()
{
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
   msg->setField(VID_DESCRIPTION, m_description);
   msg->setField(VID_USER_TAG, m_userTag);

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
   msg->setField(base + 1, m_description);
   msg->setFieldFromInt32Array(base + 2, m_objectId.length(), m_objectId.value());
   msg->setField(base + 3, m_eventCode);
}

/**
 * Notify clients about trap configuration change
 */
void NotifyOnTrapCfgChangeCB(ClientSession *session, void *arg)
{
   if (session->isAuthenticated())
      session->postMessage((NXCPMessage *)arg);
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
   nx_strncpy(m_description, _T(""), 1);
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
      m_objectId = NULL;
      m_position = _tcstoul(&oid[4], NULL, 10);
   }
   else
   {
      m_objectId = new SNMP_ObjectId(SNMP_ObjectId::parse(oid));
      m_position = 0;
   }

   DBGetField(mapResult, row, 1, m_description, MAX_DB_STRING);
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
      m_objectId = NULL;
      m_position = position; // Positional parameter
   }
   else
   {
      m_objectId = new SNMP_ObjectId(SNMP_ObjectId::parse(entry->getSubEntryValue(_T("oid"), 0, _T("")))); // OID parameter
      m_position = 0;
   }

   nx_strncpy(m_description, entry->getSubEntryValue(_T("description"), 0, _T("")), MAX_DB_STRING);
   m_flags = entry->getSubEntryValueAsUInt(_T("flags"), 0, 0);
}

/**
 * Create SNMP trap parameter map object from NXCPMessage
 */
SNMPTrapParameterMapping::SNMPTrapParameterMapping(NXCPMessage *msg, UINT32 base)
{
   m_flags = msg->getFieldAsUInt32(base);
   msg->getFieldAsString(base + 1, m_description, MAX_DB_STRING);
   if (msg->getFieldAsUInt32(base + 2) == BY_POSITION)
   {
      m_objectId = NULL;
      m_position = msg->getFieldAsUInt32(base + 3);
   }
   else
   {
      UINT32 buffer[MAX_OID_LENGTH];
      msg->getFieldAsInt32Array(base + 3, msg->getFieldAsUInt32(base + 4), buffer);
      m_objectId = new SNMP_ObjectId(buffer, msg->getFieldAsUInt32(base + 4));
   }
}

/**
 * Destructor for SNMP trap parameter map object
 */
SNMPTrapParameterMapping::~SNMPTrapParameterMapping()
{
   delete m_objectId;
}

/**
 * Fill NXCP message with trap parameter map configuration data
 */
void SNMPTrapParameterMapping::fillMessage(NXCPMessage *msg, UINT32 base) const
{
   msg->setField(base, isPositional());
   if (isPositional())
      msg->setField(base + 1, m_position);
   else
      msg->setFieldFromInt32Array(base + 1, m_objectId->length(), m_objectId->value());
   msg->setField(base + 2, m_description);
   msg->setField(base + 3, m_flags);

}

/**
 * Load trap configuration from database
 */
void LoadTrapCfg()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Load traps
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT trap_id,snmp_oid,event_code,description,user_tag,guid FROM snmp_trap_cfg"));
   if (hResult != NULL)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT snmp_oid,description,flags FROM snmp_trap_pmap WHERE trap_id=? ORDER BY parameter"));
      if (hStmt != NULL)
      {
         int numRows = DBGetNumRows(hResult);
         for(int i = 0; i < numRows; i++)
         {
            SNMPTrapConfiguration *trapCfg = new SNMPTrapConfiguration(hResult, hStmt, i);
            if (!trapCfg->getOid().isValid())
            {
               TCHAR buffer[MAX_DB_STRING];
               nxlog_write(MSG_INVALID_TRAP_OID, NXLOG_ERROR, "s",
                           DBGetField(hResult, i, 1, buffer, MAX_DB_STRING));
            }
            m_trapCfgList.add(trapCfg);
         }
         DBFreeStatement(hStmt);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Initialize trap handling
 */
void InitTraps()
{
	LoadTrapCfg();
	m_bLogAllTraps = ConfigReadInt(_T("LogAllSNMPTraps"), FALSE);
	s_allowVarbindConversion = ConfigReadInt(_T("AllowTrapVarbindsConversion"), 1) ? true : false;

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(trap_id) FROM snmp_trap_log"));
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
		   s_trapId = DBGetFieldInt64(hResult, 0, 0) + 1;
		DBFreeResult(hResult);
	}
	DBConnectionPoolReleaseConnection(hdb);

	m_wTrapPort = (UINT16)ConfigReadULong(_T("SNMPTrapPort"), m_wTrapPort); // 162 by default;
}

/**
 * Generate event for matched trap
 */
static void GenerateTrapEvent(UINT32 dwObjectId, UINT32 dwIndex, SNMP_PDU *pdu, int sourcePort)
{
   TCHAR *argList[32], szBuffer[256];
   TCHAR *names[33];
   char szFormat[] = "sssssssssssssssssssssssssssssssss";
   SNMPTrapConfiguration *trapCfg = m_trapCfgList.get(dwIndex);
   int iResult;

   memset(argList, 0, sizeof(argList));
   memset(names, 0, sizeof(names));
   names[0] = (TCHAR *)_T("oid");

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
         if (varbind != NULL)
         {
				bool convertToHex = true;
				argList[i] = _tcsdup(
               (s_allowVarbindConversion && !(pm->getFlags() & TRAP_VARBIND_FORCE_TEXT)) ?
                  varbind->getValueAsPrintableString(szBuffer, 256, &convertToHex) :
                  varbind->getValueAsString(szBuffer, 256));
				names[i + 1] = _tcsdup(varbind->getName().toString());
         }
      }
      else
      {
			// Extract by varbind OID
         for(int j = 0; j < pdu->getNumVariables(); j++)
         {
            SNMP_Variable *varbind = pdu->getVariable(j);
            iResult = varbind->getName().compare(*(pm->getOid()));
            if ((iResult == OID_EQUAL) || (iResult == OID_LONGER))
            {
					bool convertToHex = true;
					argList[i] = _tcsdup(
                  (s_allowVarbindConversion && !(pm->getFlags() & TRAP_VARBIND_FORCE_TEXT)) ?
                     varbind->getValueAsPrintableString(szBuffer, 256, &convertToHex) :
                     varbind->getValueAsString(szBuffer, 256));
	            names[i + 1] = _tcsdup(varbind->getName().toString());
               break;
            }
         }
      }
   }

   argList[numMaps] = (TCHAR *)malloc(16 * sizeof(TCHAR));
   _sntprintf(argList[numMaps], 16, _T("%d"), sourcePort);
   names[numMaps + 1] = (TCHAR *)_T("sourcePort");
   szFormat[numMaps + 2] = 0;
   PostEventWithTagAndNames(
            trapCfg->getEventCode(), dwObjectId,
            trapCfg->getUserTag(), szFormat, (const TCHAR **)names,
            (const TCHAR *)pdu->getTrapId()->toString(),
            argList[0], argList[1], argList[2], argList[3],
            argList[4], argList[5], argList[6], argList[7],
            argList[8], argList[9], argList[10], argList[11],
            argList[12], argList[13], argList[14], argList[15],
            argList[16], argList[17], argList[18], argList[19],
            argList[20], argList[21], argList[22], argList[23],
            argList[24], argList[25], argList[26], argList[27],
            argList[28], argList[29], argList[30], argList[31]);

   for(int i = 0; i < numMaps; i++)
   {
      free(argList[i]);
      free(names[i + 1]);
   }
   free(argList[numMaps]);
}

/**
 * Handler for EnumerateSessions()
 */
static void BroadcastNewTrap(ClientSession *pSession, void *pArg)
{
   pSession->onNewSNMPTrap((NXCPMessage *)pArg);
}

/**
 * Process trap
 */
void ProcessTrap(SNMP_PDU *pdu, const InetAddress& srcAddr, UINT32 zoneUIN, int srcPort, SNMP_Transport *snmpTransport, SNMP_Engine *localEngine, bool isInformRq)
{
   UINT32 dwBufPos, dwBufSize;
   TCHAR *pszTrapArgs, szBuffer[4096];
   SNMP_Variable *pVar;
	BOOL processed = FALSE;
   int iResult;

   nxlog_debug(4, _T("Received SNMP %s %s from %s"), isInformRq ? _T("INFORM-REQUEST") : _T("TRAP"),
             pdu->getTrapId()->toString(&szBuffer[96], 4000), srcAddr.toString(szBuffer));
   s_trapCountersLock.lock();
   g_snmpTrapsReceived++; // FIXME: change to 64 bit volatile counter
   s_trapCountersLock.unlock();

	if (isInformRq)
	{
		SNMP_PDU *response = new SNMP_PDU(SNMP_RESPONSE, pdu->getRequestId(), pdu->getVersion());
		if (snmpTransport->getSecurityContext() == NULL)
		{
		   snmpTransport->setSecurityContext(new SNMP_SecurityContext(pdu->getCommunity()));
		}
		response->setMessageId(pdu->getMessageId());
		response->setContextEngineId(localEngine->getId(), localEngine->getIdLen());
		snmpTransport->sendMessage(response);
		delete response;
	}

   // Match IP address to object
   Node *node = FindNodeByIP((g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) ? ALL_ZONES : zoneUIN, srcAddr);

   // Write trap to log if required
   if (m_bLogAllTraps || (node != NULL))
   {
      NXCPMessage msg;
      TCHAR szQuery[8192], oidText[1024];
      UINT32 dwTimeStamp = (UINT32)time(NULL);

      dwBufSize = pdu->getNumVariables() * 4096 + 16;
      pszTrapArgs = (TCHAR *)malloc(sizeof(TCHAR) * dwBufSize);
      pszTrapArgs[0] = 0;
      dwBufPos = 0;
		for(int i = (pdu->getVersion() == SNMP_VERSION_1) ? 0 : 2; i < pdu->getNumVariables(); i++)
      {
         pVar = pdu->getVariable((int)i);
			bool convertToHex = true;
         dwBufPos += _sntprintf(&pszTrapArgs[dwBufPos], dwBufSize - dwBufPos, _T("%s%s == '%s'"),
                                (dwBufPos == 0) ? _T("") : _T("; "),
                                pVar->getName().toString(oidText, 1024),
										  s_allowVarbindConversion ? pVar->getValueAsPrintableString(szBuffer, 3000, &convertToHex) : pVar->getValueAsString(szBuffer, 3000));
      }

      // Write new trap to database
		s_trapCountersLock.lock();
		UINT64 trapId = s_trapId++; // FIXME: change to 64 bit volatile counter
		s_trapCountersLock.unlock();
      _sntprintf(szQuery, 8192, _T("INSERT INTO snmp_trap_log (trap_id,trap_timestamp,")
                                _T("ip_addr,object_id,trap_oid,trap_varlist) VALUES ")
                                _T("(") INT64_FMT _T(",%d,'%s',%d,'%s',%s)"),
                 trapId, dwTimeStamp, srcAddr.toString(szBuffer),
                 (node != NULL) ? node->getId() : (UINT32)0, pdu->getTrapId()->toString(oidText, 1024),
                 (const TCHAR *)DBPrepareString(g_dbDriver, pszTrapArgs));
      QueueSQLRequest(szQuery);

      // Notify connected clients
      msg.setCode(CMD_TRAP_LOG_RECORDS);
      msg.setField(VID_NUM_RECORDS, (UINT32)1);
      msg.setField(VID_RECORDS_ORDER, (WORD)RECORD_ORDER_NORMAL);
      msg.setField(VID_TRAP_LOG_MSG_BASE, trapId);
      msg.setField(VID_TRAP_LOG_MSG_BASE + 1, dwTimeStamp);
      msg.setField(VID_TRAP_LOG_MSG_BASE + 2, srcAddr);
      msg.setField(VID_TRAP_LOG_MSG_BASE + 3, (node != NULL) ? node->getId() : (UINT32)0);
      msg.setField(VID_TRAP_LOG_MSG_BASE + 4, pdu->getTrapId()->toString(oidText, 1024));
      msg.setField(VID_TRAP_LOG_MSG_BASE + 5, pszTrapArgs);
      EnumerateClientSessions(BroadcastNewTrap, &msg);
      free(pszTrapArgs);
   }

   // Process trap if it is coming from host registered in database
   if (node != NULL)
   {
      nxlog_debug(4, _T("ProcessTrap: trap matched to node %s [%d]"), node->getName(), node->getId());
      node->incSnmpTrapCount();
      if ((node->getStatus() != STATUS_UNMANAGED) || (g_flags & AF_TRAPS_FROM_UNMANAGED_NODES))
      {
         // Pass trap to loaded modules
         if (!(g_flags & AF_SHUTDOWN))
         {
            for(UINT32 i = 0; i < g_dwNumModules; i++)
            {
               if (g_pModuleList[i].pfTrapHandler != NULL)
               {
                  if (g_pModuleList[i].pfTrapHandler(pdu, node))
				      {
					      processed = TRUE;
                     break;   // Trap was processed by the module
				      }
               }
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
               iResult = pdu->getTrapId()->compare(trapCfg->getOid());
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
            GenerateTrapEvent(node->getId(), matchIndex, pdu, srcPort);
         }
         else     // Process unmatched traps
         {
            // Handle unprocessed traps
            if (!processed)
            {
               TCHAR oidText[1024];

               // Build trap's parameters string
               dwBufSize = pdu->getNumVariables() * 4096 + 16;
               pszTrapArgs = (TCHAR *)malloc(sizeof(TCHAR) * dwBufSize);
               pszTrapArgs[0] = 0;
               for(int i = (pdu->getVersion() == SNMP_VERSION_1) ? 0 : 2, dwBufPos = 0; i < pdu->getNumVariables(); i++)
               {
                  pVar = pdu->getVariable(i);
					   bool convertToHex = true;
                  dwBufPos += _sntprintf(&pszTrapArgs[dwBufPos], dwBufSize - dwBufPos, _T("%s%s == '%s'"),
                                         (dwBufPos == 0) ? _T("") : _T("; "),
                                         pVar->getName().toString(oidText, 1024),
												     s_allowVarbindConversion ? pVar->getValueAsPrintableString(szBuffer, 512, &convertToHex) : pVar->getValueAsString(szBuffer, 512));
               }

               // Generate default event for unmatched traps
               const TCHAR *names[3] = { _T("oid"), NULL, _T("sourcePort") };
               PostEventWithNames(EVENT_SNMP_UNMATCHED_TRAP, node->getId(), "ssd", names,
                  pdu->getTrapId()->toString(oidText, 1024), pszTrapArgs, srcPort);
               free(pszTrapArgs);
            }
         }
         s_trapCfgLock.unlock();
      }
      else
      {
         nxlog_debug(4, _T("ProcessTrap: Node %s [%d] is in UNMANAGED state, trap ignored"), node->getName(), node->getId());
      }
   }
   else if (g_flags & AF_SNMP_TRAP_DISCOVERY)  // unknown node, discovery enabled
   {
      nxlog_debug(4, _T("ProcessTrap: trap not matched to node, adding new IP address %s for discovery"), srcAddr.toString(szBuffer));
      CheckPotentialNode(srcAddr, zoneUIN);
   }
   else  // unknown node, discovery disabled
   {
      nxlog_debug(4, _T("ProcessTrap: trap not matched to any node"));
   }
}

/**
 * Context finder - tries to find SNMPv3 security context by IP address
 */
static SNMP_SecurityContext *ContextFinder(struct sockaddr *addr, socklen_t addrLen)
{
   InetAddress ipAddr = InetAddress::createFromSockaddr(addr);
	Node *node = FindNodeByIP((g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) ? ALL_ZONES : 0, ipAddr);
	TCHAR buffer[64];
	nxlog_debug(6, _T("SNMPTrapReceiver: looking for SNMP security context for node %s %s"),
      ipAddr.toString(buffer), (node != NULL) ? node->getName() : _T("<unknown>"));
	return (node != NULL) ? node->getSnmpSecurityContext() : NULL;
}

/**
 * Create SNMP transport for receiver
 */
static SNMP_Transport *CreateTransport(SOCKET hSocket)
{
   if (hSocket == INVALID_SOCKET)
      return NULL;

   SNMP_Transport *t = new SNMP_UDPTransport(hSocket);
	t->enableEngineIdAutoupdate(true);
	t->setPeerUpdatedOnRecv(true);
   return t;
}

/**
 * SNMP trap receiver thread
 */
THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *pArg)
{
   static BYTE engineId[] = { 0x80, 0x00, 0x00, 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 };
   SNMP_Engine localEngine(engineId, 12);

   ThreadSetName("SNMPTrapRecv");

   SOCKET hSocket = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = socket(AF_INET6, SOCK_DGRAM, 0);
#endif

#ifdef WITH_IPV6
   if ((hSocket == INVALID_SOCKET) && (hSocket6 == INVALID_SOCKET))
#else
   if (hSocket == INVALID_SOCKET)
#endif
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("SNMPTrapReceiver"));
      return THREAD_OK;
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
   servAddr.sin_port = htons(m_wTrapPort);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(m_wTrapPort);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   DbgPrintf(5, _T("Trying to bind on UDP %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", m_wTrapPort, _T("SNMPTrapReceiver"), WSAGetLastError());
      bindFailures++;
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }

#ifdef WITH_IPV6
   DbgPrintf(5, _T("Trying to bind on UDP [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", m_wTrapPort, _T("SNMPTrapReceiver"), WSAGetLastError());
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
      DbgPrintf(1, _T("SNMP trap receiver aborted - cannot bind at least one socket"));
      return THREAD_OK;
   }

   if (hSocket != INVALID_SOCKET)
	   nxlog_write(MSG_LISTENING_FOR_SNMP, EVENTLOG_INFORMATION_TYPE, "ad", ntohl(servAddr.sin_addr.s_addr), m_wTrapPort);
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
	   nxlog_write(MSG_LISTENING_FOR_SNMP, EVENTLOG_INFORMATION_TYPE, "Hd", servAddr6.sin6_addr.s6_addr, m_wTrapPort);
#endif

   SNMP_Transport *snmp = CreateTransport(hSocket);
#ifdef WITH_IPV6
   SNMP_Transport *snmp6 = CreateTransport(hSocket6);
#endif

   SocketPoller sp;

   DbgPrintf(1, _T("SNMP Trap Receiver started on port %u"), m_wTrapPort);

   // Wait for packets
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
         if ((bytes > 0) && (pdu != NULL))
         {
            InetAddress sourceAddr = InetAddress::createFromSockaddr((struct sockaddr *)&addr);
            DbgPrintf(6, _T("SNMPTrapReceiver: received PDU of type %d from %s"), pdu->getCommand(), (const TCHAR *)sourceAddr.toString());
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
				   DbgPrintf(6, _T("SNMPTrapReceiver: EngineId discovery"));

				   SNMP_PDU *response = new SNMP_PDU(SNMP_REPORT, pdu->getRequestId(), pdu->getVersion());
				   response->setReportable(false);
				   response->setMessageId(pdu->getMessageId());
				   response->setContextEngineId(localEngine.getId(), localEngine.getIdLen());

				   SNMP_Variable *var = new SNMP_Variable(_T(".1.3.6.1.6.3.15.1.1.4.0"));
				   var->setValueFromString(ASN_INTEGER, _T("2"));
				   response->bindVariable(var);

				   SNMP_SecurityContext *context = new SNMP_SecurityContext();
				   localEngine.setTime((int)time(NULL));
				   context->setAuthoritativeEngine(localEngine);
				   context->setSecurityModel(SNMP_SECURITY_MODEL_USM);
				   context->setAuthMethod(SNMP_AUTH_NONE);
				   context->setPrivMethod(SNMP_ENCRYPT_NONE);
				   transport->setSecurityContext(context);

				   transport->sendMessage(response);
				   delete response;
			   }
			   else if (pdu->getCommand() == SNMP_REPORT)
			   {
				   DbgPrintf(6, _T("SNMPTrapReceiver: REPORT PDU with error %s"), (const TCHAR *)pdu->getVariable(0)->getName().toString());
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
   DbgPrintf(1, _T("SNMP Trap Receiver terminated"));
   return THREAD_OK;
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

static void NotifyOnTrapCfgDelete(UINT32 id)
{
	NXCPMessage msg;

	msg.setCode(CMD_TRAP_CFG_UPDATE);
	msg.setField(VID_NOTIFICATION_CODE, (UINT32)NX_NOTIFY_TRAPCFG_DELETED);
	msg.setField(VID_TRAP_ID, id);
	EnumerateClientSessions(NotifyOnTrapCfgChangeCB, &msg);
}

/**
 * Delete trap configuration record
 */
UINT32 DeleteTrap(UINT32 id)
{
   UINT32 dwResult = RCC_INVALID_TRAP_ID;

   s_trapCfgLock.lock();

   for(int i = 0; i < m_trapCfgList.size(); i++)
   {
      if (m_trapCfgList.get(i)->getId() == id)
      {
         // Remove trap entry from database
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmtCfg = DBPrepare(hdb, _T("DELETE FROM snmp_trap_cfg WHERE trap_id=?"));
         DB_STATEMENT hStmtMap = DBPrepare(hdb, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=?"));

         if (hStmtCfg != NULL && hStmtMap != NULL)
         {
            DBBind(hStmtCfg, 1, DB_SQLTYPE_INTEGER, id);
            DBBind(hStmtMap, 1, DB_SQLTYPE_INTEGER, id);

            if (DBBegin(hdb))
            {
               if (DBExecute(hStmtCfg) && DBExecute(hStmtMap))
               {
                  m_trapCfgList.remove(i);
                  NotifyOnTrapCfgDelete(id);
                  dwResult = RCC_SUCCESS;
                  DBCommit(hdb);
               }
               else
                  DBRollback(hdb);

               DBFreeStatement(hStmtCfg);
               DBFreeStatement(hStmtMap);
            }
         }
         DBConnectionPoolReleaseConnection(hdb);
         break;
      }
   }

   s_trapCfgLock.unlock();
   return dwResult;
}

/**
 * Save parameter mapping to database
 */
bool SNMPTrapConfiguration::saveParameterMapping(DB_HANDLE hdb)
{
   bool result = true;
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=?"));
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

	if (DBExecute(hStmt))
   {
	   DBFreeStatement(hStmt);

		hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description,flags) VALUES (?,?,?,?,?)"));
		if (hStmt != NULL)
		{
         TCHAR oid[1024];
			for(int i = 0; i < m_mappings.size(); i++)
			{
			   const SNMPTrapParameterMapping *pm = m_mappings.get(i);
				if (!pm->isPositional())
				   pm->getOid()->toString(oid, 1024);
				else
					_sntprintf(oid, 1024, _T("POS:%d"), pm->getPosition());

				DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
				DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
				DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, oid, DB_BIND_STATIC);
				DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, pm->getDescription(), DB_BIND_STATIC);
				DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, pm->getFlags());

				if (!DBExecute(hStmt))
				{
				   result = false;
				   break;
				}
			}
			DBFreeStatement(hStmt);
			return result;
		}
   }

	return false;
}

/**
 * Create new trap configuration record
 */
UINT32 CreateNewTrap(UINT32 *pdwTrapId)
{
   UINT32 rcc = RCC_DB_FAILURE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_trap_cfg (guid,trap_id,snmp_oid,event_code,description,user_tag) VALUES (?,?,'',?,'','')"));
   if (hStmt != NULL)
   {
      SNMPTrapConfiguration *trapCfg = new SNMPTrapConfiguration();
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, trapCfg->getGuid());
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, trapCfg->getId());
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, trapCfg->getEventCode());

      if (DBExecute(hStmt))
      {
         AddTrapCfgToList(trapCfg);
         trapCfg->notifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_CREATED);
         *pdwTrapId = trapCfg->getId();
         rcc = RCC_SUCCESS;
      }
      else
         delete trapCfg;

      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   return rcc;
}

/**
 * Update trap configuration record from message
 */
UINT32 UpdateTrapFromMsg(NXCPMessage *pMsg)
{
   UINT32 rcc = RCC_INVALID_TRAP_ID;
   TCHAR oid[1024];

   UINT32 id = pMsg->getFieldAsUInt32(VID_TRAP_ID);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   for(int i = 0; i < m_trapCfgList.size(); i++)
   {
      if (m_trapCfgList.get(i)->getId() == id)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE snmp_trap_cfg SET snmp_oid=?,event_code=?,description=?,user_tag=? WHERE trap_id=?"));
         if (hStmt != NULL)
         {
            SNMPTrapConfiguration *trapCfg = new SNMPTrapConfiguration(pMsg);
            trapCfg->getOid().toString(oid, 1024);
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, oid, DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, trapCfg->getEventCode());
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, trapCfg->getDescription(), DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, trapCfg->getUserTag(), DB_BIND_STATIC);
            DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, trapCfg->getId());

            if(DBBegin(hdb))
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
               rcc = RCC_DB_FAILURE;
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
void CreateTrapExportRecord(String &xml, UINT32 id)
{
	TCHAR szBuffer[1024];
	SNMPTrapConfiguration *trapCfg;

	s_trapCfgLock.lock();
   for(int i = 0; i < m_trapCfgList.size(); i++)
   {
      trapCfg = m_trapCfgList.get(i);
      if (trapCfg->getId() == id)
      {
			xml.appendFormattedString(_T("\t\t<trap id=\"%d\">\n")
			                       _T("\t\t\t<guid>%s</guid>\n")
			                       _T("\t\t\t<oid>%s</oid>\n")
			                       _T("\t\t\t<description>%s</description>\n")
			                       _T("\t\t\t<userTag>%s</userTag>\n"), id,
                                (const TCHAR *)trapCfg->getGuid().toString(),
                                (const TCHAR *)trapCfg->getOid().toString(),
										  (const TCHAR *)EscapeStringForXML2(trapCfg->getDescription()),
										  (const TCHAR *)EscapeStringForXML2(trapCfg->getUserTag()));

		   EventNameFromCode(trapCfg->getEventCode(), szBuffer);
			xml.appendFormattedString(_T("\t\t\t<event>%s</event>\n"), (const TCHAR *)EscapeStringForXML2(szBuffer));
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
UINT32 ResolveTrapGuid(const uuid& guid)
{
   UINT32 id = 0;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT trap_id FROM snmp_trap_cfg WHERE guid=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
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
