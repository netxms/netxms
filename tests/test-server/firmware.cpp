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
** File: firmware.cpp
**
** Tests for node firmware information: change detection, SYS_FIRMWARE_CHANGED
** events, NXSL access and database persistence.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Add firmware entry to the list
 */
static void AddFirmware(StructArray<FirmwareInfo> *list, FirmwareRole role, const wchar_t *name, const wchar_t *platform, const wchar_t *version, const wchar_t *raw)
{
   FirmwareInfo *fw = list->addPlaceholder();
   memset(fw, 0, sizeof(FirmwareInfo));
   fw->role = role;
   fw->componentIndex = -1;
   wcslcpy(fw->name, name, 64);
   wcslcpy(fw->platform, platform, 64);
   wcslcpy(fw->version, version, 64);
   wcslcpy(fw->raw, raw, 256);
}

/**
 * Get messages of firmware change events logged for given node, in order of generation
 */
static void ReadFirmwareEvents(uint32_t nodeId, StringList *messages)
{
   wchar_t query[256];
   nx_swprintf(query, 256, L"SELECT event_message FROM event_log WHERE event_code=%d AND event_source=%u ORDER BY event_id", EVENT_FIRMWARE_CHANGED, nodeId);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);
   AssertNotNull(hResult);
   messages->clear();
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
      messages->addPreallocated(DBGetField(hResult, i, 0, nullptr, 0));
   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Wait until event log has expected number of firmware change events for given node, then check that
 * no more events arrive
 */
static void WaitForFirmwareEvents(uint32_t nodeId, int expectedCount, StringList *messages)
{
   for(int i = 0; i < 100; i++)
   {
      ReadFirmwareEvents(nodeId, messages);
      if (messages->size() >= expectedCount)
         break;
      ThreadSleepMs(100);
   }
   AssertEquals(messages->size(), expectedCount);
   ThreadSleepMs(500);
   ReadFirmwareEvents(nodeId, messages);
   AssertEquals(messages->size(), expectedCount);
}

/**
 * Run script on given object and return its result as string
 */
static String RunScript(const wchar_t *source, const shared_ptr<NetObj>& object)
{
   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(source, new NXSL_ServerEnv(), &diag);
   if (vm == nullptr)
      WriteToTerminalEx(L"\n   Compilation error: %s\n", diag.errorText.cstr());
   AssertNotNull(vm);
   SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
   bool success = vm->run();
   if (!success)
      WriteToTerminalEx(L"\n   Script error: %s\n", vm->getErrorText());
   AssertTrue(success);
   String result(vm->getResult()->getValueAsCString());
   delete vm;
   return result;
}

/**
 * Get number of firmware records stored in database for given object
 */
static int GetStoredFirmwareCount(uint32_t objectId)
{
   wchar_t query[256];
   nx_swprintf(query, 256, L"SELECT count(*) FROM object_firmware WHERE object_id=%u", objectId);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);
   AssertNotNull(hResult);
   int count = DBGetFieldLong(hResult, 0, 0);
   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);
   return count;
}

/**
 * Save object to database
 */
static void SaveObject(const shared_ptr<Node>& node)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   AssertTrue(DBBegin(hdb));
   bool success = node->saveToDatabase(hdb);
   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);
   DBConnectionPoolReleaseConnection(hdb);
   AssertTrue(success);
}

/**
 * Test node firmware information
 */
