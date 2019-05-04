/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2019 Raden Solutions
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
** File: datacoll.cpp
**
**/

#include "nxagentd.h"
#include <socket_listener.h>

#define DEBUG_TAG _T("proxy")
#define LISTEN_PORT 4700

/**
 * Data collectors thread pool
 */
extern ThreadPool *g_dataCollectorPool;

HashMap<ProxyKey, DataCollectionProxy> g_proxyList;
HashMap<UINT64, ZoneConfiguration> *g_proxyserverConfList = new HashMap<UINT64, ZoneConfiguration>(true);
Mutex g_proxyListMutex;

/**
 * Indicator of scheduled proxy check
 */
static bool s_proxyConnectionCheckScheduled = false;

/**
 * Create new zone configuration object
 */
ZoneConfiguration::ZoneConfiguration(UINT64 serverId, UINT32 thisNodeId, UINT32 zoneUin, BYTE *sharedSecret)
{
   m_serverId = serverId;
   m_thisNodeId = thisNodeId;
   m_zoneUin = zoneUin;
   memcpy(m_sharedSecret, sharedSecret, ZONE_PROXY_KEY_LENGTH);
}

/**
 * Create copy of existing zone configuration object
 */
ZoneConfiguration::ZoneConfiguration(const ZoneConfiguration *cfg)
{
   m_serverId = cfg->m_serverId;
   m_thisNodeId = cfg->m_thisNodeId;
   m_zoneUin = cfg->m_zoneUin;
   memcpy(m_sharedSecret, cfg->m_sharedSecret, ZONE_PROXY_KEY_LENGTH);
}

/**
 * Update zone configuration from another configuration object
 */
void ZoneConfiguration::update(const ZoneConfiguration *cfg)
{
   m_thisNodeId = cfg->m_thisNodeId;
   m_zoneUin = cfg->m_zoneUin;
   memcpy(m_sharedSecret, cfg->m_sharedSecret, ZONE_PROXY_KEY_LENGTH);
}

/**
 * Save proxy configuration to database
 */
static void SaveProxyConfiguration(UINT64 serverId, HashMap<ProxyKey, DataCollectionProxy> *proxyList, const ZoneConfiguration *zone)
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   DBBegin(hdb);

   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM dc_proxy WHERE server_id=") UINT64_FMT, serverId);
   if (!DBQuery(hdb, query))
   {
      DBRollback(hdb);
      return;
   }
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dc_proxy (server_id,proxy_id,ip_address) VALUES (?,?,?)"), true);
   if (hStmt == NULL)
   {
      DBRollback(hdb);
      return;
   }

   bool success = true;
   Iterator<DataCollectionProxy> *it = proxyList->iterator();
   while(it->hasNext() && success)
   {
      DataCollectionProxy *dcp = it->next();
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, dcp->getServerId());
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, dcp->getProxyId());
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, dcp->getAddress());
      success = DBExecute(hStmt);
   }
   DBFreeStatement(hStmt);
   delete it;

   if(success)
   {
      _sntprintf(query, 256, _T("DELETE FROM zone_config WHERE server_id=") UINT64_FMT, serverId);
      if (!DBQuery(hdb, query))
      {
         success = false;
      }
   }


   if(success)
   {
      TCHAR sharedSecret[33];
      BinToStr(zone->getSharedSecret(), ZONE_PROXY_KEY_LENGTH, sharedSecret);
      _sntprintf(query, 256, _T("INSERT INTO zone_config (this_node_id,zone_uin,shared_secret,server_id) VALUES (%d,%d,'%s',") UINT64_FMT _T(")"),
            zone->getThisNodeId(), zone->getZoneUIN(), sharedSecret, serverId);
      if (!DBQuery(hdb, query))
      {
         success = false;
      }
   }

   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);
}

/**
 * Load proxy configuration from database
 */
void LoadProxyConfiguration()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT server_id,proxy_id,ip_address FROM dc_proxy"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int row = 0; row < count; row++)
      {
         DataCollectionProxy *proxy = new DataCollectionProxy(DBGetFieldInt64(hResult, row, 0), DBGetFieldULong(hResult, row, 1),
               DBGetFieldInetAddr(hResult, row, 2));
         g_proxyList.set(proxy->getKey(), proxy);
      }
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb, _T("SELECT server_id,this_node_id,zone_uin,shared_secret FROM zone_config"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int row = 0; row < count; row++)
      {
         TCHAR tmp[33];
         DBGetField(hResult, row, 2, tmp, 33);
         BYTE sharedSecret[ZONE_PROXY_KEY_LENGTH];
         StrToBin(tmp, sharedSecret, ZONE_PROXY_KEY_LENGTH);
         ZoneConfiguration *zone = new ZoneConfiguration(DBGetFieldInt64(hResult, row, 0), DBGetFieldULong(hResult, row, 1),
               DBGetFieldULong(hResult, row, 2), sharedSecret);
         g_proxyserverConfList->set(zone->getServerId(), zone);
      }
      DBFreeResult(hResult);
   }
}

