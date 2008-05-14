/* $Id$ */
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
** File: dcithreshold.cpp
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
   m_dwRearmEventCode = EVENT_THRESHOLD_REARMED;
   m_iFunction = F_LAST;
   m_iOperation = OP_EQ;
   m_iDataType = pRelatedItem->DataType();
   m_iParam1 = 0;
   m_iParam2 = 0;
   m_bIsReached = FALSE;
	m_nRepeatInterval = -1;
	m_tmLastEventTimestamp = 0;
}


//
// Constructor for NXMP parser
//

Threshold::Threshold()
{
   m_dwId = 0;
   m_dwItemId = 0;
   m_dwEventCode = EVENT_THRESHOLD_REACHED;
   m_dwRearmEventCode = EVENT_THRESHOLD_REARMED;
   m_iFunction = F_LAST;
   m_iOperation = OP_EQ;
   m_iDataType = 0;
   m_iParam1 = 0;
   m_iParam2 = 0;
   m_bIsReached = FALSE;
	m_nRepeatInterval = -1;
	m_tmLastEventTimestamp = 0;
}


//
// Create from another threshold object
//

Threshold::Threshold(Threshold *pSrc)
{
   m_dwId = pSrc->m_dwId;
   m_dwItemId = pSrc->m_dwItemId;
   m_dwEventCode = pSrc->m_dwEventCode;
   m_dwRearmEventCode = pSrc->m_dwRearmEventCode;
   m_value = pSrc->Value();
   m_iFunction = pSrc->m_iFunction;
   m_iOperation = pSrc->m_iOperation;
   m_iDataType = pSrc->m_iDataType;
   m_iParam1 = pSrc->m_iParam1;
   m_iParam2 = pSrc->m_iParam2;
   m_bIsReached = FALSE;
	m_nRepeatInterval = pSrc->m_nRepeatInterval;
	m_tmLastEventTimestamp = 0;
}


//
// Constructor for creating object from database
// This constructor assumes that SELECT query look as following:
// SELECT threshold_id,fire_value,rearm_value,check_function,check_operation,
//        parameter_1,parameter_2,event_code,current_state,
//        rearm_event_code,repeat_interval FROM thresholds
//

