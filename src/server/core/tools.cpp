/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
		nx_strncpy(pszBuffer, _tcserror(errno), nMaxSize);
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
   if (pIfList != NULL)
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
 * Execute external command
 */
BOOL ExecCommand(TCHAR *pszCommand)
{
   BOOL bSuccess = TRUE;

#ifdef _WIN32
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = 0;

   // Create new process
   if (!CreateProcess(NULL, pszCommand, NULL, NULL, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &si, &pi))
   {
      nxlog_write(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se", pszCommand, GetLastError());
      bSuccess = FALSE;
   }
   else
   {
      // Close all handles
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
   }
#else
	bSuccess = FALSE;
	{
		int nPid;
		char *pCmd[128];
		int nCount = 0;
		char *pTmp;
		struct stat st;
		int state = 0;

#ifdef UNICODE
		pTmp = MBStringFromWideString(pszCommand);
#else
		pTmp = strdup(pszCommand);
#endif
		if (pTmp != NULL)
		{
			pCmd[nCount++] = pTmp;
			int nLen = strlen(pTmp);
			for (int i = 0; (i < nLen) && (nCount < 127); i++)
			{
				switch(pTmp[i])
				{
					case ' ':
						if (state == 0)
						{
							pTmp[i] = 0;
							if (pTmp[i + 1] != 0)
							{
								pCmd[nCount++] = pTmp + i + 1;
							}
						}
						break;
					case '"':
						state == 0 ? state++ : state--;

						memmove(pTmp + i, pTmp + i + 1, nLen - i);
						i--;
						break;
					case '\\':
						if (pTmp[i+1] == '"')
						{
							memmove(pTmp + i, pTmp + i + 1, nLen - i);
						}
						break;
					default:
						break;
				}
			}
			pCmd[nCount] = NULL;

			if ((stat(pCmd[0], &st) == 0) && (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
			{
				switch ((nPid = fork()))
				{
					case -1:
                  nxlog_write(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se", pszCommand, errno);
						break;
					case 0: // child
						{
							int sd = open("/dev/null", O_RDWR);
							if (sd == -1)
							{
								sd = open("/dev/null", O_RDONLY);
							}
							close(0);
							close(1);
							close(2);
							dup2(sd, 0);
							dup2(sd, 1);
							dup2(sd, 2);
							close(sd);
							execv(pCmd[0], pCmd);
							// should not be reached
							//_exit((errno == EACCES || errno == ENOEXEC) ? 126 : 127);
							_exit(127);
						}
						break;
					default: // parent
						bSuccess = TRUE;
						break;
				}
			}

			free(pTmp);
		}
	}
#endif

   return bSuccess;
}

/**
 * Send Wake-on-LAN packet (magic packet) to given IP address
 * with given MAC address inside
 */
BOOL SendMagicPacket(UINT32 dwIpAddr, BYTE *pbMacAddr, int iNumPackets)
{
   BYTE *pCurr, bPacketData[102];
   SOCKET hSocket;
   struct sockaddr_in addr;
   BOOL bResult = TRUE;
   int i;

   // Create data area
   memset(bPacketData, 0xFF, 6);
   for(i = 0, pCurr = bPacketData + 6; i < 16; i++, pCurr += 6)
      memcpy(pCurr, pbMacAddr, 6);

   // Create socket
   hSocket = socket(AF_INET, SOCK_DGRAM, 0);
   if (hSocket == INVALID_SOCKET)
   {
      DbgPrintf(5, _T("SendMagicPacket: ERROR creating socket: %s."), _tcserror(errno));
      return FALSE;
   }
	SetSocketBroadcast(hSocket);

   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = dwIpAddr;
   addr.sin_port = htons(53);

   // Send requested number of packets
   for(i = 0; i < iNumPackets; i++)
      if (sendto(hSocket, (char *)bPacketData, 102, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
      {
         DbgPrintf(5, _T("SendMagicPacket: ERROR sending message: %s."), _tcserror(errno));
         bResult = FALSE;
      }

   // Cleanup
   closesocket(hSocket);
   return bResult;
}

/**
 * Decode SQL string and set as NXCP variable's value
 */
void DecodeSQLStringAndSetVariable(NXCPMessage *pMsg, UINT32 dwVarId, TCHAR *pszStr)
{
   DecodeSQLString(pszStr);
   pMsg->setField(dwVarId, pszStr);
}

/**
 * Escape string
 */
void EscapeString(String &str)
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
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, UINT32 objectId, const TCHAR *query)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == NULL)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
   bool success = DBExecute(hStmt) ? true : false;
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Resolve host name using zone if needed
 */
InetAddress NXCORE_EXPORTABLE ResolveHostName(UINT32 zoneUIN, const TCHAR *hostname)
{
   InetAddress ipAddr = InetAddress::parse(hostname);
   if (!ipAddr.isValid() && IsZoningEnabled() && (zoneUIN != 0))
   {
      // resolve address through proxy agent
      Zone *zone = FindZoneByUIN(zoneUIN);
      if (zone != NULL)
      {
         Node *proxy = static_cast<Node*>(FindObjectById(zone->getProxyNodeId(NULL), OBJECT_NODE));
         if (proxy != NULL)
         {
            TCHAR query[256], buffer[128];
            _sntprintf(query, 256, _T("Net.Resolver.AddressByName(%s)"), hostname);
            if (proxy->getItemFromAgent(query, 128, buffer) == ERR_SUCCESS)
            {
               ipAddr = InetAddress::parse(buffer);
            }
         }
      }
   }

   // Resolve address through local resolver
   if (!ipAddr.isValid())
   {
      ipAddr = InetAddress::resolveHostName(hostname);
   }
   return ipAddr;
}

/**
 * Create object URL from NXCP message
 */
ObjectUrl::ObjectUrl(NXCPMessage *msg, UINT32 baseId)
{
   m_id = msg->getFieldAsUInt32(baseId);
   m_url = msg->getFieldAsString(baseId + 1);
   m_description = msg->getFieldAsString(baseId + 2);
}

/**
 * Create object URL from database result set
 */
ObjectUrl::ObjectUrl(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_url = DBGetField(hResult, row, 1, NULL, 0);
   m_description = DBGetField(hResult, row, 2, NULL, 0);
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
void ObjectUrl::fillMessage(NXCPMessage *msg, UINT32 baseId)
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
   return (*obj1)->m_distance - (*obj2)->m_distance;
}

/**
 * Calculate nearest objects from current one
 * Object ref count will be automatically decreased on array delete
 */
ObjectArray<ObjectsDistance> *FindNearestObjects(UINT32 currObjectId, int maxDistance, bool (* filter)(NetObj *object, void *data), void *sortData, int (* calculateRealDistance)(GeoLocation &loc1, GeoLocation &loc2))
{
   NetObj *currObj = FindObjectById(currObjectId);
   currObj->incRefCount();
   GeoLocation currLocation = currObj->getGeoLocation();
   if (currLocation.getType() == GL_UNSET)
   {
      currObj->decRefCount();
      return NULL;
   }

   ObjectArray<NetObj> *objects = g_idxObjectById.getObjects(true, filter, sortData);
   ObjectArray<ObjectsDistance> *result = new ObjectArray<ObjectsDistance>(16, 16, true);
	for(int i = 0; i < objects->size(); i++)
	{
	   NetObj *object = objects->get(i);
      GeoLocation location = object->getGeoLocation();
      if (currLocation.getType() == GL_UNSET)
      {
         object->decRefCount();
         continue;
      }

      // leave object only in given distance
      int distance = currLocation.calculateDistance(location);
      if (distance > maxDistance)
      {
         object->decRefCount();
         continue;
      }

      // remove current object from list
      if (object->getId() == currObjectId)
      {
         object->decRefCount();
         continue;
      }

      // Filter objects by real path calculation
      if (calculateRealDistance != NULL)
      {
         distance = calculateRealDistance(location, currLocation);
         if(distance > maxDistance)
         {
            object->decRefCount();
            continue;
         }
      }

      result->add(new ObjectsDistance(object, distance));
   }

   // Sort filtered objects
   result->sort(DistanceSortCallback);

   currObj->decRefCount();
   return result;
}

/**
 * Prepare MERGE statement if possible, otherwise INSERT or UPDATE depending on record existence
 * Identification column appended to provided column list
 */
DB_STATEMENT NXCORE_EXPORTABLE DBPrepareMerge(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, UINT32 id, const TCHAR * const *columns)
{
   String query;
   if (((g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB)) && (g_flags & AF_DB_SUPPORTS_MERGE))
   {
      query.append(_T("INSERT INTO "));
      query.append(table);
      query.append(_T(" ("));
      int count = 0;
      for(int i = 0; columns[i] != NULL; i++)
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
      for(int i = 0; columns[i] != NULL; i++)
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
      for(int i = 0; columns[i] != NULL; i++)
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
      for(int i = 0; columns[i] != NULL; i++)
      {
         query.append(_T("t."));
         query.append(columns[i]);
         query.append(_T("=d."));
         query.append(columns[i]);
         query.append(_T(','));
      }
      query.shrink();
      query.append(_T(" WHEN NOT MATCHED THEN INSERT ("));
      for(int i = 0; columns[i] != NULL; i++)
      {
         query.append(columns[i]);
         query.append(_T(','));
      }
      query.append(idColumn);
      query.append(_T(") VALUES ("));
      for(int i = 0; columns[i] != NULL; i++)
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
      for(int i = 0; columns[i] != NULL; i++)
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
      for(int i = 0; columns[i] != NULL; i++)
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
   else if (IsDatabaseRecordExist(hdb, table, idColumn, id))
   {
      query.append(_T("UPDATE "));
      query.append(table);
      query.append(_T(" SET "));
      for(int i = 0; columns[i] != NULL; i++)
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
      for(int i = 0; columns[i] != NULL; i++)
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
