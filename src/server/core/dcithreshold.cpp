/* 
** NetXMS - Network Management System
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
** $module: dcithreshold.cpp
**
**/

#include "nms_core.h"


//
// Default constructor
//

Threshold::Threshold(DCItem *pRelatedItem)
{
   m_dwId = 0;
   m_dwItemId = pRelatedItem->Id();
   m_dwEventCode = EVENT_THRESHOLD_REACHED;
   m_pszValueStr = NULL;
   m_iFunction = F_LAST;
   m_iOperation = OP_EQ;
   m_iDataType = pRelatedItem->DataType();
   m_iParam1 = 0;
   m_iParam2 = 0;
   m_bIsReached = FALSE;
}


//
// Constructor for creating object from database
// This constructor assumes that SELECT query look as following:
// SELECT threshold_id,fire_value,rearm_value,check_function,check_operation,
//        parameter_1,parameter_2,event_code FROM thresholds
//

Threshold::Threshold(DB_RESULT hResult, int iRow, DCItem *pRelatedItem)
{
   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   m_dwItemId = pRelatedItem->Id();
   m_dwEventCode = DBGetFieldULong(hResult, iRow, 7);
   m_pszValueStr = strdup(DBGetField(hResult, iRow, 1));
   m_iFunction = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iOperation = (BYTE)DBGetFieldLong(hResult, iRow, 4);
   m_iDataType = pRelatedItem->DataType();
   m_iParam1 = DBGetFieldLong(hResult, iRow, 5);
   m_iParam2 = DBGetFieldLong(hResult, iRow, 6);
   m_bIsReached = FALSE;

   switch(m_iDataType)
   {
      case DCI_DT_INT:
         m_value.iInt = strtol(m_pszValueStr, NULL, 0);
         break;
      case DCI_DT_UINT:
         m_value.dwUInt = strtoul(m_pszValueStr, NULL, 0);
         break;
      case DCI_DT_INT64:
         /* TODO: add 64-bit string to binary conversion */
         break;
      case DCI_DT_UINT64:
         /* TODO: add 64-bit string to binary conversion */
         break;
      case DCI_DT_FLOAT:
         m_value.dFloat = strtod(m_pszValueStr, NULL);
         break;
      default:
         break;
   }
}


//
// Destructor
//

Threshold::~Threshold()
{
   safe_free(m_pszValueStr);
}


//
// Create new unique id for object
//

void Threshold::CreateId(void)
{
   m_dwId = CreateUniqueId(IDG_THRESHOLD); 
}


//
// Save threshold to database
//

