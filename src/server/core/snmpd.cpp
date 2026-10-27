/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: snmpd.cpp
**
**/

#include "nxcore.h"
#include <netxms-version.h>

#define DEBUG_TAG _T("snmp.agent")

/**
 * Base class for MIB subtree handler
 */
class MIBSubTree
{
protected:
   SNMP_ObjectId m_baseOID;

public:
   MIBSubTree(const SNMP_ObjectId& baseOID) : m_baseOID(baseOID) {}
   virtual ~MIBSubTree() = default;

   virtual SNMP_Variable *get(const SNMP_ObjectId& oid) = 0;
   virtual SNMP_Variable *getNext(const SNMP_ObjectId& oid) = 0;
};

/**
 * Single-value sub-tree (handles only baseOID.0)
 */
class SingleValueMIBSubTree : public MIBSubTree
{
protected:
   std::function<SNMP_Variable *(const SNMP_ObjectId&)> m_handler;
   SNMP_ObjectId m_elementOID;

public:
   SingleValueMIBSubTree(const SNMP_ObjectId& baseOID, std::function<SNMP_Variable *(const SNMP_ObjectId&)> handler) : MIBSubTree(baseOID), m_handler(handler), m_elementOID(baseOID, 0) {}

   virtual SNMP_Variable *get(const SNMP_ObjectId& oid) override
   {
      return (oid.compare(m_elementOID) == OID_EQUAL) ? m_handler(m_elementOID) : nullptr;
   }

   virtual SNMP_Variable *getNext(const SNMP_ObjectId& oid) override
   {
      int result = oid.compare(m_baseOID);
      return ((result == OID_PRECEDING) || (result == OID_SHORTER) || (result == OID_EQUAL)) ? get(m_elementOID) : nullptr;
   }
};

/**
 * MIB tree handlers
 */
static ObjectArray<MIBSubTree> s_mib(0, 16, Ownership::True);

/**
 * MIB tree lock
 */
static RWLock s_mibLock;

/**
 * Allowed SNMP versions (bit mask)
 */
static uint32_t s_allowedVersions = 7;

/**
 * Local SNMP engine (authoritative engine for requests to built-in agent)
 */
static SNMP_Engine s_localEngine;

/**
 * Credentials lock (guards s_community and s_usmContext)
 */
static Mutex s_credentialsLock(MutexType::FAST);

/**
 * Community string
 */
static char s_community[64] = "public";

/**
 * SNMPv3 user (keys localized with local engine ID)
 */
static SNMP_SecurityContext s_usmContext;

/**
 * usmStats counters reported to manager (RFC 3414 section 3.2)
 */
static const SNMP_ObjectId s_usmStatsUnsupportedSecLevels { 1, 3, 6, 1, 6, 3, 15, 1, 1, 1, 0 };
static const SNMP_ObjectId s_usmStatsNotInTimeWindows { 1, 3, 6, 1, 6, 3, 15, 1, 1, 2, 0 };
static const SNMP_ObjectId s_usmStatsUnknownUserNames { 1, 3, 6, 1, 6, 3, 15, 1, 1, 3, 0 };
static const SNMP_ObjectId s_usmStatsUnknownEngineIDs { 1, 3, 6, 1, 6, 3, 15, 1, 1, 4, 0 };
static const SNMP_ObjectId s_usmStatsWrongDigests { 1, 3, 6, 1, 6, 3, 15, 1, 1, 5, 0 };
static const SNMP_ObjectId s_usmStatsDecryptionErrors { 1, 3, 6, 1, 6, 3, 15, 1, 1, 6, 0 };

/**
 * Maximum difference between engine time in request and local engine time (RFC 3414 section 2.2.3)
 */
#define USM_TIME_WINDOW 150

/**
 * Load SNMPv3 user from configuration variables SNMP.Agent.V3.*
 */