/**
 * Connect to given host/port
 *
 * @return connected socket on success or INVALID_SOCKET on error
 */
SOCKET ConnectToHostUDP(const InetAddress& addr, UINT16 port)
{
   SOCKET s = socket(addr.getFamily(), SOCK_DGRAM, 0);
   if (s == INVALID_SOCKET)
      return INVALID_SOCKET;

   SockAddrBuffer saBuffer;
   struct sockaddr *sa = addr.fillSockAddr(&saBuffer, port);
   int rc = connect(s, sa, SA_LEN(sa));
   if (rc == -1)
   {
      closesocket(s);
      s = INVALID_SOCKET;
   }
   return s;
}

/**
 * Create new data collection proxy object
 */
DataCollectionProxy::DataCollectionProxy(UINT64 serverId, UINT32 proxyId, const InetAddress& address)
{
   m_serverId = serverId;
   m_proxyId = proxyId;
   m_address = address;
   m_connected = true;
   m_inUse = false;
}

/**
 * Create copy of existing data collection proxy object
 */
DataCollectionProxy::DataCollectionProxy(const DataCollectionProxy *src)
{
   m_serverId = src->m_serverId;
   m_proxyId = src->m_proxyId;
   m_address = src->m_address;
   m_connected = src->m_connected;
   m_inUse = src->m_inUse;
}

/**
 * Update from other object
 */
void DataCollectionProxy::update(const DataCollectionProxy *src)
{
   m_address = src->m_address;
   m_inUse = src->m_inUse;
}

/**
 * Check if proxy is reachable
 */
void DataCollectionProxy::checkConnection()
{
   SOCKET sd = ConnectToHostUDP(m_address, LISTEN_PORT);
   if (sd == INVALID_SOCKET)
      return;

   ZoneConfiguration *zone = g_proxyserverConfList->get(m_serverId);

   ProxyMsg request;
   GenerateRandomBytes(request.challenge, PROXY_CHALLENGE_SIZE);
   request.serverId = htonq(m_serverId);
   request.proxyIdDest = htonl(m_proxyId);
   request.proxyIdSelf = htonl(zone->getThisNodeId());
   request.zoneUin = htonl(zone->getZoneUIN());
   SignMessage(&request, sizeof(request) - sizeof(request.hmac), zone->getSharedSecret(), ZONE_PROXY_KEY_LENGTH, request.hmac);

   bool result = false;
   for(int retryCount = 5; retryCount > 0; retryCount--)
   {
#ifdef MSG_NOSIGNAL
      int bytes = send(sd, &request, sizeof(request), MSG_NOSIGNAL);
#else
      int bytes = send(sd, &request, sizeof(request), 0);
#endif
      if (bytes <= 0)
         continue;

      ProxyMsg response;
      bytes = RecvEx(sd, &response, sizeof(response), 0, 1000);
      if ((bytes == -2) || (bytes != sizeof(response)))
         continue;   // Timeout or invalid packet size

      if (bytes <= 0)
         break; // in case if socket was closed or on error just fail

      if (ValidateMessageSignature(&response, sizeof(response) - sizeof(response.hmac), zone->getSharedSecret(), ZONE_PROXY_KEY_LENGTH, response.hmac) &&
            !memcmp(response.challenge, request.challenge, PROXY_CHALLENGE_SIZE) &&
            (request.proxyIdDest == response.proxyIdSelf) &&
            (response.proxyIdDest == request.proxyIdSelf) &&
            (request.zoneUin == response.zoneUin))
      {
         result = true;
         break;
      }

      if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 7)
      {
         TCHAR challenge[PROXY_CHALLENGE_SIZE * 2 + 1];
         TCHAR hmac[SHA256_DIGEST_SIZE * 2 + 1];
         nxlog_debug_tag(DEBUG_TAG, 7, _T("DataCollectionProxy::checkConnection(): Invalid response message"));
         nxlog_debug_tag(DEBUG_TAG, 7, _T("   Request:  challenge=%s serverId=") UINT64X_FMT(_T("016")) _T(" zone=%u dest=%u self=%u HMAC=%s"),
                  BinToStr(request.challenge, PROXY_CHALLENGE_SIZE, challenge), request.serverId, request.zoneUin,
                  request.proxyIdDest, request.proxyIdSelf, BinToStr(request.hmac, SHA256_DIGEST_SIZE, hmac));
         nxlog_debug_tag(DEBUG_TAG, 7, _T("   Response: challenge=%s serverId=") UINT64X_FMT(_T("016")) _T(" zone=%u dest=%u self=%u HMAC=%s"),
                  BinToStr(response.challenge, PROXY_CHALLENGE_SIZE, challenge), response.serverId, response.zoneUin,
                  response.proxyIdDest, response.proxyIdSelf, BinToStr(response.hmac, SHA256_DIGEST_SIZE, hmac));
      }
   }

   m_connected = result;
   closesocket(sd);
}

