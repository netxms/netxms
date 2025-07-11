/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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

#define DEBUG_TAG _T("bizsvc.check")

/**
 * Create empty business service check object
 */
BusinessServiceCheck::BusinessServiceCheck(uint32_t serviceId) : m_description(_T("Unnamed")), m_mutex(MutexType::FAST)
{
   m_id = CreateUniqueId(IDG_BUSINESS_SERVICE_CHECK);
   m_serviceId = serviceId;
   m_prototypeServiceId = 0;
   m_prototypeCheckId = 0;
   m_type = BusinessServiceCheckType::OBJECT;
   m_status = STATUS_NORMAL;
   m_script = nullptr;
   m_compiledScript = nullptr;
   m_reason[0] = 0;
   m_relatedObject = 0;
   m_relatedDCI = 0;
   m_currentTicket = 0;
   m_statusThreshold = 0;
}

/**
 * Create business service check by autobind
 */
BusinessServiceCheck::BusinessServiceCheck(uint32_t serviceId, BusinessServiceCheckType type, uint32_t relatedObject, uint32_t relatedDCI, const TCHAR* description, int threshhold) :
         m_description((description != nullptr) ? description : _T("Unnamed")), m_mutex(MutexType::FAST)
{
   m_id = CreateUniqueId(IDG_BUSINESS_SERVICE_CHECK);
   m_serviceId = serviceId;
   m_prototypeServiceId = serviceId;   // Autobind indication
   m_prototypeCheckId = 0;
   m_type = type;
   m_status = STATUS_NORMAL;
   m_script = nullptr;
   m_compiledScript = nullptr;
   m_reason[0] = 0;
   m_relatedObject = relatedObject;
   m_relatedDCI = relatedDCI;
   m_currentTicket = 0;
   m_statusThreshold = threshhold;
}

/**
 * Create business service check from prototype
 */
BusinessServiceCheck::BusinessServiceCheck(uint32_t serviceId, const BusinessServiceCheck& prototype) : m_description(prototype.m_description), m_mutex(MutexType::FAST)
{
   m_id = CreateUniqueId(IDG_BUSINESS_SERVICE_CHECK);
   m_serviceId = serviceId;
   m_prototypeServiceId = prototype.m_serviceId;
   m_prototypeCheckId = prototype.m_id;
	m_type = prototype.m_type;
   m_status = STATUS_NORMAL;
	m_script = MemCopyString(prototype.m_script);
	m_compiledScript = nullptr;
	m_reason[0] = 0;
	m_relatedObject = prototype.m_relatedObject;
	m_relatedDCI = prototype.m_relatedDCI;
	m_currentTicket = 0;
	m_statusThreshold = prototype.m_statusThreshold;
   compileScript();
}

/**
 * Create business service check from database
 * Expected field order: id,service_id,prototype_service_id,prototype_check_id,type,description,related_object,related_dci,status_threshold,content,status,current_ticket,failure_reason
 */
BusinessServiceCheck::BusinessServiceCheck(DB_RESULT hResult, int row) : m_mutex(MutexType::FAST)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_serviceId = DBGetFieldULong(hResult, row, 1);
   m_prototypeServiceId = DBGetFieldULong(hResult, row, 2);
   m_prototypeCheckId = DBGetFieldULong(hResult, row, 3);
   m_type = BusinessServiceCheckTypeFromInt(DBGetFieldLong(hResult, row, 4));
   m_description = DBGetFieldAsSharedString(hResult, row, 5);
   m_relatedObject = DBGetFieldULong(hResult, row, 6);
   m_relatedDCI = DBGetFieldULong(hResult, row, 7);
   m_statusThreshold = DBGetFieldULong(hResult, row, 8);
   m_script = DBGetField(hResult, row, 9, nullptr, 0);
   m_status = DBGetFieldULong(hResult, row, 10);
   m_currentTicket = DBGetFieldULong(hResult, row, 11);
   DBGetField(hResult, row, 12, m_reason, 256);
   m_compiledScript = nullptr;
   compileScript();
}

