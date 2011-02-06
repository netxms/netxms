/**
 * 
 */
package org.netxms.ui.eclipse.epp.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author victor
 *
 */
public class EditRuleActionsDlg extends Dialog
{
	private EventProcessingPolicyRule rule;
	
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
}
