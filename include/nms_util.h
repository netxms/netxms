/* 
** NetXMS - Network Management System
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
** $module: nms_util.h
**
**/

#ifndef _nms_util_h_
#define _nms_util_h_

#include <nms_common.h>
#include <nms_cscp.h>

#define INVALID_INDEX      0xFFFFFFFF


//
// Class for holding CSCP messages
//

class EXPORTABLE CSCPMessage
{
private:
   WORD m_wCode;
   DWORD m_dwId;
   DWORD m_dwNumVar;    // Number of variables
   CSCP_DF **m_ppVarList;   // List of variables

   void Set(char *szName, BYTE bType, void *pValue);
   void *Get(char *szName, BYTE bType);
   DWORD FindVariable(char *szName);

public:
   CSCPMessage();
   CSCPMessage(CSCP_MESSAGE *pMsg);
   ~CSCPMessage();

   CSCP_MESSAGE *CreateMessage(void);

   WORD GetCode(void) { return m_wCode; }
   void SetCode(WORD wCode) { m_wCode = wCode; }

   DWORD GetId(void) { return m_dwId; }
   void SetId(DWORD dwId) { m_dwId = dwId; }

   BOOL IsVariableExist(char *szName) { return (FindVariable(szName) != INVALID_INDEX) ? TRUE : FALSE; }

   void SetVariable(char *szName, DWORD dwValue) { Set(szName, DT_INTEGER, &dwValue); }
   void SetVariable(char *szName, QWORD qwValue) { Set(szName, DT_INT64, &qwValue); }
   void SetVariable(char *szName, char *szValue) { Set(szName, DT_STRING, szValue); }

   DWORD GetVariableLong(char *szName);
   QWORD GetVariableInt64(char *szName);
   char *GetVariableStr(char *szName);
};


//
// Functions
//

#define htonq(x) ntohq(x)

extern "C"
{
   void EXPORTABLE LibUtilDestroyObject(void *pObject);
   QWORD ntohq(QWORD qwVal);
}

#endif   /* _nms_util_h_ */
