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
** File: ef_snmptrap.cpp
**
** Built-in event forwarder driver that sends events as SNMP traps with structured
** varbinds (event code, name, severity, source, tags, parameters). Trap and varbind
** OIDs are defined in NETXMS-MIB (nxEventNotification) and can be overridden in
** the forwarder configuration.
**
**/

#include "nxcore.h"
#include <efdrv.h>
#include <nxsnmp.h>

#define DEBUG_TAG L"event.forwarder"

/**
 * SNMP request ID
 */
static VolatileCounter s_requestId = 0;

/**
 * Default field OID (NETXMS-MIB nxEvent group): .1.3.6.1.4.1.57163.1.1.<field>.0
 */
static inline SNMP_ObjectId DefaultFieldId(uint32_t field)
{
   uint32_t oid[] = { 1, 3, 6, 1, 4, 1, 57163, 1, 1, field, 0 };
   return SNMP_ObjectId(oid, 11);
}

/**
 * SNMP trap event forwarder driver
 */
class SnmpTrapEventForwarderDriver : public EventForwarderDriver
{
private:
   wchar_t m_hostname[MAX_DNS_NAME];
   uint16_t m_port;
   SNMP_Version m_version;
   SNMP_AuthMethod m_authMethod;
   SNMP_EncryptionMethod m_privMethod;
   char m_authName[128];
   char m_authPassword[128];
   char m_privPassword[128];
   bool m_useInformRequest;
   time_t m_startTime;
   SNMP_ObjectId m_trapId;
   SNMP_ObjectId m_sourceNameFieldId;
   SNMP_ObjectId m_severityFieldId;
   SNMP_ObjectId m_messageFieldId;
   SNMP_ObjectId m_timestampFieldId;
   SNMP_ObjectId m_eventCodeFieldId;
   SNMP_ObjectId m_eventNameFieldId;
   SNMP_ObjectId m_eventIdFieldId;
   SNMP_ObjectId m_sourceObjectIdFieldId;
   SNMP_ObjectId m_tagsFieldId;
   SNMP_ObjectId m_parametersFieldId;

   SnmpTrapEventForwarderDriver();

public:
   virtual bool forward(const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source) override;

   static SnmpTrapEventForwarderDriver *createInstance(json_t *config);
};

/**
 * Driver constructor - set defaults (NETXMS-MIB nxEventNotification)
 */
SnmpTrapEventForwarderDriver::SnmpTrapEventForwarderDriver() :
   m_trapId({ 1, 3, 6, 1, 4, 1, 57163, 1, 0, 2 }),
   m_sourceNameFieldId(DefaultFieldId(1)),
   m_severityFieldId(DefaultFieldId(2)),
   m_messageFieldId(DefaultFieldId(3)),
   m_timestampFieldId(DefaultFieldId(4)),
   m_eventCodeFieldId(DefaultFieldId(7)),
   m_eventNameFieldId(DefaultFieldId(8)),
   m_eventIdFieldId(DefaultFieldId(9)),
   m_sourceObjectIdFieldId(DefaultFieldId(10)),
   m_tagsFieldId(DefaultFieldId(11)),
   m_parametersFieldId(DefaultFieldId(12))
{
   m_hostname[0] = 0;
   m_port = 162;
   m_version = SNMP_VERSION_2C;
   m_authMethod = SNMP_AUTH_NONE;
   m_privMethod = SNMP_ENCRYPT_NONE;
   strcpy(m_authName, "public");
   m_authPassword[0] = 0;
   m_privPassword[0] = 0;
   m_useInformRequest = false;
   m_startTime = time(nullptr);
}

/**
 * Create varbind with given value
 */
static inline SNMP_Variable *CreateVarbind(const SNMP_ObjectId& name, uint32_t type, const wchar_t *value)
{
   auto v = new SNMP_Variable(name);
   v->setValueFromString(type, value);
   return v;
}

/**
 * Forward event as SNMP trap. Recipient (if not empty) is the target host,
 * otherwise "hostname" from the driver configuration is used.
 */