static void LoadUsmUser()
{
   char userName[128], authPassword[MAX_PASSWORD], privPassword[MAX_PASSWORD];
   ConfigReadStrUTF8(L"SNMP.Agent.V3.UserName", userName, sizeof(userName), "netxms");
   ConfigReadStrUTF8(L"SNMP.Agent.V3.AuthenticationPassword", authPassword, sizeof(authPassword), "");
   ConfigReadStrUTF8(L"SNMP.Agent.V3.EncryptionPassword", privPassword, sizeof(privPassword), "");
   DecryptPasswordA(userName, authPassword, authPassword, sizeof(authPassword));
   DecryptPasswordA(userName, privPassword, privPassword, sizeof(privPassword));

   int32_t authMethod = ConfigReadInt(L"SNMP.Agent.V3.AuthenticationMethod", SNMP_AUTH_NONE);
   if ((authMethod < SNMP_AUTH_NONE) || (authMethod > SNMP_AUTH_SHA512))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Invalid SNMPv3 authentication method %d in built-in SNMP agent configuration, authentication disabled", authMethod);
      authMethod = SNMP_AUTH_NONE;
   }
   int32_t privMethod = ConfigReadInt(L"SNMP.Agent.V3.EncryptionMethod", SNMP_ENCRYPT_NONE);
   if ((privMethod < SNMP_ENCRYPT_NONE) || (privMethod > SNMP_ENCRYPT_AES_256))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Invalid SNMPv3 encryption method %d in built-in SNMP agent configuration, encryption disabled", privMethod);
      privMethod = SNMP_ENCRYPT_NONE;
   }
   if ((privMethod != SNMP_ENCRYPT_NONE) && (authMethod == SNMP_AUTH_NONE))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"SNMPv3 encryption without authentication is not allowed, encryption disabled for built-in SNMP agent");
      privMethod = SNMP_ENCRYPT_NONE;
   }

   SNMP_SecurityContext context(userName, authPassword, privPassword, static_cast<SNMP_AuthMethod>(authMethod), static_cast<SNMP_EncryptionMethod>(privMethod));
   context.setAuthoritativeEngine(s_localEngine);
   context.recalculateKeys();   // Key localization is expensive, do it once here instead of on first request
   SecureZeroMemory(authPassword, sizeof(authPassword));
   SecureZeroMemory(privPassword, sizeof(privPassword));

   s_credentialsLock.lock();
   s_usmContext = std::move(context);
   s_credentialsLock.unlock();

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"SNMPv3 user for built-in SNMP agent set to \"%hs\" (authentication: %hs, encryption: %hs)",
      userName, SnmpAuthMethodToName(static_cast<SNMP_AuthMethod>(authMethod)), SnmpPrivMethodToName(static_cast<SNMP_EncryptionMethod>(privMethod)));
}

/**
 * Create SNMP transport for receiver
 */
static SNMP_Transport *CreateTransport(SOCKET hSocket)
{
   if (hSocket == INVALID_SOCKET)
      return nullptr;

   SNMP_Transport *t = new SNMP_UDPTransport(hSocket);
   t->setPeerUpdatedOnRecv(true);
   t->setDeliverPduOnSecurityFailure(true);
   return t;
}

/**
 * Check if given SNMP version is allowed by agent
 */
static inline bool IsVersionAllowed(SNMP_Version v)
{
   switch(v)
   {
      case SNMP_VERSION_1:
         return (s_allowedVersions & 1) != 0;
      case SNMP_VERSION_2C:
         return (s_allowedVersions & 2) != 0;
      case SNMP_VERSION_3:
         return (s_allowedVersions & 4) != 0;
      default:
         return false;
   }
}

/**
 * Create response PDU for given request (SNMPv3 message ID, context engine ID and context name are echoed from request as required by RFC 3412 section 7.1)
 */
static inline void InitResponse(SNMP_PDU *response, SNMP_PDU *request)
{
   response->setReportable(false);
   response->setMessageId(request->getMessageId());
   response->setContextEngineId(request->getContextEngineId(), request->getContextEngineIdLength());
   response->setContextName(request->getContextName());
}

/**
 * Send REPORT PDU with given usmStats counter. Transport security context should be set by caller.
 */
