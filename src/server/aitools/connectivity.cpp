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
** File: connectivity.cpp
**
**/

#include "aitools.h"
#include <nxsocket.h>

/**
 * Timeout for server-side TCP connect (milliseconds)
 */
#define SERVER_TCP_CONNECT_TIMEOUT  5000

/**
 * Network service check status codes returned by netsvc subagent in VID_SERVICE_STATUS
 */
enum
{
   NETSVC_STATUS_OK = 0,
   NETSVC_STATUS_BAD_PARAMS = 1,
   NETSVC_STATUS_CONNECT = 2,
   NETSVC_STATUS_NOMATCH = 3,
   NETSVC_STATUS_INTERNAL = 4,
   NETSVC_STATUS_HANDSHAKE = 5
};

/**
 * Outcome of single test (ICMP or TCP)
 */
enum class TestOutcome
{
   NOT_PERFORMED,    // test was not requested or could not be started for reasons unrelated to source
   SUCCESS,          // target responded
   NO_RESPONSE,      // ICMP: no reply; TCP: connect timeout
   REJECTED,         // ICMP: destination unreachable; TCP: connection refused or failed
   PROTOCOL_ERROR,   // TCP connected but protocol handshake or response check failed
   CANNOT_TEST       // source cannot perform the test (missing subagent, agent not reachable, local error)
};

/**
 * Supported protocols
 */
struct ProtocolDefinition
{
   const char *name;
   int serviceType;
   uint16_t defaultPort;
};
static const ProtocolDefinition s_protocols[] =
{
   { "tcp", NETSRV_CUSTOM, 0 },
   { "ssh", NETSRV_SSH, 22 },
   { "telnet", NETSRV_TELNET, 23 },
   { "http", NETSRV_HTTP, 80 },
   { "https", NETSRV_HTTPS, 443 },
   { "tls", NETSRV_TLS, 443 },
   { nullptr, 0, 0 }
};

/**
 * Convert address list to JSON array
 */
static json_t *AddressListToJson(const InetAddressList& list)
{
   json_t *array = json_array();
   char buffer[64];
   for(int i = 0; i < list.size(); i++)
      json_array_append_new(array, json_string(list.get(i).toStringA(buffer)));
   return array;
}

/**
 * Describe agent communication failure
 */
static const char *DescribeAgentError(DataCollectionError e)
{
   switch(e)
   {
      case DCE_COMM_ERROR:
         return "NetXMS agent on source node is not reachable";
      case DCE_NOT_SUPPORTED:
         return "metric is not supported by source agent";
      case DCE_ACCESS_DENIED:
         return "source agent denied access to metric";
      default:
         return "source agent returned error";
   }
}

/**
 * Resolve host name on source node. Returns invalid address on failure and sets error message.
 * On return, notResolved is set to true only if source agent answered that the name does not resolve.
 */
static InetAddress ResolveAtSource(Node *source, const wchar_t *hostName, json_t *target, bool *notResolved)
{
   *notResolved = false;
   wchar_t metric[512], value[MAX_RESULT_LENGTH];
   nx_swprintf(metric, 512, L"Net.Resolver.AddressByName(%s)", hostName);
   DataCollectionError e = source->getMetricFromAgent(metric, value, MAX_RESULT_LENGTH);
   if (e != DCE_SUCCESS)
   {
      json_object_set_new(target, "resolvedAtSource", json_null());
      json_object_set_new(target, "sourceResolutionError", json_string((e == DCE_NOT_SUPPORTED) ? "source agent does not support name resolution" : DescribeAgentError(e)));
      return InetAddress::INVALID;
   }

   InetAddress addr = InetAddress::parse(value);
   if (!addr.isValid())
   {
      json_object_set_new(target, "resolvedAtSource", json_null());
      json_object_set_new(target, "sourceResolutionError", json_string("host name does not resolve at source"));
      *notResolved = true;
      return InetAddress::INVALID;
   }

   char buffer[64];
   json_t *array = json_array();
   json_array_append_new(array, json_string(addr.toStringA(buffer)));
   json_object_set_new(target, "resolvedAtSource", array);
   return addr;
}

/**
 * ICMP ping from source node
 */
