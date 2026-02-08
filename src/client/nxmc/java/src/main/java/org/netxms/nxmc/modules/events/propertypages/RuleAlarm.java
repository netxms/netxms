/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.widgets.AlarmCategorySelector;
import org.netxms.nxmc.modules.events.widgets.EventSelector;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptSelector;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Alarm" property page for EPP rule
 */
public class RuleAlarm extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleAlarm.class);

	private static final int ALARM_NO_ACTION = 0;
	private static final int ALARM_CREATE = 1;
	private static final int ALARM_RESOLVE = 2;
	private static final int ALARM_TERMINATE = 3;

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
   private Combo alarmSeverity;
	private LabeledText alarmTimeout;
	private AlarmCategorySelector alarmCategory;
	private EventSelector timeoutEvent;
	private ScriptSelector rcaScript;
	private LabeledText alarmKeyTerminate;
	private Button checkTerminateWithRegexp;
	private Button checkCreateHelpdeskTicket;
   private Button checkRequestAiComment;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleAlarm(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleAlarm.class).tr("Alarm"));
   }

	/**
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
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
      alarmNoAction.setText(i18n.tr("Do not change alarms"));
		alarmNoAction.setSelection(alarmAction == ALARM_NO_ACTION);
      alarmNoAction.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changeAlarmAction(ALARM_NO_ACTION);
			}
		});

		alarmCreate = new Button(radioGroup, SWT.RADIO);
      alarmCreate.setText(i18n.tr("Create new alarm"));
		alarmCreate.setSelection(alarmAction == ALARM_CREATE);
      alarmCreate.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changeAlarmAction(ALARM_CREATE);
			}
		});

		alarmResolve = new Button(radioGroup, SWT.RADIO);
      alarmResolve.setText(i18n.tr("Resolve alarms"));
		alarmResolve.setSelection(alarmAction == ALARM_RESOLVE);
      alarmResolve.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changeAlarmAction(ALARM_RESOLVE);
			}
		});

		alarmTerminate = new Button(radioGroup, SWT.RADIO);
      alarmTerminate.setText(i18n.tr("Terminate alarms"));
		alarmTerminate.setSelection(alarmAction == ALARM_TERMINATE);
      alarmTerminate.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				changeAlarmAction(ALARM_TERMINATE);
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
      alarmMessage.setLabel(i18n.tr("Message"));
		alarmMessage.setText(rule.getAlarmMessage());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmMessage.setLayoutData(gd);

		alarmKeyCreate = new LabeledText(alarmCreationGroup, SWT.NONE);
      alarmKeyCreate.setLabel(i18n.tr("Alarm key"));
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
      new Label(severityGroup, SWT.NONE).setText(i18n.tr("Alarm severity"));
      alarmSeverity = new Combo(severityGroup, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER);
		for(int i = 0; i < Severity.UNKNOWN.getValue(); i++)
         alarmSeverity.add(StatusDisplayInfo.getStatusText(i));
      alarmSeverity.add(i18n.tr("From event"));
		alarmSeverity.select(rule.getAlarmSeverity().getValue());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmSeverity.setLayoutData(gd);

		alarmTimeout = new LabeledText(alarmCreationSubgroup, SWT.NONE);
      alarmTimeout.setLabel(i18n.tr("Alarm timeout"));
		alarmTimeout.getTextControl().setTextLimit(5);
		alarmTimeout.setText(Integer.toString(rule.getAlarmTimeout()));
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		alarmTimeout.setLayoutData(gd);

		timeoutEvent = new EventSelector(alarmCreationGroup, SWT.NONE);
      timeoutEvent.setLabel(i18n.tr("Timeout event"));
		timeoutEvent.setEventCode(rule.getAlarmTimeoutEvent());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		timeoutEvent.setLayoutData(gd);

      rcaScript = new ScriptSelector(alarmCreationGroup, SWT.NONE, false, true);
      rcaScript.setLabel("Root cause analysis script");
      rcaScript.setScriptName(rule.getRcaScriptName());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      rcaScript.setLayoutData(gd);
		
      alarmCategory = new AlarmCategorySelector(alarmCreationGroup, SWT.NONE);
      alarmCategory.setLabel(i18n.tr("Alarm category"));
      alarmCategory.setCategoryId(rule.getAlarmCategories());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      alarmCategory.setLayoutData(gd);

      checkCreateHelpdeskTicket = new Button(alarmCreationGroup, SWT.CHECK);
      checkCreateHelpdeskTicket.setText(i18n.tr("Create helpdesk ticket on alarm creation"));
      checkCreateHelpdeskTicket.setSelection((rule.getFlags() & EventProcessingPolicyRule.CREATE_TICKET) != 0);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      checkCreateHelpdeskTicket.setLayoutData(gd);

      checkRequestAiComment = new Button(alarmCreationGroup, SWT.CHECK);
      checkRequestAiComment.setText(i18n.tr("Add AI assistant's comment on alarm creation"));
      checkRequestAiComment.setSelection((rule.getFlags() & EventProcessingPolicyRule.REQUEST_AI_COMMENT) != 0);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      checkRequestAiComment.setLayoutData(gd);

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
      alarmKeyTerminate.setLabel((alarmAction == ALARM_TERMINATE) ? i18n.tr("Terminate all alarms with key") : i18n.tr("Resolve all alarms with key"));
		alarmKeyTerminate.setText(rule.getAlarmKey());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		alarmKeyTerminate.setLayoutData(gd);

		checkTerminateWithRegexp = new Button(alarmTerminationGroup, SWT.CHECK);
      checkTerminateWithRegexp.setText((alarmAction == ALARM_TERMINATE) ? i18n.tr("Use regular expression for alarm termination") : i18n.tr("Use regular expression for alarm resolving"));
		checkTerminateWithRegexp.setSelection((rule.getFlags() & EventProcessingPolicyRule.TERMINATE_BY_REGEXP) != 0);

      return dialogArea;
	}

	/**
	 * Calculate alarm action for the rule
	 * 
	 * @return alarm action for the rule
	 */
	private int calculateAlarmAction()
	{
		if ((rule.getFlags() & EventProcessingPolicyRule.GENERATE_ALARM) == 0)
			return ALARM_NO_ACTION;
		
		if (rule.getAlarmSeverity().compareTo(Severity.TERMINATE) < 0)
			return ALARM_CREATE;
		
		if (rule.getAlarmSeverity() == Severity.RESOLVE)
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
         alarmKeyTerminate.setLabel((alarmAction == ALARM_TERMINATE) ? i18n.tr("Terminate all alarms with key") : i18n.tr("Resolve all alarms with key"));
         checkTerminateWithRegexp.setText((alarmAction == ALARM_TERMINATE) ? i18n.tr("Use regular expression for alarm termination") : i18n.tr("Use regular expression for alarm resolving"));
		}
		dialogArea.layout(true, true);
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		switch(alarmAction)
		{
			case ALARM_CREATE:
				try
				{
					int t = Integer.parseInt(alarmTimeout.getText());
					if (t < 0)
					{
                  MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid timeout value (must be 0 or positive integer)"));
						return false;
					}
					rule.setAlarmTimeout(t);
				}
				catch(NumberFormatException e)
				{
               MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid timeout value (must be 0 or positive integer)"));
					return false;
				}
				rule.setAlarmMessage(alarmMessage.getText());
				rule.setAlarmKey(alarmKeyCreate.getText());
				rule.setAlarmSeverity(Severity.getByValue(alarmSeverity.getSelectionIndex()));
				rule.setAlarmTimeoutEvent(timeoutEvent.getEventCode());
				rule.setAlarmCategories(alarmCategory.getCategoryId());				
				rule.setRcaScriptName(rcaScript.getScriptName());
				rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.GENERATE_ALARM);
				if (checkCreateHelpdeskTicket.getSelection())
				   rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.CREATE_TICKET);
				else
               rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.CREATE_TICKET);
            if (checkRequestAiComment.getSelection())
               rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.REQUEST_AI_COMMENT);
            else
               rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.REQUEST_AI_COMMENT);
				break;
			case ALARM_RESOLVE:
			case ALARM_TERMINATE:
				rule.setAlarmKey(alarmKeyTerminate.getText());
				rule.setAlarmSeverity((alarmAction == ALARM_TERMINATE) ? Severity.TERMINATE : Severity.RESOLVE);
				rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.GENERATE_ALARM);
				if (checkTerminateWithRegexp.getSelection())
					rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.TERMINATE_BY_REGEXP);
				else
					rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.TERMINATE_BY_REGEXP);
            rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.CREATE_TICKET);
				break;
			default:
				rule.setFlags(rule.getFlags() & ~(EventProcessingPolicyRule.GENERATE_ALARM | EventProcessingPolicyRule.CREATE_TICKET));
				break;
		}
		editor.setModified(true);
		return true;
	}
}
