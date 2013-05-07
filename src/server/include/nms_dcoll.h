/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Threshold check results
 */
#define THRESHOLD_REACHED  0
#define THRESHOLD_REARMED  1
#define NO_ACTION          2

/**
 * DCI value
 */
class NXCORE_EXPORTABLE ItemValue
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

   void setTimeStamp(DWORD dwTime) { m_dwTimeStamp = dwTime; }
   DWORD getTimeStamp() { return m_dwTimeStamp; }

   const TCHAR *getString() { return m_szString; }

   operator double() { return m_dFloat; }
   operator DWORD() { return m_dwInt32; }
   operator QWORD() { return m_qwInt64; }
   operator LONG() { return m_iInt32; }
   operator INT64() { return m_iInt64; }
   operator const TCHAR*() const { return m_szString; }

   const ItemValue& operator=(const ItemValue &src);
   const ItemValue& operator=(const TCHAR *pszStr);
   const ItemValue& operator=(double dFloat);
   const ItemValue& operator=(LONG iInt32);
   const ItemValue& operator=(INT64 iInt64);
   const ItemValue& operator=(DWORD dwInt32);
   const ItemValue& operator=(QWORD qwInt64);
};


class DCItem;

/**
 * Threshold definition class
 */
class NXCORE_EXPORTABLE Threshold
{
private:
   DWORD m_id;             // Unique threshold id
   DWORD m_itemId;         // Related item id
   DWORD m_eventCode;      // Event code to be generated
   DWORD m_rearmEventCode;
   ItemValue m_value;
   BYTE m_function;          // Function code
   BYTE m_operation;         // Comparision operation code
   BYTE m_dataType;          // Related item data type
	BYTE m_currentSeverity;   // Current everity (NORMAL if threshold is inactive)
   int m_param1;             // Function's parameter #1
   int m_param2;             // Function's parameter #2
   BOOL m_isReached;
	int m_numMatches;			// Number of consecutive matches
	int m_repeatInterval;		// -1 = default, 0 = off, >0 = seconds between repeats
	time_t m_lastEventTimestamp;

   const ItemValue& value() { return m_value; }
   void calculateAverageValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);
   void calculateSumValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);
   void calculateMDValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);
   void calculateDiff(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);

public:
	Threshold();
   Threshold(DCItem *pRelatedItem);
   Threshold(Threshold *pSrc);
   Threshold(DB_RESULT hResult, int iRow, DCItem *pRelatedItem);
	Threshold(ConfigEntry *config, DCItem *parentItem);
   ~Threshold();

   void bindToItem(DWORD dwItemId) { m_itemId = dwItemId; }

   DWORD getId() { return m_id; }
   DWORD getEventCode() { return m_eventCode; }
   DWORD getRearmEventCode() { return m_rearmEventCode; }
	int getFunction() { return m_function; }
	int getOperation() { return m_operation; }
	int getParam1() { return m_param1; }
	int getParam2() { return m_param2; }
   const TCHAR *getStringValue() { return m_value.getString(); }
   BOOL isReached() { return m_isReached; }
	
	int getRepeatInterval() { return m_repeatInterval; }
	time_t getLastEventTimestamp() { return m_lastEventTimestamp; }
	int getCurrentSeverity() { return m_currentSeverity; }
	void markLastEvent(int severity);

   BOOL saveToDB(DB_HANDLE hdb, DWORD dwIndex);
   int check(ItemValue &value, ItemValue **ppPrevValues, ItemValue &fvalue);
   int checkError(DWORD dwErrorCount);

   void createMessage(CSCPMessage *msg, DWORD baseId);
   void updateFromMessage(CSCPMessage *msg, DWORD baseId);

   void createId();
   DWORD getRequiredCacheSize() { return ((m_function == F_LAST) || (m_function == F_ERROR)) ? 0 : m_param1; }

   BOOL compare(Threshold *pThr);

   void createNXMPRecord(String &str, int index);

	void associate(DCItem *pItem);
	void setFunction(int nFunc) { m_function = nFunc; }
	void setOperation(int nOp) { m_operation = nOp; }
	void setEvent(DWORD dwEvent) { m_eventCode = dwEvent; }
	void setRearmEvent(DWORD dwEvent) { m_rearmEventCode = dwEvent; }
	void setParam1(int nVal) { m_param1 = nVal; }
	void setParam2(int nVal) { m_param2 = nVal; }
	void setValue(const TCHAR *value) { m_value = value; }
};

class Template;

/**
 * Generic data collection object
 */
