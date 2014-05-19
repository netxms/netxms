/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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


/**
 * Externals
 */
extern Queue g_nodePollerQueue;

/**
 * Max SNMP packet length
 */
#define MAX_PACKET_LENGTH     65536

/**
 * Static data
 */
static MUTEX m_mutexTrapCfgAccess = NULL;
static NXC_TRAP_CFG_ENTRY *m_pTrapCfg = NULL;
static UINT32 m_dwNumTraps = 0;
static BOOL m_bLogAllTraps = FALSE;
static INT64 m_qnTrapId = 1;
static bool s_allowVarbindConversion = true;
static UINT16 m_wTrapPort = 162;

/**
 * Load trap configuration from database
 */
static BOOL LoadTrapCfg()
{
   DB_RESULT hResult;
   BOOL bResult = TRUE;
   TCHAR *pszOID, szQuery[256], szBuffer[MAX_DB_STRING];
   UINT32 i, j, pdwBuffer[MAX_OID_LEN];

   // Load traps
   hResult = DBSelect(g_hCoreDB, _T("SELECT trap_id,snmp_oid,event_code,description,user_tag FROM snmp_trap_cfg"));
   if (hResult != NULL)
   {
      m_dwNumTraps = DBGetNumRows(hResult);
      m_pTrapCfg = (NXC_TRAP_CFG_ENTRY *)malloc(sizeof(NXC_TRAP_CFG_ENTRY) * m_dwNumTraps);
      memset(m_pTrapCfg, 0, sizeof(NXC_TRAP_CFG_ENTRY) * m_dwNumTraps);
      for(i = 0; i < m_dwNumTraps; i++)
      {
         m_pTrapCfg[i].dwId = DBGetFieldULong(hResult, i, 0);
         m_pTrapCfg[i].dwOidLen = (UINT32)SNMPParseOID(DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING),
                                                       pdwBuffer, MAX_OID_LEN);
         if (m_pTrapCfg[i].dwOidLen > 0)
         {
            m_pTrapCfg[i].pdwObjectId = (UINT32 *)nx_memdup(pdwBuffer, m_pTrapCfg[i].dwOidLen * sizeof(UINT32));
         }
         else
         {
            nxlog_write(MSG_INVALID_TRAP_OID, NXLOG_ERROR, "s",
                        DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING));
            bResult = FALSE;
         }
         m_pTrapCfg[i].dwEventCode = DBGetFieldULong(hResult, i, 2);
         DBGetField(hResult, i, 3, m_pTrapCfg[i].szDescription, MAX_DB_STRING);
         DBGetField(hResult, i, 4, m_pTrapCfg[i].szUserTag, MAX_USERTAG_LENGTH);
      }
      DBFreeResult(hResult);

      // Load parameter mappings
      for(i = 0; i < m_dwNumTraps; i++)
      {
         _sntprintf(szQuery, 256, _T("SELECT snmp_oid,description,flags FROM snmp_trap_pmap ")
                                  _T("WHERE trap_id=%d ORDER BY parameter"),
                    m_pTrapCfg[i].dwId);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            m_pTrapCfg[i].dwNumMaps = DBGetNumRows(hResult);
            m_pTrapCfg[i].pMaps = (NXC_OID_MAP *)malloc(sizeof(NXC_OID_MAP) * m_pTrapCfg[i].dwNumMaps);
            for(j = 0; j < m_pTrapCfg[i].dwNumMaps; j++)
            {
               pszOID = DBGetField(hResult, j, 0, szBuffer, MAX_DB_STRING);
               if (!_tcsncmp(pszOID, _T("POS:"), 4))
               {
                  m_pTrapCfg[i].pMaps[j].dwOidLen = _tcstoul(&pszOID[4], NULL, 10) | 0x80000000;
                  m_pTrapCfg[i].pMaps[j].pdwObjectId = NULL;
               }
               else
               {
                  m_pTrapCfg[i].pMaps[j].dwOidLen = (UINT32)SNMPParseOID(pszOID, pdwBuffer, MAX_OID_LEN);
                  if (m_pTrapCfg[i].pMaps[j].dwOidLen > 0)
                  {
                     m_pTrapCfg[i].pMaps[j].pdwObjectId = 
                        (UINT32 *)nx_memdup(pdwBuffer, m_pTrapCfg[i].pMaps[j].dwOidLen * sizeof(UINT32));
                  }
                  else
                  {
                     nxlog_write(MSG_INVALID_TRAP_ARG_OID, EVENTLOG_ERROR_TYPE, "sd", 
                              DBGetField(hResult, j, 0, szBuffer, MAX_DB_STRING), m_pTrapCfg[i].dwId);
                     bResult = FALSE;
                  }
               }
               DBGetField(hResult, j, 1, m_pTrapCfg[i].pMaps[j].szDescription, MAX_DB_STRING);
					m_pTrapCfg[i].pMaps[j].dwFlags = DBGetFieldULong(hResult, j, 2);
            }
            DBFreeResult(hResult);
         }
         else
         {
            bResult = FALSE;
         }
      }
   }
   else
   {
      bResult = FALSE;
   }
   return bResult;
}

/**
 * Initialize trap handling
 */
