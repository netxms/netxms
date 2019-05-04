/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
AccessPoint::AccessPoint() : super()
{
	m_nodeId = 0;
   m_index = 0;
	memset(m_macAddr, 0, MAC_ADDR_LENGTH);
	m_vendor = NULL;
	m_model = NULL;
	m_serialNumber = NULL;
	m_radioInterfaces = NULL;
	m_apState = AP_ADOPTED;
   m_prevState = m_apState;
}

/**
 * Constructor for creating new access point object
 */
AccessPoint::AccessPoint(const TCHAR *name, UINT32 index, const BYTE *macAddr) : super(name)
{
	m_nodeId = 0;
   m_index = index;
	memcpy(m_macAddr, macAddr, MAC_ADDR_LENGTH);
	m_vendor = NULL;
	m_model = NULL;
	m_serialNumber = NULL;
	m_radioInterfaces = NULL;
	m_apState = AP_ADOPTED;
   m_prevState = m_apState;
	m_isHidden = true;
}

/**
 * Destructor
 */
AccessPoint::~AccessPoint()
{
	free(m_vendor);
	free(m_model);
	free(m_serialNumber);
	delete m_radioInterfaces;
}

/**
 * Create object from database data
 */
bool AccessPoint::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   m_id = dwId;

   if (!loadCommonProperties(hdb))
   {
      DbgPrintf(2, _T("Cannot load common properties for access point object %d"), dwId);
      return false;
   }

	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT mac_address,vendor,model,serial_number,node_id,ap_state,ap_index FROM access_points WHERE id=%d"), (int)m_id);
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult == NULL)
		return false;

	DBGetFieldByteArray2(hResult, 0, 0, m_macAddr, MAC_ADDR_LENGTH, 0);
	m_vendor = DBGetField(hResult, 0, 1, NULL, 0);
	m_model = DBGetField(hResult, 0, 2, NULL, 0);
	m_serialNumber = DBGetField(hResult, 0, 3, NULL, 0);
	m_nodeId = DBGetFieldULong(hResult, 0, 4);
	m_apState = (AccessPointState)DBGetFieldLong(hResult, 0, 5);
   m_prevState = (m_apState != AP_DOWN) ? m_apState : AP_ADOPTED;
   m_index = DBGetFieldULong(hResult, 0, 6);
	DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB(hdb);
   loadItemsFromDB(hdb);
   for(int i = 0; i < m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB(hdb))
         return false;

   // Link access point to node
	bool success = false;
   if (!m_isDeleted)
   {
      NetObj *object = FindObjectById(m_nodeId, OBJECT_NODE);
      if (object == NULL)
      {
         nxlog_write(MSG_INVALID_NODE_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, m_nodeId);
      }
      else
      {
         object->addChild(this);
         addParent(object);
         success = true;
      }
   }
   else
   {
      success = true;
   }

   return success;
}

/**
 * Save object to database
 */
bool AccessPoint::saveToDatabase(DB_HANDLE hdb)
{
   // Lock object's access
   lockProperties();

   saveCommonProperties(hdb);

   bool bResult;
   if (m_modified & MODIFY_OTHER)
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("access_points"), _T("id"), m_id))
         hStmt = DBPrepare(hdb, _T("UPDATE access_points SET mac_address=?,vendor=?,model=?,serial_number=?,node_id=?,ap_state=?,ap_index=? WHERE id=?"));
      else
         hStmt = DBPrepare(hdb, _T("INSERT INTO access_points (mac_address,vendor,model,serial_number,node_id,ap_state,ap_index,id) VALUES (?,?,?,?,?,?,?,?)"));
      if (hStmt != NULL)
      {
         TCHAR macStr[16];
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, BinToStr(m_macAddr, MAC_ADDR_LENGTH, macStr), DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_vendor), DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_model), DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_serialNumber), DB_BIND_STATIC);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_nodeId);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (INT32)m_apState);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_index);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_id);

         bResult = DBExecute(hStmt);

         DBFreeStatement(hStmt);
      }
      else
      {
         bResult = false;
      }
   }
   else
   {
      bResult = true;
   }

   // Save data collection items
   if (bResult && (m_modified & MODIFY_DATA_COLLECTION))
   {
		lockDciAccess(false);
      for(int i = 0; i < m_dcObjects->size(); i++)
         m_dcObjects->get(i)->saveToDatabase(hdb);
		unlockDciAccess();
   }

   // Save access list
   saveACLToDB(hdb);
   unlockProperties();

   return bResult;
}

