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
** File: test-authtokens.cpp
**
** Unit tests for authentication token validation and consumption. Only ephemeral,
** service and single-use tokens are issued so that the persistent token code paths,
** which require a database connection, are never entered.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <nms_users.h>
#include <testtools.h>

#define TEST_USER_ID    1

/**
 * Stubs for server core symbols referenced by authtokens.cpp. The token code
 * under test never reaches the database, so the query stub only has to exist.
 */
uint32_t NXCORE_EXPORTABLE ConfigReadULong(const wchar_t *variable, uint32_t defaultValue)
{
   return defaultValue;
}

uint32_t NXCORE_EXPORTABLE CreateUniqueId(int group)
{
   static VolatileCounter id = 0;
   return static_cast<uint32_t>(InterlockedIncrement(&id));
}

bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, uint32_t objectId, const wchar_t *query)
{
   return true;
}

wchar_t NXCORE_EXPORTABLE *ResolveUserId(uint32_t id, wchar_t *buffer, bool noFail)
{
   if (id & GROUP_FLAG)
      return noFail ? wcscpy(buffer, L"[unknown]") : nullptr;
   nx_swprintf(buffer, MAX_USER_NAME, L"user-%u", id);
   return buffer;
}

/**
 * Issue token and check that it was created
 */
static shared_ptr<AuthenticationTokenDescriptor> IssueToken(uint32_t validFor,
   AuthenticationTokenType type = AuthenticationTokenType::EPHEMERAL, uint32_t maxLifetime = 0)
{
   shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueAuthenticationToken(TEST_USER_ID, validFor, type, nullptr, maxLifetime);
   AssertNotNull(descriptor.get());
   return descriptor;
}

/**
 * Validation must reject a single-use token without destroying it
 */
static void TestValidateRejectsSingleUse()
{
   StartTest(_T("Validate rejects single-use token and leaves it valid"));

   shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueToken(600, AuthenticationTokenType::SINGLE_USE);

   uint32_t userId = 0;
   AssertFalse(ValidateAuthenticationToken(descriptor->token, &userId));
   AssertEquals(userId, static_cast<uint32_t>(0));

   // Rejection must not consume the token - it is still spendable at the designated spend point
   AuthenticationTokenType tokenType = AuthenticationTokenType::EPHEMERAL;
   AssertTrue(ConsumeAuthenticationToken(descriptor->token, &userId, &tokenType));
   AssertTrue(tokenType == AuthenticationTokenType::SINGLE_USE);
   AssertEquals(userId, static_cast<uint32_t>(TEST_USER_ID));

   EndTest();
}

/**
 * Single-use token can be consumed exactly once
 */
static void TestConsumeSingleUseOnce()
{
   StartTest(_T("Consume single-use token exactly once"));

   shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueToken(600, AuthenticationTokenType::SINGLE_USE);

   uint32_t userId = 0;
   AuthenticationTokenType tokenType = AuthenticationTokenType::SERVICE;
   AssertTrue(ConsumeAuthenticationToken(descriptor->token, &userId, &tokenType));
   AssertEquals(userId, static_cast<uint32_t>(TEST_USER_ID));
   AssertTrue(tokenType == AuthenticationTokenType::SINGLE_USE);

   userId = 0;
   tokenType = AuthenticationTokenType::SERVICE;
   AssertFalse(ConsumeAuthenticationToken(descriptor->token, &userId, &tokenType));
   AssertFalse(ConsumeAuthenticationToken(descriptor->token, &userId, &tokenType));
   AssertFalse(ValidateAuthenticationToken(descriptor->token, &userId));

   EndTest();
}

/**
 * Ordinary token must survive consumption - this is the reconnect token path
 */
