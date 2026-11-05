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
** File: sshkeys.cpp
**
** Tests for SSH key import, edit and generation through the server core functions
** used by NXCP and WebAPI handlers. Stored keys are observed through FindSshKeyById,
** which returns both keys the way agents receive them. Every key created here is
** deleted before the test returns.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>
#include <string>

/**
 * Keys generated with ssh-keygen -C "", s_pub* strings are the first two fields of ssh-keygen -y output
 */
static const char *s_keyEd25519 =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW\n"
   "QyNTUxOQAAACCOOEUiqU27Ixb0PRXsvAVN2vDzfDmPT5Kdo5epGX1ZrQAAAIhQOEmyUDhJ\n"
   "sgAAAAtzc2gtZWQyNTUxOQAAACCOOEUiqU27Ixb0PRXsvAVN2vDzfDmPT5Kdo5epGX1ZrQ\n"
   "AAAEBIrGkjUkDgzigS8krFT+NAXAqoO5+BxCYS7EIn+oV6co44RSKpTbsjFvQ9Fey8BU3a\n"
   "8PN8OY9Pkp2jl6kZfVmtAAAAAAECAwQF\n"
   "-----END OPENSSH PRIVATE KEY-----\n";
static const char *s_pubEd25519 = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAII44RSKpTbsjFvQ9Fey8BU3a8PN8OY9Pkp2jl6kZfVmt";

static const char *s_keyEcdsa256 =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAaAAAABNlY2RzYS\n"
   "1zaGEyLW5pc3RwMjU2AAAACG5pc3RwMjU2AAAAQQToImQDkuikLRiAJ8gyznG/2eARyRg0\n"
   "Eq4Z2KjHuuR/Bg+VLiUXemo/Gt/Dju/Qw1FpO90EqxCq+GQoyGAtTN44AAAAmCexuYonsb\n"
   "mKAAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBOgiZAOS6KQtGIAn\n"
   "yDLOcb/Z4BHJGDQSrhnYqMe65H8GD5UuJRd6aj8a38OO79DDUWk73QSrEKr4ZCjIYC1M3j\n"
   "gAAAAgMnVFzX3CHCo36mZt5UDt2pXl1DmtN/kIdovmxo2/K/8AAAAA\n"
   "-----END OPENSSH PRIVATE KEY-----\n";
static const char *s_pubEcdsa256 = "ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBOgiZAOS6KQtGIAnyDLOcb/Z4BHJGDQSrhnYqMe65H8GD5UuJRd6aj8a38OO79DDUWk73QSrEKr4ZCjIYC1M3jg=";

static const char *s_keyEncrypted =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAACmFlczI1Ni1jdHIAAAAGYmNyeXB0AAAAGAAAABCPkdBWFF\n"
   "GYbvnGTzb9u/B1AAAAGAAAAAEAAAAzAAAAC3NzaC1lZDI1NTE5AAAAIC5+qFVNQ6QsAXl3\n"
   "17Q90riC2frJSh9ud18KBKFuGtNAAAAAkD82ax3h184h/nQqNaz534aLFrlxpM/tq4mPDJ\n"
   "mDUYe02bh1KzuxoDpqLyJXsMvQBRvL9mVh9uzp6U/ofXYtc2dEA0fNjDZZLjEtFzIWdFfp\n"
   "8WfY5zwKSkiJejaNfCK6nE30a+3+mu+PrrasmhysMefPKc9xAFqrAiF9floN7JaAtlq6Os\n"
   "ml45CPFZ8rLE9WSQ==\n"
   "-----END OPENSSH PRIVATE KEY-----\n";

