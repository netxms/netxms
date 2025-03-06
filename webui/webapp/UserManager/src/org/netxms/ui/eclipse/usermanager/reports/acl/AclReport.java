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
package org.netxms.ui.eclipse.usermanager.reports.acl;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import org.apache.poi.hssf.usermodel.HSSFCell;
import org.apache.poi.hssf.usermodel.HSSFCellStyle;
import org.apache.poi.hssf.usermodel.HSSFFont;
import org.apache.poi.hssf.usermodel.HSSFRow;
import org.apache.poi.hssf.usermodel.HSSFSheet;
import org.apache.poi.hssf.usermodel.HSSFWorkbook;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.usermanager.reports.acl.constants.GroupsSheetCells;
import org.netxms.ui.eclipse.usermanager.reports.acl.constants.PermissionsSheetCells;
import org.netxms.ui.eclipse.usermanager.reports.acl.constants.UsersSheetCells;

/**
 * Generate detailed object access report in form of Excel table
 */
public class AclReport
{
   private final String SHEET_USERS = "Users";
   private final String SHEET_GROUPS = "Groups";
   private final String SHEET_PERMISSIONS = "Object Permissions";

   private NXCSession session = ConsoleSharedData.getSession();
   private String outputFileName;

   /**
    * Create new ACL report object.
    *
    * @param outputFileName name of output file
    */
   public AclReport(String outputFileName)
   {
      this.outputFileName = outputFileName;
   }

   /**
    * Execute report.
    *
    * @throws IOException if communicatrion with server failed or output file cannot be written
    * @throws NXCException if server returned error on any of the calls
    */
   public void execute() throws IOException, NXCException
   {
      List<ObjectAccess> permissions = new ArrayList<>();
      AbstractObject[] topLevelObjects = session.getTopLevelObjects();
      for(AbstractObject topLevelObject : topLevelObjects)
      {
         scanACL(topLevelObject, "", permissions);
      }

      // resolve user IDs
      for(ObjectAccess row : permissions)
      {
         AbstractUserObject user = session.findUserDBObjectById(row.userId, null);
         if (user != null)
         {
            row.userName = user.getName();
            if (!(user instanceof User))
            {
               row.userName += " (group)";
            }
         }
         else
         {
            row.userName = String.format("DELETED [%d]", row.userId);
         }
      }

      HSSFWorkbook wb = new HSSFWorkbook();
      HSSFFont headerFont = wb.createFont();
      headerFont.setBold(true);
      HSSFCellStyle headerCellStyle = wb.createCellStyle();
      headerCellStyle.setFont(headerFont);

      wb.createSheet();
      wb.createSheet();
      wb.createSheet();
      wb.setSheetName(0, SHEET_USERS);
      wb.setSheetName(1, SHEET_GROUPS);
      wb.setSheetName(2, SHEET_PERMISSIONS);

      generateUserSheet(wb, headerCellStyle);
      generateGroupsSheet(wb, headerCellStyle);
      generatePermissionsSheet(wb, headerCellStyle, permissions);

      // Write the output to a file
      try (OutputStream fileOut = new FileOutputStream(outputFileName))
      {
         wb.write(fileOut);
         fileOut.close();
      }
   }

