/**
 * NetXMS - open source network management system
 * Copyright (C) 2022-2025 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.packages.PackageInfo;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for editing package metadata
 */
public class EditPackageMetadataDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(EditPackageMetadataDialog.class);
   
   private PackageInfo packageInfo;
   private LabeledText name;
   private LabeledText description;
   private LabeledText version;
   private LabeledText platform;
   private LabeledText command;
   private LabeledCombo type;

   /**
    * @param parentShell
    */
   public EditPackageMetadataDialog(Shell parentShell, PackageInfo packageInfo)
   {
      super(parentShell);
      this.packageInfo = packageInfo;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Edit Package Metadata"));
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
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Name"));
      name.setText(packageInfo.getName());
      name.getTextControl().setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      gd.grabExcessHorizontalSpace = true;
      name.setLayoutData(gd);

      version = new LabeledText(dialogArea, SWT.NONE);
      version.setLabel(i18n.tr("Version"));
      version.setText(packageInfo.getVersion());
      version.getTextControl().setTextLimit(31);
      version.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      platform = new LabeledText(dialogArea, SWT.NONE);
      platform.setLabel(i18n.tr("Platform"));
      platform.setText(packageInfo.getPlatform());
      platform.getTextControl().setTextLimit(63);
      platform.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      type = new LabeledCombo(dialogArea, SWT.NONE, SWT.BORDER | SWT.DROP_DOWN);
      type.setLabel(i18n.tr("Type"));
      type.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));
      type.add("agent-installer");
      type.add("deb");
      type.add("executable");
      type.add("msi");
      type.add("msp");
      type.add("msu");
      type.add("rpm");
      type.add("tgz");
      type.add("zip");
      type.setText(packageInfo.getType());
      type.setTextLimit(15);

      command = new LabeledText(dialogArea, SWT.NONE);
      command.setLabel(i18n.tr("Command"));
      command.setText(packageInfo.getCommand());
      command.getTextControl().setTextLimit(4000);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      gd.grabExcessHorizontalSpace = false;
      gd.widthHint = 300;
      command.setLayoutData(gd);

      description = new LabeledText(dialogArea, SWT.NONE);
      description.setLabel("Description");
      description.setText(packageInfo.getDescription());
      description.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      gd.grabExcessHorizontalSpace = false;
      gd.widthHint = 300;
      description.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      String packageName = name.getText().trim();
      if (packageName.isEmpty())
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Package name cannot be empty!"));
         return;
      }

      packageInfo.setName(packageName);
      packageInfo.setType(type.getText().trim());
      packageInfo.setVersion(version.getText().trim());
      packageInfo.setPlatform(platform.getText().trim());
      packageInfo.setCommand(command.getText().trim());
      packageInfo.setDescription(description.getText().trim());

      super.okPressed();
   }
}
