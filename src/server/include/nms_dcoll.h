/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

/**
 * Instance discovery data
 */
class InstanceDiscoveryData
{
private:
   TCHAR *m_instance;
   UINT32 m_relatedObjectId;

public:
   InstanceDiscoveryData(const TCHAR *instance, UINT32 relatedObject)
   {
      m_instance = MemCopyString(instance);
      m_relatedObjectId = relatedObject;
   }
   ~InstanceDiscoveryData()
   {
      MemFree(m_instance);
   }

   const TCHAR *getInstance() const { return m_instance; }
   UINT32 getRelatedObject() const { return m_relatedObjectId; }
};

/**
 * Threshold check results
 */
enum class ThresholdCheckResult
{
   ACTIVATED = 0,
   DEACTIVATED = 1,
   ALREADY_ACTIVE = 2,
   ALREADY_INACTIVE = 3
};

/**
 * Statistic type
 */
enum class StatisticType
{
   CURRENT, MIN, MAX, AVERAGE
};

/**
 * DCI value
 */
class NXCORE_EXPORTABLE ItemValue
{
private:
   double m_double;
   INT32 m_int32;
   INT64 m_int64;
   UINT32 m_uint32;
   UINT64 m_uint64;
   TCHAR m_string[MAX_DB_STRING];
   time_t m_timestamp;

public:
   ItemValue();
   ItemValue(const TCHAR *value, time_t timestamp);
   ItemValue(const ItemValue *value);
   ~ItemValue();

   void setTimeStamp(time_t timestamp) { m_timestamp = timestamp; }
   time_t getTimeStamp() const { return m_timestamp; }

   INT32 getInt32() const { return m_int32; }
   UINT32 getUInt32() const { return m_uint32; }
   INT64 getInt64() const { return m_int64; }
   UINT64 getUInt64() const { return m_uint64; }
   double getDouble() const { return m_double; }
   const TCHAR *getString() const { return m_string; }

   operator double() const { return m_double; }
   operator UINT32() const { return m_uint32; }
   operator UINT64() const { return m_uint64; }
   operator INT32() const { return m_int32; }
   operator INT64() const { return m_int64; }
   operator const TCHAR*() const { return m_string; }

   const ItemValue& operator=(const ItemValue &src);
   const ItemValue& operator=(const TCHAR *value);
   const ItemValue& operator=(double value);
   const ItemValue& operator=(INT32 value);
   const ItemValue& operator=(INT64 value);
   const ItemValue& operator=(UINT32 value);
   const ItemValue& operator=(UINT64 value);
};


class DCItem;
class DataCollectionTarget;

/**
 * Threshold definition class
 */
class NXCORE_EXPORTABLE Threshold
{
private:
   UINT32 m_id;             // Unique threshold id
   UINT32 m_itemId;         // Parent item id
   UINT32 m_targetId;       // Parent data collection target ID
   UINT32 m_eventCode;      // Event code to be generated
   UINT32 m_rearmEventCode;
   ItemValue m_value;
   bool m_expandValue;
   ItemValue m_lastCheckValue;
   BYTE m_function;          // Function code
   BYTE m_operation;         // Comparision operation code
   BYTE m_dataType;          // Related item data type
	BYTE m_currentSeverity;   // Current everity (NORMAL if threshold is inactive)
   int m_sampleCount;        // Number of samples to calculate function on
   TCHAR *m_scriptSource;
   NXSL_Program *m_script;
   time_t m_lastScriptErrorReport;
   bool m_isReached;
   bool m_wasReachedBeforeMaint;
	int m_numMatches;			// Number of consecutive matches
	int m_repeatInterval;		// -1 = default, 0 = off, >0 = seconds between repeats
	time_t m_lastEventTimestamp;

   const ItemValue& value() { return m_value; }
   void calculateAverageValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);
   void calculateSumValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);
   void calculateMDValue(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);
   void calculateDiff(ItemValue *pResult, ItemValue &lastValue, ItemValue **ppPrevValues);
   void setScript(TCHAR *script);