static void TestConsumeNonSingleUse()
{
   StartTest(_T("Consume does not destroy ordinary token"));

   shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueToken(600);

   uint32_t userId = 0;
   AuthenticationTokenType tokenType = AuthenticationTokenType::SINGLE_USE;
   AssertTrue(ConsumeAuthenticationToken(descriptor->token, &userId, &tokenType));
   AssertEquals(userId, static_cast<uint32_t>(TEST_USER_ID));
   AssertTrue(tokenType == AuthenticationTokenType::EPHEMERAL);

   // Repeated logins with the same reconnect token must keep working
   tokenType = AuthenticationTokenType::SINGLE_USE;
   AssertTrue(ConsumeAuthenticationToken(descriptor->token, &userId, &tokenType));
   AssertTrue(tokenType == AuthenticationTokenType::EPHEMERAL);
   AssertTrue(ValidateAuthenticationToken(descriptor->token, &userId));

   // Service token type must still be reported correctly
   shared_ptr<AuthenticationTokenDescriptor> serviceDescriptor = IssueToken(600, AuthenticationTokenType::SERVICE);
   tokenType = AuthenticationTokenType::SINGLE_USE;
   AssertTrue(ConsumeAuthenticationToken(serviceDescriptor->token, &userId, &tokenType));
   AssertTrue(tokenType == AuthenticationTokenType::SERVICE);

   EndTest();
}

/**
 * Validation of an ordinary token, including expiration extension
 */
static void TestValidateNonSingleUse()
{
   StartTest(_T("Validate ordinary token with expiration extension"));

   shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueToken(60, AuthenticationTokenType::EPHEMERAL, 3600);
   time_t originalExpiration = descriptor->expirationTime;

   uint32_t userId = 0;
   time_t expiresAt = 0;
   time_t maxExpiresAt = 0;
   AssertTrue(ValidateAuthenticationToken(descriptor->token, &userId, 0, &expiresAt, &maxExpiresAt));
   AssertEquals(userId, static_cast<uint32_t>(TEST_USER_ID));
   AssertTrue(expiresAt == originalExpiration);
   AssertTrue(maxExpiresAt == descriptor->maxExpirationTime);
   AssertTrue(descriptor->expirationTime == originalExpiration);

   // Extension moves expiration forward but never past the absolute lifetime cap
   AssertTrue(ValidateAuthenticationToken(descriptor->token, &userId, 600, &expiresAt, nullptr));
   AssertTrue(descriptor->expirationTime > originalExpiration);
   AssertTrue(expiresAt == descriptor->expirationTime);
   AssertTrue(descriptor->expirationTime <= descriptor->maxExpirationTime);

   AssertTrue(ValidateAuthenticationToken(descriptor->token, &userId, 86400, nullptr, nullptr));
   AssertTrue(descriptor->expirationTime == descriptor->maxExpirationTime);

   EndTest();
}

/**
 * Single-use rejection must happen before the expiration extension block
 */
static void TestRejectionBeforeExtension()
{
   StartTest(_T("Single-use rejection happens before expiration extension"));

   shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueToken(60, AuthenticationTokenType::SINGLE_USE, 86400);
   time_t originalExpiration = descriptor->expirationTime;

   uint32_t userId = 0;
   AssertFalse(ValidateAuthenticationToken(descriptor->token, &userId, 14400));
   AssertTrue(descriptor->expirationTime == originalExpiration);

   EndTest();
}

/**
 * Concurrent consumption of a single-use token
 */
#define RACE_THREADS    16
#define RACE_ROUNDS     50

static Condition s_raceStartGate(true);
static VolatileCounter s_raceSuccessCount = 0;
static VolatileCounter s_raceGateFailures = 0;
static UserAuthenticationToken s_raceToken;

static void RaceWorkerThread()
{
   if (!s_raceStartGate.wait(10000))
      InterlockedIncrement(&s_raceGateFailures);
   uint32_t userId = 0;
   AuthenticationTokenType tokenType = AuthenticationTokenType::EPHEMERAL;
   if (ConsumeAuthenticationToken(s_raceToken, &userId, &tokenType) && (tokenType == AuthenticationTokenType::SINGLE_USE) && (userId == TEST_USER_ID))
      InterlockedIncrement(&s_raceSuccessCount);
}

