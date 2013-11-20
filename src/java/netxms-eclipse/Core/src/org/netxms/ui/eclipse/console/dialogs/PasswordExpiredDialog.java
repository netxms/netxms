/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Password expired" dialog
 */
public class PasswordExpiredDialog extends Dialog
{
	private Text textPassword1;
	private Text textPassword2;
	private String password;

	public PasswordExpiredDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
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
		pic.setImage(Activator.getImageDescriptor("icons/password.png").createImage()); //$NON-NLS-1$
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		pic.setLayoutData(gd);

		Composite editArea = new Composite(dialogArea, SWT.NONE);
		GridLayout editAreaLayout = new GridLayout();
		editAreaLayout.marginWidth = 0;
		editAreaLayout.marginHeight = 0;
		editArea.setLayout(editAreaLayout);
		
		Label msg = new Label(editArea, SWT.WRAP);
		msg.setText(Messages.get().PasswordExpiredDialog_passwd_expired); //$NON-NLS-1$
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		msg.setLayoutData(gd);
		textPassword1 = WidgetHelper.createLabeledText(editArea, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD, SWT.DEFAULT, Messages.get().PasswordExpiredDialog_new_passwd, "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$ //$NON-NLS-2$
		textPassword2 = WidgetHelper.createLabeledText(editArea, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD, SWT.DEFAULT, Messages.get().PasswordExpiredDialog_confirm_passwd, "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$ //$NON-NLS-2$

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
		textPassword1.addModifyListener(listener);
		textPassword2.addModifyListener(listener);

		return dialogArea;
	}

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

	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().PasswordExpiredDialog_title); //$NON-NLS-1$
	}

	@Override
	protected void okPressed()
	{
		password = textPassword1.getText();
		super.okPressed();
	}

	public String getPassword()
	{
		return password;
	}

}
