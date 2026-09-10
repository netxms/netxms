/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: test-ai-checks.cpp
**
** Unit tests for AI operator standing check decision logic: evaluation of script
** return values and the quiet/fired/error state transitions with cooldown and
** renotify handling. Compiles the real src/server/core/ai_check_logic.cpp.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <nxai.h>
#include <testtools.h>

/**
 * Run NXSL source and return its result value (VM is kept alive so the value stays valid)
 */
static NXSL_Value *RunScript(const char *source, NXSL_VM **vmHandle)
{
   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(source, new NXSL_Environment(), &diag);
   AssertNotNull(vm);
   AssertTrue(vm->run());
   *vmHandle = vm;
   return vm->getResult();
}

/**
 * Evaluate result of given script
 */
static AICheckResult Evaluate(const char *source)
{
   NXSL_VM *vm;
   NXSL_Value *value = RunScript(source, &vm);
   AICheckResult result = EvaluateAICheckResult(value, L"test-check");
   delete vm;
   return result;
}

/**
 * Quiet verdicts: null, false, zero, empty string
 */
static void TestQuietResults()
{
   StartTest(_T("Quiet results"));
   AssertTrue(Evaluate("return null;").verdict == AICheckVerdict::QUIET);
   AssertTrue(Evaluate("return false;").verdict == AICheckVerdict::QUIET);
   AssertTrue(Evaluate("return 0;").verdict == AICheckVerdict::QUIET);
   AssertTrue(Evaluate("return \"\";").verdict == AICheckVerdict::QUIET);
   AssertTrue(EvaluateAICheckResult(nullptr, L"test-check").verdict == AICheckVerdict::QUIET);
   EndTest();
}

/**
 * String result fires with the string as title and default severity
 */
static void TestStringResult()
{
   StartTest(_T("String result"));
   AICheckResult result = Evaluate("return \"Disk is full\";");
   AssertTrue(result.verdict == AICheckVerdict::FIRED);
   AssertEquals(result.title.c_str(), "Disk is full");
   AssertEquals(result.severity, SEVERITY_WARNING);
   AssertTrue(result.details.empty());
   EndTest();
}

/**
 * Hash result with all fields
 */
static void TestHashResult()
{
   StartTest(_T("Hash result"));
   AICheckResult result = Evaluate("return { \"title\": \"CPU high\", \"severity\": \"major\", \"details\": \"95% for 10 minutes\" };");
   AssertTrue(result.verdict == AICheckVerdict::FIRED);
   AssertEquals(result.title.c_str(), "CPU high");
   AssertEquals(result.severity, SEVERITY_MAJOR);
   AssertEquals(result.details.c_str(), "95% for 10 minutes");

   // Numeric severity
   result = Evaluate("return { \"title\": \"x\", \"severity\": 4 };");
   AssertTrue(result.verdict == AICheckVerdict::FIRED);
   AssertEquals(result.severity, SEVERITY_CRITICAL);

   // Severity names are case-insensitive
   result = Evaluate("return { \"title\": \"x\", \"severity\": \"Minor\" };");
   AssertTrue(result.verdict == AICheckVerdict::FIRED);
   AssertEquals(result.severity, SEVERITY_MINOR);
   EndTest();
}

/**
 * Hash result without title falls back to check name; without severity uses warning
 */
static void TestHashResultDefaults()
{
   StartTest(_T("Hash result defaults"));
   AICheckResult result = Evaluate("return { \"details\": \"something\" };");
   AssertTrue(result.verdict == AICheckVerdict::FIRED);
   AssertEquals(result.title.c_str(), "test-check");
   AssertEquals(result.severity, SEVERITY_WARNING);
   AssertEquals(result.details.c_str(), "something");

   result = Evaluate("return { \"title\": \"\" };");
   AssertTrue(result.verdict == AICheckVerdict::FIRED);
   AssertEquals(result.title.c_str(), "test-check");
   EndTest();
}

/**
 * Invalid severity in hash is a check error
 */
static void TestHashInvalidSeverity()
{
   StartTest(_T("Hash result with invalid severity"));
   AICheckResult result = Evaluate("return { \"title\": \"x\", \"severity\": \"urgent\" };");
   AssertTrue(result.verdict == AICheckVerdict::FAILED);
   AssertFalse(result.error.empty());

   result = Evaluate("return { \"title\": \"x\", \"severity\": 7 };");
   AssertTrue(result.verdict == AICheckVerdict::FAILED);
   EndTest();
}

/**
 * Unsupported return values are check errors
 */