/**
 * Thread checks if used in DCI proxy node is connected
 */
void ProxyConnectionChecker(void *arg)
{
   bool reschedule = false;
   g_proxyListMutex.lock();

   Iterator<DataCollectionProxy> *it = g_proxyList.iterator();
   while (it->hasNext())
   {
      DataCollectionProxy *dcProxy = it->next();
      if (dcProxy->isInUse())
      {
         reschedule = true;
         dcProxy->checkConnection();
      }
   }
   delete it;

   s_proxyConnectionCheckScheduled = reschedule;
   if (reschedule)
      ThreadPoolScheduleRelative(g_dataCollectorPool, 5000, ProxyConnectionChecker, NULL);

   g_proxyListMutex.unlock();
}

/**
 * Update proxy list on data collection configuration update
 */
void UpdateProxyConfiguration(UINT64 serverId, HashMap<ProxyKey, DataCollectionProxy> *proxyList, const ZoneConfiguration *zone)
{
   g_proxyListMutex.lock();
   Iterator<DataCollectionProxy> *it = proxyList->iterator();
   while(it->hasNext())
   {
      DataCollectionProxy *dcpNew = it->next();
      DataCollectionProxy *dcpOld = g_proxyList.get(dcpNew->getKey());
      if (dcpOld != NULL)
      {
         dcpOld->update(dcpNew);
      }
      else
      {
         g_proxyList.set(dcpNew->getKey(), new DataCollectionProxy(dcpNew));
      }
   }
   delete it;

   it = g_proxyList.iterator();
   while(it->hasNext())
   {
      DataCollectionProxy *dcpOld = it->next();
      if (dcpOld->getServerId() == serverId)
      {
         DataCollectionProxy *dcpNew = proxyList->get(dcpOld->getKey());
         if(dcpNew == NULL)
         {
            it->remove();
         }
      }
   }
   delete it;
   if(!s_proxyConnectionCheckScheduled)
      ThreadPoolScheduleRelative(g_dataCollectorPool, 0, ProxyConnectionChecker, NULL);

   ZoneConfiguration *oldZone = g_proxyserverConfList->get(serverId);
   if (oldZone == NULL)
   {
      g_proxyserverConfList->set(serverId, new ZoneConfiguration(zone));
   }
   else
   {
      oldZone->update(zone);
   }

   g_proxyListMutex.unlock();

   SaveProxyConfiguration(serverId, proxyList, zone);
}

/**
 * Client listener class
 */
class ProxyConnectionListener : public DatagramSocketListener
{
protected:
   virtual ConnectionProcessingResult processDatagram(SOCKET s);
   virtual bool isStopConditionReached();

public:
   ProxyConnectionListener(UINT16 port,  bool allowV4, bool allowV6) : DatagramSocketListener(port, allowV4, allowV6) { setName(_T("ProxyHeartbeat")); }
};

/**
 * Listener stop condition
 */
bool ProxyConnectionListener::isStopConditionReached()
{
   return IsShutdownInProgress();
}

/**
 * Process incoming message
 */
