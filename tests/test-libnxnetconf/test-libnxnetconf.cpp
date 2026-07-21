/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: test-libnxnetconf.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxnetconf.h>
#include <testtools.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(test-libnxnetconf)

/**
 * Server hello message (JunOS style, with YANG module capabilities)
 */
static const char s_serverHello[] =
   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
   "<hello xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
   "<capabilities>"
   "<capability>urn:ietf:params:netconf:base:1.0</capability>"
   "<capability>urn:ietf:params:netconf:base:1.1</capability>"
   "<capability>urn:ietf:params:netconf:capability:candidate:1.0</capability>"
   "<capability>urn:ietf:params:netconf:capability:validate:1.1</capability>"
   "<capability>http://example.com/yang/system?module=example-system&amp;revision=2024-01-15</capability>"
   "<capability>http://example.com/yang/interfaces?module=example-interfaces&amp;revision=2023-11-02&amp;features=vlan</capability>"
   "</capabilities>"
   "<session-id>4711</session-id>"
   "</hello>";

/**
 * Test capability set class
 */
static void TestCapabilitySet()
{
   StartTest(_T("NETCONF_CapabilitySet - add and lookup"));
   NETCONF_CapabilitySet capabilities;
   AssertTrue(capabilities.isEmpty());
   capabilities.add(NETCONF_CAPABILITY_BASE_1_0);
   capabilities.add(NETCONF_CAPABILITY_BASE_1_0);   // duplicates should be ignored
   capabilities.addFromUTF8String("urn:ietf:params:netconf:capability:candidate:1.0");
   AssertEquals(capabilities.size(), 2);
   AssertTrue(capabilities.contains(NETCONF_CAPABILITY_BASE_1_0));
   AssertTrue(capabilities.contains(NETCONF_CAPABILITY_CANDIDATE));
   AssertFalse(capabilities.contains(NETCONF_CAPABILITY_BASE_1_1));
   EndTest();

   StartTest(_T("NETCONF_CapabilitySet - protocol version"));
   AssertTrue(capabilities.getProtocolVersion() == NetconfVersion::NETCONF_1_0);
   capabilities.add(NETCONF_CAPABILITY_BASE_1_1);
   AssertTrue(capabilities.getProtocolVersion() == NetconfVersion::NETCONF_1_1);
   EndTest();

   StartTest(_T("NETCONF_CapabilitySet - base URI matching"));
   capabilities.add(_T("http://example.com/yang/system?module=example-system&revision=2024-01-15"));
   AssertTrue(capabilities.containsBase(_T("http://example.com/yang/system")));
   AssertTrue(capabilities.containsBase(NETCONF_CAPABILITY_BASE_1_0));
   AssertFalse(capabilities.containsBase(_T("http://example.com/yang/sys")));
   AssertFalse(capabilities.containsBase(_T("http://example.com/yang/routing")));
   EndTest();

   StartTest(_T("NETCONF_CapabilitySet - parameter extraction"));
   AssertTrue(capabilities.getParameter(_T("http://example.com/yang/system"), _T("revision")).equals(_T("2024-01-15")));
   AssertTrue(capabilities.getParameter(_T("http://example.com/yang/system"), _T("module")).equals(_T("example-system")));
   AssertTrue(capabilities.getParameter(_T("http://example.com/yang/system"), _T("features")).isEmpty());
   AssertTrue(capabilities.getParameter(_T("http://example.com/yang/routing"), _T("revision")).isEmpty());
   EndTest();

   StartTest(_T("NETCONF_CapabilitySet - module lookup"));
   AssertTrue(capabilities.findModule(_T("example-system")).equals(_T("http://example.com/yang/system?module=example-system&revision=2024-01-15")));
   AssertTrue(capabilities.findModule(_T("example-sys")).isEmpty());
   AssertTrue(capabilities.findModule(_T("nonexistent")).isEmpty());
   EndTest();
}

/**
 * Test hello message building and parsing
 */
