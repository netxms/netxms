/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
	private String recipient;
   private String subject;
   private String message;
   private String channelName;   
   private Combo channelNameCombo;
	private LabeledText recipientField;
   private LabeledText subjectField;
	private LabeledText messageField;
   private List<NotificationChannel> channels = null;
	
	/**
	 * @param parentShell
	 */
	public SendNotificationDialog(Shell parentShell)
	{
		super(parentShell);
		recipient = Activator.getDefault().getDialogSettings().get("SendNotification.PhoneNumber"); //$NON-NLS-1$
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText("Send Notification");
	}

   /**
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
            recipientField.setEnabled(channels.get(channelNameCombo.getSelectionIndex()).getConfigurationTemplate().needRecipient);
            subjectField.setEnabled(channels.get(channelNameCombo.getSelectionIndex()).getConfigurationTemplate().needSubject);
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);            
         }
      });
		
		recipientField = new LabeledText(dialogArea, SWT.NONE);
		recipientField.setLabel("Recipient");
		recipientField.setText(recipient);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		recipientField.setLayoutData(gd);
		
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
		
		if ((recipient != null) && !recipient.isEmpty())
			messageField.getTextControl().setFocus();		

      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob("Get list of notification channels", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            channels = session.getNotificationChannels();
            channels.sort(new Comparator<NotificationChannel>() {

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
                  updateChannelSelector();
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
   private void updateChannelSelector()
	{
      channelNameCombo.removeAll();
      for(int i = 0; i < channels.size(); i++)
      {
         NotificationChannel nc = channels.get(i);
         channelNameCombo.add(nc.getName());
      }	   
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		recipient = recipientField.getText().trim();
		subject = subjectField.getText();
      message = messageField.getText();
      channelName = channelNameCombo.getItem(channelNameCombo.getSelectionIndex());
		Activator.getDefault().getDialogSettings().put("SendNotification.PhoneNumber", recipient); //$NON-NLS-1$
		super.okPressed();
	}

	/**
    * Get recipient address.
    *
    * @return recipient address
    */
	public String getRecipient()
	{
		return recipient;
	}

	/**
    * Get message text.
    *
    * @return message text
    */
	public String getMessage()
	{
		return message;
	}

   /**
    * Get subject.
    *
    * @return subject
    */
   public String getSubject()
   {
      return subject;
   }

   /**
    * Get channel name.
    *
    * @return channel name
    */
   public String getChannelName()
   {
      return channelName;
   }
}
