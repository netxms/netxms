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
** File: netobj.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
NetObj::NetObj()
{
   int i;

   m_dwRefCount = 0;
   m_mutexData = MutexCreate();
   m_mutexRefCount = MutexCreate();
   m_mutexACL = MutexCreate();
   m_rwlockParentList = RWLockCreate();
   m_rwlockChildList = RWLockCreate();
   m_iStatus = STATUS_UNKNOWN;
   m_szName[0] = 0;
   m_pszComments = NULL;
   m_isModified = false;
   m_isDeleted = false;
   m_isHidden = false;
	m_isSystem = false;
   m_dwIpAddr = 0;
   m_dwChildCount = 0;
   m_pChildList = NULL;
   m_dwParentCount = 0;
   m_pParentList = NULL;
   m_pAccessList = new AccessList;
   m_bInheritAccessRights = TRUE;
	m_dwNumTrustedNodes = 0;
	m_pdwTrustedNodes = NULL;
   m_pollRequestor = NULL;
   m_iStatusCalcAlg = SA_CALCULATE_DEFAULT;
   m_iStatusPropAlg = SA_PROPAGATE_DEFAULT;
   m_iFixedStatus = STATUS_WARNING;
   m_iStatusShift = 0;
   m_iStatusSingleThreshold = 75;
   m_dwTimeStamp = 0;
   for(i = 0; i < 4; i++)
   {
      m_iStatusTranslation[i] = i + 1;
      m_iStatusThresholds[i] = 80 - i * 20;
   }
	uuid_clear(m_image);
	m_submapId = 0;
}

/**
 * Destructor
 */
NetObj::~NetObj()
{
   MutexDestroy(m_mutexData);
   MutexDestroy(m_mutexRefCount);
   MutexDestroy(m_mutexACL);
   RWLockDestroy(m_rwlockParentList);
   RWLockDestroy(m_rwlockChildList);
   safe_free(m_pChildList);
   safe_free(m_pParentList);
   delete m_pAccessList;
	safe_free(m_pdwTrustedNodes);
   safe_free(m_pszComments);
}

/**
 * Create object from database data
 */
BOOL NetObj::CreateFromDB(UINT32 dwId)
{
   return FALSE;     // Abstract objects cannot be loaded from database
}

/**
 * Save object to database
 */
BOOL NetObj::SaveToDB(DB_HANDLE hdb)
{
   return FALSE;     // Abstract objects cannot be saved to database
}

/**
 * Delete object from database
 */
bool NetObj::deleteFromDB(DB_HANDLE hdb)
{
   // Delete ACL
   bool success = executeQueryOnObject(hdb, _T("DELETE FROM acl WHERE object_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_properties WHERE object_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_custom_attributes WHERE object_id=?"));

   // Delete events
   if (success && ConfigReadInt(_T("DeleteEventsOfDeletedObject"), 1))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM event_log WHERE event_source=?"));
   }

   // Delete alarms
   if (success && ConfigReadInt(_T("DeleteAlarmsOfDeletedObject"), 1))
   {
      success = g_alarmMgr.deleteObjectAlarms(m_dwId, hdb);
   }

   return success;
}

/**
 * Prepare and execute SQL query with single binding - object ID.
 */
bool NetObj::executeQueryOnObject(DB_HANDLE hdb, const TCHAR *query)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == NULL)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
   bool success = DBExecute(hStmt) ? true : false;
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Load common object properties from database
 */
BOOL NetObj::loadCommonProperties()
{
   BOOL bResult = FALSE;

   // Load access options
	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB,
	                          _T("SELECT name,status,is_deleted,")
                             _T("inherit_access_rights,last_modified,status_calc_alg,")
                             _T("status_prop_alg,status_fixed_val,status_shift,")
                             _T("status_translation,status_single_threshold,")
                             _T("status_thresholds,comments,is_system,")
									  _T("location_type,latitude,longitude,location_accuracy,")
									  _T("location_timestamp,guid,image,submap_id FROM object_properties ")
                             _T("WHERE object_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, m_szName, MAX_OBJECT_NAME);
				m_iStatus = DBGetFieldLong(hResult, 0, 1);
				m_isDeleted = DBGetFieldLong(hResult, 0, 2) ? TRUE : FALSE;
				m_bInheritAccessRights = DBGetFieldLong(hResult, 0, 3) ? TRUE : FALSE;
				m_dwTimeStamp = DBGetFieldULong(hResult, 0, 4);
				m_iStatusCalcAlg = DBGetFieldLong(hResult, 0, 5);
				m_iStatusPropAlg = DBGetFieldLong(hResult, 0, 6);
				m_iFixedStatus = DBGetFieldLong(hResult, 0, 7);
				m_iStatusShift = DBGetFieldLong(hResult, 0, 8);
				DBGetFieldByteArray(hResult, 0, 9, m_iStatusTranslation, 4, STATUS_WARNING);
				m_iStatusSingleThreshold = DBGetFieldLong(hResult, 0, 10);
				DBGetFieldByteArray(hResult, 0, 11, m_iStatusThresholds, 4, 50);
				safe_free(m_pszComments);
				m_pszComments = DBGetField(hResult, 0, 12, NULL, 0);
				m_isSystem = DBGetFieldLong(hResult, 0, 13) ? true : false;

				int locType = DBGetFieldLong(hResult, 0, 14);
				if (locType != GL_UNSET)
				{
					TCHAR lat[32], lon[32];

					DBGetField(hResult, 0, 15, lat, 32);
					DBGetField(hResult, 0, 16, lon, 32);
					m_geoLocation = GeoLocation(locType, lat, lon, DBGetFieldLong(hResult, 0, 17), DBGetFieldULong(hResult, 0, 18));
				}
				else
				{
					m_geoLocation = GeoLocation();
				}

				DBGetFieldGUID(hResult, 0, 19, m_guid);
				DBGetFieldGUID(hResult, 0, 20, m_image);
				m_submapId = DBGetFieldULong(hResult, 0, 21);

				bResult = TRUE;
			}
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

	// Load custom attributes
	if (bResult)
	{
		hStmt = DBPrepare(g_hCoreDB, _T("SELECT attr_name,attr_value FROM object_custom_attributes WHERE object_id=?"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != NULL)
			{
				int i, count;
				TCHAR *name, *value;

				count = DBGetNumRows(hResult);
				for(i = 0; i < count; i++)
				{
		   		name = DBGetField(hResult, i, 0, NULL, 0);
		   		if (name != NULL)
		   		{
						value = DBGetField(hResult, i, 1, NULL, 0);
						if (value != NULL)
						{
							m_customAttributes.setPreallocated(name, value);
						}
					}
				}
				DBFreeResult(hResult);
			}
			else
			{
				bResult = FALSE;
			}
			DBFreeStatement(hStmt);
		}
		else
		{
			bResult = FALSE;
		}
	}

	if (bResult)
		bResult = loadTrustedNodes();

	if (!bResult)
		DbgPrintf(4, _T("NetObj::loadCommonProperties() failed for object %s [%ld] class=%d"), m_szName, (long)m_dwId, Type());

   return bResult;
}

