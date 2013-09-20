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
** File: accesspoint.cpp
**/

#include "nxcore.h"

/**
 * Default constructor
 */
AccessPoint::AccessPoint() : DataCollectionTarget()
{
	m_nodeId = 0;
	memset(m_macAddr, 0, MAC_ADDR_LENGTH);
	m_vendor = NULL;
	m_model = NULL;
	m_serialNumber = NULL;
	m_radioInterfaces = NULL;
}

/**
 * Constructor for creating new mobile device object
 */
AccessPoint::AccessPoint(const TCHAR *name, BYTE *macAddr) : DataCollectionTarget(name)
{
	m_nodeId = 0;
	memcpy(m_macAddr, macAddr, MAC_ADDR_LENGTH);
	m_vendor = NULL;
	m_model = NULL;
	m_serialNumber = NULL;
	m_radioInterfaces = NULL;
	m_isHidden = true;
}

/**
 * Destructor
 */
AccessPoint::~AccessPoint()
{
	safe_free(m_vendor);
	safe_free(m_model);
	safe_free(m_serialNumber);
	delete m_radioInterfaces;
}

/**
 * Create object from database data
 */
BOOL AccessPoint::CreateFromDB(UINT32 dwId)
{
   m_dwId = dwId;

   if (!loadCommonProperties())
   {
      DbgPrintf(2, _T("Cannot load common properties for access point object %d"), dwId);
      return FALSE;
   }

	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT mac_address,vendor,model,serial_number,node_id FROM access_points WHERE id=%d"), (int)m_dwId);
	DB_RESULT hResult = DBSelect(g_hCoreDB, query);
	if (hResult == NULL)
		return FALSE;

	DBGetFieldByteArray2(hResult, 0, 0, m_macAddr, MAC_ADDR_LENGTH, 0);
	m_vendor = DBGetField(hResult, 0, 1, NULL, 0);
	m_model = DBGetField(hResult, 0, 2, NULL, 0);
	m_serialNumber = DBGetField(hResult, 0, 3, NULL, 0);
	UINT32 nodeId = DBGetFieldULong(hResult, 0, 4);
	DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB();
   loadItemsFromDB();
   for(int i = 0; i < m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB())
         return FALSE;

   // Link access point to node
	BOOL success = FALSE;
   if (!m_isDeleted)
   {
      NetObj *object = FindObjectById(nodeId);
      if (object == NULL)
      {
         nxlog_write(MSG_INVALID_NODE_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, nodeId);
      }
      else if (object->Type() != OBJECT_NODE)
      {
         nxlog_write(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, nodeId);
      }
      else
      {
         object->AddChild(this);
         AddParent(object);
         success = TRUE;
      }
   }
   else
   {
      success = TRUE;
   }

   return success;
}

/**
 * Save object to database
 */
BOOL AccessPoint::SaveToDB(DB_HANDLE hdb)
{
   // Lock object's access
   LockData();

   saveCommonProperties(hdb);

   BOOL bResult;
	DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("access_points"), _T("id"), m_dwId))
		hStmt = DBPrepare(hdb, _T("UPDATE access_points SET mac_address=?,vendor=?,model=?,serial_number=?,node_id=? WHERE id=?"));
	else
		hStmt = DBPrepare(hdb, _T("INSERT INTO access_points (mac_address,vendor,model,serial_number,node_id,id) VALUES (?,?,?,?,?,?)"));
	if (hStmt != NULL)
	{
		TCHAR macStr[16];
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, BinToStr(m_macAddr, MAC_ADDR_LENGTH, macStr), DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_vendor), DB_BIND_STATIC);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_model), DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_serialNumber), DB_BIND_STATIC);
		DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_nodeId);
		DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_dwId);

		bResult = DBExecute(hStmt);

		DBFreeStatement(hStmt);
	}
	else
	{
		bResult = FALSE;
	}

   // Save data collection items
   if (bResult)
   {
		lockDciAccess(false);
      for(int i = 0; i < m_dcObjects->size(); i++)
         m_dcObjects->get(i)->saveToDB(hdb);
		unlockDciAccess();
   }

   // Save access list
   saveACLToDB(hdb);

   // Clear modifications flag and unlock object
	if (bResult)
		m_isModified = false;
   UnlockData();

   return bResult;
}

/**
 * Delete object from database
 */
