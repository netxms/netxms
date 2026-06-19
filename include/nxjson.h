/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: nxjson.h
**
**/

#ifndef _nxjson_h_
#define _nxjson_h_

// This header expects that json_t is already defined

// JSON_EMBED was added in jansson 2.10 - ignore it for older version
#ifndef JSON_EMBED
#define JSON_EMBED 0
#endif

/**
 * Convert UNIX timestamp to JSON string in ISO 8601 format
 */
static inline json_t *json_time_string(time_t t)
{
   return (t == 0) ? json_null() : json_string(FormatISO8601Timestamp(t).c_str());
}

/**
 * Convert UNIX timestamp expressed in milliseconds to JSON string in ISO 8601 format
 */
static inline json_t *json_time_string_ms(int64_t t)
{
   return (t == 0) ? json_null() : json_string(FormatISO8601TimestampMs(t).c_str());
}

/**
 * Create JSON string with null check
 */
static inline json_t *json_string_a(const char *s)
{
   return (s != nullptr) ? json_string(s) : json_null();
}

/**
 * Create JSON string from wide character string
 */
static inline json_t *json_string_w(const WCHAR *s)
{
   if (s == nullptr)
      return json_null();
   char *us = UTF8StringFromWideString(s);
   json_t *js = json_string(us);
   MemFree(us);
   return js;
}

#ifdef UNICODE
#define json_string_t json_string_w
#else
#define json_string_t json_string_a
#endif

/**
 * Create JSON string with base64-encoded content
 */
static inline json_t *json_base64_string(const void *data, size_t len)
{
   char *out;
   base64_encode_alloc(reinterpret_cast<const char*>(data), len, &out);
   json_t *js = json_string(out);
   MemFree(out);
   return js;
}

/**
 * Create JSON array from integer array
 */
template<typename T> static inline json_t *json_integer_array(const T *values, size_t size)
{
   json_t *a = json_array();
   for(size_t i = 0; i < size; i++)
      json_array_append_new(a, json_integer(values[i]));
   return a;
}

/**
 * Create JSON array from integer array
 */
template<typename T> static inline json_t *json_integer_array(const IntegerArray<T>& values)
{
   json_t *a = json_array();
   for(int i = 0; i < values.size(); i++)
      json_array_append_new(a, json_integer(values.get(i)));
   return a;
}

/**
 * Create JSON array from integer array
 */
template<typename T> static inline json_t *json_integer_array(const IntegerArray<T> *values)
{
   json_t *a = json_array();
   if (values != nullptr)
   {
      for(int i = 0; i < values->size(); i++)
         json_array_append_new(a, json_integer(values->get(i)));
   }
   return a;
}

/**
 * Serialize ObjectArray as JSON
 */
template<typename T> static inline json_t *json_object_array(const ObjectArray<T> *a)
{
   json_t *root = json_array();
   if (a != nullptr)
   {
      for(int i = 0; i < a->size(); i++)
      {
         T *e = a->get(i);
         if (e != nullptr)
            json_array_append_new(root, e->toJson());
      }
   }
   return root;
}

/**
 * Serialize ObjectArray as JSON
 */
template<typename T> static inline json_t *json_object_array(const ObjectArray<T>& a)
{
   return json_object_array<T>(&a);
}

/**
 * Serialize SharedObjectArray as JSON
 */
template<typename T> static inline json_t *json_object_array(const SharedObjectArray<T> *a)
{
   json_t *root = json_array();
   if (a != nullptr)
   {
      for(int i = 0; i < a->size(); i++)
      {
         T *e = a->get(i);
         if (e != nullptr)
            json_array_append_new(root, e->toJson());
      }
   }
   return root;
}

/**
 * Serialize SharedObjectArray as JSON
 */
template<typename T> static inline json_t *json_object_array(const SharedObjectArray<T>& a)
{
   return json_object_array<T>(&a);
}

/**
 * Serialize StructArray as JSON
 */
template<typename T> static inline json_t *json_struct_array(const StructArray<T> *a)
{
   json_t *root = json_array();
   if (a != nullptr)
   {
      for(int i = 0; i < a->size(); i++)
      {
         T *e = a->get(i);
         if (e != nullptr)
            json_array_append_new(root, e->toJson());
      }
   }
   return root;
}

