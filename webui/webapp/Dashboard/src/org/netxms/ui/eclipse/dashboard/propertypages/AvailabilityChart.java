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

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.ServiceContainer;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AvailabilityChartConfig;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Availability chart element properties
 */
public class AvailabilityChart extends PropertyPage
{
	private AvailabilityChartConfig config;
	private ObjectSelector objectSelector;
	private LabeledText title;
	private Button checkShowTitle;
	private Button checkShowLegend;
	private Button checkShow3D;
	private Button checkTranslucent;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (AvailabilityChartConfig)getElement().getAdapter(AvailabilityChartConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
		objectSelector.setLabel(Messages.AvailabilityChart_Object);
		objectSelector.setObjectClass(ServiceContainer.class);
		objectSelector.setObjectId(config.getObjectId());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		objectSelector.setLayoutData(gd);
		
		title = new LabeledText(dialogArea, SWT.NONE);
		title.setLabel(Messages.AvailabilityChart_Title);
		title.setText(config.getTitle());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		title.setLayoutData(gd);
		
		checkShowTitle = new Button(dialogArea, SWT.CHECK);
		checkShowTitle.setText(Messages.AvailabilityChart_ShowTitle);
		checkShowTitle.setSelection(config.isShowTitle());
		
		checkShowLegend = new Button(dialogArea, SWT.CHECK);
		checkShowLegend.setText(Messages.AvailabilityChart_ShowLegend);
		checkShowLegend.setSelection(config.isShowLegend());
		
		checkShow3D = new Button(dialogArea, SWT.CHECK);
		checkShow3D.setText(Messages.AvailabilityChart_3DView);
		checkShow3D.setSelection(config.isShowIn3D());
		
		checkTranslucent = new Button(dialogArea, SWT.CHECK);
		checkTranslucent.setText(Messages.AvailabilityChart_Translucent);
		checkTranslucent.setSelection(config.isTranslucent());
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		config.setObjectId(objectSelector.getObjectId());
		config.setTitle(title.getText());
		config.setShowTitle(checkShowTitle.getSelection());
		config.setShowLegend(checkShowLegend.getSelection());
		config.setShowIn3D(checkShow3D.getSelection());
		config.setTranslucent(checkTranslucent.getSelection());
		return true;
	}
}
