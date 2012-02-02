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
package org.netxms.ui.eclipse.datacollection.dialogs;

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
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.eventmanager.widgets.EventSelector;
import org.netxms.ui.eclipse.tools.NumericTextFieldValidator;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Threshold configuration dialog
 *
 */
public class EditThresholdDialog extends Dialog
{
	private static final long serialVersionUID = 1L;

	private Threshold threshold;
	private Combo function;
	private Combo operation;
	private Text samples;
	private Text value;
	private EventSelector activationEvent;
	private EventSelector deactivationEvent;
	private Text repeatInterval;
	private Button repeatDefault;
	private Button repeatNever;
	private Button repeatCustom;
	
	public EditThresholdDialog(Shell parentShell, Threshold threshold)
	{
		super(parentShell);
		this.threshold = threshold;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea =  (Composite)super.createDialogArea(parent);
		
		RowLayout dialogLayout = new RowLayout();
		dialogLayout.type = SWT.VERTICAL;
		dialogLayout.fill = true;
		dialogArea.setLayout(dialogLayout);
		
		// Condition area
		Group condGroup = new Group(dialogArea, SWT.NONE);
		condGroup.setText("Condition");
		
		GridLayout condLayout = new GridLayout();
		condLayout.numColumns = 2;
		condGroup.setLayout(condLayout);
		
		function = WidgetHelper.createLabeledCombo(condGroup, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY, "Function", WidgetHelper.DEFAULT_LAYOUT_DATA);
		function.add("Last polled value");
		function.add("Average value");
		function.add("Mean deviation");
		function.add("Diff with previous value");
		function.add("Data collection error");
		function.add("Sum of values");
		function.select(threshold.getFunction());
		
		samples = WidgetHelper.createLabeledText(condGroup, SWT.BORDER, 60, "Samples", Integer.toString(threshold.getArg1()), WidgetHelper.DEFAULT_LAYOUT_DATA);
		samples.setTextLimit(5);
		
		operation = WidgetHelper.createLabeledCombo(condGroup, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY, "Function", WidgetHelper.DEFAULT_LAYOUT_DATA);
		operation.add("<  : less then");
		operation.add("<= : less then or equal to");
		operation.add("== : equal to");
		operation.add(">= : great then or equal to");
		operation.add(">  : greater then");
		operation.add("!= : not equal to");
		operation.add("like");
		operation.add("not like");
		operation.select(threshold.getOperation());
		
		value = WidgetHelper.createLabeledText(condGroup, SWT.BORDER, 120, "Value", threshold.getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		// Event area
		Group eventGroup = new Group(dialogArea, SWT.NONE);
		eventGroup.setText("Event");
		GridLayout eventLayout = new GridLayout();
		eventGroup.setLayout(eventLayout);
		
		activationEvent = new EventSelector(eventGroup, SWT.NONE);
		activationEvent.setLabel("Activation event");
		activationEvent.setEventCode(threshold.getFireEvent());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		activationEvent.setLayoutData(gd);
		
		deactivationEvent = new EventSelector(eventGroup, SWT.NONE);
		deactivationEvent.setLabel("Deactivation event");
		deactivationEvent.setEventCode(threshold.getRearmEvent());
		deactivationEvent.setLayoutData(gd);
		
		// Repeat area
		Group repeatGroup = new Group(dialogArea, SWT.NONE);
		repeatGroup.setText("Repeat event");
		GridLayout repeatLayout = new GridLayout();
		repeatLayout.numColumns = 3;
		repeatGroup.setLayout(repeatLayout);
		
		repeatDefault = new Button(repeatGroup, SWT.RADIO);
		repeatDefault.setText("Use &default settings");
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.horizontalSpan = 3;
		repeatDefault.setLayoutData(gd);
		repeatDefault.setSelection(threshold.getRepeatInterval() == -1);
		
		repeatNever = new Button(repeatGroup, SWT.RADIO);
		repeatNever.setText("&Never");
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.horizontalSpan = 3;
		repeatNever.setLayoutData(gd);
		repeatNever.setSelection(threshold.getRepeatInterval() == 0);
		
		repeatCustom = new Button(repeatGroup, SWT.RADIO);
		repeatCustom.setText("&Every");
		repeatCustom.setSelection(threshold.getRepeatInterval() > 0);
		repeatCustom.addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				repeatInterval.setEnabled(repeatCustom.getSelection());
			}
		});
		
		repeatInterval = new Text(repeatGroup, SWT.BORDER);
		repeatInterval.setTextLimit(5);
		repeatInterval.setText((threshold.getRepeatInterval() > 0) ? Integer.toString(threshold.getRepeatInterval()) : "3600");
		repeatInterval.setEnabled(threshold.getRepeatInterval() > 0);
		
		new Label(repeatGroup, SWT.NONE).setText("seconds");
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Edit Threshold");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		if (!WidgetHelper.validateTextInput(samples, "Samples", new NumericTextFieldValidator(1, 1000), null))
			return;
		
		int rpt;
		if (repeatDefault.getSelection())
		{
			rpt = -1;
		}
		else if (repeatNever.getSelection())
		{
			rpt = 0;
		}
		else
		{
			if (!WidgetHelper.validateTextInput(repeatInterval, "Repeat Interval", new NumericTextFieldValidator(1, 1000000), null))
				return;
			rpt = Integer.parseInt(repeatInterval.getText());
		}
		
		threshold.setFunction(function.getSelectionIndex());
		threshold.setOperation(operation.getSelectionIndex());
		threshold.setValue(value.getText());
		threshold.setArg1(Integer.parseInt(samples.getText()));
		threshold.setRepeatInterval(rpt);
		threshold.setFireEvent((int)activationEvent.getEventCode());
		threshold.setRearmEvent((int)deactivationEvent.getEventCode());
				
		super.okPressed();
	}
}