static TestOutcome PingFromNode(Node *source, const InetAddress& addr, json_t *icmp)
{
   wchar_t addrText[64], metric[128], value[MAX_RESULT_LENGTH];
   nx_swprintf(metric, 128, L"Icmp.Ping(%s,1000,,,3)", addr.toString(addrText));
   DataCollectionError e = source->getMetricFromAgent(metric, value, MAX_RESULT_LENGTH);
   if (e != DCE_SUCCESS)
   {
      json_object_set_new(icmp, "status", json_string("cannot-test"));
      json_object_set_new(icmp, "error", json_string((e == DCE_NOT_SUPPORTED) ? "source cannot test: ping subagent not loaded" :
            ((e == DCE_COLLECTION_ERROR) ? "source agent cannot send ICMP packets" : DescribeAgentError(e))));
      return TestOutcome::CANNOT_TEST;
   }

   wchar_t *eptr;
   long rtt = wcstol(value, &eptr, 10);
   if ((*eptr != 0) || (rtt < 0))
   {
      json_object_set_new(icmp, "status", json_string("cannot-test"));
      json_object_set_new(icmp, "error", json_string("invalid response from source agent"));
      return TestOutcome::CANNOT_TEST;
   }
   if (rtt >= 10000)
   {
      json_object_set_new(icmp, "status", json_string("no-response"));
      return TestOutcome::NO_RESPONSE;
   }
   json_object_set_new(icmp, "status", json_string("success"));
   json_object_set_new(icmp, "rttMs", json_integer(rtt));
   return TestOutcome::SUCCESS;
}

/**
 * ICMP ping from server
 */
static TestOutcome PingFromServer(const InetAddress& addr, json_t *icmp)
{
   uint32_t rtt = 0;
   uint32_t rc = IcmpPing(addr, 3, g_icmpPingTimeout, &rtt, g_icmpPingSize, false);
   switch(rc)
   {
      case ICMP_SUCCESS:
         json_object_set_new(icmp, "status", json_string("success"));
         json_object_set_new(icmp, "rttMs", json_integer(rtt));
         return TestOutcome::SUCCESS;
      case ICMP_TIMEOUT:
         json_object_set_new(icmp, "status", json_string("no-response"));
         return TestOutcome::NO_RESPONSE;
      case ICMP_UNREACHABLE:
         json_object_set_new(icmp, "status", json_string("unreachable"));
         return TestOutcome::REJECTED;
      default:
         json_object_set_new(icmp, "status", json_string("cannot-test"));
         json_object_set_new(icmp, "error", json_string((rc == ICMP_RAW_SOCK_FAILED) ? "server cannot open raw socket for ICMP" : "server cannot send ICMP packets"));
         return TestOutcome::CANNOT_TEST;
   }
}

/**
 * Report source agent error for service check
 */
static TestOutcome ServiceCheckAgentError(uint32_t rcc, json_t *tcp)
{
   json_object_set_new(tcp, "status", json_string("cannot-test"));
   if (rcc == ERR_UNKNOWN_COMMAND)
   {
      json_object_set_new(tcp, "error", json_string("source cannot test: netsvc subagent not loaded"));
   }
   else if ((rcc == ERR_NOT_CONNECTED) || (rcc == ERR_CONNECTION_BROKEN) || (rcc == ERR_REQUEST_TIMEOUT))
   {
      json_object_set_new(tcp, "error", json_string("NetXMS agent on source node is not reachable"));
   }
   else
   {
      char text[256];
      snprintf(text, sizeof(text), "source agent returned error %u (%ls)", rcc, AgentErrorCodeToText(rcc));
      json_object_set_new(tcp, "error", json_string(text));
   }
   return TestOutcome::CANNOT_TEST;
}

/**
 * TCP or protocol check from source node via netsvc subagent
 */
