/*
** NetXMS - Network Management System
** Copyright (C) 2017-2020 Raden Solutions
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
** File: dfile_info.cpp
**
**/

#include "nxcore.h"

/**
 * Constructor for DownloadFileInfo class only stores given data
 */
ServerDownloadFileInfo::ServerDownloadFileInfo(const TCHAR *name, UINT32 uploadCommand, time_t lastModTime) : DownloadFileInfo(name, lastModTime)
{
   m_uploadCommand = uploadCommand;
   m_uploadData = 0;
}

/**
 * Destructor
 */
ServerDownloadFileInfo::~ServerDownloadFileInfo()
{
}

/**
 * Update database information about agent package
 */
void ServerDownloadFileInfo::updateAgentPkgDBInfo(const TCHAR *description, const TCHAR *pkgName, const TCHAR *pkgVersion, const TCHAR *platform, const TCHAR *cleanFileName)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO agent_pkg (pkg_id,pkg_name,version,description,platform,pkg_file) VALUES (?,?,?,?,?,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_uploadData);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, pkgName, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, pkgVersion, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, description, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, platform, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, cleanFileName, DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Callback for sending image library update notifications
 */
static void ImageLibraryUpdateCallback(ClientSession *pSession, void *pArg)
{
	pSession->onLibraryImageChange(*((const uuid *)pArg), false);
}

/**
 * Closes file and changes it's date if required
 */
void ServerDownloadFileInfo::close(bool success)
{
   DownloadFileInfo::close(success);

   switch(m_uploadCommand)
   {
      case CMD_INSTALL_PACKAGE:
         if (!success)
         {
            DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
            TCHAR szQuery[256];
            _sntprintf(szQuery, 256, _T("DELETE FROM agent_pkg WHERE pkg_id=%d"), m_uploadData);
            DBQuery(hdb, szQuery);
            DBConnectionPoolReleaseConnection(hdb);
         }
         break;
      case CMD_MODIFY_IMAGE:
         if (success)
            EnumerateClientSessions(ImageLibraryUpdateCallback, (void *)&m_uploadImageGuid);
         break;
      default:
         break;
   }
}