public:
	Threshold();
   Threshold(DCItem *pRelatedItem);
   Threshold(Threshold *src, bool shadowCopy);
   Threshold(DB_RESULT hResult, int iRow, DCItem *pRelatedItem);
	Threshold(ConfigEntry *config, DCItem *parentItem);
   ~Threshold();

   void bindToItem(UINT32 itemId, UINT32 targetId) { m_itemId = itemId; m_targetId = targetId; }

   UINT32 getId() const { return m_id; }
   UINT32 getEventCode() const { return m_eventCode; }
   UINT32 getRearmEventCode() const { return m_rearmEventCode; }
	int getFunction() const { return m_function; }
	int getOperation() const { return m_operation; }
	int getSampleCount() const { return m_sampleCount; }
   const TCHAR *getStringValue() const { return m_value.getString(); }
   bool isReached() const { return m_isReached; }
   bool wasReachedBeforeMaintenance() const { return m_wasReachedBeforeMaint; }
   const ItemValue& getLastCheckValue() const { return m_lastCheckValue; }
	int getRepeatInterval() const { return m_repeatInterval; }
	time_t getLastEventTimestamp() const { return m_lastEventTimestamp; }
	int getCurrentSeverity() const { return m_currentSeverity; }

	void markLastEvent(int severity);
   void updateBeforeMaintenanceState() { m_wasReachedBeforeMaint = m_isReached; }
   void setLastCheckedValue(const ItemValue &value) { m_lastCheckValue = value; }

   BOOL saveToDB(DB_HANDLE hdb, UINT32 dwIndex);
   ThresholdCheckResult check(ItemValue &value, ItemValue **ppPrevValues, ItemValue &fvalue, NetObj *target, DCItem *dci);
   ThresholdCheckResult checkError(UINT32 dwErrorCount);

   void createMessage(NXCPMessage *msg, UINT32 baseId) const;
   void updateFromMessage(NXCPMessage *msg, UINT32 baseId);

   void createId();
   UINT32 getRequiredCacheSize() { return ((m_function == F_LAST) || (m_function == F_ERROR)) ? 0 : m_sampleCount; }

   bool equals(const Threshold *t) const;

   void createExportRecord(StringBuffer &xml, int index) const;
   json_t *toJson() const;

	void associate(DCItem *pItem);
	void setDataType(BYTE type) { m_dataType = type; }

	void reconcile(const Threshold *src);
};

class DataCollectionOwner;
class DCObjectInfo;

/**
 * DCObject storage class
 */
enum class DCObjectStorageClass
{
   DEFAULT = 0,
   BELOW_7 = 1,
   BELOW_30 = 2,
   BELOW_90 = 3,
   BELOW_180 = 4,
   OTHER = 5
};

/**
 * Generic data collection object
 */
class NXCORE_EXPORTABLE DCObject
{
protected:
   UINT32 m_id;
   uuid m_guid;
   TCHAR *m_name;
   TCHAR *m_description;
	TCHAR *m_systemTag;
   time_t m_tLastPoll;           // Last poll time
   int m_iPollingInterval;       // Polling interval in seconds
   int m_iRetentionTime;         // Retention time in days
   BYTE m_source;                // origin: SNMP, agent, etc.
   BYTE m_status;                // Item status: active, disabled or not supported
   BYTE m_busy;                  // 1 when item is queued for polling, 0 if not
	BYTE m_scheduledForDeletion;  // 1 when item is scheduled for deletion, 0 if not
	UINT16 m_flags;
   UINT32 m_dwTemplateId;         // Related template's id
   UINT32 m_dwTemplateItemId;     // Related template item's id
   DataCollectionOwner *m_owner;             // Pointer to node or template object this item related to
   MUTEX m_hMutex;
   StringList *m_schedules;
   time_t m_tLastCheck;          // Last schedule checking time
   UINT32 m_dwErrorCount;         // Consequtive collection error count
	UINT32 m_dwResourceId;	   	// Associated cluster resource ID
	UINT32 m_sourceNode;          // Source node ID or 0 to disable
	WORD m_snmpPort;					// Custom SNMP port or 0 for node default
	TCHAR *m_pszPerfTabSettings;
   TCHAR *m_transformationScriptSource;   // Transformation script (source code)
   NXSL_Program *m_transformationScript;  // Compiled transformation script
   time_t m_lastScriptErrorReport;
	TCHAR *m_comments;
	ClientSession *m_pollingSession;
   WORD m_instanceDiscoveryMethod;
   TCHAR *m_instanceDiscoveryData;
   TCHAR *m_instanceFilterSource;
   NXSL_Program *m_instanceFilter;
   TCHAR *m_instance;
   IntegerArray<UINT32> *m_accessList;
   time_t m_instanceGracePeriodStart;  // Start of grace period for missing instance
   INT32 m_instanceRetentionTime;      // Retention time if instance is not found
   time_t m_startTime;                 // Time to start data collection
   UINT32 m_relatedObject;

