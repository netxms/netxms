/* 
** NetXMS - Network Management System
** Utility Library
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
** $module: cscp.cpp
**
**/

#include "libnetxms.h"


//
// Default constructor for CSCPMessage class
//

CSCPMessage::CSCPMessage()
{
   m_wCode = 0;
   m_dwId = 0;
   m_dwNumVar = 0;
   m_ppVarList = NULL;
}


//
// Create CSCPMessage object from received message
//

CSCPMessage::CSCPMessage(CSCP_MESSAGE *pMsg)
{
   WORD wPos, wSize;
   CSCP_DF *pVar;
   int iVarSize;

   m_dwNumVar = 0;
   m_ppVarList = NULL;

   m_wCode = ntohs(pMsg->wCode);
   m_dwId = ntohl(pMsg->dwId);
   wSize = ntohs(pMsg->wSize);
   
   // Parse data fields
   for(wPos = 8; wPos < wSize; wPos += iVarSize)
   {
      pVar = (CSCP_DF *)(((char *)pMsg) + wPos);

      // Calculate variable size
      switch(pVar->bType)
      {
         case DT_INTEGER:
            iVarSize = 12;
            break;
         case DT_INT64:
            iVarSize = 16;
            break;
         case DT_INT16:
            iVarSize = 8;
            break;
         case DT_STRING:
         case DT_BINARY:
            iVarSize = ntohs(pVar->data.string.wLen) + 8;
            break;
      }

      // Create new entry
      m_ppVarList = (CSCP_DF **)MemReAlloc(m_ppVarList, sizeof(CSCP_DF *) * (m_dwNumVar + 1));
      m_ppVarList[m_dwNumVar] = (CSCP_DF *)MemAlloc(iVarSize);
      memcpy(m_ppVarList[m_dwNumVar], pVar, iVarSize);

      // Convert numeric values to host format
      m_ppVarList[m_dwNumVar]->dwVarId = ntohl(m_ppVarList[m_dwNumVar]->dwVarId);
      switch(pVar->bType)
      {
         case DT_INTEGER:
            m_ppVarList[m_dwNumVar]->data.integer.dwInteger = ntohl(m_ppVarList[m_dwNumVar]->data.integer.dwInteger);
            break;
         case DT_INT64:
            m_ppVarList[m_dwNumVar]->data.int64.qwInt64 = ntohq(m_ppVarList[m_dwNumVar]->data.int64.qwInt64);
            break;
         case DT_INT16:
            m_ppVarList[m_dwNumVar]->data.wInt16 = ntohs(m_ppVarList[m_dwNumVar]->data.wInt16);
            break;
         case DT_STRING:
         case DT_BINARY:
            m_ppVarList[m_dwNumVar]->data.string.wLen = ntohs(m_ppVarList[m_dwNumVar]->data.string.wLen);
            break;
      }
      
      m_dwNumVar++;
   }
}


//
// Destructor for CSCPMessage
//

CSCPMessage::~CSCPMessage()
{
   DeleteAllVariables();
}


//
// Find variable by name
//

DWORD CSCPMessage::FindVariable(DWORD dwVarId)
{
   DWORD i;

   for(i = 0; i < m_dwNumVar; i++)
      if (m_ppVarList[i]->dwVarId == dwVarId)
         return i;
   return INVALID_INDEX;
}


//
// Set variable
// Argument dwSize (data size) is used only for DT_BINARY type
//