void TestNodeFirmware()
{
   NewNodeData data(InetAddress::parse(L"10.255.254.1"));
   shared_ptr<Node> node = make_shared<Node>(&data, 0);
   node->unpublish();   // Keep poll manager away from this node
   NetObjInsert(node, true, false);
   StringList messages;

   StartTest(L"Node firmware: first population generates no events");
   AssertEquals(node->getFirmwareInformation().size(), 0);
   FirmwareInfo mainFirmware;
   AssertFalse(node->getMainFirmware(&mainFirmware));

   StructArray<FirmwareInfo> firmware;
   AddFirmware(&firmware, FirmwareRole::BOOTLOADER, L"RouterBOOT", L"routerboot", L"7.12.1", L"7.12.1");
   AddFirmware(&firmware, FirmwareRole::MAIN, L"RouterOS", L"routeros", L"7.16.2", L"7.16.2 (stable)");
   AssertTrue(node->updateFirmwareInformation(firmware));
   AssertEquals(node->getFirmwareInformation().size(), 2);
   AssertTrue(node->getMainFirmware(&mainFirmware));
   AssertEquals(mainFirmware.platform, L"routeros");
   AssertEquals(mainFirmware.version, L"7.16.2");
   AssertEquals(mainFirmware.raw, L"7.16.2 (stable)");

   // Same information again is not a change
   AssertFalse(node->updateFirmwareInformation(firmware));
   WaitForFirmwareEvents(node->getId(), 0, &messages);
   EndTest();

   StartTest(L"Node firmware: version change");
   StructArray<FirmwareInfo> upgraded;
   AddFirmware(&upgraded, FirmwareRole::BOOTLOADER, L"RouterBOOT", L"routerboot", L"7.12.1", L"7.12.1");
   AddFirmware(&upgraded, FirmwareRole::MAIN, L"RouterOS", L"routeros", L"7.22.1", L"7.22.1 (stable)");
   AssertTrue(node->updateFirmwareInformation(upgraded));
   WaitForFirmwareEvents(node->getId(), 1, &messages);
   AssertEquals(messages.get(0), L"Firmware with role \"main\" changed (old version \"7.16.2\", new version \"7.22.1\")");
   AssertTrue(node->getMainFirmware(&mainFirmware));
   AssertEquals(mainFirmware.version, L"7.22.1");
   EndTest();

   StartTest(L"Node firmware: change of raw version only");
   StructArray<FirmwareInfo> otherChannel;
   AddFirmware(&otherChannel, FirmwareRole::BOOTLOADER, L"RouterBOOT", L"routerboot", L"7.12.1", L"7.12.1");
   AddFirmware(&otherChannel, FirmwareRole::MAIN, L"RouterOS", L"routeros", L"7.22.1", L"7.22.1 (long-term)");
   AssertTrue(node->updateFirmwareInformation(otherChannel));
   AssertTrue(node->getMainFirmware(&mainFirmware));
   AssertEquals(mainFirmware.raw, L"7.22.1 (long-term)");
   WaitForFirmwareEvents(node->getId(), 1, &messages);
   EndTest();

   StartTest(L"Node firmware: firmware added and removed");
   // Order of entries is changed as well, which must not generate events
   StructArray<FirmwareInfo> patched;
   AddFirmware(&patched, FirmwareRole::MAIN, L"RouterOS", L"routeros", L"7.22.1", L"7.22.1 (long-term)");
   AddFirmware(&patched, FirmwareRole::PATCH, L"", L"", L"P01", L"P01");   // Firmware without product name and platform
   AddFirmware(&patched, FirmwareRole::BOOTLOADER, L"RouterBOOT", L"routerboot", L"7.12.1", L"7.12.1");
   AssertTrue(node->updateFirmwareInformation(patched));
   WaitForFirmwareEvents(node->getId(), 2, &messages);
   AssertEquals(messages.get(1), L"Firmware with role \"patch\" changed (old version \"\", new version \"P01\")");

   AssertTrue(node->updateFirmwareInformation(otherChannel));
   WaitForFirmwareEvents(node->getId(), 3, &messages);
   AssertEquals(messages.get(2), L"Firmware with role \"patch\" changed (old version \"P01\", new version \"\")");
   EndTest();

   StartTest(L"Node firmware: several entries of the same kind");
   // Two patches installed, then one of them replaced: unchanged patch is paired with itself,
   // so there is single event for the replaced one
   StructArray<FirmwareInfo> twoPatches;
   AddFirmware(&twoPatches, FirmwareRole::MAIN, L"Comware", L"comware", L"7.1.070", L"7.1.070");
   AddFirmware(&twoPatches, FirmwareRole::PATCH, L"Patch", L"comware-patch", L"P01", L"P01");
   AddFirmware(&twoPatches, FirmwareRole::PATCH, L"Patch", L"comware-patch", L"P02", L"P02");
   NewNodeData data2(InetAddress::parse(L"10.255.254.2"));
   shared_ptr<Node> node2 = make_shared<Node>(&data2, 0);
   node2->unpublish();
   NetObjInsert(node2, true, false);
   AssertTrue(node2->updateFirmwareInformation(twoPatches));

   StructArray<FirmwareInfo> replacedPatch;
   AddFirmware(&replacedPatch, FirmwareRole::MAIN, L"Comware", L"comware", L"7.1.070", L"7.1.070");
   AddFirmware(&replacedPatch, FirmwareRole::PATCH, L"Patch", L"comware-patch", L"P02", L"P02");
   AddFirmware(&replacedPatch, FirmwareRole::PATCH, L"Patch", L"comware-patch", L"P03", L"P03");
   AssertTrue(node2->updateFirmwareInformation(replacedPatch));
   WaitForFirmwareEvents(node2->getId(), 1, &messages);
   AssertEquals(messages.get(0), L"Firmware with role \"patch\" changed (old version \"P01\", new version \"P03\")");

   // Empty list removes everything
   AssertTrue(node2->updateFirmwareInformation(StructArray<FirmwareInfo>()));
   AssertEquals(node2->getFirmwareInformation().size(), 0);
   WaitForFirmwareEvents(node2->getId(), 4, &messages);
   EndTest();

   StartTest(L"Node firmware: NXSL attributes");
   AssertEquals(RunScript(L"return $node.firmwareVersion;", node), L"7.22.1");
   AssertEquals(RunScript(L"return $node.firmwarePlatform;", node), L"routeros");
   AssertEquals(RunScript(L"return $node.firmware.size;", node), L"2");
   AssertEquals(RunScript(
         L"f = $node.firmware[1];\n"
         L"return classof(f) .. \"|\" .. f.role .. \"|\" .. f.componentIndex .. \"|\" .. f.name .. \"|\" .. f.platform .. \"|\" .. f.version .. \"|\" .. f.rawVersion;", node),
      L"Firmware|main|-1|RouterOS|routeros|7.22.1|7.22.1 (long-term)");
   AssertEquals(RunScript(L"return $node.firmware[0].role;", node), L"bootloader");
   AssertEquals(RunScript(L"return ($node.firmwareVersion == null) and ($node.firmwarePlatform == null) and ($node.firmware.size == 0);", node2), L"true");
   EndTest();

   StartTest(L"Node firmware: NXCP message");
   NXCPMessage msg(CMD_OBJECT, 1);
   node->fillMessage(&msg, 0, true, node->getAccessSnapshot(0));
   AssertEquals(msg.getFieldAsInt32(VID_FIRMWARE_COUNT), 2);
   wchar_t buffer[256];
   AssertEquals(msg.getFieldAsInt16(VID_FIRMWARE_LIST_BASE), static_cast<int>(FirmwareRole::BOOTLOADER));
   AssertEquals(msg.getFieldAsInt32(VID_FIRMWARE_LIST_BASE + 1), -1);
   AssertEquals(msg.getFieldAsString(VID_FIRMWARE_LIST_BASE + 2, buffer, 256), L"RouterBOOT");
   AssertEquals(msg.getFieldAsInt16(VID_FIRMWARE_LIST_BASE + 10), static_cast<int>(FirmwareRole::MAIN));
   AssertEquals(msg.getFieldAsString(VID_FIRMWARE_LIST_BASE + 12, buffer, 256), L"RouterOS");
   AssertEquals(msg.getFieldAsString(VID_FIRMWARE_LIST_BASE + 13, buffer, 256), L"routeros");
   AssertEquals(msg.getFieldAsString(VID_FIRMWARE_LIST_BASE + 14, buffer, 256), L"7.22.1");
   AssertEquals(msg.getFieldAsString(VID_FIRMWARE_LIST_BASE + 15, buffer, 256), L"7.22.1 (long-term)");

   NXCPMessage emptyMsg(CMD_OBJECT, 2);
   node2->fillMessage(&emptyMsg, 0, true, node2->getAccessSnapshot(0));
   AssertTrue(emptyMsg.isFieldExist(VID_FIRMWARE_COUNT));
   AssertEquals(emptyMsg.getFieldAsInt32(VID_FIRMWARE_COUNT), 0);
   EndTest();

   StartTest(L"Node firmware: JSON");
   json_t *json = node->toJson(0);
   json_t *list = json_object_get(json, "firmware");
   AssertTrue(json_is_array(list));
   AssertEquals(static_cast<int>(json_array_size(list)), 2);
   json_t *entry = json_array_get(list, 1);
   AssertEquals(json_object_get_string_utf8(entry, "role", ""), "main");
   AssertEquals(json_object_get_int32(entry, "componentIndex", 0), -1);
   AssertEquals(json_object_get_string_utf8(entry, "name", ""), "RouterOS");
   AssertEquals(json_object_get_string_utf8(entry, "platform", ""), "routeros");
   AssertEquals(json_object_get_string_utf8(entry, "version", ""), "7.22.1");
   AssertEquals(json_object_get_string_utf8(entry, "rawVersion", ""), "7.22.1 (long-term)");
   AssertEquals(json_object_get_string_utf8(json_array_get(list, 0), "role", ""), "bootloader");
   json_decref(json);

   json = node2->toJson(0);
   list = json_object_get(json, "firmware");
   AssertTrue(json_is_array(list));
   AssertEquals(static_cast<int>(json_array_size(list)), 0);
   json_decref(json);
   EndTest();

   StartTest(L"Node firmware: internal table");
   shared_ptr<Table> table;
   AssertEquals(static_cast<int>(node->getInternalTable(L"System.Firmware", &table)), static_cast<int>(DCE_SUCCESS));
   AssertNotNull(table.get());
   AssertEquals(table->getNumColumns(), 6);
   AssertEquals(table->getNumRows(), 2);
   AssertEquals(table->getColumnIndex(L"RAW_VERSION"), 5);
   AssertEquals(table->getAsInt(1, 0), -1);
   AssertEquals(table->getAsString(1, 1), L"main");
   AssertEquals(table->getAsString(1, 2), L"RouterOS");
   AssertEquals(table->getAsString(1, 3), L"routeros");
   AssertEquals(table->getAsString(1, 4), L"7.22.1");
   AssertEquals(table->getAsString(1, 5), L"7.22.1 (long-term)");
   AssertEquals(table->getAsString(0, 1), L"bootloader");

   table.reset();
   AssertEquals(static_cast<int>(node2->getInternalTable(L"System.Firmware", &table)), static_cast<int>(DCE_SUCCESS));
   AssertNotNull(table.get());
   AssertEquals(table->getNumRows(), 0);
   EndTest();

   StartTest(L"Node firmware: database persistence");
   SaveObject(node);
   AssertEquals(GetStoredFirmwareCount(node->getId()), 2);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT preparedStatements[LSI_MAX_VALUE];
   memset(preparedStatements, 0, sizeof(preparedStatements));
   shared_ptr<Node> loaded = make_shared<Node>();
   AssertTrue(loaded->loadFromDatabase(hdb, node->getId(), preparedStatements));
   for(int i = 0; i < LSI_MAX_VALUE; i++)
      if (preparedStatements[i] != nullptr)
         DBFreeStatement(preparedStatements[i]);
   DBConnectionPoolReleaseConnection(hdb);

   StructArray<FirmwareInfo> loadedFirmware = loaded->getFirmwareInformation();
   AssertEquals(loadedFirmware.size(), 2);
   AssertTrue(loadedFirmware.get(0)->role == FirmwareRole::BOOTLOADER);
   AssertEquals(loadedFirmware.get(0)->componentIndex, -1);
   AssertEquals(loadedFirmware.get(0)->name, L"RouterBOOT");
   AssertEquals(loadedFirmware.get(0)->platform, L"routerboot");
   AssertEquals(loadedFirmware.get(0)->version, L"7.12.1");
   AssertEquals(loadedFirmware.get(0)->raw, L"7.12.1");
   AssertTrue(loadedFirmware.get(1)->role == FirmwareRole::MAIN);
   AssertEquals(loadedFirmware.get(1)->name, L"RouterOS");
   AssertEquals(loadedFirmware.get(1)->platform, L"routeros");
   AssertEquals(loadedFirmware.get(1)->version, L"7.22.1");
   AssertEquals(loadedFirmware.get(1)->raw, L"7.22.1 (long-term)");

   // Information loaded from database is not different from what device reports
   AssertFalse(loaded->updateFirmwareInformation(otherChannel));
   loaded.reset();

   // Removed firmware is removed from database
   AssertTrue(node->updateFirmwareInformation(StructArray<FirmwareInfo>()));
   SaveObject(node);
   AssertEquals(GetStoredFirmwareCount(node->getId()), 0);
   EndTest();

   node->deleteObject();
   node2->deleteObject();
}
