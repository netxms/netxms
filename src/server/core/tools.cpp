/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
# include <io.h>
#else
# ifdef HAVE_SYS_UTSNAME_H
#  include <sys/utsname.h>
# endif
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
InetAddress GetLocalIpAddr()
{
   InetAddress addr;
   InterfaceList *pIfList = GetLocalInterfaceList();
   if (pIfList != nullptr)
   {
      // Find first interface with IP address
      for(int i = 0; i < pIfList->size(); i++)
      {
         InterfaceInfo *iface = pIfList->get(i);
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
      delete pIfList;
   }
   return addr;
}

/**
 *  Splits command line
 */
StringList *SplitCommandLine(const TCHAR *command)
{
   StringList *listOfStrings = new StringList();
   StringBuffer tmp;
   int state = 0;
   int size = (int)_tcslen(command);
   for(int i = 0; i < size; i++)
   {
      TCHAR c = command[i];
      switch(state)
      {
         case 0:
            if (c == _T(' '))
            {
               listOfStrings->add(tmp);
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
      listOfStrings->add(tmp);

   return listOfStrings;
}

/**
 * Send Wake-on-LAN packet (magic packet) to given IP address
 * with given MAC address inside
 */
bool SendMagicPacket(const InetAddress& ipAddr, const MacAddress& macAddr, int count)
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
      DbgPrintf(5, _T("SendMagicPacket: ERROR creating socket: %s."), _tcserror(errno));
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
         nxlog_debug(5, _T("SendMagicPacket: ERROR sending message: %s."), _tcserror(errno));
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
 * Prepare and execute SQL query with single binding - object ID.
 */
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, uint32_t objectId, const TCHAR *query)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == nullptr)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
   bool success = DBExecute(hStmt) ? true : false;
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Prepare and execute SQL query with single binding - object ID.
 */
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, const TCHAR *objectId, const TCHAR *query)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == nullptr)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, objectId, DB_BIND_STATIC);
   bool success = DBExecute(hStmt) ? true : false;
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Resolve host name using zone if needed
 */
InetAddress NXCORE_EXPORTABLE ResolveHostName(int32_t zoneUIN, const TCHAR *hostname)
{
   InetAddress ipAddr = InetAddress::parse(hostname);
   if (ipAddr.isValid())
      return ipAddr;

   if (IsZoningEnabled() && (zoneUIN != 0))
   {
      // resolve address through proxy agent
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zone != nullptr)
      {
         shared_ptr<NetObj> proxy = FindObjectById(zone->getProxyNodeId(nullptr), OBJECT_NODE);
         if (proxy != nullptr)
         {
            TCHAR query[256], buffer[128];
            _sntprintf(query, 256, _T("Net.Resolver.AddressByName(%s)"), hostname);
            if (static_cast<Node&>(*proxy).getMetricFromAgent(query, buffer, 128) == ERR_SUCCESS)
            {
               ipAddr = InetAddress::parse(buffer);
            }
         }
      }

      // Resolve address through local resolver as fallback
      if (!ipAddr.isValid() && ConfigReadBoolean(_T("Objects.Nodes.FallbackToLocalResolver"), false))
      {
         ipAddr = InetAddress::resolveHostName(hostname);
      }
   }
   else
   {
      ipAddr = InetAddress::resolveHostName(hostname);
   }
   return ipAddr;
}

/**
 * Event name resolver
 */
bool EventNameResolver(const TCHAR *name, UINT32 *code)
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
   free(m_url);
   free(m_description);
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
 * Distance array sorting callback
 */
int DistanceSortCallback(const ObjectsDistance **obj1, const ObjectsDistance **obj2)
{
   return (*obj1)->distance - (*obj2)->distance;
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
   result->sort(DistanceSortCallback);

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
#ifdef UNICODE
         TCHAR *wquery = WideStringFromUTF8String(query);
         DBQuery(hdb, wquery);
         MemFree(wquery);
#else
         DBQuery(hdb, query);
#endif
      }

      // Get next query ready
      if ((ptr == nullptr) || (*(++ptr) == 0))
         break;

      query = ptr;
   }

   MemFree(source);
   return true;
}