static void TestConcurrentConsume()
{
   StartTest(_T("Concurrent consumption yields exactly one winner"));

   // Repeated rounds because the contention window is only a few microseconds wide -
   // a single round can miss the claim race entirely and still pass
   s_raceGateFailures = 0;
   for(int round = 0; round < RACE_ROUNDS; round++)
   {
      shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueToken(600, AuthenticationTokenType::SINGLE_USE);
      s_raceToken = descriptor->token;
      s_raceSuccessCount = 0;
      s_raceStartGate.reset();

      THREAD threads[RACE_THREADS];
      for(int i = 0; i < RACE_THREADS; i++)
         threads[i] = ThreadCreateEx(RaceWorkerThread);
      ThreadSleepMs(5);
      s_raceStartGate.set();
      for(int i = 0; i < RACE_THREADS; i++)
         ThreadJoin(threads[i]);

      AssertEquals(static_cast<int32_t>(s_raceSuccessCount), 1);
   }

   // A gate timeout would silently degrade this into sequential consumption
   AssertEquals(static_cast<int32_t>(s_raceGateFailures), 0);

   EndTest();
}

/**
 * Expired single-use token is rejected by both entry points. Two separate tokens
 * are used because the first call removes the descriptor from the token map.
 */
static void TestExpiredSingleUse()
{
   StartTest(_T("Expired single-use token is rejected"));

   shared_ptr<AuthenticationTokenDescriptor> validationTarget = IssueToken(0, AuthenticationTokenType::SINGLE_USE);
   shared_ptr<AuthenticationTokenDescriptor> consumeTarget = IssueToken(0, AuthenticationTokenType::SINGLE_USE);

   uint32_t userId = 0;
   AssertFalse(ValidateAuthenticationToken(validationTarget->token, &userId));

   AuthenticationTokenType tokenType = AuthenticationTokenType::EPHEMERAL;
   AssertFalse(ConsumeAuthenticationToken(consumeTarget->token, &userId, &tokenType));
   AssertEquals(static_cast<int32_t>(consumeTarget->claimed), 0);

   EndTest();
}

/**
 * Single-use tokens exist in memory only - they never get a database identifier, which is
 * what makes them mutually exclusive with persistent tokens. A single-use token surviving
 * a restart would be indistinguishable from an unspent one.
 */
static void TestSingleUseIsMemoryOnly()
{
   StartTest(_T("Single-use token is memory only"));

   shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueToken(600, AuthenticationTokenType::SINGLE_USE);
   AssertEquals(descriptor->tokenId, static_cast<uint32_t>(0));
   AssertTrue(descriptor->type == AuthenticationTokenType::SINGLE_USE);

   EndTest();
}

/**
 * Token cannot be issued for a group or for an unknown user, and a persistent token that would be
 * born already expired is refused as well. Every caller depends on this null return to reject the
 * request instead of dereferencing the descriptor.
 */
static void TestIssueFailures()
{
   StartTest(_T("Token issuing failures (invalid user, already expired persistent token)"));

   AssertNull(IssueAuthenticationToken(GROUP_FLAG | TEST_USER_ID, 600).get());
   AssertNull(IssueAuthenticationToken(TEST_USER_ID, 0, AuthenticationTokenType::PERSISTENT).get());

   // Zero validity period is refused only for persistent tokens - an already expired ephemeral
   // token is harmless and is cleaned up by the expiration check
   AssertNotNull(IssueAuthenticationToken(TEST_USER_ID, 0).get());

   EndTest();
}

/**
 * Only persistent tokens are assigned an ID, so revocation by ID 0 must not match one of the
 * memory-only tokens that all carry 0
 */
static void TestRevokeByIdRejectsZero()
{
   StartTest(_T("Revocation by token ID 0 is rejected"));

   shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueToken(600);
   AssertEquals(descriptor->tokenId, static_cast<uint32_t>(0));

   AssertEquals(RevokeAuthenticationToken(static_cast<uint32_t>(0), static_cast<uint32_t>(0)), static_cast<uint32_t>(RCC_INVALID_TOKEN_ID));

   uint32_t userId = 0;
   AssertTrue(ValidateAuthenticationToken(descriptor->token, &userId));
   AssertEquals(userId, static_cast<uint32_t>(TEST_USER_ID));

   EndTest();
}

