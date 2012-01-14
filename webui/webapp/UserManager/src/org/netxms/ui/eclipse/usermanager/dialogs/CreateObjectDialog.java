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
package org.netxms.ui.eclipse.usermanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * User database object creation dialog
 * 
 */
public class CreateObjectDialog extends Dialog
{
	private Text textLogin;
	private Button checkEdit;
	private String loginName;
	private boolean editAfterCreate;
	private boolean isUser;

	public CreateObjectDialog(Shell parentShell, boolean isUser)
	{
		super(parentShell);
		this.isUser = isUser;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite) super.createDialogArea(parent);

		FillLayout layout = new FillLayout();
		layout.type = SWT.VERTICAL;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		dialogArea.setLayout(layout);

		Label label = new Label(dialogArea, SWT.NONE);
		label.setText("Login name");

		textLogin = new Text(dialogArea, SWT.SINGLE | SWT.BORDER);
		textLogin.setTextLimit(63);
		textLogin.getShell().setMinimumSize(300, 0);
		textLogin.setFocus();

		label = new Label(dialogArea, SWT.NONE);
		label.setText("");

		checkEdit = new Button(dialogArea, SWT.CHECK);
		checkEdit.setText("Define additional properties");
		checkEdit.setSelection(true);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(isUser ? "Create New User" : "Create New Group");
	}

	/**
	 * Get variable name
	 * 
	 */
	public String getLoginName()
	{
		return loginName;
	}

	/**
	 * Get variable value
	 * 
	 */
	public boolean isEditAfterCreate()
	{
		return editAfterCreate;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		loginName = textLogin.getText();
		editAfterCreate = checkEdit.getSelection();
		super.okPressed();
	}
}