void InitTraps()
{
	DB_RESULT hResult;

	m_mutexTrapCfgAccess = MutexCreate();
	LoadTrapCfg();
	m_bLogAllTraps = ConfigReadInt(_T("LogAllSNMPTraps"), FALSE);
	s_allowVarbindConversion = ConfigReadInt(_T("AllowTrapVarbindsConversion"), 1) ? true : false;

	hResult = DBSelect(g_hCoreDB, _T("SELECT max(trap_id) FROM snmp_trap_log"));
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
			m_qnTrapId = DBGetFieldInt64(hResult, 0, 0) + 1;
		DBFreeResult(hResult);
	}

	m_wTrapPort = (UINT16)ConfigReadULong(_T("SNMPTrapPort"), m_wTrapPort); // 162 by default;
}

/**
 * Generate event for matched trap
 */
static void GenerateTrapEvent(UINT32 dwObjectId, UINT32 dwIndex, SNMP_PDU *pdu)
{
   TCHAR *pszArgList[32], szBuffer[256];
   char szFormat[] = "sssssssssssssssssssssssssssssssss";
   UINT32 i;
   SNMP_Variable *pVar;
   int iResult;

	// Extract varbinds from trap and add them as event's parameters
   for(i = 0; i < m_pTrapCfg[dwIndex].dwNumMaps; i++)
   {
      if (m_pTrapCfg[dwIndex].pMaps[i].dwOidLen & 0x80000000)
      {
			// Extract by varbind position
         pVar = pdu->getVariable((m_pTrapCfg[dwIndex].pMaps[i].dwOidLen & 0x7FFFFFFF) - 1);
         if (pVar != NULL)
         {
				bool convertToHex = true;
				pszArgList[i] = _tcsdup((s_allowVarbindConversion && !(m_pTrapCfg[dwIndex].pMaps[i].dwFlags & TRAP_VARBIND_FORCE_TEXT)) ? pVar->getValueAsPrintableString(szBuffer, 256, &convertToHex) : pVar->getValueAsString(szBuffer, 256));
         }
         else
         {
            pszArgList[i] = _tcsdup(_T(""));
         }
      }
      else
      {
			// Extract by varbind OID
         size_t j;
         for(j = 0; j < pdu->getNumVariables(); j++)
         {
            iResult = pdu->getVariable((int)j)->getName()->compare(
                  m_pTrapCfg[dwIndex].pMaps[i].pdwObjectId,
                  m_pTrapCfg[dwIndex].pMaps[i].dwOidLen);
            if ((iResult == OID_EQUAL) || (iResult == OID_SHORTER))
            {
					bool convertToHex = true;
					pszArgList[i] = _tcsdup(
                  (s_allowVarbindConversion && !(m_pTrapCfg[dwIndex].pMaps[i].dwFlags & TRAP_VARBIND_FORCE_TEXT)) ? 
                     pdu->getVariable((int)j)->getValueAsPrintableString(szBuffer, 256, &convertToHex) : 
                     pdu->getVariable((int)j)->getValueAsString(szBuffer, 256));
               break;
            }
         }
         if (j == pdu->getNumVariables())
            pszArgList[i] = _tcsdup(_T(""));
      }
   }

   szFormat[m_pTrapCfg[dwIndex].dwNumMaps + 1] = 0;
   PostEventWithTag(m_pTrapCfg[dwIndex].dwEventCode, dwObjectId,
	                 m_pTrapCfg[dwIndex].szUserTag,
	                 szFormat, pdu->getTrapId()->getValueAsText(),
                    pszArgList[0], pszArgList[1], pszArgList[2], pszArgList[3],
                    pszArgList[4], pszArgList[5], pszArgList[6], pszArgList[7],
                    pszArgList[8], pszArgList[9], pszArgList[10], pszArgList[11],
                    pszArgList[12], pszArgList[13], pszArgList[14], pszArgList[15],
                    pszArgList[16], pszArgList[17], pszArgList[18], pszArgList[19],
                    pszArgList[20], pszArgList[21], pszArgList[22], pszArgList[23],
                    pszArgList[24], pszArgList[25], pszArgList[26], pszArgList[27],
                    pszArgList[28], pszArgList[29], pszArgList[30], pszArgList[31]);

   for(i = 0; i < m_pTrapCfg[dwIndex].dwNumMaps; i++)
      free(pszArgList[i]);
}

/**
 * Handler for EnumerateSessions()
 */
static void BroadcastNewTrap(ClientSession *pSession, void *pArg)
{
   pSession->onNewSNMPTrap((CSCPMessage *)pArg);
}

/**
 * Process trap
 */