/**
 * Save common object properties to database
 */
BOOL NetObj::saveCommonProperties(DB_HANDLE hdb)
{
	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("object_properties"), _T("object_id"), m_dwId))
	{
		hStmt = DBPrepare(hdb,
                    _T("UPDATE object_properties SET name=?,status=?,")
                    _T("is_deleted=?,inherit_access_rights=?,")
                    _T("last_modified=?,status_calc_alg=?,status_prop_alg=?,")
                    _T("status_fixed_val=?,status_shift=?,status_translation=?,")
                    _T("status_single_threshold=?,status_thresholds=?,")
                    _T("comments=?,is_system=?,location_type=?,latitude=?,")
						  _T("longitude=?,location_accuracy=?,location_timestamp=?,")
						  _T("guid=?,image=?,submap_id=? WHERE object_id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb,
                    _T("INSERT INTO object_properties (name,status,is_deleted,")
                    _T("inherit_access_rights,last_modified,status_calc_alg,")
                    _T("status_prop_alg,status_fixed_val,status_shift,status_translation,")
                    _T("status_single_threshold,status_thresholds,comments,is_system,")
						  _T("location_type,latitude,longitude,location_accuracy,location_timestamp,")
						  _T("guid,image,submap_id,object_id) ")
                    _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	if (hStmt == NULL)
		return FALSE;

   TCHAR szTranslation[16], szThresholds[16], lat[32], lon[32], guid[64], image[64];
   for(int i = 0, j = 0; i < 4; i++, j += 2)
   {
      _sntprintf(&szTranslation[j], 16 - j, _T("%02X"), (BYTE)m_iStatusTranslation[i]);
      _sntprintf(&szThresholds[j], 16 - j, _T("%02X"), (BYTE)m_iStatusThresholds[i]);
   }
	_sntprintf(lat, 32, _T("%f"), m_geoLocation.getLatitude());
	_sntprintf(lon, 32, _T("%f"), m_geoLocation.getLongitude());

	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_szName, DB_BIND_STATIC);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_iStatus);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)(m_isDeleted ? 1 : 0));
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)m_bInheritAccessRights);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)m_dwTimeStamp);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_iStatusCalcAlg);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (LONG)m_iStatusPropAlg);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (LONG)m_iFixedStatus);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (LONG)m_iStatusShift);
	DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, szTranslation, DB_BIND_STATIC);
	DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, (LONG)m_iStatusSingleThreshold);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, szThresholds, DB_BIND_STATIC);
	DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, m_pszComments, DB_BIND_STATIC);
   DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, (LONG)(m_isSystem ? 1 : 0));
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, (LONG)m_geoLocation.getType());
	DBBind(hStmt, 16, DB_SQLTYPE_VARCHAR, lat, DB_BIND_STATIC);
	DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, lon, DB_BIND_STATIC);
	DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, (LONG)m_geoLocation.getAccuracy());
	DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, (UINT32)m_geoLocation.getTimestamp());
	DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, uuid_to_string(m_guid, guid), DB_BIND_STATIC);
	DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, uuid_to_string(m_image, image), DB_BIND_STATIC);
	DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, m_submapId);
	DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, m_dwId);

	BOOL bResult = DBExecute(hStmt);
	DBFreeStatement(hStmt);

   // Save custom attributes
   if (bResult)
   {
		TCHAR szQuery[512];
		_sntprintf(szQuery, 512, _T("DELETE FROM object_custom_attributes WHERE object_id=%d"), m_dwId);
		bResult = DBQuery(hdb, szQuery);
		if (bResult)
		{
			hStmt = DBPrepare(hdb, _T("INSERT INTO object_custom_attributes (object_id,attr_name,attr_value) VALUES (?,?,?)"));
			if (hStmt != NULL)
			{
				for(UINT32 i = 0; i < m_customAttributes.getSize(); i++)
				{
					DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
					DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_customAttributes.getKeyByIndex(i), DB_BIND_STATIC);
					DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_customAttributes.getValueByIndex(i), DB_BIND_STATIC);
					bResult = DBExecute(hStmt);
					if (!bResult)
						break;
				}
				DBFreeStatement(hStmt);
			}
			else
			{
				bResult = FALSE;
			}
		}
   }

	if (bResult)
		bResult = saveTrustedNodes(hdb);

   return bResult;
}

/**
 * Add reference to the new child object
 */
void NetObj::AddChild(NetObj *pObject)
{
   UINT32 i;

   LockChildList(TRUE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i] == pObject)
      {
         UnlockChildList();
         return;     // Already in the child list
      }
   m_pChildList = (NetObj **)realloc(m_pChildList, sizeof(NetObj *) * (m_dwChildCount + 1));
   m_pChildList[m_dwChildCount++] = pObject;
   UnlockChildList();
	incRefCount();
   Modify();
}

/**
 * Add reference to parent object
 */
void NetObj::AddParent(NetObj *pObject)
{
   UINT32 i;

   LockParentList(TRUE);
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i] == pObject)
      {
         UnlockParentList();
         return;     // Already in the parents list
      }
   m_pParentList = (NetObj **)realloc(m_pParentList, sizeof(NetObj *) * (m_dwParentCount + 1));
   m_pParentList[m_dwParentCount++] = pObject;
   UnlockParentList();
	incRefCount();
   Modify();
}

/**
 * Delete reference to child object
 */
