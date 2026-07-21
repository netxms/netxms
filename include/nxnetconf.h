/*
** NetXMS - Network Management System
** NETCONF protocol support library
** Copyright (C) 2026 Raden Solutions
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
** File: nxnetconf.h
**
**/

#ifndef _nxnetconf_h_
#define _nxnetconf_h_

#include <nms_common.h>
#include <nms_util.h>
#include <pugixml.h>

#ifdef LIBNXNETCONF_EXPORTS
#define LIBNXNETCONF_EXPORTABLE __EXPORT
#else
#define LIBNXNETCONF_EXPORTABLE __IMPORT
#endif

/**
 * Default port for NETCONF over SSH (RFC 6242)
 */
#ifndef NETCONF_DEFAULT_PORT
#define NETCONF_DEFAULT_PORT           830
#endif

/**
 * NETCONF base namespace (used as default namespace in all protocol messages)
 */
#define NETCONF_XML_NAMESPACE          "urn:ietf:params:xml:ns:netconf:base:1.0"

/**
 * Base protocol capability URIs
 */
#define NETCONF_CAPABILITY_BASE_1_0         _T("urn:ietf:params:netconf:base:1.0")
#define NETCONF_CAPABILITY_BASE_1_1         _T("urn:ietf:params:netconf:base:1.1")
#define NETCONF_CAPABILITY_BASE_1_0_UTF8    "urn:ietf:params:netconf:base:1.0"
#define NETCONF_CAPABILITY_BASE_1_1_UTF8    "urn:ietf:params:netconf:base:1.1"

/**
 * Standard capability URIs (RFC 6241 section 8)
 */
#define NETCONF_CAPABILITY_CANDIDATE        _T("urn:ietf:params:netconf:capability:candidate:1.0")
#define NETCONF_CAPABILITY_CONFIRMED_COMMIT _T("urn:ietf:params:netconf:capability:confirmed-commit:1.1")
#define NETCONF_CAPABILITY_ROLLBACK_ON_ERROR _T("urn:ietf:params:netconf:capability:rollback-on-error:1.0")
#define NETCONF_CAPABILITY_STARTUP          _T("urn:ietf:params:netconf:capability:startup:1.0")
#define NETCONF_CAPABILITY_VALIDATE         _T("urn:ietf:params:netconf:capability:validate:1.1")
#define NETCONF_CAPABILITY_WRITABLE_RUNNING _T("urn:ietf:params:netconf:capability:writable-running:1.0")
#define NETCONF_CAPABILITY_XPATH            _T("urn:ietf:params:netconf:capability:xpath:1.0")

/**
 * NETCONF protocol version
 */
enum class NetconfVersion
{
   NETCONF_1_0 = 0,    // RFC 6241 with end-of-message framing
   NETCONF_1_1 = 1     // RFC 6241 with chunked framing
};

/**
 * Standard NETCONF datastores
 */
enum class NetconfDatastore
{
   RUNNING = 0,
   CANDIDATE = 1,
   STARTUP = 2
};

/**
 * Filter types for get / get-config requests
 */
enum class NetconfFilterType
{
   NONE = 0,
   SUBTREE = 1,
   XPATH = 2
};

/**
 * Default operation for edit-config request (UNSPECIFIED omits the element, device default is "merge")
 */
enum class NetconfDefaultOperation
{
   UNSPECIFIED = 0,
   MERGE = 1,
   REPLACE = 2,
   NONE = 3
};

/**
 * Test option for edit-config request (UNSPECIFIED omits the element)
 */
enum class NetconfTestOption
{
   UNSPECIFIED = 0,
   TEST_THEN_SET = 1,
   SET = 2,
   TEST_ONLY = 3
};

/**
 * Error option for edit-config request (UNSPECIFIED omits the element, device default is "stop-on-error")
 */
enum class NetconfErrorOption
{
   UNSPECIFIED = 0,
   STOP_ON_ERROR = 1,
   CONTINUE_ON_ERROR = 2,
   ROLLBACK_ON_ERROR = 3
};

/**
 * Set of NETCONF capabilities announced in hello message
 */
class LIBNXNETCONF_EXPORTABLE NETCONF_CapabilitySet
{
private:
   StringList m_capabilities;

public:
   NETCONF_CapabilitySet() : m_capabilities() {}

   void add(const TCHAR *uri);
   void addFromUTF8String(const char *uri);
   void clear() { m_capabilities.clear(); }

   int size() const { return m_capabilities.size(); }
   bool isEmpty() const { return m_capabilities.isEmpty(); }
   const TCHAR *get(int index) const { return m_capabilities.get(index); }
   const StringList& getAll() const { return m_capabilities; }

   bool contains(const TCHAR *uri) const { return m_capabilities.contains(uri); }
   bool containsBase(const TCHAR *baseUri) const;
   String getParameter(const TCHAR *baseUri, const TCHAR *name) const;
   String findModule(const TCHAR *moduleName) const;
   NetconfVersion getProtocolVersion() const;

   bool parseHelloMessage(const char *data, size_t size, uint32_t *sessionId = nullptr);
};

/**
 * Error information from NETCONF rpc-error element (RFC 6241 section 4.3)
 */
class LIBNXNETCONF_EXPORTABLE NETCONF_Error
{
private:
   String m_type;
   String m_tag;
   String m_severity;
   String m_appTag;
   String m_path;
   String m_message;

public:
   NETCONF_Error(const pugi::xml_node& node);

