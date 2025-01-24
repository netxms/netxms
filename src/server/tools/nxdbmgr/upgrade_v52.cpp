/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022-2025 Raden Solutions
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
** File: upgrade_v52.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

#include <pugixml.h>
#include <jansson.h>
#include <string>

/**
 * Convert string to int if applicable
 */
static void ConvertValueXmlToJson(json_t *object, const char *name, const char *text)
{
   char *eptr;
   int32_t val = strtol(text, &eptr, 10);
   if (*eptr == 0 && strlen(text) > 0)
   {
      json_object_set_new(object, name, json_integer(val));
   }
   else
   {
      json_object_set_new(object, name, json_string(text));
   }
}

/**
 * Function to convert an XML node to a JSON object
 */
static json_t* XmlNodeToJson(const pugi::xml_node &node)
{
   json_t *jsonObject = json_object();

   for (pugi::xml_node child : node.children())
   {
      std::string childName = child.name();

      if (childName.empty() || child.first_child().empty())
      {
         continue;
      }

      //special processing of exceptional elements
      if (!strcmp(childName.c_str(), "TextBox") || !strcmp(childName.c_str(), "DCIList"))
      {
         pugi::xml_document xmlDoc;
         if (!xmlDoc.load_buffer(child.child_value(), strlen(child.child_value())))
         {
            json_object_set_new(jsonObject, child.name(), json_string(child.child_value()));
            continue; //ignore if failed to parse
         }

         pugi::xml_node xml = xmlDoc.first_child();
         if (!strcmp(childName.c_str(), "DCIList"))
         {
            if ((xml.child("config") != nullptr))
            {
               xml = xml.child("config");
            }
            if ((xml.child("dciImageConfiguration") != nullptr))
            {
               xml = xml.child("dciImageConfiguration");
            }
         }
         json_t *converted = XmlNodeToJson(xml);
         char *jsonText = json_dumps(converted, JSON_COMPACT);
         json_object_set_new(jsonObject, child.name(), json_string(jsonText));
         MemFree(jsonText);
         json_decref(converted);
      }
      else if ((child.attribute("length") != nullptr) || child.attribute("lenght") != nullptr)
      {
         if (strlen(child.child_value()) > 0)
         {
            String s(child.child_value(), "utf8");
            unique_ptr<StringList> values(s.split(_T(","), true));
            IntegerArray<int32_t> arr;
            for (int i = 0; i < values->size(); i++)
            {
               TCHAR *eptr;
               int32_t val = _tcstol(values->get(i), &eptr, 10);
               if (*eptr == 0)
               {
                  arr.add(val);
               }
            }
            json_object_set_new(jsonObject, child.name(), arr.toJson());
         }
         else
         {
            json_t *array = json_array();
            for (pugi::xml_node grandChild : child.children())
            {
               json_array_append_new(array, XmlNodeToJson(grandChild));
            }
            json_object_set_new(jsonObject, childName.c_str(), array);
         }
      }
      else if (child.attribute("class") != nullptr)
      {
         json_t *array = json_array();
         for (pugi::xml_node grandChild : child.children())
         {
            if (!strcmp(grandChild.name(), "long"))
            {
               const char *text = grandChild.child_value();

               char *eptr;
               int32_t val = strtol(text, &eptr, 10);
               if (*eptr == 0 && strlen(text) > 0)
               {
                  json_array_append_new(array, json_integer(val));
               }
            }
            else
               json_array_append_new(array, XmlNodeToJson(grandChild));
         }
         json_object_set_new(jsonObject, childName.c_str(), array);
      }
      else if (child.first_child() == child.last_child())
      {
         ConvertValueXmlToJson(jsonObject, childName.c_str(), child.child_value());
      }
      else
      {
         json_t *object = XmlNodeToJson(child);
         for (pugi::xml_attribute attr : child.attributes())
         {
            if (strcmp(attr.name(), "class") == 0)
               continue;

            ConvertValueXmlToJson(object, attr.name(), attr.value());
         }
         json_object_set_new(jsonObject, childName.c_str(), object);
      }
   }

   for (pugi::xml_attribute attr : node.attributes())
   {
      ConvertValueXmlToJson(jsonObject, attr.name(), attr.value());
   }

   return jsonObject;
}

/**
 * Convert network map XML configuration to JSON
 */