void NetObj::DeleteChild(NetObj *pObject)
{
   UINT32 i;

   LockChildList(TRUE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i] == pObject)
         break;

   if (i == m_dwChildCount)   // No such object
   {
      UnlockChildList();
      return;
   }
   m_dwChildCount--;
   if (m_dwChildCount > 0)
   {
      memmove(&m_pChildList[i], &m_pChildList[i + 1], sizeof(NetObj *) * (m_dwChildCount - i));
      m_pChildList = (NetObj **)realloc(m_pChildList, sizeof(NetObj *) * m_dwChildCount);
   }
   else
   {
      free(m_pChildList);
      m_pChildList = NULL;
   }
   UnlockChildList();
	decRefCount();
   Modify();
}

/**
 * Delete reference to parent object
 */
void NetObj::DeleteParent(NetObj *pObject)
{
   UINT32 i;

   LockParentList(TRUE);
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i] == pObject)
         break;
   if (i == m_dwParentCount)   // No such object
   {
      UnlockParentList();
      return;
   }
   m_dwParentCount--;
   if (m_dwParentCount > 0)
   {
      memmove(&m_pParentList[i], &m_pParentList[i + 1], sizeof(NetObj *) * (m_dwParentCount - i));
      m_pParentList = (NetObj **)realloc(m_pParentList, sizeof(NetObj *) * m_dwParentCount);
   }
   else
   {
      free(m_pParentList);
      m_pParentList = NULL;
   }
   UnlockParentList();
	decRefCount();
   Modify();
}

/**
 * Walker callback to call OnObjectDelete for each active object
 */
void NetObj::onObjectDeleteCallback(NetObj *object, void *data)
{
	UINT32 currId = ((NetObj *)data)->Id();
	if ((object->Id() != currId) && !object->isDeleted())
		object->onObjectDelete(currId);
}

/**
 * Prepare object for deletion - remove all references, etc.
 */
void NetObj::deleteObject()
{
   UINT32 i;

   DbgPrintf(4, _T("Deleting object %d [%s]"), m_dwId, m_szName);

	// Prevent object change propagation until it's marked as deleted
	// (to prevent the object's re-appearance in GUI if client hides the object
	// after successful call to Session::deleteObject())
	LockData();
   m_isHidden = true;
	UnlockData();

	// Notify modules about object deletion
	for(UINT32 i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].pfPreObjectDelete != NULL)
			g_pModuleList[i].pfPreObjectDelete(this);
	}

   prepareForDeletion();

   // Remove references to this object from parent objects
   DbgPrintf(5, _T("NetObj::Delete(): clearing parent list for object %d"), m_dwId);
   LockParentList(TRUE);
   for(i = 0; i < m_dwParentCount; i++)
   {
      m_pParentList[i]->DeleteChild(this);
      m_pParentList[i]->calculateCompoundStatus();
		decRefCount();
   }
   free(m_pParentList);
   m_pParentList = NULL;
   m_dwParentCount = 0;
   UnlockParentList();

   // Delete references to this object from child objects
   DbgPrintf(5, _T("NetObj::Delete(): clearing child list for object %d"), m_dwId);
   LockChildList(TRUE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      m_pChildList[i]->DeleteParent(this);
		decRefCount();
      if (m_pChildList[i]->isOrphaned())
			m_pChildList[i]->deleteObject();
   }
   free(m_pChildList);
   m_pChildList = NULL;
   m_dwChildCount = 0;
   UnlockChildList();

   LockData();
   m_isHidden = false;
   m_isDeleted = true;
   Modify();
   UnlockData();

   DbgPrintf(5, _T("NetObj::Delete(): deleting object %d from indexes"), m_dwId);
   NetObjDeleteFromIndexes(this);

   // Notify all other objects about object deletion
   DbgPrintf(5, _T("NetObj::Delete(): calling OnObjectDelete(%d)"), m_dwId);
	g_idxObjectById.forEach(onObjectDeleteCallback, this);

   DbgPrintf(4, _T("Object %d successfully deleted"), m_dwId);
}

/**
 * Default handler for object deletion notification
 */
void NetObj::onObjectDelete(UINT32 dwObjectId)
{
   PostEvent(EVENT_SUBNET_DELETED, dwObjectId, NULL);
}

/**
 * Get childs IDs in printable form
 */
const TCHAR *NetObj::dbgGetChildList(TCHAR *szBuffer)
{
   UINT32 i;
   TCHAR *pBuf = szBuffer;

   *pBuf = 0;
   LockChildList(FALSE);
   for(i = 0, pBuf = szBuffer; i < m_dwChildCount; i++)
   {
      _sntprintf(pBuf, 10, _T("%d "), m_pChildList[i]->Id());
      while(*pBuf)
         pBuf++;
   }
   UnlockChildList();
   if (pBuf != szBuffer)
      *(pBuf - 1) = 0;
   return szBuffer;
}

/**
 * Get parents IDs in printable form
 */
const TCHAR *NetObj::dbgGetParentList(TCHAR *szBuffer)
{
   UINT32 i;
   TCHAR *pBuf = szBuffer;

   *pBuf = 0;
   LockParentList(FALSE);
   for(i = 0; i < m_dwParentCount; i++)
   {
      _sntprintf(pBuf, 10, _T("%d "), m_pParentList[i]->Id());
      while(*pBuf)
         pBuf++;
   }
   UnlockParentList();
   if (pBuf != szBuffer)
      *(pBuf - 1) = 0;
   return szBuffer;
}

/**
 * Calculate status for compound object based on childs' status
 */
