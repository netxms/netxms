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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TableComparisonChartConfig;
import org.netxms.ui.eclipse.datacollection.widgets.DciSelector;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Data Source" page for table DCI charts
 */
public class TableDataSource extends PropertyPage
{
	private TableComparisonChartConfig config;
	private DciSelector dci;
	private LabeledText instanceColumn;
	private LabeledText dataColumn;
	private Button checkIgnoreZeroValues;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (TableComparisonChartConfig)getElement().getAdapter(TableComparisonChartConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 1;
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);
		
		dci = new DciSelector(dialogArea, SWT.NONE, false);
		dci.setLabel(Messages.get().TableDataSource_Object);
		dci.setDciId(config.getNodeId(), config.getDciId());
		dci.setDcObjectType(DataCollectionObject.DCO_TYPE_TABLE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dci.setLayoutData(gd);
		
		instanceColumn = new LabeledText(dialogArea, SWT.NONE);
		instanceColumn.setLabel(Messages.get().TableDataSource_InstanceColumn);
		instanceColumn.setText(config.getInstanceColumn());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		instanceColumn.setLayoutData(gd);

		dataColumn = new LabeledText(dialogArea, SWT.NONE);
		dataColumn.setLabel(Messages.get().TableDataSource_DataColumn);
		dataColumn.setText(config.getDataColumn());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dataColumn.setLayoutData(gd);
		
		checkIgnoreZeroValues = new Button(dialogArea, SWT.CHECK);
		checkIgnoreZeroValues.setText(Messages.get().TableDataSource_IgnoreZero);
		checkIgnoreZeroValues.setSelection(config.isIgnoreZeroValues());

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		config.setNodeId(dci.getNodeId());
		config.setDciId(dci.getDciId());
		config.setInstanceColumn(instanceColumn.getText().trim());
		config.setDataColumn(dataColumn.getText().trim());
		config.setIgnoreZeroValues(checkIgnoreZeroValues.getSelection());
		return true;
	}
}