static void ProcessTrap(SNMP_PDU *pdu, struct sockaddr_in *pOrigin, SNMP_Transport *pTransport, SNMP_Engine *localEngine, bool isInformRq)
{
   UINT32 dwOriginAddr, dwBufPos, dwBufSize, dwMatchLen, dwMatchIdx;
   TCHAR *pszTrapArgs, szBuffer[4096];
   SNMP_Variable *pVar;
   Node *pNode;
	BOOL processed = FALSE;
   int iResult;

   dwOriginAddr = ntohl(pOrigin->sin_addr.s_addr);
   DbgPrintf(4, _T("Received SNMP %s %s from %s"), isInformRq ? _T("INFORM-REQUEST") : _T("TRAP"),
             pdu->getTrapId()->getValueAsText(), IpToStr(dwOriginAddr, szBuffer));
	if (isInformRq)
	{
		SNMP_PDU *response = new SNMP_PDU(SNMP_RESPONSE, pdu->getRequestId(), pdu->getVersion());
		if (pTransport->getSecurityContext() == NULL)
		{
			pTransport->setSecurityContext(new SNMP_SecurityContext(pdu->getCommunity()));
		}
		response->setMessageId(pdu->getMessageId());
		response->setContextEngineId(localEngine->getId(), localEngine->getIdLen());
		pTransport->sendMessage(response);
		delete response;
	}

   // Match IP address to object
   pNode = FindNodeByIP(0, dwOriginAddr);

   // Write trap to log if required
   if (m_bLogAllTraps || (pNode != NULL))
   {
      CSCPMessage msg;
      TCHAR szQuery[8192];
      UINT32 dwTimeStamp = (UINT32)time(NULL);

      dwBufSize = pdu->getNumVariables() * 4096 + 16;
      pszTrapArgs = (TCHAR *)malloc(sizeof(TCHAR) * dwBufSize);
      pszTrapArgs[0] = 0;
		for(size_t i = (pdu->getVersion() == SNMP_VERSION_1) ? 0 : 2, dwBufPos = 0; i < pdu->getNumVariables(); i++)
      {
         pVar = pdu->getVariable((int)i);
			bool convertToHex = true;
         dwBufPos += _sntprintf(&pszTrapArgs[dwBufPos], dwBufSize - dwBufPos, _T("%s%s == '%s'"),
                                (dwBufPos == 0) ? _T("") : _T("; "),
                                pVar->getName()->getValueAsText(), 
										  s_allowVarbindConversion ? pVar->getValueAsPrintableString(szBuffer, 3000, &convertToHex) : pVar->getValueAsString(szBuffer, 3000));
      }

      // Write new trap to database
      _sntprintf(szQuery, 8192, _T("INSERT INTO snmp_trap_log (trap_id,trap_timestamp,")
                                _T("ip_addr,object_id,trap_oid,trap_varlist) VALUES ")
                                _T("(") INT64_FMT _T(",%d,'%s',%d,'%s',%s)"),
                 m_qnTrapId, dwTimeStamp, IpToStr(dwOriginAddr, szBuffer),
                 (pNode != NULL) ? pNode->Id() : (UINT32)0, pdu->getTrapId()->getValueAsText(),
                 (const TCHAR *)DBPrepareString(g_hCoreDB, pszTrapArgs));
      QueueSQLRequest(szQuery);

      // Notify connected clients
      msg.SetCode(CMD_TRAP_LOG_RECORDS);
      msg.SetVariable(VID_NUM_RECORDS, (UINT32)1);
      msg.SetVariable(VID_RECORDS_ORDER, (WORD)RECORD_ORDER_NORMAL);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE, (QWORD)m_qnTrapId);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 1, dwTimeStamp);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 2, dwOriginAddr);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 3, (pNode != NULL) ? pNode->Id() : (UINT32)0);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 4, (TCHAR *)pdu->getTrapId()->getValueAsText());
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 5, pszTrapArgs);
      EnumerateClientSessions(BroadcastNewTrap, &msg);
      free(pszTrapArgs);

      m_qnTrapId++;
   }

   // Process trap if it is coming from host registered in database
   if (pNode != NULL)
   {
      DbgPrintf(4, _T("ProcessTrap: trap matched to node %s [%d]"), pNode->Name(), pNode->Id());
      if (pNode->Status() != STATUS_UNMANAGED)
      {
         UINT32 i;

         // Pass trap to loaded modules
         for(i = 0; i < g_dwNumModules; i++)
         {
            if (g_pModuleList[i].pfTrapHandler != NULL)
            {
               if (g_pModuleList[i].pfTrapHandler(pdu, pNode))
				   {
					   processed = TRUE;
                  break;   // Trap was processed by the module
				   }
            }
         }

         // Find if we have this trap in our list
         MutexLock(m_mutexTrapCfgAccess);

         // Try to find closest match
         for(i = 0, dwMatchLen = 0; i < m_dwNumTraps; i++)
         {
            if (m_pTrapCfg[i].dwOidLen > 0)
            {
               iResult = pdu->getTrapId()->compare(m_pTrapCfg[i].pdwObjectId, m_pTrapCfg[i].dwOidLen);
               if (iResult == OID_EQUAL)
               {
                  dwMatchLen = m_pTrapCfg[i].dwOidLen;
                  dwMatchIdx = i;
                  break;   // Find exact match
               }
               else if (iResult == OID_SHORTER)
               {
                  if (m_pTrapCfg[i].dwOidLen > dwMatchLen)
                  {
                     dwMatchLen = m_pTrapCfg[i].dwOidLen;
                     dwMatchIdx = i;
                  }
               }
            }
         }

         if (dwMatchLen > 0)
         {
            GenerateTrapEvent(pNode->Id(), dwMatchIdx, pdu);
         }
         else     // Process unmatched traps
         {
            // Handle unprocessed traps
            if (!processed)
            {
               // Build trap's parameters string
               dwBufSize = pdu->getNumVariables() * 4096 + 16;
               pszTrapArgs = (TCHAR *)malloc(sizeof(TCHAR) * dwBufSize);
               pszTrapArgs[0] = 0;
               for(i = (pdu->getVersion() == SNMP_VERSION_1) ? 0 : 2, dwBufPos = 0; i < (UINT32)pdu->getNumVariables(); i++)
               {
                  pVar = pdu->getVariable(i);
					   bool convertToHex = true;
                  dwBufPos += _sntprintf(&pszTrapArgs[dwBufPos], dwBufSize - dwBufPos, _T("%s%s == '%s'"),
                                         (dwBufPos == 0) ? _T("") : _T("; "),
                                         pVar->getName()->getValueAsText(), 
												     s_allowVarbindConversion ? pVar->getValueAsPrintableString(szBuffer, 512, &convertToHex) : pVar->getValueAsString(szBuffer, 512));
               }

               // Generate default event for unmatched traps
               PostEvent(EVENT_SNMP_UNMATCHED_TRAP, pNode->Id(), "ss", pdu->getTrapId()->getValueAsText(), pszTrapArgs);
               free(pszTrapArgs);
            }
         }
         MutexUnlock(m_mutexTrapCfgAccess);
      }
      else
      {
         DbgPrintf(4, _T("ProcessTrap: Node %s [%d] is in UNMANAGED state, trap ignored"), pNode->Name(), pNode->Id());
      }
   }
   else if (g_dwFlags & AF_SNMP_TRAP_DISCOVERY)  // unknown node, discovery enabled
   {
      DbgPrintf(4, _T("ProcessTrap: trap not matched to node, adding new IP address %s for discovery"), IpToStr(dwOriginAddr, szBuffer));
      Subnet *subnet = FindSubnetForNode(0, dwOriginAddr);
      if (subnet != NULL)
      {
         if ((subnet->IpAddr() != dwOriginAddr) && !IsBroadcastAddress(dwOriginAddr, subnet->getIpNetMask()))
         {
            NEW_NODE *pInfo;

            pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
            pInfo->dwIpAddr = dwOriginAddr;
            pInfo->dwNetMask = subnet->getIpNetMask();
				pInfo->zoneId = 0;	/* FIXME: add correct zone ID */
				pInfo->ignoreFilter = FALSE;
				memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
            g_nodePollerQueue.Put(pInfo);
         }
      }
      else
      {
         NEW_NODE *pInfo;

         pInfo = (NEW_NODE *)malloc(sizeof(NEW_NODE));
         pInfo->dwIpAddr = dwOriginAddr;
         pInfo->dwNetMask = 0;
			pInfo->zoneId = 0;	/* FIXME: add correct zone ID */
			pInfo->ignoreFilter = FALSE;
			memset(pInfo->bMacAddr, 0, MAC_ADDR_LENGTH);
         g_nodePollerQueue.Put(pInfo);
      }
   }
   else  // unknown node, discovery disabled
   {
      DbgPrintf(4, _T("ProcessTrap: trap not matched to any node"));
   }
}

