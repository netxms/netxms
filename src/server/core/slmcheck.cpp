/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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
** File: slmcheck.cpp
**
**/

#include "nxcore.h"

/**
 * SLM check default constructor
 */
SlmCheck::SlmCheck() : super()
{
	_tcscpy(m_name, _T("Default"));
	m_type = SlmCheck::check_script;
	m_script = NULL;
	m_pCompiledScript = NULL;
	m_threshold = NULL;
	m_reason[0] = 0;
	m_isTemplate = false;
	m_templateId = 0;
	m_currentTicketId = 0;
}

/**
 * Constructor for new check object
 */
SlmCheck::SlmCheck(const TCHAR *name, bool isTemplate) : super()
{
   m_isHidden = true;
	nx_strncpy(m_name, name, MAX_OBJECT_NAME);
	m_type = SlmCheck::check_script;
	m_script = NULL;
	m_pCompiledScript = NULL;
	m_threshold = NULL;
	m_reason[0] = 0;
	m_isTemplate = isTemplate;
	m_templateId = 0;
	m_currentTicketId = 0;
}


//
// Used to create a new object from a check template
//

SlmCheck::SlmCheck(SlmCheck *tmpl) : super()
{
   m_isHidden = true;
	_tcslcpy(m_name, tmpl->m_name, MAX_OBJECT_NAME);
	m_type = tmpl->m_type;
	m_script = ((m_type == check_script) && (tmpl->m_script != NULL)) ? _tcsdup(tmpl->m_script) : NULL;
	m_pCompiledScript = NULL;
	m_threshold = NULL;
	m_reason[0] = 0;
	m_isTemplate = false;
	m_templateId = tmpl->getId();
	m_currentTicketId = 0;
	compileScript();
}

/**
 * Service class destructor
 */
SlmCheck::~SlmCheck()
{
	delete m_threshold;
	free(m_script);
	delete m_pCompiledScript;
}

/**
 * Update this check from a check template
 */
void SlmCheck::updateFromTemplate(SlmCheck *tmpl)
{
	lockProperties();
	tmpl->lockProperties();
	DbgPrintf(4, _T("Updating service check %s [%d] from service check template template %s [%d]"), m_name, m_id, tmpl->getName(), tmpl->getId());

	delete m_threshold;
	free(m_script);
	delete m_pCompiledScript;

	_tcslcpy(m_name, tmpl->m_name, MAX_OBJECT_NAME);
	m_type = tmpl->m_type;
	m_script = ((m_type == check_script) && (tmpl->m_script != NULL)) ? _tcsdup(tmpl->m_script) : NULL;
	m_threshold = NULL;
	m_reason[0] = 0;
	m_isTemplate = false;
	compileScript();

	tmpl->unlockProperties();

	setModified(MODIFY_COMMON_PROPERTIES | MODIFY_OTHER);
	unlockProperties();
}

/**
 * Compile script if there is one
 */
void SlmCheck::compileScript()
{
	if (m_type != check_script || m_script == NULL)
	   return;

   const int errorMsgLen = 512;
   TCHAR errorMsg[errorMsgLen];

   m_pCompiledScript = NXSLCompileAndCreateVM(m_script, errorMsg, errorMsgLen, new NXSL_ServerEnv);
   if (m_pCompiledScript != NULL)
   {
      m_pCompiledScript->addConstant("OK", m_pCompiledScript->createValue((LONG)0));
      m_pCompiledScript->addConstant("FAIL", m_pCompiledScript->createValue((LONG)1));
   }
   else
   {
      nxlog_write(MSG_SLMCHECK_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dss", m_id, m_name, errorMsg);
   }
}

/**
 * Create object from database data
 */
bool SlmCheck::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
	UINT32 thresholdId;

	m_id = id;

	if (!loadCommonProperties(hdb))
		return false;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT type,content,threshold_id,template_id,current_ticket,is_template,reason FROM slm_checks WHERE id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from slm_checks"));
		return false;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return false;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load service check object %d - record missing"), (int)m_id);
		return false;
	}

	m_type = static_cast<SlmCheck::CheckType>(DBGetFieldLong(hResult, 0, 0));
	m_script = DBGetField(hResult, 0, 1, NULL, 0);
	thresholdId = DBGetFieldULong(hResult, 0, 2);
	m_templateId = DBGetFieldULong(hResult, 0, 3);
	m_currentTicketId = DBGetFieldULong(hResult, 0, 4);
	m_isTemplate = DBGetFieldLong(hResult, 0, 5) ? true : false;
	DBGetField(hResult, 0, 6, m_reason, 256);

	if (thresholdId > 0)
	{
		// FIXME: load threshold
	}

	compileScript();

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	// Load access list
	loadACLFromDB(hdb);

	return true;
}