/**
 * Serialize StructArray as JSON
 */
template<typename T> static inline json_t *json_struct_array(const StructArray<T>& a)
{
   return json_struct_array<T>(&a);
}

/**
 * Get string value from object
 */
static inline WCHAR *json_object_get_string_w(json_t *object, const char *tag, const WCHAR *defval)
{
   json_t *value = json_object_get(object, tag);
   return json_is_string(value) ? WideStringFromUTF8String(json_string_value(value)) : MemCopyStringW(defval);
}

/**
 * Get string value from object
 */
static inline char *json_object_get_string_a(json_t *object, const char *tag, const char *defval)
{
   json_t *value = json_object_get(object, tag);
   return json_is_string(value) ? MBStringFromUTF8String(json_string_value(value)) : MemCopyStringA(defval);
}

#ifdef UNICODE
#define json_object_get_string_t json_object_get_string_w
#else
#define json_object_get_string_t json_object_get_string_a
#endif

/**
 * Get string value from object
 */
static inline String json_object_get_string(json_t *object, const char *tag, const TCHAR *defval)
{
   json_t *value = json_object_get(object, tag);
   return json_is_string(value) ? String(json_string_value(value), "utf8") : String(defval);
}

/**
 * Get string value from object
 */
static inline const char *json_object_get_string_utf8(json_t *object, const char *tag, const char *defval)
{
   json_t *value = json_object_get(object, tag);
   return json_is_string(value) ? json_string_value(value) : defval;
}

/**
 * Get integer value with type conversion when necessary
 */
static inline json_int_t json_integer_value_ex(json_t *v, json_int_t defval)
{
   if (json_is_integer(v))
      return json_integer_value(v);
   if (json_is_string(v))
      return strtoll(json_string_value(v), nullptr, 0);
   if (json_is_boolean(v))
      return json_boolean_value(v) ? 1 : 0;
   return defval;
}

/**
 * Get integer value from object
 */
static inline int64_t json_object_get_int64(json_t *object, const char *tag, int64_t defval = 0)
{
   return json_integer_value_ex(json_object_get(object, tag), defval);
}

/**
 * Get integer value from object
 */
static inline uint64_t json_object_get_uint64(json_t *object, const char *tag, uint64_t defval = 0)
{
   return static_cast<uint64_t>(json_integer_value_ex(json_object_get(object, tag), defval));
}

/**
 * Get integer value from object
 */
static inline int32_t json_object_get_int32(json_t *object, const char *tag, int32_t defval = 0)
{
   return static_cast<int32_t>(json_integer_value_ex(json_object_get(object, tag), defval));
}

/**
 * Get unsigned integer value from object
 */
static inline uint32_t json_object_get_uint32(json_t *object, const char *tag, uint32_t defval = 0)
{
   return static_cast<uint32_t>(json_integer_value_ex(json_object_get(object, tag), defval));
}

/**
 * Get boolean value from object
 */
static inline bool json_object_get_boolean(json_t *object, const char *tag, bool defval = false)
{
   json_t *value = json_object_get(object, tag);
   if (json_is_string(value))
   {
      const char *val = json_string_value(value);
      if (!stricmp(val, "true"))
         return true;
      if (!stricmp(val, "false"))
         return false;
   }
   return json_is_boolean(value) ? json_boolean_value(value) : (json_is_integer(value) ? (json_integer_value(value) != 0) : defval);
}

/**
 * Get UUID value from object
 */
uuid LIBNETXMS_EXPORTABLE json_object_get_uuid(json_t *object, const char *tag);

/**
 * Get time value from object
 */
time_t LIBNETXMS_EXPORTABLE json_object_get_time(json_t *object, const char *tag, time_t defval = 0);

