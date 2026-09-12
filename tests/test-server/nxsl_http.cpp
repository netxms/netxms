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
** File: nxsl_http.cpp
**
** Tests for NXSL HTTP client classes (HttpRequest, HttpSession, HttpResponse), executed
** inside the test server launcher. Requests are sent to an embedded libmicrohttpd server
** listening on loopback, so the tests do not depend on network access.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <nms_users.h>
#include <microhttpd.h>
#include <testtools.h>

#define TEST_HTTP_PORT     14780
#define TEST_BASE_URL      "http://127.0.0.1:14780"

#define AUTH_REALM         "test"

/**
 * Text returned by /text endpoint (contains non-ASCII characters to check UTF-8 handling)
 */
static const char s_textBody[] = "Hello, world! Привет";

/**
 * Embedded HTTP server
 */
static MHD_Daemon *s_daemon = nullptr;

/**
 * Seed for digest authentication nonces (fixed value is sufficient for tests)
 */
static const char s_digestAuthRandom[] = "netxms-test-server-digest-auth-seed";

/**
 * Per-request context (accumulates request body)
 */
struct RequestContext
{
   ByteStream body;
};

/**
 * Create response with given body and content type
 */
static MHD_Response *CreateResponse(const void *body, size_t size, const char *contentType)
{
   MHD_Response *response = MHD_create_response_from_buffer(size, const_cast<void*>(body), MHD_RESPMEM_MUST_COPY);
   if (contentType != nullptr)
      MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, contentType);
   return response;
}

/**
 * Queue response and destroy it
 */
static MHD_Result QueueResponse(MHD_Connection *connection, unsigned int status, MHD_Response *response)
{
   MHD_Result rc = MHD_queue_response(connection, status, response);
   MHD_destroy_response(response);
   return rc;
}

/**
 * Send simple text response
 */
static MHD_Result SendText(MHD_Connection *connection, unsigned int status, const char *text, const char *contentType = "text/plain")
{
   return QueueResponse(connection, status, CreateResponse(text, strlen(text), contentType));
}

/**
 * Request header iterator for /echo endpoint (header names are stored in lower case)
 */
static MHD_Result HeaderIterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
   char name[256];
   strlcpy(name, key, sizeof(name));
   strlwr(name);
   json_object_set_new(static_cast<json_t*>(cls), name, json_string((value != nullptr) ? value : ""));
   return MHD_YES;
}

/**
 * Query argument iterator for /echo endpoint
 */
static MHD_Result ArgumentIterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
   json_object_set_new(static_cast<json_t*>(cls), key, json_string((value != nullptr) ? value : ""));
   return MHD_YES;
}

/**
 * /echo endpoint: JSON document describing received request. Also sends duplicate response header X-Dup.
 */
static MHD_Result HandleEcho(MHD_Connection *connection, const char *method, RequestContext *context)
{
   json_t *headers = json_object();
   MHD_get_connection_values(connection, MHD_HEADER_KIND, HeaderIterator, headers);

   json_t *args = json_object();
   MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, ArgumentIterator, args);

   json_t *document = json_object();
   json_object_set_new(document, "method", json_string(method));
   json_object_set_new(document, "headers", headers);
   json_object_set_new(document, "args", args);
   json_object_set_new(document, "body", json_stringn(reinterpret_cast<const char*>(context->body.buffer()), context->body.size()));
   json_object_set_new(document, "bodySize", json_integer(static_cast<json_int_t>(context->body.size())));

   char *text = json_dumps(document, JSON_COMPACT);
   json_decref(document);

   MHD_Response *response = CreateResponse(text, strlen(text), "application/json");
   MemFree(text);
   MHD_add_response_header(response, "X-Dup", "first");
   MHD_add_response_header(response, "X-Dup", "second");
   return QueueResponse(connection, MHD_HTTP_OK, response);
}

/**
 * /basic endpoint: basic authentication, returns user name on success
 */
static MHD_Result HandleBasicAuth(MHD_Connection *connection)
{
   MHD_BasicAuthInfo *auth = MHD_basic_auth_get_username_password3(connection);
   bool valid = (auth != nullptr) && !strcmp(auth->username, "user") && !strcmp(auth->password, "pass");
   if (valid)
   {
      MHD_Result rc = SendText(connection, MHD_HTTP_OK, auth->username);
      MHD_free(auth);
      return rc;
   }
   if (auth != nullptr)
      MHD_free(auth);
   MHD_Response *response = CreateResponse("unauthorized", 12, "text/plain");
   MHD_Result rc = MHD_queue_basic_auth_fail_response(connection, AUTH_REALM, response);
   MHD_destroy_response(response);
   return rc;
}

/**
 * /digest endpoint: digest authentication, returns user name on success
 */
static MHD_Result HandleDigestAuth(MHD_Connection *connection)
{
   MHD_DigestAuthResult result = MHD_digest_auth_check3(connection, AUTH_REALM, "duser", "dpass", 300, 0,
      MHD_DIGEST_AUTH_MULT_QOP_AUTH, MHD_DIGEST_AUTH_MULT_ALGO3_MD5);
   if (result == MHD_DAUTH_OK)
      return SendText(connection, MHD_HTTP_OK, "duser");

   MHD_Response *response = CreateResponse("unauthorized", 12, "text/plain");
   MHD_Result rc = MHD_queue_auth_required_response3(connection, AUTH_REALM, nullptr, nullptr, response,
      (result == MHD_DAUTH_NONCE_STALE) ? MHD_YES : MHD_NO, MHD_DIGEST_AUTH_MULT_QOP_AUTH, MHD_DIGEST_AUTH_MULT_ALGO3_MD5, MHD_NO, MHD_NO);
   MHD_destroy_response(response);
   return rc;
}

/**
 * /bearer endpoint: expects "Authorization: Bearer secret-token"
 */
static MHD_Result HandleBearerAuth(MHD_Connection *connection)
{
   const char *authorization = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);
   if ((authorization != nullptr) && !strcmp(authorization, "Bearer secret-token"))
      return SendText(connection, MHD_HTTP_OK, "bearer ok");

   MHD_Response *response = CreateResponse("unauthorized", 12, "text/plain");
   MHD_add_response_header(response, MHD_HTTP_HEADER_WWW_AUTHENTICATE, "Bearer realm=\"" AUTH_REALM "\"");
   return QueueResponse(connection, MHD_HTTP_UNAUTHORIZED, response);
}