class NXCORE_EXPORTABLE DCObject
{
protected:
   DWORD m_dwId;
   TCHAR m_szName[MAX_ITEM_NAME];
   TCHAR m_szDescription[MAX_DB_STRING];
	TCHAR m_systemTag[MAX_DB_STRING];
   time_t m_tLastPoll;           // Last poll time
   int m_iPollingInterval;       // Polling interval in seconds
   int m_iRetentionTime;         // Retention time in seconds
   BYTE m_source;                // origin: SNMP, agent, etc.
   BYTE m_status;                // Item status: active, disabled or not supported
   BYTE m_busy;                  // 1 when item is queued for polling, 0 if not
	BYTE m_scheduledForDeletion;  // 1 when item is scheduled for deletion, 0 if not
	WORD m_flags;
   DWORD m_dwTemplateId;         // Related template's id
   DWORD m_dwTemplateItemId;     // Related template item's id
   Template *m_pNode;             // Pointer to node or template object this item related to
   MUTEX m_hMutex;
   DWORD m_dwNumSchedules;
   TCHAR **m_ppScheduleList;
   time_t m_tLastCheck;          // Last schedule checking time
   DWORD m_dwErrorCount;         // Consequtive collection error count
	DWORD m_dwResourceId;	   	// Associated cluster resource ID
	DWORD m_dwProxyNode;          // Proxy node ID or 0 to disable
	WORD m_snmpPort;					// Custom SNMP port or 0 for node default
	TCHAR *m_pszPerfTabSettings;
   TCHAR *m_transformationScriptSource;   // Transformation script (source code)
   NXSL_Program *m_transformationScript;  // Compiled transformation script

   void lock() { MutexLock(m_hMutex); }
   void unlock() { MutexUnlock(m_hMutex); }

	BOOL loadCustomSchedules();

	bool matchClusterResource();
   bool matchSchedule(struct tm *pCurrTime, TCHAR *pszSchedule, BOOL *bWithSeconds, time_t currTimestamp);

	void expandMacros(const TCHAR *src, TCHAR *dst, size_t dstLen);

	virtual bool isCacheLoaded();

	// --- constructors ---
   DCObject();
   DCObject(const DCObject *src);
   DCObject(DWORD dwId, const TCHAR *szName, int iSource, int iPollingInterval, int iRetentionTime, Template *pNode,
            const TCHAR *pszDescription = NULL, const TCHAR *systemTag = NULL);
	DCObject(ConfigEntry *config, Template *owner);

public:
	virtual ~DCObject();

	virtual int getType() const { return DCO_TYPE_GENERIC; }

   virtual void updateFromTemplate(DCObject *dcObject);

   virtual BOOL saveToDB(DB_HANDLE hdb);
   virtual void deleteFromDB();
   virtual bool loadThresholdsFromDB();

   virtual void processNewValue(time_t nTimeStamp, void *value);
   virtual void processNewError();

	virtual bool hasValue();

	DWORD getId() { return m_dwId; }
   int getDataSource() { return m_source; }
   int getStatus() { return m_status; }
   const TCHAR *getName() { return m_szName; }
   const TCHAR *getDescription() { return m_szDescription; }
	const TCHAR *getSystemTag() { return m_systemTag; }
	const TCHAR *getPerfTabSettings() { return m_pszPerfTabSettings; }
   Template *getTarget() { return m_pNode; }
   DWORD getTemplateId() { return m_dwTemplateId; }
   DWORD getTemplateItemId() { return m_dwTemplateItemId; }
	DWORD getResourceId() { return m_dwResourceId; }
	DWORD getProxyNode() { return m_dwProxyNode; }
	time_t getLastPollTime() { return m_tLastPoll; }
	DWORD getErrorCount() { return m_dwErrorCount; }
	WORD getSnmpPort() { return m_snmpPort; }
   bool isShowOnObjectTooltip() { return (m_flags & DCF_SHOW_ON_OBJECT_TOOLTIP) ? true : false; }

   bool isReadyForPolling(time_t currTime);
	bool isScheduledForDeletion() { return m_scheduledForDeletion ? true : false; }
   void setLastPollTime(time_t tLastPoll) { m_tLastPoll = tLastPoll; }
   void setStatus(int status, bool generateEvent);
   void setBusyFlag(BOOL busy) { m_busy = (BYTE)busy; }
   void setTemplateId(DWORD dwTemplateId, DWORD dwItemId) 
         { m_dwTemplateId = dwTemplateId; m_dwTemplateItemId = dwItemId; }

