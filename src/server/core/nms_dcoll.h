/* 
** Project X - Network Management System
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
** $module: nms_dcoll.h
**
**/

#ifndef _nms_dcoll_h_
#define _nms_dcoll_h_


//
// Constants
//

#define MAX_ITEM_NAME         256
#define MAX_STRING_VALUE      256


//
// Data collection errors
//

#define DCE_SUCCESS        0
#define DCE_COMM_ERROR     1
#define DCE_NOT_SUPPORTED  2


//
// Data types
//

#define DTYPE_INTEGER   0
#define DTYPE_INT64     1
#define DTYPE_STRING    2
#define DTYPE_FLOAT     3


//
// Data sources
//

#define DS_INTERNAL        0
#define DS_NATIVE_AGENT    1
#define DS_SNMP_AGENT      2


//
// Item status
//

#define ITEM_STATUS_ACTIVE          0
#define ITEM_STATUS_DISABLED        1
#define ITEM_STATUS_NOT_SUPPORTED   2


//
// Threshold functions and operations
//

#define F_LAST       0
#define F_AVERAGE    1
#define F_DEVIATION  2

#define OP_LE        0
#define OP_LE_EQ     1
#define OP_EQ        2
#define OP_GT_EQ     3
#define OP_GT        4
#define OP_NE        5
#define OP_LIKE      6
#define OP_NOTLIKE   7


//
// Threshold check results
//

#define THRESHOLD_REACHED  0
#define THRESHOLD_REARMED  1
#define NO_ACTION          2


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
   char *m_pszValueStr;
   union
   {
      long iInteger;
      __int64 qwInt64;
      double dFloat;
   } m_value;
   BYTE m_iFunction;          // Function code
   BYTE m_iOperation;         // Comparision operation code
   BYTE m_iDataType;          // Related item data type
   int m_iParam1;             // Function's parameter #1
   int m_iParam2;             // Function's parameter #2
   BOOL m_bIsReached;

public:
   Threshold(DCItem *pRelatedItem);
   Threshold(DB_RESULT hResult, int iRow, DCItem *pRelatedItem);
   ~Threshold();

   DWORD EventCode(void) { return m_dwEventCode; }
   const char *Value(void) { return m_pszValueStr; }
   BOOL IsReached(void) { return m_bIsReached; }

   BOOL SaveToDB(void);
   int Check(const char *pszValue);
};


//
// Data collection item class
//

class Node;

class DCItem
{
private:
   DWORD m_dwId;
   char m_szName[MAX_ITEM_NAME];
   time_t m_tLastPoll;       // Last poll time
   int m_iPollingInterval;   // Polling interval in seconds
   int m_iRetentionTime;     // Retention time in seconds
   BYTE m_iSource;           // SNMP or native agent?
   BYTE m_iDataType;
   BYTE m_iStatus;           // Item status: active, disabled or not supported
   BYTE m_iBusy;             // 1 when item is queued for polling, 0 if not
   DWORD m_dwNumThresholds;
   Threshold **m_ppThresholdList;
   Node *m_pNode;             // Pointer to node object this item related to

public:
   DCItem();
   DCItem(DB_RESULT hResult, int iRow, Node *pNode);
   DCItem(DWORD dwId, char *szName, int iSource, int iDataType, 
          int iPollingInterval, int iRetentionTime, Node *pNode);
   ~DCItem();

   BOOL SaveToDB(void);
   BOOL LoadThresholdsFromDB(void);

   DWORD Id(void) { return m_dwId; }
   int DataSource(void) { return m_iSource; }
   int DataType(void) { return m_iDataType; }
   const char *Name(void) { return m_szName; }

   BOOL ReadyForPolling(time_t currTime) 
   { 
      return ((m_iStatus == ITEM_STATUS_ACTIVE) && (!m_iBusy) &&
              (m_tLastPoll + m_iPollingInterval < currTime));
   }

   void SetLastPollTime(time_t tLastPoll);
   void SetStatus(int iStatus) { m_iStatus = (BYTE)iStatus; }
   void SetBusyFlag(BOOL bIsBusy) { m_iBusy = (BYTE)bIsBusy; }

   void CheckThresholds(const char *pszLastValue);
};


//
// Data collection item envelope structure
//

typedef struct
{
   DWORD dwNodeId;
   DCItem *pItem;
} DCI_ENVELOPE;


//
// Functions
//

BOOL InitDataCollector(void);
void DeleteAllItemsForNode(DWORD dwNodeId);


#endif   /* _nms_dcoll_h_ */