/**
 * /cookie endpoint: sets two cookies and returns received Cookie header (or "(none)")
 */
static MHD_Result HandleCookie(MHD_Connection *connection)
{
   const char *cookie = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_COOKIE);
   if (cookie == nullptr)
      cookie = "(none)";
   MHD_Response *response = CreateResponse(cookie, strlen(cookie), "text/plain");
   MHD_add_response_header(response, MHD_HTTP_HEADER_SET_COOKIE, "session=abc123; Path=/");
   MHD_add_response_header(response, MHD_HTTP_HEADER_SET_COOKIE, "other=xyz; Path=/; HttpOnly");
   return QueueResponse(connection, MHD_HTTP_OK, response);
}

/**
 * Request dispatcher
 */
static MHD_Result ConnectionHandler(void *serverContext, MHD_Connection *connection, const char *url, const char *method,
   const char *version, const char *uploadData, size_t *uploadDataSize, void **connectionContext)
{
   if (*connectionContext == nullptr)
   {
      *connectionContext = new RequestContext();
      return MHD_YES;
   }

   auto context = static_cast<RequestContext*>(*connectionContext);
   if (*uploadDataSize > 0)
   {
      context->body.write(uploadData, *uploadDataSize);
      *uploadDataSize = 0;
      return MHD_YES;
   }

   if (!strcmp(url, "/text"))
      return SendText(connection, MHD_HTTP_OK, s_textBody, "text/plain; charset=utf-8");

   if (!strcmp(url, "/json"))
      return SendText(connection, MHD_HTTP_OK, "{\"name\":\"test\",\"values\":[1,2,3],\"nested\":{\"flag\":true}}", "application/json");

   if (!strcmp(url, "/array"))
      return SendText(connection, MHD_HTTP_OK, "[10,20,30]", "application/json");

   if (!strcmp(url, "/notjson"))
      return SendText(connection, MHD_HTTP_OK, "this is not JSON", "application/json");

   if (!strcmp(url, "/empty"))
      return QueueResponse(connection, MHD_HTTP_NO_CONTENT, CreateResponse("", 0, nullptr));

   if (!strcmp(url, "/echo"))
      return HandleEcho(connection, method, context);

   if (!strcmp(url, "/method"))
      return SendText(connection, MHD_HTTP_OK, method);

   if (!strcmp(url, "/large"))
   {
      const char *sizeArg = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "size");
      size_t size = (sizeArg != nullptr) ? strtoul(sizeArg, nullptr, 10) : 1024;
      char *body = MemAllocArrayNoInit<char>(size);
      memset(body, 'x', size);
      MHD_Response *response = CreateResponse(body, size, "application/octet-stream");
      MemFree(body);
      return QueueResponse(connection, MHD_HTTP_OK, response);
   }

   if (!strcmp(url, "/status"))
   {
      const char *codeArg = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "code");
      unsigned int code = (codeArg != nullptr) ? strtoul(codeArg, nullptr, 10) : 200;
      return SendText(connection, code, "status body");
   }

   if (!strcmp(url, "/redirect"))
   {
      MHD_Response *response = CreateResponse("redirecting", 11, "text/plain");
      MHD_add_response_header(response, MHD_HTTP_HEADER_LOCATION, TEST_BASE_URL "/text");
      MHD_add_response_header(response, "X-Redirect", "yes");
      return QueueResponse(connection, MHD_HTTP_FOUND, response);
   }

   if (!strcmp(url, "/redirect-loop"))
   {
      MHD_Response *response = CreateResponse("redirecting", 11, "text/plain");
      MHD_add_response_header(response, MHD_HTTP_HEADER_LOCATION, TEST_BASE_URL "/redirect-loop");
      return QueueResponse(connection, MHD_HTTP_FOUND, response);
   }

   if (!strcmp(url, "/redirect-file"))
   {
      MHD_Response *response = CreateResponse("redirecting", 11, "text/plain");
      MHD_add_response_header(response, MHD_HTTP_HEADER_LOCATION, "file:///etc/hosts");
      return QueueResponse(connection, MHD_HTTP_FOUND, response);
   }

   if (!strcmp(url, "/slow"))
   {
      ThreadSleepMs(2000);
      return SendText(connection, MHD_HTTP_OK, "slow response");
   }

   if (!strcmp(url, "/basic"))
      return HandleBasicAuth(connection);

   if (!strcmp(url, "/digest"))
      return HandleDigestAuth(connection);

   if (!strcmp(url, "/bearer"))
      return HandleBearerAuth(connection);

   if (!strcmp(url, "/cookie"))
      return HandleCookie(connection);

   return SendText(connection, MHD_HTTP_NOT_FOUND, "not found");
}

/**
 * Request completion handler
 */
static void RequestCompleted(void *serverContext, MHD_Connection *connection, void **connectionContext, MHD_RequestTerminationCode code)
{
   delete static_cast<RequestContext*>(*connectionContext);
   *connectionContext = nullptr;
}

/**
 * Start embedded HTTP server
 */
static void StartHttpServer()
{
   struct sockaddr_in sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_port = htons(TEST_HTTP_PORT);
   sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

   s_daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_THREAD_PER_CONNECTION,
      TEST_HTTP_PORT, nullptr, nullptr, ConnectionHandler, nullptr,
      MHD_OPTION_NOTIFY_COMPLETED, RequestCompleted, nullptr,
      MHD_OPTION_NONCE_NC_SIZE, static_cast<unsigned int>(300),
      MHD_OPTION_DIGEST_AUTH_RANDOM_COPY, sizeof(s_digestAuthRandom), s_digestAuthRandom,
      MHD_OPTION_SOCK_ADDR, reinterpret_cast<struct sockaddr*>(&sa),
      MHD_OPTION_END);
   AssertNotNullEx(s_daemon, _T("Cannot start embedded HTTP server"));
}

/**
 * Stop embedded HTTP server
 */
static void StopHttpServer()
{
   if (s_daemon != nullptr)
   {
      MHD_stop_daemon(s_daemon);
      s_daemon = nullptr;
   }
}

