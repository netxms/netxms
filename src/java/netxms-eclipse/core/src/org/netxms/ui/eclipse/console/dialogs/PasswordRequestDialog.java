/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Raden Solutions
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
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

public class PasswordRequestDialog extends Dialog
{
	private Text textPassword;
	private Label lblMessage;
	private String password = "";
	private String message = "";
	
	/**
	 * @param parentShell
	 */
	public PasswordRequestDialog(Shell parentShell)
	{
		super(parentShell);
	}

	@Override
	protected Control createDialogArea(Composite parent)
	{
		// TODO: Make this easier on the eyes
		final Composite container =  (Composite) super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
      layout.verticalSpacing = GridData.CENTER;
      layout.marginTop = 10;
      layout.marginBottom = 0;
      layout.marginWidth = 10;
      layout.marginHeight = 10;
		
      container.setLayout(layout);
      
      lblMessage = new Label(container, SWT.NONE | SWT.WRAP);
      lblMessage.setText(message);
		textPassword = new Text(container, SWT.SINGLE | SWT.BORDER | SWT.PASSWORD);
		textPassword.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));
		
		return container;
	}

	@Override
	protected void okPressed()
	{
		password = textPassword.getText();

		super.okPressed();
	}

	public String getPassword() {
		return password; 
	}
	
	public void setMessage(String msg)
	{
		message = msg;
	}
}