void NetObj::calculateCompoundStatus(BOOL bForcedRecalc)
{
   UINT32 i;
   int iMostCriticalAlarm, iMostCriticalStatus, iCount, iStatusAlg;
   int nSingleThreshold, *pnThresholds, iOldStatus = m_iStatus;
   int nRating[5], iChildStatus, nThresholds[4];

   if (m_iStatus != STATUS_UNMANAGED)
   {
      iMostCriticalAlarm = g_alarmMgr.getMostCriticalStatusForObject(m_dwId);

      LockData();
      if (m_iStatusCalcAlg == SA_CALCULATE_DEFAULT)
      {
         iStatusAlg = GetDefaultStatusCalculation(&nSingleThreshold, &pnThresholds);
      }
      else
      {
         iStatusAlg = m_iStatusCalcAlg;
         nSingleThreshold = m_iStatusSingleThreshold;
         pnThresholds = m_iStatusThresholds;
      }
      if (iStatusAlg == SA_CALCULATE_SINGLE_THRESHOLD)
      {
         for(i = 0; i < 4; i++)
            nThresholds[i] = nSingleThreshold;
         pnThresholds = nThresholds;
      }

      switch(iStatusAlg)
      {
         case SA_CALCULATE_MOST_CRITICAL:
            LockChildList(FALSE);
            for(i = 0, iCount = 0, iMostCriticalStatus = -1; i < m_dwChildCount; i++)
            {
               iChildStatus = m_pChildList[i]->getPropagatedStatus();
               if ((iChildStatus < STATUS_UNKNOWN) &&
                   (iChildStatus > iMostCriticalStatus))
               {
                  iMostCriticalStatus = iChildStatus;
                  iCount++;
               }
            }
            m_iStatus = (iCount > 0) ? iMostCriticalStatus : STATUS_UNKNOWN;
            UnlockChildList();
            break;
         case SA_CALCULATE_SINGLE_THRESHOLD:
         case SA_CALCULATE_MULTIPLE_THRESHOLDS:
            // Step 1: calculate severity raitings
            memset(nRating, 0, sizeof(int) * 5);
            LockChildList(FALSE);
            for(i = 0, iCount = 0; i < m_dwChildCount; i++)
            {
               iChildStatus = m_pChildList[i]->getPropagatedStatus();
               if (iChildStatus < STATUS_UNKNOWN)
               {
                  while(iChildStatus >= 0)
                     nRating[iChildStatus--]++;
                  iCount++;
               }
            }
            UnlockChildList();

            // Step 2: check what severity rating is above threshold
            if (iCount > 0)
            {
               for(i = 4; i > 0; i--)
                  if (nRating[i] * 100 / iCount >= pnThresholds[i - 1])
                     break;
               m_iStatus = i;
            }
            else
            {
               m_iStatus = STATUS_UNKNOWN;
            }
            break;
         default:
            m_iStatus = STATUS_UNKNOWN;
            break;
      }

      // If alarms exist for object, apply alarm severity to object's status
      if (iMostCriticalAlarm != STATUS_UNKNOWN)
      {
         if (m_iStatus == STATUS_UNKNOWN)
         {
            m_iStatus = iMostCriticalAlarm;
         }
         else
         {
            m_iStatus = max(m_iStatus, iMostCriticalAlarm);
         }
      }

      // Query loaded modules for object status
      for(i = 0; i < g_dwNumModules; i++)
	   {
		   if (g_pModuleList[i].pfCalculateObjectStatus != NULL)
		   {
			   int moduleStatus = g_pModuleList[i].pfCalculateObjectStatus(this);
            if (moduleStatus != STATUS_UNKNOWN)
            {
               if (m_iStatus == STATUS_UNKNOWN)
               {
                  m_iStatus = moduleStatus;
               }
               else
               {
                  m_iStatus = max(m_iStatus, moduleStatus);
               }
            }
		   }
	   }

      UnlockData();

      // Cause parent object(s) to recalculate it's status
      if ((iOldStatus != m_iStatus) || bForcedRecalc)
      {
         LockParentList(FALSE);
         for(i = 0; i < m_dwParentCount; i++)
            m_pParentList[i]->calculateCompoundStatus();
         UnlockParentList();
         LockData();
         Modify();
         UnlockData();
      }
   }
}

/**
 * Load ACL from database
 */
BOOL NetObj::loadACLFromDB()
{
   BOOL bSuccess = FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT user_id,access_rights FROM acl WHERE object_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int i, iNumRows;

			iNumRows = DBGetNumRows(hResult);
			for(i = 0; i < iNumRows; i++)
				m_pAccessList->addElement(DBGetFieldULong(hResult, i, 0),
												  DBGetFieldULong(hResult, i, 1));
			DBFreeResult(hResult);
			bSuccess = TRUE;
		}
		DBFreeStatement(hStmt);
	}
   return bSuccess;
}

/**
 * ACL enumeration parameters structure
 */
struct SAVE_PARAM
{
   DB_HANDLE hdb;
   UINT32 dwObjectId;
};

/**
 * Handler for ACL elements enumeration
 */
static void EnumerationHandler(UINT32 dwUserId, UINT32 dwAccessRights, void *pArg)
{
   TCHAR szQuery[256];

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO acl (object_id,user_id,access_rights) VALUES (%d,%d,%d)"),
           ((SAVE_PARAM *)pArg)->dwObjectId, dwUserId, dwAccessRights);
   DBQuery(((SAVE_PARAM *)pArg)->hdb, szQuery);
}

/**
 * Save ACL to database
 */
BOOL NetObj::saveACLToDB(DB_HANDLE hdb)
{
   TCHAR szQuery[256];
   BOOL bSuccess = FALSE;
   SAVE_PARAM sp;

   // Save access list
   LockACL();
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM acl WHERE object_id=%d"), m_dwId);
   if (DBQuery(hdb, szQuery))
   {
      sp.dwObjectId = m_dwId;
      sp.hdb = hdb;
      m_pAccessList->enumerateElements(EnumerationHandler, &sp);
      bSuccess = TRUE;
   }
   UnlockACL();
   return bSuccess;
}

/**
 * Create NXCP message with object's data
 */
