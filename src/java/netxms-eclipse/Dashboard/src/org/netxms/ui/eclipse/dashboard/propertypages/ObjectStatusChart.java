/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.TitleConfigurator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectStatusChartConfig;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Availability chart element properties
 */
public class ObjectStatusChart extends PropertyPage
{
	private ObjectStatusChartConfig config;
	private ObjectSelector objectSelector;
   private TitleConfigurator title;
	private Spinner refreshRate;
	private Button checkShowLegend;
	private Button checkTransposed;
	private Button checkTranslucent;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (ObjectStatusChartConfig)getElement().getAdapter(ObjectStatusChartConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false, true);
		objectSelector.setLabel(Messages.get().ObjectStatusChart_RootObject);
		objectSelector.setObjectClass(AbstractObject.class);
		objectSelector.setObjectId(config.getRootObject());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		objectSelector.setLayoutData(gd);
		
      title = new TitleConfigurator(dialogArea, config);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		title.setLayoutData(gd);

		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(Messages.get().ObjectStatusChart_Options);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		optionsGroup.setLayoutData(gd);
		GridLayout optionsLayout = new GridLayout();
		optionsGroup.setLayout(optionsLayout);

		checkShowLegend = new Button(optionsGroup, SWT.CHECK);
		checkShowLegend.setText(Messages.get().ObjectStatusChart_ShowLegend);
		checkShowLegend.setSelection(config.isShowLegend());

		checkTransposed = new Button(optionsGroup, SWT.CHECK);
		checkTransposed.setText(Messages.get().ObjectStatusChart_Transposed);
		checkTransposed.setSelection(config.isTransposed());

		checkTranslucent = new Button(optionsGroup, SWT.CHECK);
		checkTranslucent.setText(Messages.get().ObjectStatusChart_Translucent);
		checkTranslucent.setSelection(config.isTranslucent());

		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		refreshRate = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.get().ObjectStatusChart_RefreshInterval, 1, 10000, gd);
		refreshRate.setSelection(config.getRefreshRate());

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
      title.updateConfiguration(config);
		config.setRootObject(objectSelector.getObjectId());
		config.setShowLegend(checkShowLegend.getSelection());
		config.setTransposed(checkTransposed.getSelection());
		config.setTranslucent(checkTranslucent.getSelection());
		config.setRefreshRate(refreshRate.getSelection());
		return true;
	}
}