/**
 * Compile and run script in server environment. Script gets global variable BASE with base URL of the embedded server.
 * Script errors and failed assertions fail the test. Security context (if given) is owned by the VM.
 */
static void RunScript(const wchar_t *source, NXSL_SecurityContext *securityContext = nullptr)
{
   StringBuffer script(L"global BASE = \"" TEST_BASE_URL L"\";\n");
   script.append(source);

   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(script.cstr(), new NXSL_ServerEnv(), &diag);
   if (vm == nullptr)
      WriteToTerminalEx(_T("\n   Compilation error: %s\n"), diag.errorText.cstr());
   AssertNotNull(vm);

   if (securityContext != nullptr)
      vm->setSecurityContext(securityContext);

   bool success = vm->run();
   if (!success)
      WriteToTerminalEx(_T("\n   Script error: %s\n   Assertion message: %s\n"), vm->getErrorText(), vm->getAssertMessage());
   AssertTrue(success);
   delete vm;
}

/**
 * HttpRequest object: constructor, attributes, header management
 */
static void TestRequestObject()
{
   StartTest(_T("NXSL HTTP: HttpRequest object"));
   RunScript(LR"NXSL(
      r = new HttpRequest(BASE .. "/text");
      assert(r != null, "constructor with URL");
      assert(r.method == "GET", "default method");
      assert(r.url == BASE .. "/text", "url attribute");
      assert(r.body == null, "default body");
      assert(r.timeout == null, "default timeout");
      assert(r.verifyPeer == null, "default verifyPeer");
      assert(r.verifyHost == null, "default verifyHost");
      assert(r.followRedirects == null, "default followRedirects");
      assert(r.maxResponseSize == null, "default maxResponseSize");
      assert(r.userAgent == null, "default userAgent");
      assert(r.proxy == null, "default proxy");
      assert(r.headers.size == 0, "default headers");

      r2 = new HttpRequest("post", BASE .. "/echo");
      assert(r2.method == "POST", "method normalized to upper case");
      r2.method = "put";
      assert(r2.method == "PUT", "method attribute assignment");
      r2.url = BASE .. "/method";
      assert(r2.url == BASE .. "/method", "url attribute assignment");

      r2.timeout = 5;
      assert(r2.timeout == 5, "timeout assignment");
      r2.timeout = null;
      assert(r2.timeout == null, "timeout reset");
      r2.verifyPeer = false;
      assert(r2.verifyPeer == false, "verifyPeer assignment");
      r2.verifyHost = true;
      assert(r2.verifyHost == true, "verifyHost assignment");
      r2.followRedirects = false;
      assert(r2.followRedirects == false, "followRedirects assignment");
      r2.maxResponseSize = 4096;
      assert(r2.maxResponseSize == 4096, "maxResponseSize assignment");
      r2.userAgent = "Agent/1.0";
      assert(r2.userAgent == "Agent/1.0", "userAgent assignment");
      r2.userAgent = null;
      assert(r2.userAgent == null, "userAgent reset");
      r2.proxy = "http://proxy:3128";
      assert(r2.proxy == "http://proxy:3128", "proxy assignment");
      r2.proxy = null;
      assert(r2.proxy == null, "proxy reset");

      r2.headers = { "X-Test": "1", "Bad Header": "x", "X-Null": null };
      assert(r2.headers.size == 1, "invalid and null headers dropped from headers map");
      assert(r2.headers["X-Test"] == "1", "valid header kept");
      assert(r2.setHeader("X-Other", "2") == true, "setHeader valid");
      assert(r2.headers.size == 2, "header added");
      assert(r2.setHeader("X-Inject", "a\r\nInjected: 1") == false, "setHeader rejects CRLF in value");
      assert(r2.setHeader("X Space", "1") == false, "setHeader rejects space in name");
      assert(r2.setHeader("", "1") == false, "setHeader rejects empty name");
      assert(r2.headers.size == 2, "rejected headers not stored");
      assert(r2.setHeader("X-Test", null) == true, "setHeader with null removes header");
      assert(r2.headers.size == 1, "header removed");
      r2.removeHeader("X-Other");
      assert(r2.headers.size == 0, "removeHeader");
      r2.headers = "not a map";
      assert(r2.headers.size == 0, "headers assignment with non-map clears headers");

      r3 = new HttpRequest("FOO", BASE);
      assert(r3 == null, "unsupported method");
      r4 = new HttpRequest("options", BASE);
      assert(r4.method == "OPTIONS", "OPTIONS method");

      assert(r.setAuth("weird") == false, "setAuth with unknown type");
      assert(r.setAuth("basic", "u", "p") == true, "setAuth basic");
      assert(r.setAuth("none") == true, "setAuth none");
   )NXSL");
   EndTest();
}

/**
 * Simple GET request and response object attributes
 */
static void TestSimpleGet()
{
   StartTest(_T("NXSL HTTP: GET request and response attributes"));
   RunScript(LR"NXSL(
      r = new HttpRequest(BASE .. "/text");
      resp = r.execute();
      assert(resp != null, "response object");
      assert(resp.success == true, "success");
      assert(resp.statusCode == 200, "status code");
      assert(resp.errorMessage == null, "no error message");
      assert(resp.body == "Hello, world! Привет", "body (UTF-8)");
      assert(resp.bodySize == 26, "body size in bytes");
      assert(resp.contentType == "text/plain; charset=utf-8", "contentType");
      assert(resp.getHeader("Content-Type") == "text/plain; charset=utf-8", "getHeader is case-insensitive");
      assert(resp.getHeader("content-type") == "text/plain; charset=utf-8", "getHeader lower case");
      assert(resp.getHeader("X-Missing") == null, "getHeader for missing header");
      assert(resp.headers["content-type"] == "text/plain; charset=utf-8", "headers map uses lower case names");
      assert(resp.url == BASE .. "/text", "effective URL");
      assert(resp.responseTime >= 0, "response time");
      assert(resp.json == null, "non-JSON body is not parsed");
      assert(resp.body == "Hello, world! Привет", "body can be read repeatedly");

      // Second execution of the same request object
      resp2 = r.execute();
      assert(resp2.success, "request object can be executed again");
      assert(resp2.body == resp.body, "same body on second execution");
   )NXSL");
   EndTest();
}

