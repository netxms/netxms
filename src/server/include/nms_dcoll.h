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
** $module: nms_dcoll.h
**
**/

#ifndef _nms_dcoll_h_
#define _nms_dcoll_h_


//
// Data collection errors
//

#define DCE_SUCCESS        0
#define DCE_COMM_ERROR     1
#define DCE_NOT_SUPPORTED  2


//
// Threshold check results
//

#define THRESHOLD_REACHED  0
#define THRESHOLD_REARMED  1
#define NO_ACTION          2


//
// DCI value
//

class ItemValue
{
private:
   double m_dFloat;
   long m_iInt32;
   INT64 m_iInt64;
   DWORD m_dwInt32;
   QWORD m_qwInt64;
   TCHAR m_szString[MAX_DB_STRING];

public:
   ItemValue();
   ItemValue(const TCHAR *pszValue);
   ItemValue(const ItemValue *pValue);
   ~ItemValue();

   const TCHAR *String(void) { return m_szString; }

   operator double() { return m_dFloat; }
   operator DWORD() { return m_dwInt32; }
   operator QWORD() { return m_qwInt64; }
   operator long() { return m_iInt32; }
   operator INT64() { return m_iInt64; }
   operator const char*() const { return m_szString; }

   const ItemValue& operator=(const ItemValue &src);
   const ItemValue& operator=(const TCHAR *pszStr);
   const ItemValue& operator=(double dFloat);
   const ItemValue& operator=(long iInt32);
   const ItemValue& operator=(INT64 iInt64);
   const ItemValue& operator=(DWORD dwInt32);
   const ItemValue& operator=(QWORD qwInt64);
};


//
// Threshold definition class
//

class DCItem;

class Threshold
{
private:
   DWORD m_dwId;             // Unique threshold id
   DWORD m_dwItemId;         // Related item id
   DWORD m_dwEventCode;      // Event code to be generated
   ItemValue m_value;
   BYTE m_iFunction;          // Function code
   BYTE m_iOperation;         // Comparision operation code
   BYTE m_iDataType;          // Related item data type
   int m_iParam1;             // Function's parameter #1
   int m_iParam2;             // Function's parameter #2
   BOOL m_bIsReached;

   const ItemValue& Value(void) { return m_value; }
   void CalculateAverageValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);

public:
   Threshold(DCItem *pRelatedItem);
   Threshold(Threshold *pSrc);
   Threshold(DB_RESULT hResult, int iRow, DCItem *pRelatedItem);
   ~Threshold();

   void BindToItem(DWORD dwItemId) { m_dwItemId = dwItemId; }

   DWORD Id(void) { return m_dwId; }
   DWORD EventCode(void) { return m_dwEventCode; }
   const char *StringValue(void) { return m_value.String(); }
   BOOL IsReached(void) { return m_bIsReached; }

   BOOL SaveToDB(DWORD dwIndex);
   int Check(ItemValue &value, ItemValue **ppPrevValues, ItemValue &fvalue);

   void CreateMessage(DCI_THRESHOLD *pData);
   void UpdateFromMessage(DCI_THRESHOLD *pData);

   void CreateId(void);
   DWORD RequiredCacheSize(void) { return (m_iFunction == F_LAST) ? 0 : m_iParam1; }
};


//
// Data collection item class
//

class Template;

class DCItem
{
private:
   DWORD m_dwId;
   char m_szName[MAX_ITEM_NAME];
   char m_szDescription[MAX_DB_STRING];
   char m_szInstance[MAX_DB_STRING];
   time_t m_tLastPoll;        // Last poll time
   int m_iPollingInterval;    // Polling interval in seconds
   int m_iRetentionTime;      // Retention time in seconds
   BYTE m_iDeltaCalculation;  // Delta calculation method
   BYTE m_iSource;            // SNMP or native agent?
   BYTE m_iDataType;
   BYTE m_iStatus;            // Item status: active, disabled or not supported
   BYTE m_iBusy;              // 1 when item is queued for polling, 0 if not
   DWORD m_dwTemplateId;      // Related template's id
   DWORD m_dwNumThresholds;
   Threshold **m_ppThresholdList;
   Template *m_pNode;             // Pointer to node or template object this item related to
   char *m_pszFormula;        // Transformation formula
   MUTEX m_hMutex;
   DWORD m_dwCacheSize;       // Number of items in cache
   ItemValue **m_ppValueCache;
   ItemValue m_prevRawValue;  // Previous raw value (used for delta calculation)

   void Lock(void) { MutexLock(m_hMutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hMutex); }

   void Transform(ItemValue &value, long nElapsedTime);
   void CheckThresholds(ItemValue &value);
   void UpdateCacheSize(void);

public:
   DCItem();
   DCItem(const DCItem *pItem);
   DCItem(DB_RESULT hResult, int iRow, Template *pNode);
   DCItem(DWORD dwId, char *szName, int iSource, int iDataType, 
          int iPollingInterval, int iRetentionTime, Template *pNode,
          char *pszDescription = NULL);
   ~DCItem();

   BOOL SaveToDB(void);
   BOOL LoadThresholdsFromDB(void);
   void DeleteFromDB(void);

   DWORD Id(void) { return m_dwId; }
   int DataSource(void) { return m_iSource; }
   int DataType(void) { return m_iDataType; }
   const char *Name(void) { return m_szName; }
   Template *RelatedNode(void) { return m_pNode; }

   BOOL ReadyForPolling(time_t currTime) 
   { 
      return ((m_iStatus == ITEM_STATUS_ACTIVE) && (!m_iBusy) &&
              (m_tLastPoll + m_iPollingInterval <= currTime));
   }

   void SetLastPollTime(time_t tLastPoll) { m_tLastPoll = tLastPoll; }
   void SetStatus(int iStatus) { m_iStatus = (BYTE)iStatus; }
   void SetBusyFlag(BOOL bIsBusy) { m_iBusy = (BYTE)bIsBusy; }
   void SetId(DWORD dwNewId);
   void BindToNode(Template *pNode) { m_pNode = pNode; }

   void NewValue(DWORD dwTimeStamp, const char *pszValue);

   void GetLastValue(CSCPMessage *pMsg, DWORD dwId);

   void CreateMessage(CSCPMessage *pMsg);
   void UpdateFromMessage(CSCPMessage *pMsg, DWORD *pdwNumMaps, DWORD **ppdwMapIndex, DWORD **ppdwMapId);
};


//
// Functions
//

BOOL InitDataCollector(void);
void DeleteAllItemsForNode(DWORD dwNodeId);


//
// Variables
//

extern double g_dAvgPollerQueueSize;
extern double g_dAvgDBWriterQueueSize;
extern double g_dAvgStatusPollerQueueSize;
extern double g_dAvgConfigPollerQueueSize;
extern DWORD g_dwAvgDCIQueuingTime;


#endif   /* _nms_dcoll_h_ */