   virtual void createMessage(CSCPMessage *pMsg);
   virtual void updateFromMessage(CSCPMessage *pMsg);

   virtual void changeBinding(DWORD dwNewId, Template *pNode, BOOL doMacroExpansion);

	virtual void deleteExpiredData();
	virtual bool deleteAllData();

   virtual void getEventList(DWORD **ppdwList, DWORD *pdwSize);
   virtual void createNXMPRecord(String &str);

	void setName(const TCHAR *pszName) { nx_strncpy(m_szName, pszName, MAX_ITEM_NAME); }
	void setDescription(const TCHAR *pszDescr) { nx_strncpy(m_szDescription, pszDescr, MAX_DB_STRING); }
	void setOrigin(int origin) { m_source = origin; }
	void setRetentionTime(int nTime) { m_iRetentionTime = nTime; }
	void setInterval(int nInt) { m_iPollingInterval = nInt; }
	void setAdvScheduleFlag(BOOL bFlag) { if (bFlag) m_flags |= DCF_ADVANCED_SCHEDULE; else m_flags &= ~DCF_ADVANCED_SCHEDULE; }
	void addSchedule(const TCHAR *pszSchedule);
   void setTransformationScript(const TCHAR *pszScript);

	bool prepareForDeletion();
};

/**
 * Data collection item class
 */
class NXCORE_EXPORTABLE DCItem : public DCObject
{
protected:
   TCHAR m_instance[MAX_DB_STRING];
   BYTE m_deltaCalculation;      // Delta calculation method
   BYTE m_dataType;
	int m_sampleCount;            // Number of samples required to calculate value
	ObjectArray<Threshold> *m_thresholds;
   DWORD m_dwCacheSize;          // Number of items in cache
   ItemValue **m_ppValueCache;
   ItemValue m_prevRawValue;     // Previous raw value (used for delta calculation)
   time_t m_tPrevValueTimeStamp;
   bool m_bCacheLoaded;
	int m_nBaseUnits;
	int m_nMultiplier;
	TCHAR *m_customUnitName;
	WORD m_snmpRawValueType;		// Actual SNMP raw value type for input transformation
	WORD m_instanceDiscoveryMethod;
	TCHAR *m_instanceDiscoveryData;
	TCHAR *m_instanceFilterSource;
	NXSL_Program *m_instanceFilter;

   void transform(ItemValue &value, time_t nElapsedTime);
   void checkThresholds(ItemValue &value);
   void clearCache();

	virtual bool isCacheLoaded();

public:
   DCItem();
   DCItem(const DCItem *pItem);
   DCItem(DB_RESULT hResult, int iRow, Template *pNode);
   DCItem(DWORD dwId, const TCHAR *szName, int iSource, int iDataType, 
          int iPollingInterval, int iRetentionTime, Template *pNode,
          const TCHAR *pszDescription = NULL, const TCHAR *systemTag = NULL);
	DCItem(ConfigEntry *config, Template *owner);
   virtual ~DCItem();

	virtual int getType() const { return DCO_TYPE_ITEM; }

   virtual void updateFromTemplate(DCObject *dcObject);

   virtual BOOL saveToDB(DB_HANDLE hdb);
   virtual void deleteFromDB();
   virtual bool loadThresholdsFromDB();

   void updateCacheSize(DWORD dwCondId = 0);

   int getDataType() { return m_dataType; }
	bool isInterpretSnmpRawValue() { return (m_flags & DCF_RAW_VALUE_OCTET_STRING) ? true : false; }
	WORD getSnmpRawValueType() { return m_snmpRawValueType; }
	bool hasActiveThreshold();
	WORD getInstanceDiscoveryMethod() { return m_instanceDiscoveryMethod; }
	const TCHAR *getInstanceDiscoveryData() { return m_instanceDiscoveryData; }
	NXSL_Program *getInstanceFilter() { return m_instanceFilter; }
	const TCHAR *getInstance() { return m_instance; }
	int getSampleCount() { return m_sampleCount; }

	void filterInstanceList(StringList *instances);
	void expandInstance();

   void processNewValue(time_t nTimeStamp, void *value);
   void processNewError();

	virtual bool hasValue();

   void getLastValue(CSCPMessage *pMsg, DWORD dwId);
   NXSL_Value *getValueForNXSL(int nFunction, int nPolls);

   virtual void createMessage(CSCPMessage *pMsg);
   void updateFromMessage(CSCPMessage *pMsg, DWORD *pdwNumMaps, DWORD **ppdwMapIndex, DWORD **ppdwMapId);
	void fillMessageWithThresholds(CSCPMessage *msg);