void NetObj::CreateMessage(CSCPMessage *pMsg)
{
   UINT32 i, dwId;

   pMsg->SetVariable(VID_OBJECT_CLASS, (WORD)Type());
   pMsg->SetVariable(VID_OBJECT_ID, m_dwId);
	pMsg->SetVariable(VID_GUID, m_guid, UUID_LENGTH);
   pMsg->SetVariable(VID_OBJECT_NAME, m_szName);
   pMsg->SetVariable(VID_OBJECT_STATUS, (WORD)m_iStatus);
   pMsg->SetVariable(VID_IP_ADDRESS, m_dwIpAddr);
   pMsg->SetVariable(VID_IS_DELETED, (WORD)(m_isDeleted ? 1 : 0));
   pMsg->SetVariable(VID_IS_SYSTEM, (WORD)(m_isSystem ? 1 : 0));

   LockParentList(FALSE);
   pMsg->SetVariable(VID_PARENT_CNT, m_dwParentCount);
   for(i = 0, dwId = VID_PARENT_ID_BASE; i < m_dwParentCount; i++, dwId++)
      pMsg->SetVariable(dwId, m_pParentList[i]->Id());
   UnlockParentList();

   LockChildList(FALSE);
   pMsg->SetVariable(VID_CHILD_CNT, m_dwChildCount);
   for(i = 0, dwId = VID_CHILD_ID_BASE; i < m_dwChildCount; i++, dwId++)
      pMsg->SetVariable(dwId, m_pChildList[i]->Id());
   UnlockChildList();

   pMsg->SetVariable(VID_INHERIT_RIGHTS, (WORD)m_bInheritAccessRights);
   pMsg->SetVariable(VID_STATUS_CALCULATION_ALG, (WORD)m_iStatusCalcAlg);
   pMsg->SetVariable(VID_STATUS_PROPAGATION_ALG, (WORD)m_iStatusPropAlg);
   pMsg->SetVariable(VID_FIXED_STATUS, (WORD)m_iFixedStatus);
   pMsg->SetVariable(VID_STATUS_SHIFT, (WORD)m_iStatusShift);
   pMsg->SetVariable(VID_STATUS_TRANSLATION_1, (WORD)m_iStatusTranslation[0]);
   pMsg->SetVariable(VID_STATUS_TRANSLATION_2, (WORD)m_iStatusTranslation[1]);
   pMsg->SetVariable(VID_STATUS_TRANSLATION_3, (WORD)m_iStatusTranslation[2]);
   pMsg->SetVariable(VID_STATUS_TRANSLATION_4, (WORD)m_iStatusTranslation[3]);
   pMsg->SetVariable(VID_STATUS_SINGLE_THRESHOLD, (WORD)m_iStatusSingleThreshold);
   pMsg->SetVariable(VID_STATUS_THRESHOLD_1, (WORD)m_iStatusThresholds[0]);
   pMsg->SetVariable(VID_STATUS_THRESHOLD_2, (WORD)m_iStatusThresholds[1]);
   pMsg->SetVariable(VID_STATUS_THRESHOLD_3, (WORD)m_iStatusThresholds[2]);
   pMsg->SetVariable(VID_STATUS_THRESHOLD_4, (WORD)m_iStatusThresholds[3]);
   pMsg->SetVariable(VID_COMMENTS, CHECK_NULL_EX(m_pszComments));
	pMsg->SetVariable(VID_IMAGE, m_image, UUID_LENGTH);
	pMsg->SetVariable(VID_SUBMAP_ID, m_submapId);
	pMsg->SetVariable(VID_NUM_TRUSTED_NODES, m_dwNumTrustedNodes);
	if (m_dwNumTrustedNodes > 0)
		pMsg->SetVariableToInt32Array(VID_TRUSTED_NODES, m_dwNumTrustedNodes, m_pdwTrustedNodes);

	pMsg->SetVariable(VID_NUM_CUSTOM_ATTRIBUTES, m_customAttributes.getSize());
	for(i = 0, dwId = VID_CUSTOM_ATTRIBUTES_BASE; i < m_customAttributes.getSize(); i++)
	{
		pMsg->SetVariable(dwId++, m_customAttributes.getKeyByIndex(i));
		pMsg->SetVariable(dwId++, m_customAttributes.getValueByIndex(i));
	}

   m_pAccessList->fillMessage(pMsg);
	m_geoLocation.fillMessage(*pMsg);
}

/**
 * Handler for EnumerateSessions()
 */
static void BroadcastObjectChange(ClientSession *pSession, void *pArg)
{
   if (pSession->isAuthenticated())
      pSession->onObjectChange((NetObj *)pArg);
}

/**
 * Mark object as modified and put on client's notification queue
 * We assume that object is locked at the time of function call
 */
void NetObj::Modify()
{
   if (g_bModificationsLocked)
      return;

   m_isModified = true;
   m_dwTimeStamp = (UINT32)time(NULL);

   // Send event to all connected clients
   if (!m_isHidden && !m_isSystem)
      EnumerateClientSessions(BroadcastObjectChange, this);
}

/**
 * Modify object from NXCP message
 */
