/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Threshold configuration dialog
 *
 */
public class EditThresholdDialog extends Dialog
{
	private Threshold threshold;
	private Combo function;
	private Combo operation;
	private Text samples;
	private Text value;
	
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
		
		function = WidgetHelper.createLabeledCombo(condGroup, SWT.BORDER | SWT.DROP_DOWN, "Function", WidgetHelper.DEFAULT_LAYOUT_DATA);
		function.add("Last polled value");
		function.add("Average value");
		function.add("Mean deviation");
		function.add("Diff with previous value");
		function.add("Data collection error");
		function.select(threshold.getFunction());
		
		samples = WidgetHelper.createLabeledText(condGroup, SWT.BORDER, 60, "Samples", Integer.toString(threshold.getArg1()), WidgetHelper.DEFAULT_LAYOUT_DATA);
		samples.setTextLimit(5);
		
		operation = WidgetHelper.createLabeledCombo(condGroup, SWT.BORDER | SWT.DROP_DOWN, "Function", WidgetHelper.DEFAULT_LAYOUT_DATA);
		operation.add("<  : less then");
		operation.add("<= : less then or equal to");
		operation.add("== : equal to");
		operation.add(">= : great then or equal to");
		operation.add(">  : greater then");
		operation.select(threshold.getOperation());
		
		value = WidgetHelper.createLabeledText(condGroup, SWT.BORDER, 120, "Value", threshold.getValue(), WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		// Event area
		Group eventGroup = new Group(dialogArea, SWT.NONE);
		eventGroup.setText("Event");
		
		return dialogArea;
	}
}
