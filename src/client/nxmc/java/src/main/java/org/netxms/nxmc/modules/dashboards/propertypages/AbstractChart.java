/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.AbstractChartConfig;
import org.netxms.nxmc.modules.dashboards.config.BarChartConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.GaugeConfig;
import org.netxms.nxmc.modules.dashboards.config.LineChartConfig;
import org.netxms.nxmc.modules.dashboards.config.PieChartConfig;
import org.netxms.nxmc.modules.dashboards.config.ScriptedBarChartConfig;
import org.netxms.nxmc.modules.dashboards.config.ScriptedPieChartConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.datacollection.widgets.YAxisRangeEditor;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Generic chart properties
 */
public class AbstractChart extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(AbstractChart.class);

	private AbstractChartConfig config;
   private TitleConfigurator title;
	private Spinner timeRange;
	private Combo timeUnits;
	private LabeledSpinner refreshRate;
	private Combo legendPosition;
	private Button checkShowLegend;
   private Button checkExtendedLegend;
	private Button checkShowGrid;
	private Button checkTranslucent;
	private Button checkTransposed;
   private Button checkDoughnutRendering;
   private Button checkShowTotal;
   private Button checkLogScale;
   private Button checkUseMultipliers;
   private Button checkStacked;
   private Button checkAreaChart;
   private Button checkInteractive;
   private LabeledSpinner lineWidth;
   private YAxisRangeEditor yAxisRange;   
   private ObjectSelector drillDownObject;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public AbstractChart(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(AbstractChart.class).tr("Chart"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "chart";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof AbstractChartConfig;
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
      config = (AbstractChartConfig)elementConfig;
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = false;
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
		gd.verticalSpan = (config instanceof LineChartConfig) ? 3 : 2;
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

      if (!(config instanceof GaugeConfig))
      {
         checkExtendedLegend = new Button(optionsGroup, SWT.CHECK);
         checkExtendedLegend.setText(i18n.tr("E&xtended legend"));
         checkExtendedLegend.setSelection(config.isExtendedLegend());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         checkExtendedLegend.setLayoutData(gd);
      }

      if (config instanceof LineChartConfig)
      {
         checkUseMultipliers = new Button(optionsGroup, SWT.CHECK);
         checkUseMultipliers.setText(i18n.tr("Use &multipliers"));
         checkUseMultipliers.setSelection(((LineChartConfig)config).isUseMultipliers());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         checkUseMultipliers.setLayoutData(gd);

         checkLogScale = new Button(optionsGroup, SWT.CHECK);
         checkLogScale.setText(i18n.tr("L&ogarithmic scale"));
         checkLogScale.setSelection(((LineChartConfig)config).isLogScaleEnabled());
         gd = new GridData();
         gd.horizontalSpan = layout.numColumns;
         checkLogScale.setLayoutData(gd);
         
         checkStacked = new Button(optionsGroup, SWT.CHECK);
         checkStacked.setText(i18n.tr("&Stacked"));
         checkStacked.setSelection(((LineChartConfig)config).isStacked());
         gd = new GridData();
         gd.horizontalSpan = layout.numColumns;
         checkStacked.setLayoutData(gd);
         
         checkAreaChart = new Button(optionsGroup, SWT.CHECK);
         checkAreaChart.setText(i18n.tr("&Area chart"));
         checkAreaChart.setSelection(((LineChartConfig)config).isArea());
         gd = new GridData();
         gd.horizontalSpan = layout.numColumns;
         checkAreaChart.setLayoutData(gd);
      }
      
      checkTranslucent = new Button(optionsGroup, SWT.CHECK);
      checkTranslucent.setText(i18n.tr("T&ranslucent"));
      checkTranslucent.setSelection(config.isTranslucent());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      checkTranslucent.setLayoutData(gd);

      if ((config instanceof BarChartConfig) || (config instanceof ScriptedBarChartConfig))
		{
			checkTransposed = new Button(optionsGroup, SWT.CHECK);
         checkTransposed.setText(i18n.tr("Trans&posed"));
         checkTransposed.setSelection((config instanceof BarChartConfig) ? ((BarChartConfig)config).isTransposed() : ((ScriptedBarChartConfig)config).isTransposed());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			checkTransposed.setLayoutData(gd);
		}

      if ((config instanceof PieChartConfig) || (config instanceof ScriptedPieChartConfig))
      {
         checkDoughnutRendering = new Button(optionsGroup, SWT.CHECK);
         checkDoughnutRendering.setText(i18n.tr("&Doughnut rendering"));
         checkDoughnutRendering.setSelection((config instanceof PieChartConfig) ? ((PieChartConfig)config).isDoughnutRendering() : ((ScriptedPieChartConfig)config).isDoughnutRendering());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         checkDoughnutRendering.setLayoutData(gd);

         checkShowTotal = new Button(optionsGroup, SWT.CHECK);
         checkShowTotal.setText(i18n.tr("Show &total"));
         checkShowTotal.setSelection((config instanceof PieChartConfig) ? ((PieChartConfig)config).isShowTotal() : ((ScriptedPieChartConfig)config).isShowTotal());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         checkShowTotal.setLayoutData(gd);
      }

		if (config instanceof LineChartConfig)
		{
			checkShowGrid = new Button(optionsGroup, SWT.CHECK);
         checkShowGrid.setText(i18n.tr("Show &grid"));
			checkShowGrid.setSelection(((LineChartConfig)config).isShowGrid());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			checkShowGrid.setLayoutData(gd);

         checkInteractive = new Button(optionsGroup, SWT.CHECK);
         checkInteractive.setText(i18n.tr("&Interactive"));
         checkInteractive.setSelection(((LineChartConfig)config).isInteractive());
         gd = new GridData();
         gd.horizontalSpan = layout.numColumns;
         checkInteractive.setLayoutData(gd);

			Composite timeRangeArea = new Composite(dialogArea, SWT.NONE);
			layout = new GridLayout();
			layout.numColumns = 2;
			layout.marginWidth = 0;
			layout.marginHeight = 0;
			layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
			timeRangeArea.setLayout(layout);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			timeRangeArea.setLayoutData(gd);

         timeRange = WidgetHelper.createLabeledSpinner(timeRangeArea, SWT.BORDER, i18n.tr("Time interval"), 1, 10000, WidgetHelper.DEFAULT_LAYOUT_DATA);
			timeRange.setSelection(((LineChartConfig)config).getTimeRange());

         timeUnits = WidgetHelper.createLabeledCombo(timeRangeArea, SWT.READ_ONLY, i18n.tr("Time units"), WidgetHelper.DEFAULT_LAYOUT_DATA);
         timeUnits.add(i18n.tr("Minutes"));
         timeUnits.add(i18n.tr("Hours"));
         timeUnits.add(i18n.tr("Days"));
			timeUnits.select(((LineChartConfig)config).getTimeUnits());
		}

		Composite rateAndWidthArea = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.numColumns = (config instanceof LineChartConfig) ? 2 : 1;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.makeColumnsEqualWidth = true;
		rateAndWidthArea.setLayout(layout);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      rateAndWidthArea.setLayoutData(gd);

		refreshRate = new LabeledSpinner(rateAndWidthArea, SWT.NONE);
      refreshRate.setLabel(i18n.tr("Refresh interval (seconds)"));
		refreshRate.setRange(1, 10000);
      refreshRate.setSelection(config.getRefreshRate());
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		refreshRate.setLayoutData(gd);

		if (config instanceof LineChartConfig)
		{
         lineWidth = new LabeledSpinner(rateAndWidthArea, SWT.NONE);
         lineWidth.setLabel(i18n.tr("Line width"));
         lineWidth.setRange(1, 32);
         lineWidth.setSelection(((LineChartConfig)config).getLineWidth());
         gd = new GridData();
         gd.verticalAlignment = SWT.TOP;
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         lineWidth.setLayoutData(gd);
		}

      if (!(config instanceof PieChartConfig) && !(config instanceof ScriptedPieChartConfig))
      {
	      yAxisRange = new YAxisRangeEditor(dialogArea, SWT.NONE);
	      gd = new GridData();
	      gd.horizontalSpan = layout.numColumns;
	      gd.horizontalAlignment = SWT.FILL;
	      gd.grabExcessHorizontalSpace = true;
	      yAxisRange.setLayoutData(gd);
         yAxisRange.setSelection(config.isAutoScale(), config.modifyYBase(), config.getMinYScaleValue(), config.getMaxYScaleValue(), config.getYAxisLabel());
      }

      if (!(config instanceof LineChartConfig))
      {
         drillDownObject = new ObjectSelector(dialogArea, SWT.NONE, true);
         drillDownObject.setLabel("Drill-down object");
         drillDownObject.setObjectClass(AbstractObject.class);
         drillDownObject.setClassFilter(ObjectSelectionDialog.createDashboardAndNetworkMapSelectionFilter());
         drillDownObject.setObjectId(config.getDrillDownObjectId());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         drillDownObject.setLayoutData(gd);
      }

		return dialogArea;
	}

	/**
	 * @param value
	 * @return
	 */
   private static int positionIndexFromValue(int value)
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
		config.setShowLegend(checkShowLegend.getSelection());
		config.setRefreshRate(refreshRate.getSelection());
      config.setTranslucent(checkTranslucent.getSelection());

      if (!(config instanceof GaugeConfig))
      {
         config.setExtendedLegend(checkExtendedLegend.getSelection());
      }

      if (config instanceof PieChartConfig)
      {
         ((PieChartConfig)config).setDoughnutRendering(checkDoughnutRendering.getSelection());
         ((PieChartConfig)config).setShowTotal(checkShowTotal.getSelection());
      }
      else if (config instanceof ScriptedPieChartConfig)
      {
         ((ScriptedPieChartConfig)config).setDoughnutRendering(checkDoughnutRendering.getSelection());
         ((ScriptedPieChartConfig)config).setShowTotal(checkShowTotal.getSelection());
      }
      else
      {
	      if (!yAxisRange.validate(true))
	         return false;

   		config.setAutoScale(yAxisRange.isAuto());
   		config.setMinYScaleValue(yAxisRange.getMinY());
         config.setMaxYScaleValue(yAxisRange.getMaxY());
         config.setModifyYBase(yAxisRange.modifyYBase());
         config.setYAxisLabel(yAxisRange.getLabel());
      }

		if (config instanceof BarChartConfig)
		{
			((BarChartConfig)config).setTransposed(checkTransposed.getSelection());
		}
      else if (config instanceof ScriptedBarChartConfig)
      {
         ((ScriptedBarChartConfig)config).setTransposed(checkTransposed.getSelection());
      }

		if (config instanceof LineChartConfig)
		{
			((LineChartConfig)config).setTimeRange(timeRange.getSelection());
			((LineChartConfig)config).setTimeUnits(timeUnits.getSelectionIndex());
			((LineChartConfig)config).setShowGrid(checkShowGrid.getSelection());
         ((LineChartConfig)config).setLogScaleEnabled(checkLogScale.getSelection());
         ((LineChartConfig)config).setUseMultipliers(checkUseMultipliers.getSelection());
         ((LineChartConfig)config).setStacked(checkStacked.getSelection());
         ((LineChartConfig)config).setArea(checkAreaChart.getSelection());
         ((LineChartConfig)config).setInteractive(checkInteractive.getSelection());
         ((LineChartConfig)config).setLineWidth(lineWidth.getSelection());
		}
      else
      {
         config.setDrillDownObjectId(drillDownObject.getObjectId());
      }
		return true;
	}
}