/**
 * Request details as seen by server: method, headers, body, query arguments, user agent
 */
static void TestEcho()
{
   StartTest(_T("NXSL HTTP: request headers, body and user agent"));
   RunScript(LR"NXSL(
      r = new HttpRequest("POST", BASE .. "/echo?a=1&b=two");
      r.body = "payload Привет";
      r.setHeader("X-Custom", "value");
      r.setHeader("Content-Type", "text/plain; charset=utf-8");
      resp = r.execute();
      assert(resp.success, "POST success");
      j = resp.json;
      assert(j != null, "JSON response parsed");
      assert(j.method == "POST", "method");
      assert(j.body == "payload Привет", "body");
      assert(j.bodySize == 20, "body size in bytes");
      assert(j.headers.get("x-custom") == "value", "custom header");
      assert(j.headers.get("content-type") == "text/plain; charset=utf-8", "content type header");
      assert(j.headers.get("user-agent") like "NetXMS Server/*", "default user agent");
      assert(j.args.get("a") == "1", "query argument a");
      assert(j.args.get("b") == "two", "query argument b");
      assert(resp.getHeader("X-Dup") == "first, second", "duplicate response headers joined");
      j2 = resp.json;
      assert(j2.method == "POST", "parsed JSON accessible repeatedly");

      r.userAgent = "TestAgent/1.0";
      r.body = null;
      resp = r.execute();
      j = resp.json;
      assert(j.headers.get("user-agent") == "TestAgent/1.0", "custom user agent");
      assert(j.body == "", "empty POST body");
      assert(j.headers.get("content-length") == "0", "content-length for empty POST");

      // Numeric body converted to string
      r.body = 42;
      assert(r.body == "42", "numeric body converted to string");
      resp = r.execute();
      assert(resp.json.body == "42", "numeric body sent as text");

      // GET never sends body
      g = new HttpRequest("GET", BASE .. "/echo");
      g.body = "ignored";
      resp = g.execute();
      assert(resp.json.method == "GET", "GET method");
      assert(resp.json.body == "", "GET body ignored");
   )NXSL");
   EndTest();
}

/**
 * Serialization of JSON, hash map and array bodies
 */
static void TestJsonBody()
{
   StartTest(_T("NXSL HTTP: JSON request bodies"));
   RunScript(LR"NXSL(
      r = new HttpRequest("PUT", BASE .. "/echo");
      r.body = { "name": "test", "value": 42 };
      assert(r.body like "*\"name\":\"test\"*", "hash map serialized to JSON");
      assert(r.body like "*\"value\":42*", "hash map integer value");
      assert(r.headers["Content-Type"] == "application/json", "Content-Type set for JSON body");
      resp = r.execute();
      j = resp.json;
      assert(j.method == "PUT", "PUT method");
      assert(j.headers.get("content-type") == "application/json", "server received JSON content type");
      body = JsonParse(j.body);
      assert(body.name == "test", "server received hash map name");
      assert(body.value == 42, "server received hash map value");

      // JsonObject body
      jo = new JsonObject();
      jo.a = 1;
      jo.b = "text";
      r2 = new HttpRequest("PATCH", BASE .. "/echo");
      r2.body = jo;
      resp = r2.execute();
      assert(resp.json.method == "PATCH", "PATCH method");
      body = JsonParse(resp.json.body);
      assert(body.a == 1, "JsonObject body a");
      assert(body.b == "text", "JsonObject body b");

      // Array body
      r3 = new HttpRequest("POST", BASE .. "/echo");
      r3.body = [1, 2, 3];
      assert(r3.body == "[1,2,3]", "array serialized to JSON");
      resp = r3.execute();
      assert(resp.json.body == "[1,2,3]", "array body received");

      // JsonArray body
      ja = new JsonArray();
      ja.append(5);
      ja.append("x");
      r4 = new HttpRequest("POST", BASE .. "/echo");
      r4.body = ja;
      resp = r4.execute();
      assert(resp.json.body == "[5,\"x\"]", "JsonArray body received");

      // Explicit content type is kept
      r5 = new HttpRequest("POST", BASE .. "/echo");
      r5.setHeader("Content-Type", "application/vnd.test+json");
      r5.body = { "x": 1 };
      assert(r5.headers.size == 1, "no duplicate Content-Type");
      r5.setHeader("content-type", "application/vnd.test+json");
      assert(r5.headers.size == 1, "header names are case-insensitive");
      resp = r5.execute();
      assert(resp.json.headers.get("content-type") == "application/vnd.test+json", "explicit content type kept");

      // null clears body
      r5.body = null;
      assert(r5.body == null, "body cleared");
   )NXSL");
   EndTest();
}

/**
 * HEAD, DELETE, OPTIONS methods and status codes
 */
static void TestMethodsAndStatusCodes()
{
   StartTest(_T("NXSL HTTP: methods and status codes"));
   RunScript(LR"NXSL(
      r = new HttpRequest("HEAD", BASE .. "/text");
      resp = r.execute();
      assert(resp.success, "HEAD success");
      assert(resp.statusCode == 200, "HEAD status");
      assert(resp.bodySize == 0, "HEAD has no body");
      assert(resp.body == "", "HEAD body is empty string");
      assert(resp.contentType == "text/plain; charset=utf-8", "HEAD returns headers");

      r = new HttpRequest("DELETE", BASE .. "/method");
      resp = r.execute();
      assert(resp.body == "DELETE", "DELETE method");

      r = new HttpRequest("OPTIONS", BASE .. "/method");
      resp = r.execute();
      assert(resp.body == "OPTIONS", "OPTIONS method");

      r = new HttpRequest(BASE .. "/status?code=404");
      resp = r.execute();
      assert(resp.success == false, "404 is not success");
      assert(resp.statusCode == 404, "404 status code");
      assert(resp.errorMessage == null, "404 is not transport error");
      assert(resp.body == "status body", "404 body available");

      r = new HttpRequest(BASE .. "/status?code=500");
      resp = r.execute();
      assert(resp.success == false, "500 is not success");
      assert(resp.statusCode == 500, "500 status code");

      r = new HttpRequest(BASE .. "/status?code=201");
      resp = r.execute();
      assert(resp.success == true, "201 is success");

      r = new HttpRequest(BASE .. "/empty");
      resp = r.execute();
      assert(resp.success == true, "204 is success");
      assert(resp.statusCode == 204, "204 status code");
      assert(resp.body == "", "204 empty body");
      assert(resp.bodySize == 0, "204 body size");
      assert(resp.contentType == null, "no content type");

      r = new HttpRequest(BASE .. "/missing");
      resp = r.execute();
      assert(resp.statusCode == 404, "unknown path");
   )NXSL");
   EndTest();
}