static bool ConvertXmlToJson(const TCHAR *reqest, const TCHAR *update, const char *topElement)
{
   DB_RESULT hResult = SQLSelect(reqest);
     if (hResult != nullptr)
     {
        int count = DBGetNumRows(hResult);
        DB_STATEMENT hStmt = DBPrepare(g_dbHandle, update, count > 1);
        if (hStmt != nullptr)
        {
           for (int i = 0; i < count; i++)
           {
              char *xmlText = DBGetFieldUTF8(hResult, i, 2, nullptr, 0);
              pugi::xml_document xml;
              if (!xml.load_buffer(xmlText, strlen(xmlText)))
              {
                 MemFree(xmlText);
                 continue; //ignore filed
              }

              json_t *result = XmlNodeToJson(xml.child(topElement));
              char *jsonText = json_dumps(result, JSON_COMPACT);
              MemFree(jsonText);

              MemFree(xmlText);

              DBBind(hStmt, 1, DB_SQLTYPE_TEXT, result, DB_BIND_DYNAMIC);
              DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 0));
              DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, DBGetFieldULong(hResult, i, 1));
              if (!SQLExecute(hStmt) && !g_ignoreErrors)
              {
                 DBFreeResult(hResult);
                 DBFreeStatement(hStmt);
                 return false;
              }
           }
           DBFreeStatement(hStmt);
        }
        else if (!g_ignoreErrors)
        {
           DBFreeResult(hResult);
           return false;
        }

        DBFreeResult(hResult);
     }
     return true;
}

/**
 * Upgrade from 52.6 to 52.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(ConvertXmlToJson(L"SELECT map_id,link_id,element_data FROM network_map_links", L"UPDATE network_map_links SET element_data=? WHERE map_id=? AND link_id=?", "config"));
   CHK_EXEC(ConvertXmlToJson(L"SELECT map_id,element_id,element_data FROM network_map_elements", L"UPDATE network_map_elements SET element_data=? WHERE map_id=? AND element_id=?", "element"));
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 52.5 to 52.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(CreateTable(
         L"CREATE TABLE package_deployment_jobs ("
         L"   id integer not null,"
         L"   pkg_id integer not null,"
         L"   node_id integer not null,"
         L"   user_id integer not null,"
         L"   creation_time integer not null,"
         L"   execution_time integer not null,"
         L"   completion_time integer not null,"
         L"   status integer not null,"
         L"   failure_reason varchar(255) null,"
         L"   PRIMARY KEY(id))"));

   if (g_dbSyntax == DB_SYNTAX_TSDB)
   {
      CHK_EXEC(CreateTable(
            L"CREATE TABLE package_deployment_log ("
            L"   job_id integer not null,"
            L"   execution_time timestamptz not null,"
            L"   completion_time timestamptz not null,"
            L"   node_id integer not null,"
            L"   user_id integer not null,"
            L"   status integer not null,"
            L"   failure_reason varchar(255) null,"
            L"   pkg_id integer not null,"
            L"   pkg_type varchar(15) null,"
            L"   pkg_name varchar(63) null,"
            L"   version varchar(31) null,"
            L"   platform varchar(63) null,"
            L"   pkg_file varchar(255) null,"
            L"   command varchar(255) null,"
            L"   description varchar(255) null,"
            L"   PRIMARY KEY(job_id,execution_time))"));
   }
   else
   {
      CHK_EXEC(CreateTable(
            L"CREATE TABLE package_deployment_log ("
            L"   job_id integer not null,"
            L"   execution_time integer not null,"
            L"   completion_time integer not null,"
            L"   node_id integer not null,"
            L"   user_id integer not null,"
            L"   status integer not null,"
            L"   failure_reason varchar(255) null,"
            L"   pkg_id integer not null,"
            L"   pkg_type varchar(15) null,"
            L"   pkg_name varchar(63) null,"
            L"   version varchar(31) null,"
            L"   platform varchar(63) null,"
            L"   pkg_file varchar(255) null,"
            L"   command varchar(255) null,"
            L"   description varchar(255) null,"
            L"   PRIMARY KEY(job_id))"));
      CHK_EXEC(SQLQuery(L"SELECT create_hypertable('package_deployment_log', 'execution_time', chunk_time_interval => interval '86400 seconds')"));
   }

   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_pkglog_exec_time ON package_deployment_log(execution_time)"));
   CHK_EXEC(SQLQuery(L"CREATE INDEX idx_pkglog_node ON package_deployment_log(node_id)"));

   CHK_EXEC(SQLQuery(L"DELETE FROM config WHERE var_name='Agent.Upgrade.NumberOfThreads'"));
   CHK_EXEC(SQLQuery(L"UPDATE config SET description='Retention time in days for logged SNMP traps. All SNMP trap records older than specified will be deleted by housekeeping process.' WHERE var_name='SNMP.Traps.LogRetentionTime'"));

   CHK_EXEC(CreateConfigParam(L"PackageDeployment.JobRetentionTime",
            L"7",
            L"Retention time in days for completed package deployment jobs. All completed jobs older than specified will be deleted by housekeeping process.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"PackageDeployment.LogRetentionTime",
            L"90",
            L"Retention time in days for package deployment log. All records older than specified will be deleted by housekeeping process.",
            L"days", 'I', true, false, false, false));
   CHK_EXEC(CreateConfigParam(L"PackageDeployment.MaxThreads",
            L"25",
            L"Maximum number of threads used for package deployment.",
            L"threads", 'I', true, true, false, false));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 52.4 to 52.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(SQLQuery(L"ALTER TABLE network_maps ADD display_priority integer"));
   CHK_EXEC(SQLQuery(L"UPDATE network_maps SET display_priority=0"));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, L"network_maps", L"display_priority"));
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 52.3 to 52.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(DBResizeColumn(g_dbHandle, L"items", L"system_tag", 63, true));
   CHK_EXEC(DBResizeColumn(g_dbHandle, L"dc_tables", L"system_tag", 63, true));
   CHK_EXEC(SQLQuery(L"ALTER TABLE items ADD user_tag varchar(63)"));
   CHK_EXEC(SQLQuery(L"ALTER TABLE dc_tables ADD user_tag varchar(63)"));
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 52.2 to 52.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(51) < 24)
   {
      CHK_EXEC(CreateConfigParam(L"Objects.Nodes.ConfigurationPoll.AlwaysCheckSNMP",
               L"1",
               L"Always check possible SNMP credentials during configuration poll, even if node is marked as unreachable via SNMP.",
               nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(51, 24));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 52.1 to 52.2
 */
