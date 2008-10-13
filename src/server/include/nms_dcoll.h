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
** File: nms_dcoll.h
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
   LONG m_iInt32;
   INT64 m_iInt64;
   DWORD m_dwInt32;
   QWORD m_qwInt64;
   TCHAR m_szString[MAX_DB_STRING];
   DWORD m_dwTimeStamp;

public:
   ItemValue();
   ItemValue(const TCHAR *pszValue, DWORD dwTimeStamp);
   ItemValue(const ItemValue *pValue);
   ~ItemValue();

   void SetTimeStamp(DWORD dwTime) { m_dwTimeStamp = dwTime; }
   DWORD GetTimeStamp(void) { return m_dwTimeStamp; }

   const TCHAR *String(void) { return m_szString; }

   operator double() { return m_dFloat; }
   operator DWORD() { return m_dwInt32; }
   operator QWORD() { return m_qwInt64; }
   operator LONG() { return m_iInt32; }
   operator INT64() { return m_iInt64; }
   operator const char*() const { return m_szString; }

   const ItemValue& operator=(const ItemValue &src);
   const ItemValue& operator=(const TCHAR *pszStr);
   const ItemValue& operator=(double dFloat);
   const ItemValue& operator=(LONG iInt32);
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
   DWORD m_dwRearmEventCode;
   ItemValue m_value;
   BYTE m_iFunction;          // Function code
   BYTE m_iOperation;         // Comparision operation code
   BYTE m_iDataType;          // Related item data type
   int m_iParam1;             // Function's parameter #1
   int m_iParam2;             // Function's parameter #2
   BOOL m_bIsReached;
	int m_iNumMatches;			// Number of consecutive matches
	int m_nRepeatInterval;		// -1 = default, 0 = off, >0 = seconds between repeats
	time_t m_tmLastEventTimestamp;

   const ItemValue& Value(void) { return m_value; }
   void CalculateAverageValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);
   void CalculateMDValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);
   void CalculateDiff(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);

public:
	Threshold();
   Threshold(DCItem *pRelatedItem);
   Threshold(Threshold *pSrc);
   Threshold(DB_RESULT hResult, int iRow, DCItem *pRelatedItem);
   ~Threshold();

   void BindToItem(DWORD dwItemId) { m_dwItemId = dwItemId; }

   DWORD Id() { return m_dwId; }
   DWORD EventCode() { return m_dwEventCode; }
   DWORD RearmEventCode() { return m_dwRearmEventCode; }
   const char *StringValue() { return m_value.String(); }
   BOOL IsReached() { return m_bIsReached; }
	
	int RepeatInterval() { return m_nRepeatInterval; }
	time_t GetLastEventTimestamp() { return m_tmLastEventTimestamp; }
	void SetLastEventTimestamp() { m_tmLastEventTimestamp = time(NULL); }

   BOOL SaveToDB(DB_HANDLE hdb, DWORD dwIndex);
   int Check(ItemValue &value, ItemValue **ppPrevValues, ItemValue &fvalue);
   int CheckError(DWORD dwErrorCount);

   void CreateMessage(DCI_THRESHOLD *pData);
   void UpdateFromMessage(DCI_THRESHOLD *pData);

   void CreateId(void);
   DWORD RequiredCacheSize(void) { return ((m_iFunction == F_LAST) || (m_iFunction == F_ERROR)) ? 0 : m_iParam1; }

   BOOL Compare(Threshold *pThr);

   void CreateNXMPRecord(String &str);

	void Associate(DCItem *pItem);
	void SetFunction(int nFunc) { m_iFunction = nFunc; }
	void SetOperation(int nOp) { m_iOperation = nOp; }
	void SetEvent(DWORD dwEvent) { m_dwEventCode = dwEvent; }
	void SetRearmEvent(DWORD dwEvent) { m_dwRearmEventCode = dwEvent; }
	void SetParam1(int nVal) { m_iParam1 = nVal; }
	void SetParam2(int nVal) { m_iParam2 = nVal; }
	void SetValue(TCHAR *pszValue) { m_value = pszValue; }
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
   BYTE m_iAdvSchedule;       // 1 if item has advanced schedule
   BYTE m_iProcessAllThresholds; // 1 if all thresholds should be processed each time
   DWORD m_dwTemplateId;      // Related template's id
   DWORD m_dwTemplateItemId;  // Related template item's id
   DWORD m_dwNumThresholds;
   Threshold **m_ppThresholdList;
   Template *m_pNode;             // Pointer to node or template object this item related to
   char *m_pszFormula;        // Transformation formula
   NXSL_Program *m_pScript;   // Compiled transformation script
   MUTEX m_hMutex;
   DWORD m_dwCacheSize;       // Number of items in cache
   ItemValue **m_ppValueCache;
   ItemValue m_prevRawValue;  // Previous raw value (used for delta calculation)
   time_t m_tPrevValueTimeStamp;
   BOOL m_bCacheLoaded;
   DWORD m_dwNumSchedules;
   TCHAR **m_ppScheduleList;
   time_t m_tLastCheck;       // Last schedule checking time
   DWORD m_dwErrorCount;      // Consequtive collection error count
	DWORD m_dwResourceId;		// Associated cluster resource ID
	DWORD m_dwProxyNode;       // Proxy node ID or 0 to disable

   void Lock(void) { MutexLock(m_hMutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hMutex); }

   void Transform(ItemValue &value, time_t nElapsedTime);
   void CheckThresholds(ItemValue &value);
   void ClearCache(void);

	BOOL MatchClusterResource(void);

   void NewFormula(TCHAR *pszFormula);
	void ExpandMacros(const TCHAR *src, TCHAR *dst, size_t dstLen);

