/* 
** NetXMS - Network Management System
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
** File: entirenet.cpp
**/

#include "nxcore.h"

/**
 * Network class default constructor
 */
Network::Network() : super()
{
   m_id = BUILTIN_OID_NETWORK;
   _tcscpy(m_name, _T("Entire Network"));
   m_guid = uuid::generate();
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
bool Network::saveToDatabase(DB_HANDLE hdb)
{
   lockProperties();

   bool success = saveCommonProperties(hdb);
   if (success)
      success = saveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_modified = 0;
   unlockProperties();
   return success;
}

/**
 * Load properties from database
 */
void Network::loadFromDatabase(DB_HANDLE hdb)
{
   loadCommonProperties(hdb);
   loadACLFromDB(hdb);
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Network::showThresholdSummary()
{
	return true;
}