static const char *s_keyPemRsa =
   "-----BEGIN RSA PRIVATE KEY-----\n"
   "MIICXAIBAAKBgQCcR/kUna8VEDdd8WGMOaU8FkQKO4bvNyZkfNUVstYZmWGnH7I1\n"
   "Z6rRDui5X/DUdQpcyZNs63678pZaKRfu+JRvCOh589DmWQ/5tFbcfoWoCCAZL9Yl\n"
   "km5S+M4zOcq7Fw3Libus9hJtaexQVyDbdJrmncrr8E/0QtINrnk3CL6YVwIDAQAB\n"
   "AoGAETn9wr/+5Jt8/uX0krhn9k+eAGK/ZO73VGxOMK4d24SEtx9PTYO32IOEM6rX\n"
   "qROdqMyva3op5h2/ZQDSmozcD5UbHBNUzeoqkgJSuhY42FxatyZHqbs+0FG1syji\n"
   "J4ZwdQmUcRtWQY2ogp2r1TF+nELoS20fGL7LJtiiukhOI8ECQQDIqpSospyeDLin\n"
   "cTBlfEpVvgecXn4NaeCuwL42/SmCASX11hD8HVL78jWr3aQbw+YUAR/IYvTqdTz5\n"
   "kHAtQ/8bAkEAx2AoQSomnPHQC0/UCZ+rwiZo281FU2K5tmtoBSqKwdfK7W4pKr3u\n"
   "WTVYiUwvyYmvr+Y029vHkg7vweT85fQTdQJATUwQOa+bzvH9i1p5lw46oC60RdLM\n"
   "YaRgTVCntQzuGUsR8VHFdyhgl/akIBEfryZWEWyhbpJX35zLoSK3Y2hm8wJBAJVb\n"
   "68wlj5ezb2+J9gbfnwRrUdKYDH9TTlziSaLVI+3jgxorlJt4fb5GwHTaplIM25ii\n"
   "lKbAFtnMZsCSkwKRvqECQB1dRsa7FBFo/6mre897lwpKhMfNCVGQ+Fn1Alzf9Q+R\n"
   "Eg64aCKf6NMnKjJ730ivF/r8mxGu3M8vFz9ltJ9F6ns=\n"
   "-----END RSA PRIVATE KEY-----\n";

/**
 * Stored key pair as agents receive it
 */
struct StoredKeys
{
   std::string publicKey;
   std::string privateKey;
};

/**
 * Get stored keys of existing SSH key
 */
static StoredKeys GetStoredKeys(uint32_t id)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, 0);
   FindSshKeyById(id, &msg);
   AssertEquals(msg.getFieldAsUInt32(VID_RCC), ERR_SUCCESS);
   char *publicKey = msg.getFieldAsUtf8String(VID_PUBLIC_KEY);
   char *privateKey = msg.getFieldAsUtf8String(VID_PRIVATE_KEY);
   StoredKeys keys;
   keys.publicKey = CHECK_NULL_EX_A(publicKey);
   keys.privateKey = CHECK_NULL_EX_A(privateKey);
   MemFree(publicKey);
   MemFree(privateKey);
   return keys;
}

/**
 * Check that SSH key with given ID does not exist
 */
static bool SshKeyExists(uint32_t id)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, 0);
   FindSshKeyById(id, &msg);
   return msg.getFieldAsUInt32(VID_RCC) == ERR_SUCCESS;
}

/**
 * Get number of configured SSH keys
 */
static size_t GetSshKeyCount()
{
   json_t *keys = GetSshKeysAsJson(false);
   size_t count = json_array_size(keys);
   json_decref(keys);
   return count;
}

/**
 * Find SSH key ID by name
 */
static uint32_t FindSshKeyIdByName(const char *name)
{
   uint32_t id = 0;
   json_t *keys = GetSshKeysAsJson(false);
   size_t i;
   json_t *key;
   json_array_foreach(keys, i, key)
   {
      if (!strcmp(json_object_get_string_utf8(key, "name", ""), name))
         id = json_object_get_uint32(key, "id");
   }
   json_decref(keys);
   return id;
}

/**
 * Create or edit key through NXCP handler. Null private key means the field is absent.
 */
static uint32_t UpdateSshKeyNXCP(uint32_t id, const wchar_t *name, const char *privateKey, const char *publicKey = nullptr)
{
   NXCPMessage msg(CMD_UPDATE_SSH_KEYS, 0);
   msg.setField(VID_SSH_KEY_ID, id);
   msg.setField(VID_NAME, name);
   if (privateKey != nullptr)
      msg.setFieldFromUtf8String(VID_PRIVATE_KEY, privateKey);
   if (publicKey != nullptr)
      msg.setFieldFromUtf8String(VID_PUBLIC_KEY, publicKey);
   return CreateOrEditSshKey(msg);
}

/**
 * Create or edit key through JSON handler. Request object is consumed.
 */
