/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: table.cpp
**
**/

#include "libnxsl.h"

/**
 * Default time format
 */
static TCHAR s_defaultFormatLocal[] = _T("%a %b %d %Y %H:%M:%S %Z");
static TCHAR s_defaultFormatUTC[] = _T("%a %b %d %Y %H:%M:%S UTC");

/**
 * NXSL "DateTime" class object
 */
NXSL_DateTimeClass LIBNXSL_EXPORTABLE g_nxslDateTimeClass;

/**
 * DateTime internal data
 */
struct DateTime
{
   time_t timestamp;
   bool utc;   // broken-down time is in UTC
   bool valid; // timestamp is valid
   struct tm data;

   /**
    * Update broken-down time structure from timestamp
    */
   void updateFromTimestamp()
   {
      if (utc)
      {
#if HAVE_GMTIME_R
         if (gmtime_r(&timestamp, &data) == nullptr)
            memset(&data, 0, sizeof(struct tm));
#else
         struct tm *p = gmtime(&timestamp);
         if (p != nullptr)
            memcpy(&data, p, sizeof(struct tm));
         else
            memset(&data, 0, sizeof(struct tm));
#endif
      }
      else
      {
#if HAVE_LOCALTIME_R
         if (localtime_r(&timestamp, &data) == nullptr)
            memset(&data, 0, sizeof(struct tm));
#else
         struct tm *p = localtime(&timestamp);
         if (p != nullptr)
            memcpy(&data, p, sizeof(struct tm));
         else
            memset(&data, 0, sizeof(struct tm));
#endif
      }
   }
};

/**
 * DateTime::format method
 */
NXSL_METHOD_DEFINITION(DateTime, format)
{
   auto dt = static_cast<DateTime*>(object->getData());

   StringBuffer f;
   if (argv[0]->isString())
   {
      f = argv[0]->getValueAsCString();
      if (f.isEmpty())
      {
         f = dt->utc ? s_defaultFormatUTC : s_defaultFormatLocal;
      }
      else if (dt->utc)
      {
         // AIX and maybe some other systems does not have correct timezone information within struct tm,
         // so for UTC we replace %Z and %z before calling strftime
         f.replace(_T("%Z"), _T("UTC"));
         f.replace(_T("%z"), _T("+0000"));
      }
   }
   else if (argv[0]->isNull())
   {
      f = dt->utc ? s_defaultFormatUTC : s_defaultFormatLocal;
   }
   else
   {
      return NXSL_ERR_NOT_STRING;
   }

   TCHAR buffer[512];
   _tcsftime(buffer, 512, f, &dt->data);
   *result = vm->createValue(buffer);
   return NXSL_ERR_SUCCESS;
}

/**
 * "DateTime" class constructor
 */
NXSL_DateTimeClass::NXSL_DateTimeClass() : NXSL_Class()
{
   setName(_T("DateTime"));

   NXSL_REGISTER_METHOD(DateTime, format, 1);
}

/**
 * Get "DateTime" class attribute
 */
NXSL_Value *NXSL_DateTimeClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   struct tm *st = (attr.value[0] != '?') ? &(static_cast<DateTime*>(object->getData())->data) : nullptr;
   if (compareAttributeName(attr, "second") || compareAttributeName(attr, "sec") || compareAttributeName(attr, "tm_sec"))
   {
      value = vm->createValue((LONG)st->tm_sec);
   }
   else if (compareAttributeName(attr, "minute") || compareAttributeName(attr, "min") || compareAttributeName(attr, "tm_min"))
   {
      value = vm->createValue((LONG)st->tm_min);
   }
   else if (compareAttributeName(attr, "hour") || compareAttributeName(attr, "tm_hour"))
   {
      value = vm->createValue((LONG)st->tm_hour);
   }
   else if (compareAttributeName(attr, "day") || compareAttributeName(attr, "mday") || compareAttributeName(attr, "tm_mday"))
   {
      value = vm->createValue((LONG)st->tm_mday);
   }
   else if (compareAttributeName(attr, "month") || compareAttributeName(attr, "mon") || compareAttributeName(attr, "tm_mon"))
   {
      value = vm->createValue((LONG)st->tm_mon);
   }
   else if (compareAttributeName(attr, "year") || compareAttributeName(attr, "tm_year"))
   {
      value = vm->createValue((LONG)(st->tm_year + 1900));
   }
   else if (compareAttributeName(attr, "dayOfYear") || compareAttributeName(attr, "yday") || compareAttributeName(attr, "tm_yday"))
   {
      value = vm->createValue((LONG)st->tm_yday);
   }
   else if (compareAttributeName(attr, "dayOfWeek") || compareAttributeName(attr, "wday") || compareAttributeName(attr, "tm_wday"))
   {
      value = vm->createValue((LONG)st->tm_wday);
   }
   else if (compareAttributeName(attr, "isDST") || compareAttributeName(attr, "isdst") || compareAttributeName(attr, "tm_isdst"))
   {
      value = vm->createValue((LONG)st->tm_isdst);
   }
   else if (compareAttributeName(attr, "isUTC"))
   {
      value = vm->createValue(static_cast<DateTime*>(object->getData())->utc);
   }
   else if (compareAttributeName(attr, "timestamp"))
   {
      auto dt = static_cast<DateTime*>(object->getData());
      if (!dt->valid)
      {
         dt->timestamp = dt->utc ? timegm(st) : mktime(st);
         dt->valid = true;
      }
      value = vm->createValue(static_cast<int64_t>(dt->timestamp));
   }
   return value;
}

