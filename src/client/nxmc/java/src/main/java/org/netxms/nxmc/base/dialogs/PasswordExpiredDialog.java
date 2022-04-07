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
package org.netxms.nxmc.base.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Password expired" dialog
 */
public class PasswordExpiredDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(PasswordExpiredDialog.class);
   private int graceLogins;
   private LabeledText textPassword1;
   private LabeledText textPassword2;
	private String password;

	/**
	 * @param parentShell
	 * @param graceLogins
	 */
	public PasswordExpiredDialog(Shell parentShell, int graceLogins)
	{
		super(parentShell);
		this.graceLogins = graceLogins;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		final GridLayout layout = new GridLayout();

		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

		Label pic = new Label(dialogArea, SWT.NONE);
      final Image image = ResourceManager.getImage("icons/password.png");
      pic.setImage(image);
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		pic.setLayoutData(gd);
      pic.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            image.dispose();
         }
      });

		Composite editArea = new Composite(dialogArea, SWT.NONE);
		GridLayout editAreaLayout = new GridLayout();
		editAreaLayout.marginWidth = 0;
		editAreaLayout.marginHeight = 0;
		editArea.setLayout(editAreaLayout);

		Label msg = new Label(editArea, SWT.WRAP);
      msg.setText(String.format(i18n.tr("Your password was expired (%d grace logins left). Please change your password now."), graceLogins)); //$NON-NLS-1$
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		msg.setLayoutData(gd);
      textPassword1 = new LabeledText(editArea, SWT.NONE, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD);
      textPassword1.setLabel(i18n.tr("New password:"));
      textPassword1.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      textPassword2 = new LabeledText(editArea, SWT.NONE, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD);
      textPassword2.setLabel(i18n.tr("Confirm new password:"));
      textPassword2.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 250;
		editArea.setLayoutData(gd);
		
		final ModifyListener listener = new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				Control button = getButton(IDialogConstants.OK_ID);
				button.setEnabled(validate());
			}
		};
      textPassword1.getTextControl().addModifyListener(listener);
      textPassword2.getTextControl().addModifyListener(listener);

		return dialogArea;
	}

   /**
    * Validate passwords
    * 
    * @return true if can proceed with password change
    */
	protected boolean validate()
	{
		final String password1 = textPassword1.getText();
		final String password2 = textPassword2.getText();

		final boolean ret;
		if (password1.equals(password2))
		{
			ret = true;
		}
		else
		{
			ret = false;
		}

		return ret;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Change Password"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		password = textPassword1.getText();
		super.okPressed();
	}

   /**
    * Get password entered by user.
    * 
    * @return password entered by user
    */
	public String getPassword()
	{
		return password;
	}

}
