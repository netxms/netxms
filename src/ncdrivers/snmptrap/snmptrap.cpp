/* 
** NetXMS - Network Management System
** Notification channel driver for sending SNMP traps
** Copyright (C) 2021-2023 Raden Solutions
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
** File: snmptrap.cpp
**
**/

#include <ncdrv.h>
#include <nxsnmp.h>

#define DEBUG_TAG _T("ncd.snmptrap")

/**
 * SNMP request ID
 */
static VolatileCounter s_requestId = 0;

/**
 * Driver class
 */
class SNMPTrapDriver : public NCDriver
{
private:
   SNMP_ObjectId m_trapId;
   SNMP_ObjectId m_sourceFieldId;
   SNMP_ObjectId m_severityFieldId;
   SNMP_ObjectId m_messageFieldId;
   SNMP_ObjectId m_timestampFieldId;
   SNMP_ObjectId m_keyFieldId;
   SNMP_ObjectId m_additionalDataFieldId;
   char m_authName[128];
   char m_authPassword[128];
   char m_privPassword[128];
   SNMP_Version m_version;
   SNMP_AuthMethod m_authMethod;
   SNMP_EncryptionMethod m_privMethod;
   uint16_t m_port;
   bool m_useInformRequest;
   time_t m_startTime;

   SNMPTrapDriver();

public:
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;

   static SNMPTrapDriver *createInstance(Config *config);
};

/**
 * Create driver instance
 */
SNMPTrapDriver::SNMPTrapDriver()
{
   strcpy(m_authName, "public");
   m_authPassword[0] = 0;
   m_privPassword[0] = 0;
   m_version = SNMP_VERSION_2C;
   m_authMethod = SNMP_AUTH_NONE;
   m_privMethod = SNMP_ENCRYPT_NONE;
   m_port = 162;
   m_useInformRequest = false;
   m_startTime = time(nullptr);
}

/**
 * Create varbind with given value
 */
static inline SNMP_Variable *CreateVarbind(const SNMP_ObjectId& name, uint32_t type, const TCHAR *value)
{
   auto v = new SNMP_Variable(name);
   v->setValueFromString(type, value);
   return v;
}

/**
 * Send notification
 *
 * Subject can contain additional event data in key=value form with entries separated by semicolon or as JSON document.
 * Possible fields:
 *    key         - alarm key
 *    source      - source object name
 *    severity    - event severity (integer in range 0..4)
 *    timestamp   - original even timestamp as UNIX time
 */
int SNMPTrapDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("recipient=\"%s\", subject=\"%s\", text=\"%s\""), recipient, subject, body);

   SNMP_PDU pdu(m_useInformRequest ? SNMP_INFORM_REQUEST : SNMP_TRAP, m_version, m_trapId, static_cast<uint32_t>(time(nullptr) - m_startTime), InterlockedIncrement(&s_requestId));

   pdu.bindVariable(CreateVarbind(m_messageFieldId, ASN_OCTET_STRING, body));
   if ((subject != nullptr) && (*subject != 0))
   {
      pdu.bindVariable(CreateVarbind(m_additionalDataFieldId, ASN_OCTET_STRING, subject));

      if (*subject == _T('{'))
      {
         // JSON format
#ifdef UNICODE
         char *utfString = UTF8StringFromWideString(subject);
#else
         char *utfString = UTF8StringFromMBString(subject);
#endif
         json_error_t error;
         json_t *json = json_loads(utfString, 0, &error);
         MemFree(utfString);
         if (json != nullptr)
         {
            TCHAR *sv = json_object_get_string_t(json, "source", nullptr);
            if (sv != nullptr)
            {
               pdu.bindVariable(CreateVarbind(m_sourceFieldId, ASN_OCTET_STRING, sv));
               MemFree(sv);
            }

            int64_t iv = json_object_get_int64(json, "severity", -1);
            if (iv != -1)
            {
               TCHAR buffer[16];
               _sntprintf(buffer, 16, INT64_FMT, iv);
               pdu.bindVariable(CreateVarbind(m_severityFieldId, ASN_INTEGER, buffer));
            }

            iv = json_object_get_int64(json, "timestamp", -1);
            if (iv != -1)
            {
               TCHAR buffer[16];
               _sntprintf(buffer, 16, INT64_FMT, iv);
               pdu.bindVariable(CreateVarbind(m_timestampFieldId, ASN_TIMETICKS, buffer));
            }

            sv = json_object_get_string_t(json, "key", nullptr);
            if (sv != nullptr)
            {
               pdu.bindVariable(CreateVarbind(m_keyFieldId, ASN_OCTET_STRING, sv));
               MemFree(sv);
            }

            json_decref(json);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot parse options as JSON object (%hs)"), error.text);
         }
      }
      else
      {
         // Key=value format
         TCHAR value[256];
         if (ExtractNamedOptionValue(subject, _T("source"), value, 256))
            pdu.bindVariable(CreateVarbind(m_sourceFieldId, ASN_OCTET_STRING, value));
         if (ExtractNamedOptionValue(subject, _T("severity"), value, 256))
            pdu.bindVariable(CreateVarbind(m_severityFieldId, ASN_INTEGER, value));
         if (ExtractNamedOptionValue(subject, _T("timestamp"), value, 256))
            pdu.bindVariable(CreateVarbind(m_timestampFieldId, ASN_TIMETICKS, value));
         if (ExtractNamedOptionValue(subject, _T("key"), value, 256))
            pdu.bindVariable(CreateVarbind(m_keyFieldId, ASN_OCTET_STRING, value));
      }
   }

   SNMP_UDPTransport transport;
   uint32_t rc = transport.createUDPTransport(recipient, m_port);
   if (rc != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot create SNMP transport for target %s (%s)"), recipient, SnmpGetErrorText(rc));
      return -1;
   }

   if (m_version == SNMP_VERSION_3)
   {
      transport.setSecurityContext(new SNMP_SecurityContext(m_authName, m_authPassword, m_privPassword, m_authMethod, m_privMethod));
   }
   else
   {
      transport.setSecurityContext(new SNMP_SecurityContext(m_authName));
   }

   rc = transport.sendTrap(&pdu, 2000);
   if (rc != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot send SNMP trap to %s (%s)"), recipient, SnmpGetErrorText(rc));
      return -1;
   }
   return 0;
}

