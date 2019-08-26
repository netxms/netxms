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
package org.netxms.ui.eclipse.actionmanager.dialogs;

import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.NotificationChannel;
import org.netxms.client.ServerAction;
import org.netxms.ui.eclipse.actionmanager.Activator;
import org.netxms.ui.eclipse.actionmanager.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Action edit dialog
 */
public class EditActionDlg extends Dialog
{
	private ServerAction action;
	private boolean createNew;
	private Text name;
	private LabeledText recipient;
	private LabeledText subject;
	private LabeledText data;
   private Combo channelName;
	private Button typeLocalExec;
	private Button typeRemoteExec;
	private Button typeExecScript;
	private Button typeEMail;
	private Button typeNotification;
	private Button typeXMPP;
	private Button typeForward;
	private Button markDisabled;
	private List<NotificationChannel> ncList = null;

	/**
	 * Selection listener for action type radio buttons
	 *
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(createNew ? Messages.get().EditActionDlg_CreateAction : Messages.get().EditActionDlg_EditAction);
	}

	/* (non-Javadoc)
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
		
		name = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, Messages.get().EditActionDlg_Name, action.getName(), WidgetHelper.DEFAULT_LAYOUT_DATA);

		/* type selection radio buttons */
		Group typeGroup = new Group(dialogArea, SWT.NONE);
		typeGroup.setText(Messages.get().EditActionDlg_Type);
		typeGroup.setLayout(new RowLayout(SWT.VERTICAL));
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		typeGroup.setLayoutData(gd);
		
		typeLocalExec = new Button(typeGroup, SWT.RADIO);
		typeLocalExec.setText(Messages.get().EditActionDlg_ExecCommandOnServer);
		typeLocalExec.setSelection(action.getType() == ServerAction.EXEC_LOCAL);
		typeLocalExec.addSelectionListener(new TypeButtonSelectionListener());
		
		typeRemoteExec = new Button(typeGroup, SWT.RADIO);
		typeRemoteExec.setText(Messages.get().EditActionDlg_ExecCommandOnNode);
		typeRemoteExec.setSelection(action.getType() == ServerAction.EXEC_REMOTE);
		typeRemoteExec.addSelectionListener(new TypeButtonSelectionListener());
		
		typeExecScript = new Button(typeGroup, SWT.RADIO);
		typeExecScript.setText(Messages.get().EditActionDlg_ExecuteScript);
		typeExecScript.setSelection(action.getType() == ServerAction.EXEC_NXSL_SCRIPT);
		typeExecScript.addSelectionListener(new TypeButtonSelectionListener());
		
		typeEMail = new Button(typeGroup, SWT.RADIO);
		typeEMail.setText(Messages.get().EditActionDlg_SenMail);
		typeEMail.setSelection(action.getType() == ServerAction.SEND_EMAIL);
		typeEMail.addSelectionListener(new TypeButtonSelectionListener());
		
		typeNotification = new Button(typeGroup, SWT.RADIO);
		typeNotification.setText("Send notification");
		typeNotification.setSelection(action.getType() == ServerAction.SEND_NOTIFICATION);
		typeNotification.addSelectionListener(new TypeButtonSelectionListener());
		
      typeXMPP = new Button(typeGroup, SWT.RADIO);
      typeXMPP.setText(Messages.get().EditActionDlg_SendXMPPMessage);
      typeXMPP.setSelection(action.getType() == ServerAction.XMPP_MESSAGE);
      typeXMPP.addSelectionListener(new TypeButtonSelectionListener());
      
		typeForward = new Button(typeGroup, SWT.RADIO);
		typeForward.setText(Messages.get().EditActionDlg_ForwardEvent);
		typeForward.setSelection(action.getType() == ServerAction.FORWARD_EVENT);
		typeForward.addSelectionListener(new TypeButtonSelectionListener());
		/* type selection radio buttons - end */

		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(Messages.get().EditActionDlg_Options);
		optionsGroup.setLayout(new RowLayout(SWT.VERTICAL));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		
		markDisabled = new Button(optionsGroup, SWT.CHECK);
		markDisabled.setText(Messages.get().EditActionDlg_ActionDisabled);
		markDisabled.setSelection(action.isDisabled());

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
		channelName = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Channel name", gd);
		channelName.addSelectionListener(new SelectionListener() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            recipient.setEnabled(ncList.get(channelName.getSelectionIndex()).getConfigurationTemplate().needRecipient);
            subject.setEnabled(ncList.get(channelName.getSelectionIndex()).getConfigurationTemplate().needSubject);
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);            
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
		subject.setLabel(Messages.get().EditActionDlg_MailSubject);
		subject.setText(action.getEmailSubject());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		subject.setLayoutData(gd);
		
