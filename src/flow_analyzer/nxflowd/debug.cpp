/*
** nxflowd - NetXMS Flow Collector Daemon
** Copyright (c) 2009-2012 Raden Solutions
*/

#include "nxflowd.h"


//
// Debug level
//

int g_debugLevel = 0;


//
// Debug printf - write debug record to log if level is less or equal current debug level
//

void DbgPrintf2(int level, const TCHAR *format, va_list args)
{
   TCHAR buffer[4096];

	if (level > g_debugLevel)
      return;     // Required application flag(s) not set

	_vsntprintf(buffer, 4096, format, args);
   nxlog_write(MSG_DEBUG, EVENTLOG_DEBUG_TYPE, "s", buffer);
}

void DbgPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   TCHAR buffer[4096];

   if (level > g_debugLevel)
      return;     // Required application flag(s) not set

   va_start(args, format);
   _vsntprintf(buffer, 4096, format, args);
   va_end(args);
   nxlog_write(MSG_DEBUG, EVENTLOG_DEBUG_TYPE, "s", buffer);
}
