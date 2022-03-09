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
