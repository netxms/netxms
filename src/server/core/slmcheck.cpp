/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 NetXMS Team
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


//
// Static data
//

NXSL_VariableSystem SlmCheck::m_nxslConstants;


//
// Initialize static members
//

void SlmCheck::init()
{
	m_nxslConstants.create(_T("OK"), new NXSL_Value((LONG)0));
	m_nxslConstants.create(_T("FAIL"), new NXSL_Value((LONG)1));
}


//
// SLM check default constructor
//

SlmCheck::SlmCheck() : NetObj()
{
	_tcscpy(m_szName, _T("Default"));
	m_type = SlmCheck::check_script;
	m_script = NULL;
	m_pCompiledScript = NULL;
	m_threshold = NULL;
	m_reason[0] = 0;
	m_isTemplate = false;
	m_templateId = 0;
	m_currentTicketId = 0;
}


//
// Constructor for new check object
//

SlmCheck::SlmCheck(const TCHAR *name, bool isTemplate) : NetObj()
{
   m_bIsHidden = TRUE;
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
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

SlmCheck::SlmCheck(SlmCheck *tmpl) : NetObj()
{
   m_bIsHidden = TRUE;
	nx_strncpy(m_szName, tmpl->m_szName, MAX_OBJECT_NAME);
	m_type = tmpl->m_type;
	m_script = ((m_type == check_script) && (tmpl->m_script != NULL)) ? _tcsdup(tmpl->m_script) : NULL;
	m_threshold = NULL;
	m_reason[0] = 0;
	m_isTemplate = false;
	m_templateId = tmpl->Id();
	m_currentTicketId = 0;
	compileScript();
}


//
// Service class destructor
//

SlmCheck::~SlmCheck()
{
	delete m_threshold;
	safe_free(m_script);
	delete m_pCompiledScript;
}


//
// Update this check from a check template
//

void SlmCheck::updateFromTemplate(SlmCheck *tmpl)
{
	LockData();
	tmpl->LockData();
	DbgPrintf(4, _T("Updating service check %s [%d] from service check template template %s [%d]"), m_szName, m_dwId, tmpl->Name(), tmpl->Id());

	delete m_threshold;
	safe_free(m_script);
	delete m_pCompiledScript;

	nx_strncpy(m_szName, tmpl->m_szName, MAX_OBJECT_NAME);
	m_type = tmpl->m_type;
	m_script = ((m_type == check_script) && (tmpl->m_script != NULL)) ? _tcsdup(tmpl->m_script) : NULL;
	m_threshold = NULL;
	m_reason[0] = 0;
	m_isTemplate = false;
	compileScript();

	tmpl->UnlockData();

	Modify();
	UnlockData();
}


//
// Compile script if there is one
//

void SlmCheck::compileScript()
{
	if (m_type == check_script && m_script != NULL)
	{
		const int errorMsgLen = 512;
		TCHAR errorMsg[errorMsgLen];

		m_pCompiledScript = NXSLCompile(m_script, errorMsg, errorMsgLen);
		if (m_pCompiledScript == NULL)
			nxlog_write(MSG_SLMCHECK_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dss", m_dwId, m_szName, errorMsg);
	}
}


//
// Create object from database data
//

BOOL SlmCheck::CreateFromDB(DWORD id)
{
	DWORD thresholdId;

	m_dwId = id;

	if (!loadCommonProperties())
		return FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT type,content,threshold_id,template_id,current_ticket,is_template,reason FROM slm_checks WHERE id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from slm_checks"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);

	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load check object %ld - record missing"), (long)m_dwId);
		return FALSE;
	}

	m_type = SlmCheck::CheckType(DBGetFieldLong(hResult, 0, 0));
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
	loadACLFromDB();

	return TRUE;
}


//
// Save service to database
//

BOOL SlmCheck::SaveToDB(DB_HANDLE hdb)
{
	BOOL bNewObject = TRUE;
	BOOL ret = FALSE;
	DB_RESULT hResult = NULL;

	LockData();

	saveCommonProperties(hdb);
   
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id FROM slm_checks WHERE id=?"));
	if (hStmt == NULL)
		goto finish;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		bNewObject = (DBGetNumRows(hResult) <= 0);
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(g_hCoreDB, bNewObject ? 
		_T("INSERT INTO slm_checks (id,type,content,threshold_id,reason,is_template,template_id,current_ticket) VALUES (?,?,?,?,?,?,?,?)") :
		_T("UPDATE slm_checks SET id=?,type=?,content=?,threshold_id=?,reason=?,is_template=?,template_id=?,current_ticket=? WHERE id=?"));
	if (hStmt == NULL)	
		goto finish;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DWORD(m_type));
	DBBind(hStmt, 3, DB_SQLTYPE_TEXT, CHECK_NULL_EX(m_script), DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_threshold ? m_threshold->getId() : 0);
	DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_STATIC);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)(m_isTemplate ? 1 : 0));
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_templateId);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_currentTicketId);
	if (!bNewObject)
		DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_dwId);
	
	if (!DBExecute(hStmt))
	{
		DBFreeStatement(hStmt);
		goto finish;
	}

	DBFreeStatement(hStmt);

	saveACLToDB(hdb);
	ret = TRUE;