/**
 * Delete object from database
 */
bool AccessPoint::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM access_points WHERE id=?"));
   return success;
}

/**
 * Create CSCP message with object's data
 */
void AccessPoint::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
   msg->setField(VID_IP_ADDRESS, m_ipAddress);
	msg->setField(VID_NODE_ID, m_nodeId);
	msg->setField(VID_MAC_ADDR, m_macAddr, MAC_ADDR_LENGTH);
	msg->setField(VID_VENDOR, CHECK_NULL_EX(m_vendor));
	msg->setField(VID_MODEL, CHECK_NULL_EX(m_model));
	msg->setField(VID_SERIAL_NUMBER, CHECK_NULL_EX(m_serialNumber));
   msg->setField(VID_STATE, (UINT16)m_apState);
   msg->setField(VID_AP_INDEX, m_index);

   if (m_radioInterfaces != NULL)
   {
      msg->setField(VID_RADIO_COUNT, (WORD)m_radioInterfaces->size());
      UINT32 varId = VID_RADIO_LIST_BASE;
      for(int i = 0; i < m_radioInterfaces->size(); i++)
      {
         RadioInterfaceInfo *rif = m_radioInterfaces->get(i);
         msg->setField(varId++, (UINT32)rif->index);
         msg->setField(varId++, rif->name);
         msg->setField(varId++, rif->macAddr, MAC_ADDR_LENGTH);
         msg->setField(varId++, rif->channel);
         msg->setField(varId++, (UINT32)rif->powerDBm);
         msg->setField(varId++, (UINT32)rif->powerMW);
         varId += 4;
      }
   }
   else
   {
      msg->setField(VID_RADIO_COUNT, (WORD)0);
   }
}

/**
 * Modify object from message
 */
UINT32 AccessPoint::modifyFromMessageInternal(NXCPMessage *msg)
{
   return super::modifyFromMessageInternal(msg);
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
			currNode->deleteChild(this);
			deleteParent(currNode);
		}
	}

	Node *newNode = (Node *)FindObjectById(nodeId, OBJECT_NODE);
	if (newNode != NULL)
	{
		newNode->addChild(this);
		addParent(newNode);
	}

	lockProperties();
	m_nodeId = nodeId;
	setModified(MODIFY_OTHER);
	unlockProperties();
}

/**
 * Update radio interfaces information
 */
void AccessPoint::updateRadioInterfaces(const ObjectArray<RadioInterfaceInfo> *ri)
{
	lockProperties();
	if (m_radioInterfaces == NULL)
		m_radioInterfaces = new ObjectArray<RadioInterfaceInfo>(ri->size(), 4, true);
	m_radioInterfaces->clear();
	for(int i = 0; i < ri->size(); i++)
	{
		RadioInterfaceInfo *info = new RadioInterfaceInfo;
		memcpy(info, ri->get(i), sizeof(RadioInterfaceInfo));
		m_radioInterfaces->add(info);
	}
	unlockProperties();
}

/**
 * Check if given radio interface index (radio ID) is on this access point
 */
bool AccessPoint::isMyRadio(int rfIndex)
{
	bool result = false;
	lockProperties();
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
	unlockProperties();
	return result;
}

/**
 * Check if given radio MAC address (BSSID) is on this access point
 */
