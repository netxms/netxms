/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.filemanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for starting file upload
 */
public class RenameFileDialog extends Dialog
{
   private LabeledText textOldFileName;
   private String oldFileName;
	private LabeledText textNewFileName;
	private String newFileName;
	
	/**
	 * 
	 * @param parentShell
	 */
	public RenameFileDialog(Shell parentShell, String oldName)
	{
		super(parentShell);
		oldFileName = oldName;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().RenameFileDialog_Title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);
		
		GridData gd;
		
		textOldFileName = new LabeledText(dialogArea, SWT.NONE);
		textOldFileName.setLabel(Messages.get().RenameFileDialog_OldName);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textOldFileName.setLayoutData(gd);
		textOldFileName.setText(oldFileName);
      textOldFileName.setEditable(false);
      
      textNewFileName = new LabeledText(dialogArea, SWT.NONE);
      textNewFileName.setLabel(Messages.get().RenameFileDialog_NewName);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textNewFileName.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
	   newFileName = textNewFileName.getText();
	   if (newFileName.contains("/") || newFileName.contains("\\")) //$NON-NLS-1$ //$NON-NLS-2$
	   {
	      MessageDialogHelper.openWarning(getShell(), Messages.get().RenameFileDialog_Warning, Messages.get().RenameFileDialog_WarningMessage);
	      return;
	   }
	      
		super.okPressed();
	}

	/**
	 * @return the remoteFileName
	 */
	public String getNewName()
	{
		return newFileName;
	}
}