void CSCPMessage::Set(DWORD dwVarId, BYTE bType, void *pValue, DWORD dwSize)
{
   DWORD dwIndex;
   CSCP_DF *pVar;

   // Create CSCP_DF structure
   switch(bType)
   {
      case DT_INTEGER:
         pVar = (CSCP_DF *)MemAlloc(12);
         pVar->data.integer.dwInteger = *((DWORD *)pValue);
         break;
      case DT_INT16:
         pVar = (CSCP_DF *)MemAlloc(8);
         pVar->data.wInt16 = *((WORD *)pValue);
         break;
      case DT_INT64:
         pVar = (CSCP_DF *)MemAlloc(16);
         pVar->data.int64.qwInt64 = *((QWORD *)pValue);
         break;
      case DT_STRING:
         pVar = (CSCP_DF *)MemAlloc(8 + strlen((char *)pValue));
         pVar->data.string.wLen = (WORD)strlen((char *)pValue);
         memcpy(pVar->data.string.szValue, pValue, pVar->data.string.wLen);
         break;
      case DT_BINARY:
         pVar = (CSCP_DF *)MemAlloc(8 + dwSize);
         pVar->data.string.wLen = (WORD)dwSize;
         memcpy(pVar->data.string.szValue, pValue, pVar->data.string.wLen);
         break;
      default:
         return;  // Invalid data type, unable to handle
   }
   pVar->dwVarId = dwVarId;
   pVar->bType = bType;

   // Check if variable exists
   dwIndex = FindVariable(pVar->dwVarId);
   if (dwIndex == INVALID_INDEX) // Add new variable to list
   {
      m_ppVarList = (CSCP_DF **)MemReAlloc(m_ppVarList, sizeof(CSCP_DF *) * (m_dwNumVar + 1));
      m_ppVarList[m_dwNumVar] = pVar;
      m_dwNumVar++;
   }
   else  // Replace existing variable
   {
      MemFree(m_ppVarList[dwIndex]);
      m_ppVarList[dwIndex] = pVar;
   }
}


//
// Get variable value
//

void *CSCPMessage::Get(DWORD dwVarId, BYTE bType)
{
   DWORD dwIndex;
   void *pData;

   // Find variable
   dwIndex = FindVariable(dwVarId);
   if (dwIndex == INVALID_INDEX)
      return NULL;      // No such variable

   // Check data type
   if (m_ppVarList[dwIndex]->bType != bType)
      return NULL;

   if ((bType == DT_INTEGER) || (bType == DT_INT64))
      pData = (void *)((char *)(&m_ppVarList[dwIndex]->data) + 2);
   else
      pData = &m_ppVarList[dwIndex]->data;

   return pData;
}


//
// Get integer variable
//

DWORD CSCPMessage::GetVariableLong(DWORD dwVarId)
{
   char *pValue;

   pValue = (char *)Get(dwVarId, DT_INTEGER);
   return pValue ? *((DWORD *)pValue) : 0;
}


//
// Get 16-bit integer variable
//

WORD CSCPMessage::GetVariableShort(DWORD dwVarId)
{
   void *pValue;

   pValue = Get(dwVarId, DT_INT16);
   return pValue ? *((WORD *)pValue) : 0;
}


//
// Get 64-bit integer variable
//

QWORD CSCPMessage::GetVariableInt64(DWORD dwVarId)
{
   char *pValue;

   pValue = (char *)Get(dwVarId, DT_INT64);
   return pValue ? *((QWORD *)pValue) : 0;
}


//
// Get string variable
// If szBuffer is NULL, memory block of required size will be allocated
// for result; if szBuffer is not NULL, entire result or part of it will
// be placed to szBuffer and pointer to szBuffer will be returned.
//

char *CSCPMessage::GetVariableStr(DWORD dwVarId, char *szBuffer, DWORD dwBufSize)
{
   void *pValue;
   char *pStr = NULL;
   int iLen;

   if ((szBuffer != NULL) && (dwBufSize == 0))
      return NULL;   // non-sense combination

   pValue = Get(dwVarId, DT_STRING);
   if (pValue != NULL)
   {
      if (szBuffer == NULL)
         pStr = (char *)MemAlloc(*((WORD *)pValue) + 1);
      else
         pStr = szBuffer;
      iLen = (szBuffer == NULL) ? *((WORD *)pValue) : min(*((WORD *)pValue), dwBufSize - 1);
      memcpy(pStr, (char *)pValue + 2, iLen);
      pStr[iLen] = 0;
   }
   else
   {
      if (szBuffer != NULL)
      {
         pStr = szBuffer;
         pStr[0] = 0;
      }
   }
   return pStr;
}


