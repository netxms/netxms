/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2022 Raden Solutions
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
** File: upgrade_v41.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Upgrade from 41.7 to 41.8
 */
static bool H_UpgradeFromV7()
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE interfaces ADD if_alias varchar(255)")));
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade from 41.6 to 41.7
 */
static bool H_UpgradeFromV6()
{
   CHK_EXEC(CreateConfigParam(_T("NotificationChannels.MaxRetryCount"), _T("3"),
                              _T("Maximum count of retries to send a message for all notification channels."),
                              _T(""), 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 41.5 to 41.6
 */
static bool H_UpgradeFromV5()
{
   if (GetSchemaLevelForMajorVersion(40) < 102)
   {
      CHK_EXEC(SQLQuery(_T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('f8bfa009-d4e4-4443-8bad-3ded09bdeb92',26,'Hook::Login','/* Available global variables:\r\n *  $user - user object (object of ''User'' class)\r\n *  $session - session object (object of ''ClientSession'' class)\r\n *\r\n * Expected return value:\r\n *  true/false - boolean - whether login for this session should continue\r\n */\r\n')")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(40, 102));
   }
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Upgrade from 41.4 to 41.5
 */
static bool H_UpgradeFromV4()
{
   CHK_EXEC(SQLQuery(_T("CREATE TABLE maintenance_journal (")
                     _T("   record_id integer not null,")
                     _T("   object_id integer not null,")
                     _T("   author integer not null,")
                     _T("   last_edited_by integer not null,")
                     _T("   description $SQL:TEXT not null,")
                     _T("   creation_time integer not null,")
                     _T("   modification_time integer not null,")
                     _T("   PRIMARY KEY(record_id))")));

   DB_RESULT result = SQLSelect(_T("SELECT access_rights,object_id FROM acl WHERE user_id=1073741825")); // Get group Admins object acl
   if (result != nullptr)
   {
      DB_STATEMENT stmt = DBPrepare(g_dbHandle, _T("UPDATE acl SET access_rights=? WHERE user_id=1073741825 AND object_id=?"));
      if (stmt != nullptr)
      {
         int rows = DBGetNumRows(result);
         for (int i = 0; i < rows; i++)
         {
            uint32_t rights = DBGetFieldULong(result, i, 0);
            if (rights & OBJECT_ACCESS_READ)
            {
               rights |= OBJECT_ACCESS_WRITE_MJOURNAL;
               DBBind(stmt, 1, DB_SQLTYPE_INTEGER, rights);
               DBBind(stmt, 2, DB_SQLTYPE_INTEGER, DBGetFieldULong(result, i, 1));
               if (!SQLExecute(stmt) && !g_ignoreErrors)
               {
                  DBFreeStatement(stmt);
                  DBFreeResult(result);
                  return false;
               }
            }
         }

         DBFreeStatement(stmt);
      }
      else if (!g_ignoreErrors)
      {
         DBFreeResult(result);
         return false;
      }
      DBFreeResult(result);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(CreateConfigParam(_T("MaintenanceJournal.RetentionTime"), _T("1826"),
                              _T("Retention time in days for maintenance journal entries. All records older than specified will be deleted by housekeeping process."),
                              _T("days"), 'I', true, false, false, false));

   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 41.3 to 41.4
 */
static bool H_UpgradeFromV3()
{
   if (GetSchemaLevelForMajorVersion(40) < 101)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET message='Duplicate MAC address found (%1 on %2)' WHERE guid='c19fbb37-98c9-43a5-90f2-7a54ba9116fa' AND message='Duplicate MAC address found (%1on %2)'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(40, 101));
   }
   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 41.2 to 41.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(40) < 100)
   {
      if (DBIsTableExist(g_dbHandle, _T("zmq_subscription")) == DBIsTableExist_Found)
      {
         CHK_EXEC(SQLQuery(_T("DROP TABLE zmq_subscription")));
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(40, 100));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 41.1 to 41.2
 */
static bool H_UpgradeFromV1()
{
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("users"), _T("xmpp_id")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config_values WHERE var_name = 'XMPP.Port'")));

   StringMap *xmppVariables = DBMgrGetConfigurationVariables(_T("XMPP.%"));
   if (xmppVariables != nullptr)
   {
      const TCHAR* login = xmppVariables->get(_T("XMPP.Login"));
      const TCHAR* password = xmppVariables->get(_T("XMPP.Password"));
      const TCHAR* server = xmppVariables->get(_T("XMPP.Server"));
      uint32_t port = xmppVariables->getUInt32(_T("XMPP.Port"), 0);

      DB_STATEMENT stmt = DBPrepare(g_dbHandle, _T("INSERT INTO notification_channels (name, driver_name, description, configuration) ")
                                                _T("VALUES ('XMPP', 'XMPP', 'Automatically generated XMPP notification channel based on old XMPP configuration', ?)"));
      StringBuffer config;
      if (server != nullptr)
      {
         config.append(_T("Server = "));
         config.append(server);
         config.append(_T("\n"));
      }
      if (port != 0)
      {
         config.append(_T("Port = "));
         config.append(port);
         config.append(_T("\n"));
      }
      if (login != nullptr)
      {
         config.append(_T("Login = "));
         config.append(login);
         config.append(_T("\n"));
      }
      if (password != nullptr)
      {
         config.append(_T("Password = "));
         config.append(password);
         config.append(_T("\n"));
      }
      delete xmppVariables;

      DBBind(stmt, 1, DB_SQLTYPE_TEXT, config, DB_BIND_STATIC);
      if (!SQLExecute(stmt) && !g_ignoreErrors)
      {
         DBFreeStatement(stmt);
         return false;
      }
      DBFreeStatement(stmt);

      CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name LIKE 'XMPP.%'")));
      CHK_EXEC(SQLQuery(_T("UPDATE actions SET action_type = 3,channel_name = 'XMPP' WHERE action_type = 6")));
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   DB_RESULT result = SQLSelect(_T("SELECT id, system_access FROM users"));
   if (result != nullptr)
   {
      TCHAR query[256];
      int count = DBGetNumRows(result);
      for (int i = 0; i < count; i++)
      {
         _sntprintf(query, 256, _T("UPDATE users SET system_access = ") UINT64_FMT _T(" WHERE id = ") UINT64_FMT,
               DBGetFieldUInt64(result, i, 1) & ~MASK_BIT64(26), DBGetFieldUInt64(result, i, 0));
         CHK_EXEC(SQLQuery(query));
      }
      DBFreeResult(result);
   }
   else if (!g_ignoreErrors)
   {
      return false;
   }

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 41.0 to 41.1
 */
static bool H_UpgradeFromV0()
{
   CHK_EXEC(CreateConfigParam(_T("AgentTunnels.Certificates.ReissueInterval"),
         _T("30"),
         _T("Interval in days between reissuing agent certificates"),
         _T("days"),
         'I', true, false, false, false));

   CHK_EXEC(CreateConfigParam(_T("AgentTunnels.Certificates.ValidityPeriod"),
         _T("90"),
         _T("Validity period in days for newly issued agent certificates"),
         _T("days"),
         'I', true, false, false, false));

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
   { 7,  41, 8,  H_UpgradeFromV7  },
   { 6,  41, 7,  H_UpgradeFromV6  },
   { 5,  41, 6,  H_UpgradeFromV5  },
   { 4,  41, 5,  H_UpgradeFromV4  },
   { 3,  41, 4,  H_UpgradeFromV3  },
   { 2,  41, 3,  H_UpgradeFromV2  },
   { 1,  41, 2,  H_UpgradeFromV1  },
   { 0,  41, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V41()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while ((major == 41) && (minor < DB_SCHEMA_VERSION_V41_MINOR))
   {
      // Find upgrade procedure
      int i;
      for (i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 41.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 41.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
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