BOOL Threshold::SaveToDB(DWORD dwIndex)
{
   char szQuery[512];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;

   // Check for object's existence in database
   sprintf(szQuery, "SELECT threshold_id FROM thresholds WHERE threshold_id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Prepare and execute query
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO thresholds (threshold_id,item_id,fire_value,rearm_value,"
                       "check_function,check_operation,parameter_1,parameter_2,event_code,"
                       "sequence_number) VALUES (%ld,%ld,'%s','',%d,%d,%ld,%ld,%ld,%ld)", 
              m_dwId, m_dwItemId, m_pszValueStr, m_iFunction, m_iOperation, m_iParam1,
              m_iParam2, m_dwEventCode, dwIndex);
   else
      sprintf(szQuery, "UPDATE thresholds SET item_id=%ld,fire_value='%s',check_function=%d,"
                       "check_operation=%d,parameter_1=%ld,parameter_2=%ld,event_code=%ld,"
                       "sequence_number=%ld WHERE threshold_id=%ld", m_dwItemId, 
              m_pszValueStr, m_iFunction, m_iOperation, m_iParam1, m_iParam2, m_dwEventCode, 
              dwIndex, m_dwId);
   return DBQuery(g_hCoreDB, szQuery);
}


//
// Check threshold
// Function will return the following codes:
//    THRESHOLD_REACHED - when item's value match the threshold condition while previous check doesn't
//    THRESHOLD_REARMED - when item's value doesn't match the threshold condition while previous check do
//    NO_ACTION - when there are no changes in item's value match to threshold's condition
//

int Threshold::Check(ItemValue &value)
{
   BOOL bMatch = FALSE;
   int iResult;
   union
   {
      const char *pszStr;
      long iInt;
      DWORD dwUInt;
      INT64 iInt64;
      QWORD qwUInt64;
      double dFloat;
   } fvalue;

   // Execute function on value
   switch(m_iFunction)
   {
      case F_LAST:         // Check last value only
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               fvalue.iInt = (long)value;
               break;
            case DCI_DT_UINT:
               fvalue.dwUInt = (DWORD)value;
               break;
            case DCI_DT_INT64:
               fvalue.iInt64 = (INT64)value;
               break;
            case DCI_DT_UINT64:
               fvalue.qwUInt64 = (QWORD)value;
               break;
            case DCI_DT_FLOAT:
               fvalue.dFloat = (double)value;
               break;
            case DCI_DT_STRING:
               fvalue.pszStr = (const char *)value;
               break;
            default:
               WriteLog(MSG_INVALID_DTYPE, EVENTLOG_ERROR_TYPE, "ds", m_iDataType, "Threshold::Check()");
               break;
         }
         break;
      case F_AVERAGE:      // Check average value for last n polls
         /* TODO */
         break;
      case F_DEVIATION:    // Check deviation from the mean value
         /* TODO */
         break;
      default:
         break;
   }

   // Run comparision operation on function result and threshold value
   switch(m_iOperation)
   {
      case OP_LE:    // Less
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               bMatch = (fvalue.iInt < m_value.iInt);
               break;
            case DCI_DT_UINT:
               bMatch = (fvalue.dwUInt < m_value.dwUInt);
               break;
            case DCI_DT_INT64:
               bMatch = (fvalue.iInt64 < m_value.iInt64);
               break;
            case DCI_DT_UINT64:
               bMatch = (fvalue.qwUInt64 < m_value.qwUInt64);
               break;
            case DCI_DT_FLOAT:
               bMatch = (fvalue.dFloat < m_value.dFloat);
               break;
         }
         break;
      case OP_LE_EQ: // Less or equal
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               bMatch = (fvalue.iInt <= m_value.iInt);
               break;
            case DCI_DT_UINT:
               bMatch = (fvalue.dwUInt <= m_value.dwUInt);
               break;
            case DCI_DT_INT64:
               bMatch = (fvalue.iInt64 <= m_value.iInt64);
               break;
            case DCI_DT_UINT64:
               bMatch = (fvalue.qwUInt64 <= m_value.qwUInt64);
               break;
            case DCI_DT_FLOAT:
               bMatch = (fvalue.dFloat <= m_value.dFloat);
               break;
         }
         break;
      case OP_EQ:    // Equal
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               bMatch = (fvalue.iInt == m_value.iInt);
               break;
            case DCI_DT_UINT:
               bMatch = (fvalue.dwUInt == m_value.dwUInt);
               break;
            case DCI_DT_INT64:
               bMatch = (fvalue.iInt64 == m_value.iInt64);
               break;
            case DCI_DT_UINT64:
               bMatch = (fvalue.qwUInt64 == m_value.qwUInt64);
               break;
            case DCI_DT_FLOAT:
               bMatch = (fvalue.dFloat == m_value.dFloat);
               break;
            case DCI_DT_STRING:
               bMatch = !strcmp(fvalue.pszStr, m_pszValueStr);
               break;
         }
         break;
      case OP_GT_EQ: // Greater or equal
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               bMatch = (fvalue.iInt >= m_value.iInt);
               break;
            case DCI_DT_UINT:
               bMatch = (fvalue.dwUInt >= m_value.dwUInt);
               break;
            case DCI_DT_INT64:
               bMatch = (fvalue.iInt64 >= m_value.iInt64);
               break;
            case DCI_DT_UINT64:
               bMatch = (fvalue.qwUInt64 >= m_value.qwUInt64);
               break;
            case DCI_DT_FLOAT:
               bMatch = (fvalue.dFloat >= m_value.dFloat);
               break;
         }
         break;
      case OP_GT:    // Greater
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               bMatch = (fvalue.iInt > m_value.iInt);
               break;
            case DCI_DT_UINT:
               bMatch = (fvalue.dwUInt > m_value.dwUInt);
               break;
            case DCI_DT_INT64:
               bMatch = (fvalue.iInt64 > m_value.iInt64);
               break;
            case DCI_DT_UINT64:
               bMatch = (fvalue.qwUInt64 > m_value.qwUInt64);
               break;
            case DCI_DT_FLOAT:
               bMatch = (fvalue.dFloat > m_value.dFloat);
               break;
         }
         break;
      case OP_NE:    // Not equal
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               bMatch = (fvalue.iInt != m_value.iInt);
               break;
            case DCI_DT_UINT:
               bMatch = (fvalue.dwUInt != m_value.dwUInt);
               break;
            case DCI_DT_INT64:
               bMatch = (fvalue.iInt64 != m_value.iInt64);
               break;
            case DCI_DT_UINT64:
               bMatch = (fvalue.qwUInt64 != m_value.qwUInt64);
               break;
            case DCI_DT_FLOAT:
               bMatch = (fvalue.dFloat != m_value.dFloat);
               break;
            case DCI_DT_STRING:
               bMatch = strcmp(fvalue.pszStr, m_pszValueStr);
               break;
         }
         break;
      case OP_LIKE:
         // This operation can be performed only on strings
         if (m_iDataType == DCI_DT_STRING)
            bMatch = MatchString(m_pszValueStr, fvalue.pszStr, TRUE);
         break;
      case OP_NOTLIKE:
         // This operation can be performed only on strings
         if (m_iDataType == DCI_DT_STRING)
            bMatch = !MatchString(m_pszValueStr, fvalue.pszStr, TRUE);
         break;
      default:
         break;
   }

   iResult = (bMatch & !m_bIsReached) ? THRESHOLD_REACHED :
                ((!bMatch & m_bIsReached) ? THRESHOLD_REARMED : NO_ACTION);
   m_bIsReached = bMatch;
   return iResult;
}


