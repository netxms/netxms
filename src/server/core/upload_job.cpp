/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: upload_job.cpp
**
**/

#include "nxcore.h"


//
// Static members
//

int FileUploadJob::m_activeJobs = 0;
int FileUploadJob::m_maxActiveJobs = 10;
MUTEX FileUploadJob::m_sharedDataMutex = INVALID_MUTEX_HANDLE;


//
// Static initializer
//

void FileUploadJob::init()
{
	m_sharedDataMutex = MutexCreate();
	m_maxActiveJobs = ConfigReadInt(_T("MaxActiveUploadJobs"), 10);
}


//
// Constructor
//

FileUploadJob::FileUploadJob(Node *node, const TCHAR *localFile, const TCHAR *remoteFile, DWORD userId, bool createOnHold)
              : ServerJob(_T("UPLOAD_FILE"), _T("Upload file to managed node"), node->Id(), userId, createOnHold)
{
	m_node = node;
	node->IncRefCount();

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Upload file %s"), GetCleanFileName(localFile));
	setDescription(buffer);

	m_localFile = _tcsdup(localFile);
	m_remoteFile = (remoteFile != NULL) ? _tcsdup(remoteFile) : NULL;

	_sntprintf(buffer, 1024, _T("Local file: %s; Remote file: %s"), m_localFile, CHECK_NULL(m_remoteFile));
	m_info = _tcsdup(buffer);
}


//
// Destructor
//

FileUploadJob::~FileUploadJob()
{
	m_node->DecRefCount();
	safe_free(m_localFile);
	safe_free(m_remoteFile);
	safe_free(m_info);
}


//
// Run job
//

bool FileUploadJob::run()
{
	bool success = false;
	
	while(true)
	{
		MutexLock(m_sharedDataMutex, INFINITE);
		if (m_activeJobs < m_maxActiveJobs)
		{
			m_activeJobs++;
			MutexUnlock(m_sharedDataMutex);
			break;
		}
		MutexUnlock(m_sharedDataMutex);
		ThreadSleep(5);
	}

	AgentConnectionEx *conn = m_node->createAgentConnection();
	if (conn != NULL)
	{
		m_fileSize = (INT64)FileSize(m_localFile);
		DWORD rcc = conn->uploadFile(m_localFile, m_remoteFile, uploadCallback, this);
		if (rcc == ERR_SUCCESS)
		{
			success = true;
		}
		else
		{
			setFailureMessage(AgentErrorCodeToText(rcc));
		}
	}
	else
	{
		setFailureMessage(_T("Agent connection not available"));
	}
	
	MutexLock(m_sharedDataMutex, INFINITE);
	m_activeJobs--;
	MutexUnlock(m_sharedDataMutex);

	return success;
}


//
// Upload progress callback
//

void FileUploadJob::uploadCallback(INT64 size, void *arg)
{
	if (((FileUploadJob *)arg)->m_fileSize > 0)
		((FileUploadJob *)arg)->markProgress((int)(size * _LL(100) / ((FileUploadJob *)arg)->m_fileSize));
	else
		((FileUploadJob *)arg)->markProgress(100);
}


//
// Get additional info for logging
//

const TCHAR *FileUploadJob::getAdditionalInfo()
{
	return m_info;
}