/**
 * JSON response parsing
 */
static void TestJsonResponse()
{
   StartTest(_T("NXSL HTTP: JSON responses"));
   RunScript(LR"NXSL(
      resp = (new HttpRequest(BASE .. "/json")).execute();
      j = resp.json;
      assert(j != null, "JSON object parsed");
      assert(j.name == "test", "string field");
      assert(j.values.size == 3, "array field size");
      assert(j.values.get(1) == 2, "array element");
      assert(j.nested.flag == true, "nested boolean");

      resp = (new HttpRequest(BASE .. "/array")).execute();
      a = resp.json;
      assert(a != null, "JSON array parsed");
      assert(a.size == 3, "array size");
      assert(a.get(2) == 30, "array element");

      resp = (new HttpRequest(BASE .. "/notjson")).execute();
      assert(resp.success, "invalid JSON body still success");
      assert(resp.json == null, "invalid JSON not parsed");
      assert(resp.json == null, "invalid JSON not parsed on second access");
      assert(resp.body == "this is not JSON", "body available");

      resp = (new HttpRequest(BASE .. "/empty")).execute();
      assert(resp.json == null, "empty body is not JSON");
   )NXSL");
   EndTest();
}

/**
 * Redirect handling
 */
static void TestRedirects()
{
   StartTest(_T("NXSL HTTP: redirects"));
   RunScript(LR"NXSL(
      r = new HttpRequest(BASE .. "/redirect");
      resp = r.execute();
      assert(resp.success, "redirect followed");
      assert(resp.statusCode == 200, "final status code");
      assert(resp.url == BASE .. "/text", "effective URL after redirect");
      assert(resp.body == "Hello, world! Привет", "body from final location");
      assert(resp.getHeader("X-Redirect") == null, "headers from intermediate response discarded");
      assert(resp.getHeader("Location") == null, "Location header from intermediate response discarded");

      r.followRedirects = false;
      resp = r.execute();
      assert(resp.success == false, "redirect not followed");
      assert(resp.statusCode == 302, "302 status code");
      assert(resp.errorMessage == null, "no transport error");
      assert(resp.getHeader("Location") == BASE .. "/text", "Location header available");
      assert(resp.getHeader("X-Redirect") == "yes", "headers of redirect response available");
      assert(resp.url == BASE .. "/redirect", "effective URL is original");

      r = new HttpRequest(BASE .. "/redirect-loop");
      resp = r.execute();
      assert(resp.success == false, "redirect loop failed");
      assert(resp.errorMessage != null, "redirect loop error message");
      assert(resp.errorMessage like "*redirect*", "redirect loop error text: " .. resp.errorMessage);

      r = new HttpRequest(BASE .. "/redirect-file");
      resp = r.execute();
      assert(resp.success == false, "redirect to file:// rejected");
      assert(resp.errorMessage != null, "redirect to file:// error message");
      assert(resp.bodySize == 0, "redirect to file:// returned no body");
   )NXSL");
   EndTest();
}

/**
 * Response size limits
 */
static void TestResponseSizeLimit()
{
   StartTest(_T("NXSL HTTP: response size limit"));
   RunScript(LR"NXSL(
      r = new HttpRequest(BASE .. "/large?size=1000000");
      resp = r.execute();
      assert(resp.success, "1 MB response received");
      assert(resp.bodySize == 1000000, "1 MB body size");
      assert(length(resp.body) == 1000000, "1 MB body length");

      r.maxResponseSize = 1000;
      resp = r.execute();
      assert(resp.success == false, "response over limit failed");
      assert(resp.errorMessage == "Response size limit exceeded", "limit error message");
      assert(resp.bodySize <= 1000, "body truncated at limit");

      r.maxResponseSize = 0;
      resp = r.execute();
      assert(resp.success, "zero limit means default");

      r.maxResponseSize = 1000;
      r.url = BASE .. "/large?size=1000";
      resp = r.execute();
      assert(resp.success, "response exactly at limit");
      assert(resp.bodySize == 1000, "body at limit");

      r.url = BASE .. "/large?size=1001";
      resp = r.execute();
      assert(resp.success == false, "response one byte over limit failed");

      r.maxResponseSize = null;
      resp = r.execute();
      assert(resp.success, "limit reset");

      // Hard limit of 16 MB cannot be raised by script
      r.url = BASE .. "/large?size=17000000";
      r.maxResponseSize = 100000000;
      resp = r.execute();
      assert(resp.success == false, "response over hard limit failed");
      assert(resp.errorMessage == "Response size limit exceeded", "hard limit error message");
   )NXSL");
   EndTest();
}

/**
 * Timeouts (script attribute and server configuration)
 */
