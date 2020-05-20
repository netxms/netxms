/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
** File: console.cpp
**
**/

#include "nxcore.h"
#include <entity_mib.h>

/**
 * Externals
 */
extern ObjectQueue<DiscoveredAddress> g_nodePollerQueue;
extern Queue g_syslogProcessingQueue;
extern Queue g_syslogWriteQueue;
extern ThreadPool *g_pollerThreadPool;
extern ThreadPool *g_schedulerThreadPool;
extern ThreadPool *g_dataCollectorThreadPool;
extern ThreadPool *g_npeThreadPool;
extern ThreadPool *g_syncerThreadPool;
extern ThreadPool *g_discoveryThreadPool;

void ShowPredictionEngines(CONSOLE_CTX console);
void ShowAgentTunnels(CONSOLE_CTX console);
UINT32 BindAgentTunnel(UINT32 tunnelId, UINT32 nodeId, UINT32 userId);
UINT32 UnbindAgentTunnel(UINT32 nodeId, UINT32 userId);
INT64 GetEventLogWriterQueueSize();
void DiscoveryPoller(PollerInfo *poller);
void RangeScanCallback(const InetAddress& addr, int32_t zoneUIN, const Node *proxy, uint32_t rtt, ServerConsole *console, void *context);
void CheckRange(const InetAddressListElement& range, void(*callback)(const InetAddress&, int32_t, const Node *, uint32_t, ServerConsole *, void *), ServerConsole *console, void *context);
void ShowSyncerStats(ServerConsole *console);

/**
 * Format string to show value of global flag
 */
#define SHOW_FLAG_VALUE(x) _T("  %-38s = %d\n"), _T(#x), (g_flags & x) ? 1 : 0

/**
 * Compare given string to command template with abbreviation possibility
 */
bool NXCORE_EXPORTABLE IsCommand(const TCHAR *cmdTemplate, const TCHAR *str, int minChars)
{
   TCHAR temp[256];
   _tcslcpy(temp, str, 256);
   _tcsupr(temp);

   int i;
   for(i = 0; temp[i] != 0; i++)
      if (temp[i] != cmdTemplate[i])
         return false;
   return i >= minChars;
}

/**
 * Show memory usage
 */
static void ShowMemoryUsage(ServerConsole *console)
{
   console->printf(_T("Alarms ...................: %.02f MB\n"), static_cast<double>(GetAlarmMemoryUsage()) / 1048576);
   console->printf(_T("Data collection cache ....: %.02f MB\n"), static_cast<double>(GetDCICacheMemoryUsage()) / 1048576);
   console->printf(_T("Raw DCI data write cache .: %.02f MB\n"), static_cast<double>(GetRawDataWriterMemoryUsage()) / 1048576);
   console->print(_T("\n"));
}

/**
 * Print ARP cache
 */
static void PrintArpCache(CONSOLE_CTX ctx, Node *node, ArpCache *arpCache)
{
   ConsolePrintf(ctx, _T("\x1b[1mIP address\x1b[0m      | \x1b[1mMAC address\x1b[0m       | \x1b[1mIfIndex\x1b[0m | \x1b[1mInterface\x1b[0m\n"));
   ConsolePrintf(ctx, _T("----------------+-------------------+---------+----------------------\n"));

   TCHAR ipAddrStr[64], macAddrStr[64];
   for(int i = 0; i < arpCache->size(); i++)
   {
      const ArpEntry *e = arpCache->get(i);
      shared_ptr<Interface> iface = node->findInterfaceByIndex(e->ifIndex);
      ConsolePrintf(ctx, _T("%-15s | %s | %7d | %-20s\n"), e->ipAddr.toString(ipAddrStr), e->macAddr.toString(macAddrStr),
         e->ifIndex, (iface != nullptr) ? iface->getName() : _T("\x1b[31;1mUNKNOWN\x1b[0m"));
   }
   ConsolePrintf(ctx, _T("\n%d entries\n\n"), arpCache->size());
}

/**
 * Dump index callback (by IP address)
 */
static void DumpIndexCallbackByInetAddr(const InetAddress& addr, NetObj *object, void *context)
{
   TCHAR buffer[64];
   static_cast<ServerConsole*>(context)->printf(_T("%-40s [%7u] %s [%d]\n"),
            addr.toString(buffer), object->getId(), object->getName(), (int)object->getId());
}

/**
 * Dump index (by IP address)
 */
void DumpIndex(CONSOLE_CTX pCtx, InetAddressIndex *index)
{
   index->forEach(DumpIndexCallbackByInetAddr, pCtx);
}

/**
 * Dump index callback (by GUID)
 */
static void DumpIndexCallbackByGUID(const uuid *guid, NetObj *object, void *context)
{
   TCHAR buffer[64];
   static_cast<ServerConsole*>(context)->printf(_T("%s [%7u] %s\n"), guid->toString(buffer), object->getId(), object->getName());
}

/**
 * Dump index (by ID)
 */
static void DumpIndex(ServerConsole *console, HashIndex<uuid> *index)
{
   index->forEach(DumpIndexCallbackByGUID, console);
}

/**
 * Dump index callback (by ID)
 */
static void DumpIndexCallbackById(NetObj *object, void *context)
{
   static_cast<ServerConsole*>(context)->printf(_T("%08X %p %s\n"), object->getId(), object, object->getName());
}

/**
 * Dump index (by ID)
 */
static void DumpIndex(ServerConsole *console, ObjectIndex *index)
{
   index->forEach(DumpIndexCallbackById, console);
}

/**
 * Compare debug tags for alphabetical sorting
 */
static int CompareDebugTags(const DebugTagInfo **t1, const DebugTagInfo **t2)
{
   return _tcsicmp((*t1)->tag, (*t2)->tag);
}

/**
 * Callback for address scan
 */
static void PrintScanCallback(const InetAddress& addr, int32_t zoneUIN, const Node *proxy, UINT32 rtt, ServerConsole *console, void *context)
{
   TCHAR ipAddrText[64];
   if (proxy != nullptr)
   {
      ConsolePrintf(console, _T("   Reply from %s to ICMP ping via proxy %s [%u]\n"),
            addr.toString(ipAddrText), proxy->getName(), proxy->getId());
   }
   else
   {
      ConsolePrintf(console, _T("   Reply from %s in %dms\n"), addr.toString(ipAddrText), rtt);
   }

}

/**
 * Callback for enumerating discovery queue
 */
static EnumerationCallbackResult ShowDiscoveryQueueElement(const DiscoveredAddress *address, CONSOLE_CTX console)
{
   static const TCHAR *sourceTypes[] = { _T("arp"), _T("route"), _T("agent reg"), _T("snmp trap"), _T("syslog"), _T("active") };
   TCHAR ipAddrText[48], nodeInfo[256];
   if (address->sourceNodeId != 0)
   {
      shared_ptr<NetObj> node = FindObjectById(address->sourceNodeId, OBJECT_NODE);
      _sntprintf(nodeInfo, 256, _T("%s [%u]"), (node != nullptr) ? node->getName() : _T("<unknown>"), address->sourceNodeId);
   }
   else
   {
      nodeInfo[0] = 0;
   }
   ConsolePrintf(console, _T("%-40s %-10s %s\n"),
            address->ipAddr.toString(ipAddrText), sourceTypes[address->sourceType], nodeInfo);
   return _CONTINUE;
}

/**
 * Process command entered from command line in standalone mode
 * Return TRUE if command was _T("down")
 */
