/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AbstractChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.BarChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ComparisonChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LineChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.PieChartConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TubeChartConfig;
import org.netxms.ui.eclipse.perfview.widgets.YAxisRangeEditor;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Generic chart properties
 */
public class AbstractChart extends PropertyPage
{
	private AbstractChartConfig config;
	private LabeledText title;
	private Spinner timeRange;
	private Combo timeUnits;
	private LabeledSpinner refreshRate;
	private Combo legendPosition;
	private Button checkShowTitle;
	private Button checkShowLegend;
   private Button checkExtendedLegend;
	private Button checkShowGrid;
	private Button checkShowIn3D;
	private Button checkTranslucent;
	private Button checkTransposed;
   private Button checkLogScale;
   private Button checkStacked;
   private LabeledSpinner lineWidth;
   private YAxisRangeEditor yAxisRange;   

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
		layout.makeColumnsEqualWidth = false;
		dialogArea.setLayout(layout);
		
		title = new LabeledText(dialogArea, SWT.NONE);
		title.setLabel(Messages.get().AbstractChart_Title);
		title.setText(config.getTitle());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		title.setLayoutData(gd);
		
		legendPosition = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, Messages.get().AbstractChart_LegendPosition, WidgetHelper.DEFAULT_LAYOUT_DATA);
		legendPosition.add(Messages.get().AbstractChart_Left);
		legendPosition.add(Messages.get().AbstractChart_Right);
		legendPosition.add(Messages.get().AbstractChart_Top);
		legendPosition.add(Messages.get().AbstractChart_Bottom);
		legendPosition.select(positionIndexFromValue(config.getLegendPosition()));
		
		Group optionsGroup = new Group(dialogArea, SWT.NONE);
		optionsGroup.setText(Messages.get().AbstractChart_Options);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalSpan = (config instanceof LineChartConfig) ? 3 : 2;
		optionsGroup.setLayoutData(gd);
		GridLayout optionsLayout = new GridLayout();
		optionsGroup.setLayout(optionsLayout);

		checkShowTitle = new Button(optionsGroup, SWT.CHECK);
		checkShowTitle.setText(Messages.get().AbstractChart_ShowTitle);
		checkShowTitle.setSelection(config.isShowTitle());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		checkShowTitle.setLayoutData(gd);
		
		checkShowLegend = new Button(optionsGroup, SWT.CHECK);
		checkShowLegend.setText(Messages.get().AbstractChart_ShowLegend);
		checkShowLegend.setSelection(config.isShowLegend());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		checkShowLegend.setLayoutData(gd);

      if (config instanceof LineChartConfig)
      {
         checkExtendedLegend = new Button(optionsGroup, SWT.CHECK);
         checkExtendedLegend.setText(Messages.get().AbstractChart_ExtendedLegend);
         checkExtendedLegend.setSelection(((LineChartConfig)config).isExtendedLegend());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         checkExtendedLegend.setLayoutData(gd);
         
         checkLogScale = new Button(optionsGroup, SWT.CHECK);
         checkLogScale.setText(Messages.get().AbstractChart_LogartithmicScale);
         checkLogScale.setSelection(((LineChartConfig)config).isLogScaleEnabled());
         gd = new GridData();
         gd.horizontalSpan = layout.numColumns;
         checkLogScale.setLayoutData(gd);
         
         checkStacked = new Button(optionsGroup, SWT.CHECK);
         checkStacked.setText("&Stacked");
         checkStacked.setSelection(((LineChartConfig)config).isStacked());
         gd = new GridData();
         gd.horizontalSpan = layout.numColumns;
         checkStacked.setLayoutData(gd);
      }
      
		if (config instanceof ComparisonChartConfig)
		{
			checkShowIn3D = new Button(optionsGroup, SWT.CHECK);
			checkShowIn3D.setText(Messages.get().AbstractChart_3DView);
			checkShowIn3D.setSelection(((ComparisonChartConfig)config).isShowIn3D());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			checkShowIn3D.setLayoutData(gd);
			
			checkTranslucent = new Button(optionsGroup, SWT.CHECK);
			checkTranslucent.setText(Messages.get().AbstractChart_Translucent);
			checkTranslucent.setSelection(((ComparisonChartConfig)config).isTranslucent());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			checkTranslucent.setLayoutData(gd);

			if ((config instanceof BarChartConfig) || (config instanceof TubeChartConfig))
			{
				checkTransposed = new Button(optionsGroup, SWT.CHECK);
				checkTransposed.setText(Messages.get().AbstractChart_Transposed);
				checkTransposed.setSelection((config instanceof BarChartConfig) ? ((BarChartConfig)config).isTransposed() : ((TubeChartConfig)config).isTransposed());
				gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				checkTransposed.setLayoutData(gd);
			}
		}
		
		if (config instanceof LineChartConfig)
		{
			checkShowGrid = new Button(optionsGroup, SWT.CHECK);
			checkShowGrid.setText(Messages.get().AbstractChart_ShowGrid);
			checkShowGrid.setSelection(((LineChartConfig)config).isShowGrid());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			checkShowGrid.setLayoutData(gd);
			
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
			
			timeRange = WidgetHelper.createLabeledSpinner(timeRangeArea, SWT.BORDER, Messages.get().AbstractChart_TimeInterval, 1, 10000, WidgetHelper.DEFAULT_LAYOUT_DATA);
			timeRange.setSelection(((LineChartConfig)config).getTimeRange());
			
			timeUnits = WidgetHelper.createLabeledCombo(timeRangeArea, SWT.READ_ONLY, Messages.get().AbstractChart_TimeUnits, WidgetHelper.DEFAULT_LAYOUT_DATA);
			timeUnits.add(Messages.get().AbstractChart_Minutes);
			timeUnits.add(Messages.get().AbstractChart_Hours);
			timeUnits.add(Messages.get().AbstractChart_Days);
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
		refreshRate.setLabel(Messages.get().AbstractChart_RefreshInterval);
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
         lineWidth.setLabel("Line width");
         lineWidth.setRange(1, 32);
         lineWidth.setSelection(((LineChartConfig)config).getLineWidth());
         gd = new GridData();
         gd.verticalAlignment = SWT.TOP;
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         lineWidth.setLayoutData(gd);
		}
		
		if (!(config instanceof PieChartConfig))
      {
	      yAxisRange = new YAxisRangeEditor(dialogArea, SWT.NONE);
	      gd = new GridData();
	      gd.horizontalSpan = layout.numColumns;
	      gd.horizontalAlignment = SWT.FILL;
	      gd.grabExcessHorizontalSpace = true;
	      yAxisRange.setLayoutData(gd);
	      yAxisRange.setSelection(config.isAutoScale(), config.getMinYScaleValue(), config.getMaxYScaleValue());
      }
		
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
		if(!(config instanceof PieChartConfig))
      {
   		config.setAutoScale(yAxisRange.isAuto());
   		config.setMinYScaleValue(yAxisRange.getMinY());
         config.setMaxYScaleValue(yAxisRange.getMaxY());
      }
		if (config instanceof ComparisonChartConfig)
		{
			((ComparisonChartConfig)config).setShowIn3D(checkShowIn3D.getSelection());
			((ComparisonChartConfig)config).setTranslucent(checkTranslucent.getSelection());
			if (config instanceof BarChartConfig)
			{
				((BarChartConfig)config).setTransposed(checkTransposed.getSelection());
			}
			else if (config instanceof TubeChartConfig)
			{
				((TubeChartConfig)config).setTransposed(checkTransposed.getSelection());
			}
		}
		if (config instanceof LineChartConfig)
		{
			((LineChartConfig)config).setTimeRange(timeRange.getSelection());
			((LineChartConfig)config).setTimeUnits(timeUnits.getSelectionIndex());
			((LineChartConfig)config).setShowGrid(checkShowGrid.getSelection());
         ((LineChartConfig)config).setExtendedLegend(checkExtendedLegend.getSelection());
         ((LineChartConfig)config).setLogScaleEnabled(checkLogScale.getSelection());
         ((LineChartConfig)config).setStacked(checkStacked.getSelection());
         ((LineChartConfig)config).setLineWidth(lineWidth.getSelection());
		}
		return true;
	}
}