static void TestErrorResults()
{
   StartTest(_T("Error results"));
   AICheckResult result = Evaluate("return true;");
   AssertTrue(result.verdict == AICheckVerdict::FAILED);
   AssertFalse(result.error.empty());

   AssertTrue(Evaluate("return 42;").verdict == AICheckVerdict::FAILED);
   AssertTrue(Evaluate("return 1.5;").verdict == AICheckVerdict::FAILED);
   AssertTrue(Evaluate("return [1, 2, 3];").verdict == AICheckVerdict::FAILED);
   EndTest();
}

/**
 * Quiet -> fired edge fires; fired -> fired without renotify does nothing
 */
static void TestEdgeTransitions()
{
   StartTest(_T("Edge transitions"));
   time_t now = 1000000;

   // Never run -> fired
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::NONE, 0, now, 0, 0, AICheckVerdict::FIRED) == AICheckTransition::FIRE_EDGE);
   // Quiet -> fired
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::QUIET, 0, now, 0, 0, AICheckVerdict::FIRED) == AICheckTransition::FIRE_EDGE);
   // Fired -> fired, no renotify
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FIRED, now - 60, now, 0, 0, AICheckVerdict::FIRED) == AICheckTransition::NONE);
   // Fired -> quiet
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FIRED, now - 60, now, 0, 0, AICheckVerdict::QUIET) == AICheckTransition::CLEAR);
   // Quiet -> quiet
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::QUIET, 0, now, 0, 0, AICheckVerdict::QUIET) == AICheckTransition::NONE);
   // Never run -> quiet
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::NONE, 0, now, 0, 0, AICheckVerdict::QUIET) == AICheckTransition::NONE);
   EndTest();
}

/**
 * Renotify interval re-applies the action while still fired
 */
static void TestRenotify()
{
   StartTest(_T("Renotify"));
   time_t now = 1000000;

   // Not yet elapsed
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FIRED, now - 100, now, 0, 300, AICheckVerdict::FIRED) == AICheckTransition::NONE);
   // Elapsed
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FIRED, now - 300, now, 0, 300, AICheckVerdict::FIRED) == AICheckTransition::FIRE_RENOTIFY);
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FIRED, now - 1000, now, 0, 300, AICheckVerdict::FIRED) == AICheckTransition::FIRE_RENOTIFY);
   // Elapsed but still inside a longer cooldown
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FIRED, now - 300, now, 600, 300, AICheckVerdict::FIRED) == AICheckTransition::NONE);
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FIRED, now - 600, now, 600, 300, AICheckVerdict::FIRED) == AICheckTransition::FIRE_RENOTIFY);
   EndTest();
}

/**
 * Cooldown suppresses a new edge shortly after the previous fire
 */
static void TestCooldown()
{
   StartTest(_T("Cooldown"));
   time_t now = 1000000;

   // Edge inside cooldown is suppressed
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::QUIET, now - 100, now, 300, 0, AICheckVerdict::FIRED) == AICheckTransition::SUPPRESSED);
   // Edge after cooldown fires
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::QUIET, now - 300, now, 300, 0, AICheckVerdict::FIRED) == AICheckTransition::FIRE_EDGE);
   // First fire ever is never suppressed
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::QUIET, 0, now, 300, 0, AICheckVerdict::FIRED) == AICheckTransition::FIRE_EDGE);
   // Cooldown does not affect clearing
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FIRED, now - 10, now, 300, 0, AICheckVerdict::QUIET) == AICheckTransition::CLEAR);
   EndTest();
}

/**
 * Script errors always produce FAILURE, regardless of previous state; a fired verdict after an error is an edge again
 */
static void TestFailures()
{
   StartTest(_T("Failures"));
   time_t now = 1000000;

   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::NONE, 0, now, 0, 0, AICheckVerdict::FAILED) == AICheckTransition::FAILURE);
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::QUIET, 0, now, 0, 0, AICheckVerdict::FAILED) == AICheckTransition::FAILURE);
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FIRED, now - 60, now, 0, 0, AICheckVerdict::FAILED) == AICheckTransition::FAILURE);
   // Recovery from error: quiet is not a clear (nothing was fired from the check's point of view)
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FAILED, 0, now, 0, 0, AICheckVerdict::QUIET) == AICheckTransition::NONE);
   // Fired after error is an edge, still subject to cooldown
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FAILED, 0, now, 0, 0, AICheckVerdict::FIRED) == AICheckTransition::FIRE_EDGE);
   AssertTrue(EvaluateAICheckTransition(AICheckVerdict::FAILED, now - 10, now, 300, 0, AICheckVerdict::FIRED) == AICheckTransition::SUPPRESSED);
   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   TestQuietResults();
   TestStringResult();
   TestHashResult();
   TestHashResultDefaults();
   TestHashInvalidSeverity();
   TestErrorResults();
   TestEdgeTransitions();
   TestRenotify();
   TestCooldown();
   TestFailures();

   return 0;
}
