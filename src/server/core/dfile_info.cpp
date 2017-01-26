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
DownloadFileInfo::DownloadFileInfo(const TCHAR *name, UINT32 uploadCommand, time_t lastModTime)
{
   m_fileName = _tcsdup(name);
   m_uploadCommand = uploadCommand;
   m_uploadData = 0;
   m_lastModTime = lastModTime;
   m_file = -1;
}

/**
 * Destructor
 */
DownloadFileInfo::~DownloadFileInfo()
{
   if (m_file != -1)
      close(false);
   delete m_fileName;
}

/**
 * Opens file and returns if it was successfully
 */
bool DownloadFileInfo::open()
{
   m_file = _topen(m_fileName, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
   return m_file != -1;
}

/**
 * Set upload data for package
 */
void DownloadFileInfo::setUploadData(UINT32 data)
{
   m_uploadData = data;
}
/**
 * Set downloadable image guid
 */
void DownloadFileInfo::setGUID(uuid_t guid)
{
   memcpy(m_uploadImageGuid, guid, UUID_LENGTH);
}

/**
 * Update database information about agent package
 */
void DownloadFileInfo::updateAgentPkgDBInfo(const TCHAR *description, const TCHAR *pkgName, const TCHAR *pkgVersion, const TCHAR *platform, const TCHAR *cleanFileName)
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
 * Function that writes incoming data to file
 */
bool DownloadFileInfo::write(const BYTE *data, int dataSize)
{
   return _write(m_file, data, dataSize) == dataSize;
}


/**
 * Callback for sending image library update notifications
 */
static void ImageLibraryUpdateCallback(ClientSession *pSession, void *pArg)
{
	pSession->onLibraryImageChange((uuid_t *)pArg, false);
}

/**
 * Closes file and changes it's date if required
 */
void DownloadFileInfo::close(bool success)
{
   _close(m_file);
   m_file = -1;

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
         EnumerateClientSessions(ImageLibraryUpdateCallback, (void *)&m_uploadImageGuid);
         break;
      case CMD_UPLOAD_FILE:
         if(m_lastModTime != 0)
            SetLastModificationTime(m_fileName, m_lastModTime);
         break;
      default:
         break;
   }

   // Remove received file in case of failure
   if (!success)
      _tunlink(m_fileName);
}
