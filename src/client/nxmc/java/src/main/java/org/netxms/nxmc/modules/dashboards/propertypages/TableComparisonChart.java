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
package org.netxms.nxmc.modules.dashboards.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.TableBarChartConfig;
import org.netxms.nxmc.modules.dashboards.config.TableComparisonChartConfig;
import org.netxms.nxmc.modules.dashboards.config.TablePieChartConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.datacollection.widgets.YAxisRangeEditor;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Table comparison chart properties
 */
public class TableComparisonChart extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(TableComparisonChart.class);

	private TableComparisonChartConfig config;
   private TitleConfigurator title;
	private Spinner refreshRate;
	private Combo legendPosition;
	private Button checkShowLegend;
   private Button checkExtendedLegend;
	private Button checkTranslucent;
	private Button checkTransposed;
   private YAxisRangeEditor yAxisRange;
   private ObjectSelector drillDownObject;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public TableComparisonChart(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(TableComparisonChart.class).tr("Chart"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "table-comparison-chart";
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
      return 0;
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
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      title.setLayoutData(gd);

      legendPosition = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Legend position"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      legendPosition.add(i18n.tr("Left"));
      legendPosition.add(i18n.tr("Right"));
      legendPosition.add(i18n.tr("Top"));
      legendPosition.add(i18n.tr("Bottom"));
		legendPosition.select(positionIndexFromValue(config.getLegendPosition()));

		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(i18n.tr("Options"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalSpan = 2;
		optionsGroup.setLayoutData(gd);
		GridLayout optionsLayout = new GridLayout();
		optionsGroup.setLayout(optionsLayout);

		checkShowLegend = new Button(optionsGroup, SWT.CHECK);
      checkShowLegend.setText(i18n.tr("Show &legend"));
		checkShowLegend.setSelection(config.isShowLegend());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		checkShowLegend.setLayoutData(gd);
		
      checkExtendedLegend = new Button(optionsGroup, SWT.CHECK);
      checkExtendedLegend.setText(i18n.tr("E&xtended legend"));
      checkExtendedLegend.setSelection(config.isExtendedLegend());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      checkExtendedLegend.setLayoutData(gd);
		
		checkTranslucent = new Button(optionsGroup, SWT.CHECK);
		checkTranslucent.setText(i18n.tr("T&ranslucent"));
		checkTranslucent.setSelection(config.isTranslucent());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		checkTranslucent.setLayoutData(gd);
		
      if (config instanceof TableBarChartConfig)
		{
			checkTransposed = new Button(optionsGroup, SWT.CHECK);
			checkTransposed.setText(i18n.tr("Trans&posed"));
         checkTransposed.setSelection(((TableBarChartConfig)config).isTransposed());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			checkTransposed.setLayoutData(gd);
		}

		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		refreshRate = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Refresh interval (seconds)"), 1, 10000, gd);
		refreshRate.setSelection(config.getRefreshRate());

      if (!(config instanceof TablePieChartConfig))
      {
         yAxisRange = new YAxisRangeEditor(dialogArea, SWT.NONE);
         gd = new GridData();
         gd.horizontalSpan = layout.numColumns;
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         yAxisRange.setLayoutData(gd);
         yAxisRange.setSelection(config.isAutoScale(), config.modifyYBase(), config.getMinYScaleValue(), config.getMaxYScaleValue(), config.getYAxisLabel());
      }

      drillDownObject = new ObjectSelector(dialogArea, SWT.NONE, true);
      drillDownObject.setLabel(i18n.tr("Drill-down object"));
      drillDownObject.setObjectClass(AbstractObject.class);
      drillDownObject.setClassFilter(ObjectSelectionDialog.createDashboardAndNetworkMapSelectionFilter());
      drillDownObject.setObjectId(config.getDrillDownObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      drillDownObject.setLayoutData(gd);
		
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
			case ChartConfiguration.POSITION_BOTTOM:
				return 3;
			case ChartConfiguration.POSITION_LEFT:
				return 0;
			case ChartConfiguration.POSITION_RIGHT:
				return 1;
			case ChartConfiguration.POSITION_TOP:
				return 2;
		}
		return 0;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
      title.updateConfiguration(config);
		config.setLegendPosition(1 << legendPosition.getSelectionIndex());
		config.setRefreshRate(refreshRate.getSelection());
		config.setShowLegend(checkShowLegend.getSelection());
      config.setExtendedLegend(checkExtendedLegend.getSelection());
		config.setTranslucent(checkTranslucent.getSelection());	
      config.setDrillDownObjectId(drillDownObject.getObjectId());

      if (!(config instanceof TablePieChartConfig))
      {
	      if (!yAxisRange.validate(true))
	         return false;

         config.setAutoScale(yAxisRange.isAuto());
         config.setMinYScaleValue(yAxisRange.getMinY());
         config.setMaxYScaleValue(yAxisRange.getMaxY());
         config.setModifyYBase(yAxisRange.modifyYBase());
         config.setYAxisLabel(yAxisRange.getLabel());
      }

		if (config instanceof TableBarChartConfig)
		{
			((TableBarChartConfig)config).setTransposed(checkTransposed.getSelection());
		}
		return true;
	}
}