static void TestTimeouts()
{
   StartTest(_T("NXSL HTTP: timeouts"));
   RunScript(LR"NXSL(
      r = new HttpRequest(BASE .. "/slow");
      r.timeout = 1;
      resp = r.execute();
      assert(resp.success == false, "request timed out");
      assert(resp.statusCode == 0, "no status code on timeout");
      assert(resp.errorMessage != null, "timeout error message");
      assert(resp.errorMessage like "*timed out*", "timeout error text: " .. resp.errorMessage);
      assert(resp.responseTime >= 900, "response time reflects timeout");
      assert(resp.responseTime < 1900, "response time reflects timeout, not server delay");

      r.timeout = 10;
      resp = r.execute();
      assert(resp.success, "request completed within timeout");
      assert(resp.body == "slow response", "slow response body");
   )NXSL");

   // Maximum timeout from server configuration caps script value
   AssertTrue(ConfigWriteInt(L"NXSL.HttpRequests.MaxTimeout", 1, true));
   RunScript(LR"NXSL(
      r = new HttpRequest(BASE .. "/slow");
      r.timeout = 30;
      resp = r.execute();
      assert(resp.success == false, "script timeout capped by MaxTimeout");
      assert(resp.errorMessage like "*timed out*", "capped timeout error text");
   )NXSL");
   AssertTrue(ConfigWriteInt(L"NXSL.HttpRequests.MaxTimeout", 300, true));

   // Default timeout from server configuration applies when script does not set one
   AssertTrue(ConfigWriteInt(L"NXSL.HttpRequests.DefaultTimeout", 1, true));
   RunScript(LR"NXSL(
      r = new HttpRequest(BASE .. "/slow");
      resp = r.execute();
      assert(resp.success == false, "default timeout applied");
      assert(resp.errorMessage like "*timed out*", "default timeout error text");

      // Explicit script timeout overrides default
      r.timeout = 10;
      resp = r.execute();
      assert(resp.success, "script timeout overrides default");
   )NXSL");
   AssertTrue(ConfigWriteInt(L"NXSL.HttpRequests.DefaultTimeout", 30, true));

   // Configuration restored
   RunScript(LR"NXSL(
      r = new HttpRequest(BASE .. "/slow");
      resp = r.execute();
      assert(resp.success, "default timeout restored");
   )NXSL");
   EndTest();
}

/**
 * Connection failures and protocol restrictions
 */
static void TestFailures()
{
   StartTest(_T("NXSL HTTP: connection failures and protocol restrictions"));
   RunScript(LR"NXSL(
      r = new HttpRequest("http://127.0.0.1:1/");
      resp = r.execute();
      assert(resp != null, "response object on failure");
      assert(resp.success == false, "connection refused");
      assert(resp.statusCode == 0, "no status code");
      assert(resp.errorMessage != null, "connection error message");
      assert(resp.body == "", "empty body on failure");
      assert(resp.bodySize == 0, "zero body size on failure");
      assert(resp.json == null, "no JSON on failure");
      assert(resp.headers.size == 0, "no headers on failure");

      r = new HttpRequest("file:///etc/hosts");
      resp = r.execute();
      assert(resp.success == false, "file:// rejected");
      assert(resp.errorMessage != null, "file:// error message");
      assert(resp.bodySize == 0, "file:// no body");

      r = new HttpRequest("ftp://127.0.0.1/");
      resp = r.execute();
      assert(resp.success == false, "ftp:// rejected");
      assert(resp.errorMessage != null, "ftp:// error message");

      r = new HttpRequest("not a url");
      resp = r.execute();
      assert(resp.success == false, "malformed URL rejected");
      assert(resp.errorMessage != null, "malformed URL error message");

      r = new HttpRequest("");
      resp = r.execute();
      assert(resp.success == false, "empty URL rejected");

      r = new HttpRequest(BASE .. "/text");
      r.proxy = "http://127.0.0.1:1";
      resp = r.execute();
      assert(resp.success == false, "unreachable proxy");
      assert(resp.errorMessage != null, "proxy error message");
      r.proxy = null;
      resp = r.execute();
      assert(resp.success, "proxy reset");
   )NXSL");
   EndTest();
}

/**
 * Authentication: basic, digest, bearer
 */
static void TestAuthentication()
{
   StartTest(_T("NXSL HTTP: authentication"));
   RunScript(LR"NXSL(
      r = new HttpRequest(BASE .. "/basic");
      resp = r.execute();
      assert(resp.statusCode == 401, "basic: no credentials");
      assert(resp.getHeader("WWW-Authenticate") like "Basic*", "basic: challenge header");

      r.setBasicAuth("user", "pass");
      resp = r.execute();
      assert(resp.statusCode == 200, "basic: valid credentials");
      assert(resp.body == "user", "basic: user name returned");

      r.setBasicAuth("user", "wrong");
      resp = r.execute();
      assert(resp.statusCode == 401, "basic: wrong password");

      r.clearAuth();
      resp = r.execute();
      assert(resp.statusCode == 401, "basic: credentials cleared");

      assert(r.setAuth("BASIC", "user", "pass"), "setAuth basic (case-insensitive type)");
      resp = r.execute();
      assert(resp.statusCode == 200, "basic via setAuth");

      assert(r.setAuth("none"), "setAuth none");
      resp = r.execute();
      assert(resp.statusCode == 401, "auth none");

      // Digest
      r = new HttpRequest(BASE .. "/digest");
      resp = r.execute();
      assert(resp.statusCode == 401, "digest: no credentials");
      assert(resp.getHeader("WWW-Authenticate") like "Digest*", "digest: challenge header");

      r.setAuth("digest", "duser", "dpass");
      resp = r.execute();
      assert(resp.statusCode == 200, "digest: valid credentials");
      assert(resp.body == "duser", "digest: user name returned");

      r.setAuth("digest", "duser", "wrong");
      resp = r.execute();
      assert(resp.statusCode == 401, "digest: wrong password");

      // Basic credentials are not accepted by digest endpoint
      r.setBasicAuth("duser", "dpass");
      resp = r.execute();
      assert(resp.statusCode == 401, "digest: basic credentials rejected");

      // Bearer
      r = new HttpRequest(BASE .. "/bearer");
      resp = r.execute();
      assert(resp.statusCode == 401, "bearer: no token");

      r.setBearerAuth("secret-token");
      resp = r.execute();
      assert(resp.statusCode == 200, "bearer: valid token");
      assert(resp.body == "bearer ok", "bearer: body");

      r.setBearerAuth("wrong-token");
      resp = r.execute();
      assert(resp.statusCode == 401, "bearer: wrong token");

      r.setAuth("bearer", "", "secret-token");
      resp = r.execute();
      assert(resp.statusCode == 200, "bearer via setAuth");

      // Bearer token visible on the wire as Authorization header
      e = new HttpRequest(BASE .. "/echo");
      e.setBearerAuth("secret-token");
      resp = e.execute();
      assert(resp.json.headers.get("authorization") == "Bearer secret-token", "bearer: Authorization header");

      // Basic credentials sent proactively
      e.setBasicAuth("user", "pass");
      resp = e.execute();
      assert(resp.json.headers.get("authorization") == "Basic dXNlcjpwYXNz", "basic: Authorization header");

      e.clearAuth();
      resp = e.execute();
      assert(resp.json.headers.get("authorization") == null, "no Authorization header after clearAuth");
   )NXSL");
   EndTest();
}

