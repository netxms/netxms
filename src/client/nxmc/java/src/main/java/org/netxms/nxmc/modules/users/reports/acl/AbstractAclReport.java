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

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import org.apache.poi.xssf.usermodel.XSSFCellStyle;
import org.apache.poi.xssf.usermodel.XSSFFont;
import org.apache.poi.xssf.usermodel.XSSFRow;
import org.apache.poi.xssf.usermodel.XSSFWorkbook;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.reports.acl.constants.PermissionsSheetCells;
import org.xnap.commons.i18n.I18n;

/**
 * Generate detailed object access report in form of Excel table
 */
public abstract class AbstractAclReport
{
   private final I18n i18n = LocalizationHelper.getI18n(AbstractAclReport.class);

   protected final String SHEET_USERS = i18n.tr("Users");
   protected final String SHEET_GROUPS = i18n.tr("Groups");
   protected final String SHEET_PERMISSIONS = i18n.tr("Object Permissions");

   protected NXCSession session = Registry.getSession();

   private String outputFileName;

   /**
    * Create new ACL report object.
    *
    * @param outputFileName name of output file
    */
   public AbstractAclReport(String outputFileName)
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

      XSSFWorkbook wb = new XSSFWorkbook();
      XSSFFont headerFont = wb.createFont();
      headerFont.setBold(true);
      XSSFCellStyle headerCellStyle = wb.createCellStyle();
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
    * Create single cell for system access
    *
    * @param row workbook's row
    * @param columnIndex column index
    * @param accessBit access bit to test
    * @param user user or group
    */
   protected void createPermissionCell(XSSFRow row, int columnIndex, long accessBit, AbstractUserObject userObject)
   {
      row.createCell(columnIndex).setCellValue(((userObject.getSystemRights() & accessBit) != 0) ? i18n.tr("YES") : i18n.tr("NO"));
   }

   /**
    * Generate "Users" sheet.
    *
    * @param wb
    * @param headerStyle
    */
   protected abstract void generateUserSheet(XSSFWorkbook wb, XSSFCellStyle headerStyle);

   /**
    * Generate "Groups" sheet.
    *
    * @param wb
    * @param headerStyle
    */
   protected abstract void generateGroupsSheet(XSSFWorkbook wb, XSSFCellStyle headerStyle);

   /**
    * Generate "Permissions" sheet.
    *
    * @param wb
    * @param headerStyle
    * @param permissions
    */
   protected abstract void generatePermissionsSheet(XSSFWorkbook wb, XSSFCellStyle headerStyle, List<ObjectAccess> permissions);

   /**
    * Create single cell on "Permissions" page.
    *
    * @param row workbook's row
    * @param cellId cell ID
    * @param accessBit access bit to test
    * @param element access list element
    */
   protected void createPermissionCell(XSSFRow row, PermissionsSheetCells cellId, int accessBit, ObjectAccess element)
   {
      row.createCell(cellId.ordinal()).setCellValue(((element.accessRights & accessBit) != 0) ? i18n.tr("YES") : i18n.tr("NO"));
   }

   /**
    * Scan ACL for given object and its children.
    *
    * @param currentObject object to start scan from
    * @param parentFullName full name of parent object
    * @param results list to store results
    */
   protected static void scanACL(AbstractObject currentObject, String parentFullName, List<ObjectAccess> results)
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

   /**
    * Process single object and add its access rights to results.
    *
    * @param currentObject object to process
    * @param fullName full name of object
    * @param results list to store results
    */
   protected static void processObject(AbstractObject currentObject, String fullName, List<ObjectAccess> results)
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
   protected static class ObjectAccess
   {
      public String name;
      public int userId;
      public String userName;
      public boolean inheritAccessRights;
      public int accessRights;

      public ObjectAccess(String name, boolean inheritAccessRights, int userId, int accessRights)
      {
         this.name = name;
         this.inheritAccessRights = inheritAccessRights;
         this.userId = userId;
         this.accessRights = accessRights;
      }
   }
}
