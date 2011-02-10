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
package org.netxms.ui.eclipse.epp.dialogs;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
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
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.epp.dialogs.helpers.ActionComparator;
import org.netxms.ui.eclipse.epp.views.EventProcessingPolicyEditor;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.ImageCombo;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dialog for editing rule's actions
 *
 */
public class EditRuleActionsDlg extends Dialog
{
	private static final int ALARM_NO_ACTION = 0;
	private static final int ALARM_CREATE = 1;
	private static final int ALARM_TERMINATE = 2;
	
	private EventProcessingPolicyEditor editor;
	private EventProcessingPolicyRule rule;
	private int alarmAction;
	private Map<Long, ServerAction> actions = new HashMap<Long, ServerAction>();
	
	private Button alarmNoAction;
	private Button alarmCreate;
	private Button alarmTerminate;
	private Composite alarmCreationGroup;
	private Composite alarmTerminationGroup;
	private LabeledText alarmMessage;
	private LabeledText alarmKeyCreate;
	private ImageCombo alarmSeverity;
	private LabeledText alarmTimeout;
	private EventSelector timeoutEvent;
	private LabeledText alarmKeyTerminate;
	private Button checkTerminateWithRegexp;
	private TableViewer actionList;
	private Button addActionButton;
	private Button removeActionButton;
	private Button checkStopProcessing;
	
	/**
	 * Default constructor
	 * 
	 * @param parentShell
	 */
	public EditRuleActionsDlg(Shell parentShell, EventProcessingPolicyEditor editor, EventProcessingPolicyRule rule)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.editor = editor;
		this.rule = rule;
		alarmAction = calculateAlarmAction();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Edit Rule Actions");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);
		
		/* alarm group */
		Group alarmGroup = new Group(dialogArea, SWT.NONE);
		alarmGroup.setText("Alarm");
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmGroup.setLayoutData(gd);
		
		layout = new GridLayout();
		layout.numColumns = 2;
		alarmGroup.setLayout(layout);
		
		Composite radioGroup = new Composite(alarmGroup, SWT.NONE);
		RowLayout rbLayout = new RowLayout(SWT.VERTICAL);
		rbLayout.marginBottom = 0;
		rbLayout.marginTop = 0;
		rbLayout.marginLeft = 0;
		rbLayout.marginRight = 0;
		radioGroup.setLayout(rbLayout);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		radioGroup.setLayoutData(gd);
		
		alarmNoAction = new Button(radioGroup, SWT.RADIO);
		alarmNoAction.setText("Do not change alarms");
		alarmNoAction.setSelection(alarmAction == ALARM_NO_ACTION);
		alarmNoAction.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changeAlarmAction(ALARM_NO_ACTION);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		alarmCreate = new Button(radioGroup, SWT.RADIO);
		alarmCreate.setText("Create new alarm");
		alarmCreate.setSelection(alarmAction == ALARM_CREATE);
		alarmCreate.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changeAlarmAction(ALARM_CREATE);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		alarmTerminate = new Button(radioGroup, SWT.RADIO);
		alarmTerminate.setText("Terminate alarms");
		alarmTerminate.setSelection(alarmAction == ALARM_TERMINATE);
		alarmTerminate.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changeAlarmAction(ALARM_TERMINATE);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		alarmCreationGroup = new Composite(alarmGroup, SWT.NONE);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.TOP;
		gd.exclude = (alarmAction != ALARM_CREATE);
		alarmCreationGroup.setLayoutData(gd);
		alarmCreationGroup.setVisible(alarmAction == ALARM_CREATE);
		layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.marginLeft = 10;
		alarmCreationGroup.setLayout(layout);
		
		alarmMessage = new LabeledText(alarmCreationGroup, SWT.NONE);
		alarmMessage.setLabel("Message");
		alarmMessage.setText(rule.getAlarmMessage());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmMessage.setLayoutData(gd);
		
		alarmKeyCreate = new LabeledText(alarmCreationGroup, SWT.NONE);
		alarmKeyCreate.setLabel("Alarm key");
		alarmKeyCreate.setText(rule.getAlarmKey());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmKeyCreate.setLayoutData(gd);
		