//
// Get binary (byte array) variable
// Result will be placed to the buffer provided (no more than dwBufSize bytes,
// and actual size of data will be returned
// If pBuffer is NULL, just actual data length is returned
//

DWORD CSCPMessage::GetVariableBinary(DWORD dwVarId, BYTE *pBuffer, DWORD dwBufSize)
{
   void *pValue;
   DWORD dwSize;

   pValue = Get(dwVarId, DT_BINARY);
   if (pValue != NULL)
   {
      dwSize = *((WORD *)pValue);
      if (pBuffer != NULL)
         memcpy(pBuffer, (BYTE *)pValue + 2, min(dwBufSize, dwSize));
   }
   else
   {
      dwSize = 0;
   }
   return dwSize;
}


//
// Build protocol message ready to be send over the wire
//

CSCP_MESSAGE *CSCPMessage::CreateMessage(void)
{
   WORD wSize;
   int iVarSize;
   DWORD i;
   CSCP_MESSAGE *pMsg;
   CSCP_DF *pVar;

   // Calculate message size
   for(i = 0, wSize = 8; i < m_dwNumVar; i++)
      switch(m_ppVarList[i]->bType)
      {
         case DT_INTEGER:
            wSize += 12;
            break;
         case DT_INT64:
            wSize += 16;
            break;
         case DT_INT16:
            wSize += 8;
            break;
         case DT_STRING:
         case DT_BINARY:
            wSize += m_ppVarList[i]->data.string.wLen + 8;
            break;
      }

   // Create message
   pMsg = (CSCP_MESSAGE *)MemAlloc(wSize);
   pMsg->wCode = htons(m_wCode);
   pMsg->wSize = htons(wSize);
   pMsg->dwId = htonl(m_dwId);

   // Fill data fields
   for(i = 0, pVar = (CSCP_DF *)((char *)pMsg + 8); i < m_dwNumVar; i++)
   {
      // Calculate variable size
      switch(m_ppVarList[i]->bType)
      {
         case DT_INTEGER:
            iVarSize = 12;
            break;
         case DT_INT64:
            iVarSize = 16;
            break;
         case DT_INT16:
            iVarSize = 8;
            break;
         case DT_STRING:
         case DT_BINARY:
            iVarSize = m_ppVarList[i]->data.string.wLen + 8;
            break;
      }

      memcpy(pVar, m_ppVarList[i], iVarSize);

      // Convert numeric values to network format
      pVar->dwVarId = htonl(pVar->dwVarId);
      switch(pVar->bType)
      {
         case DT_INTEGER:
            pVar->data.integer.dwInteger = htonl(pVar->data.integer.dwInteger);
            break;
         case DT_INT64:
            pVar->data.int64.qwInt64 = htonq(pVar->data.int64.qwInt64);
            break;
         case DT_INT16:
            pVar->data.wInt16 = htons(pVar->data.wInt16);
            break;
         case DT_STRING:
         case DT_BINARY:
            pVar->data.string.wLen = htons(pVar->data.string.wLen);
            break;
      }

      pVar = (CSCP_DF *)((char *)pVar + iVarSize);
   }

   return pMsg;
}


//
// Delete all variables
//

void CSCPMessage::DeleteAllVariables(void)
{
   if (m_ppVarList != NULL)
   {
      DWORD i;

      for(i = 0; i < m_dwNumVar; i++)
         MemFree(m_ppVarList[i]);
      MemFree(m_ppVarList);

      m_ppVarList = NULL;
      m_dwNumVar = 0;
   }
}


//
// Receive raw CSCP message from network
// If pMsg is NULL, temporary buffer will be re-initialized
//

