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
** File: tools.cpp
**
**/

#include "nxcore.h"

#ifdef _WIN32
#include <io.h>
#else
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#endif

/**
 * Get system information string
 */
void GetSysInfoStr(TCHAR *pszBuffer, int nMaxSize)
{
#ifdef _WIN32
   DWORD dwSize;
   TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1] = _T("localhost"), osVersion[256] = _T("unknown");

   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(computerName, &dwSize);

	GetWindowsVersionString(osVersion, 256);
   _sntprintf(pszBuffer, nMaxSize, _T("%s %s"), computerName, osVersion);
#else
# ifdef HAVE_SYS_UTSNAME_H
	struct utsname uName;
	if (uname(&uName) >= 0)
	{
		_sntprintf(pszBuffer, nMaxSize, _T("%hs %hs Release %hs"), uName.nodename, uName.sysname, uName.release);
	}
	else
	{
#if HAVE_POSIX_STRERROR_R
		_tcserror_r(errno, pszBuffer, nMaxSize);
#else
      _tcslcpy(pszBuffer, _tcserror(errno), nMaxSize);
#endif
	}
# else
   _tprintf(_T("GetSysInfoStr: code not implemented\n"));
   _tcscpy(pszBuffer, _T("UNIX"));
# endif // HAVE_SYS_UTSNAME_H

#endif
}

/**
 * Get IP address for local machine
 */
InetAddress GetLocalIPAddress()
{
   InetAddress addr;
   InterfaceList *ifList = GetLocalInterfaceList();
   if (ifList == nullptr)
      return addr;

   // Find first interface with IP address
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_SOFTWARE_LOOPBACK)
         continue;
      for(int j = 0; j < iface->ipAddrList.size(); j++)
      {
         const InetAddress& a = iface->ipAddrList.get(j);
         if (a.isValidUnicast())
         {
            addr = a;
            goto stop;
         }
      }
   }
stop:
   delete ifList;
   return addr;
}

/**
 * Check if given IP address is a local IP address
 */
bool IsLocalIPAddress(const InetAddress& addr)
{
   InterfaceList *ifList = GetLocalInterfaceList();
   if (ifList == nullptr)
      return false;

   bool found = false;
   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_SOFTWARE_LOOPBACK)
         continue;
      for(int j = 0; j < iface->ipAddrList.size(); j++)
      {
         if (addr.equals(iface->ipAddrList.get(j)))
         {
            found = true;
            goto stop;
         }
      }
   }
stop:
   delete ifList;
   return found;
}

/**
 *  Splits command line
 */
StringList NXCORE_EXPORTABLE SplitCommandLine(const wchar_t *command)
{
   StringList elements;
   StringBuffer tmp;
   int state = 0;
   int size = (int)_tcslen(command);
   for(int i = 0; i < size; i++)
   {
      wchar_t c = command[i];
      switch(state)
      {
         case 0:
            if (c == _T(' '))
            {
               elements.add(tmp);
               tmp.clear();
               state = 3;
            }
            else if (c == _T('"'))
            {
               state = 1;
            }
            else if (c == _T('\''))
            {
               state = 2;
            }
            else
            {
               tmp.append(c);
            }
            break;
         case 1: // double quoted string
            if (c == _T('"'))
            {
               state = 0;
            }
            else
            {
               tmp.append(c);
            }
            break;
         case 2: // single quoted string
            if (c == '\'')
            {
               state = 0;
            }
            else
            {
               tmp.append(c);
            }
            break;
         case 3: // skip
            if (c != _T(' '))
            {
               if (c == _T('"'))
               {
                  state = 1;
               }
               else if (c == '\'')
               {
                  state = 2;
               }
               else
               {
                  tmp.append(c);
                  state = 0;
               }
            }
            break;
      }
   }
   if (state != 3)
      elements.add(tmp);

   return elements;
}

/**
 * Send Wake-on-LAN packet (magic packet) to given IP address
 * with given MAC address inside
 */
