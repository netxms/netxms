/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: vss_xp.cpp
**
**/

#define _WIN32_WINNT 0x0502
#include "libnxlp.h"
#include <win2003/vss.h>
#include <win2003/vswriter.h>
#include <win2003/vsbackup.h>
#include <comdef.h>

#define DEBUG_TAG    _T("logwatch.vss")

/**
 * Entry point
 */
extern HRESULT (STDAPICALLTYPE *__CreateVssBackupComponents)(IVssBackupComponents **);

/**
* Helper function to display error and return failure in FileSnapshot::create
*/
inline FileSnapshot *CreateFailure(HRESULT hr, IVssBackupComponents *bc, const TCHAR *format)
{
   _com_error err(hr);
   nxlog_debug_tag(DEBUG_TAG, 3, format, err.ErrorMessage(), hr);
   if (bc != NULL)
      bc->Release();
   return NULL;
}

/**
 * Create file snapshot using VSS (Server 2003 version)
 */
FileSnapshot *CreateSnapshotSrv2003(const TCHAR *path)
{
   IVssBackupComponents *bc;
   HRESULT hr = __CreateVssBackupComponents(&bc);
   if (FAILED(hr))
      return CreateFailure(hr, NULL, _T("Call to CreateVssBackupComponents failed (%s) HRESULT=0x%08X"));

   hr = bc->InitializeForBackup();
   if (FAILED(hr))
      return CreateFailure(hr, bc, _T("Call to IVssBackupComponents::InitializeForBackup failed (%s) HRESULT=0x%08X"));

   hr = bc->SetBackupState(false, false, VSS_BT_COPY);
   if (FAILED(hr))
      return CreateFailure(hr, bc, _T("Call to IVssBackupComponents::SetBackupState failed (%s) HRESULT=0x%08X"));

   hr = bc->SetContext(VSS_CTX_FILE_SHARE_BACKUP);
   if (FAILED(hr))
      return CreateFailure(hr, bc, _T("Call to IVssBackupComponents::SetContext failed (%s) HRESULT=0x%08X"));

   VSS_ID snapshotSetId;
   hr = bc->StartSnapshotSet(&snapshotSetId);
   if (FAILED(hr))
      return CreateFailure(hr, bc, _T("Call to IVssBackupComponents::StartSnapshotSet failed (%s) HRESULT=0x%08X"));

   const TCHAR *s = _tcschr(path, _T(':'));
   if (s == NULL)
      return CreateFailure(S_OK, bc, _T("Unsupported file path format"));
   s++;

   size_t len = s - path;
   TCHAR device[64];
   _tcslcpy(device, path, std::min(static_cast<size_t>(64), len + 1));
   _tcslcat(device, _T("\\"), 64);
   nxlog_debug_tag(DEBUG_TAG, 7, _T("LogWatch: Adding device %s to VSS snapshot"), device);

   VSS_ID snapshotId;
   hr = bc->AddToSnapshotSet(device, GUID_NULL, &snapshotId);
   if (FAILED(hr))
      return CreateFailure(hr, bc, _T("Call to IVssBackupComponents::AddToSnapshotSet failed (%s) HRESULT=0x%08X"));

   IVssAsync *async;
   hr = bc->DoSnapshotSet(&async);
   if (FAILED(hr))
      return CreateFailure(hr, bc, _T("Call to IVssBackupComponents::DoSnapshotSet failed (%s) HRESULT=0x%08X"));

   hr = async->Wait();
   async->Release();
   if (FAILED(hr))
      return CreateFailure(hr, bc, _T("Call to IVssAsync::Wait failed (%s) HRESULT=0x%08X"));

   VSS_SNAPSHOT_PROP prop;
   hr = bc->GetSnapshotProperties(snapshotId, &prop);
   if (FAILED(hr))
      return CreateFailure(hr, bc, _T("Call to IVssBackupComponents::GetSnapshotProperties failed (%s) HRESULT=0x%08X"));

   nxlog_debug_tag(DEBUG_TAG, 7, _T("LogWatch: Created VSS snapshot %s"), prop.m_pwszSnapshotDeviceObject);
   String sname(prop.m_pwszSnapshotDeviceObject);
   sname.append(s);

   FileSnapshot *object = (FileSnapshot *)malloc(sizeof(FileSnapshot));
   object->handle = bc;
   object->name = _tcsdup(sname);
   return object;
}

/**
 * Destroy backup components
 */
void DestroyBackupComponentsSrv2003(void *bc)
{
   static_cast<IVssBackupComponents*>(bc)->Release();
}
