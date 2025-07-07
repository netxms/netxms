#ifdef _WIN32
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>
#include <netxms-version.h>

// Define NXCORE_EXPORTABLE for test build
#ifndef NXCORE_EXPORTABLE
#define NXCORE_EXPORTABLE
#endif

#include <jwt.h>

NETXMS_EXECUTABLE_HEADER(test-jwt)

/**
 * Test JWT token creation and encoding
 */
static void TestJwtTokenCreation()
{
   StartTest(_T("JWT token creation"));
   
   json_t *payload = json_object();
   json_object_set_new(payload, "userId", json_integer(1));
   json_object_set_new(payload, "systemAccessRights", json_integer(123456));
   json_object_set_new(payload, "type", json_string("access"));
   json_object_set_new(payload, "jti", json_string("test-jti-12345"));
   
   const char *secret = "test-secret-key-256-bit-minimum-32-bytes-required-for-hmac-sha256-security";
   time_t expiration = time(nullptr) + 3600; // 1 hour from now
   
   JwtToken *token = JwtToken::create(payload, secret, expiration);
   AssertNotNull(token);
   AssertTrue(token->isValid());
   AssertFalse(token->isExpired());
   
   String encoded = token->encode(secret);
   AssertFalse(encoded.isEmpty());
   
   
   // JWT should have 3 parts separated by dots
   StringList parts;
   parts.splitAndAdd(encoded, _T("."));
   AssertEquals(parts.size(), 3);
   
   // Verify the token can be parsed back with the same secret
   char *encodedUtf8 = UTF8StringFromTString(encoded.cstr());
   JwtToken *parsedToken = JwtToken::parse(encodedUtf8, secret);
   MemFree(encodedUtf8);
   AssertNotNull(parsedToken);
   AssertTrue(parsedToken->isValid());
   
   // Verify claims match
   AssertEquals((int64_t)parsedToken->getClaimAsInt("userId"), (int64_t)1);
   AssertEquals((int64_t)parsedToken->getClaimAsInt("systemAccessRights"), (int64_t)123456);
   String type = parsedToken->getClaim("type");
   AssertTrue(type.equals(_T("access")));
   String jti = parsedToken->getJti();
   AssertTrue(jti.equals(_T("test-jti-12345")));
   
   delete parsedToken;
   delete token;
   json_decref(payload);
   
   EndTest();
}

/**
 * Test JWT token parsing and validation
 */
static void TestJwtTokenParsing()
{
   StartTest(_T("JWT token parsing"));
   
   // Create a token first
   json_t *payload = json_object();
   json_object_set_new(payload, "userId", json_integer(42));
   json_object_set_new(payload, "type", json_string("access"));
   json_object_set_new(payload, "jti", json_string("parse-test-jti"));
   
   const char *secret = "parse-test-secret-32-bytes-minimum-for-hmac-sha256-security-compliance";
   time_t expiration = time(nullptr) + 3600;
   
   JwtToken *originalToken = JwtToken::create(payload, secret, expiration);
   String encoded = originalToken->encode(secret);
   delete originalToken;
   
   // Now parse it back
   char *encodedUtf8 = UTF8StringFromTString(encoded.cstr());
   JwtToken *parsedToken = JwtToken::parse(encodedUtf8, secret);
   MemFree(encodedUtf8);
   
   AssertNotNull(parsedToken);
   AssertTrue(parsedToken->isValid());
   AssertFalse(parsedToken->isExpired());
   
   // Verify claims
   AssertEquals((int64_t)parsedToken->getClaimAsInt("userId"), (int64_t)42);
   String type = parsedToken->getClaim("type");
   AssertTrue(type.equals(_T("access")));
   String jti = parsedToken->getJti();
   AssertTrue(jti.equals(_T("parse-test-jti")));
   
   delete parsedToken;
   json_decref(payload);
   
   EndTest();
}

/**
 * Test JWT token with invalid signature
 */
static void TestJwtTokenInvalidSignature()
{
   StartTest(_T("JWT token invalid signature"));
   
   json_t *payload = json_object();
   json_object_set_new(payload, "userId", json_integer(1));
   json_object_set_new(payload, "type", json_string("access"));
   
   const char *secret1 = "secret-one-32-bytes-minimum-required-for-hmac-sha256-security-standards";
   const char *secret2 = "secret-two-32-bytes-minimum-required-for-hmac-sha256-security-standards";
   time_t expiration = time(nullptr) + 3600;
   
   JwtToken *token = JwtToken::create(payload, secret1, expiration);
   String encoded = token->encode(secret1);
   delete token;
   
   // Try to parse with different secret
   char *encodedUtf8 = UTF8StringFromTString(encoded.cstr());
   JwtToken *parsedToken = JwtToken::parse(encodedUtf8, secret2);
   MemFree(encodedUtf8);
   
   AssertNull(parsedToken); // Should fail due to signature mismatch
   
   json_decref(payload);
   
   EndTest();
}

