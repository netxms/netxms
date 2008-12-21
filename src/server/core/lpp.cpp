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
** File: lpp.cpp
**
**/

#include "nxcore.h"


//
// Send LPP directory to client
//

void CreateLPPDirectoryMessage(CSCPMessage &msg)
{
   DB_RESULT hResult;
   DWORD i, dwNumRows, dwId;
   TCHAR szBuffer[MAX_DB_STRING];

   hResult = DBSelect(g_hCoreDB, _T("SELECT lpp_id,lpp_group_id,lpp_name,lpp_version,lpp_flags FROM lpp"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
      for(i = 0, dwId = VID_LPP_LIST_BASE; i < dwNumRows; i++, dwId += 5)
      {
         msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 0));
         msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 1));
         msg.SetVariable(dwId++, DBGetField(hResult, i, 2, szBuffer, MAX_DB_STRING));
         msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 3));
         msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 4));
      }
      DBFreeResult(hResult);

		hResult = DBSelect(g_hCoreDB, _T("SELECT lpp_group_id,parent_group,lpp_group_name FROM lpp_groups"));
		if (hResult != NULL)
		{
			dwNumRows = DBGetNumRows(hResult);
			msg.SetVariable(VID_NUM_GROUPS, dwNumRows);
			for(i = 0, dwId = VID_LPPGROUP_LIST_BASE; i < dwNumRows; i++, dwId += 7)
			{
				msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 0));
				msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 1));
				msg.SetVariable(dwId++, DBGetField(hResult, i, 2, szBuffer, MAX_DB_STRING));
			}
			DBFreeResult(hResult);
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }
}