   void lock() { MutexLock(m_hMutex); }
   bool tryLock() { return MutexTryLock(m_hMutex); }
   void unlock() { MutexUnlock(m_hMutex); }

   bool loadAccessList(DB_HANDLE hdb);
	bool loadCustomSchedules(DB_HANDLE hdb);
   bool matchSchedule(const TCHAR *schedule, bool *withSeconds, struct tm *currLocalTime, time_t currTimestamp);

	void expandMacros(const TCHAR *src, TCHAR *dst, size_t dstLen);
	void expandMacrosAndReplace(TCHAR **str, size_t maxLen);
   void expandMacrosAndReplace(const TCHAR *src, TCHAR **dst, size_t maxLen);

   void setTransformationScript(const TCHAR *source);

	virtual bool isCacheLoaded();

	// --- constructors ---
   DCObject();
   DCObject(UINT32 dwId, const TCHAR *szName, int iSource, int iPollingInterval, int iRetentionTime, DataCollectionOwner *pNode,
            const TCHAR *pszDescription = NULL, const TCHAR *systemTag = NULL);
	DCObject(ConfigEntry *config, DataCollectionOwner *owner);
   DCObject(const DCObject *src, bool shadowCopy);

public:

	virtual ~DCObject();

   virtual DCObject *clone() const = 0;

	virtual int getType() const { return DCO_TYPE_GENERIC; }

   virtual void updateFromTemplate(DCObject *dcObject);
   virtual void updateFromImport(ConfigEntry *config);

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual void deleteFromDatabase();
   virtual bool loadThresholdsFromDB(DB_HANDLE hdb);

   virtual bool processNewValue(time_t nTimeStamp, const void *value, bool *updateStatus);
   void processNewError(bool noInstance);
   virtual void processNewError(bool noInstance, time_t now);
   virtual void updateThresholdsBeforeMaintenanceState();
   virtual void generateEventsBasedOnThrDiff();