static bool H_UpgradeFromV1()
{
   if (GetSchemaLevelForMajorVersion(51) < 23)
   {
      CHK_EXEC(CreateConfigParam(L"Client.MinVersion",
            L"",
            L"Minimal client version allowed for connection to this server.",
            nullptr, 'S', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(51, 23));
   }
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 52.0 to 52.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(SQLQuery(L"INSERT INTO script_library (guid,script_id,script_name,script_code) "
         L"VALUES ('b322c142-fdd5-433f-a820-05b2aa3daa39',27,'Hook::RegisterForConfigurationBackup','/* Available global variables:\r\n *  $node - node to be tested (object of ''Node'' class)\r\n *\r\n * Expected return value:\r\n *  true/false - boolean - whether this node should be registered for configuration backup\r\n */\r\nreturn $node.isSNMP;\r\n')"));

   CHK_EXEC(SetMinorSchemaVersion(1));
   return true;
}

/**
 * Upgrade map
 */
static struct
{
   int version;
   int nextMajor;
   int nextMinor;
   bool (*upgradeProc)();
} s_dbUpgradeMap[] = {
   { 6,  52, 7,  H_UpgradeFromV6  },
   { 5,  52, 6,  H_UpgradeFromV5  },
   { 4,  52, 5,  H_UpgradeFromV4  },
   { 3,  52, 4,  H_UpgradeFromV3  },
   { 2,  52, 3,  H_UpgradeFromV2  },
   { 1,  52, 2,  H_UpgradeFromV1  },
   { 0,  52, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V52()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 52) && (minor < DB_SCHEMA_VERSION_V52_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         WriteToTerminalEx(L"Unable to find upgrade procedure for version 51.%d\n", minor);
         return false;
      }
      WriteToTerminalEx(L"Upgrading from version 52.%d to %d.%d\n", minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_dbHandle);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_dbHandle);
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            return false;
      }
      else
      {
         WriteToTerminal(L"Rolling back last stage due to upgrade errors...\n");
         DBRollback(g_dbHandle);
         return false;
      }
   }
   return true;
}