static void SendReport(SNMP_Transport *transport, SNMP_PDU *request, const SNMP_ObjectId& counterOid)
{
   SNMP_PDU report(SNMP_REPORT, request->getRequestId(), request->getVersion());
   InitResponse(&report, request);
   report.setContextEngineId(s_localEngine.getId(), s_localEngine.getIdLen());

   SNMP_Variable *var = new SNMP_Variable(counterOid);
   var->setValueFromUInt32(ASN_COUNTER32, 1);
   report.bindVariable(var);

   transport->sendMessage(&report, 1000);
}

/**
 * Process GET or GET-NEXT request and send response. Transport security context should be set by caller.
 */
static void ProcessRequest(SNMP_Transport *transport, SNMP_PDU *request)
{
   SNMP_PDU response(SNMP_RESPONSE, request->getRequestId(), request->getVersion());
   InitResponse(&response, request);
   bool getNext = (request->getCommand() == SNMP_GET_NEXT_REQUEST);

   s_mibLock.readLock();
   for(int i = 0; i < request->getNumVariables(); i++)
   {
      SNMP_Variable *vreq = request->getVariable(i);
      SNMP_Variable *vrsp = nullptr;
      for(int j = 0; (j < s_mib.size()) && (vrsp == nullptr); j++)
      {
         MIBSubTree *ms = s_mib.get(j);
         vrsp = getNext ? ms->getNext(vreq->getName()) : ms->get(vreq->getName());
      }
      response.bindVariable((vrsp != nullptr) ? vrsp : new SNMP_Variable(vreq->getName(), getNext ? ASN_END_OF_MIBVIEW : ASN_NO_SUCH_OBJECT));
   }
   s_mibLock.unlock();

   transport->sendMessage(&response, 1000);
}

/**
 * Process SNMPv1/v2c request
 */
static void ProcessCommunityRequest(SNMP_Transport *transport, SNMP_PDU *request, const InetAddress& sourceAddr)
{
   s_credentialsLock.lock();
   bool communityMatch = !strcmp(request->getCommunity(), s_community);
   s_credentialsLock.unlock();
   if (!communityMatch)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv%hs request from %s rejected: invalid community string", SnmpVersionToName(request->getVersion()), sourceAddr.toString().cstr());
      return;
   }

   if ((request->getCommand() != SNMP_GET_REQUEST) && (request->getCommand() != SNMP_GET_NEXT_REQUEST))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv%hs request from %s ignored: unsupported PDU type %d", SnmpVersionToName(request->getVersion()), sourceAddr.toString().cstr(), request->getCommand());
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, L"SNMPv%hs request from %s passed validation", SnmpVersionToName(request->getVersion()), sourceAddr.toString().cstr());
   transport->setSecurityContext(SNMP_SecurityContext(request->getCommunity()));
   ProcessRequest(transport, request);
}

/**
 * Process SNMPv3 request. Checks follow RFC 3414 section 3.2; security context of the transport holds
 * the configured USM user (it was used to authenticate and decrypt the message) and is adjusted here
 * to the security level each report or response has to be sent with.
 */
