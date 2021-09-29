/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: businessservicecheck.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("business.service.check")

/**
 * Business service check default constructor
 */
BusinessServiceCheck::BusinessServiceCheck(uint32_t serviceId)
{
	m_id = 0;
	m_type = CheckType::OBJECT;
	m_script = NULL;
	m_pCompiledScript = NULL;
	m_reason[0] = 0;
	m_relatedObject = 0;
	m_relatedDCI = 0;
	m_currentTicket = 0;
	m_serviceId = serviceId;
	m_statusThreshold = 0;
	_tcscpy(m_name, _T("Default check name"));
}

/**
 * Business service check constructor
 */
BusinessServiceCheck::BusinessServiceCheck(uint32_t serviceId, int type, uint32_t relatedObject, uint32_t relatedDCI, const TCHAR* name, int threshhold, const TCHAR* script)
{
	m_id = 0;
	m_type = type;
	m_script = MemCopyString(script);
	m_pCompiledScript = nullptr;
	m_reason[0] = 0;
	m_relatedObject = relatedObject;
	m_relatedDCI = relatedDCI;
	m_currentTicket = 0;
	m_serviceId = serviceId;
	m_statusThreshold = threshhold;
	if (name != nullptr)
	{
		_tcslcpy(m_name, name, 1023);
	}
	else
	{
		_tcscpy(m_name, _T("Default check name"));
	}

}

/**
 * Service class destructor
 */
BusinessServiceCheck::~BusinessServiceCheck()
{
	MemFree(m_script);
	delete m_pCompiledScript;
}

/**
 * Generates unique ID for business service check
 */
void BusinessServiceCheck::generateId()
{
	m_id = CreateUniqueId(IDG_BUSINESS_SERVICE_CHECK);
}

/**
 * Modify check from request
 */
void BusinessServiceCheck::modifyFromMessage(NXCPMessage *request)
{
	// If new check
   if (m_id == 0)
		generateId();

	if (request->isFieldExist(VID_BUSINESS_SERVICE_CHECK_TYPE))
   {
      m_type = request->getFieldAsUInt32(VID_BUSINESS_SERVICE_CHECK_TYPE);
   }
	if (request->isFieldExist(VID_BUSINESS_SERVICE_CHECK_RELATED_OBJECT))
   {
      m_relatedObject = request->getFieldAsUInt32(VID_BUSINESS_SERVICE_CHECK_RELATED_OBJECT);
   }
	if (request->isFieldExist(VID_BUSINESS_SERVICE_CHECK_RELATED_DCI))
   {
      m_relatedDCI = request->getFieldAsUInt32(VID_BUSINESS_SERVICE_CHECK_RELATED_DCI);
   }
	if (request->isFieldExist(VID_SCRIPT))
   {
		MemFree(m_script);
      m_script = request->getFieldAsString(VID_SCRIPT);
		compileScript();
   }
	if (request->isFieldExist(VID_DESCRIPTION))
   {
      request->getFieldAsString(VID_DESCRIPTION, m_name, 1023);
   }
	if (request->isFieldExist(VID_THRESHOLD))
   {
      m_statusThreshold = request->getFieldAsInt32(VID_THRESHOLD);
   }

	saveToDatabase();
}

/**
 * Load business service check from database
 */
void BusinessServiceCheck::loadFromSelect(DB_RESULT hResult, int row)
{
	m_id = DBGetFieldULong(hResult, row, 0);
	m_serviceId = DBGetFieldULong(hResult, row, 1);
	m_type = DBGetFieldLong(hResult, row, 2);
   DBGetField(hResult, row, 3, m_name, 1023);
	m_relatedObject = DBGetFieldULong(hResult, row, 4);
	m_relatedDCI = DBGetFieldULong(hResult, row, 5);
   m_statusThreshold = DBGetFieldULong(hResult, row, 6);
	MemFree(m_script);
	m_script = DBGetField(hResult, row, 7, nullptr, 0);
	m_currentTicket = DBGetFieldULong(hResult, row, 8);
	compileScript();
}

/**
 * Compile script if there is one
 */
