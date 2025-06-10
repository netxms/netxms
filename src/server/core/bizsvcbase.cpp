/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: bizsvcbase.cpp
**
**/

#include "nxcore.h"

/**
 * Base business service default constructor
 */
BaseBusinessService::BaseBusinessService() : super(), AutoBindTarget(this)
{
   m_id = 0;
   m_pollingDisabled = false;
   m_objectStatusThreshhold = 0;
   m_dciStatusThreshhold = 0;
}

/**
 * Base business service default constructor
 */
BaseBusinessService::BaseBusinessService(const TCHAR *name) : super(name), AutoBindTarget(this)
{
   m_pollingDisabled = false;
   m_objectStatusThreshhold = 0;
   m_dciStatusThreshhold = 0;
}

/**
 * Create business service from prototype
 */
BaseBusinessService::BaseBusinessService(const BaseBusinessService& prototype, const TCHAR *name) : super(name), AutoBindTarget(this)
{
   m_pollingDisabled = false;
   m_objectStatusThreshhold = prototype.m_objectStatusThreshhold;
   m_dciStatusThreshhold = prototype.m_dciStatusThreshhold;

   for(int i = 0; i < MAX_AUTOBIND_TARGET_FILTERS; i++)
      setAutoBindFilter(i, prototype.m_autoBindFilterSources[i]);
   m_autoBindFlags = prototype.m_autoBindFlags;
}

/**
 * Base business service destructor
 */
BaseBusinessService::~BaseBusinessService()
{
}

/**
 * Get all checks
 */
unique_ptr<SharedObjectArray<BusinessServiceCheck>> BaseBusinessService::getChecks() const
{
   checksLock();
   auto checks = make_unique<SharedObjectArray<BusinessServiceCheck>>(m_checks);
   checksUnlock();
   return checks;
}

/**
 * Load business service checks from database
 */
bool BaseBusinessService::loadChecksFromDatabase(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id,service_id,prototype_service_id,prototype_check_id,type,description,related_object,related_dci,status_threshold,content,status,current_ticket,failure_reason FROM business_service_checks WHERE service_id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      return false;
   }

   int rows = DBGetNumRows(hResult);
   for (int i = 0; i < rows; i++)
   {
      m_checks.add(make_shared<BusinessServiceCheck>(hResult, i));
   }

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);
   return true;
}

/**
 * Delete business service check from service and database. Returns client RCC.
 */
uint32_t BaseBusinessService::deleteCheck(uint32_t checkId)
{
   uint32_t rcc = RCC_INVALID_BUSINESS_CHECK_ID;
   checksLock();
   SharedPtrIterator<BusinessServiceCheck> it = m_checks.begin();
   while (it.hasNext())
   {
      if (it.next()->getId() == checkId)
      {
         m_deletedChecks.add(checkId);
         it.remove();
         setModified(MODIFY_BIZSVC_CHECKS, false);
         NotifyClientsOnBusinessServiceCheckDelete(*this, checkId);
         rcc = RCC_SUCCESS;
         break;
      }
   }
   checksUnlock();

   if (rcc == RCC_SUCCESS)
      onCheckDelete(checkId);

   return rcc;
}

/**
 * Create or modify business service check from request. Returns client RCC.
 */
uint32_t BaseBusinessService::modifyCheckFromMessage(const NXCPMessage& request)
{
   uint32_t rcc = RCC_INVALID_BUSINESS_CHECK_ID;
   uint32_t checkId = request.getFieldAsUInt32(VID_CHECK_ID);
   shared_ptr<BusinessServiceCheck> check;

   checksLock();
   if (checkId != 0)
   {
      for (const shared_ptr<BusinessServiceCheck>& c : m_checks)
      {
         if (c->getId() == checkId)
         {
            if (c->getPrototypeServiceId() != 0)
            {
               rcc = RCC_AUTO_CREATED_CHECK;
            }
            else
            {
               check = c;
            }
            break;
         }
      }
   }
   else
   {
      check = make_shared<BusinessServiceCheck>(m_id);
   }

   if (check != nullptr)
   {
      check->modifyFromMessage(request);
      if (checkId == 0) // new check was created
         m_checks.add(check);
      setModified(MODIFY_BIZSVC_CHECKS, false);
      rcc = RCC_SUCCESS;
   }

   checksUnlock();

   if (rcc == RCC_SUCCESS)
   {
      NotifyClientsOnBusinessServiceCheckUpdate(*this, check);
      onCheckModify(check);
   }

   return rcc;
}

/**
 * Modify business service from request
 */
uint32_t BaseBusinessService::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   AutoBindTarget::modifyFromMessage(msg);
   if (msg.isFieldExist(VID_OBJECT_STATUS_THRESHOLD))
   {
      m_objectStatusThreshhold = msg.getFieldAsUInt32(VID_OBJECT_STATUS_THRESHOLD);
   }
   if (msg.isFieldExist(VID_DCI_STATUS_THRESHOLD))
   {
      m_dciStatusThreshhold = msg.getFieldAsUInt32(VID_DCI_STATUS_THRESHOLD);
   }
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Fill message with business service data
 */
void BaseBusinessService::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_OBJECT_STATUS_THRESHOLD, m_objectStatusThreshhold);
   msg->setField(VID_DCI_STATUS_THRESHOLD, m_dciStatusThreshhold);
}

/**
 * Fill message with business service data
 */
void BaseBusinessService::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
   AutoBindTarget::fillMessage(msg);
}

/**
 * Load Business service from database
 */
bool BaseBusinessService::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   if (!super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (!loadChecksFromDatabase(hdb))
      return false;

   if (!AutoBindTarget::loadFromDatabase(hdb, id))
      return false;

   return true;
}

/**
 * Save business service to database
 */
bool BaseBusinessService::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success)
      success = AutoBindTarget::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_BIZSVC_CHECKS))
   {
      checksLock();
      for (int i = 0; (i < m_checks.size()) && success; i++)
         success = m_checks.get(i)->saveToDatabase(hdb);
      if (success && !m_deletedChecks.isEmpty())
      {
         StringBuffer query(_T("DELETE FROM business_service_checks WHERE id IN ("));
         for(int i = 0; i < m_deletedChecks.size(); i++)
         {
            query.append(m_deletedChecks.get(i));
            query.append(_T(","));
         }
         query.shrink(1);
         query.append(_T(")"));
         success = DBQuery(hdb, query);
         if (success)
            m_deletedChecks.clear();
      }
      checksUnlock();
   }

   return success;
}

/**
 * Delete object from database
 */
bool BaseBusinessService::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = AutoBindTarget::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM business_service_checks WHERE service_id=?"));
   return success;
}

/**
 * Callback for check modification
 */
void BaseBusinessService::onCheckModify(const shared_ptr<BusinessServiceCheck>& check)
{
}

/**
 * Callback for check delete
 */
void BaseBusinessService::onCheckDelete(uint32_t checkId)
{
}
