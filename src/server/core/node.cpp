/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: node.cpp
**
**/

#include "nms_core.h"


//
// Node class default constructor
//

Node::Node()
     :NetObj()
{
   m_dwFlags = 0;
   m_dwDiscoveryFlags = 0;
}


//
// Constructor for new node object
//

Node::Node(DWORD dwAddr, DWORD dwFlags, DWORD dwDiscoveryFlags)
     :NetObj()
{
   m_dwIpAddr = dwAddr;
   m_dwFlags = dwFlags;
   m_dwDiscoveryFlags = dwDiscoveryFlags;
}


//
// Node destructor
//

Node::~Node()
{
}


//
// Save object to database
//

BOOL Node::SaveToDB(void)
{
   char szQuery[1024];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;

   // Lock object's access
   Lock();

   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM nodes WHERE id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO nodes (id,name,status,is_deleted,primary_ip,is_snmp,is_agent,is_bridge,is_router,snmp_version,discovery_flags)"
                       " VALUES (%d,'%s',%d,%d,%d,%d,%d,%d,%d,%d,%d)",
              m_dwId, m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, 
              m_dwFlags & NF_IS_SNMP ? 1 : 0,
              m_dwFlags & NF_IS_NATIVE_AGENT ? 1 : 0,
              m_dwFlags & NF_IS_BRIDGE ? 1 : 0,
              m_dwFlags & NF_IS_ROUTER ? 1 : 0,
              0, m_dwDiscoveryFlags);
   else
      sprintf(szQuery, "UPDATE nodes SET name='%s',status=%d,is_deleted=%d,primary_ip=%d,"
                       "is_snmp=%d,is_agent=%d,is_bridge=%d,is_router=%d,snmp_version=%d,"
                       "discovery_flags=%d WHERE id=%d",
              m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, 
              m_dwFlags & NF_IS_SNMP ? 1 : 0,
              m_dwFlags & NF_IS_NATIVE_AGENT ? 1 : 0,
              m_dwFlags & NF_IS_BRIDGE ? 1 : 0,
              m_dwFlags & NF_IS_ROUTER ? 1 : 0,
              0, m_dwDiscoveryFlags, m_dwId);
   DBQuery(g_hCoreDB, szQuery);

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   Unlock();

   return TRUE;
}


//
// Delete object from database
//

BOOL Node::DeleteFromDB(void)
{
   char szQuery[256];

   sprintf(szQuery, "DELETE FROM nodes WHERE id=%ld", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
   return TRUE;
}
