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
// Constructor
//

FileUploadJob::FileUploadJob(Node *node, const TCHAR *localFile, const TCHAR *remoteFile, DWORD userId)
              : ServerJob(_T("UPLOAD_FILE"), _T("Upload file to managed node"), node->Id(), userId)
{
	m_node = node;
	node->IncRefCount();

	TCHAR buffer[1024];
	_sntprintf(buffer, 1024, _T("Upload file %s"), localFile);
	setDescription(buffer);
}


//
// Destructor
//

FileUploadJob::~FileUploadJob()
{
	m_node->DecRefCount();
}


//
// Run job
//

bool FileUploadJob::run()
{
	bool success = false;

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
