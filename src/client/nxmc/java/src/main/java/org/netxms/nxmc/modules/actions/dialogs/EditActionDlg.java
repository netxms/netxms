/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.actions.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.NotificationChannel;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.ServerActionType;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Action edit dialog
 */
public class EditActionDlg extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditActionDlg.class);

	private ServerAction action;
	private boolean createNew;
	private Text name;
	private LabeledText recipient;
	private LabeledText subject;
	private LabeledText data;
   private LabeledCombo channelName;
	private Button typeLocalExec;
	private Button typeRemoteExec;
	private Button typeRemoteSshExec;
	private Button typeExecScript;
	private Button typeNotification;
	private Button typeForward;
	private Button markDisabled;
	private List<NotificationChannel> notificationChannels = null;

	/**
	 * Selection listener for action type radio buttons
	 */
	class TypeButtonSelectionListener implements SelectionListener
	{
		@Override
		public void widgetDefaultSelected(SelectionEvent e)
		{
			widgetSelected(e);
		}

		@Override
		public void widgetSelected(SelectionEvent e)
		{
			onTypeChange();
		}
	};

	/**
	 * @param parentShell
	 */
	public EditActionDlg(Shell parentShell, ServerAction action, boolean createNew)
	{
		super(parentShell);
		this.action = action;
		this.createNew = createNew;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(createNew ? i18n.tr("Create Action") : i18n.tr("Edit Action"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
		layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);

		name = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, i18n.tr("Name"), action.getName(), WidgetHelper.DEFAULT_LAYOUT_DATA);

		/* type selection radio buttons */
		Group typeGroup = new Group(dialogArea, SWT.NONE);
      typeGroup.setText(i18n.tr("Type"));
		typeGroup.setLayout(new RowLayout(SWT.VERTICAL));
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		typeGroup.setLayoutData(gd);
		
		typeLocalExec = new Button(typeGroup, SWT.RADIO);
      typeLocalExec.setText(i18n.tr("Execute command on management server"));
      typeLocalExec.setSelection(action.getType() == ServerActionType.LOCAL_COMMAND);
		typeLocalExec.addSelectionListener(new TypeButtonSelectionListener());
		
		typeRemoteExec = new Button(typeGroup, SWT.RADIO);
      typeRemoteExec.setText(i18n.tr("Execute command on remote node via agent"));
      typeRemoteExec.setSelection(action.getType() == ServerActionType.AGENT_COMMAND);
		typeRemoteExec.addSelectionListener(new TypeButtonSelectionListener());
		
		typeRemoteSshExec = new Button(typeGroup, SWT.RADIO);
      typeRemoteSshExec.setText(i18n.tr("Execute command on remote node via SSH"));
      typeRemoteSshExec.setSelection(action.getType() == ServerActionType.SSH_COMMAND);
		typeRemoteSshExec.addSelectionListener(new TypeButtonSelectionListener());

		typeExecScript = new Button(typeGroup, SWT.RADIO);
      typeExecScript.setText(i18n.tr("Execute &NXSL script"));
      typeExecScript.setSelection(action.getType() == ServerActionType.NXSL_SCRIPT);
		typeExecScript.addSelectionListener(new TypeButtonSelectionListener());
		
		typeNotification = new Button(typeGroup, SWT.RADIO);
      typeNotification.setText(i18n.tr("Send notification"));
      typeNotification.setSelection(action.getType() == ServerActionType.NOTIFICATION);
		typeNotification.addSelectionListener(new TypeButtonSelectionListener());
      
		typeForward = new Button(typeGroup, SWT.RADIO);
      typeForward.setText(i18n.tr("Forward event to other NetXMS server"));
      typeForward.setSelection(action.getType() == ServerActionType.FORWARD_EVENT);
		typeForward.addSelectionListener(new TypeButtonSelectionListener());
		/* type selection radio buttons - end */

		Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(i18n.tr("Options"));
		optionsGroup.setLayout(new RowLayout(SWT.VERTICAL));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		
		markDisabled = new Button(optionsGroup, SWT.CHECK);
      markDisabled.setText(i18n.tr("Action is &disabled"));
		markDisabled.setSelection(action.isDisabled());

      channelName = new LabeledCombo(dialogArea, SWT.NONE);
      channelName.setLabel(i18n.tr("Channel name"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      channelName.setLayoutData(gd);
      channelName.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            recipient.setEnabled(notificationChannels.get(channelName.getSelectionIndex()).getConfigurationTemplate().needRecipient);
            subject.setEnabled(notificationChannels.get(channelName.getSelectionIndex()).getConfigurationTemplate().needSubject);
         }
      });

		recipient = new LabeledText(dialogArea, SWT.NONE);
		recipient.setLabel(getRcptLabel(action.getType()));
		recipient.setText(action.getRecipientAddress());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		recipient.setLayoutData(gd);

		subject = new LabeledText(dialogArea, SWT.NONE);
      subject.setLabel(i18n.tr("Subject"));
		subject.setText(action.getEmailSubject());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		subject.setLayoutData(gd);
		
      data = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER);
      data.setLabel(getDataLabel(action.getType()));
      data.getTextControl().setFont(JFaceResources.getTextFont());
      data.setText(action.getData());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 200;
      gd.widthHint = 400;
      data.setLayoutData(gd);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Get list of notification channels"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            notificationChannels = session.getNotificationChannels();
            notificationChannels.sort((NotificationChannel c1, NotificationChannel c2) -> c1.getName().compareTo(c2.getName()));
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  onTypeChange();
               }
            });
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
	 * Get "recipient" field label depending on action type
	 * 
	 * @param type Action type
	 * @return Label for "recipient" field
	 */
   private String getRcptLabel(ServerActionType type)
	{
		switch(type)
		{
         case AGENT_COMMAND:
         case SSH_COMMAND:
            return i18n.tr("Remote host");
         case FORWARD_EVENT:
            return i18n.tr("Remote NetXMS server");
         case NXSL_SCRIPT:
            return i18n.tr("Script name");
         default:
            return i18n.tr("Recipient's address");
		}
	}

	/**
	 * Get "data" field label depending on action type
	 * 
	 * @param type Action type
	 * @return Label for "data" field
	 */
   private String getDataLabel(ServerActionType type)
	{
		switch(type)
		{
         case LOCAL_COMMAND:
            return i18n.tr("Command");
         case AGENT_COMMAND:
         case SSH_COMMAND:
            return i18n.tr("Agent's action");
         default:
            return i18n.tr("Message text");
		}
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		if (typeLocalExec.getSelection())
         action.setType(ServerActionType.LOCAL_COMMAND);
		else if (typeRemoteExec.getSelection())
         action.setType(ServerActionType.AGENT_COMMAND);
		else if (typeRemoteSshExec.getSelection())
         action.setType(ServerActionType.SSH_COMMAND);
		else if (typeExecScript.getSelection())
         action.setType(ServerActionType.NXSL_SCRIPT);
		else if (typeNotification.getSelection())
         action.setType(ServerActionType.NOTIFICATION);
		else if (typeForward.getSelection())
         action.setType(ServerActionType.FORWARD_EVENT);

		action.setName(name.getText());
		action.setRecipientAddress(recipient.getText());
		action.setEmailSubject(subject.getText());
		action.setData(data.getText());
		action.setDisabled(markDisabled.getSelection());

      if (typeNotification.getSelection())
         action.setChannelName(notificationChannels.get(channelName.getSelectionIndex()).getName());
		else
		   action.setChannelName("");

		super.okPressed();
	}

	/**
	 * Update list of available channels
	 */
	private void updateChannelList()
	{
	   channelName.removeAll();

      boolean needRecipient = false;
      boolean needSubject = false;

	   for(int i = 0; i < notificationChannels.size(); i++)
	   {
	      NotificationChannel nc = notificationChannels.get(i);
	      channelName.add(nc.getName());
	      if (nc.getName().equals(action.getChannelName()))
	      {
	         channelName.select(i);
	         needRecipient = nc.getConfigurationTemplate().needRecipient;
	         needSubject = nc.getConfigurationTemplate().needSubject;
	      }
	   }

      recipient.setEnabled(needRecipient);
      subject.setEnabled(needSubject);
	}

	/**
	 * Handle action type change
	 */
	private void onTypeChange()
	{
      ServerActionType type = null;

		if (typeLocalExec.getSelection())
         type = ServerActionType.LOCAL_COMMAND;
		else if (typeRemoteExec.getSelection())
         type = ServerActionType.AGENT_COMMAND;
		else if (typeRemoteSshExec.getSelection())
         type = ServerActionType.SSH_COMMAND;
		else if (typeExecScript.getSelection())
         type = ServerActionType.NXSL_SCRIPT;
		else if (typeNotification.getSelection())
         type = ServerActionType.NOTIFICATION;
		else if (typeForward.getSelection())
         type = ServerActionType.FORWARD_EVENT;

		if (type == null)
         return;

		switch(type)
		{
			case LOCAL_COMMAND:
			   channelName.setEnabled(false);
				recipient.setEnabled(false);
				subject.setEnabled(false);
				data.setEnabled(true);
				break;
         case AGENT_COMMAND:
         case SSH_COMMAND:
            channelName.setEnabled(false);
				recipient.setEnabled(true);
				subject.setEnabled(false);
				data.setEnabled(true);
				break;
         case FORWARD_EVENT:
         case NXSL_SCRIPT:
            channelName.setEnabled(false);
				recipient.setEnabled(true);
				subject.setEnabled(false);
				data.setEnabled(false);
				break;
         case NOTIFICATION:
            channelName.setEnabled(true);
            updateChannelList();            
            data.setEnabled(true);
            break;
		}

		recipient.setLabel(getRcptLabel(type));
		data.setLabel(getDataLabel(type));
	}
}
