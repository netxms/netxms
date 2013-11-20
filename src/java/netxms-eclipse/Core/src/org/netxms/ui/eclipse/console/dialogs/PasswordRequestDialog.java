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
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 *
 */
public class PasswordRequestDialog extends Dialog
{
   private LabeledText textPassword;
	private Label lblMessage;
	private String title;
	private String password = ""; //$NON-NLS-1$
	private String message = ""; //$NON-NLS-1$
	
	/**
	 * @param parentShell
	 */
	public PasswordRequestDialog(Shell parentShell)
	{
		super(parentShell);
	}	
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell shell) 
	{
	   super.configureShell(shell);
	   
	   shell.setText(title);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		// TODO: Make this easier on the eyes
		final Composite container = (Composite) super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		
      container.setLayout(layout);
      
      lblMessage = new Label(container, SWT.NONE | SWT.WRAP);
      lblMessage.setText(message);
      lblMessage.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
      
		textPassword = new LabeledText(container, SWT.NONE, SWT.BORDER | SWT.PASSWORD);
		textPassword.setLabel(Messages.get().LoginDialog_Passwd);
		textPassword.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.GRAB_HORIZONTAL));
		
		return container;
	}	
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#getInitialSize()
	 */
	@Override
   protected Point getInitialSize()
   {
      return new Point(300, 210);
   }

   /* (non-Javadoc)
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
	public void setMessage(String msg)
	{
		message = msg;
	}
	
	/**
	 * @param title
	 */
	public void setTitle(String title)
	{
	   this.title = title;
	}
}