static TestOutcome CheckServiceFromNode(Node *source, const InetAddress& addr, const ProtocolDefinition *protocol, uint16_t port, const char *hostName, json_t *tcp)
{
   bool isHttp = (protocol->serviceType == NETSRV_HTTP) || (protocol->serviceType == NETSRV_HTTPS);
   uint32_t status = 0, responseTime = 0;
   if ((protocol->serviceType == NETSRV_TLS) && (hostName != nullptr))
   {
      // Network service check command passes only IP address, so TLS handshake would use address as SNI and fail
      // on servers selecting certificate by name; metric takes host name, which source agent resolves itself
      wchar_t metric[512], value[MAX_RESULT_LENGTH];
      nx_swprintf(metric, 512, L"NetworkService.TLSStatus(%hs,%u)", hostName, static_cast<uint32_t>(port));
      int64_t startTime = GetMonotonicClockTime();
      DataCollectionError e = source->getMetricFromAgent(metric, value, MAX_RESULT_LENGTH);
      responseTime = static_cast<uint32_t>(GetMonotonicClockTime() - startTime);
      if (e != DCE_SUCCESS)
      {
         json_object_set_new(tcp, "status", json_string("cannot-test"));
         json_object_set_new(tcp, "error", json_string((e == DCE_NOT_SUPPORTED) ? "source cannot test: netsvc subagent not loaded" : DescribeAgentError(e)));
         return TestOutcome::CANNOT_TEST;
      }
      status = wcstoul(value, nullptr, 10);
   }
   else
   {
      wchar_t request[1024] = L"";
      wchar_t response[16] = L"";
      if (isHttp)
      {
         // netsvc expects "host:uri" in request and regular expression for response body in response
         if (hostName != nullptr)
         {
            nx_swprintf(request, 1024, L"%hs:/", hostName);
         }
         else
         {
            wchar_t addrText[64];
            nx_swprintf(request, 1024, L"%s:/", addr.toString(addrText));
         }
      }

      uint32_t rcc = source->checkNetworkService(&status, addr, protocol->serviceType, port, 0, request, response, &responseTime);
      if (rcc != ERR_SUCCESS)
         return ServiceCheckAgentError(rcc, tcp);
   }

   json_object_set_new(tcp, "responseTimeMs", json_integer(responseTime));
   switch(status)
   {
      case NETSVC_STATUS_OK:
         json_object_set_new(tcp, "status", json_string((protocol->serviceType == NETSRV_CUSTOM) ? "open" : "ok"));
         return TestOutcome::SUCCESS;
      case NETSVC_STATUS_CONNECT:
         json_object_set_new(tcp, "status", json_string("connect-failed"));
         json_object_set_new(tcp, "error", json_string(isHttp ? "cannot resolve host name or connect to server" : "connection refused or timed out"));
         return TestOutcome::REJECTED;
      case NETSVC_STATUS_HANDSHAKE:
         if (isHttp)
         {
            // netsvc reports every request error other than resolve/connect failure this way
            json_object_set_new(tcp, "status", json_string("request-failed"));
            json_object_set_new(tcp, "error", json_string("HTTP request failed: timeout, TLS or certificate error, or invalid HTTP response (source agent netsvc timeout applies to the whole request)"));
         }
         else
         {
            json_object_set_new(tcp, "status", json_string("handshake-failed"));
            json_object_set_new(tcp, "error", json_string((protocol->serviceType == NETSRV_TLS) ?
                  "TCP connection established but TLS handshake failed" : "TCP connection established but protocol handshake failed"));
         }
         return TestOutcome::PROTOCOL_ERROR;
      case NETSVC_STATUS_NOMATCH:
         json_object_set_new(tcp, "status", json_string("unexpected-response"));
         json_object_set_new(tcp, "error", json_string("connection established but service response is not valid for the protocol (or empty)"));
         return TestOutcome::PROTOCOL_ERROR;
      default:
         json_object_set_new(tcp, "status", json_string("cannot-test"));
         json_object_set_new(tcp, "error", json_string("source agent cannot perform check (internal error or invalid parameters)"));
         return TestOutcome::CANNOT_TEST;
   }
}

/**
 * TCP connect from server
 */