/**
 * HttpSession: cookies, convenience methods, close
 */
static void TestSession()
{
   StartTest(_T("NXSL HTTP: session cookies and convenience methods"));
   RunScript(LR"NXSL(
      s = new HttpSession();
      assert(s != null, "session object");
      assert(s.cookies.size == 0, "no cookies initially");
      assert(s.timeout == null, "default session timeout");
      assert(s.headers.size == 0, "default session headers");

      resp = s.get(BASE .. "/cookie");
      assert(resp.success, "first request");
      assert(resp.body == "(none)", "no cookies sent on first request");
      assert(resp.getHeader("Set-Cookie") like "session=abc123*", "Set-Cookie header visible: " .. resp.getHeader("Set-Cookie"));
      c = s.cookies;
      assert(c.size == 2, "two cookies stored");
      assert(c["session"] == "abc123", "session cookie value");
      assert(c["other"] == "xyz", "HttpOnly cookie value");

      resp = s.get(BASE .. "/cookie");
      assert(resp.body like "*session=abc123*", "session cookie sent: " .. resp.body);
      assert(resp.body like "*other=xyz*", "other cookie sent: " .. resp.body);

      s.clearCookies();
      assert(s.cookies.size == 0, "cookies cleared");
      resp = s.get(BASE .. "/cookie");
      assert(resp.body == "(none)", "no cookies after clearCookies");
      assert(s.cookies.size == 2, "cookies stored again");

      s.close();
      assert(s.cookies.size == 0, "no cookies after close");
      resp = s.get(BASE .. "/cookie");
      assert(resp.body == "(none)", "session reusable after close");
      assert(s.cookies.size == 2, "cookies stored after reuse");

      // Cookies are not shared between sessions
      s2 = new HttpSession();
      resp = s2.get(BASE .. "/cookie");
      assert(resp.body == "(none)", "cookies isolated between sessions");

      // Standalone requests do not keep cookies
      r = new HttpRequest(BASE .. "/cookie");
      r.execute();
      resp = r.execute();
      assert(resp.body == "(none)", "standalone request does not keep cookies");

      // Convenience methods
      resp = s.head(BASE .. "/text");
      assert(resp.statusCode == 200 && resp.bodySize == 0, "head()");

      resp = s.post(BASE .. "/echo", "text body");
      assert(resp.json.method == "POST", "post() method");
      assert(resp.json.body == "text body", "post() body");
      assert(resp.json.headers.get("cookie") like "*session=abc123*", "post() sends cookies");

      resp = s.post(BASE .. "/echo", { "k": "v" });
      assert(resp.json.headers.get("content-type") == "application/json", "post() with hash map sets JSON content type");
      assert(resp.json.body == "{\"k\":\"v\"}", "post() hash map body");

      resp = s.post(BASE .. "/echo", null);
      assert(resp.json.method == "POST", "post() with null body");
      assert(resp.json.body == "", "post() null body is empty");

      resp = s.put(BASE .. "/echo", "put body");
      assert(resp.json.method == "PUT", "put() method");
      assert(resp.json.body == "put body", "put() body");

      resp = s.patch(BASE .. "/echo", "patch body");
      assert(resp.json.method == "PATCH", "patch() method");
      assert(resp.json.body == "patch body", "patch() body");

      resp = s.delete(BASE .. "/echo");
      assert(resp.json.method == "DELETE", "delete() method");
      assert(resp.json.body == "", "delete() has no body");

      // execute() with request object
      r = new HttpRequest("POST", BASE .. "/echo");
      r.body = "via execute";
      resp = s.execute(r);
      assert(resp.json.method == "POST", "execute() method");
      assert(resp.json.body == "via execute", "execute() body");
      assert(resp.json.headers.get("cookie") like "*session=abc123*", "execute() sends cookies");
   )NXSL");
   EndTest();
}

/**
 * Precedence of session and request settings
 */
static void TestSettingsPrecedence()
{
   StartTest(_T("NXSL HTTP: session and request settings precedence"));
   RunScript(LR"NXSL(
      s = new HttpSession();
      s.setHeader("X-Session", "s");
      s.setHeader("X-Both", "session");
      s.userAgent = "SessionAgent/1.0";
      assert(s.headers.size == 2, "session headers");
      assert(s.userAgent == "SessionAgent/1.0", "session user agent");

      resp = s.get(BASE .. "/echo");
      j = resp.json;
      assert(j.headers.get("x-session") == "s", "session header sent");
      assert(j.headers.get("x-both") == "session", "session header sent (both)");
      assert(j.headers.get("user-agent") == "SessionAgent/1.0", "session user agent sent");

      r = new HttpRequest(BASE .. "/echo");
      r.setHeader("X-Both", "request");
      r.setHeader("X-Request", "r");
      resp = s.execute(r);
      j = resp.json;
      assert(j.headers.get("x-session") == "s", "session header inherited by request");
      assert(j.headers.get("x-both") == "request", "request header overrides session header");
      assert(j.headers.get("x-request") == "r", "request header sent");
      assert(j.headers.get("user-agent") == "SessionAgent/1.0", "session user agent inherited");

      r.userAgent = "RequestAgent/2.0";
      resp = s.execute(r);
      assert(resp.json.headers.get("user-agent") == "RequestAgent/2.0", "request user agent overrides session");

      // Session settings are not modified by request execution
      assert(s.headers.size == 2, "session headers unchanged");
      assert(s.userAgent == "SessionAgent/1.0", "session user agent unchanged");

      // Request executed standalone does not see session settings
      resp = r.execute();
      assert(resp.json.headers.get("x-session") == null, "standalone execution ignores session headers");

      // Authentication precedence
      s.setBasicAuth("user", "pass");
      resp = s.get(BASE .. "/basic");
      assert(resp.statusCode == 200, "session auth");

      r = new HttpRequest(BASE .. "/basic");
      resp = s.execute(r);
      assert(resp.statusCode == 200, "request inherits session auth");

      r.setAuth("none");
      resp = s.execute(r);
      assert(resp.statusCode == 401, "request auth none overrides session auth");

      r.clearAuth();
      resp = s.execute(r);
      assert(resp.statusCode == 200, "request clearAuth falls back to session auth");

      r.setBasicAuth("user", "wrong");
      resp = s.execute(r);
      assert(resp.statusCode == 401, "request credentials override session credentials");

      s.clearAuth();
      resp = s.get(BASE .. "/basic");
      assert(resp.statusCode == 401, "session auth cleared");

      // Timeout precedence
      s.timeout = 1;
      resp = s.get(BASE .. "/slow");
      assert(resp.success == false, "session timeout applied");
      r = new HttpRequest(BASE .. "/slow");
      r.timeout = 10;
      resp = s.execute(r);
      assert(resp.success, "request timeout overrides session timeout");
      s.timeout = null;
      resp = s.get(BASE .. "/slow");
      assert(resp.success, "session timeout reset");

      // Response size limit precedence
      s.maxResponseSize = 100;
      resp = s.get(BASE .. "/large?size=1000");
      assert(resp.success == false, "session response size limit applied");
      r = new HttpRequest(BASE .. "/large?size=1000");
      r.maxResponseSize = 2000;
      resp = s.execute(r);
      assert(resp.success, "request response size limit overrides session limit");
      s.maxResponseSize = null;

      // Redirect setting precedence
      s.followRedirects = false;
      resp = s.get(BASE .. "/redirect");
      assert(resp.statusCode == 302, "session followRedirects applied");
      r = new HttpRequest(BASE .. "/redirect");
      r.followRedirects = true;
      resp = s.execute(r);
      assert(resp.statusCode == 200, "request followRedirects overrides session");
      s.followRedirects = null;

      // Header map assignment on session
      s.headers = { "X-Map": "1", "Bad Name": "x" };
      assert(s.headers.size == 1, "session headers map assignment with validation");
      resp = s.get(BASE .. "/echo");
      assert(resp.json.headers.get("x-map") == "1", "session header from map sent");
      assert(resp.json.headers.get("x-session") == null, "old session headers replaced");
   )NXSL");
   EndTest();
}