/**
 * Test JWT token expiration
 */
static void TestJwtTokenExpiration()
{
   StartTest(_T("JWT token expiration"));
   
   json_t *payload = json_object();
   json_object_set_new(payload, "userId", json_integer(1));
   json_object_set_new(payload, "type", json_string("access"));
   
   const char *secret = "expiration-test-secret-32-bytes-minimum-for-hmac-sha256-security";
   time_t now = time(nullptr);
   time_t expiration = now - 60; // 1 minute ago (expired)
   
   JwtToken *token = JwtToken::create(payload, secret, expiration);
   AssertNotNull(token);
   String encoded = token->encode(secret);
   delete token;
   
   char *encodedUtf8 = UTF8StringFromTString(encoded.cstr());
   JwtToken *parsedToken = JwtToken::parse(encodedUtf8, secret);
   MemFree(encodedUtf8);
   AssertNull(parsedToken); // Should reject expired token during parsing
   json_decref(payload);
   
   EndTest();
}

/**
 * Test JWT token blocklist functionality
 */
static void TestJwtTokenBlocklist()
{
   StartTest(_T("JWT token blocklist"));
   
   JwtTokenBlocklist *blocklist = JwtTokenBlocklist::getInstance();
   AssertNotNull(blocklist);
   
   const char *testJti = "blocklist-test-jti";
   time_t expiration = time(nullptr) + 3600;
   
   // Initially should not be blocked
   AssertFalse(blocklist->isBlocked(testJti));
   
   // Add to blocklist
   blocklist->addToken(testJti, expiration);
   
   // Should now be blocked
   AssertTrue(blocklist->isBlocked(testJti));
   
   // Test with null JTI
   AssertFalse(blocklist->isBlocked(nullptr));
   
   EndTest();
}

/**
 * Test JWT claim extraction
 */
static void TestJwtClaims()
{
   StartTest(_T("JWT claims extraction"));
   
   json_t *payload = json_object();
   json_object_set_new(payload, "userId", json_integer(999));
   json_object_set_new(payload, "userName", json_string("testuser"));
   json_object_set_new(payload, "systemAccessRights", json_integer(0x7FFFFFFF));
   json_object_set_new(payload, "isAdmin", json_boolean(true));
   json_object_set_new(payload, "score", json_real(95.5));
   
   const char *secret = "claims-test-secret-32-bytes-minimum-required-for-hmac-sha256";
   time_t expiration = time(nullptr) + 3600;
   
   JwtToken *token = JwtToken::create(payload, secret, expiration);
   String encoded = token->encode(secret);
   delete token;
   
   char *encodedUtf8 = UTF8StringFromTString(encoded.cstr());
   JwtToken *parsedToken = JwtToken::parse(encodedUtf8, secret);
   MemFree(encodedUtf8);
   
   AssertNotNull(parsedToken);
   
   // Test integer claims
   AssertEquals((int64_t)parsedToken->getClaimAsInt("userId"), (int64_t)999);
   AssertEquals((int64_t)parsedToken->getClaimAsInt("systemAccessRights"), (int64_t)0x7FFFFFFF);
   AssertEquals((int64_t)parsedToken->getClaimAsInt("nonexistent", -1), (int64_t)-1);
   
   // Test string claims
   String userName = parsedToken->getClaim("userName");
   AssertTrue(userName.equals(_T("testuser")));
   
   // Test boolean claims (returned as string)
   String isAdmin = parsedToken->getClaim("isAdmin");
   AssertTrue(isAdmin.equals(_T("true")));
   
   // Test real claims (returned as string)
   String score = parsedToken->getClaim("score");
   AssertTrue(score.startsWith(_T("95.5")));
   
   // Test nonexistent claim
   String nonexistent = parsedToken->getClaim("nonexistent");
   AssertTrue(nonexistent.isEmpty());
   
   delete parsedToken;
   json_decref(payload);
   
   EndTest();
}

/**
 * Test base64url encoding/decoding
 */
