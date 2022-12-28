/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2022 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: problems.cpp
**/

#include "nxagentd.h"

/**
 * Agent problem
 */
struct AgentProblem
{
   int severity;
   TCHAR *key;
   TCHAR *message;

   AgentProblem(int severity, const TCHAR *key, const TCHAR *message)
   {
      this->severity = severity;
      this->key = MemCopyString(key);
      this->message = MemCopyString(message);
   }

   AgentProblem(const AgentProblem& src) = delete;

   ~AgentProblem()
   {
      MemFree(key);
      MemFree(message);
   }
};

/**
 * Registered problems
 */
static ObjectArray<AgentProblem> s_problems(0, 16, Ownership::True);
static Mutex s_problemMutex(MutexType::FAST);

/**
 * Register agent problem
 */
void RegisterProblem(int severity, const TCHAR *key, const TCHAR *message)
{
   bool updated = false;
   s_problemMutex.lock();
   for(int i = 0; i < s_problems.size(); i++)
   {
      AgentProblem *p = s_problems.get(i);
      if (!_tcscmp(p->key, key))
      {
         MemFree(p->message);
         p->message = MemCopyString(message);
         p->severity = severity;
         updated = true;
         break;
      }
   }
   if (!updated)
   {
      s_problems.add(new AgentProblem(severity, key, message));
   }
   s_problemMutex.unlock();
}

/**
 * Unregister problem
 */
void UnregisterProblem(const TCHAR *key)
{
   s_problemMutex.lock();
   for(int i = 0; i < s_problems.size(); i++)
   {
      if (!_tcscmp(s_problems.get(i)->key, key))
      {
         s_problems.remove(i);
         break;
      }
   }
   s_problemMutex.unlock();
}

/**
 * Get registered problems as a table
 */
LONG H_ProblemsTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("KEY"), DCI_DT_STRING, _T("Key"), true);
   value->addColumn(_T("SEVERITY"), DCI_DT_UINT, _T("Severity"));
   value->addColumn(_T("MESSAGE"), DCI_DT_STRING, _T("Message"));

   s_problemMutex.lock();
   for(int i = 0; i < s_problems.size(); i++)
   {
      value->addRow();
      AgentProblem *p = s_problems.get(i);
      value->set(0, p->key);
      value->set(1, p->severity);
      value->set(2, p->message);
   }
   s_problemMutex.unlock();

   return SYSINFO_RC_SUCCESS;
}