UINT32 NetObj::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   BOOL bRecalcStatus = FALSE;

   if (!bAlreadyLocked)
      LockData();

   // Change object's name
   if (pRequest->IsVariableExist(VID_OBJECT_NAME))
      pRequest->GetVariableStr(VID_OBJECT_NAME, m_szName, MAX_OBJECT_NAME);

   // Change object's status calculation/propagation algorithms
   if (pRequest->IsVariableExist(VID_STATUS_CALCULATION_ALG))
   {
      m_iStatusCalcAlg = (int)pRequest->GetVariableShort(VID_STATUS_CALCULATION_ALG);
      m_iStatusPropAlg = (int)pRequest->GetVariableShort(VID_STATUS_PROPAGATION_ALG);
      m_iFixedStatus = (int)pRequest->GetVariableShort(VID_FIXED_STATUS);
      m_iStatusShift = pRequest->GetVariableShortAsInt32(VID_STATUS_SHIFT);
      m_iStatusTranslation[0] = (int)pRequest->GetVariableShort(VID_STATUS_TRANSLATION_1);
      m_iStatusTranslation[1] = (int)pRequest->GetVariableShort(VID_STATUS_TRANSLATION_2);
      m_iStatusTranslation[2] = (int)pRequest->GetVariableShort(VID_STATUS_TRANSLATION_3);
      m_iStatusTranslation[3] = (int)pRequest->GetVariableShort(VID_STATUS_TRANSLATION_4);
      m_iStatusSingleThreshold = (int)pRequest->GetVariableShort(VID_STATUS_SINGLE_THRESHOLD);
      m_iStatusThresholds[0] = (int)pRequest->GetVariableShort(VID_STATUS_THRESHOLD_1);
      m_iStatusThresholds[1] = (int)pRequest->GetVariableShort(VID_STATUS_THRESHOLD_2);
      m_iStatusThresholds[2] = (int)pRequest->GetVariableShort(VID_STATUS_THRESHOLD_3);
      m_iStatusThresholds[3] = (int)pRequest->GetVariableShort(VID_STATUS_THRESHOLD_4);
      bRecalcStatus = TRUE;
   }

	// Change image
	if (pRequest->IsVariableExist(VID_IMAGE))
		pRequest->GetVariableBinary(VID_IMAGE, m_image, UUID_LENGTH);

   // Change object's ACL
   if (pRequest->IsVariableExist(VID_ACL_SIZE))
   {
      UINT32 i, dwNumElements;

      LockACL();
      dwNumElements = pRequest->GetVariableLong(VID_ACL_SIZE);
      m_bInheritAccessRights = pRequest->GetVariableShort(VID_INHERIT_RIGHTS);
      m_pAccessList->deleteAll();
      for(i = 0; i < dwNumElements; i++)
         m_pAccessList->addElement(pRequest->GetVariableLong(VID_ACL_USER_BASE + i),
                                   pRequest->GetVariableLong(VID_ACL_RIGHTS_BASE +i));
      UnlockACL();
   }

	// Change trusted nodes list
   if (pRequest->IsVariableExist(VID_NUM_TRUSTED_NODES))
   {
      m_dwNumTrustedNodes = pRequest->GetVariableLong(VID_NUM_TRUSTED_NODES);
		m_pdwTrustedNodes = (UINT32 *)realloc(m_pdwTrustedNodes, sizeof(UINT32) * m_dwNumTrustedNodes);
		pRequest->GetVariableInt32Array(VID_TRUSTED_NODES, m_dwNumTrustedNodes, m_pdwTrustedNodes);
   }

   // Change custom attributes
   if (pRequest->IsVariableExist(VID_NUM_CUSTOM_ATTRIBUTES))
   {
      UINT32 i, dwId, dwNumElements;
      TCHAR *name, *value;

      dwNumElements = pRequest->GetVariableLong(VID_NUM_CUSTOM_ATTRIBUTES);
      m_customAttributes.clear();
      for(i = 0, dwId = VID_CUSTOM_ATTRIBUTES_BASE; i < dwNumElements; i++)
      {
      	name = pRequest->GetVariableStr(dwId++);
      	value = pRequest->GetVariableStr(dwId++);
      	if ((name != NULL) && (value != NULL))
	      	m_customAttributes.setPreallocated(name, value);
      }
   }

	// Change geolocation
	if (pRequest->IsVariableExist(VID_GEOLOCATION_TYPE))
	{
		m_geoLocation = GeoLocation(*pRequest);
	}

	if (pRequest->IsVariableExist(VID_SUBMAP_ID))
	{
		m_submapId = pRequest->GetVariableLong(VID_SUBMAP_ID);
	}

   Modify();
   UnlockData();

   if (bRecalcStatus)
      calculateCompoundStatus(TRUE);

   return RCC_SUCCESS;
}

/**
 * Post-modify hook
 */
void NetObj::postModify()
{
   calculateCompoundStatus(TRUE);
}

/**
 * Get rights to object for specific user
 *
 * @param userId user object ID
 */
UINT32 NetObj::getUserRights(UINT32 userId)
{
   UINT32 dwRights;

   // Admin always has all rights to any object
   if (userId == 0)
      return 0xFFFFFFFF;

	// Non-admin users have no rights to system objects
	if (m_isSystem)
		return 0;

   // Check if have direct right assignment
   LockACL();
   bool hasDirectRights = m_pAccessList->getUserRights(userId, &dwRights);
   UnlockACL();

   if (!hasDirectRights)
   {
      // We don't. If this object inherit rights from parents, get them
      if (m_bInheritAccessRights)
      {
         UINT32 i;

         LockParentList(FALSE);
         for(i = 0, dwRights = 0; i < m_dwParentCount; i++)
            dwRights |= m_pParentList[i]->getUserRights(userId);
         UnlockParentList();
      }
   }

   return dwRights;
}

/**
 * Check if given user has specific rights on this object
 *
 * @param userId user object ID
 * @param requiredRights bit mask of requested right
 * @return true if user has all rights specified in requested rights bit mask
 */
BOOL NetObj::checkAccessRights(UINT32 userId, UINT32 requiredRights)
{
   UINT32 effectiveRights = getUserRights(userId);
   return (effectiveRights & requiredRights) == requiredRights;
}

/**
 * Drop all user privileges on current object
 */
void NetObj::dropUserAccess(UINT32 dwUserId)
{
   LockACL();
   bool modified = m_pAccessList->deleteElement(dwUserId);
   UnlockACL();
   if (modified)
   {
      LockData();
      Modify();
      UnlockData();
   }
}

/**
 * Set object's management status
 */
void NetObj::setMgmtStatus(BOOL bIsManaged)
{
   UINT32 i;
   int iOldStatus;

   LockData();

   if ((bIsManaged && (m_iStatus != STATUS_UNMANAGED)) ||
       ((!bIsManaged) && (m_iStatus == STATUS_UNMANAGED)))
   {
      UnlockData();
      return;  // Status is already correct
   }

   iOldStatus = m_iStatus;
   m_iStatus = (bIsManaged ? STATUS_UNKNOWN : STATUS_UNMANAGED);

   // Generate event if current object is a node
   if (Type() == OBJECT_NODE)
      PostEvent(bIsManaged ? EVENT_NODE_UNKNOWN : EVENT_NODE_UNMANAGED, m_dwId, "d", iOldStatus);

   Modify();
   UnlockData();

   // Change status for child objects also
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      m_pChildList[i]->setMgmtStatus(bIsManaged);
   UnlockChildList();

   // Cause parent object(s) to recalculate it's status
   LockParentList(FALSE);
   for(i = 0; i < m_dwParentCount; i++)
      m_pParentList[i]->calculateCompoundStatus();
   UnlockParentList();
}

/**
 * Check if given object is an our child (possibly indirect, i.e child of child)
 *
 * @param id object ID to test
 */
