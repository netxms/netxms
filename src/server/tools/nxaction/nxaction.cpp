/* 
** nxaction - command line tool used to execute preconfigured actions 
**            on NetXMS agent
** Copyright (C) 2004-2023 Raden Solutions
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
** File: nxaction.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxsrvapi.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxaction)

/**
 * Static fields
 */
static bool s_showOutput = false;

/**
 * Output callback
 */
static void OutputCallback(ActionCallbackEvent e, const TCHAR *data, void *arg)
{
   if (e == ACE_DATA)
      _tprintf(_T("%s"), data);
}

/**
 * Parse nxaction additional options
 */
static bool ParseAdditionalOptionCb(const char ch, const TCHAR *optarg)
{
   switch(ch)
   {
      case 'o':
         s_showOutput = true;
         break;
      default:
         break;
   }
   return true;
}

/**
 * Validate argument count
 */
static bool IsArgMissingCb(int currentCount)
{
   return currentCount < 2;
}

/**
 * Execute command callback
 */
static int ExecuteCommandCb(AgentConnection *conn, int argc, TCHAR **argv, int optind, RSA_KEY serverKey)
{
   StringList list;
   int count = std::min(argc - optind - 2, 256);
   for(int i = 0, k = optind + 2; i < count; i++, k++)
      list.add(argv[k]);
   uint32_t rcc = conn->executeCommand(argv[optind + 1], list, s_showOutput, OutputCallback);
   if (rcc == ERR_SUCCESS)
      _tprintf(_T("Action executed successfully\n"));
   else
      _tprintf(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Startup
 */
#ifdef _WIN32
int _tmain(int argc, TCHAR *argv[])
#else
int main(int argc, char *argv[])
#endif
{
   ServerCommandLineTool tool;
   tool.argc = argc;
   tool.argv = argv;
   tool.displayName = _T("Agent Action Executor");
   tool.mainHelpText = _T("Usage: nxaction [<options>] <host> <action> [<action args>]\n")
                       _T("Tool specific options are:\n")
                       _T("   -o           : Show action's output.\n");
   tool.additionalOptions = "o";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}
