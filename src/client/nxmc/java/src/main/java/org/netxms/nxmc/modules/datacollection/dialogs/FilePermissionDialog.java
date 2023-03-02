/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;

/**
 * Dialog for editing file permissions
 */
public class FilePermissionDialog extends Dialog
{
   private int permissions;
   private String owner;
   private String group;
   private LabeledText textOwner;
   private LabeledText textGroup;
   private Button[] bitSelectors;

   /**
    * Create new file permission edit dialog.
    *
    * @param parentShell parent shell
    * @param currentPermissions current file permissions
    * @param owner current owner
    * @param group current group
    */
   public FilePermissionDialog(Shell parentShell, int permissions, String owner, String group)
   {
      super(parentShell);
      this.permissions = permissions;
      this.owner = owner;
      this.group = group;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Permissions");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout gridLayout = new GridLayout(3, true);
      dialogArea.setLayout(gridLayout);

      textOwner = new LabeledText(dialogArea, SWT.NONE);
      textOwner.setLabel("Owner");
      textOwner.setText(owner);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      gd.widthHint = 400;
      textOwner.setLayoutData(gd);

      textGroup = new LabeledText(dialogArea, SWT.NONE);
      textGroup.setLabel("Group");
      textGroup.setText(group);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      gd.widthHint = 400;
      textGroup.setLayoutData(gd);

      bitSelectors = new Button[9];

      Group groupUser = new Group(dialogArea, SWT.NONE);
      groupUser.setText("User");
      groupUser.setLayout(new RowLayout(SWT.VERTICAL));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      groupUser.setLayoutData(gd);

      addBitSelector(groupUser, "Read", 0);
      addBitSelector(groupUser, "Write", 1);
      addBitSelector(groupUser, "Execute", 2);

      Group groupGroup = new Group(dialogArea, SWT.NONE);
      groupGroup.setText("Group");
      groupGroup.setLayout(new RowLayout(SWT.VERTICAL));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      groupGroup.setLayoutData(gd);

      addBitSelector(groupGroup, "Read", 3);
      addBitSelector(groupGroup, "Write", 4);
      addBitSelector(groupGroup, "Execute", 5);

      Group groupOther = new Group(dialogArea, SWT.NONE);
      groupOther.setText("Other");
      groupOther.setLayout(new RowLayout(SWT.VERTICAL));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      groupOther.setLayoutData(gd);

      addBitSelector(groupOther, "Read", 6);
      addBitSelector(groupOther, "Write", 7);
      addBitSelector(groupOther, "Execute", 8);

      return dialogArea;
   }

   /**
    * Add bit selector button.
    *
    * @param parent parent control
    * @param text text
    * @param bit bit number
    */
   private void addBitSelector(Composite parent, String text, int bit)
   {
      bitSelectors[bit] = new Button(parent, SWT.CHECK);
      bitSelectors[bit].setText(text);
      bitSelectors[bit].setSelection((permissions & (1 << bit)) != 0);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      owner = textOwner.getText().trim();
      group = textGroup.getText().trim();
      permissions = 0;
      for(int i = 0; i < bitSelectors.length; i++)
      {
         if (bitSelectors[i].getSelection())
            permissions |= 1 << i;
      }
      super.okPressed();
   }

   /**
    * Get configured permissions
    *
    * @return configured permissions
    */
   public int getPermissions()
   {
      return permissions;
   }

   /**
    * @return the owner
    */
   public String getOwner()
   {
      return owner;
   }

   /**
    * @return the group
    */
   public String getGroup()
   {
      return group;
   }
}
