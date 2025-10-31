/*
** NetXMS - Network Management System
** Copyright (C) 2017-2025 Raden Solutions
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
 * Destructor
 */
ServerDownloadFileInfo::~ServerDownloadFileInfo()
{
}

/**
 * Update database information about agent package
 */
void ServerDownloadFileInfo::updatePackageDBInfo(const wchar_t *description, const wchar_t *pkgName, const wchar_t *pkgVersion, const wchar_t *pkgType,
   const wchar_t *platform, const wchar_t *cleanFileName, const wchar_t *command)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO agent_pkg (pkg_id,pkg_name,version,description,platform,pkg_file,pkg_type,command) VALUES (?,?,?,?,?,?,?,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_uploadData);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, pkgName, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, pkgVersion, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, description, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, platform, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, cleanFileName, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, pkgType, DB_BIND_STATIC);
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, command, DB_BIND_STATIC, 4000);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Closes file and changes it's date if required
 */
void ServerDownloadFileInfo::close(bool success, bool deleteOnFailure)
{
   DownloadFileInfo::close(success, deleteOnFailure);
   if (m_completionCallback != nullptr)
      m_completionCallback(m_fileName, m_uploadData, success);
}
