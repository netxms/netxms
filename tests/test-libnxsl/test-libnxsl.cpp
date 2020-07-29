#include <nms_common.h>
#include <nms_util.h>
#include <nxsl.h>
#include <testtools.h>

static TCHAR *s_testScriptDirectory = nullptr;

static const TCHAR *s_prog1 = _T("a = 1;\nb = 2;\nreturn a + b;");
static const TCHAR *s_prog2 = _T("a = 1;\nb = {;\nreturn a + b;");

/**
 * Test NXSL compiler
 */
static void TestCompiler()
{
   TCHAR errorMessage[256];
   int errorLine = -1;

   StartTest(_T("NXSLCompile"));

   NXSL_Program *p = NXSLCompile(s_prog1, errorMessage, 256, &errorLine);
   AssertNotNull(p);
   AssertEquals(errorLine, -1);
   delete p;

   p = NXSLCompile(s_prog2, errorMessage, 256, &errorLine);
   AssertNull(p);
   AssertEquals(errorLine, 2);
   AssertTrue(!_tcscmp(errorMessage, _T("Error in line 2: syntax error, unexpected '{'")));

   EndTest();

   StartTest(_T("NXSLCompileAndCreateVM"));

   NXSL_VM *vm = NXSLCompileAndCreateVM(s_prog1, errorMessage, 256, new NXSL_Environment());
   AssertNotNull(vm);
   delete vm;

   vm = NXSLCompileAndCreateVM(s_prog2, errorMessage, 256, new NXSL_Environment());
   AssertNull(vm);
   AssertTrue(!_tcscmp(errorMessage, _T("Error in line 2: syntax error, unexpected '{'")));

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

   TCHAR errorMessage[256];
   NXSL_VM *vm = NXSLCompileAndCreateVM(source, errorMessage, 256, env);
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

#ifndef UNICODE
   SetDefaultCodepage("CP1251"); // Some tests contain cyrillic symbols
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
   RunTestScript(_T("addr.nxsl"));
   RunTestScript(_T("arrays.nxsl"));
   RunTestScript(_T("base64.nxsl"));
   RunTestScript(_T("control.nxsl"));
   RunTestScript(_T("gethost.nxsl"));
   RunTestScript(_T("globals.nxsl"));
   RunTestScript(_T("json.nxsl"));
   RunTestScript(_T("like.nxsl"));
   RunTestScript(_T("math.nxsl"));
   RunTestScript(_T("regexp.nxsl"));
   RunTestScript(_T("strings.nxsl"));
   RunTestScript(_T("try-catch.nxsl"));
   RunTestScript(_T("types.nxsl"));
   RunTestScript(_T("with.nxsl"));

#ifdef UNICODE
   MemFree(s_testScriptDirectory);
#endif

   return 0;
}