static uint32_t UpdateSshKeyJson(uint32_t id, json_t *request, uint32_t *keyId = nullptr)
{
   uint32_t rcc = CreateOrEditSshKey(id, request, keyId);
   json_decref(request);
   return rcc;
}

/**
 * Build JSON request with name and private key
 */
static json_t *KeyRequest(const char *name, const char *privateKey)
{
   json_t *request = json_object();
   json_object_set_new(request, "name", json_string(name));
   json_object_set_new(request, "privateKey", json_string(privateKey));
   return request;
}

/**
 * Valid import stores derived public key and normalized private key text, client public key is ignored
 */
static void TestImport()
{
   StartTest(_T("SSH key import"));

   std::string expectedPrivateKey(s_keyEd25519, strlen(s_keyEd25519) - 1);
   std::string crlf;
   for(const char *p = s_keyEd25519; *p != 0; p++)
   {
      if (*p == '\n')
         crlf.push_back('\r');
      crlf.push_back(*p);
   }
   crlf = "  \r\n" + crlf + "\r\n  ";

   json_t *request = KeyRequest("test-import-json", crlf.c_str());
   json_object_set_new(request, "publicKey", json_string(s_pubEcdsa256));
   uint32_t id = 0;
   AssertEquals(UpdateSshKeyJson(0, request, &id), RCC_SUCCESS);
   AssertNotEquals(id, 0u);
   StoredKeys keys = GetStoredKeys(id);
   AssertEquals(keys.publicKey.c_str(), (std::string(s_pubEd25519) + " test-import-json").c_str());
   AssertEquals(keys.privateKey.c_str(), expectedPrivateKey.c_str());
   AssertEquals(DeleteSshKey(id, true, nullptr), RCC_SUCCESS);

   AssertEquals(UpdateSshKeyNXCP(0, L"test-import-nxcp", crlf.c_str(), s_pubEcdsa256), RCC_SUCCESS);
   id = FindSshKeyIdByName("test-import-nxcp");
   AssertNotEquals(id, 0u);
   keys = GetStoredKeys(id);
   AssertEquals(keys.publicKey.c_str(), (std::string(s_pubEd25519) + " test-import-nxcp").c_str());
   AssertEquals(keys.privateKey.c_str(), expectedPrivateKey.c_str());
   AssertEquals(DeleteSshKey(id, true, nullptr), RCC_SUCCESS);

   EndTest();
}

/**
 * Invalid private keys and missing private key on create are rejected without creating a key
 */
static void TestImportRejected()
{
   StartTest(_T("SSH key import - invalid keys"));

   size_t count = GetSshKeyCount();
   const char *invalidKeys[] = { "junk", "", s_keyEncrypted, s_keyPemRsa };
   for(size_t i = 0; i < sizeof(invalidKeys) / sizeof(invalidKeys[0]); i++)
   {
      AssertEquals(UpdateSshKeyNXCP(0, L"test-invalid", invalidKeys[i]), RCC_INVALID_ARGUMENT);
      AssertEquals(UpdateSshKeyJson(0, KeyRequest("test-invalid", invalidKeys[i])), RCC_INVALID_ARGUMENT);
   }

   AssertEquals(UpdateSshKeyNXCP(0, L"test-invalid", nullptr), RCC_INVALID_ARGUMENT);
   json_t *request = json_object();
   json_object_set_new(request, "name", json_string("test-invalid"));
   json_object_set_new(request, "publicKey", json_string(s_pubEd25519));
   AssertEquals(UpdateSshKeyJson(0, request), RCC_INVALID_ARGUMENT);

   request = json_object();
   json_object_set_new(request, "name", json_string("test-invalid"));
   json_object_set_new(request, "privateKey", json_null());
   AssertEquals(UpdateSshKeyJson(0, request), RCC_INVALID_ARGUMENT);

   request = json_object();
   json_object_set_new(request, "name", json_string("test-invalid"));
   json_object_set_new(request, "privateKey", json_integer(1));
   AssertEquals(UpdateSshKeyJson(0, request), RCC_INVALID_ARGUMENT);

   AssertTrue(GetSshKeyCount() == count);
   EndTest();
}

/**
 * Name rules on import and generation
 */