bool AccessPoint::isMyRadio(const BYTE *macAddr)
{
	bool result = false;
	lockProperties();
	if (m_radioInterfaces != NULL)
	{
		for(int i = 0; i < m_radioInterfaces->size(); i++)
		{
         if (!memcmp(m_radioInterfaces->get(i)->macAddr, macAddr, MAC_ADDR_LENGTH))
			{
				result = true;
				break;
			}
		}
	}
	unlockProperties();
	return result;
}

/**
 * Get radio name
 */
void AccessPoint::getRadioName(int rfIndex, TCHAR *buffer, size_t bufSize)
{
	buffer[0] = 0;
	lockProperties();
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
	unlockProperties();
}

/**
 * Get access point's parent node
 */
Node *AccessPoint::getParentNode()
{
   return (Node *)FindObjectById(m_nodeId, OBJECT_NODE);
}

/**
 * Update access point information
 */
void AccessPoint::updateInfo(const TCHAR *vendor, const TCHAR *model, const TCHAR *serialNumber)
{
	lockProperties();

	free(m_vendor);
	m_vendor = (vendor != NULL) ? _tcsdup(vendor) : NULL;

	free(m_model);
	m_model = (model != NULL) ? _tcsdup(model) : NULL;

	free(m_serialNumber);
	m_serialNumber = (serialNumber != NULL) ? _tcsdup(serialNumber) : NULL;

	setModified(MODIFY_OTHER);
	unlockProperties();
}

/**
 * Update access point state
 */
void AccessPoint::updateState(AccessPointState state)
{
   if (state == m_apState)
      return;

	lockProperties();
   if (state == AP_DOWN)
      m_prevState = m_apState;
   m_apState = state;
   if (m_status != STATUS_UNMANAGED)
   {
      switch(state)
      {
         case AP_ADOPTED:
            m_status = STATUS_NORMAL;
            break;
         case AP_UNADOPTED:
            m_status = STATUS_MAJOR;
            break;
         case AP_DOWN:
            m_status = STATUS_CRITICAL;
            break;
         default:
            m_status = STATUS_UNKNOWN;
            break;
      }
   }
   setModified(MODIFY_OTHER);
	unlockProperties();

   if ((state == AP_ADOPTED) || (state == AP_UNADOPTED) || (state == AP_DOWN))
   {
      static const TCHAR *names[] = { _T("id"), _T("name"), _T("macAddr"), _T("ipAddr"), _T("vendor"), _T("model"), _T("serialNumber") };
      PostEventWithNames((state == AP_ADOPTED) ? EVENT_AP_ADOPTED : ((state == AP_UNADOPTED) ? EVENT_AP_UNADOPTED : EVENT_AP_DOWN),
         m_nodeId, "ishAsss", names,
         m_id, m_name, m_macAddr, &m_ipAddress,
         CHECK_NULL_EX(m_vendor), CHECK_NULL_EX(m_model), CHECK_NULL_EX(m_serialNumber));
   }
}

/**
 * Do status poll
 */