	UINT32 getId() const { return m_id; }
	const uuid& getGuid() const { return m_guid; }
   int getDataSource() const { return m_source; }
   int getStatus() const { return m_status; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDescription() const { return m_description; }
	const TCHAR *getSystemTag() const { return CHECK_NULL_EX(m_systemTag); }
	const TCHAR *getPerfTabSettings() const { return m_pszPerfTabSettings; }
   int getPollingInterval() const { return m_iPollingInterval; }
   int getEffectivePollingInterval() const { return (m_iPollingInterval > 0) ? m_iPollingInterval : m_defaultPollingInterval; }
   DataCollectionOwner *getOwner() const { return m_owner; }
   UINT32 getOwnerId() const;
   const TCHAR *getOwnerName() const;
   UINT32 getTemplateId() const { return m_dwTemplateId; }
   UINT32 getTemplateItemId() const { return m_dwTemplateItemId; }
	UINT32 getResourceId() const { return m_dwResourceId; }
	UINT32 getSourceNode() const { return m_sourceNode; }
	time_t getLastPollTime() const { return m_tLastPoll; }
	UINT32 getErrorCount() const { return m_dwErrorCount; }
	WORD getSnmpPort() const { return m_snmpPort; }
   bool isShowOnObjectTooltip() const { return (m_flags & DCF_SHOW_ON_OBJECT_TOOLTIP) ? true : false; }
   bool isShowInObjectOverview() const { return (m_flags & DCF_SHOW_IN_OBJECT_OVERVIEW) ? true : false; }
   bool isAggregateOnCluster() const { return (m_flags & DCF_AGGREGATE_ON_CLUSTER) ? true : false; }
	bool isStatusDCO() const { return (m_flags & DCF_CALCULATE_NODE_STATUS) ? true : false; }
   bool isAggregateWithErrors() const { return (m_flags & DCF_AGGREGATE_WITH_ERRORS) ? true : false; }
   bool isAdvancedSchedule() const { return (m_flags & DCF_ADVANCED_SCHEDULE) ? true : false; }
   int getAggregationFunction() const { return DCF_GET_AGGREGATION_FUNCTION(m_flags); }
   int getRetentionTime() const { return m_iRetentionTime; }
   DCObjectStorageClass getStorageClass() const { return storageClassFromRetentionTime(m_iRetentionTime); }
   int getEffectiveRetentionTime() const { return (m_iRetentionTime > 0) ? m_iRetentionTime : m_defaultRetentionTime; }
   const TCHAR *getComments() const { return m_comments; }
   INT16 getAgentCacheMode();
   bool hasValue();
   bool hasAccess(UINT32 userId);
   UINT32 getRelatedObject() { return m_relatedObject; }

	bool matchClusterResource();
   bool isReadyForPolling(time_t currTime);
	bool isScheduledForDeletion() { return m_scheduledForDeletion ? true : false; }
   void setLastPollTime(time_t tLastPoll) { m_tLastPoll = tLastPoll; }
   void setStatus(int status, bool generateEvent);
   void setBusyFlag() { m_busy = 1; }
   void clearBusyFlag() { m_busy = 0; }
   void setTemplateId(UINT32 dwTemplateId, UINT32 dwItemId) { m_dwTemplateId = dwTemplateId; m_dwTemplateItemId = dwItemId; }

   virtual void createMessage(NXCPMessage *pMsg);
   virtual void updateFromMessage(NXCPMessage *pMsg);

   virtual void changeBinding(UINT32 dwNewId, DataCollectionOwner *newOwner, BOOL doMacroExpansion);

	virtual bool deleteAllData();
	virtual bool deleteEntry(time_t timestamp);

   virtual void getEventList(IntegerArray<UINT32> *eventList) = 0;
   virtual void createExportRecord(StringBuffer &xml) = 0;
   virtual json_t *toJson();

   NXSL_Value *createNXSLObject(NXSL_VM *vm);

   ClientSession *processForcePoll();
   void requestForcePoll(ClientSession *session);
   ClientSession *getPollingSession() { return m_pollingSession; }
	bool prepareForDeletion();

	WORD getInstanceDiscoveryMethod() const { return m_instanceDiscoveryMethod; }
   const TCHAR *getInstanceDiscoveryData() const { return m_instanceDiscoveryData; }
   INT32 getInstanceRetentionTime() const { return m_instanceRetentionTime; }
   StringObjectMap<InstanceDiscoveryData> *filterInstanceList(StringMap *instances);
   void setInstanceDiscoveryMethod(WORD method) { m_instanceDiscoveryMethod = method; }
   void setInstanceDiscoveryData(const TCHAR *data) { lock(); MemFree(m_instanceDiscoveryData); m_instanceDiscoveryData = MemCopyString(data); unlock(); }
   void setInstanceFilter(const TCHAR *pszScript);
   void setInstance(const TCHAR *instance) { lock(); MemFree(m_instance); m_instance = MemCopyString(instance); unlock(); }
   const TCHAR *getInstance() const { return CHECK_NULL_EX(m_instance); }
   void expandInstance();
   time_t getInstanceGracePeriodStart() const { return m_instanceGracePeriodStart; }
   void setInstanceGracePeriodStart(time_t t) { m_instanceGracePeriodStart = t; }
   void setRelatedObject(UINT32 relatedObject) { m_relatedObject = relatedObject; }

	static int m_defaultRetentionTime;
	static int m_defaultPollingInterval;

   static DCObjectStorageClass storageClassFromRetentionTime(int retentionTime);
   static const TCHAR *getStorageClassName(DCObjectStorageClass storageClass);
};

/**
 * Data collection item class
 */
class NXCORE_EXPORTABLE DCItem : public DCObject
{
protected:
   BYTE m_deltaCalculation;      // Delta calculation method
   BYTE m_dataType;
	int m_sampleCount;            // Number of samples required to calculate value
	ObjectArray<Threshold> *m_thresholds;
   UINT32 m_cacheSize;          // Number of items in cache
   UINT32 m_requiredCacheSize;
   ItemValue **m_ppValueCache;
   ItemValue m_prevRawValue;     // Previous raw value (used for delta calculation)
   time_t m_tPrevValueTimeStamp;
   bool m_bCacheLoaded;
	int m_nBaseUnits;
	int m_nMultiplier;
	TCHAR *m_customUnitName;
	WORD m_snmpRawValueType;		// Actual SNMP raw value type for input transformation
	TCHAR m_predictionEngine[MAX_NPE_NAME_LEN];