bool SnmpTrapEventForwarderDriver::forward(const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source)
{
   const wchar_t *target = ((recipient != nullptr) && (recipient[0] != 0)) ? recipient : m_hostname;
   if (target[0] == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SNMP trap forwarder: no target address (recipient is empty and \"hostname\" is not set in configuration)");
      return false;
   }

   SNMP_PDU pdu(m_useInformRequest ? SNMP_INFORM_REQUEST : SNMP_TRAP, m_version, m_trapId,
      static_cast<uint32_t>(time(nullptr) - m_startTime), InterlockedIncrement(&s_requestId));

   wchar_t buffer[64];
   if (source != nullptr)
   {
      pdu.bindVariable(CreateVarbind(m_sourceNameFieldId, ASN_OCTET_STRING, source->getName()));
      pdu.bindVariable(CreateVarbind(m_sourceObjectIdFieldId, ASN_INTEGER, IntegerToString(source->getId(), buffer)));
   }
   pdu.bindVariable(CreateVarbind(m_severityFieldId, ASN_INTEGER, IntegerToString(event.getSeverity(), buffer)));
   pdu.bindVariable(CreateVarbind(m_messageFieldId, ASN_OCTET_STRING, event.getMessage()));
   pdu.bindVariable(CreateVarbind(m_timestampFieldId, ASN_TIMETICKS, IntegerToString(static_cast<uint64_t>(event.getTimestamp()), buffer)));
   pdu.bindVariable(CreateVarbind(m_eventCodeFieldId, ASN_INTEGER, IntegerToString(event.getCode(), buffer)));
   pdu.bindVariable(CreateVarbind(m_eventNameFieldId, ASN_OCTET_STRING, event.getName()));
   pdu.bindVariable(CreateVarbind(m_eventIdFieldId, ASN_OCTET_STRING, IntegerToString(event.getId(), buffer)));

   StringBuffer tags = event.getTagsAsList();
   if (!tags.isEmpty())
      pdu.bindVariable(CreateVarbind(m_tagsFieldId, ASN_OCTET_STRING, tags.cstr()));

   if (event.getParametersCount() > 0)
   {
      json_t *parameters = json_object();
      const StringList *names = event.getParameterNames();
      for(int i = 0; i < event.getParametersCount(); i++)
      {
         const wchar_t *name = names->get(i);
         char key[64];
         if ((name != nullptr) && (name[0] != 0))
         {
            char *utf8name = UTF8StringFromWideString(name);
            strlcpy(key, utf8name, sizeof(key));
            MemFree(utf8name);
         }
         else
         {
            snprintf(key, sizeof(key), "p%d", i + 1);
         }
         json_object_set_new(parameters, key, json_string_w(event.getParameter(i, L"")));
      }
      char *text = json_dumps(parameters, JSON_COMPACT);
      json_decref(parameters);
      if (text != nullptr)
      {
         wchar_t *wtext = WideStringFromUTF8String(text);
         pdu.bindVariable(CreateVarbind(m_parametersFieldId, ASN_OCTET_STRING, wtext));
         MemFree(wtext);
         MemFree(text);
      }
   }

   SNMP_UDPTransport transport;
   uint32_t rc = transport.createUDPTransport(target, m_port);
   if (rc != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SNMP trap forwarder: cannot create SNMP transport for target %s (%s)", target, SnmpGetErrorText(rc));
      return false;
   }

   if (m_version == SNMP_VERSION_3)
      transport.setSecurityContext(new SNMP_SecurityContext(m_authName, m_authPassword, m_privPassword, m_authMethod, m_privMethod));
   else
      transport.setSecurityContext(new SNMP_SecurityContext(m_authName));

   rc = transport.sendTrap(&pdu, 2000);
   if (rc != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SNMP trap forwarder: cannot send trap to %s (%s)", target, SnmpGetErrorText(rc));
      return false;
   }
   return true;
}

/**
 * Read OID from driver configuration, keeping the default if the key is absent
 */
static bool ReadOidFromConfig(json_t *config, const char *key, SNMP_ObjectId *oid)
{
   wchar_t *value = json_object_get_string_w(config, key, nullptr);
   if (value == nullptr)
      return true;   // not set, keep default
   *oid = SNMP_ObjectId::parse(value);
   bool valid = oid->isValid();
   if (!valid)
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"SNMP trap forwarder: invalid OID \"%s\" for configuration parameter \"%hs\"", value, key);
   MemFree(value);
   return valid;
}

/**
 * Return error from SnmpTrapEventForwarderDriver::createInstance
 */
#define RETURN_ERROR(msg) do { nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, msg); delete driver; return nullptr; } while(0)

/**
 * Create driver instance from JSON configuration
 */