static void TestHelloMessage()
{
   StartTest(_T("Hello message - build"));
   NETCONF_CapabilitySet additionalCapabilities;
   additionalCapabilities.add(NETCONF_CAPABILITY_BASE_1_0);   // should not be announced twice
   additionalCapabilities.add(_T("urn:example:capability:1.0"));
   pugi::xml_document document;
   NETCONF_BuildHelloMessage(document, &additionalCapabilities);
   pugi::xml_node hello = document.child("hello");
   AssertTrue(hello);
   AssertEquals(hello.attribute("xmlns").value(), NETCONF_XML_NAMESPACE);
   int count = 0;
   for(pugi::xml_node capability = hello.child("capabilities").child("capability"); capability; capability = capability.next_sibling("capability"))
      count++;
   AssertEquals(count, 3);
   EndTest();

   StartTest(_T("Hello message - build/parse round trip"));
   ByteStream stream;
   NETCONF_EncodeMessage(document, NetconfVersion::NETCONF_1_0, stream);
   NETCONF_MessageDecoder decoder;
   decoder.feed(stream.buffer(), stream.size());
   size_t size;
   char *message = decoder.readMessage(&size);
   AssertNotNull(message);
   NETCONF_CapabilitySet parsedCapabilities;
   uint32_t sessionId = 0xFFFFFFFF;
   AssertTrue(parsedCapabilities.parseHelloMessage(message, size, &sessionId));
   MemFree(message);
   AssertEquals(parsedCapabilities.size(), 3);
   AssertTrue(parsedCapabilities.contains(NETCONF_CAPABILITY_BASE_1_0));
   AssertTrue(parsedCapabilities.contains(NETCONF_CAPABILITY_BASE_1_1));
   AssertTrue(parsedCapabilities.contains(_T("urn:example:capability:1.0")));
   AssertEquals(sessionId, 0u);   // client hello has no session ID
   EndTest();

   StartTest(_T("Hello message - parse server hello"));
   NETCONF_CapabilitySet serverCapabilities;
   sessionId = 0;
   AssertTrue(serverCapabilities.parseHelloMessage(s_serverHello, strlen(s_serverHello), &sessionId));
   AssertEquals(sessionId, 4711u);
   AssertEquals(serverCapabilities.size(), 6);
   AssertTrue(serverCapabilities.contains(NETCONF_CAPABILITY_CANDIDATE));
   AssertTrue(serverCapabilities.contains(NETCONF_CAPABILITY_VALIDATE));
   AssertTrue(serverCapabilities.getProtocolVersion() == NetconfVersion::NETCONF_1_1);
   AssertTrue(serverCapabilities.getParameter(_T("http://example.com/yang/interfaces"), _T("features")).equals(_T("vlan")));
   AssertTrue(serverCapabilities.findModule(_T("example-interfaces")).startsWith(_T("http://example.com/yang/interfaces?")));
   EndTest();

   StartTest(_T("Hello message - parse errors"));
   NETCONF_CapabilitySet c1;
   AssertFalse(c1.parseHelloMessage("not an XML document", 18));
   static const char *notHello = "<rpc-reply xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"><ok/></rpc-reply>";
   AssertFalse(c1.parseHelloMessage(notHello, strlen(notHello)));
   static const char *noCapabilities = "<hello xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"><session-id>1</session-id></hello>";
   AssertFalse(c1.parseHelloMessage(noCapabilities, strlen(noCapabilities)));
   EndTest();
}

/**
 * Test RPC request builders
 */
