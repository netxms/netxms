/*
** NetXMS - Network Management System
** Copyright (C) 2017 Raden Solutions
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
   TCHAR *escDescr = EncodeSQLString(description);
   TCHAR szQuery[2048];
   _sntprintf(szQuery, 2048, _T("INSERT INTO agent_pkg (pkg_id,pkg_name,")
                                _T("version,description,platform,pkg_file) ")
                                _T("VALUES (%d,'%s','%s','%s','%s','%s')"),
              m_uploadData, pkgName, pkgVersion, escDescr,
              platform, cleanFileName);
   free(escDescr);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DBQuery(hdb, szQuery);
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