void AccessPoint::statusPollFromController(ClientSession *session, UINT32 rqId, Queue *eventQueue, Node *controller, SNMP_Transport *snmpTransport)
{
   m_pollRequestor = session;

   sendPollerMsg(rqId, _T("   Starting status poll on access point %s\r\n"), m_name);
   sendPollerMsg(rqId, _T("      Current access point status is %s\r\n"), GetStatusAsText(m_status, true));

   AccessPointState state = controller->getAccessPointState(this, snmpTransport);
   if ((state == AP_UNKNOWN) && m_ipAddress.isValid())
   {
      DbgPrintf(6, _T("AccessPoint::statusPoll(%s [%d]): unable to get AP state from driver"), m_name, m_id);
      sendPollerMsg(rqId, POLLER_WARNING _T("      Unable to get AP state from controller\r\n"));

		UINT32 icmpProxy = 0;

      if (IsZoningEnabled() && (controller->getZoneUIN() != 0))
		{
			Zone *zone = FindZoneByUIN(controller->getZoneUIN());
			if (zone != NULL)
			{
				icmpProxy = zone->getProxyNodeId(this);
			}
		}

		if (icmpProxy != 0)
		{
			sendPollerMsg(rqId, _T("      Starting ICMP ping via proxy\r\n"));
			DbgPrintf(7, _T("AccessPoint::StatusPoll(%d,%s): ping via proxy [%u]"), m_id, m_name, icmpProxy);
			Node *proxyNode = (Node *)g_idxNodeById.get(icmpProxy);
			if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
			{
				DbgPrintf(7, _T("AccessPoint::StatusPoll(%d,%s): proxy node found: %s"), m_id, m_name, proxyNode->getName());
				AgentConnection *conn = proxyNode->createAgentConnection();
				if (conn != NULL)
				{
					TCHAR parameter[64], buffer[64];

					_sntprintf(parameter, 64, _T("Icmp.Ping(%s)"), m_ipAddress.toString(buffer));
					if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
					{
						DbgPrintf(7, _T("AccessPoint::StatusPoll(%d,%s): proxy response: \"%s\""), m_id, m_name, buffer);
						TCHAR *eptr;
						long value = _tcstol(buffer, &eptr, 10);
						if ((*eptr == 0) && (value >= 0))
						{
                     m_pingLastTimeStamp = time(NULL);
                     m_pingTime = value;
							if (value < 10000)
                     {
            				sendPollerMsg(rqId, POLLER_ERROR _T("      responded to ICMP ping\r\n"));
                        if (state == AP_DOWN)
                           state = m_prevState;  /* FIXME: get actual AP state here */
                     }
                     else
							{
            				sendPollerMsg(rqId, POLLER_ERROR _T("      no response to ICMP ping\r\n"));
                        state = AP_DOWN;
							}
						}
					}
					conn->disconnect();
					conn->decRefCount();
				}
				else
				{
					DbgPrintf(7, _T("AccessPoint::StatusPoll(%d,%s): cannot connect to agent on proxy node"), m_id, m_name);
					sendPollerMsg(rqId, POLLER_ERROR _T("      Unable to establish connection with proxy node\r\n"));
				}
			}
			else
			{
				DbgPrintf(7, _T("AccessPoint::StatusPoll(%d,%s): proxy node not available"), m_id, m_name);
				sendPollerMsg(rqId, POLLER_ERROR _T("      ICMP proxy not available\r\n"));
			}
		}
		else	// not using ICMP proxy
		{
         TCHAR buffer[64];
			sendPollerMsg(rqId, _T("      Starting ICMP ping\r\n"));
			DbgPrintf(7, _T("AccessPoint::StatusPoll(%d,%s): calling IcmpPing(%s,3,%d,NULL,%d)"), m_id, m_name, m_ipAddress.toString(buffer), g_icmpPingTimeout, g_icmpPingSize);
			UINT32 dwPingStatus = IcmpPing(m_ipAddress, 3, g_icmpPingTimeout, &m_pingTime, g_icmpPingSize, false);
         m_pingLastTimeStamp = time(NULL);
			if (dwPingStatus == ICMP_SUCCESS)
         {
				sendPollerMsg(rqId, POLLER_ERROR _T("      responded to ICMP ping\r\n"));
            if (state == AP_DOWN)
               state = m_prevState;  /* FIXME: get actual AP state here */
         }
         else
			{
				sendPollerMsg(rqId, POLLER_ERROR _T("      no response to ICMP ping\r\n"));
            state = AP_DOWN;
            m_pingTime = PING_TIME_TIMEOUT;
			}
			DbgPrintf(7, _T("AccessPoint::StatusPoll(%d,%s): ping result %d, state=%d"), m_id, m_name, dwPingStatus, state);
		}
   }

   updateState(state);

   sendPollerMsg(rqId, _T("      Access point status after poll is %s\r\n"), GetStatusAsText(m_status, true));
	sendPollerMsg(rqId, _T("   Finished status poll on access point %s\r\n"), m_name);
}