/**
 * Context finder - tries to find SNMPv3 security context by IP address
 */
static SNMP_SecurityContext *ContextFinder(struct sockaddr *addr, socklen_t addrLen)
{
	UINT32 ipAddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	Node *node = FindNodeByIP(0, ipAddr);
	TCHAR buffer[32];
	DbgPrintf(6, _T("SNMPTrapReceiver: looking for SNMP security context for node %s %s"),
	          IpToStr(ipAddr, buffer), (node != NULL) ? node->Name() : _T("<unknown>"));
	return (node != NULL) ? node->getSnmpSecurityContext() : NULL;
}

/**
 * SNMP trap receiver thread
 */
THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *pArg)
{
   SOCKET hSocket;
   struct sockaddr_in addr;
   int iBytes;
   socklen_t nAddrLen;
   SNMP_UDPTransport *pTransport;
   SNMP_PDU *pdu;

	static BYTE engineId[] = { 0x80, 0x00, 0x00, 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 };
	SNMP_Engine localEngine(engineId, 12);

   hSocket = socket(AF_INET, SOCK_DGRAM, 0);
   if (hSocket == INVALID_SOCKET)
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", "SNMPTrapReceiver");
      return THREAD_OK;
   }

	SetSocketExclusiveAddrUse(hSocket);
	SetSocketReuseFlag(hSocket);

   // Fill in local address structure
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = ResolveHostName(g_szListenAddress);
   addr.sin_port = htons(m_wTrapPort);

   // Bind socket
   if (bind(hSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", m_wTrapPort, _T("SNMPTrapReceiver"), WSAGetLastError());
      closesocket(hSocket);
      return THREAD_OK;
   }
	nxlog_write(MSG_LISTENING_FOR_SNMP, EVENTLOG_INFORMATION_TYPE, "ad", ntohl(addr.sin_addr.s_addr), m_wTrapPort);

   pTransport = new SNMP_UDPTransport(hSocket);
	pTransport->enableEngineIdAutoupdate(true);
	pTransport->setPeerUpdatedOnRecv(true);
   DbgPrintf(1, _T("SNMP Trap Receiver started on port %u"), m_wTrapPort);

   // Wait for packets
   while(!IsShutdownInProgress())
   {
      nAddrLen = sizeof(struct sockaddr_in);
      iBytes = pTransport->readMessage(&pdu, 2000, (struct sockaddr *)&addr, &nAddrLen, ContextFinder);
      if ((iBytes > 0) && (pdu != NULL))
      {
			DbgPrintf(6, _T("SNMPTrapReceiver: received PDU of type %d"), pdu->getCommand());
			if ((pdu->getCommand() == SNMP_TRAP) || (pdu->getCommand() == SNMP_INFORM_REQUEST))
			{
				if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_INFORM_REQUEST))
				{
					SNMP_SecurityContext *context = pTransport->getSecurityContext();
					context->setAuthoritativeEngine(localEngine);
				}
            ProcessTrap(pdu, &addr, pTransport, &localEngine, pdu->getCommand() == SNMP_INFORM_REQUEST);
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
				pTransport->setSecurityContext(context);

				pTransport->sendMessage(response);
				delete response;
			}
			else if (pdu->getCommand() == SNMP_REPORT)
			{
				DbgPrintf(6, _T("SNMPTrapReceiver: REPORT PDU with error %s"), pdu->getVariable(0)->getName()->getValueAsText());
			}
         delete pdu;
      }
      else
      {
         // Sleep on error
         ThreadSleepMs(100);
      }
   }

   delete pTransport;
   DbgPrintf(1, _T("SNMP Trap Receiver terminated"));
   return THREAD_OK;
}