		Composite alarmCreationSubgroup = new Composite(alarmCreationGroup, SWT.NONE);
		layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		alarmCreationSubgroup.setLayout(layout);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmCreationSubgroup.setLayoutData(gd);

		Composite severityGroup = new Composite(alarmCreationSubgroup, SWT.NONE);
		layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		severityGroup.setLayout(layout);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		severityGroup.setLayoutData(gd);
		new Label(severityGroup, SWT.NONE).setText("Alarm severity");
		alarmSeverity = new ImageCombo(severityGroup, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER);
		for(int i = 0; i < Severity.UNKNOWN; i++)
			alarmSeverity.add(StatusDisplayInfo.getStatusImage(i), StatusDisplayInfo.getStatusText(i));
		alarmSeverity.add(StatusDisplayInfo.getStatusImage(Severity.UNKNOWN), "From event");
		alarmSeverity.select(rule.getAlarmSeverity());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmSeverity.setLayoutData(gd);
		
		alarmTimeout = new LabeledText(alarmCreationSubgroup, SWT.NONE);
		alarmTimeout.setLabel("Alarm timeout");
		alarmTimeout.getTextControl().setTextLimit(5);
		alarmTimeout.setText(Integer.toString(rule.getAlarmTimeout()));
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmTimeout.setLayoutData(gd);
		
		timeoutEvent = new EventSelector(alarmCreationGroup, SWT.NONE);
		timeoutEvent.setLabel("Timeout event");
		timeoutEvent.setEventCode(rule.getAlarmTimeoutEvent());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		timeoutEvent.setLayoutData(gd);
		
		alarmTerminationGroup = new Composite(alarmGroup, SWT.NONE);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.TOP;
		gd.exclude = (alarmAction != ALARM_TERMINATE);
		alarmTerminationGroup.setLayoutData(gd);
		alarmTerminationGroup.setVisible(alarmAction == ALARM_TERMINATE);
		layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.marginLeft = 10;
		alarmTerminationGroup.setLayout(layout);
		
		alarmKeyTerminate = new LabeledText(alarmTerminationGroup, SWT.NONE);
		alarmKeyTerminate.setLabel("Terminate all alarms with key");
		alarmKeyTerminate.setText(rule.getAlarmKey());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		alarmKeyTerminate.setLayoutData(gd);
		
		checkTerminateWithRegexp = new Button(alarmTerminationGroup, SWT.CHECK);
		checkTerminateWithRegexp.setText("Use regular expression for alarm termination");
		checkTerminateWithRegexp.setSelection((rule.getFlags() & EventProcessingPolicyRule.TERMINATE_BY_REGEXP) != 0);
		
		/* actions group */
		Group actionsGroup = new Group(dialogArea, SWT.NONE);
		actionsGroup.setText("Actions");
		layout = new GridLayout();
		layout.numColumns = 2;
		actionsGroup.setLayout(layout);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		actionsGroup.setLayoutData(gd);
		
		actionList = new TableViewer(actionsGroup);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.heightHint = 150;
		gd.widthHint = 300;
		gd.verticalSpan = 2;
		actionList.getTable().setLayoutData(gd);
		actionList.setLabelProvider(new WorkbenchLabelProvider());
		actionList.setContentProvider(new ArrayContentProvider());
		actionList.setComparator(new ActionComparator());
		actions.putAll(editor.findServerActions(rule.getActions()));
		actionList.setInput(actions.values().toArray());
		
		Composite actionButtonsGroup = new Composite(actionsGroup, SWT.NONE);
		layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		actionButtonsGroup.setLayout(layout);
		
		addActionButton = new Button(actionButtonsGroup, SWT.PUSH);
		addActionButton.setText("&Add...");
		gd = new GridData();
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		addActionButton.setLayoutData(gd);
		addActionButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addAction();
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		removeActionButton = new Button(actionButtonsGroup, SWT.PUSH);
		removeActionButton.setText("&Delete");
		gd = new GridData();
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		removeActionButton.setLayoutData(gd);
		removeActionButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				removeAction();
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		/* options group */
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText("Options");
		optionsGroup.setLayout(new RowLayout(SWT.VERTICAL));
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		optionsGroup.setLayoutData(gd);
		