/**
 * Parse OID type configuration parameter
 */
static inline bool ParseOIDArgument(SNMP_ObjectId& id, const Config *config, const TCHAR *path, const TCHAR *defaultValue)
{
   id = SNMP_ObjectId::parse(config->getValue(path, defaultValue));
   if (id.isValid())
      return true;
   nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Invalid value for driver configuration parameter %s"), &path[10]);
   return false;
}

/**
 * Return error from SNMPTrapDriver::createInstance
 */
#define RETURN_ERROR(msg) do { nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, msg); delete instance; return nullptr; } while(0)

/**
 * Create driver instance
 */
SNMPTrapDriver *SNMPTrapDriver::createInstance(Config *config)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Creating new driver instance"));

   SNMPTrapDriver *instance = new SNMPTrapDriver();
   if (!ParseOIDArgument(instance->m_trapId, config, _T("/SNMPTrap/TrapID"), _T(".1.3.6.1.4.1.57163.1.0.1")) ||
       !ParseOIDArgument(instance->m_sourceFieldId, config, _T("/SNMPTrap/SourceFieldID"), _T(".1.3.6.1.4.1.57163.1.1.1.0")) ||
       !ParseOIDArgument(instance->m_severityFieldId, config, _T("/SNMPTrap/SeverityFieldID"), _T(".1.3.6.1.4.1.57163.1.1.2.0")) ||
       !ParseOIDArgument(instance->m_messageFieldId, config, _T("/SNMPTrap/MessageFieldID"), _T(".1.3.6.1.4.1.57163.1.1.3.0")) ||
       !ParseOIDArgument(instance->m_timestampFieldId, config, _T("/SNMPTrap/TimestampFieldID"), _T(".1.3.6.1.4.1.57163.1.1.4.0")) ||
       !ParseOIDArgument(instance->m_keyFieldId, config, _T("/SNMPTrap/AlarmKeyFieldID"), _T(".1.3.6.1.4.1.57163.1.1.5.0")) ||
       !ParseOIDArgument(instance->m_additionalDataFieldId, config, _T("/SNMPTrap/AdditionalDataFieldID"), _T(".1.3.6.1.4.1.57163.1.1.6.0")))
   {
      RETURN_ERROR(_T("Cannot parse OID in driver configuration"));
   }

   const TCHAR *v = config->getValue(_T("/SNMPTrap/ProtocolVersion"));
   if (v != nullptr)
   {
      if (!_tcscmp(v, _T("1")))
         instance->m_version = SNMP_VERSION_1;
      else if (!_tcscmp(v, _T("2")) || !_tcsicmp(v, _T("2c")))
         instance->m_version = SNMP_VERSION_2C;
      else if (!_tcscmp(v, _T("3")))
         instance->m_version = SNMP_VERSION_3;
      else
         RETURN_ERROR(_T("Invalid SNMP protocol version in driver configuration"));
   }

   v = config->getValue(_T("/SNMPTrap/AuthMethod"));
   if (v != nullptr)
   {
      if (!_tcsicmp(v, _T("md5")))
         instance->m_authMethod = SNMP_AUTH_MD5;
      else if (!_tcsicmp(v, _T("none")))
         instance->m_authMethod = SNMP_AUTH_NONE;
      else if (!_tcsicmp(v, _T("sha1")))
         instance->m_authMethod = SNMP_AUTH_SHA1;
      else if (!_tcsicmp(v, _T("sha224")))
         instance->m_authMethod = SNMP_AUTH_SHA224;
      else if (!_tcsicmp(v, _T("sha256")))
         instance->m_authMethod = SNMP_AUTH_SHA256;
      else if (!_tcsicmp(v, _T("sha384")))
         instance->m_authMethod = SNMP_AUTH_SHA384;
      else if (!_tcsicmp(v, _T("sha512")))
         instance->m_authMethod = SNMP_AUTH_SHA512;
      else
         RETURN_ERROR(_T("Invalid SNMP authentication method in driver configuration"));
   }

   v = config->getValue(_T("/SNMPTrap/PrivMethod"));
   if (v != nullptr)
   {
      if (!_tcsicmp(v, _T("aes")))
         instance->m_privMethod = SNMP_ENCRYPT_AES_128;
      else if (!_tcsicmp(v, _T("aes-128")))
         instance->m_privMethod = SNMP_ENCRYPT_AES_128;
      else if (!_tcsicmp(v, _T("aes-192")))
         instance->m_privMethod = SNMP_ENCRYPT_AES_192;
      else if (!_tcsicmp(v, _T("aes-256")))
         instance->m_privMethod = SNMP_ENCRYPT_AES_256;
      else if (!_tcsicmp(v, _T("des")))
         instance->m_privMethod = SNMP_ENCRYPT_DES;
      else if (!_tcsicmp(v, _T("none")))
         instance->m_privMethod = SNMP_ENCRYPT_NONE;
      else
         RETURN_ERROR(_T("Invalid SNMP privacy method in driver configuration"));
   }

   if (instance->m_version == SNMP_VERSION_3)
   {
#ifdef UNICODE
      char mbString[128];
      wchar_to_mb(config->getValue(L"/SNMPTrap/UserName", L"netxms"), -1, mbString, 128);
      strlcpy(instance->m_authName, mbString, 128);
      wchar_to_mb(config->getValue(L"/SNMPTrap/AuthPassword", L""), -1, mbString, 128);
      strlcpy(instance->m_authPassword, mbString, 128);
      wchar_to_mb(config->getValue(L"/SNMPTrap/PrivPassword", L""), -1, mbString, 128);
      strlcpy(instance->m_privPassword, mbString, 128);
#else
      strlcpy(instance->m_authPassword, config->getValue("/SNMPTrap/AuthPassword", ""), 128);
      strlcpy(instance->m_privPassword, config->getValue("/SNMPTrap/PrivPassword", ""), 128);
#endif
      DecryptPasswordA(instance->m_authName, instance->m_authPassword, instance->m_authPassword, 128);
      DecryptPasswordA(instance->m_authName, instance->m_privPassword, instance->m_privPassword, 128);
      instance->m_useInformRequest = config->getValueAsBoolean(_T("/SNMPTrap/UseInformRequest"), instance->m_useInformRequest);
   }
   else
   {
#ifdef UNICODE
      char mbString[128];
      wchar_to_mb(config->getValue(L"/SNMPTrap/Community", L"public"), -1, mbString, 128);
      strlcpy(instance->m_authName, mbString, 128);
#else
      strlcpy(instance->m_authName, config->getValue("/SNMPTrap/Community", "public"), 128);
#endif
      DecryptPasswordA("netxms", instance->m_authName, instance->m_authName, 128);
   }

   instance->m_port = static_cast<uint16_t>(config->getValueAsUInt(_T("/SNMPTrap/Port"), instance->m_port));

   return instance;
}

/**
 * Configuration template
 */
static const NCConfigurationTemplate s_configurationTemplate(true, true);

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(SNMPTrap, &s_configurationTemplate)
{
   return SNMPTrapDriver::createInstance(config);
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
