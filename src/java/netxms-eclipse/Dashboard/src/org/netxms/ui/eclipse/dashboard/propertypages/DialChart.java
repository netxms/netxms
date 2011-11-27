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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DialChartConfig;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Dial chart properties
 */
public class DialChart extends PropertyPage
{
	private DialChartConfig config;
	private LabeledText title;
	private LabeledText maxValue;
	private LabeledText yellowZone;
	private LabeledText redZone;

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
		
		title = new LabeledText(dialogArea, SWT.NONE);
		title.setLabel("Title");
		title.setText(config.getTitle());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		title.setLayoutData(gd);
		
		maxValue = new LabeledText(dialogArea, SWT.NONE);
		maxValue.setLabel("Maximum value");
		maxValue.setText(Double.toString(config.getMaxValue()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		maxValue.setLayoutData(gd);
		
		yellowZone = new LabeledText(dialogArea, SWT.NONE);
		yellowZone.setLabel("Yellow zone start");
		yellowZone.setText(Double.toString(config.getYellowZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		yellowZone.setLayoutData(gd);
		
		new Label(dialogArea, SWT.NONE).setText("");	// placeholder
		
		redZone = new LabeledText(dialogArea, SWT.NONE);
		redZone.setLabel("Red zone start");
		redZone.setText(Double.toString(config.getRedZone()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		redZone.setLayoutData(gd);
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		double m, y, r;
		try
		{
			m = Double.parseDouble(maxValue.getText().trim());
			y = Double.parseDouble(yellowZone.getText().trim());
			r = Double.parseDouble(redZone.getText().trim());
		}
		catch(NumberFormatException e)
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please enter correct number");
			return false;
		}
		
		config.setTitle(title.getText());
		config.setMaxValue(m);
		config.setYellowZone(y);
		config.setRedZone(r);
		return true;
	}
}