/**
 * Server configuration switch NXSL.HttpRequests.Enabled
 */
static void TestConfigurationSwitch()
{
   StartTest(_T("NXSL HTTP: NXSL.HttpRequests.Enabled"));
   AssertTrue(ConfigWriteInt(L"NXSL.HttpRequests.Enabled", 0, true));
   RunScript(LR"NXSL(
      assert(new HttpRequest(BASE .. "/text") == null, "HttpRequest disabled");
      assert(new HttpSession() == null, "HttpSession disabled");
   )NXSL");
   AssertTrue(ConfigWriteInt(L"NXSL.HttpRequests.Enabled", 1, true));
   RunScript(LR"NXSL(
      assert(new HttpRequest(BASE .. "/text") != null, "HttpRequest enabled");
      assert(new HttpSession() != null, "HttpSession enabled");
   )NXSL");
   EndTest();
}

/**
 * Security contexts: system right SYSTEM_ACCESS_HTTP_REQUESTS
 */
static void TestSecurityContext()
{
   StartTest(_T("NXSL HTTP: security context"));

   static const wchar_t *allowed = LR"NXSL(
      r = new HttpRequest(BASE .. "/text");
      assert(r != null, "HttpRequest allowed");
      assert(r.execute().success, "request executed");
      assert(new HttpSession() != null, "HttpSession allowed");
   )NXSL";
   static const wchar_t *denied = LR"NXSL(
      assert(new HttpRequest(BASE .. "/text") == null, "HttpRequest denied");
      assert(new HttpSession() == null, "HttpSession denied");
   )NXSL";

   // No security context (server-initiated script)
   RunScript(allowed);

   // Administrator (member of Admins group with full system access)
   AssertTrue((GetEffectiveSystemRights(1) & SYSTEM_ACCESS_HTTP_REQUESTS) != 0);
   RunScript(allowed, new NXSL_UserSecurityContext(1));

   // Anonymous user (member of Everyone group only)
   AssertTrue((GetEffectiveSystemRights(2) & SYSTEM_ACCESS_HTTP_REQUESTS) == 0);
   RunScript(denied, new NXSL_UserSecurityContext(2));

   // Non-existing user
   RunScript(denied, new NXSL_UserSecurityContext(0xFFFF));

   // Read-only context (transformation scripts, filters, etc.)
   RunScript(denied, new NXSL_ReadOnlySecurityContext());

   EndTest();
}

/**
 * Objects are usable from user-defined functions and survive being stored in arrays and hash maps
 */
static void TestObjectLifetime()
{
   StartTest(_T("NXSL HTTP: object lifetime"));
   RunScript(LR"NXSL(
      function fetch(session, path)
      {
         return session.get(BASE .. path);
      }

      s = new HttpSession();
      responses = [];
      for(i = 0; i < 20; i++)
         responses.append(fetch(s, "/json"));
      assert(responses.size == 20, "responses stored");
      for(resp : responses)
      {
         assert(resp.success, "stored response success");
         assert(resp.json.name == "test", "stored response JSON");
      }

      m = { "req": new HttpRequest(BASE .. "/text"), "session": s };
      assert(m["req"].execute().body == "Hello, world! Привет", "request from hash map");
      assert(m["session"].get(BASE .. "/text").success, "session from hash map");

      // Response outlives request and session objects
      resp = (new HttpSession()).get(BASE .. "/json");
      s = null;
      m = null;
      assert(resp.json.name == "test", "response usable after session released");
   )NXSL");
   EndTest();
}

/**
 * Test entry point
 */
void TestNXSLHttp()
{
   StartTest(_T("NXSL HTTP: embedded HTTP server startup"));
   StartHttpServer();
   EndTest();

   TestRequestObject();
   TestSimpleGet();
   TestEcho();
   TestJsonBody();
   TestMethodsAndStatusCodes();
   TestJsonResponse();
   TestRedirects();
   TestResponseSizeLimit();
   TestTimeouts();
   TestFailures();
   TestAuthentication();
   TestSession();
   TestSettingsPrecedence();
   TestConfigurationSwitch();
   TestSecurityContext();
   TestObjectLifetime();

   StopHttpServer();
}