SnmpTrapEventForwarderDriver *SnmpTrapEventForwarderDriver::createInstance(json_t *config)
{
   SnmpTrapEventForwarderDriver *driver = new SnmpTrapEventForwarderDriver();

   wchar_t *hostname = json_object_get_string_w(config, "hostname", L"");
   wcslcpy(driver->m_hostname, hostname, MAX_DNS_NAME);
   MemFree(hostname);

   driver->m_port = static_cast<uint16_t>(json_object_get_int32(config, "port", 162));

   char *s = json_object_get_string_a(config, "version", "2c");
   if (!strcmp(s, "1"))
      driver->m_version = SNMP_VERSION_1;
   else if (!strcmp(s, "2") || !stricmp(s, "2c"))
      driver->m_version = SNMP_VERSION_2C;
   else if (!strcmp(s, "3"))
      driver->m_version = SNMP_VERSION_3;
   else
   {
      MemFree(s);
      RETURN_ERROR(L"SNMP trap forwarder: invalid SNMP protocol version in configuration");
   }
   MemFree(s);

   if (driver->m_version == SNMP_VERSION_3)
   {
      s = json_object_get_string_a(config, "authMethod", "none");
      if (!stricmp(s, "none"))
         driver->m_authMethod = SNMP_AUTH_NONE;
      else if (!stricmp(s, "md5"))
         driver->m_authMethod = SNMP_AUTH_MD5;
      else if (!stricmp(s, "sha1"))
         driver->m_authMethod = SNMP_AUTH_SHA1;
      else if (!stricmp(s, "sha224"))
         driver->m_authMethod = SNMP_AUTH_SHA224;
      else if (!stricmp(s, "sha256"))
         driver->m_authMethod = SNMP_AUTH_SHA256;
      else if (!stricmp(s, "sha384"))
         driver->m_authMethod = SNMP_AUTH_SHA384;
      else if (!stricmp(s, "sha512"))
         driver->m_authMethod = SNMP_AUTH_SHA512;
      else
      {
         MemFree(s);
         RETURN_ERROR(L"SNMP trap forwarder: invalid SNMP authentication method in configuration");
      }
      MemFree(s);

      s = json_object_get_string_a(config, "privMethod", "none");
      if (!stricmp(s, "none"))
         driver->m_privMethod = SNMP_ENCRYPT_NONE;
      else if (!stricmp(s, "des"))
         driver->m_privMethod = SNMP_ENCRYPT_DES;
      else if (!stricmp(s, "aes") || !stricmp(s, "aes-128"))
         driver->m_privMethod = SNMP_ENCRYPT_AES_128;
      else if (!stricmp(s, "aes-192"))
         driver->m_privMethod = SNMP_ENCRYPT_AES_192;
      else if (!stricmp(s, "aes-256"))
         driver->m_privMethod = SNMP_ENCRYPT_AES_256;
      else
      {
         MemFree(s);
         RETURN_ERROR(L"SNMP trap forwarder: invalid SNMP privacy method in configuration");
      }
      MemFree(s);

      s = json_object_get_string_a(config, "userName", "netxms");
      strlcpy(driver->m_authName, s, sizeof(driver->m_authName));
      MemFree(s);
      s = json_object_get_string_a(config, "authPassword", "");
      strlcpy(driver->m_authPassword, s, sizeof(driver->m_authPassword));
      MemFree(s);
      s = json_object_get_string_a(config, "privPassword", "");
      strlcpy(driver->m_privPassword, s, sizeof(driver->m_privPassword));
      MemFree(s);
      DecryptPasswordA(driver->m_authName, driver->m_authPassword, driver->m_authPassword, sizeof(driver->m_authPassword));
      DecryptPasswordA(driver->m_authName, driver->m_privPassword, driver->m_privPassword, sizeof(driver->m_privPassword));
      driver->m_useInformRequest = json_object_get_boolean(config, "useInformRequest", false);
   }
   else
   {
      s = json_object_get_string_a(config, "community", "public");
      strlcpy(driver->m_authName, s, sizeof(driver->m_authName));
      MemFree(s);
      DecryptPasswordA("netxms", driver->m_authName, driver->m_authName, sizeof(driver->m_authName));
   }

   if (!ReadOidFromConfig(config, "trapId", &driver->m_trapId) ||
       !ReadOidFromConfig(config, "sourceNameFieldId", &driver->m_sourceNameFieldId) ||
       !ReadOidFromConfig(config, "severityFieldId", &driver->m_severityFieldId) ||
       !ReadOidFromConfig(config, "messageFieldId", &driver->m_messageFieldId) ||
       !ReadOidFromConfig(config, "timestampFieldId", &driver->m_timestampFieldId) ||
       !ReadOidFromConfig(config, "eventCodeFieldId", &driver->m_eventCodeFieldId) ||
       !ReadOidFromConfig(config, "eventNameFieldId", &driver->m_eventNameFieldId) ||
       !ReadOidFromConfig(config, "eventIdFieldId", &driver->m_eventIdFieldId) ||
       !ReadOidFromConfig(config, "sourceObjectIdFieldId", &driver->m_sourceObjectIdFieldId) ||
       !ReadOidFromConfig(config, "tagsFieldId", &driver->m_tagsFieldId) ||
       !ReadOidFromConfig(config, "parametersFieldId", &driver->m_parametersFieldId))
   {
      delete driver;
      return nullptr;
   }

   return driver;
}

/**
 * Built-in SNMP trap event forwarder driver factory
 */
EventForwarderDriver *CreateSnmpTrapEventForwarderDriver(json_t *configuration)
{
   return SnmpTrapEventForwarderDriver::createInstance(configuration);
}