/**
 * Set "DateTime" class attribute
 */
bool NXSL_DateTimeClass::setAttr(NXSL_Object *object, const NXSL_Identifier& attr, NXSL_Value *value)
{
   bool success = true;
   auto dt = static_cast<DateTime*>(object->getData());
   auto st = &dt->data;
   if (compareAttributeName(attr, "second") || compareAttributeName(attr, "sec") || compareAttributeName(attr, "tm_sec"))
   {
      st->tm_sec = value->getValueAsInt32();
      dt->valid = false;
   }
   else if (compareAttributeName(attr, "minute") || compareAttributeName(attr, "min") || compareAttributeName(attr, "tm_min"))
   {
      st->tm_min = value->getValueAsInt32();
      dt->valid = false;
   }
   else if (compareAttributeName(attr, "hour") || compareAttributeName(attr, "tm_hour"))
   {
      st->tm_hour = value->getValueAsInt32();
      dt->valid = false;
   }
   else if (compareAttributeName(attr, "day") || compareAttributeName(attr, "mday") || compareAttributeName(attr, "tm_mday"))
   {
      st->tm_mday = value->getValueAsInt32();
      dt->valid = false;
   }
   else if (compareAttributeName(attr, "month") || compareAttributeName(attr, "mon") || compareAttributeName(attr, "tm_mon"))
   {
      st->tm_mon = value->getValueAsInt32();
      dt->valid = false;
   }
   else if (compareAttributeName(attr, "year") || compareAttributeName(attr, "tm_year"))
   {
      st->tm_year = value->getValueAsInt32() - 1900;
      dt->valid = false;
   }
   else if (compareAttributeName(attr, "dayOfYear") || compareAttributeName(attr, "yday") || compareAttributeName(attr, "tm_yday"))
   {
      st->tm_yday = value->getValueAsInt32();
      dt->valid = false;
   }
   else if (compareAttributeName(attr, "dayOfWeek") || compareAttributeName(attr, "wday") || compareAttributeName(attr, "tm_wday"))
   {
      st->tm_wday = value->getValueAsInt32();
      dt->valid = false;
   }
   else if (compareAttributeName(attr, "isDST") || compareAttributeName(attr, "isdst") || compareAttributeName(attr, "tm_isdst"))
   {
      st->tm_isdst = value->getValueAsInt32();
      dt->valid = false;
   }
   else if (compareAttributeName(attr, "isUTC"))
   {
      if (value->isTrue() != dt->utc)
      {
         if (!dt->valid)
         {
            dt->timestamp = dt->utc ? timegm(st) : mktime(st);
            dt->valid = true;
         }
         dt->utc = value->isTrue();
         dt->updateFromTimestamp();
      }
   }
   else if (compareAttributeName(attr, "timestamp"))
   {
      dt->timestamp = static_cast<time_t>(value->getValueAsInt64());
      dt->valid = true;
      dt->updateFromTimestamp();
   }
   else
   {
      success = false;  // Error
   }
   return success;
}

/**
 * "DateTime" class destructor
 */
void NXSL_DateTimeClass::onObjectDelete(NXSL_Object *object)
{
   MemFree(object->getData());
}

/**
 * Convert DateTime object to string
 */
void NXSL_DateTimeClass::toString(StringBuffer *sb, NXSL_Object *object)
{
   auto dt = static_cast<DateTime*>(object->getData());
   TCHAR buffer[512];
   _tcsftime(buffer, 512, dt->utc ? s_defaultFormatUTC : s_defaultFormatLocal, &dt->data);
   sb->append(buffer);
}

/**
 * Create DateTime object
 */
int F_DateTime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   auto dt = MemAllocStruct<DateTime>();
   dt->valid = true;
   dt->utc = (argc > 1) ? argv[1]->isTrue() : false;
   dt->timestamp = (argc > 0) ? static_cast<time_t>(argv[0]->getValueAsInt64()) : time(nullptr);

   if (dt->utc)
   {
#if HAVE_GMTIME_R
      if (gmtime_r(&dt->timestamp, &dt->data) == nullptr)
         memset(&dt->data, 0, sizeof(struct tm));
#else
      struct tm *p = gmtime(&dt->timestamp);
      if (p != nullptr)
         memcpy(&dt->data, p, sizeof(struct tm));
      else
         memset(&dt->data, 0, sizeof(struct tm));
#endif
   }
   else
   {
#if HAVE_LOCALTIME_R
      if (localtime_r(&dt->timestamp, &dt->data) == nullptr)
         memset(&dt->data, 0, sizeof(struct tm));
#else
      struct tm *p = localtime(&dt->timestamp);
      if (p != nullptr)
         memcpy(&dt->data, p, sizeof(struct tm));
      else
         memset(&dt->data, 0, sizeof(struct tm));
#endif
   }

   *result = vm->createValue(vm->createObject(&g_nxslDateTimeClass, dt));
   return NXSL_ERR_SUCCESS;
}