void BusinessServiceCheck::compileScript()
{
	if (m_type != CheckType::SCRIPT || m_script == NULL)
	   return;

   const int errorMsgLen = 512;
   TCHAR errorMsg[errorMsgLen];

	delete m_pCompiledScript;
	m_pCompiledScript = NXSLCompile(m_script, errorMsg, errorMsgLen, nullptr);
   if (m_pCompiledScript == nullptr)
   {
      nxlog_write(NXLOG_WARNING, _T("Failed to compile script for service check object %s [%u] (%s)"), m_name, m_id, errorMsg);
   }
}

/**
 * Fill message with business service check data
 */
void BusinessServiceCheck::fillMessage(NXCPMessage *msg, uint64_t baseId)
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_type);
   msg->setField(baseId + 2, getReason());
   msg->setField(baseId + 3, m_relatedDCI);
   msg->setField(baseId + 4, m_relatedObject);
   msg->setField(baseId + 5, m_statusThreshold);
   msg->setField(baseId + 6, m_name);
   msg->setField(baseId + 7, m_script);
}

/**
 * Get reason of violated business service check
 */
const TCHAR* BusinessServiceCheck::getReason()
{
	if (m_reason[0] == 0 && m_currentTicket != 0)
	{
   	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT reason ")
														_T("FROM business_service_tickets WHERE ticket_id=?"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_currentTicket);
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != NULL)
			{
				DBGetField(hResult, 0, 0, m_reason, 255);
				DBFreeResult(hResult);
			}
			DBFreeStatement(hStmt);
		}
		DBConnectionPoolReleaseConnection(hdb);
	}
	return m_reason;
}

/**
 * Save business service check to database
 */
bool BusinessServiceCheck::saveToDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   static const TCHAR *columns[] = {
         _T("service_id") ,_T("type"), _T("description"), _T("related_object"), _T("related_dci"), _T("status_threshold"),
         _T("content"), _T("current_ticket"), nullptr
   };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("business_service_checks"), _T("id"), m_id, columns);
	bool success = false;
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_serviceId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (uint32_t)m_type);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_relatedObject);
		DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_relatedDCI);
		DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_statusThreshold);
		DBBind(hStmt, 7, DB_SQLTYPE_TEXT, m_script, DB_BIND_STATIC);
		DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_currentTicket);
		DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_id);
		success = DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}
	else
	{
		success = false;
	}
	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Delete business service check from database
 */
bool BusinessServiceCheck::deleteFromDatabase()
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	bool success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM business_service_checks WHERE id=?"));
	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Execute check. It could be object status check, or DCI status check or script
 */