static void TestRequestBuilders()
{
   StartTest(_T("Request builder - rpc envelope"));
   pugi::xml_document document;
   pugi::xml_node rpc = NETCONF_CreateRpcEnvelope(document, 42);
   AssertTrue(rpc);
   AssertEquals(rpc.attribute("message-id").as_uint(), 42u);
   AssertEquals(rpc.attribute("xmlns").value(), NETCONF_XML_NAMESPACE);
   EndTest();

   StartTest(_T("Request builder - get without filter"));
   AssertTrue(NETCONF_BuildGetRequest(document, 1));
   rpc = document.child("rpc");
   AssertTrue(rpc.child("get"));
   AssertFalse(rpc.child("get").child("filter"));
   EndTest();

   StartTest(_T("Request builder - get with subtree filter"));
   AssertTrue(NETCONF_BuildGetRequest(document, 2, NetconfFilterType::SUBTREE, "<system><clock/></system>"));
   pugi::xml_node filter = document.child("rpc").child("get").child("filter");
   AssertTrue(filter);
   AssertEquals(filter.attribute("type").value(), "subtree");
   AssertTrue(filter.child("system").child("clock"));
   EndTest();

   StartTest(_T("Request builder - get with XPath filter"));
   AssertTrue(NETCONF_BuildGetRequest(document, 3, NetconfFilterType::XPATH, "/system/clock"));
   filter = document.child("rpc").child("get").child("filter");
   AssertEquals(filter.attribute("type").value(), "xpath");
   AssertEquals(filter.attribute("select").value(), "/system/clock");
   EndTest();

   StartTest(_T("Request builder - get with invalid subtree filter"));
   AssertFalse(NETCONF_BuildGetRequest(document, 4, NetconfFilterType::SUBTREE, "<system><clock></system>"));
   EndTest();

   StartTest(_T("Request builder - get-config"));
   AssertTrue(NETCONF_BuildGetConfigRequest(document, 5, NetconfDatastore::CANDIDATE, NetconfFilterType::SUBTREE, "<interfaces/>"));
   pugi::xml_node operation = document.child("rpc").child("get-config");
   AssertTrue(operation.child("source").child("candidate"));
   AssertTrue(operation.child("filter").child("interfaces"));
   EndTest();

   StartTest(_T("Request builder - edit-config"));
   AssertTrue(NETCONF_BuildEditConfigRequest(document, 6, NetconfDatastore::CANDIDATE,
         "<interfaces><interface><name>eth0</name><mtu>9000</mtu></interface></interfaces>",
         NetconfDefaultOperation::MERGE, NetconfTestOption::TEST_THEN_SET, NetconfErrorOption::ROLLBACK_ON_ERROR));
   operation = document.child("rpc").child("edit-config");
   AssertTrue(operation.child("target").child("candidate"));
   AssertEquals(operation.child("default-operation").child_value(), "merge");
   AssertEquals(operation.child("test-option").child_value(), "test-then-set");
   AssertEquals(operation.child("error-option").child_value(), "rollback-on-error");
   AssertEquals(operation.child("config").child("interfaces").child("interface").child("mtu").child_value(), "9000");
   EndTest();

   StartTest(_T("Request builder - edit-config with default options"));
   AssertTrue(NETCONF_BuildEditConfigRequest(document, 7, NetconfDatastore::RUNNING, "<system/>"));
   operation = document.child("rpc").child("edit-config");
   AssertTrue(operation.child("target").child("running"));
   AssertFalse(operation.child("default-operation"));
   AssertFalse(operation.child("test-option"));
   AssertFalse(operation.child("error-option"));
   EndTest();

   StartTest(_T("Request builder - edit-config without content"));
   AssertFalse(NETCONF_BuildEditConfigRequest(document, 8, NetconfDatastore::RUNNING, nullptr));
   AssertFalse(NETCONF_BuildEditConfigRequest(document, 9, NetconfDatastore::RUNNING, ""));
   EndTest();

   StartTest(_T("Request builder - lock/unlock"));
   NETCONF_BuildLockRequest(document, 10, NetconfDatastore::RUNNING);
   AssertTrue(document.child("rpc").child("lock").child("target").child("running"));
   NETCONF_BuildUnlockRequest(document, 11, NetconfDatastore::STARTUP);
   AssertTrue(document.child("rpc").child("unlock").child("target").child("startup"));
   EndTest();

   StartTest(_T("Request builder - commit/validate/discard-changes/close-session"));
   NETCONF_BuildCommitRequest(document, 12);
   AssertTrue(document.child("rpc").child("commit"));
   NETCONF_BuildValidateRequest(document, 13, NetconfDatastore::CANDIDATE);
   AssertTrue(document.child("rpc").child("validate").child("source").child("candidate"));
   NETCONF_BuildDiscardChangesRequest(document, 14);
   AssertTrue(document.child("rpc").child("discard-changes"));
   NETCONF_BuildCloseSessionRequest(document, 15);
   AssertTrue(document.child("rpc").child("close-session"));
   EndTest();

   StartTest(_T("Request builder - raw RPC"));
   AssertTrue(NETCONF_BuildRpcRequest(document, 16, "<get-schema xmlns=\"urn:ietf:params:xml:ns:yang:ietf-netconf-monitoring\"><identifier>example-system</identifier></get-schema>"));
   rpc = document.child("rpc");
   AssertEquals(rpc.attribute("message-id").as_uint(), 16u);
   AssertEquals(rpc.child("get-schema").child("identifier").child_value(), "example-system");
   AssertFalse(NETCONF_BuildRpcRequest(document, 17, "<unbalanced"));
   EndTest();
}