bool NetObj::isChild(UINT32 id)
{
   UINT32 i;
   bool bResult = false;

   // Check for our own ID (object ID should never change, so we may not lock object's data)
   if (m_dwId == id)
      bResult = true;

   // First, walk through our own child list
   if (!bResult)
   {
      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
         if (m_pChildList[i]->Id() == id)
         {
            bResult = true;
            break;
         }
      UnlockChildList();
   }

   // If given object is not in child list, check if it is indirect child
   if (!bResult)
   {
      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
         if (m_pChildList[i]->isChild(id))
         {
            bResult = true;
            break;
         }
      UnlockChildList();
   }

   return bResult;
}

/**
 * Send message to client, who requests poll, if any
 * This method is used by Node and Interface class objects
 */
void NetObj::sendPollerMsg(UINT32 dwRqId, const TCHAR *pszFormat, ...)
{
   if (m_pollRequestor != NULL)
   {
      va_list args;
      TCHAR szBuffer[1024];

      va_start(args, pszFormat);
      _vsntprintf(szBuffer, 1024, pszFormat, args);
      va_end(args);
      m_pollRequestor->sendPollerMsg(dwRqId, szBuffer);
   }
}

/**
 * Add child node objects (direct and indirect childs) to list
 */
void NetObj::addChildNodesToList(ObjectArray<Node> *nodeList, UINT32 dwUserId)
{
   UINT32 i;

   LockChildList(FALSE);

   // Walk through our own child list
   for(i = 0; i < m_dwChildCount; i++)
   {
      if (m_pChildList[i]->Type() == OBJECT_NODE)
      {
         // Check if this node already in the list
			int j;
			for(j = 0; j < nodeList->size(); j++)
				if (nodeList->get(j)->Id() == m_pChildList[i]->Id())
               break;
         if (j == nodeList->size())
         {
            m_pChildList[i]->incRefCount();
				nodeList->add((Node *)m_pChildList[i]);
         }
      }
      else
      {
         if (m_pChildList[i]->checkAccessRights(dwUserId, OBJECT_ACCESS_READ))
            m_pChildList[i]->addChildNodesToList(nodeList, dwUserId);
      }
   }

   UnlockChildList();
}

/**
 * Add child data collection targets (direct and indirect childs) to list
 */
void NetObj::addChildDCTargetsToList(ObjectArray<DataCollectionTarget> *dctList, UINT32 dwUserId)
{
   UINT32 i;

   LockChildList(FALSE);

   // Walk through our own child list
   for(i = 0; i < m_dwChildCount; i++)
   {
      if ((m_pChildList[i]->Type() == OBJECT_NODE) || (m_pChildList[i]->Type() == OBJECT_MOBILEDEVICE))
      {
         // Check if this objects already in the list
			int j;
			for(j = 0; j < dctList->size(); j++)
				if (dctList->get(j)->Id() == m_pChildList[i]->Id())
               break;
         if (j == dctList->size())
         {
            m_pChildList[i]->incRefCount();
				dctList->add((DataCollectionTarget *)m_pChildList[i]);
         }
      }
      else
      {
         if (m_pChildList[i]->checkAccessRights(dwUserId, OBJECT_ACCESS_READ))
            m_pChildList[i]->addChildDCTargetsToList(dctList, dwUserId);
      }
   }

   UnlockChildList();
}

/**
 * Hide object and all its childs
 */
void NetObj::hide()
{
   UINT32 i;

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      m_pChildList[i]->hide();
   UnlockChildList();

	LockData();
   m_isHidden = true;
   UnlockData();
}

/**
 * Unhide object and all its childs
 */
void NetObj::unhide()
{
   UINT32 i;

   LockData();
   m_isHidden = false;
   if (!m_isSystem)
      EnumerateClientSessions(BroadcastObjectChange, this);
   UnlockData();

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      m_pChildList[i]->unhide();
   UnlockChildList();
}

/**
 * Return status propagated to parent
 */
int NetObj::getPropagatedStatus()
{
   int iStatus;

   if (m_iStatusPropAlg == SA_PROPAGATE_DEFAULT)
   {
      iStatus = DefaultPropagatedStatus(m_iStatus);
   }
   else
   {
      switch(m_iStatusPropAlg)
      {
         case SA_PROPAGATE_UNCHANGED:
            iStatus = m_iStatus;
            break;
         case SA_PROPAGATE_FIXED:
            iStatus = ((m_iStatus > STATUS_NORMAL) && (m_iStatus < STATUS_UNKNOWN)) ? m_iFixedStatus : m_iStatus;
            break;
         case SA_PROPAGATE_RELATIVE:
            if ((m_iStatus > STATUS_NORMAL) && (m_iStatus < STATUS_UNKNOWN))
            {
               iStatus = m_iStatus + m_iStatusShift;
               if (iStatus < 0)
                  iStatus = 0;
               if (iStatus > STATUS_CRITICAL)
                  iStatus = STATUS_CRITICAL;
            }
            else
            {
               iStatus = m_iStatus;
            }
            break;
         case SA_PROPAGATE_TRANSLATED:
            if ((m_iStatus > STATUS_NORMAL) && (m_iStatus < STATUS_UNKNOWN))
            {
               iStatus = m_iStatusTranslation[m_iStatus - 1];
            }
            else
            {
               iStatus = m_iStatus;
            }
            break;
         default:
            iStatus = STATUS_UNKNOWN;
            break;
      }
   }
   return iStatus;
}

/**
 * Prepare object for deletion. Method should return only
 * when object deletion is safe
 */
void NetObj::prepareForDeletion()
{
}

/**
 * Set object's comments.
 * NOTE: pszText should be dynamically allocated or NULL
 */
void NetObj::setComments(TCHAR *pszText)
{
   LockData();
   safe_free(m_pszComments);
   m_pszComments = pszText;
   Modify();
   UnlockData();
}

/**
 * Copy object's comments to NXCP message
 */
void NetObj::commentsToMessage(CSCPMessage *pMsg)
{
   LockData();
   pMsg->SetVariable(VID_COMMENTS, CHECK_NULL_EX(m_pszComments));
   UnlockData();
}

/**
 * Load trusted nodes list from database
 */