uint32_t BusinessServiceCheck::execute(BusinessServiceTicketData* ticket)
{
	uint32_t oldStatus = m_status;
	switch (m_type)
	{
		case CheckType::OBJECT:
			{
				shared_ptr<NetObj> obj = FindObjectById(m_relatedObject);
				if (obj != nullptr)
				{
					int threshold = m_statusThreshold != 0 ? m_statusThreshold : ConfigReadInt(_T("BusinessServices.Check.Threshold.Objects"), 1);
					m_status = obj->getStatus() >= threshold ? STATUS_NORMAL : STATUS_CRITICAL;
					_tcslcpy(m_reason, _T("Object status threshold violation"), 256);
				}
			}
			break;
		case CheckType::SCRIPT:
			if (m_pCompiledScript != NULL)
			{
				NXSL_VM *script = CreateServerScriptVM(m_pCompiledScript, nullptr);
				if (script != nullptr)
				{
					script->addConstant("OK", script->createValue((LONG)0));
					script->addConstant("FAIL", script->createValue((LONG)1));
					script->setGlobalVariable("$reason", script->createValue());
					shared_ptr<NetObj> obj = FindObjectById(m_relatedObject);
					if (obj != nullptr)
						script->setGlobalVariable("$object", obj->createNXSLObject(script));
					if (obj != nullptr && obj->getObjectClass() == OBJECT_NODE)
						script->setGlobalVariable("$node", obj->createNXSLObject(script));
					NXSL_VariableSystem *globals = nullptr;
					ObjectRefArray<NXSL_Value> args(0);
					if (script->run(args, &globals))
					{
						NXSL_Value *pValue = script->getResult();
						if (pValue->getDataType() == NXSL_DT_STRING)
						{
							m_status = STATUS_CRITICAL;
							_tcslcpy(m_reason, pValue->getValueAsCString(), 256);
						}
						else
						{
							if (pValue->getDataType() == NXSL_DT_BOOLEAN)
							{
								m_status = pValue->isTrue() ? STATUS_NORMAL : STATUS_CRITICAL;
							}
							else
							{
								m_status = (pValue->getValueAsInt32() == 0) ? STATUS_NORMAL : STATUS_CRITICAL;
							}
							if (m_status == STATUS_CRITICAL && m_reason[0] == 0)
							{
								NXSL_Variable *reason = globals->find("$reason");
								if (reason != nullptr && reason->getValue()->getValueAsCString()[0] != 0)
								{
									_tcslcpy(m_reason, reason->getValue()->getValueAsCString(), 256);
								}
								else
								{
									_tcslcpy(m_reason, _T("Check script returns error"), 256);
								}
							}
						}
						delete globals;
					}
					else
					{
						TCHAR buffer[1024];

						_sntprintf(buffer, 1024, _T("ServiceCheck::%s::%d"), m_name, m_id);
						PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, script->getErrorText(), m_id);
						nxlog_write_tag(2, DEBUG_TAG, _T("Failed to execute script for service check object %s [%u] (%s)"), m_name, m_id, script->getErrorText());
						m_status = STATUS_NORMAL;
					}
					delete script;
				}
				else
				{
					m_status = STATUS_NORMAL;
				}
			}
			else
			{
				m_status = STATUS_NORMAL;
			}
			break;
		case CheckType::DCI:
			{
				shared_ptr<NetObj> obj = FindObjectById(m_relatedObject);
				if (obj != nullptr && obj->isDataCollectionTarget())
				{
					shared_ptr<DataCollectionTarget> target = static_pointer_cast<DataCollectionTarget>(obj);
					int threshold = m_statusThreshold != 0 ? m_statusThreshold : ConfigReadInt(_T("BusinessServices.Check.Threshold.DataCollection"), 1);
					m_status = target->getDciThreshold(m_relatedDCI) >= threshold ? STATUS_NORMAL : STATUS_CRITICAL;
					_tcslcpy(m_reason, _T("DCI status threshold violation"), 256);
				}
			}
			break;
		default:
			nxlog_write_tag(4, DEBUG_TAG, _T("BusinessServiceCheck::execute() called for undefined check type, check %s/%ld"), m_name, (long)m_id);
			m_status = STATUS_NORMAL;
			break;
	}

	if (m_status != oldStatus)
	{
		if (m_status == STATUS_CRITICAL)
		{
			insertTicket(ticket);
		}
		else
		{
			closeTicket();
			m_reason[0] = 0;
		}
	}
	return m_status;
}

/**
 * Insert ticket for this check into business_service_tickets
 */
bool BusinessServiceCheck::insertTicket(BusinessServiceTicketData* ticket)
{
	if (m_status == STATUS_NORMAL)
		return false;

	m_currentTicket = CreateUniqueId(IDG_BUSINESS_SERVICE_TICKET);

	bool success = false;
	time_t currentTime = time(nullptr);
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO business_service_tickets (ticket_id,original_ticket_id,original_service_id,check_id,check_description,service_id,create_timestamp,close_timestamp,reason) VALUES (?,0,0,?,?,?,?,0,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_currentTicket);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_TRANSIENT);
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_serviceId);
		DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (uint32_t)currentTime);
		DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_TRANSIENT);
		success = DBExecute(hStmt) ? true : false;
		DBFreeStatement(hStmt);
	}

	if (success)
	{
		ticket->ticket_id = m_currentTicket;
		ticket->check_id = m_id;
		_tcslcpy(ticket->description, m_name, 1023);
		ticket->service_id = m_serviceId;
		ticket->create_timestamp = currentTime;
		_tcslcpy(ticket->reason, m_reason, 256);

		hStmt = DBPrepare(hdb, _T("UPDATE business_service_checks SET current_ticket=? WHERE id=?"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_currentTicket);
			DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
			success = DBExecute(hStmt) ? true : false;
			DBFreeStatement(hStmt);
		}
	}

	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Close current ticket
 */
void BusinessServiceCheck::closeTicket()
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE business_service_tickets SET close_timestamp=? WHERE ticket_id=? OR original_ticket_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_currentTicket);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_currentTicket);
		DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}

	DBConnectionPoolReleaseConnection(hdb);
	m_currentTicket = 0;
	m_reason[0] = 0;
}