/**
 * Expiration time of a persistent token is stored in a 32-bit database column, so a
 * descriptor with an oversized validity period must be clamped at construction time -
 * otherwise the token would be written back and read as a different (or negative) time.
 *
 * MAX_PERSISTENT_TOKEN_EXPIRATION_TIME is also the largest representable time_t on platforms
 * with a 32-bit time_t, where an oversized validity period overflows the type instead of
 * exceeding the cap. Clamping cannot be observed there, so the check only runs on 64-bit time_t.
 */
static void TestPersistentExpirationClamp()
{
   StartTest(_T("Persistent token expiration time is clamped"));

   if (sizeof(time_t) > 4)
   {
      AuthenticationTokenDescriptor descriptor(TEST_USER_ID, UINT32_MAX, AuthenticationTokenType::PERSISTENT, nullptr);
      AssertEquals(static_cast<int64_t>(descriptor.expirationTime), MAX_PERSISTENT_TOKEN_EXPIRATION_TIME);
      AssertEquals(static_cast<int64_t>(descriptor.maxExpirationTime), MAX_PERSISTENT_TOKEN_EXPIRATION_TIME);

      AuthenticationTokenDescriptor ephemeral(TEST_USER_ID, UINT32_MAX, AuthenticationTokenType::EPHEMERAL, nullptr);
      AssertTrue(static_cast<int64_t>(ephemeral.expirationTime) > MAX_PERSISTENT_TOKEN_EXPIRATION_TIME);
   }
   else
   {
      _tprintf(_T("(skipped on 32-bit time_t) "));
   }

   EndTest();
}

/**
 * A memory-only token stops authenticating at its absolute lifetime cap, so a validity period
 * longer than the cap must be reduced at construction time - otherwise the descriptor (and the
 * expiration time reported to the client) would promise validity the token will never have.
 */
static void TestLifetimeCapClamp()
{
   StartTest(_T("Expiration time is clamped to absolute lifetime cap"));

   shared_ptr<AuthenticationTokenDescriptor> capped = IssueToken(86400, AuthenticationTokenType::EPHEMERAL, 3600);
   AssertTrue(capped->expirationTime == capped->maxExpirationTime);
   AssertEquals(static_cast<int64_t>(capped->expirationTime - capped->issuingTime), static_cast<int64_t>(3600));

   // A validity period within the cap is left alone
   shared_ptr<AuthenticationTokenDescriptor> uncapped = IssueToken(600, AuthenticationTokenType::SINGLE_USE, 3600);
   AssertTrue(uncapped->expirationTime < uncapped->maxExpirationTime);
   AssertEquals(static_cast<int64_t>(uncapped->expirationTime - uncapped->issuingTime), static_cast<int64_t>(600));

   EndTest();
}

/**
 * A token that reached its absolute lifetime cap must be rejected and evicted by both entry
 * points even though its nominal expiration time is still in the future
 */
static void TestMaxLifetimeReached()
{
   StartTest(_T("Token that reached maximum lifetime is rejected"));

   shared_ptr<AuthenticationTokenDescriptor> validationTarget = IssueToken(3600, AuthenticationTokenType::EPHEMERAL, 3600);
   shared_ptr<AuthenticationTokenDescriptor> consumeTarget = IssueToken(3600, AuthenticationTokenType::SINGLE_USE, 3600);

   time_t now = time(nullptr);
   validationTarget->maxExpirationTime = now - 1;
   consumeTarget->maxExpirationTime = now - 1;

   uint32_t userId = 0;
   AssertFalse(ValidateAuthenticationToken(validationTarget->token, &userId));
   AssertTrue(validationTarget->expirationTime > now);

   AssertFalse(ConsumeAuthenticationToken(consumeTarget->token, &userId));
   AssertEquals(static_cast<int32_t>(consumeTarget->claimed), 0);

   EndTest();
}