   bool transform(ItemValue &value, time_t nElapsedTime);
   void checkThresholds(ItemValue &value);
   void updateCacheSizeInternal(bool allowLoad, UINT32 conditionId = 0);
   void clearCache();

   bool hasScriptThresholds() const;
   Threshold *getThresholdById(UINT32 id) const;

	virtual bool isCacheLoaded();

   using DCObject::updateFromMessage;

public:
   DCItem(const DCItem *src, bool shadowCopy);
   DCItem(DB_HANDLE hdb, DB_RESULT hResult, int iRow, DataCollectionOwner *pNode, bool useStartupDelay);
   DCItem(UINT32 dwId, const TCHAR *szName, int iSource, int iDataType,
          int iPollingInterval, int iRetentionTime, DataCollectionOwner *pNode,
          const TCHAR *pszDescription = NULL, const TCHAR *systemTag = NULL);
	DCItem(ConfigEntry *config, DataCollectionOwner *owner);
   virtual ~DCItem();

   virtual DCObject *clone() const;

	virtual int getType() const override { return DCO_TYPE_ITEM; }

   virtual void updateFromTemplate(DCObject *dcObject) override;
   virtual void updateFromImport(ConfigEntry *config) override;

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual void deleteFromDatabase() override;
   virtual bool loadThresholdsFromDB(DB_HANDLE hdb) override;

   void updateCacheSize(UINT32 conditionId = 0) { lock(); updateCacheSizeInternal(true, conditionId); unlock(); }
   void reloadCache(bool forceReload);

   int getDataType() const { return m_dataType; }
   int getNXSLDataType() const;
   int getDeltaCalculationMethod() const { return m_deltaCalculation; }
	bool isInterpretSnmpRawValue() const { return (m_flags & DCF_RAW_VALUE_OCTET_STRING) ? true : false; }
	WORD getSnmpRawValueType() const { return m_snmpRawValueType; }
	bool hasActiveThreshold();
   int getThresholdSeverity();
	int getSampleCount() const { return m_sampleCount; }
	const TCHAR *getPredictionEngine() const { return m_predictionEngine; }

   virtual bool processNewValue(time_t nTimeStamp, const void *value, bool *updateStatus) override;
   virtual void processNewError(bool noInstance, time_t now) override;
   virtual void updateThresholdsBeforeMaintenanceState() override;
   virtual void generateEventsBasedOnThrDiff() override;

   void fillLastValueMessage(NXCPMessage *pMsg, UINT32 dwId);
   NXSL_Value *getValueForNXSL(NXSL_VM *vm, int nFunction, int nPolls);
   NXSL_Value *getRawValueForNXSL(NXSL_VM *vm);
   const TCHAR *getLastValue();
   ItemValue *getInternalLastValue();
   TCHAR *getAggregateValue(AggregationFunction func, time_t periodStart, time_t periodEnd);

   virtual void createMessage(NXCPMessage *msg) override;
   void updateFromMessage(NXCPMessage *msg, UINT32 *pdwNumMaps, UINT32 **ppdwMapIndex, UINT32 **ppdwMapId);
   void fillMessageWithThresholds(NXCPMessage *msg, bool activeOnly);

   virtual void changeBinding(UINT32 dwNewId, DataCollectionOwner *pNode, BOOL doMacroExpansion) override;

	virtual bool deleteAllData() override;
   virtual bool deleteEntry(time_t timestamp) override;

   virtual void getEventList(IntegerArray<UINT32> *eventList) override;
   virtual void createExportRecord(StringBuffer &str) override;
   virtual json_t *toJson() override;

	int getThresholdCount() const { return (m_thresholds != NULL) ? m_thresholds->size() : 0; }
	BOOL enumThresholds(BOOL (* pfCallback)(Threshold *, UINT32, void *), void *pArg);