/**
 * Service class destructor
 */
BusinessServiceCheck::~BusinessServiceCheck()
{
	MemFree(m_script);
	delete m_compiledScript;
}

/**
 * Update check from prototype
 */
void BusinessServiceCheck::updateFromPrototype(const BusinessServiceCheck& prototype)
{
   m_type = prototype.m_type;
   MemFree(m_script);
   m_script = MemCopyString(prototype.m_script);
   compileScript();
   m_relatedObject = prototype.m_relatedObject;
   m_relatedDCI = prototype.m_relatedDCI;
   m_statusThreshold = prototype.m_statusThreshold;
}

/**
 * Modify check from request
 */
void BusinessServiceCheck::modifyFromMessage(const NXCPMessage& request)
{
	lock();
	if (request.isFieldExist(VID_BIZSVC_CHECK_TYPE))
   {
      m_type = BusinessServiceCheckTypeFromInt(request.getFieldAsInt16(VID_BIZSVC_CHECK_TYPE));
   }
	if (request.isFieldExist(VID_RELATED_OBJECT))
   {
      m_relatedObject = request.getFieldAsUInt32(VID_RELATED_OBJECT);
   }
	if (request.isFieldExist(VID_RELATED_DCI))
   {
      m_relatedDCI = request.getFieldAsUInt32(VID_RELATED_DCI);
   }
	if (request.isFieldExist(VID_SCRIPT))
   {
		MemFree(m_script);
      m_script = request.getFieldAsString(VID_SCRIPT);
		compileScript();
   }
	if (request.isFieldExist(VID_DESCRIPTION))
   {
      m_description = request.getFieldAsSharedString(VID_DESCRIPTION);
   }
	if (request.isFieldExist(VID_THRESHOLD))
   {
      m_statusThreshold = request.getFieldAsInt32(VID_THRESHOLD);
   }
	unlock();
}

/**
 * Compile script if there is one
 */
void BusinessServiceCheck::compileScript()
{
	if ((m_type != BusinessServiceCheckType::SCRIPT) || (m_script == nullptr))
	   return;

	delete m_compiledScript;
   NXSL_CompilationDiagnostic diag;
   NXSL_ServerEnv env;
	m_compiledScript = NXSLCompile(m_script, &env, &diag);
   if (m_compiledScript == nullptr)
   {
      ReportScriptError(SCRIPT_CONTEXT_BIZSVC, FindObjectById(m_serviceId).get(), 0, diag.errorText, _T("BusinessServiceCheck::%u"), m_id);
   }
}

/**
 * Fill message with business service check data
 */
void BusinessServiceCheck::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
	lock();
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, static_cast<uint16_t>(m_type));
   msg->setField(baseId + 2, m_reason);
   msg->setField(baseId + 3, m_relatedDCI);
   msg->setField(baseId + 4, m_relatedObject);
   msg->setField(baseId + 5, m_statusThreshold);
   msg->setField(baseId + 6, m_description);
   msg->setField(baseId + 7, m_script);
   msg->setField(baseId + 8, m_status);
   msg->setField(baseId + 9, m_prototypeServiceId);
   msg->setField(baseId + 10, m_prototypeCheckId);
   msg->setField(baseId + 11, m_serviceId);
	unlock();
}

/**
 * Save business service check to database
 */
bool BusinessServiceCheck::saveToDatabase(DB_HANDLE hdb) const
{
   static const TCHAR *columns[] = {
         _T("service_id"), _T("prototype_service_id"), _T("prototype_check_id"), _T("type"), _T("description"), _T("related_object"),
         _T("related_dci"), _T("status_threshold"), _T("content"), _T("status"), _T("current_ticket"), _T("failure_reason"), nullptr
   };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("business_service_checks"), _T("id"), m_id, columns);
	bool success;
	if (hStmt != nullptr)
	{
		lock();
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_serviceId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_prototypeServiceId);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_prototypeCheckId);
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_type));
		DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
		DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_relatedObject);
		DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_relatedDCI);
		DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_statusThreshold);
		DBBind(hStmt, 9, DB_SQLTYPE_TEXT, m_script, DB_BIND_STATIC);
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_status);
		DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_currentTicket);
      DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_STATIC);
		DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_id);
		success = DBExecute(hStmt);
      unlock();
		DBFreeStatement(hStmt);
	}
	else
	{
		success = false;
	}

	return success;
}

