/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: netsrv.cpp
**
**/

#include "nxcore.h"


//
// Constructor
//

NetworkService::NetworkService()
               :NetObj()
{
}


//
// Destructor
//

NetworkService::~NetworkService()
{
}


//
// Save object to database
//

BOOL NetworkService::SaveToDB(void)
{
   char szQuery[1024];
   DWORD i;

   Lock();

   // Save access list
   SaveACLToDB();

   // Unlock object and clear modification flag
   Unlock();
   m_bIsModified = FALSE;
   return TRUE;
}


//
// Load properties from database
//

BOOL NetworkService::CreateFromDB(DWORD dwId)
{
   m_dwId = dwId;

   LoadACLFromDB();
   return TRUE;
}


//
// Delete object from database
//

BOOL NetworkService::DeleteFromDB(void)
{
   return TRUE;
}


//
// Create CSCP message with object's data
//

void NetworkService::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_SERVICE_TYPE, (WORD)m_iServiceType);
}


//
// Modify object from message
//

DWORD NetworkService::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      Lock();

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}
