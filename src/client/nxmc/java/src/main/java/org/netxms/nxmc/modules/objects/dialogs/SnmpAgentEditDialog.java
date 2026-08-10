/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import java.net.InetAddress;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.snmp.SnmpAgentConfiguration;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.PasswordInputField;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Edit dialog for additional SNMP agent configuration
 */
public class SnmpAgentEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SnmpAgentEditDialog.class);

   private SnmpAgentConfiguration configuration;
   private boolean createNew;
   private LabeledText name;
   private LabeledText address;
   private LabeledText port;
   private LabeledCombo version;
   private PasswordInputField authName;
   private LabeledCombo authMethod;
   private PasswordInputField authPassword;
   private LabeledCombo privMethod;
   private PasswordInputField privPassword;
   private LabeledText contextName;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param configuration agent configuration to edit
    * @param createNew true if new agent is being created
    */
   public SnmpAgentEditDialog(Shell parentShell, SnmpAgentConfiguration configuration, boolean createNew)
   {
      super(parentShell);
      this.configuration = configuration;
      this.createNew = createNew;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(createNew ? i18n.tr("Add SNMP Agent") : i18n.tr("Edit SNMP Agent"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Name"));
      name.setText(configuration.getName());
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1);
      gd.widthHint = 400;
      name.setLayoutData(gd);

      address = new LabeledText(dialogArea, SWT.NONE);
      address.setLabel(i18n.tr("IP address (empty to use node's primary IP address)"));
      address.setText((configuration.getAddress() != null) ? configuration.getAddress().getHostAddress() : "");
      address.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      port = new LabeledText(dialogArea, SWT.NONE);
      port.setLabel(i18n.tr("UDP Port"));
      port.setText(Integer.toString(configuration.getPort()));
      port.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      version = new LabeledCombo(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      version.setLabel(i18n.tr("Version"));
      version.add("1");
      version.add("2c");
      version.add("3");
      version.select(snmpVersionToIndex(configuration.getVersion()));
      version.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
      version.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onSnmpVersionChange();
         }
      });

      contextName = new LabeledText(dialogArea, SWT.NONE);
      contextName.setLabel(i18n.tr("Context"));
      contextName.setText(configuration.getContextName());
      contextName.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      boolean isV3 = (configuration.getVersion() == SnmpVersion.V3);

      authName = new PasswordInputField(dialogArea, SWT.NONE);
      authName.setLabel(isV3 ? i18n.tr("User name") : i18n.tr("Community string"));
      authName.setText(configuration.getAuthName());
      authName.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      authMethod = new LabeledCombo(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      authMethod.setLabel(i18n.tr("Authentication"));
      authMethod.add(i18n.tr("NONE"));
      authMethod.add("MD5");
      authMethod.add("SHA1");
      authMethod.add("SHA224");
      authMethod.add("SHA256");
      authMethod.add("SHA384");
      authMethod.add("SHA512");
      authMethod.select(configuration.getAuthMethod());
      authMethod.setEnabled(isV3);
      authMethod.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      authPassword = new PasswordInputField(dialogArea, SWT.NONE);
      authPassword.setLabel(i18n.tr("Authentication password"));
      authPassword.setText(configuration.getAuthPassword());
      authPassword.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      authPassword.setInputControlsEnabled(isV3);

      privMethod = new LabeledCombo(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      privMethod.setLabel(i18n.tr("Encryption"));
      privMethod.add(i18n.tr("NONE"));
      privMethod.add("DES");
      privMethod.add("AES-128");
      privMethod.add("AES-192");
      privMethod.add("AES-256");
      privMethod.select(configuration.getPrivMethod());
      privMethod.setEnabled(isV3);
      privMethod.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      privPassword = new PasswordInputField(dialogArea, SWT.NONE);
      privPassword.setLabel(i18n.tr("Encryption password"));
      privPassword.setText(configuration.getPrivPassword());
      privPassword.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      privPassword.setInputControlsEnabled(isV3);

      return dialogArea;
   }

   /**
    * Convert SNMP version to index in combo box
    *
    * @param version SNMP version
    * @return index in combo box
    */
   private int snmpVersionToIndex(SnmpVersion version)
   {
      switch(version)
      {
         case V1:
            return 0;
         case V2C:
            return 1;
         case V3:
            return 2;
         default:
            return 1;
      }
   }

   /**
    * Handler for SNMP version change
    */
   private void onSnmpVersionChange()
   {
      boolean isV3 = (version.getSelectionIndex() == 2);
      authName.setLabel(isV3 ? i18n.tr("User name") : i18n.tr("Community string"));
      authMethod.setEnabled(isV3);
      privMethod.setEnabled(isV3);
      authPassword.setInputControlsEnabled(isV3);
      privPassword.setInputControlsEnabled(isV3);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      String n = name.getText().trim();
      if (n.isEmpty())
      {
         MessageDialog.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter agent name"));
         return;
      }

      InetAddress a = null;
      String addressText = address.getText().trim();
      if (!addressText.isEmpty())
      {
         try
         {
            a = InetAddress.getByName(addressText);
         }
         catch(Exception e)
         {
            MessageDialog.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid IP address"));
            return;
         }
      }

      int p;
      try
      {
         p = Integer.parseInt(port.getText().trim(), 10);
         if ((p < 1) || (p > 65535))
            throw new NumberFormatException();
      }
      catch(NumberFormatException e)
      {
         MessageDialog.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid SNMP port number"));
         return;
      }

      final SnmpVersion[] versions = { SnmpVersion.V1, SnmpVersion.V2C, SnmpVersion.V3 };
      configuration.setName(n);
      configuration.setAddress(a);
      configuration.setPort(p);
      configuration.setVersion(versions[version.getSelectionIndex()]);
      configuration.setAuthName(authName.getText());
      configuration.setAuthMethod(authMethod.getSelectionIndex());
      configuration.setAuthPassword(authPassword.getText());
      configuration.setPrivMethod(privMethod.getSelectionIndex());
      configuration.setPrivPassword(privPassword.getText());
      configuration.setContextName(contextName.getText().trim());

      super.okPressed();
   }
}
