/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
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
import org.netxms.client.ServerAction;
import org.netxms.ui.eclipse.actionmanager.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Action edit dialog
 *
 */
public class EditActionDlg extends Dialog
{
	private ServerAction action;
	private boolean createNew;
	private Text name;
	private LabeledText recipient;
	private LabeledText subject;
	private LabeledText data;
	private Button typeLocalExec;
	private Button typeRemoteExec;
	private Button typeExecScript;
	private Button typeEMail;
	private Button typeSMS;
	private Button typeForward;
	private Button markDisabled;

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
		newShell.setText(createNew ? Messages.EditActionDlg_CreateAction : Messages.EditActionDlg_EditAction);
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
		
		name = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, Messages.EditActionDlg_Name, action.getName(), WidgetHelper.DEFAULT_LAYOUT_DATA);

		/* type selection radio buttons */
		Group typeGroup = new Group(dialogArea, SWT.NONE);
		typeGroup.setText(Messages.EditActionDlg_Type);
		typeGroup.setLayout(new RowLayout(SWT.VERTICAL));
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		typeGroup.setLayoutData(gd);
		
		typeLocalExec = new Button(typeGroup, SWT.RADIO);
		typeLocalExec.setText(Messages.EditActionDlg_ExecCommandOnServer);
		typeLocalExec.setSelection(action.getType() == ServerAction.EXEC_LOCAL);
		typeLocalExec.addSelectionListener(new TypeButtonSelectionListener());
		
		typeRemoteExec = new Button(typeGroup, SWT.RADIO);
		typeRemoteExec.setText(Messages.EditActionDlg_ExecCommandOnNode);
		typeRemoteExec.setSelection(action.getType() == ServerAction.EXEC_REMOTE);
		typeRemoteExec.addSelectionListener(new TypeButtonSelectionListener());
		
		typeExecScript = new Button(typeGroup, SWT.RADIO);
		typeExecScript.setText("Execute &NXSL script");
		typeExecScript.setSelection(action.getType() == ServerAction.EXEC_NXSL_SCRIPT);
		typeExecScript.addSelectionListener(new TypeButtonSelectionListener());
		
		typeEMail = new Button(typeGroup, SWT.RADIO);
		typeEMail.setText(Messages.EditActionDlg_SenMail);
		typeEMail.setSelection(action.getType() == ServerAction.SEND_EMAIL);
		typeEMail.addSelectionListener(new TypeButtonSelectionListener());
		
		typeSMS = new Button(typeGroup, SWT.RADIO);
		typeSMS.setText(Messages.EditActionDlg_SendSMS);
		typeSMS.setSelection(action.getType() == ServerAction.SEND_SMS);
		typeSMS.addSelectionListener(new TypeButtonSelectionListener());
		
		typeForward = new Button(typeGroup, SWT.RADIO);
		typeForward.setText(Messages.EditActionDlg_ForwardEvent);
		typeForward.setSelection(action.getType() == ServerAction.FORWARD_EVENT);
		typeForward.addSelectionListener(new TypeButtonSelectionListener());
		/* type selection radio buttons - end */

		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(Messages.EditActionDlg_Options);
		optionsGroup.setLayout(new RowLayout(SWT.VERTICAL));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		
		markDisabled = new Button(optionsGroup, SWT.CHECK);
		markDisabled.setText(Messages.EditActionDlg_ActionDisabled);
		markDisabled.setSelection(action.isDisabled());
		
		recipient = new LabeledText(dialogArea, SWT.NONE);
		recipient.setLabel(getRcptLabel(action.getType()));
		recipient.setText(action.getRecipientAddress());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		recipient.setLayoutData(gd);

		subject = new LabeledText(dialogArea, SWT.NONE);
		subject.setLabel(Messages.EditActionDlg_MailSubject);
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
		
		onTypeChange();
		
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
				return Messages.EditActionDlg_RemoteHost;
			case ServerAction.SEND_SMS:
				return Messages.EditActionDlg_PhoneNumber;
			case ServerAction.FORWARD_EVENT:
				return Messages.EditActionDlg_RemoteServer;
			case ServerAction.EXEC_NXSL_SCRIPT:
				return "Script name";
		}
		return Messages.EditActionDlg_Recipient;
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
				return Messages.EditActionDlg_Command;
			case ServerAction.EXEC_REMOTE:
				return Messages.EditActionDlg_Action;
		}
		return Messages.EditActionDlg_MessageText;
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
		else if (typeSMS.getSelection())
			action.setType(ServerAction.SEND_SMS);
		else if (typeForward.getSelection())
			action.setType(ServerAction.FORWARD_EVENT);
		
		action.setName(name.getText());
		action.setRecipientAddress(recipient.getText());
		action.setEmailSubject(subject.getText());
		action.setData(data.getText());
		action.setDisabled(markDisabled.getSelection());
		
		super.okPressed();
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
		else if (typeSMS.getSelection())
			type = ServerAction.SEND_SMS;
		else if (typeForward.getSelection())
			type = ServerAction.FORWARD_EVENT;
		
		switch(type)
		{
			case ServerAction.EXEC_LOCAL:
				recipient.setEnabled(false);
				subject.setEnabled(false);
				data.setEnabled(true);
				break;
			case ServerAction.EXEC_REMOTE:
				recipient.setEnabled(true);
				subject.setEnabled(false);
				data.setEnabled(true);
				break;
			case ServerAction.SEND_EMAIL:
				recipient.setEnabled(true);
				subject.setEnabled(true);
				data.setEnabled(true);
				break;
			case ServerAction.SEND_SMS:
				recipient.setEnabled(true);
				subject.setEnabled(false);
				data.setEnabled(true);
				break;
			case ServerAction.FORWARD_EVENT:
				recipient.setEnabled(true);
				subject.setEnabled(false);
				data.setEnabled(false);
				break;
			case ServerAction.EXEC_NXSL_SCRIPT:
				recipient.setEnabled(true);
				subject.setEnabled(false);
				data.setEnabled(false);
				break;
		}
		
		recipient.setLabel(getRcptLabel(type));
		data.setLabel(getDataLabel(type));
	}
}