/**
 * Encode data in chunked framing using given maximum chunk size
 */
static void EncodeChunked(const char *data, size_t chunkSize, ByteStream& out)
{
   size_t length = strlen(data);
   for(size_t i = 0; i < length; i += chunkSize)
   {
      size_t count = std::min(chunkSize, length - i);
      char header[32];
      snprintf(header, sizeof(header), "\n#%u\n", static_cast<unsigned int>(count));
      out.write(header, strlen(header));
      out.write(data + i, count);
   }
   out.write("\n##\n", 4);
}

/**
 * Test end-of-message framing (NETCONF 1.0)
 */
static void TestFramingEom()
{
   StartTest(_T("EOM framing - encoding"));
   ByteStream stream;
   NETCONF_EncodeMessage("<rpc/>", 6, NetconfVersion::NETCONF_1_0, stream);
   AssertEquals(stream.size(), 12);
   AssertTrue(!memcmp(stream.buffer(), "<rpc/>]]>]]>", 12));
   EndTest();

   StartTest(_T("EOM framing - single message"));
   NETCONF_MessageDecoder decoder(NetconfVersion::NETCONF_1_0);
   AssertTrue(decoder.getVersion() == NetconfVersion::NETCONF_1_0);
   decoder.feed("<hello/>]]>]]>", 14);
   size_t size;
   char *message = decoder.readMessage(&size);
   AssertNotNull(message);
   AssertEquals(size, 8);
   AssertEquals(message, "<hello/>");
   MemFree(message);
   AssertNull(decoder.readMessage());
   AssertFalse(decoder.isError());
   EndTest();

   StartTest(_T("EOM framing - multiple messages in single feed"));
   decoder.feed("<a/>]]>]]><b/>]]>]]>", 20);
   message = decoder.readMessage();
   AssertNotNull(message);
   AssertEquals(message, "<a/>");
   MemFree(message);
   message = decoder.readMessage();
   AssertNotNull(message);
   AssertEquals(message, "<b/>");
   MemFree(message);
   AssertNull(decoder.readMessage());
   EndTest();

   StartTest(_T("EOM framing - delimiter split across feeds"));
   decoder.feed("<partial/>]]", 12);
   AssertNull(decoder.readMessage());
   decoder.feed(">]", 2);
   AssertNull(decoder.readMessage());
   decoder.feed("]>", 2);
   message = decoder.readMessage(&size);
   AssertNotNull(message);
   AssertEquals(message, "<partial/>");
   MemFree(message);
   EndTest();

   StartTest(_T("EOM framing - byte by byte"));
   const char *frame = "<x>data</x>]]>]]>";
   for(size_t i = 0; i < strlen(frame); i++)
      decoder.feed(&frame[i], 1);
   message = decoder.readMessage();
   AssertNotNull(message);
   AssertEquals(message, "<x>data</x>");
   MemFree(message);
   EndTest();

   StartTest(_T("EOM framing - message size limit"));
   NETCONF_MessageDecoder limitedDecoder(NetconfVersion::NETCONF_1_0, 16);
   char buffer[64];
   memset(buffer, 'A', sizeof(buffer));
   limitedDecoder.feed(buffer, sizeof(buffer));
   AssertTrue(limitedDecoder.isError());
   EndTest();
}