   /**
    * Generate "Users" sheet.
    *
    * @param wb
    * @param headerStyle
    */
   private void generateUserSheet(HSSFWorkbook wb, HSSFCellStyle headerStyle)
   {
      AtomicInteger rowNum = new AtomicInteger(0);
      HSSFSheet sheet = wb.getSheet(SHEET_USERS);
      HSSFRow headerRow = sheet.createRow(rowNum.getAndIncrement());

      HSSFCell cell = headerRow.createCell(UsersSheetCells.ID.ordinal());
      cell.setCellValue("ID");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.LOGIN.ordinal());
      cell.setCellValue("Login");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.ACTIVE.ordinal());
      cell.setCellValue("Active?");
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

      cell = headerRow.createCell(UsersSheetCells.ALL_SCHEDULED_TASKS.ordinal());
      cell.setCellValue("All Scheduled tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.AM_ATTRIBUTE_MANAGE.ordinal());
      cell.setCellValue("Asset Mgmt Attributes");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.CONFIGURE_TRAPS.ordinal());
      cell.setCellValue("SNMP Traps");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.DELETE_ALARMS.ordinal());
      cell.setCellValue("Delete Alarms");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.EDIT_EVENT_DB.ordinal());
      cell.setCellValue("Event Templates");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.EPP.ordinal());
      cell.setCellValue("EPP");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.EXTERNAL_INTEGRATION.ordinal());
      cell.setCellValue("External Integration");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.IMPORT_CONFIGURATION.ordinal());
      cell.setCellValue("Import Config");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_ACTIONS.ordinal());
      cell.setCellValue("Manage Actions");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_AGENT_CFG.ordinal());
      cell.setCellValue("Managet Agent Configs");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_GEO_AREAS.ordinal());
      cell.setCellValue("Manage Geo Areas");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_IMAGE_LIB.ordinal());
      cell.setCellValue("Manage Image Library");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_MAPPING_TBLS.ordinal());
      cell.setCellValue("Manage Mapping Tables");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_OBJECT_QUERIES.ordinal());
      cell.setCellValue("Manage Object Queries");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_PACKAGES.ordinal());
      cell.setCellValue("Manage Packages");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_REPOSITORIES.ordinal());
      cell.setCellValue("Manage Repositories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_SCRIPTS.ordinal());
      cell.setCellValue("Manage Scripts");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_SERVER_FILES.ordinal());
      cell.setCellValue("Manage Server Files");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_SESSIONS.ordinal());
      cell.setCellValue("Manage Sessions");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_SUMMARY_TBLS.ordinal());
      cell.setCellValue("Manage Summary Tables");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_TOOLS.ordinal());
      cell.setCellValue("Manage Tools");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_2FA_METHODS.ordinal());
      cell.setCellValue("Manage 2FA Methods");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MANAGE_USERS.ordinal());
      cell.setCellValue("Manage Users");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.MOBILE_DEVICE_LOGIN.ordinal());
      cell.setCellValue("Manage Device Login");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.OBJECT_CATEGORIES.ordinal());
      cell.setCellValue("Object Categories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.OWN_SCHEDULED_TASKS.ordinal());
      cell.setCellValue("Own Scheduled Tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.PERSISTENT_STORAGE.ordinal());
      cell.setCellValue("Persistent Storage");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.READ_SERVER_FILES.ordinal());
      cell.setCellValue("Read Server Files");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.REGISTER_AGENTS.ordinal());
      cell.setCellValue("Register Agents");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.REPORTING_SERVER.ordinal());
      cell.setCellValue("Reporting Server");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.SCHEDULE_FILE_UPLOAD.ordinal());
      cell.setCellValue("Schedule File Upload");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.SCHEDULE_MAINTENANCE.ordinal());
      cell.setCellValue("Schedule Maintenance");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.SCHEDULE_SCRIPT.ordinal());
      cell.setCellValue("Schedule Scripts");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.SEND_NOTIFICATION.ordinal());
      cell.setCellValue("Send Notifications");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.SERVER_CONFIG.ordinal());
      cell.setCellValue("Server Config");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.SERVER_CONSOLE.ordinal());
      cell.setCellValue("Server Console");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.SETUP_TCP_PROXY.ordinal());
      cell.setCellValue("Setup TCP Proxy");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.SSH_KEY_CONFIGURATION.ordinal());
      cell.setCellValue("SSH Keys");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.UA_NOTIFICATIONS.ordinal());
      cell.setCellValue("User Agent Notifications");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.UNLINK_ISSUES.ordinal());
      cell.setCellValue("Unlink Issues");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.USER_SCHEDULED_TASKS.ordinal());
      cell.setCellValue("User Scheduled Tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.VIEW_ALL_ALARMS.ordinal());
      cell.setCellValue("Vikew All Alarms");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.VIEW_ASSET_CHANGE_LOG.ordinal());
      cell.setCellValue("Asset Change Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.VIEW_AUDIT_LOG.ordinal());
      cell.setCellValue("Audit Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.VIEW_EVENT_DB.ordinal());
      cell.setCellValue("View Event Templates");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.VIEW_EVENT_LOG.ordinal());
      cell.setCellValue("Event Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.VIEW_REPOSITORIES.ordinal());
      cell.setCellValue("View Repositories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.VIEW_SYSLOG.ordinal());
      cell.setCellValue("Syslog");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.VIEW_TRAP_LOG.ordinal());
      cell.setCellValue("SNMP Trap Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(UsersSheetCells.WEB_SERVICE_DEFINITIONS.ordinal());
      cell.setCellValue("Web Service Definitions");
      cell.setCellStyle(headerStyle);
      
      sheet.setColumnWidth(UsersSheetCells.ID.ordinal(), 2048);
      sheet.setColumnWidth(UsersSheetCells.LOGIN.ordinal(), 3072);
      sheet.setColumnWidth(UsersSheetCells.ACTIVE.ordinal(), 2048);
      sheet.setColumnWidth(UsersSheetCells.FULL_NAME.ordinal(), 6144);
      sheet.setColumnWidth(UsersSheetCells.DESCRIPTION.ordinal(), 6144);
      sheet.setColumnWidth(UsersSheetCells.ORIGIN.ordinal(), 2048);
      sheet.setColumnWidth(UsersSheetCells.GROUPS.ordinal(), 6144);

      AbstractUserObject[] userDatabaseObjects = session.getUserDatabaseObjects();
      Arrays.stream(userDatabaseObjects).filter(p -> p instanceof User).sorted(Comparator.comparing(AbstractUserObject::getName)).forEach(element -> {
         User user = (User)element;
         HSSFRow row = sheet.createRow(rowNum.getAndIncrement());
         row.createCell(UsersSheetCells.ID.ordinal()).setCellValue(user.getId());
         row.createCell(UsersSheetCells.LOGIN.ordinal()).setCellValue(user.getName());
         row.createCell(UsersSheetCells.ACTIVE.ordinal()).setCellValue(user.isDisabled() ? "YES" : "NO");
         row.createCell(UsersSheetCells.FULL_NAME.ordinal()).setCellValue(user.getFullName());
         row.createCell(UsersSheetCells.DESCRIPTION.ordinal()).setCellValue(user.getDescription());
         row.createCell(UsersSheetCells.ORIGIN.ordinal()).setCellValue((user.getFlags() & AbstractUserObject.LDAP_USER) != 0 ? "LDAP" : "Local");

         boolean firstRow = true;
         for(long groupId : user.getGroups())
         {
            final HSSFRow groupRow;
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

         createPermissionCell(row, UsersSheetCells.ALL_SCHEDULED_TASKS, UserAccessRights.SYSTEM_ACCESS_ALL_SCHEDULED_TASKS, user);
         createPermissionCell(row, UsersSheetCells.AM_ATTRIBUTE_MANAGE, UserAccessRights.SYSTEM_ACCESS_AM_ATTRIBUTE_MANAGE, user);
         createPermissionCell(row, UsersSheetCells.CONFIGURE_TRAPS, UserAccessRights.SYSTEM_ACCESS_CONFIGURE_TRAPS, user);
         createPermissionCell(row, UsersSheetCells.DELETE_ALARMS, UserAccessRights.SYSTEM_ACCESS_DELETE_ALARMS, user);
         createPermissionCell(row, UsersSheetCells.EDIT_EVENT_DB, UserAccessRights.SYSTEM_ACCESS_EDIT_EVENT_DB, user);
         createPermissionCell(row, UsersSheetCells.EPP, UserAccessRights.SYSTEM_ACCESS_EPP, user);
         createPermissionCell(row, UsersSheetCells.EXTERNAL_INTEGRATION, UserAccessRights.SYSTEM_ACCESS_EXTERNAL_INTEGRATION, user);
         createPermissionCell(row, UsersSheetCells.IMPORT_CONFIGURATION, UserAccessRights.SYSTEM_ACCESS_IMPORT_CONFIGURATION, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_ACTIONS, UserAccessRights.SYSTEM_ACCESS_MANAGE_ACTIONS, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_AGENT_CFG, UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_CFG, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_GEO_AREAS, UserAccessRights.SYSTEM_ACCESS_MANAGE_GEO_AREAS, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_IMAGE_LIB, UserAccessRights.SYSTEM_ACCESS_MANAGE_IMAGE_LIB, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_MAPPING_TBLS, UserAccessRights.SYSTEM_ACCESS_MANAGE_MAPPING_TBLS, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_OBJECT_QUERIES, UserAccessRights.SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_PACKAGES, UserAccessRights.SYSTEM_ACCESS_MANAGE_PACKAGES, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_REPOSITORIES, UserAccessRights.SYSTEM_ACCESS_MANAGE_REPOSITORIES, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_SCRIPTS, UserAccessRights.SYSTEM_ACCESS_MANAGE_SCRIPTS, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_SERVER_FILES, UserAccessRights.SYSTEM_ACCESS_MANAGE_SERVER_FILES, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_SESSIONS, UserAccessRights.SYSTEM_ACCESS_MANAGE_SESSIONS, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_SUMMARY_TBLS, UserAccessRights.SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_TOOLS, UserAccessRights.SYSTEM_ACCESS_MANAGE_TOOLS, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_2FA_METHODS, UserAccessRights.SYSTEM_ACCESS_MANAGE_2FA_METHODS, user);
         createPermissionCell(row, UsersSheetCells.MANAGE_USERS, UserAccessRights.SYSTEM_ACCESS_MANAGE_USERS, user);
         createPermissionCell(row, UsersSheetCells.MOBILE_DEVICE_LOGIN, UserAccessRights.SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN, user);
         createPermissionCell(row, UsersSheetCells.OBJECT_CATEGORIES, UserAccessRights.SYSTEM_ACCESS_OBJECT_CATEGORIES, user);
         createPermissionCell(row, UsersSheetCells.OWN_SCHEDULED_TASKS, UserAccessRights.SYSTEM_ACCESS_OWN_SCHEDULED_TASKS, user);
         createPermissionCell(row, UsersSheetCells.PERSISTENT_STORAGE, UserAccessRights.SYSTEM_ACCESS_PERSISTENT_STORAGE, user);
         createPermissionCell(row, UsersSheetCells.READ_SERVER_FILES, UserAccessRights.SYSTEM_ACCESS_READ_SERVER_FILES, user);
         createPermissionCell(row, UsersSheetCells.REGISTER_AGENTS, UserAccessRights.SYSTEM_ACCESS_REGISTER_AGENTS, user);
         createPermissionCell(row, UsersSheetCells.REPORTING_SERVER, UserAccessRights.SYSTEM_ACCESS_REPORTING_SERVER, user);
         createPermissionCell(row, UsersSheetCells.SCHEDULE_FILE_UPLOAD, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD, user);
         createPermissionCell(row, UsersSheetCells.SCHEDULE_MAINTENANCE, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_MAINTENANCE, user);
         createPermissionCell(row, UsersSheetCells.SCHEDULE_SCRIPT, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_SCRIPT, user);
         createPermissionCell(row, UsersSheetCells.SEND_NOTIFICATION, UserAccessRights.SYSTEM_ACCESS_SEND_NOTIFICATION, user);
         createPermissionCell(row, UsersSheetCells.SERVER_CONFIG, UserAccessRights.SYSTEM_ACCESS_SERVER_CONFIG, user);
         createPermissionCell(row, UsersSheetCells.SERVER_CONSOLE, UserAccessRights.SYSTEM_ACCESS_SERVER_CONSOLE, user);
         createPermissionCell(row, UsersSheetCells.SETUP_TCP_PROXY, UserAccessRights.SYSTEM_ACCESS_SETUP_TCP_PROXY, user);
         createPermissionCell(row, UsersSheetCells.SSH_KEY_CONFIGURATION, UserAccessRights.SYSTEM_ACCESS_SSH_KEY_CONFIGURATION, user);
         createPermissionCell(row, UsersSheetCells.UA_NOTIFICATIONS, UserAccessRights.SYSTEM_ACCESS_UA_NOTIFICATIONS, user);
         createPermissionCell(row, UsersSheetCells.UNLINK_ISSUES, UserAccessRights.SYSTEM_ACCESS_UNLINK_ISSUES, user);
         createPermissionCell(row, UsersSheetCells.USER_SCHEDULED_TASKS, UserAccessRights.SYSTEM_ACCESS_USER_SCHEDULED_TASKS, user);
         createPermissionCell(row, UsersSheetCells.VIEW_ALL_ALARMS, UserAccessRights.SYSTEM_ACCESS_VIEW_ALL_ALARMS, user);
         createPermissionCell(row, UsersSheetCells.VIEW_ASSET_CHANGE_LOG, UserAccessRights.SYSTEM_ACCESS_VIEW_ASSET_CHANGE_LOG, user);
         createPermissionCell(row, UsersSheetCells.VIEW_AUDIT_LOG, UserAccessRights.SYSTEM_ACCESS_VIEW_AUDIT_LOG, user);
         createPermissionCell(row, UsersSheetCells.VIEW_EVENT_DB, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_DB, user);
         createPermissionCell(row, UsersSheetCells.VIEW_EVENT_LOG, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG, user);
         createPermissionCell(row, UsersSheetCells.VIEW_REPOSITORIES, UserAccessRights.SYSTEM_ACCESS_VIEW_REPOSITORIES, user);
         createPermissionCell(row, UsersSheetCells.VIEW_SYSLOG, UserAccessRights.SYSTEM_ACCESS_VIEW_SYSLOG, user);
         createPermissionCell(row, UsersSheetCells.VIEW_TRAP_LOG, UserAccessRights.SYSTEM_ACCESS_VIEW_TRAP_LOG, user);
         createPermissionCell(row, UsersSheetCells.WEB_SERVICE_DEFINITIONS, UserAccessRights.SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS, user);
      });
   }

   /**
    * Create single cell for system access
    *
    * @param row workbook's row
    * @param cellId cell ID
    * @param accessBit access bit to test
    * @param user user or group
    */
   private void createPermissionCell(HSSFRow row, UsersSheetCells cellId, long accessBit, AbstractUserObject userObject)
   {
      row.createCell(cellId.ordinal()).setCellValue(((userObject.getSystemRights() & accessBit) != 0) ? "YES" : "NO");
   }

   /**
    * Generate "Groups" sheet.
    *
    * @param wb
    * @param headerStyle
    */
   private void generateGroupsSheet(HSSFWorkbook wb, HSSFCellStyle headerStyle)
   {
      AtomicInteger rowNum = new AtomicInteger(0);
      HSSFSheet sheet = wb.getSheet(SHEET_GROUPS);
      HSSFRow headerRow = sheet.createRow(rowNum.getAndIncrement());

      HSSFCell cell = headerRow.createCell(GroupsSheetCells.ID.ordinal());
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

      cell = headerRow.createCell(GroupsSheetCells.ALL_SCHEDULED_TASKS.ordinal());
      cell.setCellValue("All Scheduled tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.AM_ATTRIBUTE_MANAGE.ordinal());
      cell.setCellValue("Asset Mgmt Attributes");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.CONFIGURE_TRAPS.ordinal());
      cell.setCellValue("SNMP Traps");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.DELETE_ALARMS.ordinal());
      cell.setCellValue("Delete Alarms");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.EDIT_EVENT_DB.ordinal());
      cell.setCellValue("Event Templates");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.EPP.ordinal());
      cell.setCellValue("EPP");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.EXTERNAL_INTEGRATION.ordinal());
      cell.setCellValue("External Integration");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.IMPORT_CONFIGURATION.ordinal());
      cell.setCellValue("Import Config");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_ACTIONS.ordinal());
      cell.setCellValue("Manage Actions");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_AGENT_CFG.ordinal());
      cell.setCellValue("Managet Agent Configs");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_GEO_AREAS.ordinal());
      cell.setCellValue("Manage Geo Areas");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_IMAGE_LIB.ordinal());
      cell.setCellValue("Manage Image Library");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_MAPPING_TBLS.ordinal());
      cell.setCellValue("Manage Mapping Tables");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_OBJECT_QUERIES.ordinal());
      cell.setCellValue("Manage Object Queries");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_PACKAGES.ordinal());
      cell.setCellValue("Manage Packages");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_REPOSITORIES.ordinal());
      cell.setCellValue("Manage Repositories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_SCRIPTS.ordinal());
      cell.setCellValue("Manage Scripts");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_SERVER_FILES.ordinal());
      cell.setCellValue("Manage Server Files");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_SESSIONS.ordinal());
      cell.setCellValue("Manage Sessions");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_SUMMARY_TBLS.ordinal());
      cell.setCellValue("Manage Summary Tables");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_TOOLS.ordinal());
      cell.setCellValue("Manage Tools");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_2FA_METHODS.ordinal());
      cell.setCellValue("Manage 2FA Methods");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MANAGE_USERS.ordinal());
      cell.setCellValue("Manage Users");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.MOBILE_DEVICE_LOGIN.ordinal());
      cell.setCellValue("Manage Device Login");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.OBJECT_CATEGORIES.ordinal());
      cell.setCellValue("Object Categories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.OWN_SCHEDULED_TASKS.ordinal());
      cell.setCellValue("Own Scheduled Tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.PERSISTENT_STORAGE.ordinal());
      cell.setCellValue("Persistent Storage");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.READ_SERVER_FILES.ordinal());
      cell.setCellValue("Read Server Files");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.REGISTER_AGENTS.ordinal());
      cell.setCellValue("Register Agents");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.REPORTING_SERVER.ordinal());
      cell.setCellValue("Reporting Server");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.SCHEDULE_FILE_UPLOAD.ordinal());
      cell.setCellValue("Schedule File Upload");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.SCHEDULE_MAINTENANCE.ordinal());
      cell.setCellValue("Schedule Maintenance");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.SCHEDULE_SCRIPT.ordinal());
      cell.setCellValue("Schedule Scripts");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.SEND_NOTIFICATION.ordinal());
      cell.setCellValue("Send Notifications");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.SERVER_CONFIG.ordinal());
      cell.setCellValue("Server Config");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.SERVER_CONSOLE.ordinal());
      cell.setCellValue("Server Console");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.SETUP_TCP_PROXY.ordinal());
      cell.setCellValue("Setup TCP Proxy");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.SSH_KEY_CONFIGURATION.ordinal());
      cell.setCellValue("SSH Keys");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.UA_NOTIFICATIONS.ordinal());
      cell.setCellValue("User Agent Notifications");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.UNLINK_ISSUES.ordinal());
      cell.setCellValue("Unlink Issues");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.USER_SCHEDULED_TASKS.ordinal());
      cell.setCellValue("User Scheduled Tasks");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.VIEW_ALL_ALARMS.ordinal());
      cell.setCellValue("Vikew All Alarms");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.VIEW_ASSET_CHANGE_LOG.ordinal());
      cell.setCellValue("Asset Change Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.VIEW_AUDIT_LOG.ordinal());
      cell.setCellValue("Audit Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.VIEW_EVENT_DB.ordinal());
      cell.setCellValue("View Event Templates");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.VIEW_EVENT_LOG.ordinal());
      cell.setCellValue("Event Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.VIEW_REPOSITORIES.ordinal());
      cell.setCellValue("View Repositories");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.VIEW_SYSLOG.ordinal());
      cell.setCellValue("Syslog");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.VIEW_TRAP_LOG.ordinal());
      cell.setCellValue("SNMP Trap Log");
      cell.setCellStyle(headerStyle);

      cell = headerRow.createCell(GroupsSheetCells.WEB_SERVICE_DEFINITIONS.ordinal());
      cell.setCellValue("Web Service Definitions");
      cell.setCellStyle(headerStyle);

      sheet.setColumnWidth(GroupsSheetCells.ID.ordinal(), 3072);
      sheet.setColumnWidth(GroupsSheetCells.NAME.ordinal(), 6144);
      sheet.setColumnWidth(GroupsSheetCells.DESCRIPTION.ordinal(), 6144);
      sheet.setColumnWidth(GroupsSheetCells.MEMBERS.ordinal(), 4096);

      Arrays.stream(session.getUserDatabaseObjects()).filter(p -> p instanceof UserGroup).sorted(Comparator.comparing(AbstractUserObject::getName)).forEach(element -> {
         UserGroup group = (UserGroup)element;
         HSSFRow row = sheet.createRow(rowNum.getAndIncrement());

         row.createCell(GroupsSheetCells.ID.ordinal()).setCellValue(group.getId());
         row.createCell(GroupsSheetCells.NAME.ordinal()).setCellValue(group.getName());
         row.createCell(GroupsSheetCells.DESCRIPTION.ordinal()).setCellValue(group.getDescription());

         boolean firstRow = true;
         for(long memberId : group.getMembers())
         {
            final HSSFRow memberRow;
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

         createPermissionCell(row, GroupsSheetCells.ALL_SCHEDULED_TASKS, UserAccessRights.SYSTEM_ACCESS_ALL_SCHEDULED_TASKS, group);
         createPermissionCell(row, GroupsSheetCells.AM_ATTRIBUTE_MANAGE, UserAccessRights.SYSTEM_ACCESS_AM_ATTRIBUTE_MANAGE, group);
         createPermissionCell(row, GroupsSheetCells.CONFIGURE_TRAPS, UserAccessRights.SYSTEM_ACCESS_CONFIGURE_TRAPS, group);
         createPermissionCell(row, GroupsSheetCells.DELETE_ALARMS, UserAccessRights.SYSTEM_ACCESS_DELETE_ALARMS, group);
         createPermissionCell(row, GroupsSheetCells.EDIT_EVENT_DB, UserAccessRights.SYSTEM_ACCESS_EDIT_EVENT_DB, group);
         createPermissionCell(row, GroupsSheetCells.EPP, UserAccessRights.SYSTEM_ACCESS_EPP, group);
         createPermissionCell(row, GroupsSheetCells.EXTERNAL_INTEGRATION, UserAccessRights.SYSTEM_ACCESS_EXTERNAL_INTEGRATION, group);
         createPermissionCell(row, GroupsSheetCells.IMPORT_CONFIGURATION, UserAccessRights.SYSTEM_ACCESS_IMPORT_CONFIGURATION, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_ACTIONS, UserAccessRights.SYSTEM_ACCESS_MANAGE_ACTIONS, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_AGENT_CFG, UserAccessRights.SYSTEM_ACCESS_MANAGE_AGENT_CFG, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_GEO_AREAS, UserAccessRights.SYSTEM_ACCESS_MANAGE_GEO_AREAS, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_IMAGE_LIB, UserAccessRights.SYSTEM_ACCESS_MANAGE_IMAGE_LIB, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_MAPPING_TBLS, UserAccessRights.SYSTEM_ACCESS_MANAGE_MAPPING_TBLS, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_OBJECT_QUERIES, UserAccessRights.SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_PACKAGES, UserAccessRights.SYSTEM_ACCESS_MANAGE_PACKAGES, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_REPOSITORIES, UserAccessRights.SYSTEM_ACCESS_MANAGE_REPOSITORIES, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_SCRIPTS, UserAccessRights.SYSTEM_ACCESS_MANAGE_SCRIPTS, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_SERVER_FILES, UserAccessRights.SYSTEM_ACCESS_MANAGE_SERVER_FILES, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_SESSIONS, UserAccessRights.SYSTEM_ACCESS_MANAGE_SESSIONS, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_SUMMARY_TBLS, UserAccessRights.SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_TOOLS, UserAccessRights.SYSTEM_ACCESS_MANAGE_TOOLS, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_2FA_METHODS, UserAccessRights.SYSTEM_ACCESS_MANAGE_2FA_METHODS, group);
         createPermissionCell(row, GroupsSheetCells.MANAGE_USERS, UserAccessRights.SYSTEM_ACCESS_MANAGE_USERS, group);
         createPermissionCell(row, GroupsSheetCells.MOBILE_DEVICE_LOGIN, UserAccessRights.SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN, group);
         createPermissionCell(row, GroupsSheetCells.OBJECT_CATEGORIES, UserAccessRights.SYSTEM_ACCESS_OBJECT_CATEGORIES, group);
         createPermissionCell(row, GroupsSheetCells.OWN_SCHEDULED_TASKS, UserAccessRights.SYSTEM_ACCESS_OWN_SCHEDULED_TASKS, group);
         createPermissionCell(row, GroupsSheetCells.PERSISTENT_STORAGE, UserAccessRights.SYSTEM_ACCESS_PERSISTENT_STORAGE, group);
         createPermissionCell(row, GroupsSheetCells.READ_SERVER_FILES, UserAccessRights.SYSTEM_ACCESS_READ_SERVER_FILES, group);
         createPermissionCell(row, GroupsSheetCells.REGISTER_AGENTS, UserAccessRights.SYSTEM_ACCESS_REGISTER_AGENTS, group);
         createPermissionCell(row, GroupsSheetCells.REPORTING_SERVER, UserAccessRights.SYSTEM_ACCESS_REPORTING_SERVER, group);
         createPermissionCell(row, GroupsSheetCells.SCHEDULE_FILE_UPLOAD, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD, group);
         createPermissionCell(row, GroupsSheetCells.SCHEDULE_MAINTENANCE, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_MAINTENANCE, group);
         createPermissionCell(row, GroupsSheetCells.SCHEDULE_SCRIPT, UserAccessRights.SYSTEM_ACCESS_SCHEDULE_SCRIPT, group);
         createPermissionCell(row, GroupsSheetCells.SEND_NOTIFICATION, UserAccessRights.SYSTEM_ACCESS_SEND_NOTIFICATION, group);
         createPermissionCell(row, GroupsSheetCells.SERVER_CONFIG, UserAccessRights.SYSTEM_ACCESS_SERVER_CONFIG, group);
         createPermissionCell(row, GroupsSheetCells.SERVER_CONSOLE, UserAccessRights.SYSTEM_ACCESS_SERVER_CONSOLE, group);
         createPermissionCell(row, GroupsSheetCells.SETUP_TCP_PROXY, UserAccessRights.SYSTEM_ACCESS_SETUP_TCP_PROXY, group);
         createPermissionCell(row, GroupsSheetCells.SSH_KEY_CONFIGURATION, UserAccessRights.SYSTEM_ACCESS_SSH_KEY_CONFIGURATION, group);
         createPermissionCell(row, GroupsSheetCells.UA_NOTIFICATIONS, UserAccessRights.SYSTEM_ACCESS_UA_NOTIFICATIONS, group);
         createPermissionCell(row, GroupsSheetCells.UNLINK_ISSUES, UserAccessRights.SYSTEM_ACCESS_UNLINK_ISSUES, group);
         createPermissionCell(row, GroupsSheetCells.USER_SCHEDULED_TASKS, UserAccessRights.SYSTEM_ACCESS_USER_SCHEDULED_TASKS, group);
         createPermissionCell(row, GroupsSheetCells.VIEW_ALL_ALARMS, UserAccessRights.SYSTEM_ACCESS_VIEW_ALL_ALARMS, group);
         createPermissionCell(row, GroupsSheetCells.VIEW_ASSET_CHANGE_LOG, UserAccessRights.SYSTEM_ACCESS_VIEW_ASSET_CHANGE_LOG, group);
         createPermissionCell(row, GroupsSheetCells.VIEW_AUDIT_LOG, UserAccessRights.SYSTEM_ACCESS_VIEW_AUDIT_LOG, group);
         createPermissionCell(row, GroupsSheetCells.VIEW_EVENT_DB, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_DB, group);
         createPermissionCell(row, GroupsSheetCells.VIEW_EVENT_LOG, UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG, group);
         createPermissionCell(row, GroupsSheetCells.VIEW_REPOSITORIES, UserAccessRights.SYSTEM_ACCESS_VIEW_REPOSITORIES, group);
         createPermissionCell(row, GroupsSheetCells.VIEW_SYSLOG, UserAccessRights.SYSTEM_ACCESS_VIEW_SYSLOG, group);
         createPermissionCell(row, GroupsSheetCells.VIEW_TRAP_LOG, UserAccessRights.SYSTEM_ACCESS_VIEW_TRAP_LOG, group);
         createPermissionCell(row, GroupsSheetCells.WEB_SERVICE_DEFINITIONS, UserAccessRights.SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS, group);
      });
   }

   /**
    * Create single cell for system access
    *
    * @param row workbook's row
    * @param cellId cell ID
    * @param accessBit access bit to test
    * @param user user or group
    */
   private void createPermissionCell(HSSFRow row, GroupsSheetCells cellId, long accessBit, AbstractUserObject userObject)
   {
      row.createCell(cellId.ordinal()).setCellValue(((userObject.getSystemRights() & accessBit) != 0) ? "YES" : "NO");
   }

   /**
    * Generate "Permissions" sheet.
    *
    * @param wb
    * @param headerStyle
    * @param permissions
    */
   private void generatePermissionsSheet(HSSFWorkbook wb, HSSFCellStyle headerStyle, List<ObjectAccess> permissions)
   {
      AtomicInteger rowNum = new AtomicInteger(0);
      HSSFSheet sheet = wb.getSheet(SHEET_PERMISSIONS);
      HSSFRow headerRow = sheet.createRow(rowNum.getAndIncrement());

      HSSFCell cell = headerRow.createCell(PermissionsSheetCells.OBJECT_NAME.ordinal());
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

      cell = headerRow.createCell(PermissionsSheetCells.MODIFY.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Modify");

      cell = headerRow.createCell(PermissionsSheetCells.DELETE.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Delete");

      cell = headerRow.createCell(PermissionsSheetCells.CONTROL.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Control");

      cell = headerRow.createCell(PermissionsSheetCells.MAINTENANCE.ordinal());
      cell.setCellStyle(headerStyle);
      cell.setCellValue("Enter/Leave Maintenance");

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
         HSSFRow row = sheet.createRow(rowNum.getAndIncrement());

         row.createCell(PermissionsSheetCells.OBJECT_NAME.ordinal()).setCellValue(element.name);
         row.createCell(PermissionsSheetCells.USER_OR_GROUP_NAME.ordinal()).setCellValue(element.userName);
         row.createCell(PermissionsSheetCells.INHERIT_ACCESS.ordinal()).setCellValue(element.inheritAccessRights ? "YES" : "NO");

         createPermissionCell(row, PermissionsSheetCells.READ, UserAccessRights.OBJECT_ACCESS_READ, element);
         createPermissionCell(row, PermissionsSheetCells.MODIFY, UserAccessRights.OBJECT_ACCESS_MODIFY, element);
         createPermissionCell(row, PermissionsSheetCells.DELETE, UserAccessRights.OBJECT_ACCESS_DELETE, element);
         createPermissionCell(row, PermissionsSheetCells.CONTROL, UserAccessRights.OBJECT_ACCESS_CONTROL, element);
         createPermissionCell(row, PermissionsSheetCells.MAINTENANCE, UserAccessRights.OBJECT_ACCESS_MAINTENANCE, element);
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

   /**
    * Create single cell on "Permissions" page.
    *
    * @param row workbook's row
    * @param cellId cell ID
    * @param accessBit access bit to test
    * @param element access list element
    */
   private void createPermissionCell(HSSFRow row, PermissionsSheetCells cellId, int accessBit, ObjectAccess element)
   {
      row.createCell(cellId.ordinal()).setCellValue(((element.accessRights & accessBit) != 0) ? "YES" : "NO");
   }

   private static void scanACL(AbstractObject currentObject, String parentFullName, List<ObjectAccess> results)
   {
      String currentFullName = parentFullName + "/" + currentObject.getObjectName();
      processObject(currentObject, currentFullName, results);
      for(AbstractObject abstractObject : currentObject.getChildrenAsArray())
      {
         if (abstractObject.hasChildren())
         {
            scanACL(abstractObject, currentFullName, results);
         }
      }
   }

   private static void processObject(AbstractObject currentObject, String fullName, List<ObjectAccess> results)
   {
      if (currentObject.isInheritAccessRights() && currentObject.getAccessList().length == 0)
      {
         // ignore objects without custom permissions
         return;
      }

      for(AccessListElement element : currentObject.getAccessList())
      {
         results.add(new ObjectAccess(fullName, currentObject.isInheritAccessRights(), element.getUserId(), element.getAccessRights()));
      }
   }

   /**
    * Object access information
    */
   private static class ObjectAccess
   {
      public String name;
      public long userId;
      public String userName;
      public boolean inheritAccessRights;
      public int accessRights;

      public ObjectAccess(String name, boolean inheritAccessRights, long userId, int accessRights)
      {
         this.name = name;
         this.inheritAccessRights = inheritAccessRights;
         this.userId = userId;
         this.accessRights = accessRights;
      }
   }
}