finish:
	// Unlock object and clear modification flag
	m_bIsModified = FALSE;
	UnlockData();
	return ret;
}


//
// Delete object from database
//

BOOL SlmCheck::DeleteFromDB()
{
	BOOL bSuccess;

	bSuccess = NetObj::DeleteFromDB();
	if (bSuccess)
	{
		TCHAR szQuery[256];

		_sntprintf(szQuery, 256, _T("DELETE FROM slm_checks WHERE id=%d"), m_dwId);
		QueueSQLRequest(szQuery);
	}

	return bSuccess;
}


//
// Create CSCP message with object's data
//

void SlmCheck::CreateMessage(CSCPMessage *pMsg)
{
	NetObj::CreateMessage(pMsg);
	pMsg->SetVariable(VID_SLMCHECK_TYPE, DWORD(m_type));
	pMsg->SetVariable(VID_SCRIPT, CHECK_NULL_EX(m_script));
	pMsg->SetVariable(VID_REASON, m_reason);
	pMsg->SetVariable(VID_TEMPLATE_ID, m_templateId);
	pMsg->SetVariable(VID_IS_TEMPLATE, (WORD)(m_isTemplate ? 1 : 0));
	if (m_threshold != NULL)
		m_threshold->createMessage(pMsg, VID_THRESHOLD_BASE);
}


//
// Modify object from message
//

DWORD SlmCheck::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
	if (!bAlreadyLocked)
		LockData();

	if (pRequest->IsVariableExist(VID_SLMCHECK_TYPE))
		m_type = CheckType(pRequest->GetVariableLong(VID_SLMCHECK_TYPE));

	if (pRequest->IsVariableExist(VID_SCRIPT))
	{
		TCHAR *script = pRequest->GetVariableStr(VID_SCRIPT);
		setScript(script);
		safe_free(script);
	}

	if (pRequest->IsVariableExist(VID_THRESHOLD_BASE))
	{
		if (m_threshold == NULL)
			m_threshold = new Threshold;
		m_threshold->updateFromMessage(pRequest, VID_THRESHOLD_BASE);
	}

	return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// Post-modify hook
//

static void UpdateFromTemplateCallback(NetObj *object, void *data)
{
	SlmCheck *check = (SlmCheck *)object;
	SlmCheck *tmpl = (SlmCheck *)data;

	if (check->getTemplateId() == tmpl->Id())
		check->updateFromTemplate(tmpl);
}

void SlmCheck::postModify()
{
	if (m_isTemplate)
		g_idxServiceCheckById.forEach(UpdateFromTemplateCallback, this);
}


//
// Set check script
//

