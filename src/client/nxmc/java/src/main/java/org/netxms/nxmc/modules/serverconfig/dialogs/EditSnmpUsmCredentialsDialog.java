/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.PasswordInputField;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Add USM credentials dialog
 */
public class EditSnmpUsmCredentialsDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditSnmpUsmCredentialsDialog.class);

	private LabeledText name;
	private PasswordInputField authPasswd;
	private PasswordInputField privPasswd;
   private LabeledText comment;
	private Combo authMethod;
	private Combo privMethod;
	private SnmpUsmCredential credentials;

	/**
	 * @param parentShell
	 * @param credentials 
	 */
	public EditSnmpUsmCredentialsDialog(Shell parentShell, SnmpUsmCredential credentials)
	{
		super(parentShell);
		this.credentials = credentials;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText((credentials == null) ? i18n.tr("Add SNMP USM Credentials") : i18n.tr("Edit SNMP USM Credentials"));
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
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);

		name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("User name"));
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		name.setLayoutData(gd);

      authMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Authentication"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      authMethod.add(i18n.tr("None"));
      authMethod.add("MD5");
      authMethod.add("SHA1");
      authMethod.add("SHA224");
      authMethod.add("SHA256");
      authMethod.add("SHA384");
      authMethod.add("SHA512");
		authMethod.select(2);

      privMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Encryption"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      privMethod.add(i18n.tr("None"));
      privMethod.add("DES");
      privMethod.add("AES-128");
      privMethod.add("AES-192");
      privMethod.add("AES-256");
		privMethod.select(2);
		
		authPasswd = new PasswordInputField(dialogArea, SWT.NONE);
      authPasswd.setLabel(i18n.tr("Authentication password"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		authPasswd.setLayoutData(gd);
		
		privPasswd = new PasswordInputField(dialogArea, SWT.NONE);
      privPasswd.setLabel(i18n.tr("Encryption password"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		privPasswd.setLayoutData(gd);
      
      comment = new LabeledText(dialogArea, SWT.NONE);
      comment.setLabel("Comments");
      comment.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      comment.setLayoutData(gd);

      if (credentials != null)
		{
		   name.setText(credentials.getName());
		   authMethod.select(credentials.getAuthMethod());
		   privMethod.select(credentials.getPrivMethod());
		   authPasswd.setText(credentials.getAuthPassword());
		   privPasswd.setText(credentials.getPrivPassword());
		   comment.setText(credentials.getComment());
		}
		
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
      if (credentials == null)
	      credentials = new SnmpUsmCredential();
	   credentials.setName(name.getText().trim());
	   credentials.setAuthMethod(authMethod.getSelectionIndex());
	   credentials.setPrivMethod(privMethod.getSelectionIndex());
	   credentials.setAuthPassword(authPasswd.getText());
		credentials.setPrivPassword(privPasswd.getText());
		credentials.setComment(comment.getText());
		super.okPressed();
	}

	/**
	 * @return the value
	 */
	public SnmpUsmCredential getCredentials()
	{
		return credentials;
	}
}