   const String& getType() const { return m_type; }
   const String& getTag() const { return m_tag; }
   const String& getSeverity() const { return m_severity; }
   const String& getAppTag() const { return m_appTag; }
   const String& getPath() const { return m_path; }
   const String& getMessage() const { return m_message; }
   bool isWarning() const { return m_severity.equals(_T("warning")); }
};

/**
 * Parsed NETCONF rpc-reply message
 */
class LIBNXNETCONF_EXPORTABLE NETCONF_Response
{
private:
   pugi::xml_document m_document;
   pugi::xml_node m_reply;
   uint32_t m_messageId;
   bool m_valid;
   ObjectArray<NETCONF_Error> m_errors;

public:
   NETCONF_Response();

   bool parse(const char *data, size_t size);

   bool isValid() const { return m_valid; }
   uint32_t getMessageId() const { return m_messageId; }
   bool isOk() const;
   bool isSuccess() const;
   bool hasErrors() const { return !m_errors.isEmpty(); }
   const ObjectArray<NETCONF_Error>& getErrors() const { return m_errors; }
   String getErrorText() const;

   const pugi::xml_document& getDocument() const { return m_document; }
   pugi::xml_node getReplyElement() const { return m_reply; }
   pugi::xml_node getData() const;
   String getValueByXPath(const char *xpath) const;
};

/**
 * Incremental decoder for NETCONF framing (RFC 6242). Fed with raw bytes from transport,
 * produces complete XML messages. Framing mode can be switched after successful version
 * negotiation (must be done on message boundary).
 */
class LIBNXNETCONF_EXPORTABLE NETCONF_MessageDecoder
{
private:
   ByteStream m_input;
   ByteStream m_message;
   ObjectArray<ByteStream> m_completedMessages;
   NetconfVersion m_version;
   int m_state;
   uint64_t m_chunkSize;
   size_t m_scanPos;
   size_t m_maxMessageSize;
   bool m_error;

   void decodeEomFrames();
   void decodeChunkedFrames();
   void completeMessage(const void *data, size_t size);

public:
   NETCONF_MessageDecoder(NetconfVersion version = NetconfVersion::NETCONF_1_0, size_t maxMessageSize = 0x10000000);

   NetconfVersion getVersion() const { return m_version; }
   void setVersion(NetconfVersion version);

   void feed(const void *data, size_t size);
   char *readMessage(size_t *size = nullptr);
   bool isError() const { return m_error; }
};

/**
 * Message builders. Builders that accept XML fragments (filters, configuration content)
 * return false if the fragment cannot be parsed; other builders cannot fail.
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildHelloMessage(pugi::xml_document& doc, const NETCONF_CapabilitySet *additionalCapabilities = nullptr);
pugi::xml_node LIBNXNETCONF_EXPORTABLE NETCONF_CreateRpcEnvelope(pugi::xml_document& doc, uint32_t messageId);
bool LIBNXNETCONF_EXPORTABLE NETCONF_BuildGetRequest(pugi::xml_document& doc, uint32_t messageId,
      NetconfFilterType filterType = NetconfFilterType::NONE, const char *filter = nullptr);
bool LIBNXNETCONF_EXPORTABLE NETCONF_BuildGetConfigRequest(pugi::xml_document& doc, uint32_t messageId,
      NetconfDatastore source = NetconfDatastore::RUNNING, NetconfFilterType filterType = NetconfFilterType::NONE, const char *filter = nullptr);
bool LIBNXNETCONF_EXPORTABLE NETCONF_BuildEditConfigRequest(pugi::xml_document& doc, uint32_t messageId,
      NetconfDatastore target, const char *config,
      NetconfDefaultOperation defaultOperation = NetconfDefaultOperation::UNSPECIFIED,
      NetconfTestOption testOption = NetconfTestOption::UNSPECIFIED,
      NetconfErrorOption errorOption = NetconfErrorOption::UNSPECIFIED);
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildLockRequest(pugi::xml_document& doc, uint32_t messageId, NetconfDatastore target);
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildUnlockRequest(pugi::xml_document& doc, uint32_t messageId, NetconfDatastore target);
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildCommitRequest(pugi::xml_document& doc, uint32_t messageId);
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildValidateRequest(pugi::xml_document& doc, uint32_t messageId, NetconfDatastore source);
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildDiscardChangesRequest(pugi::xml_document& doc, uint32_t messageId);
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildCloseSessionRequest(pugi::xml_document& doc, uint32_t messageId);
bool LIBNXNETCONF_EXPORTABLE NETCONF_BuildRpcRequest(pugi::xml_document& doc, uint32_t messageId, const char *content);

/**
 * Message encoding (serialization + framing)
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_EncodeMessage(const pugi::xml_document& doc, NetconfVersion version, ByteStream& out);
void LIBNXNETCONF_EXPORTABLE NETCONF_EncodeMessage(const char *data, size_t size, NetconfVersion version, ByteStream& out);

/**
 * XML helpers (namespace prefix agnostic access, XPath value extraction)
 */
const char LIBNXNETCONF_EXPORTABLE *NETCONF_LocalName(const pugi::xml_node& node);
pugi::xml_node LIBNXNETCONF_EXPORTABLE NETCONF_FindChildByLocalName(const pugi::xml_node& parent, const char *name);
String LIBNXNETCONF_EXPORTABLE NETCONF_GetValueByXPath(const pugi::xml_node& context, const char *xpath);
const char LIBNXNETCONF_EXPORTABLE *NETCONF_DatastoreName(NetconfDatastore datastore);

#endif   /* _nxnetconf_h_ */
