/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.SSHCredentials;
import org.netxms.client.SshKeyPair;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.PasswordInputField;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Add/edit SSH credentials dialog
 */
public class EditSSHCredentialsDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditSSHCredentialsDialog.class);

   private LabeledText login;
   private PasswordInputField password;
   private Combo key;
   private SSHCredentials credentials;
   private List<SshKeyPair> sshKeys;

	/**
    * @param parentShell parent shell
    * @param credentials currently chosen credential
    * @param sshKeys list of available SSH keys
    */
   public EditSSHCredentialsDialog(Shell parentShell, SSHCredentials credentials, List<SshKeyPair> sshKeys)
	{
		super(parentShell);
		this.credentials = credentials;
      this.sshKeys = sshKeys;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText((credentials == null) ? i18n.tr("Add SSH Credentials") : i18n.tr("Edit SSH Credentials"));
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
      dialogArea.setLayout(layout);

      login = new LabeledText(dialogArea, SWT.NONE);
      login.setLabel("Login");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      login.setLayoutData(gd);

      password = new PasswordInputField(dialogArea, SWT.NONE);
      password.setLabel("Password");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      password.setLayoutData(gd);

      key = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "SSH Key", WidgetHelper.DEFAULT_LAYOUT_DATA);

      key.add("");
      for(int i = 0; i < sshKeys.size(); i++)
      {
         SshKeyPair kp = sshKeys.get(i);
         key.add(kp.getName());
         if ((credentials != null) && (kp.getId() == credentials.getKeyId()))
            key.select(key.getItemCount() - 1);
      }

      if (credentials != null)
      {
         login.setText(credentials.getLogin());
         password.setText(credentials.getPassword());
      }

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
      int index = key.getSelectionIndex();
      int keyId = (index > 0) ? sshKeys.get(index - 1).getId() : 0;

      if (login.getText().trim().equals(""))
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Login should not be empty."));
         return;
      }

      if (password.getText().equals("") && (keyId == 0))
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Either password or key should be provided."));
         return;
      }

      if (credentials == null)
      {
         credentials = new SSHCredentials(login.getText().trim(), password.getText(), keyId);
      }
      else
      {
         credentials.setLogin(login.getText().trim());
         credentials.setPassword(password.getText().trim());
         credentials.setKeyId(keyId);
      }

      super.okPressed();
   }

	/**
	 * @return the value
	 */
   public SSHCredentials getCredentials()
	{
		return credentials;
	}
}