		checkStopProcessing = new Button(optionsGroup, SWT.CHECK);
		checkStopProcessing.setText("Stop event processing");
		checkStopProcessing.setSelection((rule.getFlags() & EventProcessingPolicyRule.STOP_PROCESSING) != 0);
		
		return dialogArea;
	}
	
	/**
	 * Calculate alarm action for the rule
	 * 
	 * @return
	 */
	private int calculateAlarmAction()
	{
		if ((rule.getFlags() & EventProcessingPolicyRule.GENERATE_ALARM) == 0)
			return ALARM_NO_ACTION;
		
		if (rule.getAlarmSeverity() < Severity.UNMANAGED)
			return ALARM_CREATE;
		
		return ALARM_TERMINATE;
	}
	
	/**
	 * Change alarm action
	 * @param newAction
	 */
	private void changeAlarmAction(int newAction)
	{
		if (alarmAction != ALARM_NO_ACTION)
		{
			Composite group = (alarmAction == ALARM_CREATE) ? alarmCreationGroup : alarmTerminationGroup;
			((GridData)group.getLayoutData()).exclude = true;
			group.setVisible(false);
		}
		alarmAction = newAction;
		if (alarmAction != ALARM_NO_ACTION)
		{
			Composite group = (alarmAction == ALARM_CREATE) ? alarmCreationGroup : alarmTerminationGroup;
			((GridData)group.getLayoutData()).exclude = false;
			group.setVisible(true);
		}
		((Composite)getDialogArea()).layout();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		switch(alarmAction)
		{
			case ALARM_CREATE:
				try
				{
					int t = Integer.parseInt(alarmTimeout.getText());
					if (t < 0)
					{
						MessageDialog.openWarning(getShell(), "Warning", "Please enter valid timeout value (must be 0 or positive integer");
						return;
					}
					rule.setAlarmTimeout(t);
				}
				catch(NumberFormatException e)
				{
					MessageDialog.openWarning(getShell(), "Warning", "Please enter valid timeout value (must be 0 or positive integer");
					return;
				}
				rule.setAlarmMessage(alarmMessage.getText());
				rule.setAlarmKey(alarmKeyCreate.getText());
				rule.setAlarmSeverity(alarmSeverity.getSelectionIndex());
				rule.setAlarmTimeoutEvent(timeoutEvent.getEventCode());
				rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.GENERATE_ALARM);
				break;
			case ALARM_TERMINATE:
				rule.setAlarmKey(alarmKeyTerminate.getText());
				rule.setAlarmSeverity(Severity.UNMANAGED);
				rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.GENERATE_ALARM);
				if (checkTerminateWithRegexp.getSelection())
					rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.TERMINATE_BY_REGEXP);
				else
					rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.TERMINATE_BY_REGEXP);
				break;
			default:
				rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.GENERATE_ALARM);
				break;
		}
		
		rule.setActions(new ArrayList<Long>(actions.keySet()));
		
		if (checkStopProcessing.getSelection())
			rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.STOP_PROCESSING);
		else
			rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.STOP_PROCESSING);
		
		super.okPressed();
	}
	
	/**
	 * Add action(s)
	 */
	private void addAction()
	{
		ActionSelectionDialog dlg = new ActionSelectionDialog(getShell(), editor.getActions());
		dlg.enableMultiSelection(true);
		if (dlg.open() == Window.OK)
		{
			for(ServerAction a : dlg.getSelectedActions())
				actions.put(a.getId(), a);
			actionList.setInput(actions.values().toArray());
		}
	}
	
	/**
	 * Remove actions
	 */
	@SuppressWarnings("rawtypes")
	private void removeAction()
	{
		IStructuredSelection selection = (IStructuredSelection)actionList.getSelection();
		if (!selection.isEmpty())
		{
			Iterator it = selection.iterator();
			while(it.hasNext())
			{
				ServerAction a = (ServerAction)it.next();
				actions.remove(a.getId());
			}
			actionList.setInput(actions.values().toArray());
		}
	}
}