	void setDataType(int dataType) { m_dataType = dataType; }
	void setDeltaCalculationMethod(int method) { m_deltaCalculation = method; }
	void setAllThresholdsFlag(BOOL bFlag) { if (bFlag) m_flags |= DCF_ALL_THRESHOLDS; else m_flags &= ~DCF_ALL_THRESHOLDS; }
	void addThreshold(Threshold *pThreshold);
	void deleteAllThresholds();

	void prepareForRecalc();
	void recalculateValue(ItemValue &value);

   static bool testTransformation(DataCollectionTarget *object, DCObjectInfo *dcObjectInfo, const TCHAR *script, const TCHAR *value, TCHAR *buffer, size_t bufSize);
};

struct TableThresholdCbData;

/**
 * Table column definition
 */
class NXCORE_EXPORTABLE DCTableColumn
{
private:
	TCHAR m_name[MAX_COLUMN_NAME];
   TCHAR *m_displayName;
	SNMP_ObjectId *m_snmpOid;
	UINT16 m_flags;

public:
	DCTableColumn(const DCTableColumn *src);
	DCTableColumn(NXCPMessage *msg, UINT32 baseId);
	DCTableColumn(DB_RESULT hResult, int row);
   DCTableColumn(ConfigEntry *e);
	~DCTableColumn();

	const TCHAR *getName() const { return m_name; }
   const TCHAR *getDisplayName() const { return (m_displayName != NULL) ? m_displayName : m_name; }
   UINT16 getFlags() const { return m_flags; }
   int getDataType() const { return TCF_GET_DATA_TYPE(m_flags); }
   int getAggregationFunction() const { return TCF_GET_AGGREGATION_FUNCTION(m_flags); }
	const SNMP_ObjectId *getSnmpOid() const { return m_snmpOid; }
   bool isInstanceColumn() const { return (m_flags & TCF_INSTANCE_COLUMN) != 0; }
   bool isConvertSnmpStringToHex() const { return (m_flags & TCF_SNMP_HEX_STRING) != 0; }

   void createExportRecord(StringBuffer &xml, int id) const;
   json_t *toJson() const;
};

/**
 * Table column ID hash entry
 */
struct TC_ID_MAP_ENTRY
{
	INT32 id;
	TCHAR name[MAX_COLUMN_NAME];
};

/**
 * Condition for table DCI threshold
 */
class NXCORE_EXPORTABLE DCTableCondition
{
private:
   TCHAR *m_column;
   int m_operation;
   ItemValue m_value;

public:
   DCTableCondition(const TCHAR *column, int operation, const TCHAR *value);
   DCTableCondition(DCTableCondition *src);
   ~DCTableCondition();

   bool check(Table *value, int row);

   const TCHAR *getColumn() const { return m_column; }
   int getOperation() const { return m_operation; }
   const TCHAR *getValue() const { return m_value.getString(); }

   json_t *toJson() const;

   bool equals(DCTableCondition *c) const;
};

/**
 * Condition group for table DCI threshold
 */
class NXCORE_EXPORTABLE DCTableConditionGroup
{
private:
   ObjectArray<DCTableCondition> *m_conditions;

public:
   DCTableConditionGroup();
   DCTableConditionGroup(NXCPMessage *msg, UINT32 *baseId);
   DCTableConditionGroup(DCTableConditionGroup *src);
   DCTableConditionGroup(ConfigEntry *e);
   ~DCTableConditionGroup();

   void addCondition(DCTableCondition *c) { m_conditions->add(c); }

   const ObjectArray<DCTableCondition> *getConditions() const { return m_conditions; }

   UINT32 fillMessage(NXCPMessage *msg, UINT32 baseId) const;
   json_t *toJson() const;

   bool equals(DCTableConditionGroup *g) const;

   bool check(Table *value, int row);
};

/**
 * Threshold instance
 */
class NXCORE_EXPORTABLE DCTableThresholdInstance
{
private:
   TCHAR *m_name;
   int m_matchCount;
   bool m_active;
   int m_row;

public:
   DCTableThresholdInstance(const TCHAR *name, int matchCount, bool active, int row);
   DCTableThresholdInstance(const DCTableThresholdInstance *src);
   ~DCTableThresholdInstance();