//
// Fill DCI_THRESHOLD with object's data ready to send over the network
//

void Threshold::CreateMessage(DCI_THRESHOLD *pData)
{
   pData->dwId = htonl(m_dwId);
   pData->dwEvent = htonl(m_dwEventCode);
   pData->wFunction = htons((WORD)m_iFunction);
   pData->wOperation = htons((WORD)m_iOperation);
   pData->dwArg1 = htonl(m_iParam1);
   pData->dwArg2 = htonl(m_iParam2);
   switch(m_iDataType)
   {
      case DCI_DT_INT:
         pData->value.dwInt32 = htonl(m_value.iInt);
         break;
      case DCI_DT_UINT:
         pData->value.dwInt32 = htonl(m_value.dwUInt);
         break;
      case DCI_DT_INT64:
         pData->value.qwInt64 = htonq(m_value.iInt64);
         break;
      case DCI_DT_UINT64:
         pData->value.qwInt64 = htonq(m_value.qwUInt64);
         break;
      case DCI_DT_FLOAT:
         pData->value.dFloat = htond(m_value.dFloat);
         break;
      case DCI_DT_STRING:
         strcpy(pData->value.szString,  m_pszValueStr);
         break;
      default:
         break;
   }
}


//
// Update threshold object from DCI_THRESHOLD structure
//

void Threshold::UpdateFromMessage(DCI_THRESHOLD *pData)
{
   m_dwEventCode = ntohl(pData->dwEvent);
   m_iFunction = (BYTE)ntohs(pData->wFunction);
   m_iOperation = (BYTE)ntohs(pData->wOperation);
   m_iParam1 = ntohl(pData->dwArg1);
   m_iParam2 = ntohl(pData->dwArg2);
   safe_free(m_pszValueStr);
   switch(m_iDataType)
   {
      case DCI_DT_INT:
         m_value.iInt = (long)ntohl(pData->value.dwInt32);
         m_pszValueStr = (char *)malloc(32);
         sprintf(m_pszValueStr, "%ld", m_value.iInt);
         break;
      case DCI_DT_UINT:
         m_value.dwUInt = ntohl(pData->value.dwInt32);
         m_pszValueStr = (char *)malloc(32);
         sprintf(m_pszValueStr, "%lu", m_value.dwUInt);
         break;
      case DCI_DT_INT64:
         m_value.iInt64 = (INT64)ntohq(pData->value.qwInt64);
         m_pszValueStr = (char *)malloc(32);
#ifdef _WIN32
         sprintf(m_pszValueStr, "%I64d", m_value.iInt64);
#else
         sprintf(m_pszValueStr, "%lld", m_value.iInt64);
#endif
         break;
      case DCI_DT_UINT64:
         m_value.iInt64 = (QWORD)ntohq(pData->value.qwInt64);
         m_pszValueStr = (char *)malloc(32);
#ifdef _WIN32
         sprintf(m_pszValueStr, "%I64u", m_value.qwUInt64);
#else
         sprintf(m_pszValueStr, "%llu", m_value.qwUInt64);
#endif
         break;
      case DCI_DT_FLOAT:
         m_value.dFloat = ntohd(pData->value.dFloat);
         m_pszValueStr = (char *)malloc(32);
         sprintf(m_pszValueStr, "%f", m_value.dFloat);
         break;
      case DCI_DT_STRING:
         m_pszValueStr = strdup(pData->value.szString);
         break;
      default:
         DbgPrintf(AF_DEBUG_DC, "WARNING: Invalid datatype %d in threshold object %d\n", m_iDataType, m_dwId);
         break;
   }
}