/**
 * Revocation by token value works for memory-only tokens, which cannot be revoked by ID
 */
static void TestRevokeByTokenValue()
{
   StartTest(_T("Revocation by token value"));

   shared_ptr<AuthenticationTokenDescriptor> descriptor = IssueToken(600);
   uint32_t userId = 0;
   AssertTrue(ValidateAuthenticationToken(descriptor->token, &userId));

   RevokeAuthenticationToken(descriptor->token);
   AssertFalse(ValidateAuthenticationToken(descriptor->token, &userId));

   // Revocation of already revoked token must be a no-op
   RevokeAuthenticationToken(descriptor->token);
   AssertFalse(ValidateAuthenticationToken(descriptor->token, &userId));

   EndTest();
}

/**
 * Token type name is what audit log entries and the server console listing show
 */
static void TestTokenTypeNames()
{
   StartTest(_T("Token type names"));

   AssertEquals(AuthenticationTokenTypeName(AuthenticationTokenType::EPHEMERAL), L"ephemeral");
   AssertEquals(AuthenticationTokenTypeName(AuthenticationTokenType::PERSISTENT), L"persistent");
   AssertEquals(AuthenticationTokenTypeName(AuthenticationTokenType::SERVICE), L"service");
   AssertEquals(AuthenticationTokenTypeName(AuthenticationTokenType::SINGLE_USE), L"single-use");

   EndTest();
}

/**
 * Neither protocol carries the token type - both derive three mutually exclusive booleans
 * from it, and a client reading them back must see exactly one set (or none, for ephemeral)
 */
static void TestSerializedTokenType()
{
   StartTest(_T("Token type is serialized as mutually exclusive flags"));

   static const AuthenticationTokenType types[] =
   {
      AuthenticationTokenType::EPHEMERAL,
      AuthenticationTokenType::PERSISTENT,
      AuthenticationTokenType::SERVICE,
      AuthenticationTokenType::SINGLE_USE
   };

   for(size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++)
   {
      // Descriptor is constructed directly instead of being issued, so that a persistent
      // token is not written to the database
      AuthenticationTokenDescriptor descriptor(TEST_USER_ID, 600, types[i], L"serialization test");
      bool persistent = (types[i] == AuthenticationTokenType::PERSISTENT);
      bool service = (types[i] == AuthenticationTokenType::SERVICE);
      bool singleUse = (types[i] == AuthenticationTokenType::SINGLE_USE);

      NXCPMessage msg(CMD_REQUEST_COMPLETED, 1);
      descriptor.fillMessage(&msg, VID_ELEMENT_LIST_BASE);
      AssertTrue(msg.getFieldAsBoolean(VID_ELEMENT_LIST_BASE + 2) == persistent);
      AssertTrue(msg.getFieldAsBoolean(VID_ELEMENT_LIST_BASE + 7) == service);
      AssertTrue(msg.getFieldAsBoolean(VID_ELEMENT_LIST_BASE + 8) == singleUse);

      json_t *json = descriptor.toJson();
      AssertTrue((json_is_true(json_object_get(json, "persistent")) != 0) == persistent);
      AssertTrue((json_is_true(json_object_get(json, "service")) != 0) == service);
      AssertTrue((json_is_true(json_object_get(json, "singleUse")) != 0) == singleUse);
      AssertNotNull(json_object_get(json, "value"));
      json_decref(json);
   }

   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   TestValidateRejectsSingleUse();
   TestConsumeSingleUseOnce();
   TestConsumeNonSingleUse();
   TestValidateNonSingleUse();
   TestRejectionBeforeExtension();
   TestConcurrentConsume();
   TestExpiredSingleUse();
   TestSingleUseIsMemoryOnly();
   TestIssueFailures();
   TestRevokeByIdRejectsZero();
   TestPersistentExpirationClamp();
   TestLifetimeCapClamp();
   TestMaxLifetimeReached();
   TestRevokeByTokenValue();
   TestTokenTypeNames();
   TestSerializedTokenType();

   return 0;
}
