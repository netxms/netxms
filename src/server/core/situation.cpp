/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: situation.cpp
**
**/

#include "nxcore.h"


//
// Static data
//

static INDEX *m_pSituationIndex = NULL;
static DWORD m_dwSituationIndexSize = 0;
static RWLOCK m_rwlockSituationIndex;


//
// SituationInstance constructor
//

SituationInstance::SituationInstance(const TCHAR *name)
{
	m_name = _tcsdup(name);
}


//
// SituationInstance destructor
//

SituationInstance::~SituationInstance()
{
	safe_free(m_name);
}


//
// Update atribute
//

void SituationInstance::UpdateAttribute(const TCHAR *attribute, const TCHAR *value)
{
}


//
// Situation constructor
//

Situation::Situation(const TCHAR *name)
{
	m_id = CreateUniqueId(IDG_SITUATION);
	m_name = _tcsdup(name);
	m_comments = NULL;
	m_numInstances = 0;
	m_instanceList = NULL;
}


//
// Situation constructor for loading from database
// Expected field order: id,name,comments
//

Situation::Situation(DB_RESULT handle, int row)
{
	m_id = DBGetFieldULong(handle, row, 0);
	m_name = DBGetField(handle, row, 1, NULL, 0);
	m_comments = DBGetField(handle, row, 2, NULL, 0);
}


//
// Situation destructor
//

Situation::~Situation()
{
	int i;

	safe_free(m_name);
	safe_free(m_comments);
	for(i = 0; i < m_numInstances; i++)
		delete m_instanceList[i];
	safe_free(m_instanceList);
}


//
// Update situation
//

void Situation::UpdateSituation(const TCHAR *instance, const TCHAR *attribute, const TCHAR *value)
{
}


//
// Initialize situations
//

BOOL SituationsInit(void)
{
	DWORD i;
	DB_RESULT result;
	BOOL success = TRUE;
	
   m_rwlockSituationIndex = RWLockCreate();
   
   // Load situations from database
   result = DBSelect(g_hCoreDB, _T("SELECT id,name,comments FROM situations ORDER BY id"));
   if (result != NULL)
   {
   	m_dwSituationIndexSize = DBGetNumRows(result);
   	m_pSituationIndex = (INDEX *)realloc(m_pSituationIndex, sizeof(INDEX) * m_dwSituationIndexSize);
   	for(i = 0; i < m_dwSituationIndexSize; i++)
   	{
   		m_pSituationIndex[i].dwKey = DBGetFieldULong(result, i, 0);
   		m_pSituationIndex[i].pObject = (NetObj *)(new Situation(result, i));
		}
   	DBFreeResult(result);
	}
	else
	{
		DbgPrintf(3, _T("Cannot load situations from database due to DBSelect() failure"));
		success = FALSE;
	}
	return success;
}


//
// Find situation by ID
//

Situation *FindSituationById(DWORD dwId)
{
   DWORD dwPos;
   Situation *st;

   if (m_pSituationIndex == NULL)
      return NULL;

   RWLockReadLock(m_rwlockSituationIndex, INFINITE);
   dwPos = SearchIndex(m_pSituationIndex, m_dwSituationIndexSize, dwId);
   st = (dwPos == INVALID_INDEX) ? NULL : (Situation *)m_pSituationIndex[dwPos].pObject;
   RWLockUnlock(m_rwlockSituationIndex);
   return st;
}


//
// Create new situation
//

Situation *CreateSituation(const TCHAR *name)
{
	Situation *st;
	
	st = new Situation(name);
   RWLockWriteLock(m_rwlockSituationIndex, INFINITE);
   AddObjectToIndex(&m_pSituationIndex, &m_dwSituationIndexSize, st->GetId(), st);
   RWLockUnlock(m_rwlockSituationIndex);
}