   virtual void changeBinding(DWORD dwNewId, Template *pNode, BOOL doMacroExpansion);

   virtual void deleteExpiredData();
	virtual bool deleteAllData();

   virtual void getEventList(DWORD **ppdwList, DWORD *pdwSize);
   virtual void createNXMPRecord(String &str);

	int getThresholdCount() const { return (m_thresholds != NULL) ? m_thresholds->size() : 0; }
	BOOL enumThresholds(BOOL (* pfCallback)(Threshold *, DWORD, void *), void *pArg);

	void setInstance(const TCHAR *instance) { nx_strncpy(m_instance, instance, MAX_DB_STRING); }
	void setDataType(int dataType) { m_dataType = dataType; }
	void setDeltaCalcMethod(int method) { m_deltaCalculation = method; }
	void setAllThresholdsFlag(BOOL bFlag) { if (bFlag) m_flags |= DCF_ALL_THRESHOLDS; else m_flags &= ~DCF_ALL_THRESHOLDS; }
	void addThreshold(Threshold *pThreshold);
	void deleteAllThresholds();
	void setInstanceDiscoveryMethod(WORD method) { m_instanceDiscoveryMethod = method; }
	void setInstanceDiscoveryData(const TCHAR *data) { safe_free(m_instanceDiscoveryData); m_instanceDiscoveryData = (data != NULL) ? _tcsdup(data) : NULL; }
   void setInstanceFilter(const TCHAR *pszScript);

	BOOL testTransformation(const TCHAR *script, const TCHAR *value, TCHAR *buffer, size_t bufSize);
};

/**
 * Table column definition
 */
class NXCORE_EXPORTABLE DCTableColumn
{
private:
	TCHAR m_name[MAX_COLUMN_NAME];
	int m_dataType;
	SNMP_ObjectId *m_snmpOid;

public:
	DCTableColumn(const DCTableColumn *src);
	DCTableColumn(CSCPMessage *msg, DWORD baseId);
	DCTableColumn(DB_RESULT hResult, int row);
	~DCTableColumn();

	const TCHAR *getName() { return m_name; }
	int getDataType() { return m_dataType; }
	SNMP_ObjectId *getSnmpOid() { return m_snmpOid; }
};


//
// Table column ID hash entry
//

struct TC_ID_MAP_ENTRY
{
	LONG id;
	TCHAR name[MAX_COLUMN_NAME];
};


//
// Tabular data collection object
//

class NXCORE_EXPORTABLE DCTable : public DCObject
{
protected:
	TCHAR m_instanceColumn[MAX_COLUMN_NAME];
	ObjectArray<DCTableColumn> *m_columns;
	Table *m_lastValue;

	static TC_ID_MAP_ENTRY *m_cache;
	static int m_cacheSize;
	static int m_cacheAllocated;
	static MUTEX m_cacheMutex;

public:
	DCTable();
   DCTable(const DCTable *src);
   DCTable(DWORD id, const TCHAR *name, int source, int pollingInterval, int retentionTime,
	        Template *node, const TCHAR *instanceColumn = NULL, const TCHAR *description = NULL, const TCHAR *systemTag = NULL);
   DCTable(DB_RESULT hResult, int iRow, Template *pNode);
	virtual ~DCTable();

	virtual int getType() const { return DCO_TYPE_TABLE; }

   virtual BOOL saveToDB(DB_HANDLE hdb);
   virtual void deleteFromDB();

   virtual void processNewValue(time_t nTimeStamp, void *value);
   virtual void processNewError();

   virtual void createMessage(CSCPMessage *pMsg);
   virtual void updateFromMessage(CSCPMessage *pMsg);

	virtual void deleteExpiredData();
	virtual bool deleteAllData();

	void getLastValue(CSCPMessage *msg);
   void getLastValueSummary(CSCPMessage *pMsg, DWORD dwId);

	LONG getInstanceColumnId();

	static LONG columnIdFromName(const TCHAR *name);
};


//
// Functions
//

BOOL InitDataCollector();
void DeleteAllItemsForNode(DWORD dwNodeId);
void WriteFullParamListToMessage(CSCPMessage *pMsg, WORD flags);

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
extern double g_dAvgIDataWriterQueueSize;
extern double g_dAvgDBAndIDataWriterQueueSize;
extern double g_dAvgStatusPollerQueueSize;
extern double g_dAvgConfigPollerQueueSize;
extern DWORD g_dwAvgDCIQueuingTime;


#endif   /* _nms_dcoll_h_ */