void SlmCheck::setScript(const TCHAR *script)
{
	if (script != NULL)
	{
		safe_free(m_script);
		delete m_pCompiledScript;
		m_script = _tcsdup(script);
		if (m_script != NULL)
		{
			TCHAR error[256];

			m_pCompiledScript = NXSLCompile(m_script, error, 256);
			if (m_pCompiledScript == NULL)
				nxlog_write(MSG_SLMCHECK_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dss", m_dwId, m_szName, error);
		}
		else
		{
			m_pCompiledScript = NULL;
		}
	}
	else
	{
		delete_and_null(m_pCompiledScript);
		safe_free_and_null(m_script);
	}
	Modify();
}


//
// Execute check
//

void SlmCheck::execute()
{
	if (m_isTemplate)
		return;

	DWORD oldStatus;

	switch (m_type)
	{
		case check_script:
			oldStatus = m_iStatus;
			if (m_pCompiledScript != NULL)
			{
				NXSL_ServerEnv *pEnv = new NXSL_ServerEnv;
				NXSL_VariableSystem *pGlobals = NULL;

				m_pCompiledScript->setGlobalVariable(_T("$reason"), new NXSL_Value(m_reason));
				m_pCompiledScript->setGlobalVariable(_T("$node"), getNodeObjectForNXSL());
				if (m_pCompiledScript->run(pEnv, 0, NULL, NULL, &pGlobals, &m_nxslConstants) == 0)
				{
					NXSL_Value *pValue = m_pCompiledScript->getResult();
					m_iStatus = (pValue->getValueAsInt32() == 0) ? STATUS_NORMAL : STATUS_CRITICAL;
					if (m_iStatus == STATUS_CRITICAL)
					{
						NXSL_Variable *reason = pGlobals->find(_T("$reason"));
						setReason((reason != NULL) ? reason->getValue()->getValueAsCString() : _T("Check script returns error"));
					}
					DbgPrintf(6, _T("SlmCheck::execute: %s [%ld] return value %d"), m_szName, (long)m_dwId, pValue->getValueAsInt32());
				}
				else
				{
					TCHAR buffer[1024];

					_sntprintf(buffer, 1024, _T("ServiceCheck::%s::%d"), m_szName, m_dwId);
					PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, m_pCompiledScript->getErrorText(), m_dwId);
					nxlog_write(MSG_SLMCHECK_SCRIPT_EXECUTION_ERROR, NXLOG_WARNING, "dss", m_dwId, m_szName, m_pCompiledScript->getErrorText());
					m_iStatus = STATUS_UNKNOWN;
				}
				delete pGlobals;
			}
			else
			{
				m_iStatus = STATUS_UNKNOWN;
			}
			break;
		case check_threshold:
		default:
			DbgPrintf(4, _T("SlmCheck::execute() called for undefined check type, check %s/%ld"), m_szName, (long)m_dwId);
			m_iStatus = STATUS_UNKNOWN;
			break;
	}

	LockData();
	if (m_iStatus != oldStatus)
	{
		if (m_iStatus == STATUS_CRITICAL)
			insertTicket();
		else
			closeTicket();
		Modify();
	}
	UnlockData();
}


//
// Insert ticket for this check into slm_tickets
//

bool SlmCheck::insertTicket()
{
	DbgPrintf(4, _T("SlmCheck::insertTicket() called for %s [%d], reason='%s'"), m_szName, (int)m_dwId, m_reason);

	if (m_iStatus == STATUS_NORMAL)
		return false;

	m_currentTicketId = CreateUniqueId(IDG_SLM_TICKET);

	bool success = false;
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO slm_tickets (ticket_id,check_id,service_id,create_timestamp,close_timestamp,reason) VALUES (?,?,?,?,0,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_currentTicketId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwId);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, getOwnerId());
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (DWORD)time(NULL));
		DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_TRANSIENT);
		success = DBExecute(hStmt) ? true : false;
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);
	return success;
}


//
// Close current ticket
//

void SlmCheck::closeTicket()
{
	DbgPrintf(4, _T("SlmCheck::closeTicket() called for %s [%d], ticketId=%d"), m_szName, (int)m_dwId, (int)m_currentTicketId);
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE slm_tickets SET close_timestamp=? WHERE ticket_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (DWORD)time(NULL));
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_currentTicketId);
		DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);
	m_currentTicketId = 0;
}


//
// Get ID of owning SLM object (business service or node link)
//

DWORD SlmCheck::getOwnerId()
{
	DWORD ownerId = 0;

	LockParentList(FALSE);
	for(DWORD i = 0; i < m_dwParentCount; i++)
	{
		if ((m_pParentList[i]->Type() == OBJECT_BUSINESSSERVICE) ||
		    (m_pParentList[i]->Type() == OBJECT_NODELINK))
		{
			ownerId = m_pParentList[i]->Id();
			break;
		}
	}
	UnlockParentList();
	return ownerId;
}


//
// Get related node object for use in NXSL script
// Will return NXSL_Value of type NULL if there are no associated node
//

NXSL_Value *SlmCheck::getNodeObjectForNXSL()
{
	NXSL_Value *value = NULL;
	DWORD nodeId = 0;

	LockParentList(FALSE);
	for(DWORD i = 0; i < m_dwParentCount; i++)
	{
		if (m_pParentList[i]->Type() == OBJECT_NODELINK)
		{
			nodeId = ((NodeLink *)m_pParentList[i])->getNodeId();
			break;
		}
	}
	UnlockParentList();

	if (nodeId != 0)
	{
		NetObj *node = FindObjectById(nodeId);
		if ((node != NULL) && (node->Type() == OBJECT_NODE))
		{
			value = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, node));
		}
	}

	return (value != NULL) ? value : new NXSL_Value;
}


//
// Object deletion handler
//

void SlmCheck::OnObjectDelete(DWORD objectId)
{
	// Delete itself if object curemtly being deleted is
	// a template used to create this check
	if (objectId == m_templateId)
	{
		DbgPrintf(4, _T("SlmCheck %s [%d] delete itself because of template deletion"), m_szName, (int)m_dwId);
		deleteObject();
	}
}