int LIBNETXMS_EXPORTABLE RecvCSCPMessage(SOCKET hSocket, CSCP_MESSAGE *pMsg, CSCP_BUFFER *pBuffer)
{
   DWORD dwMsgSize = 0, dwBytesRead = 0, dwBytesToCopy;
   int iErr;

   // Initialize buffer if requested
   if (pMsg == NULL)
   {
      pBuffer->dwBufSize = 0;
      pBuffer->dwBufPos = 0;
      return 0;
   }

   // Check if we have something in buffer
   if (pBuffer->dwBufSize > 0)
   {
      // Handle the case when entire message header have not been read into the buffer
      if (pBuffer->dwBufSize < 8)
      {
         // Most likely we are at the buffer end, so move content
         // to the beginning
         memmove(pBuffer->szBuffer, &pBuffer->szBuffer[pBuffer->dwBufPos], pBuffer->dwBufSize);
         pBuffer->dwBufPos = 0;

         // Receive new portion of data from the network 
         // and append it to existing data in buffer
         iErr = recv(hSocket, &pBuffer->szBuffer[pBuffer->dwBufSize], CSCP_TEMP_BUF_SIZE - pBuffer->dwBufSize, 0);
         if (iErr <= 0)
            return iErr;
         pBuffer->dwBufSize += (DWORD)iErr;
      }

      // Get message size from message header and copy available 
      // message bytes from buffer
      dwMsgSize = (DWORD)ntohs(((CSCP_MESSAGE *)(&pBuffer->szBuffer[pBuffer->dwBufPos]))->wSize);
      dwBytesRead = min(dwMsgSize, pBuffer->dwBufSize);
      memcpy(pMsg, &pBuffer->szBuffer[pBuffer->dwBufPos], dwBytesRead);
      pBuffer->dwBufSize -= dwBytesRead;
      pBuffer->dwBufPos = (pBuffer->dwBufSize > 0) ? (pBuffer->dwBufPos + dwBytesRead) : 0;
      if (dwBytesRead == dwMsgSize)
         return (int)dwBytesRead;   // We have read entire message
   }

   // Receive rest of message from the network
   do
   {
      iErr = recv(hSocket, pBuffer->szBuffer, CSCP_TEMP_BUF_SIZE, 0);
      if (iErr <= 0)
         return iErr;

      if (dwBytesRead == 0)   // New message?
         dwMsgSize = (DWORD)ntohs(((CSCP_MESSAGE *)(pBuffer->szBuffer))->wSize);
      dwBytesToCopy = min((DWORD)iErr, dwMsgSize - dwBytesRead);
      memcpy(((char *)pMsg) + dwBytesRead, pBuffer->szBuffer, dwBytesToCopy);
      dwBytesRead += dwBytesToCopy;
   }
   while(dwBytesRead < dwMsgSize);
   
   // Check if we have something left in buffer
   if (dwBytesToCopy < (DWORD)iErr)
   {
      pBuffer->dwBufPos = dwBytesToCopy;
      pBuffer->dwBufSize = (DWORD)iErr - dwBytesToCopy;
   }

   return (int)dwMsgSize;
}


//
// Create CSCP message with raw data (MF_BINARY flag)
// If pBuffer is NULL, new buffer is allocated with MemAlloc()
// Buffer should be of dwDataSize + 16 bytes.
//

CSCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawCSCPMessage(WORD wCode, DWORD dwId, DWORD dwDataSize, void *pData, CSCP_MESSAGE *pBuffer)
{
   CSCP_MESSAGE *pMsg;

   if (pBuffer == NULL)
      pMsg = (CSCP_MESSAGE *)MemAlloc(dwDataSize + 16);
   else
      pMsg = pBuffer;

   pMsg->wCode = htons(wCode | MF_BINARY);
   pMsg->dwId = htonl(dwId);
   pMsg->wSize = htons((WORD)dwDataSize + 8);
   memcpy(pMsg->df, pData, dwDataSize);

   return pMsg;
}
