/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.serverconfig.dialogs;

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
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for sending Notification message
 */
public class SendNotificationDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SendNotificationDialog.class);

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
	public SendNotificationDialog(Shell parentShell, String channelName)
	{
		super(parentShell);
      PreferenceStore settings = PreferenceStore.getInstance();
      recipient = settings.getAsString("SendNotification.Recipient", "");
      subject = settings.getAsString("SendNotification.Subject", "");
		this.channelName = channelName;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText((channelName != null) ? i18n.tr("Send Notification via {0}", channelName) : i18n.tr("Send Notification"));
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

      if (channelName == null)
      {
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         channelNameCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Channel name"), gd);
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
      }

		recipientField = new LabeledText(dialogArea, SWT.NONE);
      recipientField.setLabel(i18n.tr("Recipient"));
		recipientField.setText(recipient);
      GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		recipientField.setLayoutData(gd);

		subjectField = new LabeledText(dialogArea, SWT.NONE);
      subjectField.setLabel(i18n.tr("Subject"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      subjectField.setLayoutData(gd);

		messageField = new LabeledText(dialogArea, SWT.NONE);
      messageField.setLabel(i18n.tr("Message"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		messageField.setLayoutData(gd);

		if ((recipient != null) && !recipient.isEmpty())
			messageField.getTextControl().setFocus();		

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Reading list of notification channels"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            channels = session.getNotificationChannels();
            channels.sort((c1, c2) -> c1.getName().compareTo(c2.getName()));
            runInUIThread(() -> updateChannelSelector());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of notification channels");
         }
      }.start();

		return dialogArea;
	}

	/**
	 * Update notification channel selector
	 */
   private void updateChannelSelector()
	{
      if (channelName == null)
      {
         channelNameCombo.removeAll();
         int selection = -1;
         for(int i = 0; i < channels.size(); i++)
         {
            NotificationChannel nc = channels.get(i);
            channelNameCombo.add(nc.getName());
            if (channelName != null && channelName.equalsIgnoreCase(nc.getName()))
            {
               selection = i;
            }
         }
         if (selection > 0)
         {
            channelNameCombo.select(selection);
            recipientField.setEnabled(channels.get(channelNameCombo.getSelectionIndex()).getConfigurationTemplate().needRecipient);
            subjectField.setEnabled(channels.get(channelNameCombo.getSelectionIndex()).getConfigurationTemplate().needSubject);
         }
      }
      else
      {
         for(int i = 0; i < channels.size(); i++)
         {
            NotificationChannel c = channels.get(i);
            if (c.getName().equals(channelName))
            {
               recipientField.setEnabled(c.getConfigurationTemplate().needRecipient);
               subjectField.setEnabled(c.getConfigurationTemplate().needSubject);
               break;
            }
         }
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
      if (channelName == null)
         channelName = channelNameCombo.getItem(channelNameCombo.getSelectionIndex());
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set("SendNotification.Recipient", recipient);
      settings.set("SendNotification.Subject", subject);
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
