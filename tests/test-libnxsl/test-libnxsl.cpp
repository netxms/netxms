#include <nms_common.h>
#include <nms_util.h>
#include <nxsl.h>
#include <testtools.h>
#include <netxms-version.h>

static TCHAR *s_testScriptDirectory = nullptr;

static const TCHAR *s_prog1 = _T("a = 1;\nb = 2;\nreturn a + b;");
static const TCHAR *s_prog2 = _T("a = 1;\nb = {;\nreturn a + b;");
static const TCHAR *s_prog3 = _T("a = substr('abc', 1, 1);");

/**
 * Test NXSL compiler
 */
static void TestCompiler()
{
   StartTest(_T("NXSLCompile"));

   NXSL_Environment compileTimeEnvironment;
   NXSL_CompilationDiagnostic compileDiag;

   NXSL_Program *p = NXSLCompile(s_prog1, &compileTimeEnvironment, &compileDiag);
   AssertNotNull(p);
   AssertEquals(compileDiag.errorLineNumber, -1);
   delete p;
   compileDiag.reset();

   p = NXSLCompile(s_prog2, &compileTimeEnvironment, &compileDiag);
   AssertNull(p);
   AssertEquals(compileDiag.errorLineNumber, 2);
   // Some BISON versions may not report ", unexpected '{'" after "syntax error"
   AssertTrue(!_tcsncmp(compileDiag.errorText, _T("Error in line 2: syntax error"), 29));
   compileDiag.reset();

   p = NXSLCompile(s_prog3, &compileTimeEnvironment, &compileDiag);
   AssertNotNull(p);
   AssertEquals(compileDiag.errorLineNumber, -1);
   AssertFalse(compileDiag.warnings.isEmpty());
   AssertEquals(compileDiag.warnings.get(0)->lineNumber, 1);
   AssertTrue(!_tcscmp(compileDiag.warnings.get(0)->message, _T("Function \"substr\" is deprecated")));
   delete p;
   compileDiag.reset();

   EndTest();

   StartTest(_T("NXSLCompileAndCreateVM"));

   NXSL_VM *vm = NXSLCompileAndCreateVM(s_prog1, new NXSL_Environment(), &compileDiag);
   AssertNotNull(vm);
   delete vm;
   compileDiag.reset();

   vm = NXSLCompileAndCreateVM(s_prog2, new NXSL_Environment(), &compileDiag);
   AssertNull(vm);
   AssertTrue(!_tcsncmp(compileDiag.errorText, _T("Error in line 2: syntax error"), 29));

   EndTest();
}

static void StopThread(NXSL_VM *vm)
{
   ThreadSleepMs(1000);
   vm->stop();
}

/**
 * Test NXSL VM stop function
 */
static void TestStop()
{
   StartTest(_T("NXSL_VM::stop"));

   NXSL_CompilationDiagnostic compileDiag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(_T("c = 0; for(i = 0; i < 100000000; i++) c++;"), new NXSL_Environment(), &compileDiag);
   AssertNotNull(vm);

   ThreadCreate(StopThread, vm);
   AssertFalse(vm->run());
   AssertEquals(vm->getErrorCode(), NXSL_ERR_EXECUTION_ABORTED);
   delete vm;

   EndTest();
}

/**
 * Run test NXSL script
 */
static void RunTestScript(const TCHAR *name)
{
   StartTest(name);

   TCHAR path[MAX_PATH];
   if (s_testScriptDirectory != nullptr)
   {
      _tcslcpy(path, s_testScriptDirectory, MAX_PATH);
      if (path[_tcslen(path) - 1] != FS_PATH_SEPARATOR_CHAR)
         _tcslcat(path, FS_PATH_SEPARATOR, MAX_PATH);
   }
   else
   {
      GetNetXMSDirectory(nxDirShare, path);
      _tcslcat(path, FS_PATH_SEPARATOR _T("nxsltest") FS_PATH_SEPARATOR, MAX_PATH);
   }
   _tcslcat(path, name, MAX_PATH);

   TCHAR *source = NXSLLoadFile(path);
   AssertNotNull(source);

   NXSL_Environment *env = new NXSL_Environment();
   env->registerIOFunctions();

   NXSL_CompilationDiagnostic compileDiag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(source, env, &compileDiag);
   MemFree(source);
   AssertNotNull(vm);

   AssertTrue(vm->run());
   AssertNotNull(vm->getResult());
   AssertTrue(vm->getResult()->isInteger());
   AssertEquals(vm->getResult()->getValueAsInt32(), 0);

   delete vm;
   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

#ifndef UNICODE
   SetDefaultCodepage("CP1251"); // Some tests contain cyrillic symbols
#endif

#ifdef _WIN32
   SetEnvironmentVariable(_T("TZ"), _T("EET-02EES"));
   _tzset();
#else
   SetEnvironmentVariable(_T("TZ"), _T("EET-02EEST-03,M3.5.0,M10.5.0"));
#if HAVE_TZSET
   tzset();
#endif
#endif

   if (argc > 1)
   {
#ifdef UNICODE
      s_testScriptDirectory = WideStringFromMBStringSysLocale(argv[1]);
#else
      s_testScriptDirectory = argv[1];
#endif
   }

   TestCompiler();
   TestStop();
   RunTestScript(_T("addr.nxsl"));
   RunTestScript(_T("arrays.nxsl"));
   RunTestScript(_T("base64.nxsl"));
   RunTestScript(_T("bitops.nxsl"));
   RunTestScript(_T("boolean.nxsl"));
   RunTestScript(_T("bytestream.nxsl"));
   RunTestScript(_T("control.nxsl"));
   RunTestScript(_T("gethost.nxsl"));
   RunTestScript(_T("globals.nxsl"));
   RunTestScript(_T("hashmap.nxsl"));
   RunTestScript(_T("inetaddr.nxsl"));
   RunTestScript(_T("invoke.nxsl"));
   RunTestScript(_T("json.nxsl"));
   RunTestScript(_T("like.nxsl"));
   RunTestScript(_T("macaddr.nxsl"));
   RunTestScript(_T("math.nxsl"));
   RunTestScript(_T("range.nxsl"));
   RunTestScript(_T("regexp.nxsl"));
   RunTestScript(_T("strings.nxsl"));
   RunTestScript(_T("time.nxsl"));
   RunTestScript(_T("try-catch.nxsl"));
   RunTestScript(_T("types.nxsl"));
   RunTestScript(_T("with.nxsl"));

#ifdef UNICODE
   MemFree(s_testScriptDirectory);
#endif

   return 0;
}
