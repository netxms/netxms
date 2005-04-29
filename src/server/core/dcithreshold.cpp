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

#include "nxcore.h"


//
// Default constructor
//

Threshold::Threshold(DCItem *pRelatedItem)
{
   m_dwId = 0;
   m_dwItemId = pRelatedItem->Id();
   m_dwEventCode = EVENT_THRESHOLD_REACHED;
   m_iFunction = F_LAST;
   m_iOperation = OP_EQ;
   m_iDataType = pRelatedItem->DataType();
   m_iParam1 = 0;
   m_iParam2 = 0;
   m_bIsReached = FALSE;
}


//
// Create from another threshold object
//

Threshold::Threshold(Threshold *pSrc)
{
   m_dwId = pSrc->m_dwId;
   m_dwItemId = pSrc->m_dwItemId;
   m_dwEventCode = pSrc->m_dwEventCode;
   m_value = pSrc->Value();
   m_iFunction = pSrc->m_iFunction;
   m_iOperation = pSrc->m_iOperation;
   m_iDataType = pSrc->m_iDataType;
   m_iParam1 = pSrc->m_iParam1;
   m_iParam2 = pSrc->m_iParam2;
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
   m_value = DBGetField(hResult, iRow, 1);
   m_iFunction = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iOperation = (BYTE)DBGetFieldLong(hResult, iRow, 4);
   m_iDataType = pRelatedItem->DataType();
   m_iParam1 = DBGetFieldLong(hResult, iRow, 5);
   m_iParam2 = DBGetFieldLong(hResult, iRow, 6);
   m_bIsReached = FALSE;
}


//
// Destructor
//

