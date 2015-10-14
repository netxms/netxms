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
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.ScheduledTask;
import org.netxms.client.server.ServerFile;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.filemanager.widgets.ServerFileSelector;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.ScheduleSelector;

/**
 * Dialog for starting file upload
 */
public class StartServerToAgentFileUploadDialog extends Dialog
{
	private ServerFileSelector fileSelector;
	private LabeledText textRemoteFile;
	private Button checkJobOnHold;
   private Button checkIsSchedule;
   private ScheduleSelector scheduleSelector;
	private ServerFile serverFile;
	private String remoteFileName;
	private boolean createJobOnHold;
	private boolean scheduledTask;
	private ScheduledTask schedule;
	private boolean canScheduleFileUpload;
	
	/**
	 * 
	 * @param parentShell
	 * @param canScheduleFileUpload 
	 */
	public StartServerToAgentFileUploadDialog(Shell parentShell, boolean canScheduleFileUpload)
	{
		super(parentShell);
		this.canScheduleFileUpload = canScheduleFileUpload;
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
		checkJobOnHold.addSelectionListener(new SelectionListener() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {        
            checkIsSchedule.setEnabled(!checkJobOnHold.getSelection());
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e); 
         }
      });
		
		if(canScheduleFileUpload)
		{
   		checkIsSchedule = new Button(dialogArea, SWT.CHECK);
   		checkIsSchedule.setText("Schedule task");
         checkIsSchedule.addSelectionListener(new SelectionListener() {
            
            @Override
            public void widgetSelected(SelectionEvent e)
            {        
               checkJobOnHold.setEnabled(!checkIsSchedule.getSelection());
               scheduleSelector.setEnabled(checkIsSchedule.getSelection());
            }
            
            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e); 
            }
         });
         
         scheduleSelector = new ScheduleSelector(dialogArea, SWT.NONE);
         scheduleSelector.setEnabled(false);
		}
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
	   scheduledTask = checkIsSchedule.getSelection();
	   if(scheduledTask)
	   {
	      schedule = scheduleSelector.getSchedule();
	   }
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
	
	/**
    * @return the scheduledTask
    */
	public boolean isScheduled()
	{
	   return scheduledTask;
	}
	
	/**
    * @return the schedule
    */
	public ScheduledTask getScheduledTask()
	{
	   return schedule;
	}
	
}