static TestOutcome ConnectFromServer(const InetAddress& addr, uint16_t port, json_t *tcp)
{
   SOCKET s = CreateSocket(addr.getFamily(), SOCK_STREAM, 0);
   if (s == INVALID_SOCKET)
   {
      json_object_set_new(tcp, "status", json_string("cannot-test"));
      json_object_set_new(tcp, "error", json_string("server cannot create socket"));
      return TestOutcome::CANNOT_TEST;
   }

   SockAddrBuffer saBuffer;
   struct sockaddr *sa = addr.fillSockAddr(&saBuffer, port);
   bool isTimeout;
   int64_t startTime = GetMonotonicClockTime();
   int rc = ConnectEx(s, sa, SA_LEN(sa), SERVER_TCP_CONNECT_TIMEOUT, &isTimeout);
   uint32_t elapsed = static_cast<uint32_t>(GetMonotonicClockTime() - startTime);
   closesocket(s);

   json_object_set_new(tcp, "responseTimeMs", json_integer(elapsed));
   if (rc == 0)
   {
      json_object_set_new(tcp, "status", json_string("open"));
      return TestOutcome::SUCCESS;
   }
   if (isTimeout)
   {
      json_object_set_new(tcp, "status", json_string("timeout"));
      json_object_set_new(tcp, "error", json_string("no response to connection request (filtered or host down)"));
      return TestOutcome::NO_RESPONSE;
   }
   json_object_set_new(tcp, "status", json_string("connect-failed"));
   json_object_set_new(tcp, "error", json_string("connection refused or network unreachable"));
   return TestOutcome::REJECTED;
}

/**
 * Check connectivity from a node or from the server to given target
 */
