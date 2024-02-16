/*
** NetXMS database manager library
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
** File: confirm.cpp
**
**/

#include "libnxdbmgr.h"

#ifdef _WIN32
#include <conio.h>
#endif

/**
 * "Operation in progress" flag
 */
static bool m_operationInProgress = false;

/**
 * Set "operation in progress" flag
 */
void SetOperationInProgress(bool inProgress)
{
   m_operationInProgress = inProgress;
}

/**
 * GUI mode
 */
static bool s_guiMode = false;

/**
 * Set GUI mode
 */
void LIBNXDBMGR_EXPORTABLE SetDBMgrGUIMode(bool guiMode)
{
   s_guiMode = guiMode;
}

/**
 * Forced confirmation mode
 */
static bool s_forcedConfirmation = false;

/**
 * Set force configrmation mode
 */
void LIBNXDBMGR_EXPORTABLE SetDBMgrForcedConfirmationMode(bool forced)
{
   s_forcedConfirmation = forced;
}

/**
 * Flag for "all" answer in GetYesNo
 */
static bool s_yesForAll = false;
static bool s_noForAll = false;


/**
 * Fail execution if fix required
 */
static bool g_failOnFix = false;

/**
 * Set GUI mode
 */
void LIBNXDBMGR_EXPORTABLE SetDBMgrFailOnFixMode(bool fail)
{
   g_failOnFix = fail;
}

/**
 * Get Yes or No answer from keyboard
 */
static bool GetYesNoInternal(bool allowBulk, const TCHAR *format, va_list args)
{
   if (g_failOnFix)
   {
      _tprintf(_T("\n DB fix required. Exit check.\n"));
      exit(1);
   }

   if (s_guiMode)
   {
      if (s_forcedConfirmation || s_yesForAll)
         return true;
      if (s_noForAll)
         return false;

      TCHAR message[8192];
      _vsntprintf(message, 8192, format, args);

#ifdef _WIN32
      return MessageBox(nullptr, message, _T("NetXMS Database Manager"), MB_YESNO | MB_ICONQUESTION) == IDYES;
#else
      return false;
#endif
   }
   else
   {
      if (m_operationInProgress)
         _tprintf(_T("\n"));
      _vtprintf(format, args);
      _tprintf(allowBulk ? _T(" (Yes/No/All/Skip) ") : _T(" (Yes/No) "));

      if (s_forcedConfirmation || s_yesForAll)
      {
         _tprintf(_T("Y\n"));
         return true;
      }
      else if (s_noForAll)
      {
         _tprintf(_T("N\n"));
         return false;
      }
      else
      {
#ifdef _WIN32
         while(true)
         {
            int ch = _getch();
            if ((ch == 'y') || (ch == 'Y'))
            {
               _tprintf(_T("Y\n"));
               return true;
            }
            if (allowBulk && ((ch == 'a') || (ch == 'A')))
            {
               _tprintf(_T("A\n"));
               s_yesForAll = true;
               return true;
            }
            if ((ch == 'n') || (ch == 'N'))
            {
               _tprintf(_T("N\n"));
               return false;
            }
            if (allowBulk && ((ch == 's') || (ch == 'S')))
            {
               _tprintf(_T("S\n"));
               s_noForAll = true;
               return false;
            }
         }
#else
         fflush(stdout);
         TCHAR buffer[16];
         _fgetts(buffer, 16, stdin);
         Trim(buffer);
         if (allowBulk)
         {
            if ((buffer[0] == 'a') || (buffer[0] == 'A'))
            {
               s_yesForAll = true;
               return true;
            }
            if ((buffer[0] == 's') || (buffer[0] == 'S'))
            {
               s_noForAll = true;
               return false;
            }
         }
         return ((buffer[0] == 'y') || (buffer[0] == 'Y'));
#endif
      }
   }
}

/**
 * Get Yes or No answer from keyboard
 */
bool LIBNXDBMGR_EXPORTABLE GetYesNo(const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   bool result = GetYesNoInternal(false, format, args);
   va_end(args);
   return result;
}

/**
 * Get Yes or No answer from keyboard (with bulk answer options)
 */
bool LIBNXDBMGR_EXPORTABLE GetYesNoEx(const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   bool result = GetYesNoInternal(true, format, args);
   va_end(args);
   return result;
}

/**
 * Reset bulk yes/no answer
 */
void LIBNXDBMGR_EXPORTABLE ResetBulkYesNo()
{
   s_yesForAll = false;
   s_noForAll = false;
}