Threshold::Threshold(DB_RESULT hResult, int iRow, DCItem *pRelatedItem)
{
   TCHAR szBuffer[MAX_DB_STRING];

   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   m_dwItemId = pRelatedItem->Id();
   m_dwEventCode = DBGetFieldULong(hResult, iRow, 7);
   m_dwRearmEventCode = DBGetFieldULong(hResult, iRow, 9);
   DBGetField(hResult, iRow, 1, szBuffer, MAX_DB_STRING);
   DecodeSQLString(szBuffer);
   m_value = szBuffer;
   m_iFunction = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iOperation = (BYTE)DBGetFieldLong(hResult, iRow, 4);
   m_iDataType = pRelatedItem->DataType();
   m_iParam1 = DBGetFieldLong(hResult, iRow, 5);
   m_iParam2 = DBGetFieldLong(hResult, iRow, 6);
   m_bIsReached = DBGetFieldLong(hResult, iRow, 8);
	m_nRepeatInterval = DBGetFieldLong(hResult, iRow, 10);
	m_tmLastEventTimestamp = 0;
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

BOOL Threshold::SaveToDB(DB_HANDLE hdb, DWORD dwIndex)
{
   TCHAR *pszEscValue, szQuery[512];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;

   // Check for object's existence in database
   _stprintf(szQuery, _T("SELECT threshold_id FROM thresholds WHERE threshold_id=%d"), m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Prepare and execute query
   pszEscValue = EncodeSQLString(m_value.String());
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO thresholds (threshold_id,item_id,fire_value,rearm_value,"
                       "check_function,check_operation,parameter_1,parameter_2,event_code,"
                       "sequence_number,current_state,rearm_event_code,repeat_interval) VALUES "
                       "(%d,%d,'%s','#00',%d,%d,%d,%d,%d,%d,%d,%d,%d)", 
              m_dwId, m_dwItemId, pszEscValue, m_iFunction, m_iOperation, m_iParam1,
              m_iParam2, m_dwEventCode, dwIndex, m_bIsReached, m_dwRearmEventCode,
				  m_nRepeatInterval);
   else
      sprintf(szQuery, "UPDATE thresholds SET item_id=%d,fire_value='%s',check_function=%d,"
                       "check_operation=%d,parameter_1=%d,parameter_2=%d,event_code=%d,"
                       "sequence_number=%d,current_state=%d,"
                       "rearm_event_code=%d,repeat_interval=%d WHERE threshold_id=%d",
              m_dwItemId, pszEscValue, m_iFunction, m_iOperation, m_iParam1,
              m_iParam2, m_dwEventCode, dwIndex, m_bIsReached,
              m_dwRearmEventCode, m_nRepeatInterval, m_dwId);
   free(pszEscValue);
   return DBQuery(hdb, szQuery);
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
      case F_DEVIATION:    // Check mean absolute deviation
         CalculateMDValue(&fvalue, value, ppPrevValues);
         break;
      case F_DIFF:
         CalculateDiff(&fvalue, value, ppPrevValues);
         if (m_iDataType == DCI_DT_STRING)
            iDataType = DCI_DT_INT;  // diff() for strings is an integer
         break;
      case F_ERROR:        // Check for collection error
         fvalue = (DWORD)0;
         break;
      default:
         break;
   }

   // Run comparision operation on function result and threshold value
   if (m_iFunction == F_ERROR)
   {
      // Threshold::Check() can be called only for valid values, which
      // means that error thresholds cannot be active
      bMatch = FALSE;
   }
   else
   {
      switch(m_iOperation)
      {
         case OP_LE:    // Less
            switch(iDataType)
            {
               case DCI_DT_INT:
                  bMatch = ((LONG)fvalue < (LONG)m_value);
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
                  bMatch = ((LONG)fvalue <= (LONG)m_value);
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
                  bMatch = ((LONG)fvalue == (LONG)m_value);
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
                  bMatch = ((LONG)fvalue >= (LONG)m_value);
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
                  bMatch = ((LONG)fvalue > (LONG)m_value);
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
                  bMatch = ((LONG)fvalue != (LONG)m_value);
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
   }

   iResult = (bMatch & !m_bIsReached) ? THRESHOLD_REACHED :
                ((!bMatch & m_bIsReached) ? THRESHOLD_REARMED : NO_ACTION);
   m_bIsReached = bMatch;
   if (iResult != NO_ACTION)
   {
      TCHAR szQuery[256];

      // Update threshold status in database
      _sntprintf(szQuery, 256,
                 _T("UPDATE thresholds SET current_state=%d WHERE threshold_id=%d"),
                 m_bIsReached, m_dwId);
      QueueSQLRequest(szQuery);
   }
   return iResult;
}


//
// Check for collection error thresholds
// Return same values as Check()
//

int Threshold::CheckError(DWORD dwErrorCount)
{
   int nResult;
   BOOL bMatch;

   if (m_iFunction != F_ERROR)
      return NO_ACTION;

   bMatch = ((DWORD)m_iParam1 <= dwErrorCount);
   nResult = (bMatch & !m_bIsReached) ? THRESHOLD_REACHED :
                ((!bMatch & m_bIsReached) ? THRESHOLD_REARMED : NO_ACTION);
   m_bIsReached = bMatch;
   if (nResult != NO_ACTION)
   {
      TCHAR szQuery[256];

      // Update threshold status in database
      _sntprintf(szQuery, 256,
                 _T("UPDATE thresholds SET current_state=%d WHERE threshold_id=%d"),
                 m_bIsReached, m_dwId);
      QueueSQLRequest(szQuery);
   }
   return nResult;
}


//
// Fill DCI_THRESHOLD with object's data ready to send over the network
//

void Threshold::CreateMessage(DCI_THRESHOLD *pData)
{
   pData->dwId = htonl(m_dwId);
   pData->dwEvent = htonl(m_dwEventCode);
   pData->dwRearmEvent = htonl(m_dwRearmEventCode);
   pData->wFunction = htons((WORD)m_iFunction);
   pData->wOperation = htons((WORD)m_iOperation);
   pData->dwArg1 = htonl(m_iParam1);
   pData->dwArg2 = htonl(m_iParam2);
	pData->nRepeatInterval = htonl(m_nRepeatInterval);
   switch(m_iDataType)
   {
      case DCI_DT_INT:
         pData->value.dwInt32 = htonl((LONG)m_value);
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
#ifdef UNICODE
#ifdef UNICODE_UCS4
         ucs4_to_ucs2(m_value.String(), -1, pData->value.szString, MAX_DCI_STRING_VALUE);
#else
         wcscpy(pData->value.szString,  m_value.String());
#endif
#else
         mb_to_ucs2(m_value.String(), -1, pData->value.szString, MAX_DCI_STRING_VALUE);
#endif
         SwapWideString(pData->value.szString);
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
#ifndef UNICODE_UCS2
   TCHAR szBuffer[MAX_DCI_STRING_VALUE];
#endif

   m_dwEventCode = ntohl(pData->dwEvent);
   m_dwRearmEventCode = ntohl(pData->dwRearmEvent);
   m_iFunction = (BYTE)ntohs(pData->wFunction);
   m_iOperation = (BYTE)ntohs(pData->wOperation);
   m_iParam1 = ntohl(pData->dwArg1);
   m_iParam2 = ntohl(pData->dwArg2);
	m_nRepeatInterval = ntohl(pData->nRepeatInterval);
   switch(m_iDataType)
   {
      case DCI_DT_INT:
         m_value = (LONG)ntohl(pData->value.dwInt32);
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
         SwapWideString(pData->value.szString);
#ifdef UNICODE
#ifdef UNICODE_UCS4
         ucs2_to_ucs4(pData->value.szString, -1, szBuffer, MAX_DCI_STRING_VALUE);
         m_value = szBuffer;
#else
         m_value = pData->value.szString;
#endif         
#else
         ucs2_to_mb(pData->value.szString, -1, szBuffer, MAX_DCI_STRING_VALUE);
         m_value = szBuffer;
#endif
         break;
      default:
         DbgPrintf(3, "WARNING: Invalid datatype %d in threshold object %d", m_iDataType, m_dwId);
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
         CALC_AVG_VALUE(LONG);
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
// Calculate mean absolute deviation for parameter
//

#define CALC_MD_VALUE(vtype) \
{ \
   vtype mean, dev; \
   mean = (vtype)lastValue; \
   for(i = 1, nValueCount = 1; i < m_iParam1; i++) \
   { \
      if (ppPrevValues[i - 1]->GetTimeStamp() != 1) \
      { \
         mean += (vtype)(*ppPrevValues[i - 1]); \
         nValueCount++; \
      } \
   } \
   mean /= (vtype)nValueCount; \
   dev = ABS((vtype)lastValue - mean); \
   for(i = 1, nValueCount = 1; i < m_iParam1; i++) \
   { \
      if (ppPrevValues[i - 1]->GetTimeStamp() != 1) \
      { \
         dev += ABS((vtype)(*ppPrevValues[i - 1]) - mean); \
         nValueCount++; \
      } \
   } \
   *pResult = dev / (vtype)nValueCount; \
}

void Threshold::CalculateMDValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues)
{
   int i, nValueCount;

   switch(m_iDataType)
   {
      case DCI_DT_INT:
#define ABS(x) ((x) < 0 ? -(x) : (x))
         CALC_MD_VALUE(LONG);
         break;
      case DCI_DT_INT64:
         CALC_MD_VALUE(INT64);
         break;
      case DCI_DT_FLOAT:
         CALC_MD_VALUE(double);
         break;
      case DCI_DT_UINT:
#undef ABS
#define ABS(x) (x)
         CALC_MD_VALUE(DWORD);
         break;
      case DCI_DT_UINT64:
         CALC_MD_VALUE(QWORD);
         break;
      case DCI_DT_STRING:
         *pResult = _T("");   // Mean deviation for string is meaningless
         break;
      default:
         break;
   }
}

#undef ABS


//
// Calculate difference between last and previous value
//

void Threshold::CalculateDiff(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues)
{
   CalculateItemValueDiff(*pResult, m_iDataType, lastValue, *ppPrevValues[0]);
}


//
// Compare to another threshold
//

BOOL Threshold::Compare(Threshold *pThr)
{
   BOOL bMatch;

   switch(m_iDataType)
   {
      case DCI_DT_INT:
         bMatch = ((LONG)pThr->m_value == (LONG)m_value);
         break;
      case DCI_DT_UINT:
         bMatch = ((DWORD)pThr->m_value == (DWORD)m_value);
         break;
      case DCI_DT_INT64:
         bMatch = ((INT64)pThr->m_value == (INT64)m_value);
         break;
      case DCI_DT_UINT64:
         bMatch = ((QWORD)pThr->m_value == (QWORD)m_value);
         break;
      case DCI_DT_FLOAT:
         bMatch = ((double)pThr->m_value == (double)m_value);
         break;
      case DCI_DT_STRING:
         bMatch = !strcmp(pThr->m_value.String(), m_value.String());
         break;
      default:
         bMatch = TRUE;
         break;
   }
   return bMatch &&
          (pThr->m_dwEventCode == m_dwEventCode) &&
          (pThr->m_dwRearmEventCode == m_dwRearmEventCode) &&
          (pThr->m_iDataType == m_iDataType) &&
          (pThr->m_iFunction == m_iFunction) &&
          (pThr->m_iOperation == m_iOperation) &&
          (pThr->m_iParam1 == m_iParam1) &&
          (pThr->m_iParam2 == m_iParam2) &&
			 (pThr->m_nRepeatInterval == m_nRepeatInterval);
}


//
// Create management pack record
//

void Threshold::CreateNXMPRecord(String &str)
{
   TCHAR szEvent1[MAX_EVENT_NAME], szEvent2[MAX_EVENT_NAME];
   String strValue;

   strValue = (TCHAR *)m_value.String();
   ResolveEventName(m_dwEventCode, szEvent1);
   ResolveEventName(m_dwRearmEventCode, szEvent2);
   str.AddFormattedString(_T("\t\t\t\t\t@THRESHOLD\n\t\t\t\t\t{\n")
                          _T("\t\t\t\t\t\tFUNCTION=%d;\n")
                          _T("\t\t\t\t\t\tCONDITION=%d;\n")
                          _T("\t\t\t\t\t\tVALUE=\"%s\";\n")
                          _T("\t\t\t\t\t\tACTIVATION_EVENT=\"%s\";\n")
                          _T("\t\t\t\t\t\tDEACTIVATION_EVENT=\"%s\";\n")
                          _T("\t\t\t\t\t\tPARAM1=%d;\n")
                          _T("\t\t\t\t\t\tPARAM2=%d;\n")
                          _T("\t\t\t\t\t\tREPEAT_INTERVAL=%d;\n")
                          _T("\t\t\t\t\t}\n"),
                          m_iFunction, m_iOperation, (TCHAR *)strValue,
                          szEvent1, szEvent2, m_iParam1, m_iParam2,
								  m_nRepeatInterval);
}


//
// Made an association with DCI (used by management pack parser)
//

void Threshold::Associate(DCItem *pItem)
{
   m_dwItemId = pItem->Id();
   m_iDataType = pItem->DataType();
}