static void ProcessUsmRequest(SNMP_Transport *transport, SNMP_PDU *request, const InetAddress& sourceAddr)
{
   SNMP_SecurityContext& user = transport->getSecurityContext();
   int flags = request->getFlags();

   if (request->getSecurityModel() != SNMP_SECURITY_MODEL_USM)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv3 request from %s dropped: unsupported security model %d", sourceAddr.toString().cstr(), request->getSecurityModel());
      return;
   }

   if ((flags & SNMP_PRIV_FLAG) && !(flags & SNMP_AUTH_FLAG))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv3 request from %s dropped: privacy without authentication requested", sourceAddr.toString().cstr());
      return;
   }

   // Reports are generated only for requests that ask for them (RFC 3412 section 7.2 step 7)
   bool reportable = request->isReportable();

   const SNMP_Engine& requestEngine = request->getAuthoritativeEngine();
   if (!requestEngine.equals(s_localEngine))
   {
      // Engine ID discovery (empty engine ID) or request localized to some other engine - answer with local engine ID, boots and time
      nxlog_debug_tag(DEBUG_TAG, 6, L"SNMPv3 request from %s: engine ID discovery (engine ID in request: %s)", sourceAddr.toString().cstr(),
         (requestEngine.getIdLen() > 0) ? requestEngine.toString().cstr() : L"none");
      if (reportable)
      {
         SNMP_SecurityContext context;
         context.setSecurityModel(SNMP_SECURITY_MODEL_USM);
         context.setAuthoritativeEngine(s_localEngine);
         transport->setSecurityContext(std::move(context));
         SendReport(transport, request, s_usmStatsUnknownEngineIDs);
      }
      return;
   }

   if (strcmp(request->getUser(), user.getUserName()))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv3 request from %s rejected: unknown user \"%hs\"", sourceAddr.toString().cstr(), request->getUser());
      if (reportable)
      {
         user.setAuthMethod(SNMP_AUTH_NONE);
         user.setPrivMethod(SNMP_ENCRYPT_NONE);
         user.setUserName(request->getUser());
         SendReport(transport, request, s_usmStatsUnknownUserNames);
      }
      return;
   }

   if (((flags & SNMP_AUTH_FLAG) && (user.getAuthMethod() == SNMP_AUTH_NONE)) || ((flags & SNMP_PRIV_FLAG) && (user.getPrivMethod() == SNMP_ENCRYPT_NONE)))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv3 request from %s rejected: requested security level is not supported by user \"%hs\"", sourceAddr.toString().cstr(), request->getUser());
      if (reportable)
      {
         user.setAuthMethod(SNMP_AUTH_NONE);
         user.setPrivMethod(SNMP_ENCRYPT_NONE);
         SendReport(transport, request, s_usmStatsUnsupportedSecLevels);
      }
      return;
   }

   if (flags & SNMP_AUTH_FLAG)
   {
      if (request->getSecurityError() == SNMP_ERR_AUTH_FAILURE)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv3 request from %s rejected: authentication failure", sourceAddr.toString().cstr());
         if (reportable)
         {
            user.setAuthMethod(SNMP_AUTH_NONE);
            user.setPrivMethod(SNMP_ENCRYPT_NONE);
            SendReport(transport, request, s_usmStatsWrongDigests);
         }
         return;
      }

      // Time window is checked only for authenticated messages, and the report is sent authenticated (RFC 3414 section 3.2 step 7)
      uint32_t localTime = s_localEngine.getAdjustedTime();
      uint32_t timeDiff = (requestEngine.getTime() > localTime) ? requestEngine.getTime() - localTime : localTime - requestEngine.getTime();
      if ((requestEngine.getBoots() != s_localEngine.getBoots()) || (timeDiff > USM_TIME_WINDOW))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv3 request from %s rejected: not in time window (request: boots=%u time=%u, local: boots=%u time=%u)",
            sourceAddr.toString().cstr(), requestEngine.getBoots(), requestEngine.getTime(), s_localEngine.getBoots(), localTime);
         if (reportable)
         {
            user.setPrivMethod(SNMP_ENCRYPT_NONE);
            SendReport(transport, request, s_usmStatsNotInTimeWindows);
         }
         return;
      }
   }

   if (request->getSecurityError() != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv3 request from %s rejected: %s", sourceAddr.toString().cstr(),
         (request->getSecurityError() == SNMP_ERR_DECRYPTION) ? L"decryption error" : L"security check failure");
      if (reportable && (request->getSecurityError() == SNMP_ERR_DECRYPTION))
      {
         user.setAuthMethod(SNMP_AUTH_NONE);
         user.setPrivMethod(SNMP_ENCRYPT_NONE);
         SendReport(transport, request, s_usmStatsDecryptionErrors);
      }
      return;
   }

   if ((request->getCommand() != SNMP_GET_REQUEST) && (request->getCommand() != SNMP_GET_NEXT_REQUEST))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv3 request from %s ignored: unsupported PDU type %d", sourceAddr.toString().cstr(), request->getCommand());
      return;
   }

   // Request below configured security level passes USM but is denied by access control; response goes
   // at the request's security level (RFC 3412 section 7.1 step 7, error-status authorizationError)
   if ((!(flags & SNMP_AUTH_FLAG) && (user.getAuthMethod() != SNMP_AUTH_NONE)) || (!(flags & SNMP_PRIV_FLAG) && (user.getPrivMethod() != SNMP_ENCRYPT_NONE)))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv3 request from %s rejected: security level below configured for user \"%hs\"", sourceAddr.toString().cstr(), request->getUser());
      if (!(flags & SNMP_AUTH_FLAG))
         user.setAuthMethod(SNMP_AUTH_NONE);
      user.setPrivMethod(SNMP_ENCRYPT_NONE);

      SNMP_PDU response(SNMP_RESPONSE, request->getRequestId(), request->getVersion());
      InitResponse(&response, request);
      response.setErrorCode(SNMP_PDU_ERR_AUTHORIZATION_ERROR);
      for(int i = 0; i < request->getNumVariables(); i++)
         response.bindVariable(new SNMP_Variable(*request->getVariable(i)));
      transport->sendMessage(&response, 1000);
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, L"SNMPv3 request from %s passed validation (user \"%hs\", %hs)", sourceAddr.toString().cstr(), request->getUser(),
      (flags & SNMP_PRIV_FLAG) ? "authPriv" : ((flags & SNMP_AUTH_FLAG) ? "authNoPriv" : "noAuthNoPriv"));
   ProcessRequest(transport, request);
}

