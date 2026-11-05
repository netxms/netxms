/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: nxcore_timeline.h
**
**/

#ifndef _nxcore_timeline_h_
#define _nxcore_timeline_h_

#include <nms_objects.h>
#include <vector>
#include <map>
#include <string>

/**
 * Single object timeline record
 */
struct ObjectTimelineRecord
{
   time_t timestamp;
   time_t endTime;         // End of interval record (0 if point record or interval still open)
   bool interval;          // true if record describes an interval (downtime, problem, deployment job)
   const char *category;   // Static string
   int severity;           // Event severity (SEVERITY_*) or -1 if not applicable
   String summary;
   json_t *details;        // Category-specific attributes (owned by record, can be nullptr)
   int sequence;           // Insertion order, used as sort tie-breaker

   ObjectTimelineRecord(time_t _timestamp, time_t _endTime, bool _interval, const char *_category, int _severity, const wchar_t *_summary, json_t *_details, int _sequence) :
      summary(_summary)
   {
      timestamp = _timestamp;
      endTime = _endTime;
      interval = _interval;
      category = _category;
      severity = _severity;
      details = _details;
      sequence = _sequence;
   }

   ~ObjectTimelineRecord()
   {
      json_decref(details);
   }
};

/**
 * Per-category state of timeline being built
 */
struct ObjectTimelineCategoryState
{
   int count;
   bool truncated;
   bool accessDenied;
   time_t oldestTimestamp;
   int firstSequence;      // Sequence number of first record in this category (-1 if none), defines category order for records with same timestamp

   ObjectTimelineCategoryState()
   {
      firstSequence = -1;
      count = 0;
      truncated = false;
      accessDenied = false;
      oldestTimestamp = 0;
   }
};

/**
 * Object timeline builder. Passed to timeline providers, which add records for the object within requested time range.
 * Providers are expected to select records in descending time order and add them until addPointRecord / addIntervalRecord
 * returns false - category limit is enforced by the builder, so when truncation happens the most recent records are kept.
 */
class NXCORE_EXPORTABLE ObjectTimelineBuilder
{
private:
   shared_ptr<NetObj> m_object;
   time_t m_timeFrom;
   time_t m_timeTo;
   uint32_t m_userId;
   uint64_t m_systemAccessRights;
   const StringSet *m_categories;
   int m_limitPerCategory;
   int m_minEventSeverity;
   ObjectArray<ObjectTimelineRecord> m_records;
   std::map<std::string, ObjectTimelineCategoryState> m_state;

   bool addRecord(time_t timestamp, time_t endTime, bool interval, const char *category, int severity, const wchar_t *summary, json_t *details);

public:
   ObjectTimelineBuilder(const shared_ptr<NetObj>& object, time_t timeFrom, time_t timeTo, uint32_t userId, const StringSet *categories,
            int limitPerCategory, int minEventSeverity);

   const NetObj& getObject() const { return *m_object; }
   const shared_ptr<NetObj>& getObjectPtr() const { return m_object; }
   uint32_t getObjectId() const { return m_object->getId(); }
   time_t getTimeFrom() const { return m_timeFrom; }
   time_t getTimeTo() const { return m_timeTo; }
   uint32_t getUserId() const { return m_userId; }
   uint64_t getSystemAccessRights() const { return m_systemAccessRights; }
   int getLimitPerCategory() const { return m_limitPerCategory; }
   int getMinEventSeverity() const { return m_minEventSeverity; }

   bool isCategoryRequested(const char *category) const;
   void setAccessDenied(const char *category);

   /**
    * Prepare query "SELECT <columns> <tail>" limited to category limit + 1 rows (so truncation can be detected).
    * Tail should select records in descending time order. Object ID, start and end of time range are bound
    * to positions 1, 2 and 3; caller can bind additional parameters starting from position 4.
    */
   DB_STATEMENT prepareRangeQuery(DB_HANDLE hdb, const wchar_t *columns, const wchar_t *tail) const;

   /**
    * Add point record. Takes ownership of details. Returns false if category limit is reached (record is not added).
    */
   bool addPointRecord(time_t timestamp, const char *category, int severity, const wchar_t *summary, json_t *details)
   {
      return addRecord(timestamp, 0, false, category, severity, summary, details);
   }

   /**
    * Add interval record (endTime is 0 for still open interval). Takes ownership of details.
    * Returns false if category limit is reached (record is not added).
    */
   bool addIntervalRecord(time_t startTime, time_t endTime, const char *category, int severity, const wchar_t *summary, json_t *details)
   {
      return addRecord(startTime, endTime, true, category, severity, summary, details);
   }

   json_t *toJson();
};

/**
 * Timeline provider. Called with pooled database connection.
 */
typedef void (*ObjectTimelineProvider)(ObjectTimelineBuilder *builder, DB_HANDLE hdb);

/**
 * Register timeline provider and categories it produces. Category names must be static strings.
 * Intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterObjectTimelineProvider(const char *name, const std::vector<const char*>& categories, ObjectTimelineProvider provider);

/**
 * Assign event code to timeline category (records for such events are taken from event log). Category name must be static string.
 * Intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterObjectTimelineEventCategory(uint32_t eventCode, const char *category);

/**
 * Get list of all known timeline categories
 */
std::vector<const char*> NXCORE_EXPORTABLE GetObjectTimelineCategories();

/**
 * Build timeline for given object. Categories set can be nullptr to include all categories.
 * Minimum event severity applies only to generic "event" category.
 */
json_t NXCORE_EXPORTABLE *BuildObjectTimeline(const shared_ptr<NetObj>& object, time_t timeFrom, time_t timeTo, uint32_t userId,
         const StringSet *categories, int limitPerCategory, int minEventSeverity);

#endif