bool AccessPoint::deleteFromDB(DB_HANDLE hdb)
{
   bool success = DataCollectionTarget::deleteFromDB(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM access_points WHERE id=?"));
   return success;
}

/**
 * Create CSCP message with object's data
 */
void AccessPoint::CreateMessage(CSCPMessage *msg)
{
   DataCollectionTarget::CreateMessage(msg);
	msg->SetVariable(VID_NODE_ID, m_nodeId);
	msg->SetVariable(VID_MAC_ADDR, m_macAddr, MAC_ADDR_LENGTH);
	msg->SetVariable(VID_VENDOR, CHECK_NULL_EX(m_vendor));
	msg->SetVariable(VID_MODEL, CHECK_NULL_EX(m_model));
	msg->SetVariable(VID_SERIAL_NUMBER, CHECK_NULL_EX(m_serialNumber));

   if (m_radioInterfaces != NULL)
   {
      msg->SetVariable(VID_RADIO_COUNT, (WORD)m_radioInterfaces->size());
      UINT32 varId = VID_RADIO_LIST_BASE;
      for(int i = 0; i < m_radioInterfaces->size(); i++)
      {
         RadioInterfaceInfo *rif = m_radioInterfaces->get(i);
         msg->SetVariable(varId++, (UINT32)rif->index);
         msg->SetVariable(varId++, rif->name);
         msg->SetVariable(varId++, rif->macAddr, MAC_ADDR_LENGTH);
         msg->SetVariable(varId++, rif->channel);
         msg->SetVariable(varId++, (UINT32)rif->powerDBm);
         msg->SetVariable(varId++, (UINT32)rif->powerMW);
         varId += 4;
      }
   }
   else
   {
      msg->SetVariable(VID_RADIO_COUNT, (WORD)0);
   }
}

/**
 * Modify object from message
 */
UINT32 AccessPoint::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   return DataCollectionTarget::ModifyFromMessage(pRequest, TRUE);
}

/**
 * Attach access point to node
 */
void AccessPoint::attachToNode(UINT32 nodeId)
{
	if (m_nodeId == nodeId)
		return;

	if (m_nodeId != 0)
	{
		Node *currNode = (Node *)FindObjectById(m_nodeId, OBJECT_NODE);
		if (currNode != NULL)
		{
			currNode->DeleteChild(this);
			DeleteParent(currNode);
		}
	}

	Node *newNode = (Node *)FindObjectById(nodeId, OBJECT_NODE);
	if (newNode != NULL)
	{
		newNode->AddChild(this);
		AddParent(newNode);
	}

	LockData();
	m_nodeId = nodeId;
	Modify();
	UnlockData();
}

/**
 * Update radio interfaces information
 */
void AccessPoint::updateRadioInterfaces(ObjectArray<RadioInterfaceInfo> *ri)
{
	LockData();
	if (m_radioInterfaces == NULL)
		m_radioInterfaces = new ObjectArray<RadioInterfaceInfo>(ri->size(), 4, true);
	m_radioInterfaces->clear();
	for(int i = 0; i < ri->size(); i++)
	{
		RadioInterfaceInfo *info = new RadioInterfaceInfo;
		memcpy(info, ri->get(i), sizeof(RadioInterfaceInfo));
		m_radioInterfaces->add(info);
	}
	UnlockData();
}

/**
 * Check if given radio interface index (radio ID) is on this access point
 */
bool AccessPoint::isMyRadio(int rfIndex)
{
	bool result = false;
	LockData();
	if (m_radioInterfaces != NULL)
	{
		for(int i = 0; i < m_radioInterfaces->size(); i++)
		{
			if (m_radioInterfaces->get(i)->index == rfIndex)
			{
				result = true;
				break;
			}
		}
	}
	UnlockData();
	return result;
}

/**
 * Get radio name
 */
void AccessPoint::getRadioName(int rfIndex, TCHAR *buffer, size_t bufSize)
{
	buffer[0] = 0;
	LockData();
	if (m_radioInterfaces != NULL)
	{
		for(int i = 0; i < m_radioInterfaces->size(); i++)
		{
			if (m_radioInterfaces->get(i)->index == rfIndex)
			{
				nx_strncpy(buffer, m_radioInterfaces->get(i)->name, bufSize);
				break;
			}
		}
	}
	UnlockData();
}

/**
 * Update access point information
 */
void AccessPoint::updateInfo(const TCHAR *vendor, const TCHAR *model, const TCHAR *serialNumber)
{
	LockData();

	safe_free(m_vendor);
	m_vendor = (vendor != NULL) ? _tcsdup(vendor) : NULL;

	safe_free(m_model);
	m_model = (model != NULL) ? _tcsdup(model) : NULL;

	safe_free(m_serialNumber);
	m_serialNumber = (serialNumber != NULL) ? _tcsdup(serialNumber) : NULL;

	Modify();
	UnlockData();
}
