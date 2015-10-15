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

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.filemanager.widgets.LocalFileSelector;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for starting file upload
 */
public class StartClientToServerFileUploadDialog extends Dialog
{
	private LocalFileSelector fileSelector;
	private LabeledText textRemoteFile;
	private List<File> localFiles = new ArrayList<File>();
	private String remoteFileName;
	
	/**
	 * 
	 * @param parentShell
	 */
	public StartClientToServerFileUploadDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().StartClientToServerFileUploadDialog_Title);
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
		
		fileSelector = new LocalFileSelector(dialogArea, SWT.NONE, false, SWT.OPEN | SWT.MULTI);
		fileSelector.setLabel(Messages.get().StartClientToServerFileUploadDialog_LocalFile);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		fileSelector.setLayoutData(gd);
		fileSelector.addModifyListener(new ModifyListener(){

         @Override
         public void modifyText(ModifyEvent e)
         {
            localFiles = fileSelector.getFileList();
            if(localFiles.size() > 1)
            {
               textRemoteFile.setEditable(false);
            }
            else
            {
               textRemoteFile.setEditable(true);
            }
         }
		   
		});
		
		textRemoteFile = new LabeledText(dialogArea, SWT.NONE);
		textRemoteFile.setLabel(Messages.get().StartClientToServerFileUploadDialog_RemoteFileName);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textRemoteFile.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		if (localFiles.size() <= 0)
		{
			MessageDialogHelper.openWarning(getShell(), Messages.get().StartClientToServerFileUploadDialog_Warning, Messages.get().StartClientToServerFileUploadDialog_WarningText);
			return;
		}
		
		remoteFileName = textRemoteFile.getText().trim();
		if(remoteFileName.isEmpty() && localFiles.size() == 1)
		   remoteFileName = localFiles.get(0).getName();
		super.okPressed();
	}

	/**
	 * @return the remoteFileName
	 */
	public String getRemoteFileName()
	{
		return remoteFileName;
	}

	/**
	 * @return the localFile
	 */
	public List<File> getLocalFiles()
	{
		return localFiles;
	}
}
