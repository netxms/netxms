/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: nxsl_audit.cpp
**
** Tests for audit log records written for changes made by user-initiated NXSL scripts.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Client session that collects audit records instead of writing them to audit log
 */
class AuditCollectorSession : public GenericClientSession
{
public:
   mutable StringList messages;
   mutable StringList oldValues;
   mutable StringList newValues;
   mutable IntegerArray<uint32_t> objects;

   AuditCollectorSession() : GenericClientSession()
   {
      m_id = 1;
      m_systemAccessRights = ~static_cast<uint64_t>(0);
   }

   virtual void writeAuditLog(const wchar_t *subsys, bool success, uint32_t objectId, const wchar_t *format, ...) const override
   {
      va_list args;
      va_start(args, format);
      StringBuffer message;
      message.appendFormattedStringV(format, args);
      va_end(args);
      add(objectId, nullptr, nullptr, message);
   }

   virtual void writeAuditLogWithValues(const wchar_t *subsys, bool success, uint32_t objectId, const wchar_t *oldValue, const wchar_t *newValue, char valueType, const wchar_t *format, ...) const override
   {
      va_list args;
      va_start(args, format);
      StringBuffer message;
      message.appendFormattedStringV(format, args);
      va_end(args);
      add(objectId, oldValue, newValue, message);
   }

   virtual void writeAuditLogWithValues(const wchar_t *subsys, bool success, uint32_t objectId, json_t *oldValue, json_t *newValue, const wchar_t *format, ...) const override
   {
   }

   void add(uint32_t objectId, const wchar_t *oldValue, const wchar_t *newValue, const wchar_t *message) const
   {
      objects.add(objectId);
      oldValues.add(CHECK_NULL_EX(oldValue));
      newValues.add(CHECK_NULL_EX(newValue));
      messages.add(message);
   }
};

/**
 * Run script for given object with given security context (can be null). Script errors fail the test.
 */
static void RunScript(const wchar_t *source, const shared_ptr<NetObj>& object, NXSL_SecurityContext *context)
{
   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(source, new NXSL_ServerEnv(), &diag);
   if (vm == nullptr)
      WriteToTerminalEx(L"\n   Compilation error: %s\n", diag.errorText.cstr());
   AssertNotNull(vm);
   SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
   if (context != nullptr)
      vm->setSecurityContext(context);
   bool success = vm->run();
   if (!success)
      WriteToTerminalEx(L"\n   Script error: %s\n", vm->getErrorText());
   AssertTrue(success);
   delete vm;
}

/**
 * Test audit of changes made by NXSL scripts
 */
void TestNXSLAudit()
{
   shared_ptr<Container> object = make_shared<Container>();
   object->setName(L"nxsl-audit");
   NetObjInsert(object, true, false);

   static const wchar_t *script =
      L"$object.setCustomAttribute(\"audit_test\", \"v2\");\n"
      L"$object.setAlias(\"new alias\");\n"
      L"$object.unmanage();\n"
      L"RenameObject($object, \"nxsl-audit-renamed\");\n";

   StartTest(L"NXSL audit: script executed within client session");
   object->setCustomAttribute(L"audit_test", L"v1", StateChange::IGNORE);
   AuditCollectorSession session;
   RunScript(script, object, new NXSL_UserSecurityContext(&session));
   AssertEquals(session.messages.size(), 4);
   for(int i = 0; i < 4; i++)
      AssertEquals(session.objects.get(i), object->getId());
   AssertTrue(wcsstr(session.messages.get(0), L"audit_test") != nullptr);
   AssertTrue(!wcscmp(session.oldValues.get(0), L"v1"));
   AssertTrue(!wcscmp(session.newValues.get(0), L"v2"));
   AssertTrue(!wcscmp(session.newValues.get(1), L"new alias"));
   AssertTrue(wcsstr(session.messages.get(2), L"unmanaged") != nullptr);
   AssertTrue(!wcscmp(session.oldValues.get(3), L"nxsl-audit"));
   AssertTrue(!wcscmp(session.newValues.get(3), L"nxsl-audit-renamed"));
   EndTest();

   object->deleteObject();
}