/**
 * Fill NXCP message with trap configuration data
 */
static void FillTrapConfigDataMsg(CSCPMessage &msg, NXC_TRAP_CFG_ENTRY *trap)
{
   UINT32 i, dwId1, dwId2, dwId3, dwId4;

	msg.SetVariable(VID_TRAP_ID, trap->dwId);
   msg.SetVariable(VID_TRAP_OID_LEN, trap->dwOidLen); 
   msg.setFieldInt32Array(VID_TRAP_OID, trap->dwOidLen, trap->pdwObjectId);
   msg.SetVariable(VID_EVENT_CODE, trap->dwEventCode);
   msg.SetVariable(VID_DESCRIPTION, trap->szDescription);
   msg.SetVariable(VID_USER_TAG, trap->szUserTag);
   msg.SetVariable(VID_TRAP_NUM_MAPS, trap->dwNumMaps);
   for(i = 0, dwId1 = VID_TRAP_PLEN_BASE, dwId2 = VID_TRAP_PNAME_BASE, dwId3 = VID_TRAP_PDESCR_BASE, dwId4 = VID_TRAP_PFLAGS_BASE; 
       i < trap->dwNumMaps; i++, dwId1++, dwId2++, dwId3++, dwId4++)
   {
      msg.SetVariable(dwId1, trap->pMaps[i].dwOidLen);
      if ((trap->pMaps[i].dwOidLen & 0x80000000) == 0)
         msg.setFieldInt32Array(dwId2, trap->pMaps[i].dwOidLen, trap->pMaps[i].pdwObjectId);
      msg.SetVariable(dwId3, trap->pMaps[i].szDescription);
		msg.SetVariable(dwId4, trap->pMaps[i].dwFlags);
   }
}

/**
 * Send all trap configuration records to client
 */
void SendTrapsToClient(ClientSession *pSession, UINT32 dwRqId)
{
   UINT32 i;
   CSCPMessage msg;

   // Prepare message
   msg.SetCode(CMD_TRAP_CFG_RECORD);
   msg.SetId(dwRqId);

   MutexLock(m_mutexTrapCfgAccess);
   for(i = 0; i < m_dwNumTraps; i++)
   {
		FillTrapConfigDataMsg(msg, &m_pTrapCfg[i]);
      pSession->sendMessage(&msg);
      msg.deleteAllVariables();
   }
   MutexUnlock(m_mutexTrapCfgAccess);

   msg.SetVariable(VID_TRAP_ID, (UINT32)0);
   pSession->sendMessage(&msg);
}

/**
 * Prepare single message with all trap configuration records
 */
void CreateTrapCfgMessage(CSCPMessage &msg)
{
   UINT32 i, dwId;

   MutexLock(m_mutexTrapCfgAccess);
	msg.SetVariable(VID_NUM_TRAPS, m_dwNumTraps);
   for(i = 0, dwId = VID_TRAP_INFO_BASE; i < m_dwNumTraps; i++, dwId += 5)
   {
      msg.SetVariable(dwId++, m_pTrapCfg[i].dwId);
      msg.SetVariable(dwId++, m_pTrapCfg[i].dwOidLen); 
      msg.setFieldInt32Array(dwId++, m_pTrapCfg[i].dwOidLen, m_pTrapCfg[i].pdwObjectId);
      msg.SetVariable(dwId++, m_pTrapCfg[i].dwEventCode);
      msg.SetVariable(dwId++, m_pTrapCfg[i].szDescription);
   }
   MutexUnlock(m_mutexTrapCfgAccess);
}

/**
 * Notify clients about trap configuration change
 */
static void NotifyOnTrapCfgChangeCB(ClientSession *session, void *arg)
{
	if (session->isAuthenticated())
		session->postMessage((CSCPMessage *)arg);
}

static void NotifyOnTrapCfgChange(UINT32 code, NXC_TRAP_CFG_ENTRY *trap)
{
	CSCPMessage msg;

	msg.SetCode(CMD_TRAP_CFG_UPDATE);
	msg.SetVariable(VID_NOTIFICATION_CODE, code);
	FillTrapConfigDataMsg(msg, trap);
	EnumerateClientSessions(NotifyOnTrapCfgChangeCB, &msg);
}

