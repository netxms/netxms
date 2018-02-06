#include <nms_common.h>
#include <nms_util.h>
#include <nxproc.h>
#include <testtools.h>

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
}
