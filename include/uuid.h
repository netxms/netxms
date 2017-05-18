/*
 * Public include file for the UUID library
 * 
 * Copyright (C) 1996, 1997, 1998 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU 
 * Library General Public License.
 * %End-Header%
 */

#ifndef _UUID_H_
#define _UUID_H_

#define UUID_LENGTH     16

#if !HAVE_UUID_T
#undef uuid_t
typedef unsigned char uuid_t[UUID_LENGTH];
#endif

/* UUID Variant definitions */
#define UUID_VARIANT_NCS         0
#define UUID_VARIANT_DCE         1
#define UUID_VARIANT_MICROSOFT   2
#define UUID_VARIANT_OTHER       3

void LIBNETXMS_EXPORTABLE _uuid_clear(uuid_t uu);
int LIBNETXMS_EXPORTABLE _uuid_compare(const uuid_t uu1, const uuid_t uu2);
void LIBNETXMS_EXPORTABLE _uuid_generate(uuid_t out);
bool LIBNETXMS_EXPORTABLE _uuid_is_null(const uuid_t uu);
int LIBNETXMS_EXPORTABLE _uuid_parse(const TCHAR *in, uuid_t uu);
TCHAR LIBNETXMS_EXPORTABLE *_uuid_to_string(const uuid_t uu, TCHAR *out);
#ifdef UNICODE
char LIBNETXMS_EXPORTABLE *_uuid_to_stringA(const uuid_t uu, char *out);
#else
#define _uuid_to_stringA _uuid_to_string
#endif

#ifdef __cplusplus

/**
 * Wrapper class for UUID
 * Lower case used to avoid conflict with UUID struct defined in Windows headers
 */
class LIBNETXMS_EXPORTABLE uuid
{
private:
   uuid_t m_value;

public:
   uuid() { _uuid_clear(m_value); }
   uuid(const uuid_t v) { memcpy(m_value, v, UUID_LENGTH); }

   int compare(const uuid& u) const { return _uuid_compare(m_value, u.m_value); }
   bool equals(const uuid& u) const { return _uuid_compare(m_value, u.m_value) == 0; }
   const uuid_t& getValue() const { return m_value; }
   bool isNull() const { return _uuid_is_null(m_value); }
   TCHAR *toString(TCHAR *buffer) const { return _uuid_to_string(m_value, buffer); }
   String toString() const { TCHAR buffer[64]; return String(_uuid_to_string(m_value, buffer)); }
   char *toStringA(char *buffer) const { return _uuid_to_stringA(m_value, buffer); }
   json_t *toJson() const { char buffer[64]; return json_string(toStringA(buffer)); }

   /**
    * Generate new UUID
    */
   static uuid generate() 
   { 
      uuid_t u;
      _uuid_generate(u);
      return uuid(u);
   }

   /**
    * Parse string into UUID. Returns NULL UUID on error.
    */
   static uuid parse(const TCHAR *s)
   {
      uuid_t u;
      if (_uuid_parse(s, u) != 0)
         return NULL_UUID;
      return uuid(u);
   }

   static const uuid NULL_UUID;
};

#endif

#endif