bool NXCORE_EXPORTABLE SendMagicPacket(const InetAddress& ipAddr, const MacAddress& macAddr, int count)
{
   if (!macAddr.isValid() || (macAddr.length() != 6) || (ipAddr.getFamily() != AF_INET))
      return false;

   // Create data area
   BYTE packetData[102];
   memset(packetData, 0xFF, 6);
   BYTE *curr = packetData + 6;
   for(int i = 0; i < 16; i++, curr += 6)
      memcpy(curr, macAddr.value(), 6);

   // Create socket
   SOCKET hSocket = CreateSocket(AF_INET, SOCK_DGRAM, 0);
   if (hSocket == INVALID_SOCKET)
   {
      nxlog_debug_tag(_T("wol"), 5, _T("SendMagicPacket(%s): cannot create socket (%s)"), ipAddr.toString().cstr(), _tcserror(errno));
      return FALSE;
   }
	SetSocketBroadcast(hSocket);

   struct sockaddr_in addr;
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(ipAddr.getAddressV4());
   addr.sin_port = htons(53);

   // Send requested number of packets
   bool success = true;
   for(int i = 0; i < count; i++)
      if (sendto(hSocket, (char *)packetData, 102, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
      {
         nxlog_debug_tag(_T("wol"), 5, _T("SendMagicPacket(%s): error sending message (%s)"), ipAddr.toString().cstr(), _tcserror(errno));
         success = false;
      }

   // Cleanup
   closesocket(hSocket);
   return success;
}

/**
 * Escape string
 */
void EscapeString(StringBuffer &str)
{
   str.escapeCharacter(_T('\\'), _T('\\'));
   str.escapeCharacter(_T('"'), _T('\\'));
   str.replace(_T("\b"), _T("\\b"));
   str.replace(_T("\r"), _T("\\r"));
   str.replace(_T("\n"), _T("\\n"));
   str.replace(_T("\t"), _T("\\t"));
}

/**
 * Prepare and execute SQL query with single binding - object ID (common implementation).
 */
template<typename _Ti, int _Tsql>
static inline bool ExecuteQueryOnObjectImpl(DB_HANDLE hdb, _Ti objectId, const wchar_t *query)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == nullptr)
      return false;
   DBBind(hStmt, 1, _Tsql, objectId);
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Prepare and execute SQL query with single binding - object ID.
 */
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, int32_t objectId, const wchar_t *query)
{
   return ExecuteQueryOnObjectImpl<int32_t, DB_SQLTYPE_INTEGER>(hdb, objectId, query);
}

/**
 * Prepare and execute SQL query with single binding - object ID.
 */
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, uint32_t objectId, const wchar_t *query)
{
   return ExecuteQueryOnObjectImpl<uint32_t, DB_SQLTYPE_INTEGER>(hdb, objectId, query);
}

/**
 * Prepare and execute SQL query with single binding - object ID.
 */
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, uint64_t objectId, const wchar_t *query)
{
   return ExecuteQueryOnObjectImpl<uint64_t, DB_SQLTYPE_BIGINT>(hdb, objectId, query);
}

/**
 * Prepare and execute SQL query with single binding - object ID.
 */
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, const wchar_t *objectId, const wchar_t *query)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == nullptr)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, objectId, DB_BIND_STATIC);
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Execute SQL SELECT-type query replacing macro {id} with object ID.
 */
DB_RESULT NXCORE_EXPORTABLE ExecuteSelectOnObject(DB_HANDLE hdb, uint32_t objectId, const wchar_t *query)
{
   StringBuffer fullQuery(query);
   TCHAR idText[16];
   fullQuery.replace(L"{id}", IntegerToString(objectId, idText));
   return DBSelect(hdb, fullQuery);
}

/**
 * Get address family hint from config
 */
static inline int GetAddressFamilyHint()
{
   switch(ConfigReadInt(L"Objects.Nodes.Resolver.AddressFamilyHint", 0))
   {
      case 1:
         return AF_INET;
      case 2:
         return AF_INET6;
      default:
         return AF_UNSPEC;
   }
}

/**
 * Resolve host name using zone if needed
 */