static void TestNameRules()
{
   StartTest(_T("SSH key name rules"));

   size_t count = GetSshKeyCount();
   std::wstring longName(255, L'n');
   std::wstring oversizedName(4000, L'n');
   const wchar_t *invalidNames[] = { L"", L"  \t ", longName.c_str(), oversizedName.c_str() };
   for(size_t i = 0; i < sizeof(invalidNames) / sizeof(invalidNames[0]); i++)
   {
      AssertEquals(UpdateSshKeyNXCP(0, invalidNames[i], s_keyEd25519), RCC_INVALID_OBJECT_NAME);
      char *utf8name = UTF8StringFromWideString(invalidNames[i]);
      AssertEquals(UpdateSshKeyJson(0, KeyRequest(utf8name, s_keyEd25519)), RCC_INVALID_OBJECT_NAME);
      MemFree(utf8name);
      AssertEquals(GenerateSshKey(invalidNames[i]), RCC_INVALID_OBJECT_NAME);
   }

   json_t *request = json_object();
   json_object_set_new(request, "privateKey", json_string(s_keyEd25519));
   AssertEquals(UpdateSshKeyJson(0, request), RCC_INVALID_OBJECT_NAME);

   request = json_object();
   json_object_set_new(request, "name", json_integer(1));
   json_object_set_new(request, "privateKey", json_string(s_keyEd25519));
   AssertEquals(UpdateSshKeyJson(0, request), RCC_INVALID_OBJECT_NAME);

   AssertTrue(GetSshKeyCount() == count);

   std::wstring maxName(254, L'n');
   maxName[0] = L'a';
   AssertEquals(UpdateSshKeyNXCP(0, maxName.c_str(), s_keyEd25519), RCC_SUCCESS);
   std::string maxNameUtf8(254, 'n');
   maxNameUtf8[0] = 'a';
   uint32_t id = FindSshKeyIdByName(maxNameUtf8.c_str());
   AssertNotEquals(id, 0u);
   AssertEquals(GetStoredKeys(id).publicKey.c_str(), (std::string(s_pubEd25519) + " " + maxNameUtf8).c_str());
   AssertEquals(DeleteSshKey(id, true, nullptr), RCC_SUCCESS);

   maxName[0] = L'b';
   maxNameUtf8[0] = 'b';
   AssertEquals(GenerateSshKey(maxName.c_str(), &id), RCC_SUCCESS);
   AssertEquals(FindSshKeyIdByName(maxNameUtf8.c_str()), id);
   AssertEquals(DeleteSshKey(id, true, nullptr), RCC_SUCCESS);

   EndTest();
}

/**
 * Rename without private key keeps stored keys, also for server-generated (PEM RSA) keys
 */
static void TestRename()
{
   StartTest(_T("SSH key rename"));

   uint32_t id;
   AssertEquals(GenerateSshKey(L"test-generated", &id), RCC_SUCCESS);
   StoredKeys before = GetStoredKeys(id);
   AssertTrue(before.privateKey.compare(0, 31, "-----BEGIN RSA PRIVATE KEY-----") == 0);

   AssertEquals(UpdateSshKeyNXCP(id, L"test-generated-renamed", nullptr), RCC_SUCCESS);
   StoredKeys after = GetStoredKeys(id);
   AssertEquals(after.publicKey.c_str(), before.publicKey.c_str());
   AssertEquals(after.privateKey.c_str(), before.privateKey.c_str());
   AssertEquals(FindSshKeyIdByName("test-generated-renamed"), id);

   json_t *request = json_object();
   json_object_set_new(request, "name", json_string("test-generated-json"));
   json_object_set_new(request, "publicKey", json_string(s_pubEd25519));
   AssertEquals(UpdateSshKeyJson(id, request), RCC_SUCCESS);
   after = GetStoredKeys(id);
   AssertEquals(after.publicKey.c_str(), before.publicKey.c_str());
   AssertEquals(after.privateKey.c_str(), before.privateKey.c_str());
   AssertEquals(FindSshKeyIdByName("test-generated-json"), id);

   AssertEquals(DeleteSshKey(id, true, nullptr), RCC_SUCCESS);
   EndTest();
}

/**
 * Replacing private key on edit; invalid replacement keeps old keys
 */
