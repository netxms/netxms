/*
** NetXMS - Network Management System
** Notification channel driver that writes messages to text file
** Copyright (C) 2019-2025 Raden Solutions
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
** File: shell.cpp
**
**/

#include <ncdrv.h>
#include <nxproc.h>

#define DEBUG_TAG _T("ncd.shell")

static const NCConfigurationTemplate s_config(true, true); 

/**
 * Process executor that logs output
 */
class OutputLoggingExecutor : public ProcessExecutor
{
protected:
   virtual void onOutput(const char *text, size_t length) override;

public:
   OutputLoggingExecutor(const TCHAR *command) : ProcessExecutor(command, true, true)
   {
      m_sendOutput = true;
   };
};

/**
 * Log output executor output handler
 */
void OutputLoggingExecutor::onOutput(const char *text, size_t length)
{
   TCHAR *buffer;
#ifdef UNICODE
   buffer = WideStringFromMBStringSysLocale(text);
#else
   buffer = MemCopyStringA(text);
#endif
   StringList outputLines = String::split(buffer, _tcslen(buffer), _T("\n"));
   MemFree(buffer);
   for (int i = 0; i < outputLines.size(); i++)
   {
      nxlog_write_tag(5, DEBUG_TAG, _T("Output: %s"), outputLines.get(i));
   }
}

/**
 * Shell command notification driver class
 */
class ShellDriver : public NCDriver
{
private:
   String m_command;

   ShellDriver(const TCHAR *command) : m_command(command) { }

public:
   virtual int send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;

   static ShellDriver *createInstance(Config *config);
};

/**
 * Create driver instance
 */
ShellDriver *ShellDriver::createInstance(Config *config)
{
   const TCHAR * command = config->getValue(_T("/Shell/Command"));
   if (command == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Driver configuration not found"));
      return nullptr;
   }
   return new ShellDriver(command);
}

/**
 * Driver send method
 */
int ShellDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   StringBuffer command;
   bool execSyntax = (m_command[0] == '[');
   bool escapeQuote = false;
   const TCHAR *s = m_command;
   while(*s != 0)
   {
      switch(*s)
      {
         case '$':
            s++;
            if (*s == '{')
            {
               s++;
               StringBuffer name;
               while((*s != '}') && (*s != 0))
                  name.append(*s++);
               if (*s == 0)
                  break;
               s++;

               const TCHAR *value;
               if (!_tcsicmp(name, _T("recipient")))
                  value = recipient;
               else if (!_tcsicmp(name, _T("subject")))
                  value = subject;
               else if (!_tcsicmp(name, _T("text")))
                  value = body;
               else
                  value = nullptr;

               if (value != nullptr)
               {
                  if (execSyntax && escapeQuote)
                  {
                     for(const TCHAR *p = value; *p != 0; p++)
                     {
                        command.append(*p);
                        if (*p == _T('\''))
                           command.append(_T('\''));
                     }
                  }
                  else
                  {
                     command.append(value);
                  }
               }
            }
            else
            {
               command.append(_T('$'));
            }
            break;
         case '\'':
            escapeQuote = !escapeQuote;
            /* no break */
         default:
            command.append(*s);
            s++;
            break;
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Executing command %s"), command.cstr());
   auto procexec = new OutputLoggingExecutor(command);
   if (procexec->execute())
      return 0;
   delete procexec;
   return -1;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(Shell, &s_config)
{
   return ShellDriver::createInstance(config);
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
