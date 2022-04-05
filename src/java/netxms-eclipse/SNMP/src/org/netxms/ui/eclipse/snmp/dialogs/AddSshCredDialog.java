/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.snmp.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.SshCredential;
import org.netxms.client.SshKeyPair;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Add USM credentials dialog
 */
public class AddSshCredDialog extends Dialog
{
   private LabeledText login;
   private LabeledText password;
   private Combo key;

   private SshCredential cred;
   private List<SshKeyPair> keyList;
			
	/**
    * @param parentShell parent shell
    * @param cred currently chosen credential
    * @param keyList list of available SSH keys
    */
   public AddSshCredDialog(Shell parentShell, SshCredential cred, List<SshKeyPair> keyList)
	{
		super(parentShell);
		this.cred = cred;
      this.keyList = keyList;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      if (cred == null)
         newShell.setText("Add SSH credential");
      else
         newShell.setText("Edit SSH credential");
	}

	/* (non-Javadoc)
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

      password = new LabeledText(dialogArea, SWT.NONE);
      password.setLabel("Password");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      password.setLayoutData(gd);

      key = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "SSH Key", WidgetHelper.DEFAULT_LAYOUT_DATA);

      int initialKeyIndex = 0;
      key.add(" ");
      for(int i = 0; i < keyList.size(); i++)
      {
         key.add(keyList.get(i).getName());
         if (cred != null && keyList.get(i).getId() == cred.getKeyId())
            initialKeyIndex = i + 1;
      }

      if (cred != null)
      {
         login.setText(cred.getLogin());
         password.setText(cred.getPassword());
      }

      key.select(initialKeyIndex);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
      int index = key.getSelectionIndex();
      int keyId = (index > 0) ? keyList.get(index - 1).getId() : 0;

      if (login.getText().trim().equals(""))
      {
         MessageDialogHelper.openError(getShell(), "Error", "Please enter login.");
         return;
      }

      if (password.getText().trim().equals("") && index == 0)
      {
         MessageDialogHelper.openError(getShell(), "Error", "Please enter password and/or key.");
         return;
      }

      if (cred == null)
      {
         cred = new SshCredential(login.getText().trim(), password.getText().trim(), keyId);
      }
      else
      {
         cred.setLogin(login.getText().trim());
         cred.setPassword(password.getText().trim());
         cred.setKeyId(keyId);
      }
      super.okPressed();
   }

	/**
	 * @return the value
	 */
   public SshCredential getValue()
	{
		return cred;
	}
}