ConnectionProcessingResult ProxyConnectionListener::processDatagram(SOCKET s)
{
   ProxyMsg request;
   SockAddrBuffer addr;
   socklen_t addrLen = sizeof(SockAddrBuffer);
   int bytes = recvfrom(s, &request, sizeof(request), 0, (struct sockaddr *)&addr, &addrLen);
   if (bytes > 0)
   {
      g_proxyListMutex.lock();
      ZoneConfiguration *cfg = g_proxyserverConfList->get(ntohq(request.serverId));
      bool isValid = false;

      if ((cfg != NULL) &&
          ValidateMessageSignature(&request, sizeof(request) - sizeof(request.hmac), cfg->getSharedSecret(), ZONE_PROXY_KEY_LENGTH, request.hmac) &&
          (ntohl(request.proxyIdDest) == cfg->getThisNodeId()) &&
          (ntohl(request.zoneUin) == cfg->getZoneUIN()) &&
          g_proxyList.contains(ProxyKey(cfg->getServerId(), ntohl(request.proxyIdSelf))))
      {
         isValid = true;
         UINT32 tmp = request.proxyIdDest;
         request.proxyIdDest = request.proxyIdSelf;
         request.proxyIdSelf = tmp;
         SignMessage(&request, sizeof(request) - sizeof(request.hmac), cfg->getSharedSecret(), ZONE_PROXY_KEY_LENGTH, request.hmac);
      }
      g_proxyListMutex.unlock();

      if (isValid)
      {
         if (sendto(s, &request, sizeof(request), 0, (struct sockaddr *)&addr, addrLen) < 0)
         {
            TCHAR buffer[64];
            nxlog_debug_tag(DEBUG_TAG, 1, _T("ProxyConnectionListener: cannot send response to requester at %s"), SockaddrToStr((struct sockaddr *)&addr, buffer));
         }
      }
      else
      {
         TCHAR buffer[64];
         nxlog_debug_tag(DEBUG_TAG, 1, _T("ProxyConnectionListener: invalid packet drop: ip=%s, serverid=") UINT64X_FMT(_T("016")) _T(", nodeid=%d"),
                        SockaddrToStr((struct sockaddr *)&addr, buffer), ntohq(request.serverId), ntohl(request.proxyIdSelf));
      }
   }
   return CPR_COMPLETED;
}

/**
 * Listener thread
 */
THREAD_RESULT THREAD_CALL ProxyListenerThread(void *arg)
{
   ThreadSetName("ProxyHbLsnr");
   ProxyConnectionListener listener(LISTEN_PORT, (g_dwFlags & AF_DISABLE_IPV4) == 0, (g_dwFlags & AF_DISABLE_IPV6) == 0);
   if (!listener.initialize())
      return THREAD_OK;

   listener.mainLoop();
   listener.shutdown();
   return THREAD_OK;
}

/**
 * Handler for Agent.ZoneProxies table
 */
LONG H_ZoneProxies(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("SERVER_ID"), DCI_DT_STRING, _T("Server ID"), true);
   value->addColumn(_T("PROXY_ID"), DCI_DT_UINT, _T("Proxy ID"), true);
   value->addColumn(_T("ADDRESS"), DCI_DT_STRING, _T("Address"));
   value->addColumn(_T("CONNECTED"), DCI_DT_STRING, _T("Connected"));
   value->addColumn(_T("IN_USE"), DCI_DT_STRING, _T("In use"));

   g_proxyListMutex.lock();
   Iterator<DataCollectionProxy> *it = g_proxyList.iterator();
   while(it->hasNext())
   {
      DataCollectionProxy *proxy = it->next();

      TCHAR serverId[20];
      _sntprintf(serverId, 20, UINT64X_FMT(_T("016")), proxy->getServerId());

      value->addRow();
      value->set(0, serverId);
      value->set(1, proxy->getProxyId());
      value->set(2, proxy->getAddress().toString());
      value->set(3, proxy->isConnected() ? _T("YES") : _T("NO"));
      value->set(4, proxy->isInUse() ? _T("YES") : _T("NO"));
   }
   delete it;
   g_proxyListMutex.unlock();

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.ZoneConfigurations table
 */
LONG H_ZoneConfigurations(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("SERVER_ID"), DCI_DT_STRING, _T("Server ID"), true);
   value->addColumn(_T("ZONE_UIN"), DCI_DT_UINT, _T("Zone UIN"));
   value->addColumn(_T("LOCAL_ID"), DCI_DT_UINT, _T("Local ID"));
   value->addColumn(_T("SECRET"), DCI_DT_STRING, _T("Secret"));

   g_proxyListMutex.lock();
   Iterator<ZoneConfiguration> *it = g_proxyserverConfList->iterator();
   while(it->hasNext())
   {
      ZoneConfiguration *zone = it->next();

      TCHAR serverId[20];
      _sntprintf(serverId, 20, UINT64X_FMT(_T("016")), zone->getServerId());

      value->addRow();
      value->set(0, serverId);
      value->set(1, zone->getZoneUIN());
      value->set(2, zone->getThisNodeId());
      if (session->isMasterServer())
      {
         TCHAR text[ZONE_PROXY_KEY_LENGTH * 2 + 1];
         value->set(3, BinToStr(zone->getSharedSecret(), ZONE_PROXY_KEY_LENGTH, text));
      }
   }
   delete it;
   g_proxyListMutex.unlock();

   return SYSINFO_RC_SUCCESS;
}
