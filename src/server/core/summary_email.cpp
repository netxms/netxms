/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Raden Solutions
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
** File: summary_email.cpp
**
**/

#include "nxcore.h"

static TCHAR s_recipients[MAX_CONFIG_VALUE];
static String s_severities[8] = { _T("<td style=\"background:rgb(0, 192, 0)\">Normal</td>"), _T("<td style=\"background:rgb(0, 255, 255)\">Warning</td>"),
                                  _T("<td style=\"background:rgb(231, 226, 0)\">Minor</td>"), _T("<td style=\"background:rgb(255, 128, 0)\">Major</td>"),
                                  _T("<td style=\"background:rgb(192, 0, 0)\">Critical</td>"), _T("<td style=\"background:rgb(0, 0, 128)\">Unknown</td>"),
                                  _T("<td style=\"background:darkred\">Terminate</td>"), _T("<td style=\"background:green\">Resolve</td>") };
static String s_states[4] = { _T("<td style=\"background:gold\">Outstanding</td>"), _T("<td style=\"background:yellow\">Acknowledged</td>"),
                              _T("<td style=\"background:green\">Resolved</td>"), _T("<td style=\"background:darkred\">Terminated</td>") };

/**
 * Formats time in seconds into date - dd.mm.yy HH:MM:SS
 */
static TCHAR *FormatDate(time_t t, TCHAR *buffer, size_t bufferSize)
{
#if HAVE_LOCALTIME_R
   struct tm ltmBuffer;
   struct tm *loc = localtime_r(&t, &ltmBuffer);
#else
   struct tm *loc = localtime(&t);
#endif
   _tcsftime(buffer, bufferSize, _T("%d.%m.%y %H:%M:%S"), loc);
   return buffer;
}

/**
 * Creates a HTML formated alarm summary to be used in an email
 */
static String CreateAlarmSummary()
{
   TCHAR timeFmt[128], *objName, *message;

   String summary(_T("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"));
   summary.append(_T("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"));
   summary.append(_T("<head>\n"));
   summary.append(_T("<meta charset=\"UTF-8\">\n"));
   summary.append(_T("<meta http-equiv=\"Content-Type\" content=\"text/html;charset=UTF-8\">\n"));
   summary.append(_T("<style>\n"));
   summary.append(_T("table, th, td {\n"));
   summary.append(_T("border: 1px solid black;\n"));
   summary.append(_T("}\n"));
   summary.append(_T("</style>\n"));
   summary.append(_T("</head>\n"));
   summary.append(_T("<body>\n"));
   summary.append(_T("<table style=\"width:100%\">\n"));
   summary.append(_T("<tr>\n"));
   summary.append(_T("<th>Severity</th>\n"));
   summary.append(_T("<th>State</th>\n"));
   summary.append(_T("<th>Source</th>\n"));
   summary.append(_T("<th>Message</th>\n"));
   summary.append(_T("<th>Count</th>\n"));
   summary.append(_T("<th>HelpDesk ID</th>\n"));
   summary.append(_T("<th>Ack/Resolved by</th>\n"));
   summary.append(_T("<th>Created</th>\n"));
   summary.append(_T("<th>Last change</th>\n"));
   summary.append(_T("</tr>\n"));

   ObjectArray<Alarm> *alarms = GetAlarms(0);
   for(int i = 0; i < alarms->size(); i++)
   {
      summary.append(_T("<tr>\n"));
      summary.append(s_severities[alarms->get(i)->getCurrentSeverity()]);
      summary.append(s_states[alarms->get(i)->getState()]);
      summary.append(_T("<td>"));
      objName = EscapeStringForXML(GetObjectName(alarms->get(i)->getSourceObject(), _T("Unknown node")), -1);
      summary.append(objName);
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      message = EscapeStringForXML(alarms->get(i)->getMessage(), -1);
      summary.append(message);
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(alarms->get(i)->getRepeatCount());
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(alarms->get(i)->getHelpDeskRef());
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(alarms->get(i)->getResolvedByUser());
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(FormatDate(alarms->get(i)->getCreationTime(), timeFmt, 128));
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(FormatDate(alarms->get(i)->getLastChangeTime(), timeFmt, 128));
      summary.append(_T("</td>\n"));
      summary.append(_T("</tr>\n"));

      free(objName);
      free(message);
   }

   summary.append(_T("</table>"));
   summary.append(_T("</body>"));
   summary.append(_T("</html>"));

   delete(alarms);
   return summary;
}

/**
 * Enable alarm summary e-mails by creating a scheduled task.
 * If task has already been set, updates schedule
 */
void EnableAlarmSummaryEmails()
{
   TCHAR schedule[MAX_DB_STRING];
   ConfigReadStr(_T("AlarmSummaryEmailSchedule"), schedule, MAX_DB_STRING, _T("0 0 * * *"));

   ScheduledTask *task = FindScheduledTaskByHandlerId(ALARM_SUMMARY_EMAIL_TASK_ID);
   if (task != NULL)
   {
      if (_tcscmp(task->getSchedule(), schedule))
         UpdateScheduledTask(task->getId(), ALARM_SUMMARY_EMAIL_TASK_ID, schedule, _T(""), _T(""), 0, 0, SYSTEM_ACCESS_FULL, task->getFlags());
   }
   else
   {
      AddScheduledTask(ALARM_SUMMARY_EMAIL_TASK_ID, schedule, _T(""), 0, 0, SYSTEM_ACCESS_FULL, _T(""), 0);
   }
}

/**
 * Scheduled task handler - send summary email
 */
void SendAlarmSummaryEmail(const ScheduledTaskParameters *params)
{
   String summary = CreateAlarmSummary();
   time_t currTime;
   TCHAR timeFmt[128], subject[64];

   time(&currTime);
   FormatDate(currTime, timeFmt, 128);
   _sntprintf(subject, 64, _T("NetXMS Alarm Summary for %s"), timeFmt);

   TCHAR *next, *curr;

   ConfigReadStr(_T("AlarmSummaryEmailRecipients"), s_recipients, MAX_CONFIG_VALUE, _T("0"));
   curr = s_recipients;
   do
   {
      next = _tcschr(curr, _T(';'));
      if (next != NULL)
         *next = 0;
      StrStrip(curr);
      PostMail(curr, subject, summary, true);
      curr = next + 1;
   } while(next != NULL);
}