static void NotifyOnTrapCfgDelete(UINT32 id)
{
	CSCPMessage msg;

	msg.SetCode(CMD_TRAP_CFG_UPDATE);
	msg.SetVariable(VID_NOTIFICATION_CODE, (UINT32)NX_NOTIFY_TRAPCFG_DELETED);
	msg.SetVariable(VID_TRAP_ID, id);
	EnumerateClientSessions(NotifyOnTrapCfgChangeCB, &msg);
}

/**
 * Delete trap configuration record
 */
UINT32 DeleteTrap(UINT32 dwId)
{
   UINT32 i, j, dwResult = RCC_INVALID_TRAP_ID;
   TCHAR szQuery[256];

   MutexLock(m_mutexTrapCfgAccess);
   
   for(i = 0; i < m_dwNumTraps; i++)
   {
      if (m_pTrapCfg[i].dwId == dwId)
      {
         // Free allocated resources
         for(j = 0; j < m_pTrapCfg[i].dwNumMaps; j++)
            safe_free(m_pTrapCfg[i].pMaps[j].pdwObjectId);
         safe_free(m_pTrapCfg[i].pMaps);
         safe_free(m_pTrapCfg[i].pdwObjectId);

         // Remove trap entry from list
         m_dwNumTraps--;
         memmove(&m_pTrapCfg[i], &m_pTrapCfg[i + 1], sizeof(NXC_TRAP_CFG_ENTRY) * (m_dwNumTraps - i));

         // Remove trap entry from database
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM snmp_trap_cfg WHERE trap_id=%d"), dwId);
         QueueSQLRequest(szQuery);
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM snmp_trap_pmap WHERE trap_id=%d"), dwId);
         QueueSQLRequest(szQuery);
         dwResult = RCC_SUCCESS;

			NotifyOnTrapCfgDelete(dwId);
         break;
      }
   }

   MutexUnlock(m_mutexTrapCfgAccess);
   return dwResult;
}

/**
 * Save parameter mapping to database
 */
static BOOL SaveParameterMapping(DB_HANDLE hdb, NXC_TRAP_CFG_ENTRY *pTrap)
{
	TCHAR szQuery[1024];
   _sntprintf(szQuery, 1024, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=%d"), pTrap->dwId);
   BOOL bRet = DBQuery(hdb, szQuery);

	if (bRet)
   {
		DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description,flags) VALUES (?,?,?,?,?)"));
		if (hStmt != NULL)
		{
			for(UINT32 i = 0; i < pTrap->dwNumMaps; i++)
			{
				TCHAR oid[1024];

				if ((pTrap->pMaps[i].dwOidLen & 0x80000000) == 0)
				{
					SNMPConvertOIDToText(pTrap->pMaps[i].dwOidLen,
												pTrap->pMaps[i].pdwObjectId,
												oid, 1024);
				}
				else
				{
					_sntprintf(oid, 1024, _T("POS:%d"), pTrap->pMaps[i].dwOidLen & 0x7FFFFFFF);
				}

				DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, pTrap->dwId);
				DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
				DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, oid, DB_BIND_STATIC);
				DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, pTrap->pMaps[i].szDescription, DB_BIND_STATIC);
				DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, pTrap->pMaps[i].dwFlags);

				bRet = DBExecute(hStmt);
				if (!bRet)
					break;
			}
			DBFreeStatement(hStmt);
		}
		else
		{
			bRet = FALSE;
		}
   }

	return bRet;
}

/**
 * Create new trap configuration record
 */
UINT32 CreateNewTrap(UINT32 *pdwTrapId)
{
   UINT32 dwResult = RCC_SUCCESS;
   TCHAR szQuery[256];

   MutexLock(m_mutexTrapCfgAccess);
   
   *pdwTrapId = CreateUniqueId(IDG_SNMP_TRAP);
   m_pTrapCfg = (NXC_TRAP_CFG_ENTRY *)realloc(m_pTrapCfg, sizeof(NXC_TRAP_CFG_ENTRY) * (m_dwNumTraps + 1));
   memset(&m_pTrapCfg[m_dwNumTraps], 0, sizeof(NXC_TRAP_CFG_ENTRY));
   m_pTrapCfg[m_dwNumTraps].dwId = *pdwTrapId;
   m_pTrapCfg[m_dwNumTraps].dwEventCode = EVENT_SNMP_UNMATCHED_TRAP;

	NotifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_CREATED, &m_pTrapCfg[m_dwNumTraps]);

	m_dwNumTraps++;
   MutexUnlock(m_mutexTrapCfgAccess);

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_code,description,user_tag) ")
                      _T("VALUES (%d,'',%d,'','')"), *pdwTrapId, (UINT32)EVENT_SNMP_UNMATCHED_TRAP);
   if (!DBQuery(g_hCoreDB, szQuery))
      dwResult = RCC_DB_FAILURE;

   return dwResult;
}

/**
 * Create new trap configuration record from NXMP data
 */
