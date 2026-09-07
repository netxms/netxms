/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: ai_check_logic.cpp
**
** Pure decision logic for AI operator standing checks. Kept free of database and
** scheduler dependencies so that it can be unit tested in isolation.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <nxai.h>

/**
 * Parse severity value from check result hash. Returns -1 for invalid value.
 */
static int ParseCheckSeverity(NXSL_Value *value)
{
   if (value->isNumeric())
   {
      int severity = value->getValueAsInt32();
      return ((severity >= SEVERITY_NORMAL) && (severity <= SEVERITY_CRITICAL)) ? severity : -1;
   }

   if (value->getDataType() != NXSL_DT_STRING)
      return -1;

   const wchar_t *text = value->getValueAsCString();
   if (!wcsicmp(text, L"normal"))
      return SEVERITY_NORMAL;
   if (!wcsicmp(text, L"warning"))
      return SEVERITY_WARNING;
   if (!wcsicmp(text, L"minor"))
      return SEVERITY_MINOR;
   if (!wcsicmp(text, L"major"))
      return SEVERITY_MAJOR;
   if (!wcsicmp(text, L"critical"))
      return SEVERITY_CRITICAL;
   return -1;
}

/**
 * Set UTF-8 string field from NXSL value
 */
static inline void SetUtf8Field(std::string& field, NXSL_Value *value)
{
   char *text = UTF8StringFromWideString(value->getValueAsCString());
   field = text;
   MemFree(text);
}

/**
 * Evaluate value returned by a standing check script: null, false, 0, and empty string are quiet;
 * a string or a hash { title, severity, details } fires; anything else is a check error.
 */
AICheckResult NXCORE_EXPORTABLE EvaluateAICheckResult(NXSL_Value *value, const wchar_t *checkName)
{
   AICheckResult result;

   if ((value == nullptr) || value->isNull())
   {
      result.verdict = AICheckVerdict::QUIET;
      return result;
   }

   if (value->isHashMap())
   {
      NXSL_HashMap *map = value->getValueAsHashMap();
      result.verdict = AICheckVerdict::FIRED;

      NXSL_Value *title = map->get(L"title");
      if ((title != nullptr) && !title->isNull() && (*title->getValueAsCString() != 0))
      {
         SetUtf8Field(result.title, title);
      }
      else
      {
         char *text = UTF8StringFromWideString(checkName);
         result.title = text;
         MemFree(text);
      }

      NXSL_Value *severity = map->get(L"severity");
      if ((severity != nullptr) && !severity->isNull())
      {
         int s = ParseCheckSeverity(severity);
         if (s < 0)
         {
            result.verdict = AICheckVerdict::FAILED;
            result.error = "invalid severity in check result (expected normal, warning, minor, major, critical, or 0..4)";
            return result;
         }
         result.severity = s;
      }

      NXSL_Value *details = map->get(L"details");
      if ((details != nullptr) && !details->isNull())
         SetUtf8Field(result.details, details);

      return result;
   }

   // Type predicates are ordered: isNumeric() covers only numbers, while NXSL_DT_STRING is checked
   // by exact type because isString() is also true for booleans and numbers
   if (value->isNumeric())
   {
      if (value->isFalse())
      {
         result.verdict = AICheckVerdict::QUIET;
      }
      else
      {
         result.verdict = AICheckVerdict::FAILED;
         result.error = "check returned a non-zero number; return a string or a hash { title, severity, details } to fire";
      }
      return result;
   }

   if (value->getDataType() == NXSL_DT_BOOLEAN)
   {
      if (value->isFalse())
      {
         result.verdict = AICheckVerdict::QUIET;
      }
      else
      {
         result.verdict = AICheckVerdict::FAILED;
         result.error = "check returned true; return a string or a hash { title, severity, details } to fire";
      }
      return result;
   }

   if (value->getDataType() == NXSL_DT_STRING)
   {
      if (*value->getValueAsCString() == 0)
      {
         result.verdict = AICheckVerdict::QUIET;
      }
      else
      {
         result.verdict = AICheckVerdict::FIRED;
         SetUtf8Field(result.title, value);
      }
      return result;
   }

   result.verdict = AICheckVerdict::FAILED;
   result.error = "check returned a value of unsupported type (expected null, false, string, or hash)";
   return result;
}

/**
 * Decide state transition for a standing check given its previous verdict and the current run result.
 * Cooldown is the minimum time between two action applications; renotify interval re-applies the action
 * while the check stays fired (0 = fire only on the quiet->fired edge).
 */
AICheckTransition NXCORE_EXPORTABLE EvaluateAICheckTransition(AICheckVerdict previous, time_t lastFire, time_t now,
   uint32_t cooldown, uint32_t renotifyInterval, AICheckVerdict current)
{
   switch(current)
   {
      case AICheckVerdict::QUIET:
         return (previous == AICheckVerdict::FIRED) ? AICheckTransition::CLEAR : AICheckTransition::NONE;
      case AICheckVerdict::FIRED:
         if (previous == AICheckVerdict::FIRED)
         {
            if (renotifyInterval == 0)
               return AICheckTransition::NONE;
            time_t elapsed = now - lastFire;
            return ((elapsed >= static_cast<time_t>(renotifyInterval)) && (elapsed >= static_cast<time_t>(cooldown))) ?
               AICheckTransition::FIRE_RENOTIFY : AICheckTransition::NONE;
         }
         if ((lastFire != 0) && (now - lastFire < static_cast<time_t>(cooldown)))
            return AICheckTransition::SUPPRESSED;
         return AICheckTransition::FIRE_EDGE;
      case AICheckVerdict::FAILED:
         return AICheckTransition::FAILURE;
      default:
         return AICheckTransition::NONE;
   }
}