Threshold::~Threshold()
{
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
              m_dwId, m_dwItemId, m_value.String(), m_iFunction, m_iOperation, m_iParam1,
              m_iParam2, m_dwEventCode, dwIndex);
   else
      sprintf(szQuery, "UPDATE thresholds SET item_id=%ld,fire_value='%s',check_function=%d,"
                       "check_operation=%d,parameter_1=%ld,parameter_2=%ld,event_code=%ld,"
                       "sequence_number=%ld WHERE threshold_id=%ld", m_dwItemId, 
              m_value.String(), m_iFunction, m_iOperation, m_iParam1, m_iParam2, m_dwEventCode, 
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

int Threshold::Check(ItemValue &value, ItemValue **ppPrevValues, ItemValue &fvalue)
{
   BOOL bMatch = FALSE;
   int iResult, iDataType = m_iDataType;

   // Execute function on value
   switch(m_iFunction)
   {
      case F_LAST:         // Check last value only
         fvalue = value;
         break;
      case F_AVERAGE:      // Check average value for last n polls
         CalculateAverageValue(&fvalue, value, ppPrevValues);
         break;
      case F_DEVIATION:    // Check deviation from the mean value
         /* TODO */
         break;
      case F_DIFF:
         CalculateDiff(&fvalue, value, ppPrevValues);
         if (m_iDataType == DCI_DT_STRING)
            iDataType = DCI_DT_INT;  // diff() for strings is an integer
         break;
      default:
         break;
   }

   // Run comparision operation on function result and threshold value
   switch(m_iOperation)
   {
      case OP_LE:    // Less
         switch(iDataType)
         {
            case DCI_DT_INT:
               bMatch = ((long)fvalue < (long)m_value);
               break;
            case DCI_DT_UINT:
               bMatch = ((DWORD)fvalue < (DWORD)m_value);
               break;
            case DCI_DT_INT64:
               bMatch = ((INT64)fvalue < (INT64)m_value);
               break;
            case DCI_DT_UINT64:
               bMatch = ((QWORD)fvalue < (QWORD)m_value);
               break;
            case DCI_DT_FLOAT:
               bMatch = ((double)fvalue < (double)m_value);
               break;
         }
         break;
      case OP_LE_EQ: // Less or equal
         switch(iDataType)
         {
            case DCI_DT_INT:
               bMatch = ((long)fvalue <= (long)m_value);
               break;
            case DCI_DT_UINT:
               bMatch = ((DWORD)fvalue <= (DWORD)m_value);
               break;
            case DCI_DT_INT64:
               bMatch = ((INT64)fvalue <= (INT64)m_value);
               break;
            case DCI_DT_UINT64:
               bMatch = ((QWORD)fvalue <= (QWORD)m_value);
               break;
            case DCI_DT_FLOAT:
               bMatch = ((double)fvalue <= (double)m_value);
               break;
         }
         break;
      case OP_EQ:    // Equal
         switch(iDataType)
         {
            case DCI_DT_INT:
               bMatch = ((long)fvalue == (long)m_value);
               break;
            case DCI_DT_UINT:
               bMatch = ((DWORD)fvalue == (DWORD)m_value);
               break;
            case DCI_DT_INT64:
               bMatch = ((INT64)fvalue == (INT64)m_value);
               break;
            case DCI_DT_UINT64:
               bMatch = ((QWORD)fvalue == (QWORD)m_value);
               break;
            case DCI_DT_FLOAT:
               bMatch = ((double)fvalue == (double)m_value);
               break;
            case DCI_DT_STRING:
               bMatch = !strcmp(fvalue.String(), m_value.String());
               break;
         }
         break;
      case OP_GT_EQ: // Greater or equal
         switch(iDataType)
         {
            case DCI_DT_INT:
               bMatch = ((long)fvalue >= (long)m_value);
               break;
            case DCI_DT_UINT:
               bMatch = ((DWORD)fvalue >= (DWORD)m_value);
               break;
            case DCI_DT_INT64:
               bMatch = ((INT64)fvalue >= (INT64)m_value);
               break;
            case DCI_DT_UINT64:
               bMatch = ((QWORD)fvalue >= (QWORD)m_value);
               break;
            case DCI_DT_FLOAT:
               bMatch = ((double)fvalue >= (double)m_value);
               break;
         }
         break;
      case OP_GT:    // Greater
         switch(iDataType)
         {
            case DCI_DT_INT:
               bMatch = ((long)fvalue > (long)m_value);
               break;
            case DCI_DT_UINT:
               bMatch = ((DWORD)fvalue > (DWORD)m_value);
               break;
            case DCI_DT_INT64:
               bMatch = ((INT64)fvalue > (INT64)m_value);
               break;
            case DCI_DT_UINT64:
               bMatch = ((QWORD)fvalue > (QWORD)m_value);
               break;
            case DCI_DT_FLOAT:
               bMatch = ((double)fvalue > (double)m_value);
               break;
         }
         break;
      case OP_NE:    // Not equal
         switch(iDataType)
         {
            case DCI_DT_INT:
               bMatch = ((long)fvalue != (long)m_value);
               break;
            case DCI_DT_UINT:
               bMatch = ((DWORD)fvalue != (DWORD)m_value);
               break;
            case DCI_DT_INT64:
               bMatch = ((INT64)fvalue != (INT64)m_value);
               break;
            case DCI_DT_UINT64:
               bMatch = ((QWORD)fvalue != (QWORD)m_value);
               break;
            case DCI_DT_FLOAT:
               bMatch = ((double)fvalue != (double)m_value);
               break;
            case DCI_DT_STRING:
               bMatch = strcmp(fvalue.String(), m_value.String());
               break;
         }
         break;
      case OP_LIKE:
         // This operation can be performed only on strings
         if (m_iDataType == DCI_DT_STRING)
            bMatch = MatchString(m_value.String(), fvalue.String(), TRUE);
         break;
      case OP_NOTLIKE:
         // This operation can be performed only on strings
         if (m_iDataType == DCI_DT_STRING)
            bMatch = !MatchString(m_value.String(), fvalue.String(), TRUE);
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
         pData->value.dwInt32 = htonl((long)m_value);
         break;
      case DCI_DT_UINT:
         pData->value.dwInt32 = htonl((DWORD)m_value);
         break;
      case DCI_DT_INT64:
         pData->value.qwInt64 = htonq((INT64)m_value);
         break;
      case DCI_DT_UINT64:
         pData->value.qwInt64 = htonq((QWORD)m_value);
         break;
      case DCI_DT_FLOAT:
         pData->value.dFloat = htond((double)m_value);
         break;
      case DCI_DT_STRING:
         strcpy(pData->value.szString,  m_value.String());
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
   switch(m_iDataType)
   {
      case DCI_DT_INT:
         m_value = (long)ntohl(pData->value.dwInt32);
         break;
      case DCI_DT_UINT:
         m_value = (DWORD)ntohl(pData->value.dwInt32);
         break;
      case DCI_DT_INT64:
         m_value = (INT64)ntohq(pData->value.qwInt64);
         break;
      case DCI_DT_UINT64:
         m_value = (QWORD)ntohq(pData->value.qwInt64);
         break;
      case DCI_DT_FLOAT:
         m_value = ntohd(pData->value.dFloat);
         break;
      case DCI_DT_STRING:
         m_value = pData->value.szString;
         break;
      default:
         DbgPrintf(AF_DEBUG_DC, "WARNING: Invalid datatype %d in threshold object %d", m_iDataType, m_dwId);
         break;
   }
}


//
// Calculate average value for parameter
//

#define CALC_AVG_VALUE(vtype) \
{ \
   vtype var; \
   var = (vtype)lastValue; \
   for(i = 1, nValueCount = 1; i < m_iParam1; i++) \
   { \
      if (ppPrevValues[i - 1]->GetTimeStamp() != 1) \
      { \
         var += (vtype)(*ppPrevValues[i - 1]); \
         nValueCount++; \
      } \
   } \
   *pResult = var / (vtype)nValueCount; \
}

void Threshold::CalculateAverageValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues)
{
   int i, nValueCount;

   switch(m_iDataType)
   {
      case DCI_DT_INT:
         CALC_AVG_VALUE(long);
         break;
      case DCI_DT_UINT:
         CALC_AVG_VALUE(DWORD);
         break;
      case DCI_DT_INT64:
         CALC_AVG_VALUE(INT64);
         break;
      case DCI_DT_UINT64:
         CALC_AVG_VALUE(QWORD);
         break;
      case DCI_DT_FLOAT:
         CALC_AVG_VALUE(double);
         break;
      case DCI_DT_STRING:
         *pResult = _T("");   // Average value for string is meaningless
         break;
      default:
         break;
   }
}


//
// Calculate difference between last and previous value
//

void Threshold::CalculateDiff(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues)
{
   switch(m_iDataType)
   {
      case DCI_DT_INT:
         *pResult = (long)lastValue - (long)(*ppPrevValues[0]);
         break;
      case DCI_DT_UINT:
         *pResult = (DWORD)lastValue - (DWORD)(*ppPrevValues[0]);
         break;
      case DCI_DT_INT64:
         *pResult = (INT64)lastValue - (INT64)(*ppPrevValues[0]);
         break;
      case DCI_DT_UINT64:
         *pResult = (QWORD)lastValue - (QWORD)(*ppPrevValues[0]);
         break;
      case DCI_DT_FLOAT:
         *pResult = (double)lastValue - (double)(*ppPrevValues[0]);
         break;
      case DCI_DT_STRING:
         *pResult = (long)((_tcscmp((const TCHAR *)lastValue, (const TCHAR *)(*ppPrevValues[0])) == 0) ? 0 : 1);
         break;
      default:
         // Delta calculation is not supported for other types
         *pResult = lastValue;
         break;
   }
}