   const TCHAR *getName() const { return m_name; }
   int getMatchCount() const { return m_matchCount; }
   int getRow() const { return m_row; }
   bool isActive() const { return m_active; }
   void updateRow(int row) { m_row = row; }

   void incMatchCount() { m_matchCount++; }
   void setActive() { m_active = true; }
};

/**
 * Threshold definition for tabe DCI
 */
class NXCORE_EXPORTABLE DCTableThreshold
{
private:
   UINT32 m_id;
   ObjectArray<DCTableConditionGroup> *m_groups;
   UINT32 m_activationEvent;
   UINT32 m_deactivationEvent;
   int m_sampleCount;
   StringObjectMap<DCTableThresholdInstance> *m_instances;
   StringObjectMap<DCTableThresholdInstance> *m_instancesBeforeMaint;

   void loadConditions(DB_HANDLE hdb);
   void loadInstances(DB_HANDLE hdb);

public:
   DCTableThreshold();
   DCTableThreshold(DB_HANDLE hdb, DB_RESULT hResult, int row);
   DCTableThreshold(NXCPMessage *msg, UINT32 *baseId);
   DCTableThreshold(DCTableThreshold *src, bool shadowCopy);
   DCTableThreshold(ConfigEntry *e);
   ~DCTableThreshold();

   void copyState(DCTableThreshold *src);

   ThresholdCheckResult check(Table *value, int row, const TCHAR *instance);
   void updateBeforeMaintenanceState();
   void generateEventsBasedOnThrDiff(TableThresholdCbData *data);
   DCTableThresholdInstance *findInstance(const TCHAR *instance, bool originalList);

   bool saveToDatabase(DB_HANDLE hdb, UINT32 tableId, int seq);
   UINT32 fillMessage(NXCPMessage *msg, UINT32 baseId);

   void createExportRecord(StringBuffer &xml, int id) const;
   json_t *toJson() const;

   bool equals(const DCTableThreshold *t) const;

   UINT32 getId() const { return m_id; }
   UINT32 getActivationEvent() const { return m_activationEvent; }
   UINT32 getDeactivationEvent() const { return m_deactivationEvent; }
   int getSampleCount() const { return m_sampleCount; }
};

/**
 * Tabular data collection object
 */
class NXCORE_EXPORTABLE DCTable : public DCObject
{
protected:
	ObjectArray<DCTableColumn> *m_columns;
   ObjectArray<DCTableThreshold> *m_thresholds;
	Table *m_lastValue;

	static TC_ID_MAP_ENTRY *m_cache;
	static int m_cacheSize;
	static int m_cacheAllocated;
	static MUTEX m_cacheMutex;

   bool transform(Table *value);
   void checkThresholds(Table *value);

   bool loadThresholds(DB_HANDLE hdb);
   bool saveThresholds(DB_HANDLE hdb);

public:
   DCTable(const DCTable *src, bool shadowCopy);
   DCTable(UINT32 id, const TCHAR *name, int source, int pollingInterval, int retentionTime,
         DataCollectionOwner *node, const TCHAR *description = NULL, const TCHAR *systemTag = NULL);
   DCTable(DB_HANDLE hdb, DB_RESULT hResult, int iRow, DataCollectionOwner *pNode, bool useStartupDelay);
   DCTable(ConfigEntry *config, DataCollectionOwner *owner);
	virtual ~DCTable();

	virtual DCObject *clone() const override;

	virtual int getType() const override { return DCO_TYPE_TABLE; }

   virtual void updateFromTemplate(DCObject *dcObject) override;
   virtual void updateFromImport(ConfigEntry *config) override;

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual void deleteFromDatabase() override;

   virtual bool processNewValue(time_t nTimeStamp, const void *value, bool *updateStatus) override;
   virtual void processNewError(bool noInstance, time_t now) override;
   virtual void updateThresholdsBeforeMaintenanceState() override;
   virtual void generateEventsBasedOnThrDiff() override;

   virtual void createMessage(NXCPMessage *msg) override;
   virtual void updateFromMessage(NXCPMessage *msg) override;

	virtual bool deleteAllData() override;
   virtual bool deleteEntry(time_t timestamp) override;