/**
 * SNMP request receiver thread
 */
static void SNMPAgentReceiver()
{
   ThreadSetName("SNMPD");

   uint16_t snmpdPort = static_cast<uint16_t>(ConfigReadULong(_T("SNMP.Agent.ListenerPort"), 161));

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
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to create socket for SNMP agent (%s)"), GetLastSocketErrorText(buffer, 1024));
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
   servAddr.sin_port = htons(snmpdPort);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(snmpdPort);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv4 socket for SNMP agent (%s)"), GetLastSocketErrorText(buffer, 1024));
      bindFailures++;
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }

#ifdef WITH_IPV6
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv6 socket for SNMP agent (%s)"), GetLastSocketErrorText(buffer, 1024));
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
      nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP agent cannot start - cannot bind at least one socket"));
      return;
   }

   if (hSocket != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for SNMP requests on UDP socket %s:%u"), InetAddress(ntohl(servAddr.sin_addr.s_addr)).toString(ipAddrText), snmpdPort);
   }
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for SNMP requests on UDP socket %s:%u"), InetAddress(servAddr6.sin6_addr.s6_addr).toString(ipAddrText), snmpdPort);
   }
#endif

   SNMP_Transport *snmp = CreateTransport(hSocket);
#ifdef WITH_IPV6
   SNMP_Transport *snmp6 = CreateTransport(hSocket6);