/**
 * Coerce a JSON value into an array. Useful when consuming arguments from
 * LLM-generated tool calls where some models serialize array arguments as
 * stringified JSON or as bare scalars instead of native arrays. Accepts:
 *   - native JSON array (returned with incremented reference count)
 *   - JSON string containing an array literal "[...]" (parsed)
 *   - any other scalar value (string, integer, boolean, object) - wrapped in a
 *     single-element array
 * Returns nullptr if the tag is absent from `object` or the value is present
 * but cannot be coerced (e.g. malformed JSON string). Always returns a NEW
 * reference - caller should transfer ownership via json_array_append_new /
 * json_object_set_new, or release with json_decref.
 */
json_t LIBNETXMS_EXPORTABLE *json_object_get_array_ex(json_t *object, const char *tag);

/**
 * Coerce a JSON value into an object/map. Companion to json_object_get_array_ex
 * for map-typed arguments. Accepts a native object (incremented refcount) or
 * a JSON string containing an object literal "{...}" (parsed). Returns nullptr
 * if the tag is absent or cannot be coerced. Caller owns the returned ref.
 */
json_t LIBNETXMS_EXPORTABLE *json_object_get_object_ex(json_t *object, const char *tag);

/**
 * Get element from object by path (separated by /)
 */
json_t LIBNETXMS_EXPORTABLE *json_object_get_by_path_a(json_t *root, const char *path);

/**
 * Get element from object by path (separated by /)
 */
static inline json_t *json_object_get_by_path_w(json_t *root, const WCHAR *path)
{
   char utf8path[1024];
   wchar_to_utf8(path, -1, utf8path, 1024);
   utf8path[1023] = 0;
   return json_object_get_by_path_a(root, utf8path);
}

#ifdef UNICODE
#define json_object_get_by_path json_object_get_by_path_w
#else
#define json_object_get_by_path json_object_get_by_path_a
#endif

#if !HAVE_DECL_JSON_OBJECT_UPDATE_NEW

/**
 * Update json object with new object
 */
static inline int json_object_update_new(json_t *object, json_t *other)
{
    int ret = json_object_update(object, other);
    json_decref(other);
    return ret;
}

#endif /* HAVE_DECL_JSON_OBJECT_UPDATE_NEW */

/**
 * Mapping between bitmask flag values and JSON field names
 */
struct FlagNameMapping
{
   uint64_t bit;        // Bit flag value
   const char *name;    // JSON field name (nullptr = end sentinel)
};

/**
 * Expand bitmask flags into JSON object with boolean attributes
 */
static inline json_t *json_boolean_object(uint64_t flags, const FlagNameMapping *mapping)
{
   json_t *object = json_object();
   for (int i = 0; mapping[i].name != nullptr; i++)
      json_object_set_new(object, mapping[i].name, json_boolean(flags & mapping[i].bit));
   return object;
}

/**
 * Compact JSON object with boolean attributes into bitmask flags
 */
static inline uint64_t json_object_get_flags(json_t *object, const FlagNameMapping *mapping)
{
   uint64_t flags = 0;
   for (int i = 0; mapping[i].name != nullptr; i++)
   {
      if (json_is_true(json_object_get(object, mapping[i].name)))
         flags |= mapping[i].bit;
   }
   return flags;
}

/**
 * Update bitmask flags from JSON object (PATCH semantics - only modifies bits present in JSON)
 */
static inline uint64_t json_object_update_flags(json_t *object, uint64_t currentFlags, const FlagNameMapping *mapping)
{
   uint64_t flags = currentFlags;
   for (int i = 0; mapping[i].name != nullptr; i++)
   {
      json_t *value = json_object_get(object, mapping[i].name);
      if (value != nullptr)
      {
         if (json_is_true(value))
            flags |= mapping[i].bit;
         else
            flags &= ~mapping[i].bit;
      }
   }
   return flags;
}

/**
 * Update integer field from a JSON object property using strict validation, intended for
 * modifyFromJSON() style handlers. When the property is present it must be an integer or
 * JSON null (null is interpreted as 0); the field is left unchanged when the property is
 * absent. Returns false only when the property is present with an incompatible type, which
 * the caller typically maps to RCC_INVALID_ARGUMENT.
 */
