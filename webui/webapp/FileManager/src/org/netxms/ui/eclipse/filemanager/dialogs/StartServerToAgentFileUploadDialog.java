/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.server.ServerFile;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.filemanager.widgets.ServerFileSelector;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for starting file upload
 */
public class StartServerToAgentFileUploadDialog extends Dialog
{
	private ServerFileSelector fileSelector;
	private LabeledText textRemoteFile;
	private Button checkJobOnHold;
	private ServerFile serverFile;
	private String remoteFileName;
	private boolean createJobOnHold;
	
	/**
	 * 
	 * @param parentShell
	 */
	public StartServerToAgentFileUploadDialog(Shell parentShell)
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
		newShell.setText(Messages.get().StartServerToAgentFileUploadDialog_Title);
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
		
		fileSelector = new ServerFileSelector(dialogArea, SWT.NONE);
		fileSelector.setLabel(Messages.get().StartServerToAgentFileUploadDialog_ServerFile);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		fileSelector.setLayoutData(gd);
		
		textRemoteFile = new LabeledText(dialogArea, SWT.NONE);
		textRemoteFile.setLabel(Messages.get().StartServerToAgentFileUploadDialog_RemoteFileName);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textRemoteFile.setLayoutData(gd);
		
		checkJobOnHold = new Button(dialogArea, SWT.CHECK);
		checkJobOnHold.setText(Messages.get().StartServerToAgentFileUploadDialog_CreateJobOnHold);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		serverFile = fileSelector.getFile();
		if (serverFile == null)
		{
			MessageDialogHelper.openWarning(getShell(), Messages.get().StartServerToAgentFileUploadDialog_Warning, Messages.get().StartServerToAgentFileUploadDialog_WarningText);
			return;
		}
		remoteFileName = textRemoteFile.getText().trim();
		createJobOnHold = checkJobOnHold.getSelection();
		super.okPressed();
	}

	/**
	 * @return the serverFile
	 */
	public ServerFile getServerFile()
	{
		return serverFile;
	}

	/**
	 * @return the remoteFileName
	 */
	public String getRemoteFileName()
	{
		return remoteFileName;
	}

	/**
	 * @return the createJobOnHold
	 */
	public boolean isCreateJobOnHold()
	{
		return createJobOnHold;
	}
}