#endif

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("SNMP agent started on port %u"), snmpdPort);

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
         // Message is authenticated and decrypted while being read, so configured SNMPv3 user has to be in place before that
         s_credentialsLock.lock();
         transport->setSecurityContext(s_usmContext);
         s_credentialsLock.unlock();

         int bytes = transport->readMessage(&pdu, 2000, (struct sockaddr *)&addr, &addrLen);
         if ((bytes > 0) && (pdu != nullptr))
         {
            InetAddress sourceAddr = InetAddress::createFromSockaddr((struct sockaddr *)&addr);
            nxlog_debug_tag(DEBUG_TAG, 6, L"SNMP agent received PDU of type %d from %s", pdu->getCommand(), sourceAddr.toString().cstr());

            if (!IsVersionAllowed(pdu->getVersion()))
            {
               nxlog_debug_tag(DEBUG_TAG, 5, L"SNMPv%hs request from %s rejected: version not allowed", SnmpVersionToName(pdu->getVersion()), sourceAddr.toString().cstr());
            }
            else if (pdu->getVersion() == SNMP_VERSION_3)
            {
               ProcessUsmRequest(transport, pdu, sourceAddr);
            }
            else
            {
               ProcessCommunityRequest(transport, pdu, sourceAddr);
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
   nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP agent stopped"));
}

/**
 * Agent receiver thread
 */
static THREAD s_receiverThread = INVALID_THREAD_HANDLE;

/**
 * SNMP agent object ID
 */
static SNMP_ObjectId s_agentObjectId { 1, 3, 6, 1, 4, 1, 57163, 1, 2 };

/**
 * Start SNMP agent
 */
void StartSNMPAgent()
{
   if (!ConfigReadBoolean(_T("SNMP.Agent.Enable"), false))
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Build-in SNMP agent is disabled"));
      return;
   }

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
   s_localEngine = SNMP_Engine(engineId, engineIdLen);
   // Engine time is wall clock time, so it never goes backwards across server restarts and managers
   // that cache it never see our boot counter (always 0) as stale
   s_localEngine.setTime(static_cast<uint32_t>(time(nullptr)));
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Local SNMP engine ID set to %s"), s_localEngine.toString().cstr());

   s_allowedVersions = ConfigReadULong(L"SNMP.Agent.AllowedVersions", 7);
   ConfigReadStrUTF8(L"SNMP.Agent.CommunityString", s_community, sizeof(s_community), "public");
   LoadUsmUser();

   // Setup MIB tree
   s_mib.add(new SingleValueMIBSubTree(SNMP_ObjectId::parse(_T(".1.3.6.1.2.1.1.1")),
      [] (const SNMP_ObjectId& oid) -> SNMP_Variable*
      {
         auto v = new SNMP_Variable(oid);
         v->setValueFromString(ASN_OCTET_STRING, _T("NetXMS Server ") NETXMS_VERSION_STRING);
         return v;
      }));
   s_mib.add(new SingleValueMIBSubTree(SNMP_ObjectId::parse(_T(".1.3.6.1.2.1.1.2")),
      [] (const SNMP_ObjectId& oid) -> SNMP_Variable*
      {
         auto v = new SNMP_Variable(oid);
         v->setValueFromObjectId(ASN_OBJECT_ID, s_agentObjectId);
         return v;
      }));
   s_mib.add(new SingleValueMIBSubTree(SNMP_ObjectId::parse(_T(".1.3.6.1.2.1.1.3")),
      [] (const SNMP_ObjectId& oid) -> SNMP_Variable*
      {
         auto v = new SNMP_Variable(oid);
         v->setValueFromUInt32(ASN_TIMETICKS, static_cast<uint32_t>(time(nullptr) - g_serverStartTime) * 10);
         return v;
      }));

   s_receiverThread = ThreadCreateEx(SNMPAgentReceiver);
}

/**
 * Stop SNMP agent
 */
void StopSNMPAgent()
{
   ThreadJoin(s_receiverThread);
}

/**
 * Process configuration changes related to SNMP agent
 */
void OnSNMPAgentConfigurationChange(const wchar_t *name, const wchar_t *value)
{
   if (!wcscmp(name, L"SNMP.Agent.CommunityString"))
   {
      s_credentialsLock.lock();
      memset(s_community, 0, sizeof(s_community));
      wchar_to_utf8(value, -1, s_community, sizeof(s_community) - 1);
      s_credentialsLock.unlock();
   }
   else if (!wcscmp(name, L"SNMP.Agent.AllowedVersions"))
   {
      s_allowedVersions = wcstoul(value, nullptr, 10);
      nxlog_debug_tag(DEBUG_TAG, 3, L"Allowed SNMP versions for built-in SNMP agent set to 0x%X", s_allowedVersions);
   }
   else if (!wcsncmp(name, L"SNMP.Agent.V3.", 14) && (s_receiverThread != INVALID_THREAD_HANDLE))
   {
      LoadUsmUser();
   }
}
