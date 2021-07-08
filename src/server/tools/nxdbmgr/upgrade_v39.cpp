/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2020-2021 Raden Solutions
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
** File: upgrade_v39.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Upgrade from 39.7 to 40.0
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(SetMajorSchemaVersion(40, 0));
   return true;
}

/**
 * Upgrade from 39.6 to 39.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE notification_log (")
      _T("   id $SQL:INT64 not null,")
      _T("   notification_channel varchar(63) not null,")
      _T("   notification_timestamp integer not null,")
      _T("   recipient varchar(2000) null,")
      _T("   subject varchar(2000) null,")
      _T("   message varchar(2000) null,")
      _T("   success integer not null,")
      _T("PRIMARY KEY(id))")));

      CHK_EXEC(CreateTable(
      _T("CREATE TABLE server_action_execution_log (")
      _T("   id $SQL:INT64 not null,")
      _T("   action_timestamp integer not null,")
      _T("   action_code integer not null,")
      _T("   action_name varchar(63) null,")
      _T("   channel_name varchar(63) null,")
      _T("   recipient varchar(2000) null,")
      _T("   subject varchar(2000) null,")
      _T("   action_data varchar(2000) null,")
      _T("   success integer not null,")
      _T("PRIMARY KEY(id))")));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 39.5 to 39.6
 */
static bool H_UpgradeFromV5()
{
   CHK_EXEC(CreateConfigParam(_T("LDAP.Mapping.Email"),
         _T(""),
         _T("The name of an attribute whose value will be used as a user's email."),
         nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("LDAP.Mapping.PhoneNumber"),
         _T(""),
         _T("The name of an attribute whose value will be used as a user's phone number."),
         nullptr, 'S', true, false, false, false));

   static const TCHAR *batch =
         _T("UPDATE config SET var_name='LDAP.Mapping.Description' WHERE var_name='LDAP.MappingDescription'\n")
         _T("UPDATE config SET var_name='LDAP.Mapping.FullName' WHERE var_name='LDAP.MappingFullName'\n")
         _T("UPDATE config SET var_name='LDAP.Mapping.GroupName' WHERE var_name='LDAP.GroupMappingName'\n")
         _T("UPDATE config SET var_name='LDAP.Mapping.UserName' WHERE var_name='LDAP.UserMappingName'\n")
         _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SQLQuery(_T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
         _T("VALUES ('4fae91b5-8802-4f6c-aace-a03f9f7fa8ef',25,'Hook::LDAPSynchronization','/* Available global variables:\r\n *  $ldapObject - LDAP object being synchronized (object of ''LDAPObject'' class)\r\n *\r\n * Expected return value:\r\n *  true/false - boolean - whether processing of this LDAP object should continue\r\n */\r\n')")));

   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 39.4 to 39.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(CreateConfigParam(_T("Geolocation.History.RetentionTime"),
         _T("90"),
         _T("Retention time in days for object's geolocation history. All records older than specified will be deleted by housekeeping process."),
         _T("days"),
         'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 39.3 to 39.4
 */
static bool H_UpgradeFromV3()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE input_fields ADD flags integer")));

   DB_RESULT hResult = SQLSelect(_T("SELECT category,owner_id,name,config FROM input_fields"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         TCHAR category[2], name[MAX_OBJECT_NAME];
         DBGetField(hResult, i, 0, category, 2);
         uint32_t ownerId = DBGetFieldLong(hResult, i, 1);
         DBGetField(hResult, i, 2, name, MAX_OBJECT_NAME);
         TCHAR *config = DBGetField(hResult, i, 3, nullptr, 0);

         TCHAR query[1024];
         _sntprintf(query, 1024, _T("UPDATE input_fields SET flags=%d WHERE category='%s' AND owner_id=%u AND name=%s"),
                  (_tcsstr(config, _T("<validatePassword>true</validatePassword>")) != nullptr) ? 1 : 0, category, ownerId, DBPrepareString(g_dbHandle, name).cstr());
         MemFree(config);

         if (!SQLQuery(query) && !g_ignoreErrors)
         {
            DBFreeResult(hResult);
            return false;
         }
      }
      DBFreeResult(hResult);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("input_fields"), _T("flags")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("input_fields"), _T("config")));

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 39.2 to 39.3
 */
static bool H_UpgradeFromV2()
{
   CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("object_tools_input_fields")));
   CHK_EXEC(DBRenameTable(g_dbHandle, _T("object_tools_input_fields"), _T("input_fields")));
   CHK_EXEC(DBRenameColumn(g_dbHandle, _T("input_fields"), _T("tool_id"), _T("owner_id")));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE input_fields ADD category char(1)")));
   CHK_EXEC(SQLQuery(_T("UPDATE input_fields SET category='T'")));
   CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("input_fields"), _T("category")));
   CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("input_fields"), _T("category,owner_id,name")));
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 39.1 to 39.2
 */
static bool H_UpgradeFromV1()
{
   if (DBIsTableExist(g_dbHandle, _T("zmq_subscription")) == DBIsTableExist_Found)
      CHK_EXEC(SQLQuery(_T("DROP TABLE zmq_subscription")));
   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 39.0 to 39.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE object_queries (")
         _T("   id integer not null,")
         _T("   guid varchar(36) not null,")
         _T("   name varchar(63) not null,")
         _T("   description varchar(255) null,")
         _T("   script $SQL:TEXT null,")
         _T("PRIMARY KEY(id))")));

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
} s_dbUpgradeMap[] =
{
   { 7,  40, 0,  H_UpgradeFromV7  },
   { 6,  39, 7,  H_UpgradeFromV6  },
   { 5,  39, 6,  H_UpgradeFromV5  },
   { 4,  39, 5,  H_UpgradeFromV4  },
   { 3,  39, 4,  H_UpgradeFromV3  },
   { 2,  39, 3,  H_UpgradeFromV2  },
   { 1,  39, 2,  H_UpgradeFromV1  },
   { 0,  39, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V39()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while(major == 39)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 39.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 39.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_dbHandle);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_dbHandle);
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            return false;
      }
      else
      {
         _tprintf(_T("Rolling back last stage due to upgrade errors...\n"));
         DBRollback(g_dbHandle);
         return false;
      }
   }
   return true;
}