   virtual void getEventList(IntegerArray<UINT32> *eventList) override;
   virtual void createExportRecord(StringBuffer &xml) override;
   virtual json_t *toJson() override;

	void fillLastValueMessage(NXCPMessage *msg);
   void fillLastValueSummaryMessage(NXCPMessage *pMsg, UINT32 dwId);

   int getColumnDataType(const TCHAR *name);
   ObjectArray<DCTableColumn> *getColumns() { return m_columns; }
   Table *getLastValue();
   IntegerArray<UINT32> *getThresholdIdList();

   void mergeValues(Table *dest, Table *src, int count);

   void updateResultColumns(Table *t);

	static INT32 columnIdFromName(const TCHAR *name);
};

/**
 * Callback data for after maintenance event generation
 */
struct TableThresholdCbData
{
   DCTableThreshold *threshold;
   DCTable *table;
   bool originalList;
};

/**
 * Data collection object information (for NXSL)
 */
class DCObjectInfo
{
private:
   UINT32 m_id;
   UINT32 m_ownerId;
   UINT32 m_templateId;
   UINT32 m_templateItemId;
   int m_type;
   TCHAR m_name[MAX_ITEM_NAME];
   TCHAR m_description[MAX_DB_STRING];
   TCHAR m_systemTag[MAX_DB_STRING];
   TCHAR m_instance[MAX_DB_STRING];
   TCHAR *m_instanceData;
   TCHAR *m_comments;
   int m_dataType;
   int m_origin;
   int m_status;
   UINT32 m_errorCount;
   INT32 m_pollingInterval;
   time_t m_lastPollTime;
   bool m_hasActiveThreshold;
   int m_thresholdSeverity;
   UINT32 m_relatedObject;

public:
   DCObjectInfo(DCObject *object);
   DCObjectInfo(const NXCPMessage *msg, DCObject *object);
   ~DCObjectInfo();

   UINT32 getId() const { return m_id; }
   UINT32 getTemplateId() const { return m_templateId; }
   UINT32 getTemplateItemId() const { return m_templateItemId; }
   int getType() const { return m_type; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDescription() const { return m_description; }
   const TCHAR *getSystemTag() const { return m_systemTag; }
   const TCHAR *getInstance() const { return m_instance; }
   const TCHAR *getInstanceData() const { return m_instanceData; }
   const TCHAR *getComments() const { return m_comments; }
   int getDataType() const { return m_dataType; }
   int getOrigin() const { return m_origin; }
   int getStatus() const { return m_status; }
   UINT32 getErrorCount() const { return m_errorCount; }
   INT32 getPollingInterval() const { return m_pollingInterval; }
   time_t getLastPollTime() const { return m_lastPollTime; }
   UINT32 getOwnerId() const { return m_ownerId; }
   bool hasActiveThreshold() const { return m_hasActiveThreshold; }
   int getThresholdSeverity() const { return m_thresholdSeverity; }
   UINT32 getRelatedObject() const { return m_relatedObject; }
};

/**
 * Functions
 */
void InitDataCollector();
void DeleteAllItemsForNode(UINT32 dwNodeId);
void WriteFullParamListToMessage(NXCPMessage *pMsg, int origin, WORD flags);
int GetDCObjectType(UINT32 nodeId, UINT32 dciId);

void CalculateItemValueDiff(ItemValue &result, int nDataType, const ItemValue &value1, const ItemValue &value2);
void CalculateItemValueAverage(ItemValue &result, int nDataType, const ItemValue * const *valueList, size_t numValues);
void CalculateItemValueMD(ItemValue &result, int nDataType, const ItemValue * const *valueList, size_t numValues);
void CalculateItemValueTotal(ItemValue &result, int nDataType, const ItemValue *const *valueList, size_t numValues);
void CalculateItemValueMin(ItemValue &result, int nDataType, const ItemValue *const *valueList, size_t numValues);
void CalculateItemValueMax(ItemValue &result, int nDataType, const ItemValue *const *valueList, size_t numValues);

DataCollectionError GetQueueStatistic(const TCHAR *parameter, StatisticType type, TCHAR *value);

/**
 * DCI cache loader queue
 */
extern ObjectQueue<DCObjectInfo> g_dciCacheLoaderQueue;

#endif   /* _nms_dcoll_h_ */
