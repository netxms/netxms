/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2023 Raden Soultions
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
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.reports.acl.constants.GroupsSheetCells;
import org.netxms.nxmc.modules.users.reports.acl.constants.PermissionsSheetCells;
import org.netxms.nxmc.modules.users.reports.acl.constants.UsersSheetCells;
import org.xnap.commons.i18n.I18n;

/**
 * Generate detailed object access report in form of Excel table
 */
public class AclReport
{
   private final I18n i18n = LocalizationHelper.getI18n(AclReport.class);

   private final String SHEET_USERS = i18n.tr("Users");
   private final String SHEET_GROUPS = i18n.tr("Groups");
   private final String SHEET_PERMISSIONS = i18n.tr("Object Permissions");

   private NXCSession session = Registry.getSession();
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
               row.userName += i18n.tr(" (group)");
            }
         }
         else
         {
            row.userName = String.format(i18n.tr("DELETED [%d]"), row.userId);
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
      });

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
      });
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
         row.createCell(PermissionsSheetCells.INHERIT_ACCESS.ordinal()).setCellValue(element.inheritAccessRights ? i18n.tr("YES") : i18n.tr("NO"));
         
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
      row.createCell(cellId.ordinal()).setCellValue(((element.accessRights & accessBit) != 0) ? i18n.tr("YES") : i18n.tr("NO"));
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
