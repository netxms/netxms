/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DciSummaryTableConfig;
import org.netxms.ui.eclipse.datacollection.widgets.SummaryTableSelector;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * DCI summary table element properties
 */
public class DciSummaryTable extends PropertyPage
{
	private DciSummaryTableConfig config;
	private ObjectSelector objectSelector;
	private SummaryTableSelector tableSelector;
	private Spinner refreshInterval;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (DciSummaryTableConfig)getElement().getAdapter(DciSummaryTableConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
		objectSelector.setLabel("Base object");
		objectSelector.setObjectClass(AbstractObject.class);
		objectSelector.setObjectId(config.getBaseObjectId());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		objectSelector.setLayoutData(gd);
		
		tableSelector = new SummaryTableSelector(dialogArea, SWT.NONE, AbstractSelector.SHOW_CLEAR_BUTTON);
		tableSelector.setLabel("Summary table");
		tableSelector.setTableId(config.getTableId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      tableSelector.setLayoutData(gd);
      
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      refreshInterval = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, "Refresh interval", 0, 10000, gd);
      refreshInterval.setSelection(config.getRefreshInterval());
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		config.setBaseObjectId(objectSelector.getObjectId());
		config.setTableId(tableSelector.getTableId());
		config.setRefreshInterval(refreshInterval.getSelection());
		return true;
	}
}