/**
 * Get current time
 */
int F_GetCurrentTime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   *result = vm->createValue(static_cast<int64_t>(time(nullptr)));
   return NXSL_ERR_SUCCESS;
}

/**
 * Get current time in milliseconds
 */
int F_GetCurrentTimeMs(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   *result = vm->createValue(GetCurrentTimeMs());
   return NXSL_ERR_SUCCESS;
}

/**
 * Get current time from monotonic clock in milliseconds
 */
int F_GetMonotonicClockTime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   *result = vm->createValue(GetMonotonicClockTime());
   return NXSL_ERR_SUCCESS;
}

/**
 * Return parsed local time
 */
int F_localtime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   time_t t;

   if (argc == 0)
   {
      t = time(nullptr);
   }
   else if (argc == 1)
   {
      if (!argv[0]->isInteger())
         return NXSL_ERR_NOT_INTEGER;

      t = argv[0]->getValueAsUInt32();
   }
   else
   {
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   }

   auto dt = MemAllocStruct<DateTime>();
   dt->timestamp = t;
   dt->valid = true;
#if HAVE_LOCALTIME_R
   if (localtime_r(&t, &dt->data) == nullptr)
      memset(&dt->data, 0, sizeof(struct tm));
#else
   struct tm *p = localtime(&t);
   if (p != nullptr)
      memcpy(&dt->data, p, sizeof(struct tm));
   else
      memset(&dt->data, 0, sizeof(struct tm));
#endif
   *result = vm->createValue(vm->createObject(&g_nxslDateTimeClass, dt));
   return NXSL_ERR_SUCCESS;
}

/**
 * Return parsed UTC time
 */
int F_gmtime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   time_t t;

   if (argc == 0)
   {
      t = time(nullptr);
   }
   else if (argc == 1)
   {
      if (!argv[0]->isInteger())
         return NXSL_ERR_NOT_INTEGER;

      t = argv[0]->getValueAsUInt32();
   }
   else
   {
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   }

   auto dt = MemAllocStruct<DateTime>();
   dt->timestamp = t;
   dt->valid = true;
   dt->utc = true;
#if HAVE_GMTIME_R
   if (gmtime_r(&t, &dt->data) == nullptr)
      memset(&dt->data, 0, sizeof(struct tm));
#else
   struct tm *p = gmtime(&t);
   if (p != nullptr)
      memcpy(&dt->data, p, sizeof(struct tm));
   else
      memset(&dt->data, 0, sizeof(struct tm));
#endif
   *result = vm->createValue(vm->createObject(&g_nxslDateTimeClass, dt));
   return NXSL_ERR_SUCCESS;
}

/**
 * Create time from DAteTime class
 */
int F_mktime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   if (_tcscmp(argv[0]->getValueAsObject()->getClass()->getName(), g_nxslDateTimeClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   auto st = &(static_cast<DateTime*>(argv[0]->getValueAsObject()->getData())->data);
   *result = vm->createValue(static_cast<int64_t>(mktime(st)));
   return NXSL_ERR_SUCCESS;
}

/**
 * Convert time_t into string
 * PATCH: by Edgar Chupit
 */
int F_strftime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   time_t tTime;
   bool useLocalTime;

   if ((argc == 0) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   if (argc > 1)
   {
      if (!argv[1]->isInteger() && !argv[1]->isNull())
         return NXSL_ERR_NOT_INTEGER;
      tTime = argv[1]->isNull() ? time(nullptr) : (time_t)argv[1]->getValueAsUInt64();

      useLocalTime = (argc > 2) ? argv[2]->isTrue() : true;
   }
   else
   {
      // No second argument
      tTime = time(nullptr);
      useLocalTime = true;
   }

#if HAVE_LOCALTIME_R && HAVE_GMTIME_R
   struct tm tbuffer;
   struct tm *ptm = useLocalTime ? localtime_r(&tTime, &tbuffer) : gmtime_r(&tTime, &tbuffer);
#else
   struct tm *ptm = useLocalTime ? localtime(&tTime) : gmtime(&tTime);
#endif
   if (ptm != nullptr)
   {
      TCHAR buffer[512];
      _tcsftime(buffer, 512, argv[0]->getValueAsCString(), ptm);
      *result = vm->createValue(buffer);
   }
   else
   {
      *result = vm->createValue();
   }
   return NXSL_ERR_SUCCESS;
}