template<typename T> static inline bool json_object_update_integer(json_t *object, const char *tag, T *field)
{
   json_t *value = json_object_get(object, tag);
   if (value == nullptr)
      return true;
   if (!json_is_integer(value) && !json_is_null(value))
      return false;
   *field = static_cast<T>(json_integer_value(value));
   return true;
}

/**
 * Update boolean field from a JSON object property using strict validation. When the property
 * is present it must be a JSON boolean; the field is left unchanged when the property is absent.
 * Returns false only when the property is present with an incompatible type.
 */
static inline bool json_object_update_boolean(json_t *object, const char *tag, bool *field)
{
   json_t *value = json_object_get(object, tag);
   if (value == nullptr)
      return true;
   if (!json_is_boolean(value))
      return false;
   *field = json_is_true(value);
   return true;
}

/**
 * Update object flag bit(s) from a boolean JSON object property using strict validation. When
 * the property is present it must be a JSON boolean: the given bit(s) are added to *mask and
 * set in *setFlags when the value is true. Both outputs are left unchanged when the property is
 * absent, so this can be called repeatedly to collect a set of changes for a single
 * updateFlags(setFlags, mask) call. Returns false only when the property is present with an
 * incompatible type.
 */
static inline bool json_object_update_flag(json_t *object, const char *tag, uint32_t bit, uint32_t *setFlags, uint32_t *mask)
{
   json_t *value = json_object_get(object, tag);
   if (value == nullptr)
      return true;
   if (!json_is_boolean(value))
      return false;
   *mask |= bit;
   if (json_is_true(value))
      *setFlags |= bit;
   return true;
}

/**
 * Update a fixed-size wide character buffer from a JSON object property using strict validation.
 * When the property is present it must be a JSON string or null (null and empty string both clear
 * the buffer); the buffer is left unchanged when the property is absent. The value is converted
 * from UTF-8 and the buffer is always null-terminated. `size` is the buffer capacity in characters.
 * Returns false only when the property is present with an incompatible type.
 */
static inline bool json_object_update_string(json_t *object, const char *tag, wchar_t *buffer, size_t size)
{
   json_t *value = json_object_get(object, tag);
   if (value == nullptr)
      return true;
   if (!json_is_string(value) && !json_is_null(value))
      return false;
   utf8_to_wchar(json_is_string(value) ? json_string_value(value) : "", -1, buffer, size);
   buffer[size - 1] = 0;
   return true;
}

/**
 * Update a fixed-size UTF-8 character buffer from a JSON object property using strict validation.
 * Behaves like json_object_update_string but stores the raw UTF-8 string via strlcpy; `size` is the
 * buffer capacity in bytes. Returns false only when the property is present with an incompatible type.
 */
static inline bool json_object_update_string_utf8(json_t *object, const char *tag, char *buffer, size_t size)
{
   json_t *value = json_object_get(object, tag);
   if (value == nullptr)
      return true;
   if (!json_is_string(value) && !json_is_null(value))
      return false;
   strlcpy(buffer, json_is_string(value) ? json_string_value(value) : "", size);
   return true;
}

/**
 * Update an enumeration field from a JSON object property using strict validation. The property may
 * be given either as a symbolic name (string, matched case-insensitively against the supplied
 * code/name lookup table) or as a raw numeric code (integer); the field is left unchanged when the
 * property is absent. Returns false only when the property is present with an incompatible type or
 * with an unknown symbolic name, which the caller typically maps to RCC_INVALID_ARGUMENT.
 */
template<typename T> static inline bool json_object_update_enum(json_t *object, const char *tag, CodeLookupElement *lookupTable, T *field)
{
   json_t *value = json_object_get(object, tag);
   if (value == nullptr)
      return true;
   if (json_is_integer(value))
   {
      *field = static_cast<T>(json_integer_value(value));
      return true;
   }
   if (json_is_string(value))
   {
      // Convert via String so the lookup works with TCHAR in both UNICODE and non-UNICODE builds
      int code = CodeFromText(String(json_string_value(value), "utf8"), lookupTable, INT_MIN);
      if (code == INT_MIN)
         return false;
      *field = static_cast<T>(code);
      return true;
   }
   return false;
}

#endif
