/* 
** NetXMS - Network Management System
** CSCP API Library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nxcscpapi.h
**
**/

#ifndef _nxcscpapi_h_
#define _nxcscpapi_h_


#ifdef _WIN32
#ifdef LIBNXCSCP_EXPORTS
#define LIBNXCSCP_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXCSCP_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXCSCP_EXPORTABLE
#endif


//
// Temporary buffer structure for RecvCSCPMessage() function
//

typedef struct
{
   DWORD dwBufSize;
   DWORD dwBufPos;
   char szBuffer[CSCP_TEMP_BUF_SIZE];
} CSCP_BUFFER;


//
// Class for holding CSCP messages
//

class LIBNXCSCP_EXPORTABLE CSCPMessage
{
private:
   WORD m_wCode;
   DWORD m_dwId;
   DWORD m_dwNumVar;    // Number of variables
   CSCP_DF **m_ppVarList;   // List of variables

   void Set(DWORD dwVarId, BYTE bType, void *pValue, DWORD dwSize = 0);
   void *Get(DWORD dwVarId, BYTE bType);
   DWORD FindVariable(DWORD dwVarId);

public:
   CSCPMessage();
   CSCPMessage(CSCP_MESSAGE *pMsg);
   ~CSCPMessage();

   CSCP_MESSAGE *CreateMessage(void);

   WORD GetCode(void) { return m_wCode; }
   void SetCode(WORD wCode) { m_wCode = wCode; }

   DWORD GetId(void) { return m_dwId; }
   void SetId(DWORD dwId) { m_dwId = dwId; }

   BOOL IsVariableExist(DWORD dwVarId) { return (FindVariable(dwVarId) != INVALID_INDEX) ? TRUE : FALSE; }

   void SetVariable(DWORD dwVarId, WORD wValue) { Set(dwVarId, DT_INT16, &wValue); }
   void SetVariable(DWORD dwVarId, DWORD dwValue) { Set(dwVarId, DT_INTEGER, &dwValue); }
   void SetVariable(DWORD dwVarId, QWORD qwValue) { Set(dwVarId, DT_INT64, &qwValue); }
   void SetVariable(DWORD dwVarId, char *szValue) { Set(dwVarId, DT_STRING, szValue); }
   void SetVariable(DWORD dwVarId, BYTE *pValue, DWORD dwSize) { Set(dwVarId, DT_BINARY, pValue, dwSize); }

   DWORD GetVariableLong(DWORD dwVarId);
   QWORD GetVariableInt64(DWORD dwVarId);
   WORD GetVariableShort(DWORD dwVarId);
   char *GetVariableStr(DWORD dwVarId, char *szBuffer = NULL, DWORD dwBufSize = 0);
   DWORD GetVariableBinary(DWORD dwVarId, BYTE *pBuffer, DWORD dwBufSize);

   void DeleteAllVariables(void);
};


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

int LIBNXCSCP_EXPORTABLE RecvCSCPMessage(SOCKET hSocket, CSCP_MESSAGE *pMsg, 
                                         CSCP_BUFFER *pBuffer, DWORD dwMaxMsgSize);
CSCP_MESSAGE LIBNXCSCP_EXPORTABLE *CreateRawCSCPMessage(WORD wCode, DWORD dwId,
                                                        DWORD dwDataSize, void *pData,
                                                        CSCP_MESSAGE *pBuffer);
char LIBNXCSCP_EXPORTABLE *CSCPMessageCodeName(WORD wCode, char *pszBuffer);
   
#ifdef __cplusplus
}
#endif

#endif   /* _nxcscpapi_h_ */
