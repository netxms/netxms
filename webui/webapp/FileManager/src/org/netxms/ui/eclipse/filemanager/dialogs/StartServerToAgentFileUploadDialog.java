/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.ScheduledTask;
import org.netxms.client.server.ServerFile;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.ScheduleSelector;

/**
 * Dialog for starting file upload
 */
public class StartServerToAgentFileUploadDialog extends Dialog
{
	private TableViewer fileList;
	private Button buttonAddFile;
   private Button buttonRemoveFile;
	private LabeledText textRemoteFile;
	private Button checkJobOnHold;
   private Button checkIsSchedule;
   private ScheduleSelector scheduleSelector;
	private List<ServerFile> serverFiles = new ArrayList<ServerFile>();
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
		
		Label label = new Label(dialogArea, SWT.NONE);
      label.setText(Messages.get().StartServerToAgentFileUploadDialog_ServerFile);
      
		fileList = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		gd.heightHint = 150;
		fileList.getControl().setLayoutData(gd);
		
		fileList.setContentProvider(new ArrayContentProvider());
		fileList.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((ServerFile)element).getName();
         }
		});
		fileList.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((ServerFile)e1).getName().compareToIgnoreCase(((ServerFile)e2).getName());
         }
		});
		fileList.setInput(serverFiles);
		
		Composite buttonArea = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 2;
      buttonArea.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttonArea.setLayoutData(gd);
      
      buttonAddFile = new Button(buttonArea, SWT.PUSH);
      buttonAddFile.setText("&Add...");
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAddFile.setLayoutData(gd);
      buttonAddFile.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            SelectServerFileDialog dlg = new SelectServerFileDialog(getShell(), true);
            if (dlg.open() == Window.OK)
            {
               for(ServerFile f : dlg.getSelectedFiles())
               {
                  boolean found = false;
                  for(ServerFile sf : serverFiles)
                     if (sf.getName().equals(f.getName()))
                     {
                        found = true;
                        break;
                     }
                  if (!found)
                     serverFiles.add(f);
               }
               fileList.refresh();
            }
         }
      });
		
      buttonRemoveFile = new Button(buttonArea, SWT.PUSH);
      buttonRemoveFile.setText("&Remove");
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonRemoveFile.setLayoutData(gd);
      buttonRemoveFile.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            IStructuredSelection selection = (IStructuredSelection)fileList.getSelection();
            for(Object o : selection.toList())
               serverFiles.remove(o);
            fileList.refresh();
         }
      });
      
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
		
		if (canScheduleFileUpload)
		{
   		checkIsSchedule = new Button(dialogArea, SWT.CHECK);
   		checkIsSchedule.setText(Messages.get().StartServerToAgentFileUploadDialog_ScheduleTask);
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
	   if (scheduledTask)
	   {
	      schedule = scheduleSelector.getSchedule();
	   }
		if (serverFiles.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), Messages.get().StartServerToAgentFileUploadDialog_Warning, Messages.get().StartServerToAgentFileUploadDialog_WarningText);
			return;
		}
		remoteFileName = textRemoteFile.getText().trim();
		createJobOnHold = checkJobOnHold.getSelection();
		super.okPressed();
	}

	/**
	 * @return selected server files
	 */
	public List<ServerFile> getServerFiles()
	{
		return serverFiles;
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
