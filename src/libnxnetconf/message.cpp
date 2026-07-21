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
** File: message.cpp
**
**/

#include "libnxnetconf.h"

/**
 * Create empty rpc envelope within given document. Returned node can be used
 * to build custom RPC requests not covered by standard builders.
 */
pugi::xml_node LIBNXNETCONF_EXPORTABLE NETCONF_CreateRpcEnvelope(pugi::xml_document& doc, uint32_t messageId)
{
   doc.reset();
   pugi::xml_node rpc = doc.append_child("rpc");
   rpc.append_attribute("message-id") = messageId;
   rpc.append_attribute("xmlns") = NETCONF_XML_NAMESPACE;
   return rpc;
}

/**
 * Append filter element to get / get-config operation element
 */
static bool AppendFilter(pugi::xml_node operation, NetconfFilterType filterType, const char *filter)
{
   if (filterType == NetconfFilterType::NONE)
      return true;

   pugi::xml_node filterNode = operation.append_child("filter");
   if (filterType == NetconfFilterType::XPATH)
   {
      filterNode.append_attribute("type") = "xpath";
      filterNode.append_attribute("select") = CHECK_NULL_EX_A(filter);
      return true;
   }

   filterNode.append_attribute("type") = "subtree";
   if ((filter == nullptr) || (*filter == 0))
      return true;
   return filterNode.append_buffer(filter, strlen(filter));
}

/**
 * Build get request
 */
bool LIBNXNETCONF_EXPORTABLE NETCONF_BuildGetRequest(pugi::xml_document& doc, uint32_t messageId, NetconfFilterType filterType, const char *filter)
{
   pugi::xml_node rpc = NETCONF_CreateRpcEnvelope(doc, messageId);
   return AppendFilter(rpc.append_child("get"), filterType, filter);
}

/**
 * Build get-config request
 */
bool LIBNXNETCONF_EXPORTABLE NETCONF_BuildGetConfigRequest(pugi::xml_document& doc, uint32_t messageId,
      NetconfDatastore source, NetconfFilterType filterType, const char *filter)
{
   pugi::xml_node rpc = NETCONF_CreateRpcEnvelope(doc, messageId);
   pugi::xml_node operation = rpc.append_child("get-config");
   operation.append_child("source").append_child(NETCONF_DatastoreName(source));
   return AppendFilter(operation, filterType, filter);
}

/**
 * Build edit-config request. Configuration content is an XML fragment placed
 * inside the config element.
 */
bool LIBNXNETCONF_EXPORTABLE NETCONF_BuildEditConfigRequest(pugi::xml_document& doc, uint32_t messageId,
      NetconfDatastore target, const char *config,
      NetconfDefaultOperation defaultOperation, NetconfTestOption testOption, NetconfErrorOption errorOption)
{
   pugi::xml_node rpc = NETCONF_CreateRpcEnvelope(doc, messageId);
   pugi::xml_node operation = rpc.append_child("edit-config");
   operation.append_child("target").append_child(NETCONF_DatastoreName(target));

   switch(defaultOperation)
   {
      case NetconfDefaultOperation::MERGE:
         operation.append_child("default-operation").text() = "merge";
         break;
      case NetconfDefaultOperation::REPLACE:
         operation.append_child("default-operation").text() = "replace";
         break;
      case NetconfDefaultOperation::NONE:
         operation.append_child("default-operation").text() = "none";
         break;
      default:
         break;
   }

   switch(testOption)
   {
      case NetconfTestOption::TEST_THEN_SET:
         operation.append_child("test-option").text() = "test-then-set";
         break;
      case NetconfTestOption::SET:
         operation.append_child("test-option").text() = "set";
         break;
      case NetconfTestOption::TEST_ONLY:
         operation.append_child("test-option").text() = "test-only";
         break;
      default:
         break;
   }

   switch(errorOption)
   {
      case NetconfErrorOption::STOP_ON_ERROR:
         operation.append_child("error-option").text() = "stop-on-error";
         break;
      case NetconfErrorOption::CONTINUE_ON_ERROR:
         operation.append_child("error-option").text() = "continue-on-error";
         break;
      case NetconfErrorOption::ROLLBACK_ON_ERROR:
         operation.append_child("error-option").text() = "rollback-on-error";
         break;
      default:
         break;
   }

   pugi::xml_node configNode = operation.append_child("config");
   if ((config == nullptr) || (*config == 0))
      return false;
   return configNode.append_buffer(config, strlen(config));
}

