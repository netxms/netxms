#include <nms_common.h>
#include <nms_util.h>
#include <nxsl.h>
#include <nxcldefs.h>
#include <testtools.h>
#include <netxms-version.h>

static TCHAR *s_testScriptDirectory = nullptr;

static const char *s_prog1 = "a = 1;\nb = 2;\nreturn a + b;";
static const char *s_prog2 = "a = 1;\nb = {;\nreturn a + b;";
static const char *s_prog3 = "a = substr('abc', 1, 1);";
static const char *s_prog4 = "a = %{ \"k\": 1 };\nb = %{};\nreturn a[\"k\"] + b.size;";

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

   p = NXSLCompile(s_prog4, &compileTimeEnvironment, &compileDiag);
   AssertNotNull(p);
   AssertEquals(compileDiag.errorLineNumber, -1);
   AssertEquals(compileDiag.warnings.size(), 2);
   AssertEquals(compileDiag.warnings.get(0)->lineNumber, 1);
   AssertTrue(!_tcscmp(compileDiag.warnings.get(0)->message, _T("Hash map initialization with \"%{}\" is deprecated, use \"{}\" instead")));
   AssertEquals(compileDiag.warnings.get(1)->lineNumber, 2);
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

   char *source = LoadFileAsUTF8String(path);
   AssertNotNull(source);

   NXSL_Environment *env = new NXSL_Environment();
   env->registerIOFunctions();

   NXSL_CompilationDiagnostic compileDiag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(source, env, &compileDiag);
   MemFree(source);
   AssertNotNull(vm);

   bool success = vm->run();
   if (!success)
      WriteToTerminalEx(_T("\n   Script error: %s\n"), vm->getErrorText());
   AssertTrue(success);
   AssertNotNull(vm->getResult());
   AssertTrue(vm->getResult()->isInteger());
   AssertEquals(vm->getResult()->getValueAsInt32(), 0);

   delete vm;
   EndTest();
}

/**
 * main()
 */
/**
 * Security context that denies every access check
 */
class DenyAllSecurityContext : public NXSL_SecurityContext
{
public:
   virtual bool validateAccess(int subsystem, uint64_t requiredAccess, const void *object) override
   {
      return false;
   }
};

/**
 * Security context that grants only exact match of given system access rights
 */
class SystemAccessSecurityContext : public NXSL_SecurityContext
{
private:
   uint64_t m_grantedAccess;

public:
   SystemAccessSecurityContext(uint64_t grantedAccess) : NXSL_SecurityContext()
   {
      m_grantedAccess = grantedAccess;
   }

   virtual bool validateAccess(int subsystem, uint64_t requiredAccess, const void *object) override
   {
      return (subsystem == NXSL_AC_SYSTEM) && (requiredAccess == m_grantedAccess);
   }
};

/**
 * Run inline script with file I/O functions enabled and return its integer result
 */
static int32_t RunFileIOScript(const char *source, const TCHAR *fileName, NXSL_SecurityContext *securityContext)
{
   NXSL_Environment *env = new NXSL_Environment();
   env->registerIOFunctions();

   NXSL_CompilationDiagnostic compileDiag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(source, env, &compileDiag);
   AssertNotNull(vm);
   if (securityContext != nullptr)
      vm->setSecurityContext(securityContext);

   ObjectRefArray<NXSL_Value> args(1, 1);
   args.add(vm->createValue(fileName));
   bool success = vm->run(args);
   if (!success)
      WriteToTerminalEx(_T("\n   Script error: %s\n"), vm->getErrorText());
   AssertTrue(success);
   AssertNotNull(vm->getResult());
   AssertTrue(vm->getResult()->isInteger());
   int32_t result = vm->getResult()->getValueAsInt32();
   delete vm;
   return result;
}

/**
 * Script that succeeds only if file I/O functions are permitted
 */
static const char *s_allowedScript =
   "import IO;\n"
   "if (!FileAccess($1, 0)) return 1;\n"
   "f = OpenFile($1);\n"
   "if (f == null) return 2;\n"
   "f.close();\n"
   "return 0;";

/**
 * Script that succeeds only if every file I/O function is denied
 */
static const char *s_deniedScript =
   "import IO;\n"
   "if (FileAccess($1, 0)) return 1;\n"
   "if (OpenFile($1) != null) return 2;\n"
   "if (OpenFile($1, \"w\") != null) return 3;\n"
   "if (CopyFile($1, $1 .. \".copy\")) return 4;\n"
   "if (RenameFile($1, $1 .. \".renamed\")) return 5;\n"
   "if (CreateDirectory($1 .. \".dir\")) return 6;\n"
   "if (RemoveDirectory($1 .. \".dir\")) return 7;\n"
   "if (DeleteFile($1)) return 8;\n"
   "return 0;";

/**
 * Test access control for file I/O functions
 */
static void TestFileIOAccessControl()
{
   StartTest(_T("File I/O access control"));

   TCHAR fileName[MAX_PATH];
   GetNetXMSDirectory(nxDirData, fileName);
   _tcslcat(fileName, FS_PATH_SEPARATOR _T("nxsl-test-file-io.tmp"), MAX_PATH);
   FILE *f = _tfopen(fileName, _T("w"));
   if (f == nullptr)
   {
      // Fall back to current directory if data directory is not writable
      _tcscpy(fileName, _T("nxsl-test-file-io.tmp"));
      f = _tfopen(fileName, _T("w"));
   }
   AssertNotNull(f);
   fputs("test", f);
   fclose(f);

   // Without security context every function works
   AssertEquals(RunFileIOScript(s_allowedScript, fileName, nullptr), 0);

   // Denying security context: functions fail without touching the file system
   AssertEquals(RunFileIOScript(s_deniedScript, fileName, new DenyAllSecurityContext()), 0);
   AssertTrue(_taccess(fileName, 0) == 0);

   // Security context granting file I/O system access right: every function works
   AssertEquals(RunFileIOScript(s_allowedScript, fileName, new SystemAccessSecurityContext(SYSTEM_ACCESS_NXSL_FILE_IO)), 0);

   // Security context granting different system access right: functions fail
   AssertEquals(RunFileIOScript(s_deniedScript, fileName, new SystemAccessSecurityContext(SYSTEM_ACCESS_HTTP_REQUESTS)), 0);
   AssertTrue(_taccess(fileName, 0) == 0);

   // Without security context again - file is intact and can be deleted
   AssertEquals(RunFileIOScript("import IO;\nreturn DeleteFile($1) ? 0 : 1;", fileName, nullptr), 0);
   AssertTrue(_taccess(fileName, 0) != 0);

   EndTest();
}

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
   tzset();
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
   TestFileIOAccessControl();
   RunTestScript(_T("addr.nxsl"));
   RunTestScript(_T("arrays.nxsl"));
   RunTestScript(_T("base64.nxsl"));
   RunTestScript(_T("bitops.nxsl"));
   RunTestScript(_T("boolean.nxsl"));
   RunTestScript(_T("bytestream.nxsl"));
   RunTestScript(_T("concat.nxsl"));
   RunTestScript(_T("compound-assign.nxsl"));
   RunTestScript(_T("control.nxsl"));
   RunTestScript(_T("do-first.nxsl"));
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
   RunTestScript(_T("while-first.nxsl"));
   RunTestScript(_T("with.nxsl"));

#ifdef UNICODE
   MemFree(s_testScriptDirectory);
#endif

   return 0;
}