InetAddress NXCORE_EXPORTABLE ResolveHostName(int32_t zoneUIN, const wchar_t *hostname, int afHint)
{
   InetAddress ipAddr = InetAddress::parse(hostname);
   if (ipAddr.isValid())
      return ipAddr;

   int af = (afHint != AF_UNSPEC) ? afHint : GetAddressFamilyHint();
   if (IsZoningEnabled() && (zoneUIN != 0))
   {
      // resolve address through proxy agent
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zone != nullptr)
      {
         shared_ptr<NetObj> proxy = FindObjectById(zone->getProxyNodeId(nullptr), OBJECT_NODE);
         if (proxy != nullptr)
         {
            wchar_t query[256], buffer[128];
            _sntprintf(query, 256, L"Net.Resolver.AddressByName(%s,%d)", hostname, (af == AF_INET) ? 4 : ((af == AF_INET6) ? 6 : 0));
            if (static_cast<Node&>(*proxy).getMetricFromAgent(query, buffer, 128) == ERR_SUCCESS)
            {
               ipAddr = InetAddress::parse(buffer);
            }
            if (!ipAddr.isValid() && (af != AF_UNSPEC))
            {
               _sntprintf(query, 256, L"Net.Resolver.AddressByName(%s)", hostname);
               if (static_cast<Node&>(*proxy).getMetricFromAgent(query, buffer, 128) == ERR_SUCCESS)
               {
                  ipAddr = InetAddress::parse(buffer);
               }
            }
         }
      }

      // Resolve address through local resolver as fallback
      if (!ipAddr.isValid() && ConfigReadBoolean(L"Objects.Nodes.FallbackToLocalResolver", false))
      {
         ipAddr = InetAddress::resolveHostName(hostname, af);
         if (!ipAddr.isValid() && (af != AF_UNSPEC))
            ipAddr = InetAddress::resolveHostName(hostname, AF_UNSPEC);
      }
   }
   else
   {
      ipAddr = InetAddress::resolveHostName(hostname, af);
      if (!ipAddr.isValid() && (af != AF_UNSPEC))
         ipAddr = InetAddress::resolveHostName(hostname, AF_UNSPEC);
   }

   return ipAddr;
}

/**
 * Event name resolver
 */
bool EventNameResolver(const wchar_t *name, uint32_t *code)
{
   bool success = false;
   shared_ptr<EventTemplate> event = FindEventTemplateByName(name);
   if (event != nullptr)
   {
      *code = event->getCode();
      success = true;
   }
   return success;
}

/**
 * Create object URL from NXCP message
 */
ObjectUrl::ObjectUrl(const NXCPMessage& msg, uint32_t baseId)
{
   m_id = msg.getFieldAsUInt32(baseId);
   m_url = msg.getFieldAsString(baseId + 1);
   m_description = msg.getFieldAsString(baseId + 2);
}

/**
 * Create object URL from database result set
 */
ObjectUrl::ObjectUrl(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_url = DBGetField(hResult, row, 1, nullptr, 0);
   m_description = DBGetField(hResult, row, 2, nullptr, 0);
}

/**
 * Object URL destructor
 */
ObjectUrl::~ObjectUrl()
{
   MemFree(m_url);
   MemFree(m_description);
}

/**
 * Fill NXCP message
 */
void ObjectUrl::fillMessage(NXCPMessage *msg, uint32_t baseId)
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_url);
   msg->setField(baseId + 2, m_description);
}

/**
 * Serialize object to JSON
 */
json_t *ObjectUrl::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "url", json_string_t(m_url));
   json_object_set_new(root, "description", json_string_t(m_description));
   return root;
}

/**
 * Calculate nearest objects from current one
 * Object ref count will be automatically decreased on array delete
 */
ObjectArray<ObjectsDistance> *FindNearestObjects(uint32_t currObjectId, int maxDistance,
         bool (* filter)(NetObj *object, void *context), void *context, int (* calculateRealDistance)(GeoLocation &loc1, GeoLocation &loc2))
{
   shared_ptr<NetObj> currObj = FindObjectById(currObjectId);
   GeoLocation currLocation = currObj->getGeoLocation();
   if (currLocation.getType() == GL_UNSET)
      return nullptr;

   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(filter, context);
   auto result = new ObjectArray<ObjectsDistance>(16, 16, Ownership::True);
	for(int i = 0; i < objects->size(); i++)
	{
	   NetObj *object = objects->get(i);
      if (object->getId() == currObjectId)
         continue;

      GeoLocation location = object->getGeoLocation();
      if (currLocation.getType() == GL_UNSET)
         continue;

      // leave object only in given distance
      int distance = currLocation.calculateDistance(location);
      if (distance > maxDistance)
         continue;

      // Filter objects by real path calculation
      if (calculateRealDistance != nullptr)
      {
         distance = calculateRealDistance(location, currLocation);
         if (distance > maxDistance)
            continue;
      }

      result->add(new ObjectsDistance(objects->getShared(i), distance));
   }

   // Sort filtered objects
   result->sort([] (const ObjectsDistance **d1, const ObjectsDistance **d2) -> int { return (*d1)->distance - (*d2)->distance; });

   return result;
}

/**
 * Prepare MERGE statement if possible, otherwise INSERT or UPDATE depending on record existence
 * Identification column appended to provided column list
 */
