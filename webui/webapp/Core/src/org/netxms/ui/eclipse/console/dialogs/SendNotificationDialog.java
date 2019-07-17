/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.NotificationChannel;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for sending Notification message
 */
public class SendNotificationDialog extends Dialog
{
	private String phoneNumber;
   private String subject;
   private String message;
   private String channelName;   
   private Combo channelNameCombo;
	private LabeledText numberField;
   private LabeledText subjectField;
	private LabeledText messageField;
   private List<NotificationChannel> ncList = null;
	
	/**
	 * @param parentShell
	 */
	public SendNotificationDialog(Shell parentShell)
	{
		super(parentShell);
		phoneNumber = Activator.getDefault().getDialogSettings().get("SendNotification.PhoneNumber"); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Send notification");
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
		dialogArea.setLayout(layout);
		

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      channelNameCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Channel name", gd);
      channelNameCombo.addSelectionListener(new SelectionListener() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            numberField.setEnabled(ncList.get(channelNameCombo.getSelectionIndex()).getConfigurationTemplate().needRecipient);
            subjectField.setEnabled(ncList.get(channelNameCombo.getSelectionIndex()).getConfigurationTemplate().needSubject);
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);            
         }
      });
		
		numberField = new LabeledText(dialogArea, SWT.NONE);
		numberField.setLabel("Recipient");
		numberField.setText(phoneNumber);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		numberField.setLayoutData(gd);
		
		subjectField = new LabeledText(dialogArea, SWT.NONE);
		subjectField.setLabel("Subject");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      subjectField.setLayoutData(gd);
		
		messageField = new LabeledText(dialogArea, SWT.NONE);
		messageField.setLabel("Message");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		messageField.setLayoutData(gd);
		
		if ((phoneNumber != null) && !phoneNumber.isEmpty())
			messageField.getTextControl().setFocus();		

      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob("Get server actions", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            ncList = session.getNotificationChannels();
            ncList.sort(new Comparator<NotificationChannel>() {

               @Override
               public int compare(NotificationChannel o1, NotificationChannel o2)
               {
                  return o1.getName().compareTo(o2.getName());
               }
               
            });
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  updateNCSlector();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get notificationchannels";
         }
      }.start();
		
		return dialogArea;
	}
	
	/**
	 * Update notification channel selector
	 */
	private void updateNCSlector()
	{
      channelNameCombo.removeAll();
      for(int i = 0; i < ncList.size(); i++)
      {
         NotificationChannel nc = ncList.get(i);
         channelNameCombo.add(nc.getName());
      }	   
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		phoneNumber = numberField.getText().trim();
		subject = subjectField.getText();
      message = messageField.getText();
      channelName = channelNameCombo.getItem(channelNameCombo.getSelectionIndex());
		Activator.getDefault().getDialogSettings().put("SendNotification.PhoneNumber", phoneNumber); //$NON-NLS-1$
		super.okPressed();
	}

	/**
	 * @return the phoneNumber
	 */
	public String getPhoneNumber()
	{
		return phoneNumber;
	}

	/**
	 * @return the message
	 */
	public String getMessage()
	{
		return message;
	}

   /**
    * @return the subject
    */
   public String getSubject()
   {
      return subject;
   }

   /**
    * @return the channelName
    */
   public String getChannelName()
   {
      return channelName;
   }
}