UINT32 CreateNewTrap(NXC_TRAP_CFG_ENTRY *pTrap)
{
   UINT32 i, dwResult;
   TCHAR szQuery[4096], szOID[1024];
	BOOL bSuccess;

   MutexLock(m_mutexTrapCfgAccess);
   
   m_pTrapCfg = (NXC_TRAP_CFG_ENTRY *)realloc(m_pTrapCfg, sizeof(NXC_TRAP_CFG_ENTRY) * (m_dwNumTraps + 1));
   memcpy(&m_pTrapCfg[m_dwNumTraps], pTrap, sizeof(NXC_TRAP_CFG_ENTRY));
   m_pTrapCfg[m_dwNumTraps].dwId = CreateUniqueId(IDG_SNMP_TRAP);
	m_pTrapCfg[m_dwNumTraps].pdwObjectId = (UINT32 *)nx_memdup(pTrap->pdwObjectId, sizeof(UINT32) * pTrap->dwOidLen);
	m_pTrapCfg[m_dwNumTraps].pMaps = (NXC_OID_MAP *)nx_memdup(pTrap->pMaps, sizeof(NXC_OID_MAP) * pTrap->dwNumMaps);
	for(i = 0; i < m_pTrapCfg[m_dwNumTraps].dwNumMaps; i++)
	{
      if ((m_pTrapCfg[m_dwNumTraps].pMaps[i].dwOidLen & 0x80000000) == 0)
			m_pTrapCfg[m_dwNumTraps].pMaps[i].pdwObjectId = (UINT32 *)nx_memdup(pTrap->pMaps[i].pdwObjectId, sizeof(UINT32) * pTrap->pMaps[i].dwOidLen);
	}

	// Write new trap to database
   SNMPConvertOIDToText(m_pTrapCfg[m_dwNumTraps].dwOidLen, m_pTrapCfg[m_dwNumTraps].pdwObjectId, szOID, 1024);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_code,description,user_tag) ")
                      _T("VALUES (%d,'%s',%d,%s,%s)"), m_pTrapCfg[m_dwNumTraps].dwId,
	          szOID, m_pTrapCfg[m_dwNumTraps].dwEventCode,
				 (const TCHAR *)DBPrepareString(g_hCoreDB, m_pTrapCfg[m_dwNumTraps].szDescription),
				 (const TCHAR *)DBPrepareString(g_hCoreDB, m_pTrapCfg[m_dwNumTraps].szUserTag));

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	if(DBBegin(hdb))
   {
      bSuccess = DBQuery(hdb, szQuery);
      if (bSuccess)
      {
			bSuccess = SaveParameterMapping(hdb, &m_pTrapCfg[m_dwNumTraps]);
      }
      if (bSuccess)
         DBCommit(hdb);
      else
         DBRollback(hdb);
      dwResult = bSuccess ? RCC_SUCCESS : RCC_DB_FAILURE;
   }
   else
   {
      dwResult = RCC_DB_FAILURE;
   }
	DBConnectionPoolReleaseConnection(hdb);

	if (dwResult == RCC_SUCCESS)
		NotifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_CREATED, &m_pTrapCfg[m_dwNumTraps]);

   m_dwNumTraps++;
   MutexUnlock(m_mutexTrapCfgAccess);

   return dwResult;
}

/**
 * Update trap configuration record from message
 */
UINT32 UpdateTrapFromMsg(CSCPMessage *pMsg)
{
   UINT32 i, j, dwId1, dwId2, dwId3, dwId4, dwTrapId, dwResult = RCC_INVALID_TRAP_ID;
   TCHAR szQuery[1024], szOID[1024];
   BOOL bSuccess;

   dwTrapId = pMsg->GetVariableLong(VID_TRAP_ID);

   MutexLock(m_mutexTrapCfgAccess);
   for(i = 0; i < m_dwNumTraps; i++)
   {
      if (m_pTrapCfg[i].dwId == dwTrapId)
      {
         // Read trap configuration from event
         m_pTrapCfg[i].dwEventCode = pMsg->GetVariableLong(VID_EVENT_CODE);
         m_pTrapCfg[i].dwOidLen = pMsg->GetVariableLong(VID_TRAP_OID_LEN);
         m_pTrapCfg[i].pdwObjectId = (UINT32 *)realloc(m_pTrapCfg[i].pdwObjectId, sizeof(UINT32) * m_pTrapCfg[i].dwOidLen);
         pMsg->getFieldAsInt32Array(VID_TRAP_OID, m_pTrapCfg[i].dwOidLen, m_pTrapCfg[i].pdwObjectId);
         pMsg->GetVariableStr(VID_DESCRIPTION, m_pTrapCfg[i].szDescription, MAX_DB_STRING);
         pMsg->GetVariableStr(VID_USER_TAG, m_pTrapCfg[i].szUserTag, MAX_USERTAG_LENGTH);

         // Destroy current parameter mapping
         for(j = 0; j < m_pTrapCfg[i].dwNumMaps; j++)
            safe_free(m_pTrapCfg[i].pMaps[j].pdwObjectId);
         safe_free(m_pTrapCfg[i].pMaps);

         // Read new mappings from message
         m_pTrapCfg[i].dwNumMaps = pMsg->GetVariableLong(VID_TRAP_NUM_MAPS);
         m_pTrapCfg[i].pMaps = (NXC_OID_MAP *)malloc(sizeof(NXC_OID_MAP) * m_pTrapCfg[i].dwNumMaps);
         for(j = 0, dwId1 = VID_TRAP_PLEN_BASE, dwId2 = VID_TRAP_PNAME_BASE, dwId3 = VID_TRAP_PDESCR_BASE, dwId4 = VID_TRAP_PFLAGS_BASE; 
             j < m_pTrapCfg[i].dwNumMaps; j++, dwId1++, dwId2++, dwId3++, dwId4++)
         {
            m_pTrapCfg[i].pMaps[j].dwOidLen = pMsg->GetVariableLong(dwId1);
            if ((m_pTrapCfg[i].pMaps[j].dwOidLen & 0x80000000) == 0)
            {
               m_pTrapCfg[i].pMaps[j].pdwObjectId = 
                  (UINT32 *)malloc(sizeof(UINT32) * m_pTrapCfg[i].pMaps[j].dwOidLen);
               pMsg->getFieldAsInt32Array(dwId2, m_pTrapCfg[i].pMaps[j].dwOidLen, 
                                           m_pTrapCfg[i].pMaps[j].pdwObjectId);
            }
            else
            {
               m_pTrapCfg[i].pMaps[j].pdwObjectId = NULL;
            }
            pMsg->GetVariableStr(dwId3, m_pTrapCfg[i].pMaps[j].szDescription, MAX_DB_STRING);
				m_pTrapCfg[i].pMaps[j].dwFlags = pMsg->GetVariableLong(dwId4);
         }

         // Update database
         SNMPConvertOIDToText(m_pTrapCfg[i].dwOidLen, m_pTrapCfg[i].pdwObjectId, szOID, 1024);
         _sntprintf(szQuery, 1024, _T("UPDATE snmp_trap_cfg SET snmp_oid='%s',event_code=%d,description=%s,user_tag=%s WHERE trap_id=%d"),
                    szOID, m_pTrapCfg[i].dwEventCode,
						  (const TCHAR *)DBPrepareString(g_hCoreDB, m_pTrapCfg[i].szDescription),
						  (const TCHAR *)DBPrepareString(g_hCoreDB, m_pTrapCfg[i].szUserTag), m_pTrapCfg[i].dwId);
			DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         if(DBBegin(hdb))
         {
            bSuccess = DBQuery(hdb, szQuery);
            if (bSuccess)
            {
					bSuccess = SaveParameterMapping(hdb, &m_pTrapCfg[i]);
            }
            if (bSuccess)
               DBCommit(hdb);
            else
               DBRollback(hdb);
            dwResult = bSuccess ? RCC_SUCCESS : RCC_DB_FAILURE;
         }
         else
         {
            dwResult = RCC_DB_FAILURE;
         }
			DBConnectionPoolReleaseConnection(hdb);

			if (dwResult == RCC_SUCCESS)
				NotifyOnTrapCfgChange(NX_NOTIFY_TRAPCFG_MODIFIED, &m_pTrapCfg[i]);

         break;
      }
   }

   MutexUnlock(m_mutexTrapCfgAccess);
   return dwResult;
}

