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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DialChartConfig;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dial chart properties
 */
public class DialChart extends PropertyPage
{
	private DialChartConfig config;
	private Button checkLegendInside;
	private LabeledText minValue;
	private LabeledText maxValue;
	private LabeledText leftYellowZone;
	private LabeledText leftRedZone;
	private LabeledText rightYellowZone;
	private LabeledText rightRedZone;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (DialChartConfig)getElement().getAdapter(DialChartConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);
		
		minValue = new LabeledText(dialogArea, SWT.NONE);
		minValue.setLabel("Minimum value");
		minValue.setText(Double.toString(config.getMinValue()));
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		minValue.setLayoutData(gd);
		
		maxValue = new LabeledText(dialogArea, SWT.NONE);
		maxValue.setLabel("Maximum value");
		maxValue.setText(Double.toString(config.getMaxValue()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		maxValue.setLayoutData(gd);
		
		leftRedZone = new LabeledText(dialogArea, SWT.NONE);
		leftRedZone.setLabel("Left red zone end");
		leftRedZone.setText(Double.toString(config.getLeftRedZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		leftRedZone.setLayoutData(gd);
		
		leftYellowZone = new LabeledText(dialogArea, SWT.NONE);
		leftYellowZone.setLabel("Left yellow zone end");
		leftYellowZone.setText(Double.toString(config.getLeftYellowZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		leftYellowZone.setLayoutData(gd);
		
		rightYellowZone = new LabeledText(dialogArea, SWT.NONE);
		rightYellowZone.setLabel("Right yellow zone start");
		rightYellowZone.setText(Double.toString(config.getRightYellowZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		rightYellowZone.setLayoutData(gd);
		
		rightRedZone = new LabeledText(dialogArea, SWT.NONE);
		rightRedZone.setLabel("Right red zone start");
		rightRedZone.setText(Double.toString(config.getRightRedZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		rightRedZone.setLayoutData(gd);
		
		checkLegendInside = new Button(dialogArea, SWT.CHECK);
		checkLegendInside.setText("Place legend &inside dial");
		checkLegendInside.setSelection(config.isLegendInside());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		checkLegendInside.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		double min, max, ly, lr, ry, rr;
		try
		{
			min = Double.parseDouble(minValue.getText().trim());
			max = Double.parseDouble(maxValue.getText().trim());
			ly = Double.parseDouble(leftYellowZone.getText().trim());
			lr = Double.parseDouble(leftRedZone.getText().trim());
			ry = Double.parseDouble(rightYellowZone.getText().trim());
			rr = Double.parseDouble(rightRedZone.getText().trim());
		}
		catch(NumberFormatException e)
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please enter correct number");
			return false;
		}
		
		config.setMinValue(min);
		config.setMaxValue(max);
		config.setLeftYellowZone(ly);
		config.setLeftRedZone(lr);
		config.setRightYellowZone(ry);
		config.setRightRedZone(rr);
		config.setLegendInside(checkLegendInside.getSelection());
		return true;
	}
}
