/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: entirenet.cpp
**/

#include "nxcore.h"

/**
 * Network class default constructor
 */
Network::Network() : NetObj()
{
   m_dwId = BUILTIN_OID_NETWORK;
   _tcscpy(m_szName, _T("Entire Network"));
	uuid_generate(m_guid);
}

/**
 * Network class destructor
 */
Network::~Network()
{
}

/**
 * Save object to database
 */
BOOL Network::SaveToDB(DB_HANDLE hdb)
{
   LockData();

   saveCommonProperties(hdb);
   saveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_isModified = false;
   UnlockData();
   return TRUE;
}

/**
 * Load properties from database
 */
void Network::LoadFromDB()
{
   loadCommonProperties();
   loadACLFromDB();
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Network::showThresholdSummary()
{
	return true;
}
