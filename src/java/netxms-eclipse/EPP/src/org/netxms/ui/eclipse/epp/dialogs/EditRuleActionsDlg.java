/**
 * 
 */
package org.netxms.ui.eclipse.epp.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.TableViewer;
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
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.ImageCombo;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * @author victor
 *
 */
public class EditRuleActionsDlg extends Dialog
{
	private static final int ALARM_NO_ACTION = 0;
	private static final int ALARM_CREATE = 1;
	private static final int ALARM_TERMINATE = 2;
	
	private EventProcessingPolicyRule rule;
	private int alarmAction;
	
	private Button alarmNoAction;
	private Button alarmCreate;
	private Button alarmTerminate;
	private Composite alarmCreationGroup;
	private Composite alarmTerminationGroup;
	private LabeledText alarmMessage;
	private LabeledText alarmKeyCreate;
	private ImageCombo alarmSeverity;
	private LabeledText alarmKeyTerminate;
	private Button checkTerminateWithRegexp;
	private TableViewer actionList;
	private Button addAction;
	private Button removeAction;
	private Button checkStopProcessing;
	
	/**
	 * Default constructor
	 * 
	 * @param parentShell
	 */
	public EditRuleActionsDlg(Shell parentShell, EventProcessingPolicyRule rule)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
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
		
		alarmSeverity = new ImageCombo(alarmCreationGroup, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER);
		for(int i = 0; i < Severity.UNKNOWN; i++)
			alarmSeverity.add(StatusDisplayInfo.getStatusImage(i), StatusDisplayInfo.getStatusText(i));
		alarmSeverity.add(StatusDisplayInfo.getStatusImage(Severity.UNKNOWN), "From event");
		alarmSeverity.select(rule.getAlarmSeverity());
		
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
		gd.heightHint = 200;
		gd.widthHint = 300;
		gd.verticalSpan = 2;
		actionList.getTable().setLayoutData(gd);
		
		addAction = new Button(actionsGroup, SWT.PUSH);
		addAction.setText("&Add...");
		gd = new GridData();
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		addAction.setLayoutData(gd);
		
		removeAction = new Button(actionsGroup, SWT.PUSH);
		removeAction.setText("&Delete");
		gd = new GridData();
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		removeAction.setLayoutData(gd);
		
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
}