/**
 * Save service check to database
 */
bool SlmCheck::saveToDatabase(DB_HANDLE hdb)
{
	lockProperties();

	bool success = saveCommonProperties(hdb);

	if (success && (m_modified & MODIFY_OTHER))
	{
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("slm_checks"), _T("id"), m_id))
      {
         hStmt = DBPrepare(hdb, _T("UPDATE slm_checks SET type=?,content=?,threshold_id=?,reason=?,is_template=?,template_id=?,current_ticket=? WHERE id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO slm_checks (type,content,threshold_id,reason,is_template,template_id,current_ticket,id) VALUES (?,?,?,?,?,?,?,?)"));
      }

      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)m_type);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, m_script, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_threshold ? m_threshold->getId() : 0);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_STATIC);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)(m_isTemplate ? 1 : 0));
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_templateId);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_currentTicketId);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
	}

   if (success)
      success = saveACLToDB(hdb);

   m_modified = 0;
	unlockProperties();
	return success;
}

/**
 * Delete object from database
 */
bool SlmCheck::deleteFromDatabase(DB_HANDLE hdb)
{
	bool success = super::deleteFromDatabase(hdb);
	if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM slm_checks WHERE id=?"));
	return success;
}

/**
 * Create NXCP message with object's data
 */
void SlmCheck::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
	super::fillMessageInternal(pMsg, userId);
	pMsg->setField(VID_SLMCHECK_TYPE, UINT32(m_type));
	pMsg->setField(VID_SCRIPT, CHECK_NULL_EX(m_script));
	pMsg->setField(VID_REASON, m_reason);
	pMsg->setField(VID_TEMPLATE_ID, m_templateId);
	pMsg->setField(VID_IS_TEMPLATE, (WORD)(m_isTemplate ? 1 : 0));
	if (m_threshold != NULL)
		m_threshold->createMessage(pMsg, VID_THRESHOLD_BASE);
}

/**
 * Modify object from message
 */
UINT32 SlmCheck::modifyFromMessageInternal(NXCPMessage *pRequest)
{
	if (pRequest->isFieldExist(VID_SLMCHECK_TYPE))
		m_type = CheckType(pRequest->getFieldAsUInt32(VID_SLMCHECK_TYPE));

	if (pRequest->isFieldExist(VID_SCRIPT))
	{
		TCHAR *script = pRequest->getFieldAsString(VID_SCRIPT);
		setScript(script);
		free(script);
	}

	if (pRequest->isFieldExist(VID_THRESHOLD_BASE))
	{
		if (m_threshold == NULL)
			m_threshold = new Threshold;
		m_threshold->updateFromMessage(pRequest, VID_THRESHOLD_BASE);
	}

	return super::modifyFromMessageInternal(pRequest);
}

/**
 * Callback for post-modify hook
 */
static void UpdateFromTemplateCallback(NetObj *object, void *data)
{
	SlmCheck *check = (SlmCheck *)object;
	SlmCheck *tmpl = (SlmCheck *)data;

	if (check->getTemplateId() == tmpl->getId())
		check->updateFromTemplate(tmpl);
}

/**
 * Post-modify hook
 */
void SlmCheck::postModify()
{
	if (m_isTemplate)
		g_idxServiceCheckById.forEach(UpdateFromTemplateCallback, this);
}

/**
 * Set check script
 */
void SlmCheck::setScript(const TCHAR *script)
{
   delete_and_null(m_pCompiledScript);
	if (script != NULL)
	{
		MemFree(m_script);
		m_script = _tcsdup(script);
		compileScript();
	}
	else
	{
		MemFreeAndNull(m_script);
	}
	setModified(MODIFY_OTHER);
}

/**
 * Execute check
 */
