/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.widgets.UserSelector;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog to show effective access rights for an object
 */
public class EffectiveRightsDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EffectiveRightsDialog.class);

   private AbstractObject object;
   private NXCSession session;
   private boolean canSelectUser;
   private UserSelector userSelector;
   private Table rightsTable;
   private List<RightDefinition> rightDefinitions;
   private int currentEffectiveRights = 0;
   private Image checkImage;
   private Image uncheckImage;

   /**
    * Right definition for display
    */
   private static class RightDefinition
   {
      int mask;
      String label;

      RightDefinition(int mask, String label)
      {
         this.mask = mask;
         this.label = label;
      }
   }

   /**
    * Constructor
    *
    * @param parentShell parent shell
    * @param object the object to show rights for
    */
   public EffectiveRightsDialog(Shell parentShell, AbstractObject object)
   {
      super(parentShell);
      this.object = object;
      this.session = Registry.getSession();
      this.canSelectUser = (object.getEffectiveRights() & UserAccessRights.OBJECT_ACCESS_ACL) != 0;
      initRightDefinitions();

      checkImage = ResourceManager.getImage("icons/active.png");
      uncheckImage = ResourceManager.getImage("icons/inactive.png");

      parentShell.addDisposeListener((e) -> {
         checkImage.dispose();
         uncheckImage.dispose();
      });
   }

   /**
    * Initialize the list of right definitions
    */
   private void initRightDefinitions()
   {
      rightDefinitions = new ArrayList<>();
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_READ, i18n.tr("Read")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_READ_AGENT, i18n.tr("Read agent data")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_READ_SNMP, i18n.tr("Read SNMP data")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_MODIFY, i18n.tr("Modify")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_EDIT_COMMENTS, i18n.tr("Edit comments")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_EDIT_RESP_USERS, i18n.tr("Manage responsible users")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_CREATE, i18n.tr("Create child objects")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_DELEGATED_READ, i18n.tr("Delegated read")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_DELETE, i18n.tr("Delete")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_CONTROL, i18n.tr("Control")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_SEND_EVENTS, i18n.tr("Send events")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_READ_ALARMS, i18n.tr("View alarms")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_UPDATE_ALARMS, i18n.tr("Update alarms")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_TERM_ALARMS, i18n.tr("Terminate alarms")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_CREATE_ISSUE, i18n.tr("Create helpdesk tickets")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_PUSH_DATA, i18n.tr("Push data")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_ACL, i18n.tr("Access control")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_DOWNLOAD, i18n.tr("Download files")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_UPLOAD, i18n.tr("Upload files")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_MANAGE_FILES, i18n.tr("Manage files")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_CONFIGURE_AGENT, i18n.tr("Configure agent")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_SCREENSHOT, i18n.tr("Take screenshot")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_MAINTENANCE, i18n.tr("Control maintenance mode")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_EDIT_MNT_JOURNAL, i18n.tr("Edit maintenance journal")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_MANAGE_POLICIES, i18n.tr("Manage policies")));
      rightDefinitions.add(new RightDefinition(UserAccessRights.OBJECT_ACCESS_MANAGE_INCIDENTS, i18n.tr("Manage incidents")));
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Effective Rights"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.OK_ID, i18n.tr("Close"), true);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      // Object name display
      LabeledText objectNameField = new LabeledText(dialogArea, SWT.NONE);
      objectNameField.setLabel(i18n.tr("Object"));
      objectNameField.setText(object.getObjectName());
      objectNameField.setEditable(false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      objectNameField.setLayoutData(gd);

      userSelector = new UserSelector(dialogArea, SWT.NONE);
      userSelector.setLabel(i18n.tr("Select user"));
      userSelector.setUserId(session.getUserId());
      userSelector.setEnabled(canSelectUser);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      userSelector.setLayoutData(gd);
      userSelector.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            refreshEffectiveRights();
         }
      });

      if (!canSelectUser)
      {
         Label hint = new Label(dialogArea, SWT.NONE);
         hint.setText(i18n.tr("(You need Access Control permission to view other users' rights)"));
         hint.setForeground(parent.getDisplay().getSystemColor(SWT.COLOR_DARK_GRAY));
      }

      // Rights table
      Label tableLabel = new Label(dialogArea, SWT.NONE);
      tableLabel.setText(i18n.tr("Effective Access Rights:"));

      rightsTable = new Table(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      rightsTable.setHeaderVisible(false);
      rightsTable.setLinesVisible(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      // gd.heightHint = 400;
      rightsTable.setLayoutData(gd);

      TableColumn colStatus = new TableColumn(rightsTable, SWT.CENTER);
      colStatus.setWidth(24);

      TableColumn colLabel = new TableColumn(rightsTable, SWT.LEFT);
      colLabel.setWidth(350);

      // Populate table with right definitions
      for(RightDefinition rd : rightDefinitions)
      {
         TableItem item = new TableItem(rightsTable, SWT.NONE);
         item.setText(1, rd.label);
         item.setData(rd);
      }

      // Load initial rights
      refreshEffectiveRights();

      return dialogArea;
   }

   /**
    * Refresh effective rights from server
    */
   private void refreshEffectiveRights()
   {
      final int userId = userSelector.getUserId();
      if (userId == 0)
         return;

      Job job = new Job(i18n.tr("Loading effective rights"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final int rights = session.getEffectiveRights(object.getObjectId(), userId);
            runInUIThread(() -> {
               if (rightsTable.isDisposed())
                  return;
               currentEffectiveRights = rights;
               updateRightsDisplay();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load effective access rights");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Update the table to reflect current effective rights
    */
   private void updateRightsDisplay()
   {
      for(TableItem item : rightsTable.getItems())
      {
         RightDefinition rd = (RightDefinition)item.getData();
         boolean granted = (currentEffectiveRights & rd.mask) != 0;
         item.setImage(0, granted ? checkImage : uncheckImage);
         item.setForeground(1, granted ? null : WidgetHelper.getSystemColor(WidgetHelper.COLOR_WIDGET_DISABLED_FOREGROUND));
      }
   }
}