std::string F_CheckConnectivity(json_t *arguments, uint32_t userId)
{
   // Parse protocol and port first so that argument errors do not cause any network activity
   const char *protocolName = json_object_get_string_utf8(arguments, "protocol", nullptr);
   int32_t portArg = json_object_get_int32(arguments, "port", 0);
   if ((portArg < 0) || (portArg > 65535))
      return std::string("Invalid port number");

   const ProtocolDefinition *protocol = nullptr;
   if ((protocolName != nullptr) && (*protocolName != 0))
   {
      for(int i = 0; s_protocols[i].name != nullptr; i++)
      {
         if (!stricmp(protocolName, s_protocols[i].name))
         {
            protocol = &s_protocols[i];
            break;
         }
      }
      if (protocol == nullptr)
         return std::string("Unsupported protocol (supported protocols are tcp, ssh, telnet, http, https and tls)");
   }
   else if (portArg != 0)
   {
      protocol = &s_protocols[0];
   }

   uint16_t port = static_cast<uint16_t>(portArg);
   if ((protocol != nullptr) && (port == 0))
   {
      port = protocol->defaultPort;
      if (port == 0)
         return std::string("Port number is required for protocol tcp");
   }

   // Source
   shared_ptr<NetObj> sourceObject;
   Node *source = nullptr;
   json_t *from = json_object_get(arguments, "from");
   if ((from != nullptr) && !json_is_null(from) && !(json_is_string(from) && (*json_string_value(from) == 0)))
   {
      sourceObject = FindObjectByNameOrId(arguments, "from", OBJECT_NODE);
      if ((sourceObject == nullptr) || (sourceObject->getObjectClass() != OBJECT_NODE))
         return std::string("Source node not found");
      if (!sourceObject->checkAccessRights(userId, OBJECT_ACCESS_READ_AGENT))
         return std::string("Access denied to source node");
      source = static_cast<Node*>(sourceObject.get());
   }
   else
   {
      // Server is represented by management node object; probing from server requires the same right as probing from any other node
      shared_ptr<NetObj> mgmtNode = FindObjectById(g_dwMgmtNode, OBJECT_NODE);
      if ((mgmtNode != nullptr) && !mgmtNode->checkAccessRights(userId, OBJECT_ACCESS_READ_AGENT))
         return std::string("Access denied: checks from server require agent read access to management server node");
   }

   // Target: object ID, IP address, exact object name or host name
   json_t *targetArg = json_object_get(arguments, "target");
   if (targetArg == nullptr)
      return std::string("Target must be specified");

   shared_ptr<NetObj> targetObject;
   InetAddress targetAddress;
   char inputText[256];
   if (json_is_integer(targetArg))
   {
      snprintf(inputText, sizeof(inputText), INT64_FMTA, static_cast<int64_t>(json_integer_value(targetArg)));
      targetObject = FindObjectById(static_cast<uint32_t>(json_integer_value(targetArg)));
      if (targetObject == nullptr)
         return std::string("Target object not found");
   }
   else if (json_is_string(targetArg) && (*json_string_value(targetArg) != 0))
   {
      strlcpy(inputText, json_string_value(targetArg), sizeof(inputText));
      TrimA(inputText);
      char *eptr;
      uint32_t id = strtoul(inputText, &eptr, 10);
      if ((*eptr == 0) && (id != 0))
      {
         targetObject = FindObjectById(id);
      }
      if (targetObject == nullptr)
      {
         targetAddress = InetAddress::parse(inputText);
         if (!targetAddress.isValid())
         {
            wchar_t name[MAX_OBJECT_NAME];
            utf8_to_wchar(inputText, -1, name, MAX_OBJECT_NAME);
            name[MAX_OBJECT_NAME - 1] = 0;
            targetObject = FindObjectByName(name);
         }
      }
   }
   else
   {
      return std::string("Target must be specified");
   }

   json_t *output = json_object();

   json_t *sourceInfo = json_object();
   if (source != nullptr)
   {
      json_object_set_new(sourceInfo, "type", json_string("node"));
      json_object_set_new(sourceInfo, "nodeId", json_integer(source->getId()));
      json_object_set_new(sourceInfo, "nodeName", json_string_w(source->getName()));
   }
   else
   {
      json_object_set_new(sourceInfo, "type", json_string("server"));
   }
   json_object_set_new(output, "source", sourceInfo);

   json_t *target = json_object();
   json_object_set_new(output, "target", target);
   json_object_set_new(target, "input", json_string(inputText));

   // Host name to be resolved; for object targets name resolution is informational only, tests use object's primary IP address
   char hostName[256] = "";
   if (targetObject != nullptr)
   {
      if (!targetObject->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         json_decref(output);
         return std::string("Access denied to target object");
      }
      targetAddress = targetObject->getPrimaryIpAddress();
      if ((targetObject->getObjectClass() == OBJECT_SUBNET) || !targetAddress.isValidUnicast())
      {
         json_decref(output);
         return std::string("Target object does not have usable IP address");
      }
      json_object_set_new(target, "objectId", json_integer(targetObject->getId()));
      json_object_set_new(target, "objectName", json_string_w(targetObject->getName()));
      json_object_set_new(target, "objectClass", json_string(targetObject->getObjectClassNameA()));
      if (targetObject->getObjectClass() == OBJECT_NODE)
      {
         SharedString primaryName = static_cast<Node*>(targetObject.get())->getPrimaryHostName();
         if (!InetAddress::parse(primaryName.cstr()).isValid())
         {
            wchar_to_utf8(primaryName.cstr(), -1, hostName, sizeof(hostName) - 1);
            hostName[sizeof(hostName) - 1] = 0;
         }
      }
   }
   else if (!targetAddress.isValid())
   {
      strlcpy(hostName, inputText, sizeof(hostName));
   }

   json_t *notes = json_array();
   bool dnsFailure = false;
   if (hostName[0] != 0)
   {
      json_object_set_new(target, "hostName", json_string(hostName));
      InetAddressList *serverList = InetAddressList::resolveHostName(hostName);
      if ((serverList != nullptr) && (serverList->size() > 0))
      {
         json_object_set_new(target, "resolvedAtServer", AddressListToJson(*serverList));
         if (!targetAddress.isValid())
            targetAddress = serverList->get(0);
      }
      else
      {
         json_object_set_new(target, "resolvedAtServer", json_null());
         if (targetObject == nullptr)
            dnsFailure = true;
      }
      if (source != nullptr)
      {
         wchar_t hostNameW[256];
         utf8_to_wchar(hostName, -1, hostNameW, 256);
         hostNameW[255] = 0;
         bool notResolved;
         InetAddress sourceAddress = ResolveAtSource(source, hostNameW, target, &notResolved);
         if (targetObject == nullptr)
         {
            // Source resolution is authoritative for name targets: this is the address applications on source would connect to
            if (sourceAddress.isValid())
            {
               if ((serverList != nullptr) && (serverList->size() > 0) && !serverList->hasAddress(sourceAddress))
                  json_array_append_new(notes, json_string("host name resolves to different addresses at server and at source; address resolved at source is tested"));
               targetAddress = sourceAddress;
               dnsFailure = false;
            }
            else if (notResolved)
            {
               dnsFailure = true;
               if (targetAddress.isValid())
                  json_array_append_new(notes, json_string("host name does not resolve at source; address resolved at server is tested instead"));
            }
            else if (targetAddress.isValid())
            {
               json_array_append_new(notes, json_string("name resolution at source could not be checked; address resolved at server is tested"));
            }
         }
      }
      delete serverList;
   }

   char addrText[64];
   json_object_set_new(target, "testedAddress", targetAddress.isValid() ? json_string(targetAddress.toStringA(addrText)) : json_null());

   if ((source == nullptr) && (targetObject != nullptr) && (targetObject->getZoneUIN() != 0))
      json_array_append_new(notes, json_string("target is in a non-default zone; server may not have direct route to it - consider running check from a node in that zone"));

   // Tests
   TestOutcome icmpOutcome = TestOutcome::NOT_PERFORMED;
   TestOutcome tcpOutcome = TestOutcome::NOT_PERFORMED;
   json_t *icmp = json_object();
   json_t *tcp = (protocol != nullptr) ? json_object() : json_null();
   if (protocol != nullptr)
   {
      json_object_set_new(tcp, "port", json_integer(port));
      json_object_set_new(tcp, "protocol", json_string(protocol->name));
   }

   if (targetAddress.isValid())
   {
      icmpOutcome = (source != nullptr) ? PingFromNode(source, targetAddress, icmp) : PingFromServer(targetAddress, icmp);

      if (protocol != nullptr)
      {
         if (source != nullptr)
         {
            if (((protocol->serviceType == NETSRV_HTTP) || (protocol->serviceType == NETSRV_HTTPS)) && (hostName[0] == 0) && (targetAddress.getFamily() == AF_INET6))
            {
               json_object_set_new(tcp, "status", json_string("not-performed"));
               json_object_set_new(tcp, "error", json_string("HTTP check of IPv6 address requires target host name; use protocol tcp"));
            }
            else
            {
               tcpOutcome = CheckServiceFromNode(source, targetAddress, protocol, port, (hostName[0] != 0) ? hostName : nullptr, tcp);
            }
         }
         else
         {
            tcpOutcome = ConnectFromServer(targetAddress, port, tcp);
            if (protocol->serviceType != NETSRV_CUSTOM)
               json_array_append_new(notes, json_string("protocol checks are performed only from nodes with NetXMS agent; server performed TCP connect only"));
         }
      }
   }
   else
   {
      json_object_set_new(icmp, "status", json_string("not-performed"));
      json_object_set_new(icmp, "error", json_string("target address is unknown"));
      if (protocol != nullptr)
      {
         json_object_set_new(tcp, "status", json_string("not-performed"));
         json_object_set_new(tcp, "error", json_string("target address is unknown"));
      }
   }
   json_object_set_new(output, "icmp", icmp);
   json_object_set_new(output, "tcp", tcp);

   // Verdict
   const char *verdict;
   if (dnsFailure)
   {
      verdict = "dns-failure";
   }
   else if (protocol != nullptr)
   {
      switch(tcpOutcome)
      {
         case TestOutcome::SUCCESS:
            verdict = "reachable";
            break;
         case TestOutcome::PROTOCOL_ERROR:
            verdict = "protocol-error";
            break;
         case TestOutcome::NO_RESPONSE:
         case TestOutcome::REJECTED:
            verdict = (icmpOutcome == TestOutcome::SUCCESS) ? "port-closed" : "host-unreachable";
            break;
         default:
            verdict = "source-cannot-test";
            break;
      }
   }
   else
   {
      switch(icmpOutcome)
      {
         case TestOutcome::SUCCESS:
            verdict = "reachable";
            break;
         case TestOutcome::NO_RESPONSE:
         case TestOutcome::REJECTED:
            verdict = "host-unreachable";
            break;
         default:
            verdict = "source-cannot-test";
            break;
      }
   }
   json_object_set_new(output, "verdict", json_string(verdict));

   if (json_array_size(notes) > 0)
      json_object_set_new(output, "notes", notes);
   else
      json_decref(notes);

   json_object_set_new(output, "timestamp", json_time_string(time(nullptr)));
   return JsonToString(output);
}