static void TestReplace()
{
   StartTest(_T("SSH key replace"));

   uint32_t id = 0;
   AssertEquals(UpdateSshKeyJson(0, KeyRequest("test-replace", s_keyEd25519), &id), RCC_SUCCESS);
   StoredKeys before = GetStoredKeys(id);

   const char *invalidKeys[] = { "junk", "", s_keyEncrypted, s_keyPemRsa };
   for(size_t i = 0; i < sizeof(invalidKeys) / sizeof(invalidKeys[0]); i++)
   {
      AssertEquals(UpdateSshKeyNXCP(id, L"test-replace", invalidKeys[i]), RCC_INVALID_ARGUMENT);
      AssertEquals(UpdateSshKeyJson(id, KeyRequest("test-replace", invalidKeys[i])), RCC_INVALID_ARGUMENT);
   }

   json_t *request = json_object();
   json_object_set_new(request, "name", json_string("test-replace"));
   json_object_set_new(request, "privateKey", json_null());
   AssertEquals(UpdateSshKeyJson(id, request), RCC_INVALID_ARGUMENT);

   request = json_object();
   json_object_set_new(request, "name", json_string("test-replace"));
   json_object_set_new(request, "privateKey", json_integer(1));
   AssertEquals(UpdateSshKeyJson(id, request), RCC_INVALID_ARGUMENT);

   AssertEquals(UpdateSshKeyNXCP(id, L"", s_keyEcdsa256), RCC_INVALID_OBJECT_NAME);

   StoredKeys after = GetStoredKeys(id);
   AssertEquals(after.publicKey.c_str(), before.publicKey.c_str());
   AssertEquals(after.privateKey.c_str(), before.privateKey.c_str());

   AssertEquals(UpdateSshKeyNXCP(id, L"test-replace-nxcp", s_keyEcdsa256), RCC_SUCCESS);
   after = GetStoredKeys(id);
   AssertEquals(after.publicKey.c_str(), (std::string(s_pubEcdsa256) + " test-replace-nxcp").c_str());
   AssertEquals(after.privateKey.c_str(), std::string(s_keyEcdsa256, strlen(s_keyEcdsa256) - 1).c_str());

   AssertEquals(UpdateSshKeyJson(id, KeyRequest("test-replace-json", s_keyEd25519)), RCC_SUCCESS);
   after = GetStoredKeys(id);
   AssertEquals(after.publicKey.c_str(), (std::string(s_pubEd25519) + " test-replace-json").c_str());

   AssertEquals(DeleteSshKey(id, true, nullptr), RCC_SUCCESS);
   EndTest();
}

/**
 * Edit of deleted key is rejected and does not recreate it
 */
static void TestEditDeleted()
{
   StartTest(_T("SSH key edit after delete"));

   uint32_t id = 0;
   AssertEquals(UpdateSshKeyJson(0, KeyRequest("test-deleted", s_keyEd25519), &id), RCC_SUCCESS);
   AssertEquals(DeleteSshKey(id, true, nullptr), RCC_SUCCESS);

   AssertEquals(UpdateSshKeyNXCP(id, L"test-deleted", nullptr), RCC_INVALID_SSH_KEY_ID);
   AssertEquals(UpdateSshKeyNXCP(id, L"test-deleted", s_keyEd25519), RCC_INVALID_SSH_KEY_ID);
   json_t *request = json_object();
   json_object_set_new(request, "name", json_string("test-deleted"));
   AssertEquals(UpdateSshKeyJson(id, request), RCC_INVALID_SSH_KEY_ID);
   AssertEquals(UpdateSshKeyJson(id, KeyRequest("test-deleted", s_keyEd25519)), RCC_INVALID_SSH_KEY_ID);
   AssertEquals(UpdateSshKeyJson(id, json_object()), RCC_INVALID_SSH_KEY_ID);
   request = json_object();
   json_object_set_new(request, "name", json_integer(5));
   AssertEquals(UpdateSshKeyJson(id, request), RCC_INVALID_SSH_KEY_ID);
   request = json_object();
   json_object_set_new(request, "name", json_string("test-deleted"));
   json_object_set_new(request, "privateKey", json_null());
   AssertEquals(UpdateSshKeyJson(id, request), RCC_INVALID_SSH_KEY_ID);
   AssertFalse(SshKeyExists(id));

   EndTest();
}

/**
 * Entry point
 */
void TestSshKeys()
{
   TestImport();
   TestImportRejected();
   TestNameRules();
   TestRename();
   TestReplace();
   TestEditDeleted();
}
