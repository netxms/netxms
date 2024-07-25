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
package org.netxms.nxmc.base.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Password request dialog
 */
public class PasswordRequestDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(PasswordRequestDialog.class);
   private LabeledText textPassword;
	private Label lblMessage;
	private String title;
   private String message;
   private String password;

	/**
	 * @param parentShell
	 */
   public PasswordRequestDialog(Shell parentShell, String title, String message)
	{
		super(parentShell);
      this.title = title;
      this.message = message;
	}	

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell shell) 
	{
	   super.configureShell(shell);
	   shell.setText(title);
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		final Composite container = (Composite) super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      container.setLayout(layout);

      lblMessage = new Label(container, SWT.NONE | SWT.WRAP);
      lblMessage.setText(message);
      lblMessage.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

		textPassword = new LabeledText(container, SWT.NONE, SWT.BORDER | SWT.PASSWORD);
      textPassword.setLabel(i18n.tr("Password"));
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 400;
		textPassword.setLayoutData(gd);

		return container;
	}	

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
	protected void okPressed()
	{
		password = textPassword.getText();
		super.okPressed();
	}

	/**
	 * @return
	 */
	public String getPassword() 
	{
		return password; 
	}

	/**
	 * @param msg
	 */
   public void setMessage(String message)
	{
      this.message = message;
	}

	/**
	 * @param title
	 */
	public void setTitle(String title)
	{
	   this.title = title;
	}
}
