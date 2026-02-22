/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ObjectNameValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Cloud domain object creation dialog
 */
public class CreateCloudDomainDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CreateCloudDomainDialog.class);

   private LabeledText nameField;
   private LabeledText aliasField;
   private LabeledText connectorNameField;
   private LabeledText accountIdentifierField;
   private LabeledText credentialsField;

   private String name;
   private String alias;
   private String connectorName;
   private String accountIdentifier;
   private String credentials;

   /**
    * @param parentShell
    */
   public CreateCloudDomainDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Cloud Domain"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      nameField = new LabeledText(dialogArea, SWT.NONE);
      nameField.setLabel(i18n.tr("Name"));
      nameField.getTextControl().setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      gd.horizontalSpan = 2;
      nameField.setLayoutData(gd);

      aliasField = new LabeledText(dialogArea, SWT.NONE);
      aliasField.setLabel(i18n.tr("Alias"));
      aliasField.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      aliasField.setLayoutData(gd);

      connectorNameField = new LabeledText(dialogArea, SWT.NONE);
      connectorNameField.setLabel(i18n.tr("Connector name"));
      connectorNameField.getTextControl().setTextLimit(255);
      connectorNameField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      accountIdentifierField = new LabeledText(dialogArea, SWT.NONE);
      accountIdentifierField.setLabel(i18n.tr("Account identifier"));
      accountIdentifierField.getTextControl().setTextLimit(255);
      accountIdentifierField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      credentialsField = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.PASSWORD);
      credentialsField.setLabel(i18n.tr("Credentials"));
      credentialsField.getTextControl().setTextLimit(255);
      credentialsField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (!WidgetHelper.validateTextInput(nameField, new ObjectNameValidator()))
      {
         WidgetHelper.adjustWindowSize(this);
         return;
      }

      name = nameField.getText().trim();
      alias = aliasField.getText().trim();
      connectorName = connectorNameField.getText().trim();
      accountIdentifier = accountIdentifierField.getText().trim();
      credentials = credentialsField.getText();

      super.okPressed();
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
   }

   /**
    * @return the connectorName
    */
   public String getConnectorName()
   {
      return connectorName;
   }

   /**
    * @return the accountIdentifier
    */
   public String getAccountIdentifier()
   {
      return accountIdentifier;
   }

   /**
    * @return the credentials
    */
   public String getCredentials()
   {
      return credentials;
   }
}