static DB_STATEMENT DBPrepareMerge(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, void *id, int cType, int sqlType, int allocType, const TCHAR * const *columns)
{
   StringBuffer query;
   if (((g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB)) && (g_flags & AF_DB_SUPPORTS_MERGE))
   {
      query.append(_T("INSERT INTO "));
      query.append(table);
      query.append(_T(" ("));
      int count = 0;
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(columns[i]);
         query.append(_T(','));
         count++;
      }
      query.append(idColumn);
      query.append(_T(") VALUES (?"));
      for(int i = 0; i < count; i++)
         query.append(_T(",?"));
      query.append(_T(") ON CONFLICT ("));
      query.append(idColumn);
      query.append(_T(") DO UPDATE SET "));
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(columns[i]);
         query.append(_T("=excluded."));
         query.append(columns[i]);
         query.append(_T(','));
      }
      query.shrink();
   }
   else if (g_dbSyntax == DB_SYNTAX_ORACLE)
   {
      query.append(_T("MERGE INTO "));
      query.append(table);
      query.append(_T(" t USING (SELECT "));
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(_T("? AS "));
         query.append(columns[i]);
         query.append(_T(','));
      }
      query.append(_T("? AS "));
      query.append(idColumn);
      query.append(_T(" FROM dual) d ON (t."));
      query.append(idColumn);
      query.append(_T("=d."));
      query.append(idColumn);
      query.append(_T(") WHEN MATCHED THEN UPDATE SET "));
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(_T("t."));
         query.append(columns[i]);
         query.append(_T("=d."));
         query.append(columns[i]);
         query.append(_T(','));
      }
      query.shrink();
      query.append(_T(" WHEN NOT MATCHED THEN INSERT ("));
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(columns[i]);
         query.append(_T(','));
      }
      query.append(idColumn);
      query.append(_T(") VALUES ("));
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(_T("d."));
         query.append(columns[i]);
         query.append(_T(','));
      }
      query.append(_T("d."));
      query.append(idColumn);
      query.append(_T(')'));
   }
   else if (g_dbSyntax == DB_SYNTAX_MYSQL)
   {
      query.append(_T("INSERT INTO "));
      query.append(table);
      query.append(_T(" ("));
      int count = 0;
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(columns[i]);
         query.append(_T(','));
         count++;
      }
      query.append(idColumn);
      query.append(_T(") VALUES (?"));
      for(int i = 0; i < count; i++)
         query.append(_T(",?"));
      query.append(_T(") ON DUPLICATE KEY UPDATE "));
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(columns[i]);
         query.append(_T("=VALUES("));
         query.append(columns[i]);
         query.append(_T("),"));
      }
      query.append(idColumn);
      query.append(_T("=VALUES("));
      query.append(idColumn);
      query.append(_T(')'));
   }
   else if (IsDatabaseRecordExist(hdb, table, idColumn, id, cType, sqlType, allocType))
   {
      query.append(_T("UPDATE "));
      query.append(table);
      query.append(_T(" SET "));
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(columns[i]);
         query.append(_T("=?,"));
      }
      query.shrink();
      query.append(_T(" WHERE "));
      query.append(idColumn);
      query.append(_T("=?"));
   }
   else
   {
      query.append(_T("INSERT INTO "));
      query.append(table);
      query.append(_T(" ("));
      int count = 0;
      for(int i = 0; columns[i] != nullptr; i++)
      {
         query.append(columns[i]);
         query.append(_T(','));
         count++;
      }
      query.append(idColumn);
      query.append(_T(") VALUES (?"));
      for(int i = 0; i < count; i++)
         query.append(_T(",?"));
      query.append(_T(')'));
   }
   return DBPrepare(hdb, query);
}

/**
 * Prepare MERGE statement if possible, otherwise INSERT or UPDATE depending on record existence
 * Identification column appended to provided column list
 */
DB_STATEMENT NXCORE_EXPORTABLE DBPrepareMerge(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, uint32_t id, const TCHAR * const *columns)
{
   return DBPrepareMerge(hdb, table, idColumn, &id, DB_CTYPE_UINT32, DB_SQLTYPE_INTEGER, DB_BIND_TRANSIENT, columns);
}

/**
 * Prepare MERGE statement if possible, otherwise INSERT or UPDATE depending on record existence
 * Identification column appended to provided column list
 */
