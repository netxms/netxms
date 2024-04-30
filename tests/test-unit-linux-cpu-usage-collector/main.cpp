#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nxproc.h>
#include <testtools.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(test-unit-linux-cpu-usage-collector)

void TestCpu();

/**
 * Debug writer for logger
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != NULL)
      _tprintf(_T("[DEBUG/%-20s] "), tag);
   else
      _tprintf(_T("[DEBUG%-21s] "), _T(""));
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);
   if (argc > 1)
   {
      if (!strcmp(argv[1], "-debug"))
      {
         nxlog_set_debug_writer(DebugWriter);
         nxlog_set_debug_level(9);
      }
   }

   TestCpu();

   InitiateProcessShutdown();

   return 0;
}
