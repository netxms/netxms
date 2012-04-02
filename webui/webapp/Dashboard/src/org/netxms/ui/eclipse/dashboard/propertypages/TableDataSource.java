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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TableComparisonChartConfig;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Data Source" page for table DCI charts
 */
public class TableDataSource extends PropertyPage
{
	private TableComparisonChartConfig config;
	private ObjectSelector node;
	private LabeledText instanceColumn;
	private LabeledText dataColumn;
	
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
		
		node = new ObjectSelector(dialogArea, SWT.NONE);
		node.setLabel("Node");
		node.setObjectClass(Node.class);
		node.setObjectId(config.getNodeId());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		node.setLayoutData(gd);
		
		instanceColumn = new LabeledText(dialogArea, SWT.NONE);
		instanceColumn.setLabel("Instance column");
		instanceColumn.setText(config.getInstanceColumn());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		instanceColumn.setLayoutData(gd);

		dataColumn = new LabeledText(dialogArea, SWT.NONE);
		dataColumn.setLabel("Data column");
		dataColumn.setText(config.getDataColumn());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dataColumn.setLayoutData(gd);

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		config.setNodeId(node.getObjectId());
		config.setInstanceColumn(instanceColumn.getText().trim());
		config.setDataColumn(dataColumn.getText().trim());
		return true;
	}
}
