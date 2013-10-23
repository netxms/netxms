/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TableValueConfig;
import org.netxms.ui.eclipse.datacollection.widgets.DciSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Table last value element properties
 */
public class TableValue extends PropertyPage
{
	private TableValueConfig config;
	private DciSelector dciSelector;
	private LabeledText title;
	private Spinner refreshRate;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (TableValueConfig)getElement().getAdapter(TableValueConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		dciSelector = new DciSelector(dialogArea, SWT.NONE, true);
		dciSelector.setLabel(Messages.TableValue_Table);
		dciSelector.setDcObjectType(DataCollectionObject.DCO_TYPE_TABLE);
		dciSelector.setDciId(config.getObjectId(), config.getDciId());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dciSelector.setLayoutData(gd);
		
		title = new LabeledText(dialogArea, SWT.NONE);
		title.setLabel(Messages.AlarmViewer_Title);
		title.setText(config.getTitle());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		title.setLayoutData(gd);
		
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		refreshRate = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.AbstractChart_RefreshInterval, 1, 10000, gd);
		refreshRate.setSelection(config.getRefreshRate());

		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		config.setObjectId(dciSelector.getNodeId());
		config.setDciId(dciSelector.getDciId());
		config.setTitle(title.getText());
		config.setRefreshRate(refreshRate.getSelection());
		return true;
	}
}
