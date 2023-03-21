#include <nms_common.h>
#include <nms_util.h>
#include <nxproc.h>
#include <nxcpapi.h>
#include <testtools.h>

#ifndef _WIN32
#include <signal.h>
#endif

/**
 * Worker for process executor test
 */
void TestProcessExecutorWorker()
{
   ThreadSleep(2);
}

/**
 * Test process executor
 */
void TestProcessExecutor(const char *procname)
{
   TCHAR cmdLine[MAX_PATH];
   _sntprintf(cmdLine, MAX_PATH, _T("%hs @proc"), procname);

   StartTest(_T("Process executor - create"));
   ProcessExecutor e(cmdLine);
   AssertTrue(e.execute());
   AssertTrue(e.isRunning());
   EndTest();

   StartTest(_T("Process executor - wait timeout"));
   AssertFalse(e.waitForCompletion(500));
   EndTest();

   StartTest(_T("Process executor - wait"));
   AssertTrue(e.waitForCompletion(5000));
   EndTest();

   StartTest(_T("Process executor - not running"));
   AssertFalse(e.isRunning());
   EndTest();

   StartTest(_T("Process executor - re-run"));
   e.execute();
   AssertTrue(e.isRunning());
   EndTest();

   StartTest(_T("Process executor - stop"));
   e.stop();
   ThreadSleepMs(100);
   AssertFalse(e.isRunning());
   EndTest();

   StartTest(_T("Process executor - wait after stop"));
   AssertTrue(e.waitForCompletion(500));
   EndTest();

   StartTest(_T("Process executor - create without shell"));
   ProcessExecutor e2(cmdLine, false);
   AssertTrue(e2.execute());
   AssertTrue(e2.isRunning());
   e2.stop();
   ThreadSleepMs(100);
   AssertFalse(e2.isRunning());
   EndTest();

   StartTest(_T("Process executor - [] command line syntax"));
   _sntprintf(cmdLine, MAX_PATH, _T("['%hs', '@proc']"), procname);
   ProcessExecutor e3(cmdLine);
   AssertTrue(e3.execute());
   AssertTrue(e3.isRunning());
   e3.stop();
   ThreadSleepMs(100);
   AssertFalse(e3.isRunning());
   EndTest();
}

/**
 * Sub-process request handler
 */
NXCPMessage *TestSubProcessRequestHandler(UINT16 command, const void *data, size_t dataSize)
{
   if (command == SPC_USER)
   {
      NXCPMessage *response = new NXCPMessage(SPC_REQUEST_COMPLETED, 0);
      return response;
   }
   return NULL;
}

/**
 * Test sub-process
 */
void TestSubProcess(const char *procname, bool debug)
{
   TCHAR cmdLine[MAX_PATH];
   _sntprintf(cmdLine, MAX_PATH, _T("%hs @subproc%s"), procname, debug ? _T(" -debug") : _T(""));

   StartTest(_T("Sub-process executor - create"));
   SubProcessExecutor e(_T("TEST"), cmdLine);
   AssertTrue(e.execute());
   AssertTrue(e.isRunning());
   EndTest();

   StartTest(_T("Sub-process executor - execute request"));
   void *response = nullptr;
   size_t responseLen = 0;
   AssertTrue(e.sendRequest(SPC_USER, "ECHO", 4, &response, &responseLen, 5000));
   MemFree(response);
   EndTest();

   StartTest(_T("Sub-process executor - request timeout"));
   AssertFalse(e.sendRequest(SPC_USER + 1, NULL, 0, NULL, NULL, 2000));
   EndTest();

   StartTest(_T("Sub-process executor - background restart"));
#ifdef _WIN32
   TerminateProcess(e.getProcessHandle(), 127);
#else
   kill(e.getProcessId(), SIGKILL);
#endif
   ThreadSleepMs(200);
   AssertFalse(e.isRunning());
   ThreadSleep(5);
   AssertTrue(e.isRunning());
   EndTest();

   StartTest(_T("Sub-process executor - stop"));
   e.stop();
   ThreadSleepMs(100);
   AssertFalse(e.isRunning());
   EndTest();

   StartTest(_T("Sub-process executor - wait after stop"));
   AssertTrue(e.waitForCompletion(500));
   EndTest();
}
