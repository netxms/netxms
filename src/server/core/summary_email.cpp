/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
#include <nms_users.h>

static TCHAR s_recipients[MAX_CONFIG_VALUE_LENGTH] = _T("");
static const TCHAR *s_severities[8] = { _T("<td style=\"background:rgb(0, 192, 0)\">Normal</td>"), _T("<td style=\"background:rgb(0, 255, 255)\">Warning</td>"),
         _T("<td style=\"background:rgb(231, 226, 0)\">Minor</td>"), _T("<td style=\"background:rgb(255, 128, 0)\">Major</td>"),
         _T("<td style=\"background:rgb(192, 0, 0)\">Critical</td>"), _T("<td style=\"background:rgb(0, 0, 128)\">Unknown</td>"),
         _T("<td style=\"background:darkred\">Terminate</td>"), _T("<td style=\"background:green\">Resolve</td>") };
static const TCHAR *s_states[4] = { _T("<td style=\"background:gold\">Outstanding</td>"), _T("<td style=\"background:yellow\">Acknowledged</td>"),
         _T("<td style=\"background:green\">Resolved</td>"), _T("<td style=\"background:darkred\">Terminated</td>") };

/**
 * Creates a HTML formated alarm summary to be used in an email
 */
static StringBuffer CreateAlarmSummary()
{
   StringBuffer summary(_T("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"));
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

   TCHAR userName[MAX_USER_NAME], timeFmt[128];
   ObjectArray<Alarm> *alarms = GetAlarms(0);
   for(int i = 0; i < alarms->size(); i++)
   {
      Alarm *alarm = alarms->get(i);
      shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
      if (object == nullptr)
         continue;

      summary.append(_T("<tr>\n"));
      summary.append(s_severities[alarm->getCurrentSeverity()]);
      summary.append(s_states[alarm->getState() & ALARM_STATE_MASK]);
      summary.append(_T("<td>"));
      summary.append(EscapeStringForXML2(object->getName()));
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(EscapeStringForXML2(alarm->getMessage()));
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(alarm->getRepeatCount());
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(alarm->getHelpDeskRef());
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      if ((alarm->getState() & ALARM_STATE_MASK) == ALARM_STATE_ACKNOWLEDGED)
         summary.append(ResolveUserId(alarm->getAckByUser(), userName, true));
      else if ((alarm->getState() & ALARM_STATE_MASK) == ALARM_STATE_RESOLVED)
         summary.append(ResolveUserId(alarm->getResolvedByUser(), userName, true));
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(FormatTimestamp(alarm->getCreationTime(), timeFmt));
      summary.append(_T("</td>\n"));
      summary.append(_T("<td>"));
      summary.append(FormatTimestamp(alarm->getLastChangeTime(), timeFmt));
      summary.append(_T("</td>\n"));
      summary.append(_T("</tr>\n"));
   }

   summary.append(_T("</table>"));
   summary.append(_T("</body>"));
   summary.append(_T("</html>"));

   delete alarms;
   return summary;
}

/**
 * Enable alarm summary e-mails by creating a scheduled task.
 * If task has already been set, updates schedule
 */
void EnableAlarmSummaryEmails()
{
   TCHAR schedule[MAX_DB_STRING];
   ConfigReadStr(_T("Alarms.SummaryEmail.Schedule"), schedule, MAX_DB_STRING, _T("0 0 * * *"));

   ScheduledTask *task = FindScheduledTaskByHandlerId(ALARM_SUMMARY_EMAIL_TASK_ID);
   if (task != NULL)
   {
      if (_tcscmp(task->getSchedule(), schedule))
         UpdateRecurrentScheduledTask(task->getId(), ALARM_SUMMARY_EMAIL_TASK_ID, schedule, _T(""), nullptr, _T(""), 0, 0, SYSTEM_ACCESS_FULL);
   }
   else
   {
      AddRecurrentScheduledTask(ALARM_SUMMARY_EMAIL_TASK_ID, schedule, _T(""), nullptr, 0, 0, SYSTEM_ACCESS_FULL, _T(""), nullptr, true);
   }
}

/**
 * Scheduled task handler - send summary email
 */
void SendAlarmSummaryEmail(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   StringBuffer summary = CreateAlarmSummary();
   time_t currTime;
   TCHAR timeFmt[128], subject[64];

   time(&currTime);
   FormatTimestamp(currTime, timeFmt);
   _sntprintf(subject, 64, _T("NetXMS Alarm Summary for %s"), timeFmt);

   TCHAR channelName[MAX_OBJECT_NAME];
   ConfigReadStr(_T("DefaultNotificationChannel.SMTP.Html"), channelName, MAX_OBJECT_NAME, _T("SMTP-HTML"));
   ConfigReadStr(_T("Alarms.SummaryEmail.Recipients"), s_recipients, MAX_CONFIG_VALUE_LENGTH, _T(""));
   SendNotification(channelName, s_recipients, subject, summary, 0, 0, uuid::NULL_UUID);
}