/**
 * Execute check. It could be object status check, or DCI status check or script
 */
int BusinessServiceCheck::execute(const shared_ptr<BusinessServiceTicketData>& ticket)
{
	lock();
	int oldState = m_status;
	switch (m_type)
	{
		case BusinessServiceCheckType::OBJECT:
			{
				shared_ptr<NetObj> obj = FindObjectById(m_relatedObject);
				if (obj != nullptr)
				{
					int threshold = (m_statusThreshold != 0) ? m_statusThreshold : ConfigReadInt(_T("BusinessServices.Check.Threshold.Objects"), STATUS_WARNING);
					if ((obj->getStatus() >= threshold) && (obj->getStatus() <= STATUS_CRITICAL))
					{
					   m_status = STATUS_CRITICAL;
					   if (oldState != STATUS_CRITICAL)
	                  _tcscpy(m_reason, _T("Object status threshold violation"));
					}
					else if ((obj->getStatus() > STATUS_NORMAL) && (obj->getStatus() <= STATUS_CRITICAL))
					{
                  m_status = STATUS_MINOR;
                  if (oldState != STATUS_MINOR)
                     _tcscpy(m_reason, _T("Object status threshold violation"));
					}
					else
					{
					   m_status = STATUS_NORMAL;
					}
				}
			}
			break;
		case BusinessServiceCheckType::SCRIPT:
			if (m_compiledScript != nullptr)
			{
				NXSL_VM *vm = CreateServerScriptVM(m_compiledScript, FindObjectById(m_relatedObject));
				if (vm != nullptr)
				{
					vm->addConstant("OK", vm->createValue(true));
					vm->addConstant("FAIL", vm->createValue(false));
					vm->setGlobalVariable("$reason", vm->createValue());
					shared_ptr<NetObj> serviceObject = FindObjectById(m_serviceId);
					if (serviceObject != nullptr)
					   vm->setGlobalVariable("$service", serviceObject->createNXSLObject(vm));
					NXSL_VariableSystem *globals = nullptr;
					ObjectRefArray<NXSL_Value> args(0);
					if (vm->run(args, &globals))
					{
						NXSL_Value *value = vm->getResult();
						if (value->getDataType() == NXSL_DT_STRING)
						{
							m_status = STATUS_CRITICAL;
							_tcslcpy(m_reason, value->getValueAsCString(), 256);
						}
						else
						{
                     m_status = value->isTrue() ? STATUS_NORMAL : STATUS_CRITICAL;
							if (m_status == STATUS_CRITICAL)
							{
								NXSL_Variable *reason = globals->find("$reason");
								if ((reason != nullptr) && (reason->getValue()->getValueAsCString()[0] != 0))
								{
									_tcslcpy(m_reason, reason->getValue()->getValueAsCString(), 256);
								}
								else
								{
									_tcscpy(m_reason, _T("Check script returned error"));
								}
							}
						}
					}
					else
					{
					   ReportScriptError(SCRIPT_CONTEXT_BIZSVC, FindObjectById(m_serviceId).get(), 0, vm->getErrorText(), _T("BusinessServiceCheck::%u"), m_id);
						m_status = STATUS_NORMAL;
					}
               delete globals;
					delete vm;
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
		case BusinessServiceCheckType::DCI:
			{
				shared_ptr<NetObj> object = FindObjectById(m_relatedObject);
				if ((object != nullptr) && object->isDataCollectionTarget())
				{
					int threshold = (m_statusThreshold != 0) ? m_statusThreshold : ConfigReadInt(_T("BusinessServices.Check.Threshold.DataCollection"), STATUS_WARNING);
					int dciStatus = static_cast<DataCollectionTarget&>(*object).getDciThresholdSeverity(m_relatedDCI);
					if (dciStatus >= threshold)
					{
					   m_status = STATUS_CRITICAL;
					   if (oldState != STATUS_CRITICAL)
					      _tcscpy(m_reason, _T("DCI threshold violation"));
					}
					else if (dciStatus != STATUS_NORMAL)
					{
                  m_status = STATUS_MINOR;
                  if (oldState != STATUS_MINOR)
                     _tcscpy(m_reason, _T("DCI threshold violation"));
					}
					else
					{
					   m_status = STATUS_NORMAL;
					}
				}
			}
			break;
		default:
			nxlog_write_tag(4, DEBUG_TAG, _T("BusinessServiceCheck::execute(%s [%u]) called for undefined check type %d"), m_description.cstr(), m_id, m_type);
			m_status = STATUS_NORMAL;
			break;
	}

	if (m_status != oldState)
	{
		if (m_status == STATUS_CRITICAL)
		{
			insertTicket(ticket);
		}
		else if (oldState == STATUS_CRITICAL)
		{
			closeTicket();
			if (m_status == STATUS_NORMAL)
			   m_reason[0] = 0;
		}
	}
	unlock();
	return m_status;
}

/**
 * Insert ticket into database
 */
static void InsertTicketIntoDB(const shared_ptr<BusinessServiceTicketData>& ticket)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (DBBegin(hdb))
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO business_service_tickets (ticket_id,original_ticket_id,original_service_id,check_id,check_description,service_id,create_timestamp,close_timestamp,reason) VALUES (?,0,0,?,?,?,?,0,?)"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, ticket->ticketId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, ticket->checkId);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, ticket->description, DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, ticket->serviceId);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(ticket->timestamp));
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, ticket->reason, DB_BIND_STATIC);
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }

      hStmt = DBPrepare(hdb, _T("UPDATE business_service_checks SET current_ticket=?,status=4 WHERE id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, ticket->ticketId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, ticket->checkId);
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }

      DBCommit(hdb);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Insert ticket for this check into business_service_tickets. Expected to be called while lock on check is held.
 */
void BusinessServiceCheck::insertTicket(const shared_ptr<BusinessServiceTicketData>& ticket)
{
	if (m_status == STATUS_NORMAL)
		return;

   time_t currentTime = time(nullptr);
	m_currentTicket = CreateUniqueId(IDG_BUSINESS_SERVICE_TICKET);
   ticket->ticketId = m_currentTicket;
   ticket->checkId = m_id;
   _tcslcpy(ticket->description, m_description, 1024);
   ticket->serviceId = m_serviceId;
   ticket->timestamp = currentTime;
   _tcslcpy(ticket->reason, m_reason, 256);

   ThreadPoolExecuteSerialized(g_mainThreadPool, _T("BizSvcTicketUpdate"), InsertTicketIntoDB, ticket);
}

/**
 * Close ticket in database
 */
static void CloseTicketInDB(uint32_t checkId, int state, time_t timestamp, uint32_t ticketId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE business_service_tickets SET close_timestamp=? WHERE ticket_id=? OR original_ticket_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(timestamp));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, ticketId);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, ticketId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   hStmt = DBPrepare(hdb, _T("UPDATE business_service_checks SET current_ticket=0,status=? WHERE id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, state);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, checkId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBCommit(hdb);
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Close current ticket.  Expected to be called while lock on check is held.
 */
void BusinessServiceCheck::closeTicket()
{
   ThreadPoolExecuteSerialized(g_mainThreadPool, _T("BizSvcTicketUpdate"), CloseTicketInDB, m_id, m_status, time(nullptr), m_currentTicket);
	m_currentTicket = 0;
	m_reason[0] = 0;
}