void SlmCheck::execute()
{
	if (m_isTemplate)
		return;

	UINT32 oldStatus = m_status;

	switch (m_type)
	{
		case check_script:
			if (m_pCompiledScript != NULL)
			{
				NXSL_VariableSystem *pGlobals = NULL;

				m_pCompiledScript->setGlobalVariable("$reason", m_pCompiledScript->createValue(m_reason));
				m_pCompiledScript->setGlobalVariable("$node", getNodeObjectForNXSL(m_pCompiledScript));
				if (m_pCompiledScript->run(0, NULL, &pGlobals))
				{
					NXSL_Value *pValue = m_pCompiledScript->getResult();
					m_status = (pValue->getValueAsInt32() == 0) ? STATUS_NORMAL : STATUS_CRITICAL;
					if (m_status == STATUS_CRITICAL)
					{
						NXSL_Variable *reason = pGlobals->find("$reason");
						setReason((reason != NULL) ? reason->getValue()->getValueAsCString() : _T("Check script returns error"));
					}
					DbgPrintf(6, _T("SlmCheck::execute: %s [%ld] return value %d"), m_name, (long)m_id, pValue->getValueAsInt32());
				}
				else
				{
					TCHAR buffer[1024];

					_sntprintf(buffer, 1024, _T("ServiceCheck::%s::%d"), m_name, m_id);
					PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, m_pCompiledScript->getErrorText(), m_id);
					nxlog_write(MSG_SLMCHECK_SCRIPT_EXECUTION_ERROR, NXLOG_WARNING, "dss", m_id, m_name, m_pCompiledScript->getErrorText());
					m_status = STATUS_UNKNOWN;
				}
				delete pGlobals;
			}
			else
			{
				m_status = STATUS_UNKNOWN;
			}
			break;
		case check_threshold:
		default:
			DbgPrintf(4, _T("SlmCheck::execute() called for undefined check type, check %s/%ld"), m_name, (long)m_id);
			m_status = STATUS_UNKNOWN;
			break;
	}

	lockProperties();
	if (m_status != oldStatus)
	{
		if (m_status == STATUS_CRITICAL)
			insertTicket();
		else
			closeTicket();
		setModified(MODIFY_COMMON_PROPERTIES);
	}
	unlockProperties();
}

/**
 * Insert ticket for this check into slm_tickets
 */
bool SlmCheck::insertTicket()
{
	DbgPrintf(4, _T("SlmCheck::insertTicket() called for %s [%d], reason='%s'"), m_name, (int)m_id, m_reason);

	if (m_status == STATUS_NORMAL)
		return false;

	m_currentTicketId = CreateUniqueId(IDG_SLM_TICKET);

	bool success = false;
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO slm_tickets (ticket_id,check_id,service_id,create_timestamp,close_timestamp,reason) VALUES (?,?,?,?,0,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_currentTicketId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, getOwnerId());
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
		DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_TRANSIENT);
		success = DBExecute(hStmt) ? true : false;
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Close current ticket
 */
void SlmCheck::closeTicket()
{
	DbgPrintf(4, _T("SlmCheck::closeTicket() called for %s [%d], ticketId=%d"), m_name, (int)m_id, (int)m_currentTicketId);
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE slm_tickets SET close_timestamp=? WHERE ticket_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_currentTicketId);
		DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);
	m_currentTicketId = 0;
}

/**
 * Get ID of owning SLM object (business service or node link)
 */
UINT32 SlmCheck::getOwnerId()
{
	UINT32 ownerId = 0;

	lockParentList(false);
	for(int i = 0; i < m_parentList->size(); i++)
	{
      NetObj *object = m_parentList->get(i);
		if ((object->getObjectClass() == OBJECT_BUSINESSSERVICE) ||
		    (object->getObjectClass() == OBJECT_NODELINK))
		{
			ownerId = object->getId();
			break;
		}
	}
	unlockParentList();
	return ownerId;
}

/**
 * Get related node object for use in NXSL script
 * Will return NXSL_Value of type NULL if there are no associated node
 */
NXSL_Value *SlmCheck::getNodeObjectForNXSL(NXSL_VM *vm)
{
	NXSL_Value *value = NULL;
	UINT32 nodeId = 0;

	lockParentList(false);
	for(int i = 0; i < m_parentList->size(); i++)
	{
	   NetObj *object = m_parentList->get(i);
		if (object->getObjectClass() == OBJECT_NODELINK)
		{
			nodeId = ((NodeLink *)object)->getNodeId();
			break;
		}
	}
	unlockParentList();

	if (nodeId != 0)
	{
		NetObj *node = FindObjectById(nodeId);
		if ((node != NULL) && (node->getObjectClass() == OBJECT_NODE))
		{
			value = vm->createValue(new NXSL_Object(vm, &g_nxslNodeClass, node));
		}
	}

	return (value != NULL) ? value : vm->createValue();
}

/**
 * Object deletion handler
 */
void SlmCheck::onObjectDelete(UINT32 objectId)
{
	// Delete itself if object curemtly being deleted is
	// a template used to create this check
	if (objectId == m_templateId)
	{
		DbgPrintf(4, _T("SlmCheck %s [%d] delete itself because of template deletion"), m_name, (int)m_id);
		deleteObject();
	}
   super::onObjectDelete(objectId);
}
