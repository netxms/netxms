/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Object creation dialog
 *
 */
public class CreateObjectDialog extends Dialog
{
	private String objectClassName;
	private String objectName;
	private Text textName;

	/**
	 * Constructor
	 * 
	 * @param parentShell Parent shell
	 * @param objectClassName Object class - this string will be added to dialog's title
	 */
	public CreateObjectDialog(Shell parentShell, String objectClassName)
	{
		super(parentShell);
		this.objectClassName = objectClassName;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.CreateObjectDialog_TitlePrefix + objectClassName);
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
      dialogArea.setLayout(layout);
		
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT,
      		Messages.CreateObjectDialog_ObjectName, "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$
      textName.getShell().setMinimumSize(300, 0);
      textName.setTextLimit(63);
      textName.setFocus();
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		objectName = textName.getText().trim();
		if (objectName.isEmpty())
		{
			MessageDialog.openWarning(getShell(), Messages.CreateObjectDialog_Warning, Messages.CreateObjectDialog_WarningText);
			return;
		}
		super.okPressed();
	}

	/**
	 * @return the objectName
	 */
	public String getObjectName()
	{
		return objectName;
	}
}
