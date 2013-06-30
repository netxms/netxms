/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.propertypages;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.epp.Messages;
import org.netxms.ui.eclipse.epp.widgets.RuleEditor;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.ImageCombo;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Alarm" property page for EPP rule
 *
 */
public class RuleAlarm extends PropertyPage
{
	private static final int ALARM_NO_ACTION = 0;
	private static final int ALARM_CREATE = 1;
	private static final int ALARM_RESOLVE = 2;
	private static final int ALARM_TERMINATE = 3;
	
	private RuleEditor editor;
	private EventProcessingPolicyRule rule;
	private int alarmAction;
	
	private Composite dialogArea;
	private Button alarmNoAction;
	private Button alarmCreate;
	private Button alarmResolve;
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		editor = (RuleEditor)getElement().getAdapter(RuleEditor.class);
		rule = editor.getRule();
		alarmAction = calculateAlarmAction();
		
		dialogArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING * 2;
      dialogArea.setLayout(layout);

		Composite radioGroup = new Composite(dialogArea, SWT.NONE);
		RowLayout rbLayout = new RowLayout(SWT.VERTICAL);
		rbLayout.marginBottom = 0;
		rbLayout.marginTop = 0;
		rbLayout.marginLeft = 0;
		rbLayout.marginRight = 0;
		radioGroup.setLayout(rbLayout);
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		radioGroup.setLayoutData(gd);
		
		alarmNoAction = new Button(radioGroup, SWT.RADIO);
		alarmNoAction.setText(Messages.RuleAlarm_DoNotChange);
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
		alarmCreate.setText(Messages.RuleAlarm_CreateNew);
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
		
		alarmResolve = new Button(radioGroup, SWT.RADIO);
		alarmResolve.setText(Messages.RuleAlarm_Resolve);
		alarmResolve.setSelection(alarmAction == ALARM_RESOLVE);
		alarmResolve.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changeAlarmAction(ALARM_RESOLVE);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		alarmTerminate = new Button(radioGroup, SWT.RADIO);
		alarmTerminate.setText(Messages.RuleAlarm_Terminate);
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
		
		alarmCreationGroup = new Composite(dialogArea, SWT.NONE);
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
		alarmCreationGroup.setLayout(layout);
		
		alarmMessage = new LabeledText(alarmCreationGroup, SWT.NONE);
		alarmMessage.setLabel(Messages.RuleAlarm_Message);
		alarmMessage.setText(rule.getAlarmMessage());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmMessage.setLayoutData(gd);
		
		alarmKeyCreate = new LabeledText(alarmCreationGroup, SWT.NONE);
		alarmKeyCreate.setLabel(Messages.RuleAlarm_Key);
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
		new Label(severityGroup, SWT.NONE).setText(Messages.RuleAlarm_Severity);
		alarmSeverity = new ImageCombo(severityGroup, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER);
		for(int i = 0; i < Severity.UNKNOWN; i++)
			alarmSeverity.add(StatusDisplayInfo.getStatusImage(i), StatusDisplayInfo.getStatusText(i));
		alarmSeverity.add(StatusDisplayInfo.getStatusImage(Severity.UNKNOWN), Messages.RuleAlarm_FromEvent);
		alarmSeverity.select(rule.getAlarmSeverity());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmSeverity.setLayoutData(gd);
		
		alarmTimeout = new LabeledText(alarmCreationSubgroup, SWT.NONE);
		alarmTimeout.setLabel(Messages.RuleAlarm_Timeout);
		alarmTimeout.getTextControl().setTextLimit(5);
		alarmTimeout.setText(Integer.toString(rule.getAlarmTimeout()));
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmTimeout.setLayoutData(gd);
		
		timeoutEvent = new EventSelector(alarmCreationGroup, SWT.NONE);
		timeoutEvent.setLabel(Messages.RuleAlarm_TimeoutEvent);
		timeoutEvent.setEventCode(rule.getAlarmTimeoutEvent());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		timeoutEvent.setLayoutData(gd);
		
		alarmTerminationGroup = new Composite(dialogArea, SWT.NONE);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.TOP;
		gd.exclude = ((alarmAction != ALARM_TERMINATE) && (alarmAction != ALARM_RESOLVE));
		alarmTerminationGroup.setLayoutData(gd);
		alarmTerminationGroup.setVisible((alarmAction == ALARM_TERMINATE) || (alarmAction == ALARM_RESOLVE));
		layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		alarmTerminationGroup.setLayout(layout);
		
		alarmKeyTerminate = new LabeledText(alarmTerminationGroup, SWT.NONE);
		alarmKeyTerminate.setLabel((alarmAction == ALARM_TERMINATE)? Messages.RuleAlarm_TerminateAll : Messages.RuleAlarm_ResolveAll);
		alarmKeyTerminate.setText(rule.getAlarmKey());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		alarmKeyTerminate.setLayoutData(gd);
		
		checkTerminateWithRegexp = new Button(alarmTerminationGroup, SWT.CHECK);
		checkTerminateWithRegexp.setText((alarmAction == ALARM_TERMINATE)? Messages.RuleAlarm_UseRegexpForTerminate : Messages.RuleAlarm_UseRegexpForResolve);
		checkTerminateWithRegexp.setSelection((rule.getFlags() & EventProcessingPolicyRule.TERMINATE_BY_REGEXP) != 0);

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
		
		if (rule.getAlarmSeverity() == Severity.DISABLED)
			return ALARM_RESOLVE;
		
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
			alarmKeyTerminate.setLabel((alarmAction == ALARM_TERMINATE)? Messages.RuleAlarm_TerminateAll : Messages.RuleAlarm_ResolveAll);
			checkTerminateWithRegexp.setText((alarmAction == ALARM_TERMINATE)? Messages.RuleAlarm_UseRegexpForTerminate : Messages.RuleAlarm_UseRegexpForResolve);
		}
		dialogArea.layout(true, true);
	}
	
	/**
	 * Apply data
	 */
	private boolean doApply()
	{
		switch(alarmAction)
		{
			case ALARM_CREATE:
				try
				{
					int t = Integer.parseInt(alarmTimeout.getText());
					if (t < 0)
					{
						MessageDialogHelper.openWarning(getShell(), Messages.RuleAlarm_Warning, Messages.RuleAlarm_WarningInvalidTimeout);
						return false;
					}
					rule.setAlarmTimeout(t);
				}
				catch(NumberFormatException e)
				{
					MessageDialog.openWarning(getShell(), Messages.RuleAlarm_Warning, Messages.RuleAlarm_WarningInvalidTimeout);
					return false;
				}
				rule.setAlarmMessage(alarmMessage.getText());
				rule.setAlarmKey(alarmKeyCreate.getText());
				rule.setAlarmSeverity(alarmSeverity.getSelectionIndex());
				rule.setAlarmTimeoutEvent(timeoutEvent.getEventCode());
				rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.GENERATE_ALARM);
				break;
			case ALARM_RESOLVE:
			case ALARM_TERMINATE:
				rule.setAlarmKey(alarmKeyTerminate.getText());
				rule.setAlarmSeverity((alarmAction == ALARM_TERMINATE) ? Severity.UNMANAGED : Severity.DISABLED);
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
		editor.setModified(true);
		return true;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		doApply();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		return doApply();
	}
}