BOOL NetObj::loadTrustedNodes()
{
	DB_RESULT hResult;
	TCHAR query[256];
	int i, count;

	_sntprintf(query, 256, _T("SELECT target_node_id FROM trusted_nodes WHERE source_object_id=%d"), m_dwId);
	hResult = DBSelect(g_hCoreDB, query);
	if (hResult != NULL)
	{
		count = DBGetNumRows(hResult);
		if (count > 0)
		{
			m_dwNumTrustedNodes = count;
			m_pdwTrustedNodes = (UINT32 *)malloc(sizeof(UINT32) * count);
			for(i = 0; i < count; i++)
			{
				m_pdwTrustedNodes[i] = DBGetFieldULong(hResult, i, 0);
			}
		}
		DBFreeResult(hResult);
	}
	return (hResult != NULL);
}

/**
 * Save list of trusted nodes to database
 */
BOOL NetObj::saveTrustedNodes(DB_HANDLE hdb)
{
	TCHAR query[256];
	UINT32 i;
	BOOL rc = FALSE;

	_sntprintf(query, 256, _T("DELETE FROM trusted_nodes WHERE source_object_id=%d"), m_dwId);
	if (DBQuery(hdb, query))
	{
		for(i = 0; i < m_dwNumTrustedNodes; i++)
		{
			_sntprintf(query, 256, _T("INSERT INTO trusted_nodes (source_object_id,target_node_id) VALUES (%d,%d)"),
			           m_dwId, m_pdwTrustedNodes[i]);
			if (!DBQuery(hdb, query))
				break;
		}
		if (i == m_dwNumTrustedNodes)
			rc = TRUE;
	}
	return rc;
}

/**
 * Check if given node is in trust list
 * Will always return TRUE if system parameter CheckTrustedNodes set to 0
 */
bool NetObj::isTrustedNode(UINT32 id)
{
	bool rc;

	if (g_dwFlags & AF_CHECK_TRUSTED_NODES)
	{
		UINT32 i;

		LockData();
		for(i = 0, rc = false; i < m_dwNumTrustedNodes; i++)
		{
			if (m_pdwTrustedNodes[i] == id)
			{
				rc = true;
				break;
			}
		}
		UnlockData();
	}
	else
	{
		rc = true;
	}
	return rc;
}

/**
 * Get list of parent objects for NXSL script
 */
NXSL_Array *NetObj::getParentsForNXSL()
{
	NXSL_Array *parents = new NXSL_Array;
	int index = 0;

	LockParentList(FALSE);
	for(UINT32 i = 0; i < m_dwParentCount; i++)
	{
		if ((m_pParentList[i]->Type() == OBJECT_CONTAINER) ||
			 (m_pParentList[i]->Type() == OBJECT_SERVICEROOT) ||
			 (m_pParentList[i]->Type() == OBJECT_NETWORK))
		{
			parents->set(index++, new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, m_pParentList[i])));
		}
	}
	UnlockParentList();

	return parents;
}

/**
 * Get list of child objects for NXSL script
 */
NXSL_Array *NetObj::getChildrenForNXSL()
{
	NXSL_Array *children = new NXSL_Array;
	int index = 0;

	LockChildList(FALSE);
	for(UINT32 i = 0; i < m_dwChildCount; i++)
	{
		if (m_pChildList[i]->Type() == OBJECT_NODE)
		{
			children->set(index++, new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_pChildList[i])));
		}
		else if (m_pChildList[i]->Type() == OBJECT_INTERFACE)
		{
			children->set(index++, new NXSL_Value(new NXSL_Object(&g_nxslInterfaceClass, m_pChildList[i])));
		}
		else
		{
			children->set(index++, new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, m_pChildList[i])));
		}
	}
	UnlockChildList();

	return children;
}

/**
 * Get full list of child objects (including both direct and indirect childs)
 */
void NetObj::getFullChildListInternal(ObjectIndex *list, bool eventSourceOnly)
{
	LockChildList(FALSE);
	for(UINT32 i = 0; i < m_dwChildCount; i++)
	{
		if (!eventSourceOnly || IsEventSource(m_pChildList[i]->Type()))
			list->put(m_pChildList[i]->Id(), m_pChildList[i]);
		m_pChildList[i]->getFullChildListInternal(list, eventSourceOnly);
	}
	UnlockChildList();
}

/**
 * Get full list of child objects (including both direct and indirect childs).
 *  Returned array is dynamically allocated and must be deleted by the caller.
 *
 * @param eventSourceOnly if true, only objects that can be event source will be included
 */
ObjectArray<NetObj> *NetObj::getFullChildList(bool eventSourceOnly, bool updateRefCount)
{
	ObjectIndex list;
	getFullChildListInternal(&list, eventSourceOnly);
	return list.getObjects(updateRefCount);
}

/**
 * Get list of child objects (direct only). Returned array is
 * dynamically allocated and must be deleted by the caller.
 *
 * @param typeFilter Only return objects with class ID equals given value.
 *                   Set to -1 to disable filtering.
 */
ObjectArray<NetObj> *NetObj::getChildList(int typeFilter)
{
	LockChildList(FALSE);
	ObjectArray<NetObj> *list = new ObjectArray<NetObj>((int)m_dwChildCount, 16, false);
	for(UINT32 i = 0; i < m_dwChildCount; i++)
	{
		if ((typeFilter == -1) || (typeFilter == m_pChildList[i]->Type()))
			list->add(m_pChildList[i]);
	}
	UnlockChildList();
	return list;
}

/**
 * Get list of parent objects (direct only). Returned array is
 * dynamically allocated and must be deleted by the caller.
 *
 * @param typeFilter Only return objects with class ID equals given value.
 *                   Set to -1 to disable filtering.
 */
ObjectArray<NetObj> *NetObj::getParentList(int typeFilter)
{
    LockParentList(FALSE);
    ObjectArray<NetObj> *list = new ObjectArray<NetObj>((int)m_dwParentCount, 16, false);
    for(UINT32 i = 0; i < m_dwParentCount; i++)
    {
        if ((typeFilter == -1) || (typeFilter == m_pParentList[i]->Type()))
            list->add(m_pParentList[i]);
    }
    UnlockParentList();
    return list;
}

/**
 * Called by client session handler to check if threshold summary should
 * be shown for this object. Default implementation always returns false.
 */
bool NetObj::showThresholdSummary()
{
	return false;
}

int NetObj::getChildCount()
{
   return m_dwChildCount;
}

int NetObj::getParentCount()
{
   return m_dwParentCount;
}