		data = new LabeledText(dialogArea, SWT.NONE);
		data.setLabel(getDataLabel(action.getType()));
		data.setText(action.getData());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		data.setLayoutData(gd);

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
                  onTypeChange();
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
	 * Get "recipient" field label depending on action type
	 * 
	 * @param type Action type
	 * @return Label for "recipient" field
	 */
	private String getRcptLabel(int type)
	{
		switch(type)
		{
			case ServerAction.EXEC_REMOTE:
				return Messages.get().EditActionDlg_RemoteHost;
         case ServerAction.XMPP_MESSAGE:
            return Messages.get().EditActionDlg_XMPPID;
			case ServerAction.FORWARD_EVENT:
				return Messages.get().EditActionDlg_RemoteServer;
			case ServerAction.EXEC_NXSL_SCRIPT:
				return Messages.get().EditActionDlg_ScriptName;
		}
		return Messages.get().EditActionDlg_Recipient;
	}
	
	/**
	 * Get "data" field label depending on action type
	 * 
	 * @param type Action type
	 * @return Label for "data" field
	 */
	private String getDataLabel(int type)
	{
		switch(type)
		{
			case ServerAction.EXEC_LOCAL:
				return Messages.get().EditActionDlg_Command;
			case ServerAction.EXEC_REMOTE:
				return Messages.get().EditActionDlg_Action;
		}
		return Messages.get().EditActionDlg_MessageText;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		if (typeLocalExec.getSelection())
			action.setType(ServerAction.EXEC_LOCAL);
		else if (typeRemoteExec.getSelection())
			action.setType(ServerAction.EXEC_REMOTE);
		else if (typeExecScript.getSelection())
			action.setType(ServerAction.EXEC_NXSL_SCRIPT);
		else if (typeEMail.getSelection())
			action.setType(ServerAction.SEND_EMAIL);
		else if (typeNotification.getSelection())
			action.setType(ServerAction.SEND_NOTIFICATION);
      else if (typeXMPP.getSelection())
         action.setType(ServerAction.XMPP_MESSAGE);
		else if (typeForward.getSelection())
			action.setType(ServerAction.FORWARD_EVENT);
		
		action.setName(name.getText());
		action.setRecipientAddress(recipient.getText());
		action.setEmailSubject(subject.getText());
		action.setData(data.getText());
		action.setDisabled(markDisabled.getSelection());
		if(typeNotification.getSelection())
         action.setChannelName(ncList.get(channelName.getSelectionIndex()).getName());
		else
		   action.setChannelName("");
		
		super.okPressed();
	}
	
	private void updateChannelList()
	{
	   channelName.removeAll();
	   for(int i = 0; i < ncList.size(); i++)
	   {
	      NotificationChannel nc = ncList.get(i);
	      channelName.add(nc.getName());
	      if(nc.getName().equals(action.getChannelName()))
	      {
	         channelName.select(i);
            recipient.setEnabled(nc.getConfigurationTemplate().needRecipient);
            subject.setEnabled(nc.getConfigurationTemplate().needSubject);
	      }
	   }
	}
	
	/**
	 * Handle action type change
	 */
	private void onTypeChange()
	{
		int type = -1;
		
		if (typeLocalExec.getSelection())
			type = ServerAction.EXEC_LOCAL;
		else if (typeRemoteExec.getSelection())
			type = ServerAction.EXEC_REMOTE;
		else if (typeExecScript.getSelection())
			type = ServerAction.EXEC_NXSL_SCRIPT;
		else if (typeEMail.getSelection())
			type = ServerAction.SEND_EMAIL;
		else if (typeNotification.getSelection())
			type = ServerAction.SEND_NOTIFICATION;
      else if (typeXMPP.getSelection())
         type = ServerAction.XMPP_MESSAGE;
		else if (typeForward.getSelection())
			type = ServerAction.FORWARD_EVENT;
		
		switch(type)
		{
			case ServerAction.EXEC_LOCAL:
			   channelName.setEnabled(false);
				recipient.setEnabled(false);
				subject.setEnabled(false);
				data.setEnabled(true);
				break;
			case ServerAction.EXEC_REMOTE:
         case ServerAction.XMPP_MESSAGE:
            channelName.setEnabled(false);
				recipient.setEnabled(true);
				subject.setEnabled(false);
				data.setEnabled(true);
				break;
			case ServerAction.SEND_EMAIL:
            channelName.setEnabled(false);
				recipient.setEnabled(true);
				subject.setEnabled(true);
				data.setEnabled(true);
				break;
			case ServerAction.FORWARD_EVENT:
			case ServerAction.EXEC_NXSL_SCRIPT:
            channelName.setEnabled(false);
				recipient.setEnabled(true);
				subject.setEnabled(false);
				data.setEnabled(false);
				break;
         case ServerAction.SEND_NOTIFICATION:
            channelName.setEnabled(true);
            updateChannelList();            
            data.setEnabled(true);
            break;
		}
		
		recipient.setLabel(getRcptLabel(type));
		data.setLabel(getDataLabel(type));
	}
}