/**
 * Test chunked framing (NETCONF 1.1)
 */
static void TestFramingChunked()
{
   StartTest(_T("Chunked framing - encoding"));
   ByteStream stream;
   NETCONF_EncodeMessage("<rpc/>", 6, NetconfVersion::NETCONF_1_1, stream);
   AssertEquals(stream.size(), 14);
   AssertTrue(!memcmp(stream.buffer(), "\n#6\n<rpc/>\n##\n", 14));
   EndTest();

   StartTest(_T("Chunked framing - single chunk"));
   NETCONF_MessageDecoder decoder(NetconfVersion::NETCONF_1_1);
   decoder.feed(stream.buffer(), stream.size());
   size_t size;
   char *message = decoder.readMessage(&size);
   AssertNotNull(message);
   AssertEquals(size, 6);
   AssertEquals(message, "<rpc/>");
   MemFree(message);
   AssertFalse(decoder.isError());
   EndTest();

   StartTest(_T("Chunked framing - multiple chunks"));
   const char *text = "<rpc-reply message-id=\"5\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"><ok/></rpc-reply>";
   ByteStream chunked;
   EncodeChunked(text, 7, chunked);
   decoder.feed(chunked.buffer(), chunked.size());
   message = decoder.readMessage();
   AssertNotNull(message);
   AssertEquals(message, text);
   MemFree(message);
   EndTest();

   StartTest(_T("Chunked framing - byte by byte"));
   size_t chunkedSize;
   const BYTE *chunkedData = chunked.buffer(&chunkedSize);
   for(size_t i = 0; i < chunkedSize; i++)
      decoder.feed(&chunkedData[i], 1);
   message = decoder.readMessage();
   AssertNotNull(message);
   AssertEquals(message, text);
   MemFree(message);
   AssertFalse(decoder.isError());
   EndTest();

   StartTest(_T("Chunked framing - two messages in single feed"));
   ByteStream twoMessages;
   EncodeChunked("<a/>", 100, twoMessages);
   EncodeChunked("<b/>", 100, twoMessages);
   decoder.feed(twoMessages.buffer(), twoMessages.size());
   message = decoder.readMessage();
   AssertNotNull(message);
   AssertEquals(message, "<a/>");
   MemFree(message);
   message = decoder.readMessage();
   AssertNotNull(message);
   AssertEquals(message, "<b/>");
   MemFree(message);
   EndTest();

   StartTest(_T("Chunked framing - protocol violations"));
   NETCONF_MessageDecoder d1(NetconfVersion::NETCONF_1_1);
   d1.feed("garbage", 7);
   AssertTrue(d1.isError());
   NETCONF_MessageDecoder d2(NetconfVersion::NETCONF_1_1);
   d2.feed("\n#01\nx", 6);   // leading zero in chunk size is not allowed
   AssertTrue(d2.isError());
   NETCONF_MessageDecoder d3(NetconfVersion::NETCONF_1_1);
   d3.feed("\n#3x", 4);
   AssertTrue(d3.isError());
   EndTest();

   StartTest(_T("Chunked framing - message size limit"));
   NETCONF_MessageDecoder limitedDecoder(NetconfVersion::NETCONF_1_1, 16);
   limitedDecoder.feed("\n#32\n", 5);
   AssertTrue(limitedDecoder.isError());
   EndTest();

   StartTest(_T("Framing version switch after hello"));
   NETCONF_MessageDecoder sessionDecoder;   // starts with EOM framing as required for hello exchange
   ByteStream sessionData;
   sessionData.write(s_serverHello, strlen(s_serverHello));
   sessionData.write("]]>]]>", 6);
   EncodeChunked(text, 10, sessionData);   // server may pipeline next message in new framing
   sessionDecoder.feed(sessionData.buffer(), sessionData.size());
   message = sessionDecoder.readMessage();
   AssertNotNull(message);
   NETCONF_CapabilitySet capabilities;
   AssertTrue(capabilities.parseHelloMessage(message, strlen(message), nullptr));
   MemFree(message);
   AssertNull(sessionDecoder.readMessage());
   sessionDecoder.setVersion(capabilities.getProtocolVersion());
   AssertTrue(sessionDecoder.getVersion() == NetconfVersion::NETCONF_1_1);
   message = sessionDecoder.readMessage();
   AssertNotNull(message);
   AssertEquals(message, text);
   MemFree(message);
   AssertFalse(sessionDecoder.isError());
   EndTest();
}

