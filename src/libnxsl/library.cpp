/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2025 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: library.cpp
**
**/

#include "libnxsl.h"

/**
 * Constructor
 */
NXSL_Library::NXSL_Library() : m_mutex(MutexType::FAST)
{
   m_scriptList = new ObjectArray<NXSL_LibraryScript>(16, 16, Ownership::True);
}

/**
 * Destructor
 */
NXSL_Library::~NXSL_Library()
{
   delete m_scriptList;
}

/**
 * Add script to library
 */
bool NXSL_Library::addScript(NXSL_LibraryScript *script)
{
   m_scriptList->add(script);
   return true;
}

/**
 * Delete script by name
 */
void NXSL_Library::deleteScript(const TCHAR *pszName)
{
   for(int i = 0; i < m_scriptList->size(); i++)
      if (!_tcsicmp(m_scriptList->get(i)->getName(), pszName))
      {
         m_scriptList->remove(i);
         break;
      }
}

/**
 * Delete script by id
 */
void NXSL_Library::deleteScript(uint32_t id)
{
   for(int i = 0; i < m_scriptList->size(); i++)
      if (m_scriptList->get(i)->getId() == id)
      {
         m_scriptList->remove(i);
         break;
      }
}

/**
 * Find compiled NXSL program by name
 */
NXSL_Program *NXSL_Library::findNxslProgram(const TCHAR *name)
{
   for(int i = 0; i < m_scriptList->size(); i++)
   {
      NXSL_LibraryScript *script = m_scriptList->get(i);
      if (!_tcsicmp(script->getName(), name))
      {
         return script->isValid() ? script->getProgram() : nullptr;
      }
   }
   return nullptr;
}

/**
 * Find script object by ID
 */
NXSL_LibraryScript *NXSL_Library::findScript(uint32_t id)
{
   for(int i = 0; i < m_scriptList->size(); i++)
   {
      NXSL_LibraryScript *script = m_scriptList->get(i);
      if (script->getId() == id)
      {
         return script;
      }
   }
   return nullptr;
}

/**
 * Find script by name
 */
NXSL_LibraryScript *NXSL_Library::findScript(const TCHAR *name)
{
   for(int i = 0; i < m_scriptList->size(); i++)
   {
      NXSL_LibraryScript *script = m_scriptList->get(i);
      if (!_tcsicmp(script->getName(), name))
      {
         return script;
      }
   }
   return nullptr;
}

/**
 * Find script by name
 */
void NXSL_Library::forEach(std::function<void (const NXSL_LibraryScript*)> callback)
{
   for(int i = 0; i < m_scriptList->size(); i++)
      callback(m_scriptList->get(i));
}

/**
 * Create ready to run VM for given script using provided environment creator and script validator.
 * This method will do library lock internally.
 * VM must be deleted by caller when no longer needed.
 */
ScriptVMHandle NXSL_Library::createVM(const TCHAR *name, std::function<NXSL_Environment *()> environmentCreator, std::function<ScriptVMFailureReason (NXSL_LibraryScript*)> scriptValidator)
{
   lock();
   NXSL_LibraryScript *s = findScript(name);
   ScriptVMFailureReason reson = ScriptVMFailureReason::SCRIPT_NOT_FOUND;
   NXSL_VM *vm = nullptr;
   if (s != nullptr)
   {
      if (s->isValid())
      {
         if (scriptValidator != nullptr)
         {
            reson = scriptValidator(s);
         }
      }
      else
      {
         reson = ScriptVMFailureReason::SCRIPT_VALIDATION_ERROR;
      }

      if (reson == ScriptVMFailureReason::SUCCESS)
      {
         vm = new NXSL_VM(environmentCreator());
         if (!vm->load(s->getProgram()))
         {
            delete vm;
            vm = nullptr;
            reson = ScriptVMFailureReason::SCRIPT_LOAD_ERROR;
         }
      }
   }
   unlock();
   return (vm != nullptr) ? ScriptVMHandle(vm) : ScriptVMHandle(reson);
}

/**
 * Create ready to run VM for given script. This method will do library lock internally.
 * VM must be deleted by caller when no longer needed.
 */
NXSL_VM *NXSL_Library::createVM(const TCHAR *name, NXSL_Environment *env)
{
   NXSL_VM *vm = nullptr;
   lock();
   NXSL_Program *p = findNxslProgram(name);
   if (p != nullptr)
   {
      vm = new NXSL_VM(env);
      if (!vm->load(p))
      {
         delete vm;
         vm = nullptr;
      }
   }
   else
   {
      delete env;
   }
   unlock();
   return vm;
}

/**
 * Get script dependencies
 */
StringList *NXSL_Library::getScriptDependencies(const TCHAR *name)
{
   StringList *dependencies = nullptr;
   lock();
   NXSL_LibraryScript *script = findScript(name);
   if (script != nullptr)
   {
      dependencies = script->getProgram()->getRequiredModules();
   }
   unlock();
   return dependencies;
}

/**
 * Fill message with script library
 */
void NXSL_Library::fillMessage(NXCPMessage *msg)
{
   lock();
   msg->setField(VID_NUM_SCRIPTS, m_scriptList->size());
   UINT32 fieldId = VID_SCRIPT_LIST_BASE;
   for(int i = 0; i < m_scriptList->size(); i++, fieldId+=2)
   {
      m_scriptList->get(i)->fillMessage(msg, fieldId);
   }
   unlock();
}

/**
 * Create empty NXSL Script
 */
NXSL_LibraryScript::NXSL_LibraryScript()
{
   m_id = 0;
   _tcslcpy(m_name, _T(""), 1);
   m_source = nullptr;
   m_program = new NXSL_Program();
}

/**
 * Create NXSL Script
 */
NXSL_LibraryScript::NXSL_LibraryScript(uint32_t id, uuid guid, const TCHAR *name, TCHAR *source, NXSL_Environment *env)
{
   m_id = id;
   m_guid = guid;
   _tcslcpy(m_name, name, 1024);
   m_source = source;
   m_program = NXSLCompile(m_source, env, &m_diag);
}

/**
 * NXSL Script destructor
 */
NXSL_LibraryScript::~NXSL_LibraryScript()
{
   MemFree(m_source);
   delete m_program;
}

/**
 * Fill message with script id and name for list
 */
void NXSL_LibraryScript::fillMessage(NXCPMessage *msg, uint32_t base) const
{
   msg->setField(base, m_id);
   msg->setField(base + 1, m_name);
}

/**
 * Fill message with script data
 */
void NXSL_LibraryScript::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_SCRIPT_ID, m_id);
   msg->setField(VID_NAME, m_name);
   msg->setField(VID_SCRIPT_CODE, m_source);
}
