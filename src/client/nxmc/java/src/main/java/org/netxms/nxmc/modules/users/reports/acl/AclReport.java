/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2025 Raden Soultions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.users.reports.acl;

import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import org.apache.poi.xssf.usermodel.XSSFCell;
import org.apache.poi.xssf.usermodel.XSSFCellStyle;
import org.apache.poi.xssf.usermodel.XSSFRow;
import org.apache.poi.xssf.usermodel.XSSFSheet;
import org.apache.poi.xssf.usermodel.XSSFWorkbook;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.reports.acl.constants.GroupsSheetCells;
import org.netxms.nxmc.modules.users.reports.acl.constants.PermissionsSheetCells;
import org.netxms.nxmc.modules.users.reports.acl.constants.UsersSheetCells;
import org.xnap.commons.i18n.I18n;

/**
 * Generate detailed object access report in form of Excel table
 */
public class AclReport extends AbstractAclReport
{
   private final I18n i18n = LocalizationHelper.getI18n(AclReport.class);

   /**
    * Create new ACL report object.
    *
    * @param outputFileName name of output file
    */
   public AclReport(String outputFileName)
   {
      super(outputFileName);
   }

   /**
    * @see org.netxms.nxmc.modules.users.reports.acl.AbstractAclReport#generateUserSheet(org.apache.poi.xssf.usermodel.XSSFWorkbook,
    *      org.apache.poi.xssf.usermodel.XSSFCellStyle)
    */
   @Override
   protected void generateUserSheet(XSSFWorkbook wb, XSSFCellStyle headerStyle)
   {
      AtomicInteger rowNum = new AtomicInteger(0);
      XSSFSheet sheet = wb.getSheet(SHEET_USERS);
      XSSFRow headerRow = sheet.createRow(rowNum.getAndIncrement());

      XSSFCell cell = headerRow.createCell(UsersSheetCells.ID.ordinal());
      cell.setCellValue("ID");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.LOGIN.ordinal());
      cell.setCellValue("Login");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.STATUS.ordinal());
      cell.setCellValue("Status");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.FULL_NAME.ordinal());
      cell.setCellValue("Full Name");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.DESCRIPTION.ordinal());
      cell.setCellValue("Description");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.ORIGIN.ordinal());
      cell.setCellValue("Origin");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.GROUPS.ordinal());
      cell.setCellValue("Member of");
      cell.setCellStyle(headerStyle);

      int columnIndex = UsersSheetCells.ACL_START_COLUMN.ordinal();

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("All Scheduled tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Asset Mgmt Attributes");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("SNMP Traps");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Delete Alarms");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Event Templates");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("EPP");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("External Integration");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Import Config");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Actions");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Agent Configs");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Agent Tunnels");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Geo Areas");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Image Library");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Mapping Tables");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Object Queries");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Packages");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Repositories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Scripts");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Server Files");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Sessions");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Summary Tables");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Tools");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage 2FA Methods");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Users");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Mobile Device Login");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Object Categories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Own Scheduled Tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Persistent Storage");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Read Server Files");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Reporting Server");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Schedule File Upload");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Schedule Maintenance");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Schedule Scripts");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Send Notifications");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Server Config");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Server Console");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Setup TCP Proxy");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("SSH Keys");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("User Agent Notifications");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Unlink Issues");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("User Scheduled Tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("View All Alarms");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Asset Change Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Audit Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("View Event Templates");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Event Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("View Repositories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Syslog");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("SNMP Trap Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Web Service Definitions");
      cell.setCellStyle(headerStyle);

      sheet.setColumnWidth(UsersSheetCells.ID.ordinal(), 2048);
      sheet.setColumnWidth(UsersSheetCells.LOGIN.ordinal(), 3072);
      sheet.setColumnWidth(UsersSheetCells.STATUS.ordinal(), 2048);
      sheet.setColumnWidth(UsersSheetCells.FULL_NAME.ordinal(), 6144);
      sheet.setColumnWidth(UsersSheetCells.DESCRIPTION.ordinal(), 6144);
      sheet.setColumnWidth(UsersSheetCells.ORIGIN.ordinal(), 2048);
      sheet.setColumnWidth(UsersSheetCells.GROUPS.ordinal(), 6144);

      AbstractUserObject[] userDatabaseObjects = session.getUserDatabaseObjects();
      Arrays.stream(userDatabaseObjects).filter(p -> p instanceof User).sorted(Comparator.comparing(AbstractUserObject::getName)).forEach(element -> {
         User user = (User)element;
         XSSFRow row = sheet.createRow(rowNum.getAndIncrement());
         row.createCell(UsersSheetCells.ID.ordinal()).setCellValue(user.getId());
         row.createCell(UsersSheetCells.LOGIN.ordinal()).setCellValue(user.getName());
         row.createCell(UsersSheetCells.STATUS.ordinal()).setCellValue(user.isDisabled() ? "Disabled" : "Active");
         row.createCell(UsersSheetCells.FULL_NAME.ordinal()).setCellValue(user.getFullName());
         row.createCell(UsersSheetCells.DESCRIPTION.ordinal()).setCellValue(user.getDescription());
         row.createCell(UsersSheetCells.ORIGIN.ordinal()).setCellValue((user.getFlags() & AbstractUserObject.LDAP_USER) != 0 ? "LDAP" : "Local");

         boolean firstRow = true;
         for(int groupId : user.getGroups())
         {
            final XSSFRow groupRow;
            if (firstRow)
            {
               groupRow = row;
               firstRow = false;
            }
            else
            {
               groupRow = sheet.createRow(rowNum.getAndIncrement());
            }
            AbstractUserObject group = session.findUserDBObjectById(groupId, null);
            groupRow.createCell(UsersSheetCells.GROUPS.ordinal()).setCellValue(group.getName());
         }

         int aclColumnIndex = UsersSheetCells.ACL_START_COLUMN.ordinal();
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_ALL_SCHEDULED_TASKS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_AM_ATTRIBUTE_MANAGE, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_CONFIGURE_TRAPS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_DELETE_ALARMS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_EDIT_EVENT_DB, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_EPP, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_EXTERNAL_INTEGRATION, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_IMPORT_CONFIGURATION, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_ACTIONS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_CFG, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_TUNNELS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_GEO_AREAS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_IMAGE_LIB, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_MAPPING_TBLS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_PACKAGES, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_REPOSITORIES, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_SCRIPTS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_SERVER_FILES, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_SESSIONS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_TOOLS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_2FA_METHODS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_USERS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_OBJECT_CATEGORIES, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_OWN_SCHEDULED_TASKS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_PERSISTENT_STORAGE, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_READ_SERVER_FILES, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_REPORTING_SERVER, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_MAINTENANCE, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_SCRIPT, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SEND_NOTIFICATION, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SERVER_CONFIG, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SERVER_CONSOLE, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SETUP_TCP_PROXY, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SSH_KEY_CONFIGURATION, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_UA_NOTIFICATIONS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_UNLINK_ISSUES, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_USER_SCHEDULED_TASKS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_ALL_ALARMS, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_ASSET_CHANGE_LOG, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_AUDIT_LOG, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_DB, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_REPOSITORIES, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_SYSLOG, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_TRAP_LOG, user);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS, user);
      });
   }

   /**
    * @see org.netxms.nxmc.modules.users.reports.acl.AbstractAclReport#generateGroupsSheet(org.apache.poi.xssf.usermodel.XSSFWorkbook,
    *      org.apache.poi.xssf.usermodel.XSSFCellStyle)
    */
   @Override
   protected void generateGroupsSheet(XSSFWorkbook wb, XSSFCellStyle headerStyle)
   {
      AtomicInteger rowNum = new AtomicInteger(0);
      XSSFSheet sheet = wb.getSheet(SHEET_GROUPS);
      XSSFRow headerRow = sheet.createRow(rowNum.getAndIncrement());

      XSSFCell cell = headerRow.createCell(GroupsSheetCells.ID.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("ID");

      cell = headerRow.createCell(GroupsSheetCells.NAME.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Name");

      cell = headerRow.createCell(GroupsSheetCells.DESCRIPTION.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Description");

      cell = headerRow.createCell(GroupsSheetCells.MEMBERS.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Members");

      int columnIndex = GroupsSheetCells.ACL_START_COLUMN.ordinal();

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("All Scheduled tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Asset Mgmt Attributes");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Configure SNMP Traps");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Delete Alarms");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Event Templates");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("EPP");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("External Integration");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Import Config");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Actions");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Agent Configs");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Agent Tunnels");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Geo Areas");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Image Library");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Mapping Tables");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Object Queries");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Packages");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Repositories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Scripts");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Server Files");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Sessions");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Summary Tables");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Tools");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage 2FA Methods");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Manage Users");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Mobile Device Login");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Object Categories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Own Scheduled Tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Persistent Storage");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Read Server Files");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Reporting Server");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Schedule File Upload");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Schedule Maintenance");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Schedule Scripts");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Send Notifications");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Server Config");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Server Console");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Setup TCP Proxy");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("SSH Keys");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("User Agent Notifications");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Unlink Issues");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("User Scheduled Tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("View All Alarms");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Asset Change Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Audit Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("View Event Templates");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Event Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("View Repositories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Syslog");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("SNMP Trap Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(columnIndex++);
      cell.setCellValue("Web Service Definitions");
      cell.setCellStyle(headerStyle);

      sheet.setColumnWidth(GroupsSheetCells.ID.ordinal(), 3072);
      sheet.setColumnWidth(GroupsSheetCells.NAME.ordinal(), 6144);
      sheet.setColumnWidth(GroupsSheetCells.DESCRIPTION.ordinal(), 6144);
      sheet.setColumnWidth(GroupsSheetCells.MEMBERS.ordinal(), 4096);

      Arrays.stream(session.getUserDatabaseObjects()).filter(p -> p instanceof UserGroup).sorted(Comparator.comparing(AbstractUserObject::getName)).forEach(element -> {
         UserGroup group = (UserGroup)element;
         XSSFRow row = sheet.createRow(rowNum.getAndIncrement());

         row.createCell(GroupsSheetCells.ID.ordinal()).setCellValue(group.getId());
         row.createCell(GroupsSheetCells.NAME.ordinal()).setCellValue(group.getName());
         row.createCell(GroupsSheetCells.DESCRIPTION.ordinal()).setCellValue(group.getDescription());

         boolean firstRow = true;
         for(int memberId : group.getMembers())
         {
            final XSSFRow memberRow;
            if (firstRow)
            {
               memberRow = row;
               firstRow = false;
            }
            else
            {
               memberRow = sheet.createRow(rowNum.getAndIncrement());
            }
            AbstractUserObject member = session.findUserDBObjectById(memberId, null);
            if (member != null)
            {
               final String name;
               if (member instanceof User)
               {
                  name = member.getName();
               }
               else
               {
                  name = member.getName() + " (group)";
               }
               memberRow.createCell(GroupsSheetCells.MEMBERS.ordinal()).setCellValue(name);
            }
         }

         int aclColumnIndex = GroupsSheetCells.ACL_START_COLUMN.ordinal();
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_ALL_SCHEDULED_TASKS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_AM_ATTRIBUTE_MANAGE, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_CONFIGURE_TRAPS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_DELETE_ALARMS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_EDIT_EVENT_DB, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_EPP, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_EXTERNAL_INTEGRATION, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_IMPORT_CONFIGURATION, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_ACTIONS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_CFG, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_TUNNELS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_GEO_AREAS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_IMAGE_LIB, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_MAPPING_TBLS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_PACKAGES, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_REPOSITORIES, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_SCRIPTS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_SERVER_FILES, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_SESSIONS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_TOOLS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_2FA_METHODS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MANAGE_USERS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_OBJECT_CATEGORIES, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_OWN_SCHEDULED_TASKS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_PERSISTENT_STORAGE, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_READ_SERVER_FILES, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_REPORTING_SERVER, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_MAINTENANCE, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_SCRIPT, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SEND_NOTIFICATION, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SERVER_CONFIG, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SERVER_CONSOLE, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SETUP_TCP_PROXY, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_SSH_KEY_CONFIGURATION, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_UA_NOTIFICATIONS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_UNLINK_ISSUES, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_USER_SCHEDULED_TASKS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_ALL_ALARMS, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_ASSET_CHANGE_LOG, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_AUDIT_LOG, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_DB, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_REPOSITORIES, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_SYSLOG, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_VIEW_TRAP_LOG, group);
         createPermissionCell(row, aclColumnIndex++, UserAccessRights.SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS, group);
      });
   }

   /**
    * @see org.netxms.nxmc.modules.users.reports.acl.AbstractAclReport#generatePermissionsSheet(org.apache.poi.xssf.usermodel.XSSFWorkbook,
    *      org.apache.poi.xssf.usermodel.XSSFCellStyle, java.util.List)
    */
   @Override
   protected void generatePermissionsSheet(XSSFWorkbook wb, XSSFCellStyle headerStyle, List<ObjectAccess> permissions)
   {
      AtomicInteger rowNum = new AtomicInteger(0);
      XSSFSheet sheet = wb.getSheet(SHEET_PERMISSIONS);
      XSSFRow headerRow = sheet.createRow(rowNum.getAndIncrement());

      XSSFCell cell = headerRow.createCell(PermissionsSheetCells.OBJECT_NAME.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Object Name");

      cell = headerRow.createCell(PermissionsSheetCells.USER_OR_GROUP_NAME.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("User/Group name");

      cell = headerRow.createCell(PermissionsSheetCells.INHERIT_ACCESS.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Inherit Access");

      cell = headerRow.createCell(PermissionsSheetCells.READ.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Read");

      cell = headerRow.createCell(PermissionsSheetCells.DELEGATED_READ.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Delegated Read");

      cell = headerRow.createCell(PermissionsSheetCells.MODIFY.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Modify");

      cell = headerRow.createCell(PermissionsSheetCells.EDIT_COMMENTS.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Edit Comments");

      cell = headerRow.createCell(PermissionsSheetCells.EDIT_RESP_USERS.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Edit Responsible Users");

      cell = headerRow.createCell(PermissionsSheetCells.DELETE.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Delete");

      cell = headerRow.createCell(PermissionsSheetCells.CONTROL.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Control");

      cell = headerRow.createCell(PermissionsSheetCells.MAINTENANCE.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Enter/Leave Maintenance");

      cell = headerRow.createCell(PermissionsSheetCells.CONFIGURE_AGENT.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Configure Agent");

      cell = headerRow.createCell(PermissionsSheetCells.READ_AGENT.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Read Agent");

      cell = headerRow.createCell(PermissionsSheetCells.READ_SNMP.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Read SNMP");

      cell = headerRow.createCell(PermissionsSheetCells.TAKE_SCREENSHOT.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Take Screenshots");

      cell = headerRow.createCell(PermissionsSheetCells.EDIT_MAINTENANCE_JOURNAL.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Edit Maintenance Journal");

      cell = headerRow.createCell(PermissionsSheetCells.CREATE_CHILD_OBJECTS.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Create Child Objects");

      cell = headerRow.createCell(PermissionsSheetCells.VIEW_ALARMS.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("View Alarms");

      cell = headerRow.createCell(PermissionsSheetCells.ACKNOWLEDGE_ALARMS.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Acknowledge Alarms");

      cell = headerRow.createCell(PermissionsSheetCells.TERMINATE_ALARMS.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Terminate Alarms");

      cell = headerRow.createCell(PermissionsSheetCells.ACCESS_CONTROL.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Access Control");

      cell = headerRow.createCell(PermissionsSheetCells.CREATE_HELPDESK_ISSUES.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Create Helpdesk Issues");

      cell = headerRow.createCell(PermissionsSheetCells.DOWNLOAD_FILES.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Download Files");

      cell = headerRow.createCell(PermissionsSheetCells.UPLOAD_FILES.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Upload Files");

      cell = headerRow.createCell(PermissionsSheetCells.MANAGE_FILES.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Manage Files");

      cell = headerRow.createCell(PermissionsSheetCells.SEND_EVENTS.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Send Events");

      cell = headerRow.createCell(PermissionsSheetCells.PUSH_DATA.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Push Data");

      sheet.setColumnWidth(PermissionsSheetCells.OBJECT_NAME.ordinal(), 20480);
      sheet.setColumnWidth(PermissionsSheetCells.USER_OR_GROUP_NAME.ordinal(), 4096);

      for(ObjectAccess element : permissions)
      {
         XSSFRow row = sheet.createRow(rowNum.getAndIncrement());

         row.createCell(PermissionsSheetCells.OBJECT_NAME.ordinal()).setCellValue(element.name);
         row.createCell(PermissionsSheetCells.USER_OR_GROUP_NAME.ordinal()).setCellValue(element.userName);
         row.createCell(PermissionsSheetCells.INHERIT_ACCESS.ordinal()).setCellValue(element.inheritAccessRights ? i18n.tr("YES") : i18n.tr("NO"));

         createPermissionCell(row, PermissionsSheetCells.READ, UserAccessRights.OBJECT_ACCESS_READ, element);
         createPermissionCell(row, PermissionsSheetCells.DELEGATED_READ, UserAccessRights.OBJECT_ACCESS_DELEGATED_READ, element);
         createPermissionCell(row, PermissionsSheetCells.MODIFY, UserAccessRights.OBJECT_ACCESS_MODIFY, element);
         createPermissionCell(row, PermissionsSheetCells.EDIT_COMMENTS, UserAccessRights.OBJECT_ACCESS_EDIT_COMMENTS, element);
         createPermissionCell(row, PermissionsSheetCells.EDIT_RESP_USERS, UserAccessRights.OBJECT_ACCESS_EDIT_RESP_USERS, element);
         createPermissionCell(row, PermissionsSheetCells.DELETE, UserAccessRights.OBJECT_ACCESS_DELETE, element);
         createPermissionCell(row, PermissionsSheetCells.CONTROL, UserAccessRights.OBJECT_ACCESS_CONTROL, element);
         createPermissionCell(row, PermissionsSheetCells.MAINTENANCE, UserAccessRights.OBJECT_ACCESS_MAINTENANCE, element);
         createPermissionCell(row, PermissionsSheetCells.CONFIGURE_AGENT, UserAccessRights.OBJECT_ACCESS_CONFIGURE_AGENT, element);
         createPermissionCell(row, PermissionsSheetCells.READ_AGENT, UserAccessRights.OBJECT_ACCESS_READ_AGENT, element);
         createPermissionCell(row, PermissionsSheetCells.READ_SNMP, UserAccessRights.OBJECT_ACCESS_READ_SNMP, element);
         createPermissionCell(row, PermissionsSheetCells.TAKE_SCREENSHOT, UserAccessRights.OBJECT_ACCESS_SCREENSHOT, element);
         createPermissionCell(row, PermissionsSheetCells.EDIT_MAINTENANCE_JOURNAL, UserAccessRights.OBJECT_ACCESS_EDIT_MNT_JOURNAL, element);
         createPermissionCell(row, PermissionsSheetCells.CREATE_CHILD_OBJECTS, UserAccessRights.OBJECT_ACCESS_CREATE, element);
         createPermissionCell(row, PermissionsSheetCells.VIEW_ALARMS, UserAccessRights.OBJECT_ACCESS_READ_ALARMS, element);
         createPermissionCell(row, PermissionsSheetCells.ACKNOWLEDGE_ALARMS, UserAccessRights.OBJECT_ACCESS_UPDATE_ALARMS, element);
         createPermissionCell(row, PermissionsSheetCells.TERMINATE_ALARMS, UserAccessRights.OBJECT_ACCESS_TERM_ALARMS, element);
         createPermissionCell(row, PermissionsSheetCells.ACCESS_CONTROL, UserAccessRights.OBJECT_ACCESS_ACL, element);
         createPermissionCell(row, PermissionsSheetCells.CREATE_HELPDESK_ISSUES, UserAccessRights.OBJECT_ACCESS_CREATE_ISSUE, element);
         createPermissionCell(row, PermissionsSheetCells.DOWNLOAD_FILES, UserAccessRights.OBJECT_ACCESS_DOWNLOAD, element);
         createPermissionCell(row, PermissionsSheetCells.UPLOAD_FILES, UserAccessRights.OBJECT_ACCESS_UPLOAD, element);
         createPermissionCell(row, PermissionsSheetCells.MANAGE_FILES, UserAccessRights.OBJECT_ACCESS_MANAGE_FILES, element);
         createPermissionCell(row, PermissionsSheetCells.SEND_EVENTS, UserAccessRights.OBJECT_ACCESS_SEND_EVENTS, element);
         createPermissionCell(row, PermissionsSheetCells.PUSH_DATA, UserAccessRights.OBJECT_ACCESS_PUSH_DATA, element);
      }
   }
}
