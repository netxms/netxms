/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AbstractChartConfig;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Generic chart properties
 */
public class AbstractChart extends PropertyPage
{
	private AbstractChartConfig config;
	private LabeledText title;
	private Spinner refreshRate;
	private Combo legendPosition;
	private Button checkShowTitle;
	private Button checkShowLegend;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (AbstractChartConfig)getElement().getAdapter(AbstractChartConfig.class);
		
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
		
		legendPosition = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Legend position", WidgetHelper.DEFAULT_LAYOUT_DATA);
		legendPosition.add("Left");
		legendPosition.add("Right");
		legendPosition.add("Top");
		legendPosition.add("Bottom");
		legendPosition.select(positionIndexFromValue(config.getLegendPosition()));
		
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText("Options");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalSpan = 2;
		optionsGroup.setLayoutData(gd);
		GridLayout optionsLayout = new GridLayout();
		optionsGroup.setLayout(optionsLayout);

		checkShowTitle = new Button(optionsGroup, SWT.CHECK);
		checkShowTitle.setText("Show &title");
		checkShowTitle.setSelection(config.isShowTitle());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		checkShowTitle.setLayoutData(gd);
		
		checkShowLegend = new Button(optionsGroup, SWT.CHECK);
		checkShowLegend.setText("Show &legend");
		checkShowLegend.setSelection(config.isShowLegend());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		checkShowLegend.setLayoutData(gd);
		
		refreshRate = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, "Refresh interval", 1, 10000, WidgetHelper.DEFAULT_LAYOUT_DATA);
		refreshRate.setSelection(config.getRefreshRate());
		
		return dialogArea;
	}
	
	/**
	 * @param value
	 * @return
	 */
	private int positionIndexFromValue(int value)
	{
		switch(value)
		{
			case GraphSettings.POSITION_BOTTOM:
				return 3;
			case GraphSettings.POSITION_LEFT:
				return 0;
			case GraphSettings.POSITION_RIGHT:
				return 1;
			case GraphSettings.POSITION_TOP:
				return 2;
		}
		return 0;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		config.setTitle(title.getText());
		config.setLegendPosition(1 << legendPosition.getSelectionIndex());
		config.setShowTitle(checkShowTitle.getSelection());
		config.setShowLegend(checkShowLegend.getSelection());
		config.setRefreshRate(refreshRate.getSelection());
		return true;
	}
}