/**
 * Test rpc-reply parsing
 */
static void TestResponseParsing()
{
   StartTest(_T("Response - ok reply"));
   NETCONF_Response response;
   const char *okReply = "<rpc-reply message-id=\"101\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"><ok/></rpc-reply>";
   AssertTrue(response.parse(okReply, strlen(okReply)));
   AssertTrue(response.isValid());
   AssertEquals(response.getMessageId(), 101u);
   AssertTrue(response.isOk());
   AssertTrue(response.isSuccess());
   AssertFalse(response.hasErrors());
   AssertFalse(response.getData());
   EndTest();

   StartTest(_T("Response - data reply"));
   const char *dataReply =
      "<rpc-reply message-id=\"102\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
      "<data>"
      "<system xmlns=\"http://example.com/yang/system\">"
      "<hostname>core-rtr-01</hostname>"
      "<uptime>86400</uptime>"
      "<interfaces><interface><name>eth0</name><oper-status>up</oper-status></interface>"
      "<interface><name>eth1</name><oper-status>down</oper-status></interface></interfaces>"
      "</system>"
      "</data>"
      "</rpc-reply>";
   AssertTrue(response.parse(dataReply, strlen(dataReply)));
   AssertTrue(response.isSuccess());
   AssertFalse(response.isOk());
   AssertTrue(response.getData());
   AssertEquals(NETCONF_LocalName(response.getData()), "data");
   EndTest();

   StartTest(_T("Response - XPath value extraction"));
   AssertTrue(response.getValueByXPath("/rpc-reply/data/system/hostname").equals(_T("core-rtr-01")));
   AssertTrue(response.getValueByXPath("//interface[name=\"eth1\"]/oper-status").equals(_T("down")));
   AssertTrue(response.getValueByXPath("count(//interface)").equals(_T("2")));
   AssertTrue(response.getValueByXPath("/rpc-reply/data/system/nonexistent").isEmpty());
   AssertTrue(response.getValueByXPath("/rpc-reply/data/system/hostname[").isEmpty());   // invalid expression
   EndTest();

   StartTest(_T("Response - error reply"));
   const char *errorReply =
      "<rpc-reply message-id=\"103\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
      "<rpc-error>"
      "<error-type>application</error-type>"
      "<error-tag>invalid-value</error-tag>"
      "<error-severity>error</error-severity>"
      "<error-path>/t:top/t:interface/t:mtu</error-path>"
      "<error-message xml:lang=\"en\">MTU value 25000 is not within range 256..9192</error-message>"
      "</rpc-error>"
      "<rpc-error>"
      "<error-type>protocol</error-type>"
      "<error-tag>operation-failed</error-tag>"
      "<error-severity>error</error-severity>"
      "</rpc-error>"
      "</rpc-reply>";
   AssertTrue(response.parse(errorReply, strlen(errorReply)));
   AssertTrue(response.isValid());
   AssertFalse(response.isSuccess());
   AssertTrue(response.hasErrors());
   AssertEquals(response.getErrors().size(), 2);
   NETCONF_Error *error = response.getErrors().get(0);
   AssertTrue(error->getType().equals(_T("application")));
   AssertTrue(error->getTag().equals(_T("invalid-value")));
   AssertTrue(error->getSeverity().equals(_T("error")));
   AssertTrue(error->getPath().equals(_T("/t:top/t:interface/t:mtu")));
   AssertTrue(error->getMessage().equals(_T("MTU value 25000 is not within range 256..9192")));
   AssertFalse(error->isWarning());
   AssertTrue(response.getErrorText().equals(_T("MTU value 25000 is not within range 256..9192; operation-failed")));
   EndTest();

   StartTest(_T("Response - warnings do not fail request"));
   const char *warningReply =
      "<rpc-reply message-id=\"104\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
      "<rpc-error>"
      "<error-type>application</error-type>"
      "<error-tag>partial-operation</error-tag>"
      "<error-severity>warning</error-severity>"
      "</rpc-error>"
      "<ok/>"
      "</rpc-reply>";
   AssertTrue(response.parse(warningReply, strlen(warningReply)));
   AssertTrue(response.hasErrors());
   AssertTrue(response.isSuccess());
   AssertTrue(response.isOk());
   EndTest();

   StartTest(_T("Response - namespace prefixed reply"));
   const char *prefixedReply =
      "<nc:rpc-reply message-id=\"105\" xmlns:nc=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
      "<nc:data><config>value</config></nc:data>"
      "</nc:rpc-reply>";
   AssertTrue(response.parse(prefixedReply, strlen(prefixedReply)));
   AssertEquals(response.getMessageId(), 105u);
   AssertTrue(response.getData());
   AssertEquals(NETCONF_FindChildByLocalName(response.getData(), "config").child_value(), "value");
   EndTest();

   StartTest(_T("Response - parse errors"));
   NETCONF_Response r1;
   AssertFalse(r1.parse("not an XML document", 18));
   AssertFalse(r1.isValid());
   AssertFalse(r1.isSuccess());
   const char *notReply = "<hello xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"/>";
   AssertFalse(r1.parse(notReply, strlen(notReply)));
   EndTest();
}