DB_STATEMENT NXCORE_EXPORTABLE DBPrepareMerge(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, const TCHAR *id, const TCHAR * const *columns)
{
   return DBPrepareMerge(hdb, table, idColumn, const_cast<TCHAR*>(id), DB_CTYPE_STRING, DB_SQLTYPE_VARCHAR, DB_BIND_STATIC, columns);
}

/**
 * Parser function for SQL command file and their execution on selected database
 */
bool ExecuteSQLCommandFile(const TCHAR *filePath, DB_HANDLE hdb)
{
   // Read file contents into a string
   char *source = LoadFileAsUTF8String(filePath);
   if (source == nullptr)
      return false;

   // Parse string
   char *ptr = source;
   char *query = ptr;

   while (true)
   {
      // Trim the query
      bool inString = false;

      while (true)
      {
         // Find query terminator that's not part of a string in the query
         if ((ptr == nullptr) || (*ptr == 0))
            break;

         ptr = strpbrk(ptr, ";'");
         if (ptr == nullptr)
            break;

         if (*ptr == ';')
         {
            if (!inString)
               break;
         }
         else
         {
            inString = !inString;
         }
         ptr++;
      }

      // Cut off query at ';'
      if (ptr != nullptr)
         *ptr = 0;
      TrimA(query);

      // Execute the query
      if (*query != 0)
      {
         wchar_t *wquery = WideStringFromUTF8String(query);
         DBQuery(hdb, wquery);
         MemFree(wquery);
      }

      // Get next query ready
      if ((ptr == nullptr) || (*(++ptr) == 0))
         break;

      query = ptr;
   }

   MemFree(source);
   return true;
}

/**
 * Report server configuration error
 */
void ReportConfigurationError(const wchar_t *subsystem, const wchar_t *tag, const wchar_t *descriptionFormat, ...)
{
   va_list args;
   va_start(args, descriptionFormat);
   wchar_t description[4096];
   _vsntprintf(description, 4096, descriptionFormat, args);
   va_end(args);

   nxlog_write_tag(NXLOG_WARNING, L"config", L"%s", description);

   EventBuilder(EVENT_CONFIGURATION_ERROR, g_dwMgmtNode)
         .param(L"subsystem", L"EPP")
         .param(L"tag", L"invalid-object-id")
         .param(L"description", description)
         .post();
}

/**
 * Build description for network path check result
 */
String NetworkPathCheckResult::buildDescription() const
{
   wchar_t description[1024];
   switch(reason)
   {
      case NetworkPathFailureReason::INTERFACE_DISABLED:
         _sntprintf(description, 1024, L"Interface %s on node %s is disabled",
                  GetObjectName(rootCauseInterfaceId, L"UNKNOWN"),
                  GetObjectName(rootCauseNodeId, L"UNKNOWN"));
         break;
      case NetworkPathFailureReason::PROXY_AGENT_UNREACHABLE:
         _sntprintf(description, 1024, _T("Agent on proxy node %s is unreachable"), GetObjectName(rootCauseNodeId, _T("UNKNOWN")));
         break;
      case NetworkPathFailureReason::PROXY_NODE_DOWN:
         _sntprintf(description, 1024, _T("Proxy node %s is down"), GetObjectName(rootCauseNodeId, _T("UNKNOWN")));
         break;
      case NetworkPathFailureReason::ROUTER_DOWN:
         _sntprintf(description, 1024, _T("Intermediate router %s is down"), GetObjectName(rootCauseNodeId, _T("UNKNOWN")));
         break;
      case NetworkPathFailureReason::ROUTING_LOOP:
         _sntprintf(description, 1024, _T("Routing loop detected on intermediate router %s"), GetObjectName(rootCauseNodeId, _T("UNKNOWN")));
         break;
      case NetworkPathFailureReason::SWITCH_DOWN:
         _sntprintf(description, 1024, _T("Intermediate switch %s is down"), GetObjectName(rootCauseNodeId, _T("UNKNOWN")));
         break;
      case NetworkPathFailureReason::VPN_TUNNEL_DOWN:
         _sntprintf(description, 1024, _T("VPN tunnel %s on node %s is down"),
                  GetObjectName(rootCauseInterfaceId, _T("UNKNOWN")),
                  GetObjectName(rootCauseNodeId, _T("UNKNOWN")));
         break;
      case NetworkPathFailureReason::WIRELESS_AP_DOWN:
         _sntprintf(description, 1024, _T("Wireless access point %s is down"), GetObjectName(rootCauseNodeId, _T("UNKNOWN")));
         break;
      default:
         return String(_T("Unknown reason"));
   }

   return String(description);
}