/**
 * Build lock request
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildLockRequest(pugi::xml_document& doc, uint32_t messageId, NetconfDatastore target)
{
   pugi::xml_node rpc = NETCONF_CreateRpcEnvelope(doc, messageId);
   rpc.append_child("lock").append_child("target").append_child(NETCONF_DatastoreName(target));
}

/**
 * Build unlock request
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildUnlockRequest(pugi::xml_document& doc, uint32_t messageId, NetconfDatastore target)
{
   pugi::xml_node rpc = NETCONF_CreateRpcEnvelope(doc, messageId);
   rpc.append_child("unlock").append_child("target").append_child(NETCONF_DatastoreName(target));
}

/**
 * Build commit request
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildCommitRequest(pugi::xml_document& doc, uint32_t messageId)
{
   NETCONF_CreateRpcEnvelope(doc, messageId).append_child("commit");
}

/**
 * Build validate request
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildValidateRequest(pugi::xml_document& doc, uint32_t messageId, NetconfDatastore source)
{
   pugi::xml_node rpc = NETCONF_CreateRpcEnvelope(doc, messageId);
   rpc.append_child("validate").append_child("source").append_child(NETCONF_DatastoreName(source));
}

/**
 * Build discard-changes request
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildDiscardChangesRequest(pugi::xml_document& doc, uint32_t messageId)
{
   NETCONF_CreateRpcEnvelope(doc, messageId).append_child("discard-changes");
}

/**
 * Build close-session request
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildCloseSessionRequest(pugi::xml_document& doc, uint32_t messageId)
{
   NETCONF_CreateRpcEnvelope(doc, messageId).append_child("close-session");
}

/**
 * Build raw RPC request from XML fragment placed inside the rpc envelope
 */
bool LIBNXNETCONF_EXPORTABLE NETCONF_BuildRpcRequest(pugi::xml_document& doc, uint32_t messageId, const char *content)
{
   pugi::xml_node rpc = NETCONF_CreateRpcEnvelope(doc, messageId);
   if ((content == nullptr) || (*content == 0))
      return false;
   return rpc.append_buffer(content, strlen(content));
}

/**
 * XML writer implementation for writing into byte stream
 */
class ByteStreamXmlWriter : public pugi::xml_writer
{
private:
   ByteStream& m_out;

public:
   ByteStreamXmlWriter(ByteStream& out) : m_out(out) {}

   virtual void write(const void *data, size_t size) override
   {
      m_out.write(data, size);
   }
};

/**
 * Encode raw message data with framing appropriate for given protocol version
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_EncodeMessage(const char *data, size_t size, NetconfVersion version, ByteStream& out)
{
   if (version == NetconfVersion::NETCONF_1_1)
   {
      char header[32];
      snprintf(header, sizeof(header), "\n#%u\n", static_cast<unsigned int>(size));
      out.write(header, strlen(header));
      out.write(data, size);
      out.write("\n##\n", 4);
   }
   else
   {
      out.write(data, size);
      out.write("]]>]]>", 6);
   }
}

/**
 * Serialize XML document and encode it with framing appropriate for given protocol version
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_EncodeMessage(const pugi::xml_document& doc, NetconfVersion version, ByteStream& out)
{
   ByteStream message(4096);
   static const char declaration[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
   message.write(declaration, sizeof(declaration) - 1);

   ByteStreamXmlWriter writer(message);
   doc.print(writer, "", pugi::format_raw, pugi::encoding_utf8);

   NETCONF_EncodeMessage(reinterpret_cast<const char*>(message.buffer()), message.size(), version, out);
}