int ProcessConsoleCommand(const TCHAR *pszCmdLine, CONSOLE_CTX pCtx)
{
   TCHAR szBuffer[256], *eptr;
   int nExitCode = CMD_EXIT_CONTINUE;

   // Get command
   const TCHAR *pArg = ExtractWord(pszCmdLine, szBuffer);

   if (IsCommand(_T("AT"), szBuffer, 2))
   {
      pArg = ExtractWord(pArg, szBuffer);
      if (szBuffer[0] == _T('+'))
      {
         int offset = _tcstoul(&szBuffer[1], nullptr, 0);
         AddOneTimeScheduledTask(_T("Execute.Script"), time(nullptr) + offset, pArg, nullptr, 0, 0, SYSTEM_ACCESS_FULL);//TODO: change to correct user
      }
      else
      {
         AddRecurrentScheduledTask(_T("Execute.Script"), szBuffer, pArg, nullptr, 0, 0, SYSTEM_ACCESS_FULL); //TODO: change to correct user
      }
   }
   else if (IsCommand(_T("CLEAR"), szBuffer, 5))
   {
      pArg = ExtractWord(pArg, szBuffer);
      if (IsCommand(_T("DBWRITER"), szBuffer, 8))
      {
         ExtractWord(pArg, szBuffer);
         ClearDBWriterData(pCtx, szBuffer);
      }
      else if (szBuffer[0] == 0)
      {
         ConsoleWrite(pCtx,
                  _T("Valid components:\n")
                  _T("   DBWriter Counters\n")
                  _T("   DBWriter DataQueue\n")
                  _T("\n"));
      }
      else
      {
         ConsoleWrite(pCtx, _T("Invalid component name\n"));
      }
   }
   else if (IsCommand(_T("DBCP"), szBuffer, 3))
   {
      ExtractWord(pArg, szBuffer);
      if (IsCommand(_T("RESET"), szBuffer, 5))
      {
         ConsoleWrite(pCtx, _T("Resetting database connection pool\n"));
         DBConnectionPoolReset();
         ConsoleWrite(pCtx, _T("Database connection pool reset completed\n"));
      }
      else
      {
         ConsoleWrite(pCtx, _T("Invalid subcommand\n"));
      }
   }
   else if (IsCommand(_T("DEBUG"), szBuffer, 2))
   {
      StringList *list = ParseCommandLine(pArg);
      if (list->size() == 0)
      {
         ConsolePrintf(pCtx, _T("Current debug levels:\n"));
         ConsolePrintf(pCtx, _T("   DEFAULT              = %d\n"), nxlog_get_debug_level());

         ObjectArray<DebugTagInfo> *tags = nxlog_get_all_debug_tags();
         tags->sort(CompareDebugTags);
         for(int i = 0; i < tags->size(); i++)
         {
            const DebugTagInfo *t = tags->get(i);
            ConsolePrintf(pCtx, _T("   %-20s = %d\n"), t->tag, t->level);
         }
         delete tags;
      }
      else
      {
         int index = (list->size() == 1) ? 0 : 1;
         int level;
         if (!_tcsicmp(list->get(index), _T("OFF")))
         {
            level = 0;
         }
         else if (IsCommand(_T("DEFAULT"), list->get(index), 3))
         {
            level = -1;
         }
         else
         {
            level = (int)_tcstol(list->get(index), &eptr, 0);
            if (*eptr != 0)
               level = -99;   // mark as invalid
         }
         if ((level >= -1) && (level <= 9))
         {
            if (list->size() == 1)
            {
               if (level < 0)
                  level = 0;
               nxlog_set_debug_level(level);
               ConsolePrintf(pCtx, (level == 0) ? _T("Debug mode turned off\n") : _T("Debug level set to %d\n"), level);
            }
            else if (list->size() == 2)
            {
               nxlog_set_debug_level_tag(list->get(0), level);
               if (level == -1)
                  ConsolePrintf(pCtx,  _T("Debug level for tag \"%s\" set to default\n"), list->get(0));
               else
                  ConsolePrintf(pCtx,  _T("Debug level for tag \"%s\" set to %d\n"), list->get(0), level);
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("ERROR: Invalid debug level\n\n"));
         }
      }
      delete list;
   }
   else if (IsCommand(_T("DOWN"), szBuffer, 4))
   {
      ConsoleWrite(pCtx, _T("Proceeding with server shutdown...\n"));
      nExitCode = CMD_EXIT_SHUTDOWN;
   }
   else if (IsCommand(_T("DUMP"), szBuffer, 4))
   {
      DumpProcess(pCtx);
   }
   else if (IsCommand(_T("GET"), szBuffer, 3))
   {
      ExtractWord(pArg, szBuffer);
      if (szBuffer[0] != 0)
      {
         TCHAR value[MAX_CONFIG_VALUE];
         ConfigReadStr(szBuffer, value, MAX_CONFIG_VALUE, _T(""));
         ConsolePrintf(pCtx, _T("%s = %s\n"), szBuffer, value);
      }
      else
      {
         ConsoleWrite(pCtx, _T("Variable name missing\n"));
      }
   }
   else if (IsCommand(_T("HKRUN"), szBuffer, 2))
   {
      ConsoleWrite(pCtx, _T("Starting housekeeper\n"));
      RunHouseKeeper();
   }
   else if (IsCommand(_T("LDAPSYNC"), szBuffer, 4))
   {
      LDAPConnection conn;
      conn.syncUsers();
   }
   else if (IsCommand(_T("LOG"), szBuffer, 3))
   {
      while(_istspace(*pArg))
         pArg++;
      if (*pArg != 0)
         DbgPrintf(0, _T("%s"), pArg);
   }
   else if (IsCommand(_T("LOGMARK"), szBuffer, 4))
   {
      DbgPrintf(0, _T("******* MARK *******"));
   }
   else if (IsCommand(_T("RAISE"), szBuffer, 5))
   {
      // Get argument
      ExtractWord(pArg, szBuffer);

      if (IsCommand(_T("ACCESS"), szBuffer, 6))
      {
         ConsoleWrite(pCtx, _T("Raising exception...\n"));
         char *p = nullptr;
         *p = 0;
      }
      else if (IsCommand(_T("BREAKPOINT"), szBuffer, 5))
      {
#ifdef _WIN32
         ConsoleWrite(pCtx, _T("Raising exception...\n"));
         RaiseException(EXCEPTION_BREAKPOINT, 0, 0, nullptr);
#else
         ConsoleWrite(pCtx, _T("ERROR: Not supported on current platform\n"));
#endif
      }
      else
      {
         ConsoleWrite(pCtx, _T("Invalid exception name; possible names are:\nACCESS BREAKPOINT\n"));
      }
   }
   else if (IsCommand(_T("EXIT"), szBuffer, 4))
   {
      if (pCtx->isRemote())
      {
         ConsoleWrite(pCtx, _T("Closing session...\n"));
         nExitCode = CMD_EXIT_CLOSE_SESSION;
      }
      else
      {
         ConsoleWrite(pCtx, _T("Cannot exit from local server console\n"));
      }
   }
   else if (IsCommand(_T("KILL"), szBuffer, 4))
   {
      ExtractWord(pArg, szBuffer);
      if (szBuffer[0] != 0)
      {
         int id = _tcstol(szBuffer, &eptr, 10);
         if (*eptr == 0)
         {
            if (KillClientSession(id))
            {
               ConsoleWrite(pCtx, _T("Session killed\n"));
            }
            else
            {
               ConsoleWrite(pCtx, _T("Invalid session ID\n"));
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("Invalid session ID\n"));
         }
      }
      else
      {
         ConsoleWrite(pCtx, _T("Session ID missing\n"));
      }
   }
   else if (IsCommand(_T("PING"), szBuffer, 4))
   {
      ExtractWord(pArg, szBuffer);
      if (szBuffer[0] != 0)
      {
         InetAddress addr = InetAddress::parse(szBuffer);
         if (addr.isValid())
         {
            UINT32 rtt;
            UINT32 rc = IcmpPing(addr, 1, 2000, &rtt, 128, false);
            switch(rc)
            {
               case ICMP_SUCCESS:
                  ConsolePrintf(pCtx, _T("Success, RTT = %d ms\n"), (int)rtt);
                  break;
               case ICMP_UNREACHABLE:
                  ConsolePrintf(pCtx, _T("Destination unreachable\n"));
                  break;
               case ICMP_TIMEOUT:
                  ConsolePrintf(pCtx, _T("Request timeout\n"));
                  break;
               case ICMP_RAW_SOCK_FAILED:
                  ConsolePrintf(pCtx, _T("Cannot create raw socket\n"));
                  break;
               case ICMP_API_ERROR:
                  ConsolePrintf(pCtx, _T("API error\n"));
                  break;
               default:
                  ConsolePrintf(pCtx, _T("ERROR %d\n"), (int)rc);
                  break;
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("Invalid IP address\n"));
         }
      }
      else
      {
         ConsoleWrite(pCtx, _T("Usage: PING <address>\n"));
      }
   }
   else if (IsCommand(_T("POLL"), szBuffer, 2))
   {
      pArg = ExtractWord(pArg, szBuffer);
      if (szBuffer[0] != 0)
      {
         int pollType;
         if (IsCommand(_T("CONFIGURATION"), szBuffer, 1))
         {
            pollType = 1;
         }
         else if (IsCommand(_T("DISCOVERY"), szBuffer, 1))
         {
            pollType = 6;
         }
         else if (IsCommand(_T("INSTANCE"), szBuffer, 1))
         {
            pollType = 5;
         }
         else if (IsCommand(_T("ROUTING-TABLE"), szBuffer, 1))
         {
            pollType = 4;
         }
         else if (IsCommand(_T("STATUS"), szBuffer, 1))
         {
            pollType = 2;
         }
         else if (IsCommand(_T("TOPOLOGY"), szBuffer, 1))
         {
            pollType = 3;
         }
         else
         {
            pollType = 0;
         }

         if (pollType > 0)
         {
            ExtractWord(pArg, szBuffer);
            UINT32 id = _tcstoul(szBuffer, nullptr, 0);
            if (id != 0)
            {
               shared_ptr<NetObj> object = FindObjectById(id);
               if ((object != nullptr) && object->isDataCollectionTarget())
               {
                  switch(pollType)
                  {
                     case 1:
                        static_cast<DataCollectionTarget*>(object.get())->startForcedConfigurationPoll();
                        ThreadPoolExecute(g_pollerThreadPool, static_pointer_cast<DataCollectionTarget>(object),
                                 &DataCollectionTarget::configurationPollWorkerEntry,
                                 RegisterPoller(PollerType::CONFIGURATION, object));
                        break;
                     case 2:
                        static_cast<DataCollectionTarget*>(object.get())->startForcedStatusPoll();
                        ThreadPoolExecute(g_pollerThreadPool, static_pointer_cast<DataCollectionTarget>(object),
                                 &DataCollectionTarget::statusPollWorkerEntry,
                                 RegisterPoller(PollerType::STATUS, object));
                        break;
                     case 3:
                        if (object->getObjectClass() == OBJECT_NODE)
                        {
                           static_cast<Node*>(object.get())->startForcedTopologyPoll();
                           ThreadPoolExecute(g_pollerThreadPool, static_pointer_cast<Node>(object),
                                    &Node::topologyPollWorkerEntry,
                                    RegisterPoller(PollerType::TOPOLOGY, object));
                        }
                        else
                        {
                           ConsolePrintf(pCtx, _T("ERROR: this poll type can only be initiated for node objects\n\n"));
                        }
                        break;
                     case 4:
                        if (object->getObjectClass() == OBJECT_NODE)
                        {
                           static_cast<Node*>(object.get())->startForcedRoutePoll();
                           ThreadPoolExecute(g_pollerThreadPool, static_pointer_cast<Node>(object),
                                    &Node::routingTablePollWorkerEntry,
                                    RegisterPoller(PollerType::ROUTING_TABLE, object));
                        }
                        else
                        {
                           ConsolePrintf(pCtx, _T("ERROR: this poll type can only be initiated for node objects\n\n"));
                        }
                        break;
                     case 5:
                        static_cast<DataCollectionTarget*>(object.get())->startForcedInstancePoll();
                        ThreadPoolExecute(g_pollerThreadPool, static_pointer_cast<DataCollectionTarget>(object),
                                 &DataCollectionTarget::instanceDiscoveryPollWorkerEntry,
                                 RegisterPoller(PollerType::INSTANCE_DISCOVERY, object));
                        break;
                     case 6:
                        if (object->getObjectClass() == OBJECT_NODE)
                        {
                           static_cast<Node*>(object.get())->startForcedDiscoveryPoll();
                           ThreadPoolExecute(g_pollerThreadPool, DiscoveryPoller, RegisterPoller(PollerType::DISCOVERY, object));
                        }
                        else
                        {
                           ConsolePrintf(pCtx, _T("ERROR: this poll type can only be initiated for node objects\n\n"));
                        }
                        break;
                  }
               }
               else
               {
                  ConsolePrintf(pCtx, _T("ERROR: data collection target with ID %d does not exist\n\n"), id);
               }
            }
            else
            {
               ConsoleWrite(pCtx, _T("ERROR: Invalid or missing object ID\n\n"));
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("Usage POLL [CONFIGURATION|DISCOVERY|STATUS|TOPOLOGY|INSTANCE|ROUTING-TABLE] <object>\n"));
         }
      }
      else
      {
         ConsoleWrite(pCtx, _T("Usage POLL [CONFIGURATION|DISCOVERY|STATUS|TOPOLOGY|INSTANCE|ROUTING-TABLE] <node>\n"));
      }
   }
   else if (IsCommand(_T("SCAN"), szBuffer, 4))
   {
      pArg = ExtractWord(pArg, szBuffer);
      if (szBuffer[0] != 0)
      {
         TCHAR addr2[256];
         pArg = ExtractWord(pArg, addr2);
         if (addr2[0] != 0)
         {
            InetAddress start = InetAddress::parse(szBuffer);
            InetAddress end = InetAddress::parse(addr2);
            pArg = ExtractWord(pArg, szBuffer);
            int32_t zoneUIN = 0;
            UINT32 proxyId = 0;
            bool doDiscovery = false;
            bool syntaxError = false;
            if (szBuffer[0] != 0)
            {
               if (IsCommand(_T("ZONE"), szBuffer, 1))
               {
                  pArg = ExtractWord(pArg, szBuffer);
                  proxyId = _tcstoul(szBuffer, nullptr, 0);
                  ExtractWord(pArg, szBuffer); // Extract next word if it is discovery
               }
               else if (IsCommand(_T("PROXY"), szBuffer, 1))
               {
                  pArg = ExtractWord(pArg, szBuffer);
                  zoneUIN = _tcstol(szBuffer, nullptr, 0);
                  ExtractWord(pArg, szBuffer);  // Extract next word if it is discovery
               }
               else if (IsCommand(_T("DISCOVERY"), szBuffer, 1))
               {
                  doDiscovery = true;
               }
               else
               {
                  syntaxError = true;
               }

               if (!doDiscovery && (szBuffer[0] != 0))
               {
                  if (IsCommand(_T("DISCOVERY"), szBuffer, 4)) // if "discovery" is after proxy/zone
                  {
                     doDiscovery = true;
                  }
                  else
                  {
                     syntaxError = true;
                  }
               }

            }
            if (!syntaxError)
            {
               if (start.isValid() && end.isValid())
               {
                  InetAddressListElement range(start, end, proxyId, zoneUIN);
                  ConsolePrintf(pCtx, _T("Scanning address range %s - %s\n"), start.toString(szBuffer), end.toString(addr2));
                  CheckRange(range, doDiscovery ? RangeScanCallback : PrintScanCallback, pCtx, nullptr);
                  ConsolePrintf(pCtx, _T("Address range %s - %s scan completed\n"), start.toString(szBuffer), end.toString(addr2));
               }
               else
               {
                  ConsolePrintf(pCtx, _T("Invalid address\n"));
               }
            }
            else
            {
               ConsolePrintf(pCtx, _T("Invalid parameter\n"));
            }
         }
         else
         {
            ConsolePrintf(pCtx, _T("End address missing\n"));
         }
      }
      else
      {
         ConsolePrintf(pCtx, _T("Start address missing\n"));
      }
   }
   else if (IsCommand(_T("SET"), szBuffer, 3))
   {
      pArg = ExtractWord(pArg, szBuffer);
      if (szBuffer[0] != 0)
      {
         TCHAR value[256];
         ExtractWord(pArg, value);
         if (ConfigWriteStr(szBuffer, value, TRUE, TRUE, TRUE))
         {
            ConsolePrintf(pCtx, _T("Configuration variable %s updated\n"), szBuffer);
         }
         else
         {
            ConsolePrintf(pCtx, _T("ERROR: cannot update configuration variable %s\n"), szBuffer);
         }
      }
      else
      {
         ConsolePrintf(pCtx, _T("Variable name missing\n"));
      }
   }
   else if (IsCommand(_T("SHOW"), szBuffer, 2))
   {
      // Get argument
      pArg = ExtractWord(pArg, szBuffer);

      if (IsCommand(_T("ARP"), szBuffer, 2))
      {
         // Get argument
         ExtractWord(pArg, szBuffer);
         UINT32 dwNode = _tcstoul(szBuffer, nullptr, 0);
         if (dwNode != 0)
         {
            shared_ptr<NetObj> pObject = FindObjectById(dwNode);
            if (pObject != nullptr)
            {
               if (pObject->getObjectClass() == OBJECT_NODE)
               {
                  ArpCache *arpCache = static_cast<Node*>(pObject.get())->getArpCache();
                  if (arpCache != nullptr)
                  {
                     PrintArpCache(pCtx, static_cast<Node*>(pObject.get()), arpCache);
                     arpCache->decRefCount();
                  }
                  else
                  {
                     ConsoleWrite(pCtx, _T("ERROR: Node does not have forwarding database information\n\n"));
                  }
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: Object is not a node\n\n"));
               }
            }
            else
            {
               ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode);
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("ERROR: Invalid or missing node ID\n\n"));
         }
      }
      else if (IsCommand(_T("COMPONENTS"), szBuffer, 1))
      {
         // Get argument
         ExtractWord(pArg, szBuffer);
         UINT32 dwNode = _tcstoul(szBuffer, nullptr, 0);
         if (dwNode != 0)
         {
            shared_ptr<NetObj> pObject = FindObjectById(dwNode);
            if (pObject != nullptr)
            {
               if (pObject->getObjectClass() == OBJECT_NODE)
               {
                  shared_ptr<ComponentTree> components = static_cast<Node*>(pObject.get())->getComponents();
                  if (components != nullptr)
                  {
                     components->print(pCtx);
                  }
                  else
                  {
                     ConsoleWrite(pCtx, _T("ERROR: Node does not have physical component information\n\n"));
                  }
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: Object is not a node\n\n"));
               }
            }
            else
            {
               ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode);
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("ERROR: Invalid or missing node ID\n\n"));
         }
      }
      else if (IsCommand(_T("DBCP"), szBuffer, 4))
      {
         ObjectArray<PoolConnectionInfo> *list = DBConnectionPoolGetConnectionList();
         for(int i = 0; i < list->size(); i++)
         {
            PoolConnectionInfo *c = list->get(i);
            TCHAR accessTime[64];
            ConsolePrintf(pCtx, _T("%p %s %hs:%d\n"), c->handle, FormatTimestamp(c->lastAccessTime, accessTime), c->srcFile, c->srcLine);
         }
         ConsolePrintf(pCtx, _T("%d database connections in use\n\n"), list->size());
         delete list;
      }
      else if (IsCommand(_T("DBSTATS"), szBuffer, 3))
      {
         LIBNXDB_PERF_COUNTERS counters;
         DBGetPerfCounters(&counters);
         ConsolePrintf(pCtx, _T("SQL query counters:\n"));
         ConsolePrintf(pCtx, _T("   Total .......... ") INT64_FMT _T("\n"), counters.totalQueries);
         ConsolePrintf(pCtx, _T("   SELECT ......... ") INT64_FMT _T("\n"), counters.selectQueries);
         ConsolePrintf(pCtx, _T("   Non-SELECT ..... ") INT64_FMT _T("\n"), counters.nonSelectQueries);
         ConsolePrintf(pCtx, _T("   Long running ... ") INT64_FMT _T("\n"), counters.longRunningQueries);
         ConsolePrintf(pCtx, _T("   Failed ......... ") INT64_FMT _T("\n"), counters.failedQueries);

         ConsolePrintf(pCtx, _T("Background writer requests:\n"));
         ConsolePrintf(pCtx, _T("   DCI data ....... ") INT64_FMT _T("\n"), g_idataWriteRequests);
         ConsolePrintf(pCtx, _T("   DCI raw data ... ") INT64_FMT _T("\n"), g_rawDataWriteRequests);
         ConsolePrintf(pCtx, _T("   Others ......... ") INT64_FMT _T("\n"), g_otherWriteRequests);
      }
      else if (IsCommand(_T("DISCOVERY"), szBuffer, 2))
      {
         ExtractWord(pArg, szBuffer);
         if (IsCommand(_T("QUEUE"), szBuffer, 1))
         {
            g_nodePollerQueue.forEach(ShowDiscoveryQueueElement, pCtx);
         }
         else
         {
            ConsoleWrite(pCtx, _T("Invalid subcommand\n"));
         }
      }
      else if (IsCommand(_T("FDB"), szBuffer, 3))
      {
         // Get argument
         ExtractWord(pArg, szBuffer);
         UINT32 dwNode = _tcstoul(szBuffer, nullptr, 0);
         if (dwNode != 0)
         {
            shared_ptr<NetObj> pObject = FindObjectById(dwNode);
            if (pObject != nullptr)
            {
               if (pObject->getObjectClass() == OBJECT_NODE)
               {
                  ForwardingDatabase *fdb = static_cast<Node*>(pObject.get())->getSwitchForwardingDatabase();
                  if (fdb != nullptr)
                  {
                     fdb->print(pCtx, static_cast<Node*>(pObject.get()));
                     fdb->decRefCount();
                  }
                  else
                  {
                     ConsoleWrite(pCtx, _T("ERROR: Node does not have forwarding database information\n\n"));
                  }
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: Object is not a node\n\n"));
               }
            }
            else
            {
               ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode);
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("ERROR: Invalid or missing node ID\n\n"));
         }
      }
      else if (IsCommand(_T("FLAGS"), szBuffer, 1))
      {
         ConsolePrintf(pCtx, _T("Flags: 0x") UINT64X_FMT(_T("016")) _T("\n"), g_flags);
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DAEMON));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_USE_SYSLOG));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_PASSIVE_NETWORK_DISCOVERY));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ACTIVE_NETWORK_DISCOVERY));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DELETE_EMPTY_SUBNETS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_SNMP_TRAPD));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_ZONING));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SYNC_NODE_NAMES_WITH_DNS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_CHECK_TRUSTED_NODES));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_NXSL_CONTAINER_FUNCTIONS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_USE_FQDN_FOR_NODE_NAMES));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DEBUG_CONSOLE_DISABLED));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_OBJECT_TRANSACTIONS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_WRITE_FULL_DUMP));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_RESOLVE_NODE_NAMES));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_CATCH_EXCEPTIONS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_HELPDESK_LINK_ACTIVE));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DB_LOCKED));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DB_CONNECTION_LOST));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_NO_NETWORK_CONNECTIVITY));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_EVENT_STORM_DETECTED));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SNMP_TRAP_DISCOVERY));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_TRAPS_FROM_UNMANAGED_NODES));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_RESOLVE_IP_FOR_EACH_STATUS_POLL));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_PERFDATA_STORAGE_DRIVER_LOADED));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_BACKGROUND_LOG_WRITER));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_CASE_INSENSITIVE_LOGINS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_TRAP_SOURCES_IN_ALL_ZONES));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SYSLOG_DISCOVERY));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_LOCAL_CONSOLE));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_CACHE_DB_ON_STARTUP));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_NXSL_FILE_IO_FUNCTIONS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_EMBEDDED_PYTHON));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DB_SUPPORTS_MERGE));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_PARALLEL_NETWORK_DISCOVERY));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SINGLE_TABLE_PERF_DATA));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_MERGE_DUPLICATE_NODES));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SYSTEMD_DAEMON));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_USE_SYSTEMD_JOURNAL));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_COLLECT_ICMP_STATISTICS));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_LOG_IN_JSON_FORMAT));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_LOG_TO_STDOUT));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SERVER_INITIALIZED));
         ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SHUTDOWN));
         ConsolePrintf(pCtx, _T("\n"));
      }
      else if (IsCommand(_T("HEAP"), szBuffer, 1))
      {
         ExtractWord(pArg, szBuffer);
         if (IsCommand(_T("DETAILS"), szBuffer, 1))
         {
            TCHAR *text = GetHeapInfo();
            if (text != nullptr)
            {
               ConsoleWrite(pCtx, text);
               ConsoleWrite(pCtx, _T("\n"));
               free(text);
            }
            else
            {
               ConsoleWrite(pCtx, _T("Error reading heap information\n"));
            }
         }
         else if (IsCommand(_T("SUMMARY"), szBuffer, 1))
         {
            INT64 allocated = GetAllocatedHeapMemory();
            INT64 active = GetActiveHeapMemory();
            INT64 mapped = GetMappedHeapMemory();
            if ((allocated != -1) && (active != -1) && (mapped != -1))
            {
               ConsolePrintf(pCtx, _T("Allocated ... : %0.1f MB\n"), (double)allocated / 1024.0 / 1024.0);
               ConsolePrintf(pCtx, _T("Active ...... : %0.1f MB\n"), (double)active / 1024.0 / 1024.0);
               ConsolePrintf(pCtx, _T("Mapped ...... : %0.1f MB\n"), (double)mapped / 1024.0 / 1024.0);
            }
            else
            {
               ConsoleWrite(pCtx, _T("Heap summary information unavailable\n"));
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("Invalid subcommand\n"));
         }
      }
      else if (IsCommand(_T("INDEX"), szBuffer, 1))
      {
         // Get argument
         pArg = ExtractWord(pArg, szBuffer);

         if (IsCommand(_T("CONDITION"), szBuffer, 1))
         {
            DumpIndex(pCtx, &g_idxConditionById);
         }
         else if (IsCommand(_T("GUID"), szBuffer, 1))
         {
            DumpIndex(pCtx, &g_idxObjectByGUID);
         }
         else if (IsCommand(_T("ID"), szBuffer, 2))
         {
            DumpIndex(pCtx, &g_idxObjectById);
         }
         else if (IsCommand(_T("INTERFACE"), szBuffer, 2))
         {
            pArg = ExtractWord(pArg, szBuffer);
            if (IsCommand(_T("ZONE"), szBuffer, 1))
            {
               ExtractWord(pArg, szBuffer);
               shared_ptr<Zone> zone = FindZoneByUIN(_tcstoul(szBuffer, nullptr, 0));
               if (zone != nullptr)
               {
                  zone->dumpInterfaceIndex(pCtx);
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: Invalid zone UIN\n\n"));
               }
            }
            else if (szBuffer[0] == 0)
            {
               DumpIndex(pCtx, &g_idxInterfaceByAddr);
            }
            else
            {
               ConsoleWrite(pCtx, _T("ERROR: Invalid index modifier\n\n"));
            }
         }
         else if (IsCommand(_T("NETMAP"), szBuffer, 4))
         {
            DumpIndex(pCtx, &g_idxNetMapById);
         }
         else if (IsCommand(_T("NODEADDR"), szBuffer, 5))
         {
            pArg = ExtractWord(pArg, szBuffer);
            if (IsCommand(_T("ZONE"), szBuffer, 1))
            {
               ExtractWord(pArg, szBuffer);
               shared_ptr<Zone> zone = FindZoneByUIN(_tcstoul(szBuffer, nullptr, 0));
               if (zone != nullptr)
               {
                  zone->dumpNodeIndex(pCtx);
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: Invalid zone UIN\n\n"));
               }
            }
            else if (szBuffer[0] == 0)
            {
               DumpIndex(pCtx, &g_idxNodeByAddr);
            }
            else
            {
               ConsoleWrite(pCtx, _T("ERROR: Invalid index modifier\n\n"));
            }
         }
         else if (IsCommand(_T("NODEID"), szBuffer, 5))
         {
            DumpIndex(pCtx, &g_idxNodeById);
         }
         else if (IsCommand(_T("SENSOR"), szBuffer, 2))
         {
            DumpIndex(pCtx, &g_idxSensorById);
         }
         else if (IsCommand(_T("SUBNET"), szBuffer, 2))
         {
            pArg = ExtractWord(pArg, szBuffer);
            if (IsCommand(_T("ZONE"), szBuffer, 1))
            {
               ExtractWord(pArg, szBuffer);
               shared_ptr<Zone> zone = FindZoneByUIN(_tcstoul(szBuffer, nullptr, 0));
               if (zone != nullptr)
               {
                  zone->dumpSubnetIndex(pCtx);
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: Invalid zone UIN\n\n"));
               }
            }
            else if (szBuffer[0] == 0)
            {
               DumpIndex(pCtx, &g_idxSubnetByAddr);
            }
            else
            {
               ConsoleWrite(pCtx, _T("ERROR: Invalid index modifier\n\n"));
            }
         }
         else if (IsCommand(_T("ZONE"), szBuffer, 1))
         {
            DumpIndex(pCtx, &g_idxZoneByUIN);
         }
         else
         {
            if (szBuffer[0] == 0)
               ConsoleWrite(pCtx, _T("ERROR: Missing parameters\n")
                                  _T("Syntax:\n   SHOW INDEX name [ZONE uin]\n")
                                  _T("Valid names are: CONDITION, ID, INTERFACE, NODEADDR, NODEID, SUBNET, ZONE\n\n"));
            else
               ConsoleWrite(pCtx, _T("ERROR: Invalid index name\n\n"));
         }
      }
      else if (IsCommand(_T("LLDP"), szBuffer, 4))
      {
         // Get argument
         ExtractWord(pArg, szBuffer);
         UINT32 dwNode = _tcstoul(szBuffer, nullptr, 0);
         if (dwNode != 0)
         {
            shared_ptr<NetObj> pObject = FindObjectById(dwNode);
            if (pObject != nullptr)
            {
               if (pObject->getObjectClass() == OBJECT_NODE)
               {
                  ((Node *)pObject.get())->showLLDPInfo(pCtx);
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: Object is not a node\n\n"));
               }
            }
            else
            {
               ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode);
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("ERROR: Invalid or missing node ID\n\n"));
         }
      }
      else if (IsCommand(_T("MEMUSAGE"), szBuffer, 3))
      {
         ShowMemoryUsage(pCtx);
      }
      else if (IsCommand(_T("MODULES"), szBuffer, 3))
      {
         ConsoleWrite(pCtx, _T("Loaded server modules:\n"));
         for(UINT32 i = 0; i < g_dwNumModules; i++)
         {
            ConsolePrintf(pCtx, _T("   %s\n"), g_pModuleList[i].szName);
         }
         ConsolePrintf(pCtx, _T("%d modules loaded\n"), g_dwNumModules);
      }
      else if (IsCommand(_T("MSGWQ"), szBuffer, 2))
      {
         String text = MsgWaitQueue::getDiagInfo();
         ConsoleWrite(pCtx, text);
         ConsoleWrite(pCtx, _T("\n"));
      }
      else if (IsCommand(_T("NDD"), szBuffer, 3))
      {
         PrintNetworkDeviceDriverList(pCtx);
      }
      else if (IsCommand(_T("OBJECTS"), szBuffer, 1))
      {
         // Get filter
         ExtractWord(pArg, szBuffer);
         StrStrip(szBuffer);
         DumpObjects(pCtx, (szBuffer[0] != 0) ? szBuffer : nullptr);
      }
      else if (IsCommand(_T("PE"), szBuffer, 2))
      {
         ShowPredictionEngines(pCtx);
      }
      else if (IsCommand(_T("POLLERS"), szBuffer, 2))
      {
         ShowPollers(pCtx);
      }
      else if (IsCommand(_T("QUEUES"), szBuffer, 1))
      {
         ShowThreadPoolPendingQueue(pCtx, g_dataCollectorThreadPool, _T("Data collector"));
         ShowQueueStats(pCtx, &g_dciCacheLoaderQueue, _T("DCI cache loader"));
         ShowQueueStats(pCtx, &g_templateUpdateQueue, _T("Template updates"));
         ShowQueueStats(pCtx, &g_dbWriterQueue, _T("Database writer"));
         ShowQueueStats(pCtx, GetIDataWriterQueueSize(), _T("Database writer (IData)"));
         ShowQueueStats(pCtx, GetRawDataWriterQueueSize(), _T("Database writer (raw DCI values)"));
         ShowQueueStats(pCtx, &g_eventQueue, _T("Event processor"));
         ShowQueueStats(pCtx, GetEventLogWriterQueueSize(), _T("Event log writer"));
         ShowThreadPoolPendingQueue(pCtx, g_pollerThreadPool, _T("Poller"));
         ShowQueueStats(pCtx, GetDiscoveryPollerQueueSize(), _T("Node discovery poller"));
         ShowQueueStats(pCtx, &g_syslogProcessingQueue, _T("Syslog processing"));
         ShowQueueStats(pCtx, &g_syslogWriteQueue, _T("Syslog writer"));
         ShowThreadPoolPendingQueue(pCtx, g_schedulerThreadPool, _T("Scheduler"));
         ConsolePrintf(pCtx, _T("\n"));
      }
      else if (IsCommand(_T("ROUTING-TABLE"), szBuffer, 1))
      {
         ExtractWord(pArg, szBuffer);
         uint32_t dwNode = _tcstoul(szBuffer, nullptr, 0);
         if (dwNode != 0)
         {
            shared_ptr<NetObj> pObject = FindObjectById(dwNode);
            if (pObject != nullptr)
            {
               if (pObject->getObjectClass() == OBJECT_NODE)
               {
                  ROUTING_TABLE *pRT;
                  TCHAR szIpAddr[16];
                  int i;

                  ConsolePrintf(pCtx, _T("Routing table for node %s:\n\n"), pObject->getName());
                  pRT = ((Node *)pObject.get())->getCachedRoutingTable();
                  if (pRT != nullptr)
                  {
                     for(i = 0; i < pRT->iNumEntries; i++)
                     {
                        _sntprintf(szBuffer, 256, _T("%s/%d"), IpToStr(pRT->pRoutes[i].dwDestAddr, szIpAddr),
                                   BitsInMask(pRT->pRoutes[i].dwDestMask));
                        ConsolePrintf(pCtx, _T("%-18s %-15s %-6d %d\n"), szBuffer,
                                      IpToStr(pRT->pRoutes[i].dwNextHop, szIpAddr),
                                      pRT->pRoutes[i].dwIfIndex, pRT->pRoutes[i].dwRouteType);
                     }
                     ConsoleWrite(pCtx, _T("\n"));
                  }
                  else
                  {
                     ConsoleWrite(pCtx, _T("Node doesn't have cached routing table\n\n"));
                  }
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: Object is not a node\n\n"));
               }
            }
            else
            {
               ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode);
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("ERROR: Invalid or missing node ID\n\n"));
         }
      }
      else if (IsCommand(_T("SESSIONS"), szBuffer, 2))
      {
         ConsoleWrite(pCtx, _T("\x1b[1mCLIENT SESSIONS\x1b[0m\n============================================================\n"));
         DumpClientSessions(pCtx);
         ConsoleWrite(pCtx, _T("\n\x1b[1mMOBILE DEVICE SESSIONS\x1b[0m\n============================================================\n"));
         DumpMobileDeviceSessions(pCtx);
      }
      else if (IsCommand(_T("SIZEOF"), szBuffer, 4))
      {
         pCtx->printf(_T("NetObj .............: %d\n"), static_cast<int>(sizeof(NetObj)));
         pCtx->printf(_T("   AccessPoint .....: %d\n"), static_cast<int>(sizeof(AccessPoint)));
         pCtx->printf(_T("   Cluster .........: %d\n"), static_cast<int>(sizeof(Cluster)));
         pCtx->printf(_T("   Container .......: %d\n"), static_cast<int>(sizeof(Container)));
         pCtx->printf(_T("   Interface .......: %d\n"), static_cast<int>(sizeof(Interface)));
         pCtx->printf(_T("   Node ............: %d\n"), static_cast<int>(sizeof(Node)));
         pCtx->printf(_T("   Sensor ..........: %d\n"), static_cast<int>(sizeof(Sensor)));
         pCtx->printf(_T("   Template ........: %d\n"), static_cast<int>(sizeof(Template)));
         pCtx->printf(_T("ComponentTree ......: %d\n"), static_cast<int>(sizeof(ComponentTree)));
         pCtx->printf(_T("Component ..........: %d\n"), static_cast<int>(sizeof(Component)));
         pCtx->printf(_T("HardwareComponent ..: %d\n"), static_cast<int>(sizeof(HardwareComponent)));
         pCtx->printf(_T("SoftwarePackage ....: %d\n"), static_cast<int>(sizeof(SoftwarePackage)));
         pCtx->printf(_T("WinPerfObject ......: %d\n"), static_cast<int>(sizeof(WinPerfObject)));
         pCtx->printf(_T("WirelessStationInfo : %d\n"), static_cast<int>(sizeof(WirelessStationInfo)));
         pCtx->printf(_T("Alarm ..............: %d\n"), static_cast<int>(sizeof(Alarm)));
         pCtx->printf(_T("DCItem .............: %d\n"), static_cast<int>(sizeof(DCItem)));
         pCtx->printf(_T("DCTable ............: %d\n"), static_cast<int>(sizeof(DCTable)));
         pCtx->printf(_T("ItemValue ..........: %d\n"), static_cast<int>(sizeof(ItemValue)));
         pCtx->printf(_T("Threshold ..........: %d\n"), static_cast<int>(sizeof(Threshold)));
      }
      else if (IsCommand(_T("STATS"), szBuffer, 2))
      {
         ShowServerStats(pCtx);
      }
      else if (IsCommand(_T("SYNCER"), szBuffer, 2))
      {
         ShowSyncerStats(pCtx);
      }
      else if (IsCommand(_T("THREADS"), szBuffer, 2))
      {
         ShowThreadPool(pCtx, g_mainThreadPool);
         ShowThreadPool(pCtx, g_pollerThreadPool);
         ShowThreadPool(pCtx, g_discoveryThreadPool);
         ShowThreadPool(pCtx, g_dataCollectorThreadPool);
         ShowThreadPool(pCtx, g_schedulerThreadPool);
         ShowThreadPool(pCtx, g_agentConnectionThreadPool);
         ShowThreadPool(pCtx, g_clientThreadPool);
         ShowThreadPool(pCtx, g_npeThreadPool);
         ShowThreadPool(pCtx, g_syncerThreadPool);
      }
      else if (IsCommand(_T("TOPOLOGY"), szBuffer, 1))
      {
         ExtractWord(pArg, szBuffer);
         UINT32 nodeId = _tcstoul(szBuffer, nullptr, 0);
         if (nodeId != 0)
         {
            shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(nodeId, OBJECT_NODE));
            if (node != nullptr)
            {
               LinkLayerNeighbors *nbs = BuildLinkLayerNeighborList(node.get());
               if (nbs != nullptr)
               {
                  ConsolePrintf(pCtx, _T("Proto   | PtP | ifLocal | ifRemote | Peer\n")
                                      _T("--------+-----+---------+----------+------------------------------------\n"));
                  for(int i = 0; i < nbs->size(); i++)
                  {
                     LL_NEIGHBOR_INFO *ni = nbs->getConnection(i);
                     TCHAR peer[256];
                     if (ni->objectId != 0)
                     {
                        shared_ptr<NetObj> object = FindObjectById(ni->objectId);
                        if (object != nullptr)
                           _sntprintf(peer, 256, _T("%s [%d]"), object->getName(), ni->objectId);
                        else
                           _sntprintf(peer, 256, _T("[%d]"), ni->objectId);
                     }
                     else
                     {
                        peer[0] = 0;
                     }
                     ConsolePrintf(pCtx, _T("%-7s | %c   | %7d | %7d | %s\n"),
                        GetLinkLayerProtocolName(ni->protocol), ni->isPtToPt ? _T('Y') : _T('N'), ni->ifLocal, ni->ifRemote, peer);
                  }
                  nbs->decRefCount();
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: call to BuildLinkLayerNeighborList failed\n\n"));
               }
            }
            else
            {
               ConsolePrintf(pCtx, _T("ERROR: Node with ID %d does not exist\n\n"), nodeId);
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("ERROR: Invalid or missing node ID\n\n"));
         }
      }
      else if (IsCommand(_T("TUNNELS"), szBuffer, 2))
      {
         ShowAgentTunnels(pCtx);
      }
      else if (IsCommand(_T("USERS"), szBuffer, 1))
      {
         DumpUsers(pCtx);
      }
      else if (IsCommand(_T("VLANS"), szBuffer, 1))
      {
         ExtractWord(pArg, szBuffer);
         uint32_t nodeId = _tcstoul(szBuffer, nullptr, 0);
         if (nodeId != 0)
         {
            shared_ptr<NetObj> object = FindObjectById(nodeId);
            if (object != nullptr)
            {
               if (object->getObjectClass() == OBJECT_NODE)
               {
                  shared_ptr<VlanList> vlans = static_cast<Node*>(object.get())->getVlans();
                  if (vlans != nullptr)
                  {
                     ConsoleWrite(pCtx, _T("\x1b[1mVLAN\x1b[0m | \x1b[1mName\x1b[0m             | \x1b[1mPorts\x1b[0m\n")
                                        _T("-----+------------------+-----------------------------------------------------------------\n"));
                     for(int i = 0; i < vlans->size(); i++)
                     {
                        VlanInfo *vlan = vlans->get(i);
                        ConsolePrintf(pCtx, _T("%4d | %-16s |"), vlan->getVlanId(), vlan->getName());
                        for(int j = 0; j < vlan->getNumPorts(); j++)
                        {
                           TCHAR buffer[128];
                           ConsolePrintf(pCtx, _T(" %s"), vlan->getPorts()[j].location.toString(buffer, 128));
                        }
                        ConsolePrintf(pCtx, _T("\n"));
                     }
                     ConsolePrintf(pCtx, _T("\n"));
                  }
                  else
                  {
                     ConsoleWrite(pCtx, _T("\x1b[31mNode doesn't have VLAN information\x1b[0m\n\n"));
                  }
               }
               else
               {
                  ConsoleWrite(pCtx, _T("\x1b[31mERROR: Object is not a node\x1b[0m\n\n"));
               }
            }
            else
            {
               ConsolePrintf(pCtx, _T("\x1b[31mERROR: Object with ID %d does not exist\x1b[0m\n\n"), nodeId);
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("\x1b[31mERROR: Invalid or missing node ID\x1b[0m\n\n"));
         }
      }
      else if (IsCommand(_T("WATCHDOG"), szBuffer, 1))
      {
         WatchdogPrintStatus(pCtx);
         ConsoleWrite(pCtx, _T("\n"));
      }
      else
      {
         if (szBuffer[0] == 0)
            ConsoleWrite(pCtx, _T("ERROR: Missing subcommand\n\n"));
         else
            ConsoleWrite(pCtx, _T("ERROR: Invalid SHOW subcommand\n\n"));
      }
   }
   else if (IsCommand(_T("SNAPSHOT"), szBuffer, 4))
   {
      // create access snapshot
      ExtractWord(pArg, szBuffer);
      UINT32 userId = _tcstoul(szBuffer, nullptr, 0);
      bool success = CreateObjectAccessSnapshot(userId, OBJECT_NODE);
      ConsolePrintf(pCtx, _T("Object access snapshot creation for user %d %s\n\n"), userId, success ? _T("successful") : _T("failed"));
   }
   else if (IsCommand(_T("EXEC"), szBuffer, 3))
   {
      pArg = ExtractWord(pArg, szBuffer);

      bool libraryLocked = true;
      bool destroyCompiledScript = false;
      NXSL_Library *scriptLibrary = GetServerScriptLibrary();
      scriptLibrary->lock();

      NXSL_Program *compiledScript = scriptLibrary->findNxslProgram(szBuffer);
      if (compiledScript == nullptr)
      {
         scriptLibrary->unlock();
         libraryLocked = false;
         destroyCompiledScript = true;
         char *script;
         UINT32 fileSize;
         if ((script = (char *)LoadFile(szBuffer, &fileSize)) != nullptr)
         {
            const int errorMsgLen = 512;
            TCHAR errorMsg[errorMsgLen];
#ifdef UNICODE
            WCHAR *wscript = WideStringFromMBString(script);
            compiledScript = NXSLCompile(wscript, errorMsg, errorMsgLen, nullptr);
            free(wscript);
#else
            compiledScript = NXSLCompile(script, errorMsg, errorMsgLen, nullptr);
#endif
            free(script);
            if (compiledScript == nullptr)
            {
               ConsolePrintf(pCtx, _T("ERROR: Script compilation error: %s\n\n"), errorMsg);
            }
         }
         else
         {
            ConsolePrintf(pCtx, _T("ERROR: Script \"%s\" not found\n\n"), szBuffer);
         }
      }

      if (compiledScript != nullptr)
      {
         NXSL_ServerEnv *pEnv = new NXSL_ServerEnv;
         pEnv->setConsole(pCtx);

         NXSL_VM *vm = new NXSL_VM(pEnv);
         if (vm->load(compiledScript))
         {
            if (libraryLocked)
            {
               scriptLibrary->unlock();
               libraryLocked = false;
            }

            NXSL_Value *argv[32];
            int argc = 0;
            while(argc < 32)
            {
               pArg = ExtractWord(pArg, szBuffer);
               if (szBuffer[0] == 0)
                  break;
               argv[argc++] = vm->createValue(szBuffer);
            }

            if (vm->run(argc, argv))
            {
               NXSL_Value *pValue = vm->getResult();
               int retCode = pValue->getValueAsInt32();
               ConsolePrintf(pCtx, _T("INFO: Script finished with rc=%d\n\n"), retCode);
            }
            else
            {
               ConsolePrintf(pCtx, _T("ERROR: Script finished with error: %s\n\n"), vm->getErrorText());
            }
         }
         else
         {
            ConsolePrintf(pCtx, _T("ERROR: VM creation failed: %s\n\n"), vm->getErrorText());
         }
         delete vm;
         if (destroyCompiledScript)
            delete compiledScript;
      }
      if (libraryLocked)
         scriptLibrary->unlock();
   }
   else if (IsCommand(_T("TCPPING"), szBuffer, 4))
   {
      pArg = ExtractWord(pArg, szBuffer);
      if (szBuffer[0] != 0)
      {
         InetAddress addr = InetAddress::parse(szBuffer);
         if (addr.isValid())
         {
            ExtractWord(pArg, szBuffer);
            if (szBuffer[0] != 0)
            {
               UINT16 port = static_cast<UINT16>(_tcstoul(szBuffer, nullptr, 0));
               UINT32 rc = TcpPing(addr, port, 1000);
               switch(rc)
               {
                  case TCP_PING_SUCCESS:
                     ConsolePrintf(pCtx, _T("Success\n"));
                     break;
                  case TCP_PING_REJECT:
                     ConsolePrintf(pCtx, _T("Connection rejected\n"));
                     break;
                  case TCP_PING_SOCKET_ERROR:
                     ConsolePrintf(pCtx, _T("Socket error\n"));
                     break;
                  case TCP_PING_TIMEOUT:
                     ConsolePrintf(pCtx, _T("Timeout\n"));
                     break;
               }
            }
            else
            {
               ConsoleWrite(pCtx, _T("Usage: TCPPING <address> <port>\n"));
            }
         }
         else
         {
            ConsoleWrite(pCtx, _T("Invalid IP address\n"));
         }
      }
      else
      {
         ConsoleWrite(pCtx, _T("Usage: TCPPING <address> <port>\n"));
      }
   }
   else if (IsCommand(_T("TRACE"), szBuffer, 1))
   {
      NetworkPath *pTrace;
      TCHAR szNextHop[16];
      int i;

      // Get arguments
      pArg = ExtractWord(pArg, szBuffer);
      uint32_t dwNode1 = _tcstoul(szBuffer, nullptr, 0);

      ExtractWord(pArg, szBuffer);
      uint32_t dwNode2 = _tcstoul(szBuffer, nullptr, 0);

      if ((dwNode1 != 0) && (dwNode2 != 0))
      {
         shared_ptr<NetObj> pObject1 = FindObjectById(dwNode1);
         if (pObject1 == nullptr)
         {
            ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode1);
         }
         else
         {
            shared_ptr<NetObj> pObject2 = FindObjectById(dwNode2);
            if (pObject2 == nullptr)
            {
               ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode2);
            }
            else
            {
               if ((pObject1->getObjectClass() == OBJECT_NODE) && (pObject2->getObjectClass() == OBJECT_NODE))
               {
                  pTrace = TraceRoute(static_pointer_cast<Node>(pObject1), static_pointer_cast<Node>(pObject2));
                  if (pTrace != nullptr)
                  {
                     TCHAR sourceIp[32];
                     ConsolePrintf(pCtx, _T("Trace from %s to %s (%d hops, %s, source IP %s):\n"),
                           pObject1->getName(), pObject2->getName(), pTrace->getHopCount(),
                           pTrace->isComplete() ? _T("complete") : _T("incomplete"),
                           pTrace->getSourceAddress().toString(sourceIp));
                     shared_ptr<Interface> iface;
                     for(i = 0; i < pTrace->getHopCount(); i++)
                     {
                        NetworkPathElement *hop = pTrace->getHopInfo(i);
                        switch(hop->type)
                        {
                           case NetworkPathElementType::DUMMY:
                              ConsolePrintf(pCtx, _T("[%u] %s\n"),
                                    hop->object->getId(),
                                    hop->object->getName());
                              break;
                           case NetworkPathElementType::PROXY:
                              ConsolePrintf(pCtx, _T("[%u] %s ==> proxy link via %s [%u]\n"),
                                    hop->object->getId(),
                                    hop->object->getName(),
                                    GetObjectName(hop->ifIndex, _T("(unknown)")),
                                    hop->ifIndex);
                              break;
                           case NetworkPathElementType::ROUTE:
                              iface = static_cast<Node&>(*hop->object).findInterfaceByIndex(hop->ifIndex);
                              ConsolePrintf(pCtx, _T("[%u] %s ==> %s via %s [%u]\n"),
                                    hop->object->getId(),
                                    hop->object->getName(),
                                    hop->nextHop.toString(szNextHop),
                                    (iface != nullptr) ? iface->getName() : _T("unknown"),
                                    hop->ifIndex);
                              break;
                           case NetworkPathElementType::VPN:
                              ConsolePrintf(pCtx, _T("[%u] %s ==> VPN via %u\n"),
                                    hop->object->getId(),
                                    hop->object->getName(),
                                    GetObjectName(hop->ifIndex, _T("(unknown)")),
                                    hop->ifIndex);
                              break;
                        }
                     }
                     delete pTrace;
                     ConsolePrintf(pCtx, _T("\n"));
                  }
                  else
                  {
                     ConsoleWrite(pCtx, _T("ERROR: Call to TraceRoute() failed\n\n"));
                  }
               }
               else
               {
                  ConsoleWrite(pCtx, _T("ERROR: Object is not a node\n\n"));
               }
            }
         }
      }
      else
      {
         ConsoleWrite(pCtx, _T("ERROR: Invalid or missing node id(s)\n\n"));
      }
   }
   else if (IsCommand(_T("TUNNEL"), szBuffer, 2))
   {
      pArg = ExtractWord(pArg, szBuffer);
      if (IsCommand(_T("BIND"), szBuffer, 1))
      {
         pArg = ExtractWord(pArg, szBuffer);
         UINT32 tunnelId = _tcstoul(szBuffer, nullptr, 0);

         ExtractWord(pArg, szBuffer);
         UINT32 nodeId = _tcstoul(szBuffer, nullptr, 0);

         if ((tunnelId != 0) && (nodeId != 0))
         {
            UINT32 rcc = BindAgentTunnel(tunnelId, nodeId, 0);
            ConsolePrintf(pCtx, _T("Bind tunnel %d to node %d: RCC = %d\n\n"), tunnelId, nodeId, rcc);
         }
         else
         {
            ConsoleWrite(pCtx, _T("ERROR: Invalid or missing argument(s)\n\n"));
         }
      }
      else if (IsCommand(_T("UNBIND"), szBuffer, 1))
      {
         ExtractWord(pArg, szBuffer);
         UINT32 nodeId = _tcstoul(szBuffer, nullptr, 0);

         if (nodeId != 0)
         {
            UINT32 rcc = UnbindAgentTunnel(nodeId, 0);
            ConsolePrintf(pCtx, _T("Unbind tunnel from node %d: RCC = %d\n\n"), nodeId, rcc);
         }
         else
         {
            ConsoleWrite(pCtx, _T("ERROR: Invalid or missing argument(s)\n\n"));
         }
      }
      else
      {
         ConsoleWrite(pCtx, _T("ERROR: Invalid TUNNEL subcommand\n\n"));
      }
   }
   else if (IsCommand(_T("HELP"), szBuffer, 2) || IsCommand(_T("?"), szBuffer, 1))
   {
      ConsoleWrite(pCtx,
            _T("Valid commands are:\n")
            _T("   at +<sec> <script> [<params>]     - Schedule one time script execution task\n")
            _T("   at <schedule> <script> [<params>] - Schedule repeated script execution task\n")
            _T("   clear                             - Show list of valid component names for clearing\n")
            _T("   clear <component>                 - Clear internal data or queue for given component\n")
            _T("   dbcp reset                        - Reset database connection pool\n")
            _T("   debug [<level>|off]               - Set debug level (valid range is 0..9)\n")
            _T("   debug [<debug tag> <level>|off|default]\n")
            _T("                                     - Set debug level for a particular debug tag\n")
            _T("   down                              - Shutdown NetXMS server\n")
            _T("   exec <script> [<params>]          - Executes NXSL script from script library\n")
            _T("   exit                              - Exit from remote session\n")
            _T("   kill <session>                    - Kill client session\n")
            _T("   get <variable>                    - Get value of server configuration variable\n")
            _T("   help                              - Display this help\n")
            _T("   hkrun                             - Run housekeeper immediately\n")
            _T("   ldapsync                          - Synchronize ldap users with local user database\n")
            _T("   log <text>                        - Write given text to server log file\n")
            _T("   logmark                           - Write marker ******* MARK ******* to server log file\n")
            _T("   ping <address>                    - Send ICMP echo request to given IP address\n")
            _T("   poll <type> <node>                - Initiate node poll\n")
            _T("   raise <exception>                 - Raise exception\n")
            _T("   scan rangeStart rangeEnd [proxy <id>|zone <uin>] [discovery] \n")
            _T("                                     - Manual active discovery scan for given range. Without 'discovery' parameter prints results only\n")
            _T("   set <variable> <value>            - Set value of server configuration variable\n")
            _T("   show arp <node>                   - Show ARP cache for node\n")
            _T("   show components <node>            - Show physical components of given node\n")
            _T("   show dbcp                         - Show active sessions in database connection pool\n")
            _T("   show dbstats                      - Show DB library statistics\n")
            _T("   show discovery queue              - Show content of network discovery queue\n")
            _T("   show fdb <node>                   - Show forwarding database for node\n")
            _T("   show flags                        - Show internal server flags\n")
            _T("   show heap details                 - Show detailed heap information\n")
            _T("   show heap summary                 - Show heap usage summary\n")
            _T("   show index <index>                - Show internal index\n")
            _T("   show modules                      - Show loaded server modules\n")
            _T("   show msgwq                        - Show message wait queues information\n")
            _T("   show ndd                          - Show loaded network device drivers\n")
            _T("   show objects [<filter>]           - Dump network objects to screen\n")
            _T("   show pe                           - Show registered prediction engines\n")
            _T("   show pollers                      - Show poller threads state information\n")
            _T("   show queues                       - Show internal queues statistics\n")
            _T("   show routing-table <node>         - Show cached routing table for node\n")
            _T("   show sessions                     - Show active client sessions\n")
            _T("   show stats                        - Show global server statistics\n")
            _T("   show syncer                       - Show syncer statistics\n")
            _T("   show topology <node>              - Collect and show link layer topology for node\n")
            _T("   show tunnels                      - Show active agent tunnels\n")
            _T("   show users                        - Show users\n")
            _T("   show vlans <node>                 - Show cached VLAN information for node\n")
            _T("   show watchdog                     - Display watchdog information\n")
            _T("   trace <node1> <node2>             - Show network path trace between two nodes\n")
            _T("   tunnel bind <tunnel> <node>       - Bind agent tunnel to node\n")
            _T("   tunnel unbind <node>              - Unbind agent tunnel from node\n")
            _T("\nAlmost all commands can be abbreviated to 2 or 3 characters\n")
            _T("\n"));
   }
   else
   {
      bool processed = false;
      ENUMERATE_MODULES(pfProcessServerConsoleCommand)
      {
         if (CURRENT_MODULE.pfProcessServerConsoleCommand(pszCmdLine, pCtx))
         {
            processed = true;
            break;
         }
      }

      if (!processed)
         ConsoleWrite(pCtx, _T("UNKNOWN COMMAND\n\n"));
   }

   return nExitCode;
}
