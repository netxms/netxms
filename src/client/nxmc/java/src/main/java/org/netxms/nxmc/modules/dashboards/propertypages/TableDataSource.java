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
package org.netxms.nxmc.modules.dashboards.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.TableComparisonChartConfig;
import org.netxms.nxmc.modules.datacollection.widgets.DciSelector;
import org.xnap.commons.i18n.I18n;

/**
 * "Data Source" page for table DCI charts
 */
public class TableDataSource extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(TableDataSource.class);

	private TableComparisonChartConfig config;
	private DciSelector dci;
	private LabeledText instanceColumn;
	private LabeledText dataColumn;
	private Button checkIgnoreZeroValues;
   private Button checkSortOnDataColumn;
   private Button checkSortDescending;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public TableDataSource(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(TableDataSource.class).tr("Data Source"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "table-data-source";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof TableComparisonChartConfig;
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 20;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      config = (TableComparisonChartConfig)elementConfig;

		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.numColumns = 1;
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);

      dci = new DciSelector(dialogArea, SWT.NONE);
      dci.setLabel(i18n.tr("Table DCI"));
		dci.setDciId(config.getNodeId(), config.getDciId());
		dci.setDcObjectType(DataCollectionObject.DCO_TYPE_TABLE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dci.setLayoutData(gd);

		instanceColumn = new LabeledText(dialogArea, SWT.NONE);
      instanceColumn.setLabel(i18n.tr("Instance column"));
		instanceColumn.setText(config.getInstanceColumn());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		instanceColumn.setLayoutData(gd);

		dataColumn = new LabeledText(dialogArea, SWT.NONE);
      dataColumn.setLabel(i18n.tr("Data column"));
		dataColumn.setText(config.getDataColumn());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dataColumn.setLayoutData(gd);

		checkIgnoreZeroValues = new Button(dialogArea, SWT.CHECK);
      checkIgnoreZeroValues.setText(i18n.tr("&Ignore zero values when building chart"));
		checkIgnoreZeroValues.setSelection(config.isIgnoreZeroValues());

      checkSortOnDataColumn = new Button(dialogArea, SWT.CHECK);
      checkSortOnDataColumn.setText(i18n.tr("&Sort table on data column"));
      checkSortOnDataColumn.setSelection(config.isSortOnDataColumn());

      checkSortDescending = new Button(dialogArea, SWT.CHECK);
      checkSortDescending.setText(i18n.tr("Sort &descending"));
      checkSortDescending.setSelection(config.isSortDescending());

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		config.setNodeId(dci.getNodeId());
		config.setDciId(dci.getDciId());
		config.setInstanceColumn(instanceColumn.getText().trim());
		config.setDataColumn(dataColumn.getText().trim());
		config.setIgnoreZeroValues(checkIgnoreZeroValues.getSelection());
		config.setSortOnDataColumn(checkSortOnDataColumn.getSelection());
		config.setSortDescending(checkSortDescending.getSelection());
		return true;
	}
}