public:
   DCItem();
   DCItem(const DCItem *pItem);
   DCItem(DB_RESULT hResult, int iRow, Template *pNode);
   DCItem(DWORD dwId, const TCHAR *szName, int iSource, int iDataType, 
          int iPollingInterval, int iRetentionTime, Template *pNode,
          const TCHAR *pszDescription = NULL);
   ~DCItem();

   void PrepareForDeletion(void);
   void UpdateFromTemplate(DCItem *pItem);

   BOOL SaveToDB(DB_HANDLE hdb);
   BOOL LoadThresholdsFromDB(void);
   void DeleteFromDB(void);

   void UpdateCacheSize(DWORD dwCondId = 0);

   DWORD Id(void) { return m_dwId; }
   int DataSource(void) { return m_iSource; }
   int DataType(void) { return m_iDataType; }
   int Status(void) { return m_iStatus; }
   const char *Name(void) { return m_szName; }
   const char *Description(void) { return m_szDescription; }
   Template *RelatedNode(void) { return m_pNode; }
   DWORD TemplateId(void) { return m_dwTemplateId; }
   DWORD TemplateItemId(void) { return m_dwTemplateItemId; }
	DWORD ResourceId(void) { return m_dwResourceId; }
	DWORD ProxyNode(void) { return m_dwProxyNode; }

   BOOL ReadyForPolling(time_t currTime);
   void SetLastPollTime(time_t tLastPoll) { m_tLastPoll = tLastPoll; }
   void SetStatus(int iStatus) { m_iStatus = (BYTE)iStatus; }
   void SetBusyFlag(BOOL bIsBusy) { m_iBusy = (BYTE)bIsBusy; }
   void ChangeBinding(DWORD dwNewId, Template *pNode, BOOL doMacroExpansion);
   void SetTemplateId(DWORD dwTemplateId, DWORD dwItemId) 
         { m_dwTemplateId = dwTemplateId; m_dwTemplateItemId = dwItemId; }
	void SystemModify(const TCHAR *pszName, int nOrigin, int nRetention, int nInterval, int nDataType);

   void NewValue(time_t nTimeStamp, const char *pszValue);
   void NewError(void);

   void GetLastValue(CSCPMessage *pMsg, DWORD dwId);
   NXSL_Value *GetValueForNXSL(int nFunction, int nPolls);

   void CreateMessage(CSCPMessage *pMsg);
   void UpdateFromMessage(CSCPMessage *pMsg, DWORD *pdwNumMaps, DWORD **ppdwMapIndex, DWORD **ppdwMapId);

   void CleanData(void);

   void GetEventList(DWORD **ppdwList, DWORD *pdwSize);
   void CreateNXMPRecord(String &str);

	BOOL EnumThresholds(BOOL (* pfCallback)(Threshold *, DWORD, void *), void *pArg);

	void FinishMPParsing(void);
	void SetName(TCHAR *pszName) { nx_strncpy(m_szName, pszName, MAX_ITEM_NAME); }
	void SetDescription(TCHAR *pszDescr) { nx_strncpy(m_szDescription, pszDescr, MAX_DB_STRING); }
	void SetInstance(TCHAR *pszInstance) { nx_strncpy(m_szInstance, pszInstance, MAX_DB_STRING); }
	void SetOrigin(int nOrigin) { m_iSource = nOrigin; }
	void SetDataType(int nDataType) { m_iDataType = nDataType; }
	void SetRetentionTime(int nTime) { m_iRetentionTime = nTime; }
	void SetInterval(int nInt) { m_iPollingInterval = nInt; }
	void SetDeltaCalcMethod(int nMethod) { m_iDeltaCalculation = nMethod; }
	void SetAllThresholdsFlag(BOOL bFlag) { m_iProcessAllThresholds = bFlag ? 1 : 0; }
	void SetAdvScheduleFlag(BOOL bFlag) { m_iAdvSchedule = bFlag ? 1 : 0; }
	void AddThreshold(Threshold *pThreshold);
	void AddSchedule(TCHAR *pszSchedule);
};


//
// Functions
//

BOOL InitDataCollector(void);
void DeleteAllItemsForNode(DWORD dwNodeId);
void WriteFullParamListToMessage(CSCPMessage *pMsg);

void CalculateItemValueDiff(ItemValue &result, int nDataType,
                            ItemValue &value1, ItemValue &value2);
void CalculateItemValueAverage(ItemValue &result, int nDataType,
                               int nNumValues, ItemValue **ppValueList);
void CalculateItemValueMD(ItemValue &result, int nDataType,
                          int nNumValues, ItemValue **ppValueList);


//
// Variables
//

extern double g_dAvgPollerQueueSize;
extern double g_dAvgDBWriterQueueSize;
extern double g_dAvgStatusPollerQueueSize;
extern double g_dAvgConfigPollerQueueSize;
extern DWORD g_dwAvgDCIQueuingTime;


#endif   /* _nms_dcoll_h_ */