/**
 * Updates last ping time and ping time
 */
void AccessPoint::updatePingData()
{
   Node *pNode = getParentNode();
   if (pNode == NULL)
   {
      DbgPrintf(7, _T("AccessPoint::updatePingData: Can't find parent node"));
      return;
   }
   UINT32 icmpProxy = pNode->getIcmpProxy();

   if (IsZoningEnabled() && (pNode->getZoneUIN() != 0) && (icmpProxy == 0))
   {
      Zone *zone = FindZoneByUIN(pNode->getZoneUIN());
      if (zone != NULL)
      {
         icmpProxy = zone->getProxyNodeId(this);
      }
   }

   if (icmpProxy != 0)
   {
      DbgPrintf(7, _T("AccessPoint::updatePingData: ping via proxy [%u]"), icmpProxy);
      Node *proxyNode = (Node *)g_idxNodeById.get(icmpProxy);
      if ((proxyNode != NULL) && proxyNode->isNativeAgent() && !proxyNode->isDown())
      {
         DbgPrintf(7, _T("AccessPoint::updatePingData: proxy node found: %s"), proxyNode->getName());
         AgentConnection *conn = proxyNode->createAgentConnection();
         if (conn != NULL)
         {
            TCHAR parameter[64], buffer[64];

            _sntprintf(parameter, 64, _T("Icmp.Ping(%s)"), m_ipAddress.toString(buffer));
            if (conn->getParameter(parameter, 64, buffer) == ERR_SUCCESS)
            {
               DbgPrintf(7, _T("AccessPoint::updatePingData:  proxy response: \"%s\""), buffer);
               TCHAR *eptr;
               long value = _tcstol(buffer, &eptr, 10);
               m_pingLastTimeStamp = time(NULL);
               if ((*eptr == 0) && (value >= 0) && (value < 10000))
               {
                  m_pingTime = value;
               }
               else
               {
                  m_pingTime = PING_TIME_TIMEOUT;
                  DbgPrintf(7, _T("AccessPoint::updatePingData: incorrect value: %d or error while parsing: %s"), value, eptr);
               }
            }
            conn->disconnect();
            conn->decRefCount();
         }
         else
         {
            DbgPrintf(7, _T("AccessPoint::updatePingData: cannot connect to agent on proxy node [%u]"), icmpProxy);
         }
      }
      else
      {
         DbgPrintf(7, _T("AccessPoint::updatePingData: proxy node not available [%u]"), icmpProxy);
      }
   }
   else	// not using ICMP proxy
   {
      UINT32 dwPingStatus = IcmpPing(m_ipAddress, 3, g_icmpPingTimeout, &m_pingTime, g_icmpPingSize, false);
      if (dwPingStatus != ICMP_SUCCESS)
      {
         DbgPrintf(7, _T("AccessPoint::updatePingData: error getting ping %d"), dwPingStatus);
         m_pingTime = PING_TIME_TIMEOUT;
      }
      m_pingLastTimeStamp = time(NULL);
   }
}

/**
 * Serialize object to JSON
 */
json_t *AccessPoint::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "index", json_integer(m_index));
   json_object_set_new(root, "ipAddress", m_ipAddress.toJson());
   json_object_set_new(root, "nodeId", json_integer(m_nodeId));
   char macAddrText[64];
   json_object_set_new(root, "macAddr", json_string_a(BinToStrA(m_macAddr, sizeof(m_macAddr), macAddrText)));
   json_object_set_new(root, "vendor", json_string_t(m_vendor));
   json_object_set_new(root, "model", json_string_t(m_model));
   json_object_set_new(root, "serialNumber", json_string_t(m_serialNumber));
   json_object_set_new(root, "radioInterfaces", json_object_array(m_radioInterfaces));
   json_object_set_new(root, "state", json_integer(m_apState));
   json_object_set_new(root, "prevState", json_integer(m_prevState));
   return root;
}