static void TestBase64UrlEncoding()
{
   StartTest(_T("Base64URL encoding"));
   
   // Test data that would contain + and / in regular base64
   const char *testData = "Hello World! This is a test string with special characters: <>?";
   
   // Test the internal base64url_encode function (we'll create a simple wrapper)
   json_t *payload = json_object();
   json_object_set_new(payload, "data", json_string(testData));
   
   const char *secret = "base64url-test-secret-32-bytes-minimum-for-hmac-sha256-security";
   time_t expiration = time(nullptr) + 3600;
   
   JwtToken *token = JwtToken::create(payload, secret, expiration);
   String encoded = token->encode(secret);
   delete token;
   
   // Verify no + or / characters in the encoded string (URL-safe)
   const TCHAR *encodedStr = encoded.cstr();
   for (int i = 0; encodedStr[i]; i++)
   {
      AssertFalse(encodedStr[i] == _T('+'));
      AssertFalse(encodedStr[i] == _T('/'));
   }
   
   // Verify it can be parsed back
   char *encodedUtf8 = UTF8StringFromTString(encoded.cstr());
   JwtToken *parsedToken = JwtToken::parse(encodedUtf8, secret);
   MemFree(encodedUtf8);
   
   AssertNotNull(parsedToken);
   String retrievedData = parsedToken->getClaim("data");
   AssertTrue(retrievedData.equals(TStringFromUTF8String(testData)));
   
   delete parsedToken;
   json_decref(payload);
   
   EndTest();
}

/**
 * Test JWT external verification compatibility
 */
static void TestJwtExternalVerification()
{
   StartTest(_T("JWT external verification"));
   
   json_t *payload = json_object();
   json_object_set_new(payload, "userId", json_integer(1));
   json_object_set_new(payload, "systemAccessRights", json_integer(9223372036854775807LL));
   json_object_set_new(payload, "type", json_string("access"));
   json_object_set_new(payload, "jti", json_string("0e2e352a-b429-4ba1-8d59-d3c804ac4d7e"));
   json_object_set_new(payload, "exp", json_integer(1751926671));
   json_object_set_new(payload, "iat", json_integer(1751924871));
   
   const char *secret = "your-256-bit-secret-key-here-change-in-production-please-make-it-long-and-random";
   
   JwtToken *token = JwtToken::create(payload, secret, 0); // No expiration override
   AssertNotNull(token);
   
   String encoded = token->encode(secret);
   AssertFalse(encoded.isEmpty());
   
   
   // Verify it can be parsed back internally
   char *encodedUtf8 = UTF8StringFromTString(encoded.cstr());
   JwtToken *parsedToken = JwtToken::parse(encodedUtf8, secret);
   MemFree(encodedUtf8);
   
   AssertNotNull(parsedToken);
   AssertTrue(parsedToken->isValid());
   
   delete parsedToken;
   delete token;
   json_decref(payload);
   
   EndTest();
}

/**
 * Test JWT security - weak secret rejection
 */
static void TestJwtWeakSecretRejection()
{
   StartTest(_T("JWT weak secret rejection"));
   
   json_t *payload = json_object();
   json_object_set_new(payload, "userId", json_integer(1));
   json_object_set_new(payload, "type", json_string("access"));
   
   const char *weakSecret = "short-key"; // Only 9 bytes, should be rejected
   time_t expiration = time(nullptr) + 3600;
   
   // Should fail to create token with weak secret
   JwtToken *token = JwtToken::create(payload, weakSecret, expiration);
   AssertNull(token); // Should reject weak secret
   
   // Should also fail to parse with weak secret
   const char *testToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ0ZXN0IjoidmFsdWUifQ.test";
   JwtToken *parsedToken = JwtToken::parse(testToken, weakSecret);
   AssertNull(parsedToken); // Should reject weak secret
   
   json_decref(payload);
   
   EndTest();
}

/**
 * Main test function
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);
   
   WriteToTerminalEx(_T("NetXMS JWT Tests  v%d.%d.%d\n\n"), 
                     NETXMS_VERSION_MAJOR, NETXMS_VERSION_MINOR, NETXMS_VERSION_RELEASE);
   
   TestJwtTokenCreation();
   TestJwtTokenParsing();
   TestJwtTokenInvalidSignature();
   TestJwtTokenExpiration();
   TestJwtTokenBlocklist();
   TestJwtClaims();
   TestBase64UrlEncoding();
   TestJwtExternalVerification();
   TestJwtWeakSecretRejection();
   
   WriteToTerminal(_T("\nAll tests passed successfully!\n"));
   
   return 0;
}