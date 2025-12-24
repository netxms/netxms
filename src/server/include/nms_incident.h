/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nms_incident.h
**
**/

#ifndef _nms_incident_h_
#define _nms_incident_h_

/**
 * Scheduled task handler ID for incident creation
 */
#define INCIDENT_CREATE_TASK_ID _T("Incident.CreateFromAlarm")

/**
 * Incident comment
 */
class IncidentComment
{
private:
   uint32_t m_id;
   uint32_t m_incidentId;
   time_t m_creationTime;
   uint32_t m_userId;
   TCHAR *m_text;

public:
   IncidentComment(uint32_t id, uint32_t incidentId, time_t creationTime, uint32_t userId, const TCHAR *text);
   IncidentComment(DB_RESULT hResult, int row);
   ~IncidentComment();

   uint32_t getId() const { return m_id; }
   uint32_t getIncidentId() const { return m_incidentId; }
   time_t getCreationTime() const { return m_creationTime; }
   uint32_t getUserId() const { return m_userId; }
   const TCHAR *getText() const { return m_text; }

   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
   bool saveToDatabase() const;
};

/**
 * Incident class
 */
class NXCORE_EXPORTABLE Incident : public enable_shared_from_this<Incident>
{
private:
   uint32_t m_id;
   time_t m_creationTime;
   time_t m_lastChangeTime;
   int m_state;
   uint32_t m_assignedUserId;
   TCHAR *m_title;
   uint32_t m_sourceAlarmId;
   uint32_t m_sourceObjectId;
   uint32_t m_createdByUser;
   uint32_t m_resolvedByUser;
   uint32_t m_closedByUser;
   time_t m_resolveTime;
   time_t m_closeTime;

   IntegerArray<uint32_t> m_linkedAlarms;
   Mutex m_mutex;

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }

   void logActivity(int activityType, uint32_t userId, const TCHAR *oldValue, const TCHAR *newValue, const TCHAR *details);

public:
   Incident();
   Incident(uint32_t id, uint32_t sourceObjectId, const TCHAR *title,
            uint32_t sourceAlarmId, uint32_t createdByUser);
   Incident(DB_RESULT hResult, int row);
   ~Incident();

   uint32_t getId() const { return m_id; }
   time_t getCreationTime() const { return m_creationTime; }
   time_t getLastChangeTime() const { return m_lastChangeTime; }
   int getState() const { return m_state; }
   uint32_t getAssignedUserId() const { return m_assignedUserId; }
   const TCHAR *getTitle() const { return m_title; }
   uint32_t getSourceAlarmId() const { return m_sourceAlarmId; }
   uint32_t getSourceObjectId() const { return m_sourceObjectId; }
   uint32_t getCreatedByUser() const { return m_createdByUser; }
   uint32_t getResolvedByUser() const { return m_resolvedByUser; }
   uint32_t getClosedByUser() const { return m_closedByUser; }
   time_t getResolveTime() const { return m_resolveTime; }
   time_t getCloseTime() const { return m_closeTime; }

   const IntegerArray<uint32_t>& getLinkedAlarms() const { return m_linkedAlarms; }
   int getLinkedAlarmCount() const { return m_linkedAlarms.size(); }

   bool loadLinkedAlarms(DB_HANDLE hdb);
   bool saveToDatabase() const;
   void updateInDatabase();

   void logCreationActivity(uint32_t userId) { logActivity(INCIDENT_ACTIVITY_CREATED, userId, nullptr, nullptr, nullptr); }
   void logAddCommentActivity(uint32_t userId) { logActivity(INCIDENT_ACTIVITY_COMMENT_ADDED, userId, nullptr, nullptr, nullptr); }

   uint32_t changeState(int newState, uint32_t userId, const wchar_t *comment = nullptr);
   uint32_t assign(uint32_t userId, uint32_t assignedBy);
   uint32_t update(const wchar_t *title, uint32_t userId);

   uint32_t linkAlarm(uint32_t alarmId, uint32_t userId);
   uint32_t unlinkAlarm(uint32_t alarmId, uint32_t userId);
   bool isAlarmLinked(uint32_t alarmId) const { return m_linkedAlarms.contains(alarmId); }

   void fillMessage(NXCPMessage *msg) const;
   void fillSummaryMessage(NXCPMessage *msg, uint32_t baseId) const;

   json_t *toJson() const;
};

/**
 * Get incident state name
 */
const wchar_t NXCORE_EXPORTABLE *GetIncidentStateName(int state);

/**
 * Incident subsystem functions
 */
bool InitIncidentManager();
void ShutdownIncidentManager();

uint32_t NXCORE_EXPORTABLE CreateIncident(uint32_t sourceObjectId, const TCHAR *title,
   const TCHAR *initialComment, uint32_t sourceAlarmId, uint32_t userId, uint32_t *incidentId);
uint32_t NXCORE_EXPORTABLE GetIncident(uint32_t incidentId, NXCPMessage *msg);
shared_ptr<Incident> NXCORE_EXPORTABLE FindIncidentById(uint32_t incidentId);
shared_ptr<Incident> NXCORE_EXPORTABLE FindIncidentByAlarmId(uint32_t alarmId);

uint32_t NXCORE_EXPORTABLE ChangeIncidentState(uint32_t incidentId, int newState, uint32_t userId, const TCHAR *comment = nullptr);
uint32_t NXCORE_EXPORTABLE AssignIncident(uint32_t incidentId, uint32_t userId, uint32_t assignedBy);
uint32_t NXCORE_EXPORTABLE ResolveIncident(uint32_t incidentId, uint32_t userId);
uint32_t NXCORE_EXPORTABLE CloseIncident(uint32_t incidentId, uint32_t userId);
uint32_t NXCORE_EXPORTABLE UpdateIncident(uint32_t incidentId, const TCHAR *title, uint32_t userId);

uint32_t NXCORE_EXPORTABLE LinkAlarmToIncident(uint32_t incidentId, uint32_t alarmId, uint32_t userId);
uint32_t NXCORE_EXPORTABLE UnlinkAlarmFromIncident(uint32_t incidentId, uint32_t alarmId, uint32_t userId);

uint32_t NXCORE_EXPORTABLE AddIncidentComment(uint32_t incidentId, const TCHAR *text, uint32_t userId, uint32_t *commentId);
uint32_t NXCORE_EXPORTABLE GetIncidentActivity(uint32_t incidentId, NXCPMessage *msg);

void SendIncidentsToClient(uint32_t objectId, uint32_t requestId, ClientSession *session);

/**
 * Scheduled task handler for delayed incident creation
 */
void CreateIncidentFromAlarmTask(const shared_ptr<ScheduledTaskParameters>& parameters);

#endif