/**
 * Create trap record in NXMP file
 */
void CreateNXMPTrapRecord(String &str, UINT32 dwId)
{
	UINT32 i, j;
	TCHAR szBuffer[1024];

   MutexLock(m_mutexTrapCfgAccess);
   for(i = 0; i < m_dwNumTraps; i++)
   {
      if (m_pTrapCfg[i].dwId == dwId)
      {
			str.addFormattedString(_T("\t\t<trap id=\"%d\">\n")
			                       _T("\t\t\t<oid>%s</oid>\n")
			                       _T("\t\t\t<description>%s</description>\n")
			                       _T("\t\t\t<userTag>%s</userTag>\n"), dwId,
			                       SNMPConvertOIDToText(m_pTrapCfg[i].dwOidLen,
			                                            m_pTrapCfg[i].pdwObjectId,
																	  szBuffer, 1024),
										  (const TCHAR *)EscapeStringForXML2(m_pTrapCfg[i].szDescription),
										  (const TCHAR *)EscapeStringForXML2(m_pTrapCfg[i].szUserTag));

		   EventNameFromCode(m_pTrapCfg[i].dwEventCode, szBuffer);
			str.addFormattedString(_T("\t\t\t<event>%s</event>\n"), (const TCHAR *)EscapeStringForXML2(szBuffer));
			if (m_pTrapCfg[i].dwNumMaps > 0)
			{
				str += _T("\t\t\t<parameters>\n");
				for(j = 0; j < m_pTrapCfg[i].dwNumMaps; j++)
				{
					str.addFormattedString(_T("\t\t\t\t<parameter id=\"%d\">\n")
			                             _T("\t\t\t\t\t<flags>%d</flags>\n")
					                       _T("\t\t\t\t\t<description>%s</description>\n"),
												  j + 1, m_pTrapCfg[i].pMaps[j].dwFlags, 
												  (const TCHAR *)EscapeStringForXML2(m_pTrapCfg[i].pMaps[j].szDescription));
               if ((m_pTrapCfg[i].pMaps[j].dwOidLen & 0x80000000) == 0)
					{
						str.addFormattedString(_T("\t\t\t\t\t<oid>%s</oid>\n"),
						                       SNMPConvertOIDToText(m_pTrapCfg[i].pMaps[j].dwOidLen,
													                       m_pTrapCfg[i].pMaps[j].pdwObjectId,
																				  szBuffer, 1024));
					}
					else
					{
						str.addFormattedString(_T("\t\t\t\t\t<position>%d</position>\n"),
						                       m_pTrapCfg[i].pMaps[j].dwOidLen & 0x7FFFFFFF);
					}
					str += _T("\t\t\t\t</parameter>\n");
				}
				str += _T("\t\t\t</parameters>\n");
			}
			str += _T("\t\t</trap>\n");
			break;
		}
	}
   MutexUnlock(m_mutexTrapCfgAccess);
}