/**
 * Test XML helper functions
 */
static void TestXmlHelpers()
{
   StartTest(_T("XML helpers - local names"));
   pugi::xml_document document;
   AssertTrue(document.load_string("<ns1:root xmlns:ns1=\"urn:x\"><ns1:child>text</ns1:child><other/></ns1:root>"));
   pugi::xml_node root = document.first_child();
   AssertEquals(NETCONF_LocalName(root), "root");
   pugi::xml_node child = NETCONF_FindChildByLocalName(root, "child");
   AssertTrue(child);
   AssertEquals(child.child_value(), "text");
   AssertTrue(NETCONF_FindChildByLocalName(root, "other"));
   AssertFalse(NETCONF_FindChildByLocalName(root, "missing"));
   EndTest();

   StartTest(_T("XML helpers - XPath evaluation"));
   AssertTrue(NETCONF_GetValueByXPath(document, "/ns1:root/ns1:child").equals(_T("text")));
   AssertTrue(NETCONF_GetValueByXPath(document, "/ns1:root/missing").isEmpty());
   AssertTrue(NETCONF_GetValueByXPath(document, "!!!").isEmpty());
   EndTest();

   StartTest(_T("XML helpers - datastore names"));
   AssertEquals(NETCONF_DatastoreName(NetconfDatastore::RUNNING), "running");
   AssertEquals(NETCONF_DatastoreName(NetconfDatastore::CANDIDATE), "candidate");
   AssertEquals(NETCONF_DatastoreName(NetconfDatastore::STARTUP), "startup");
   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   TestCapabilitySet();
   TestHelloMessage();
   TestRequestBuilders();
   TestFramingEom();
   TestFramingChunked();
   TestResponseParsing();
   TestXmlHelpers();
   return 0;
}
