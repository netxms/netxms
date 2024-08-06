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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Scale;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.TimePeriodSelector;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.YAxisRangeEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page for chart
 */
public class GeneralChart extends PreferencePage
{
   private final I18n i18n = LocalizationHelper.getI18n(GeneralChart.class);

   private ChartConfiguration config;
	private LabeledText title;
	private Button checkShowGrid;
	private Button checkShowLegend;
	private Button checkShowHostNames;
	private Button checkAutoRefresh;
	private Button checkLogScale;
	private Button checkStacked;
	private Button checkExtendedLegend;
   private Button checkTranslucent;
   private Button checkAreaChart;
   private Button checkUseMultipliers;
   private LabeledSpinner lineWidth;
	private Combo legendLocation;
	private Scale refreshIntervalScale;
	private Spinner refreshIntervalSpinner;
	private TimePeriodSelector timeSelector;
	private YAxisRangeEditor yAxisRange;
   private boolean saveToDatabase;

   /**
    * Constructor
    * @param settings
    */
   public GeneralChart(ChartConfiguration settings, boolean saveToDatabase)
   {
      super("General");
      config = settings;     
      this.saveToDatabase = saveToDatabase;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      title = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER);
      title.setLabel(i18n.tr("Title"));
      title.setText(config.getTitle());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      title.setLayoutData(gd);
      
      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(i18n.tr("Options"));
      layout = new GridLayout();
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.makeColumnsEqualWidth = true;
      layout.numColumns = 3;
      optionsGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      optionsGroup.setLayoutData(gd);
      
      checkShowGrid = new Button(optionsGroup, SWT.CHECK);
      checkShowGrid.setText(i18n.tr("Show &grid lines"));
      checkShowGrid.setSelection(config.isGridVisible());

      checkLogScale = new Button(optionsGroup, SWT.CHECK);
      checkLogScale.setText(i18n.tr("L&ogarithmic scale"));
      checkLogScale.setSelection(config.isLogScale());

      lineWidth = new LabeledSpinner(optionsGroup, SWT.NONE);
      lineWidth.setLabel(i18n.tr("Line width"));
      lineWidth.setRange(1, 99);
      lineWidth.setSelection(config.getLineWidth());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.verticalSpan = 2;
      lineWidth.setLayoutData(gd);

      checkStacked = new Button(optionsGroup, SWT.CHECK);
      checkStacked.setText(i18n.tr("Stacked"));
      checkStacked.setSelection(config.isStacked());

      checkTranslucent= new Button(optionsGroup, SWT.CHECK);
      checkTranslucent.setText(i18n.tr("Translucent"));
      checkTranslucent.setSelection(config.isTranslucent());

      checkShowLegend = new Button(optionsGroup, SWT.CHECK);
      checkShowLegend.setText(i18n.tr("Show &legend"));
      checkShowLegend.setSelection(config.isLegendVisible());
      checkShowLegend.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            checkExtendedLegend.setEnabled(checkShowLegend.getSelection());
            legendLocation.setEnabled(checkShowLegend.getSelection());
            checkShowHostNames.setEnabled(checkShowLegend.getSelection());
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });

      checkShowHostNames = new Button(optionsGroup, SWT.CHECK);
      checkShowHostNames.setText(i18n.tr("Show &host names"));
      checkShowHostNames.setSelection(config.isShowHostNames());
      checkShowHostNames.setEnabled(config.isLegendVisible());

      gd = new GridData();
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalSpan = 2;
      gd.verticalAlignment = SWT.TOP;
      legendLocation = WidgetHelper.createLabeledCombo(optionsGroup, SWT.READ_ONLY, i18n.tr("Legend position"), gd);
      legendLocation.add(i18n.tr("Left"));
      legendLocation.add(i18n.tr("Right"));
      legendLocation.add(i18n.tr("Top"));
      legendLocation.add(i18n.tr("Bottom"));
      legendLocation.select(31 - Integer.numberOfLeadingZeros(config.getLegendPosition()));      
      legendLocation.setEnabled(config.isLegendVisible());
      
      checkExtendedLegend = new Button(optionsGroup, SWT.CHECK);
      checkExtendedLegend.setText(i18n.tr("Show extended legend"));
      checkExtendedLegend.setSelection(config.isExtendedLegend());         
      checkExtendedLegend.setEnabled(config.isLegendVisible());
      
      checkAreaChart = new Button(optionsGroup, SWT.CHECK);
      checkAreaChart.setText(i18n.tr("Area chart"));
      checkAreaChart.setSelection(config.isArea());         

      checkUseMultipliers = new Button(optionsGroup, SWT.CHECK);
      checkUseMultipliers.setText(i18n.tr("Use multipliers"));
      checkUseMultipliers.setSelection(config.isUseMultipliers());

      Composite refreshGroup = new Composite(optionsGroup, SWT.NONE);
      layout = new GridLayout();
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.marginTop = WidgetHelper.OUTER_SPACING;
      refreshGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      refreshGroup.setLayoutData(gd);

      checkAutoRefresh = new Button(refreshGroup, SWT.CHECK);
      checkAutoRefresh.setText(i18n.tr("&Refresh automatically"));
      checkAutoRefresh.setSelection(config.isAutoRefresh());
      checkAutoRefresh.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            refreshIntervalSpinner.setEnabled(checkAutoRefresh.getSelection());
            refreshIntervalScale.setEnabled(checkAutoRefresh.getSelection());
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      Composite refreshIntervalGroup = new Composite(refreshGroup, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.marginTop = WidgetHelper.OUTER_SPACING;
      refreshIntervalGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      refreshIntervalGroup.setLayoutData(gd);
      
      Label label = new Label(refreshIntervalGroup, SWT.NONE);
      label.setText(i18n.tr("Refresh interval:"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.horizontalSpan = 2;
      label.setLayoutData(gd);
      
      refreshIntervalScale = new Scale(refreshIntervalGroup, SWT.HORIZONTAL);
      refreshIntervalScale.setMinimum(1);
      refreshIntervalScale.setMaximum(600);
      refreshIntervalScale.setSelection(config.getRefreshRate());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      refreshIntervalScale.setLayoutData(gd);
      refreshIntervalScale.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				refreshIntervalSpinner.setSelection(refreshIntervalScale.getSelection());
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
      });   
      refreshIntervalScale.setEnabled(config.isAutoRefresh()); 
      
      refreshIntervalSpinner = new Spinner(refreshIntervalGroup, SWT.BORDER);
      refreshIntervalSpinner.setMinimum(1);
      refreshIntervalSpinner.setMaximum(600);
      refreshIntervalSpinner.setSelection(config.getRefreshRate());
      refreshIntervalSpinner.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				refreshIntervalScale.setSelection(refreshIntervalSpinner.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
      refreshIntervalSpinner.setEnabled(config.isAutoRefresh()); 

      Group timeSelectorGroup = new Group(dialogArea, SWT.NONE);
      timeSelectorGroup.setText(i18n.tr("Time Period"));
      layout = new GridLayout();
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      timeSelectorGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      timeSelectorGroup.setLayoutData(gd);

      timeSelector = new TimePeriodSelector(timeSelectorGroup, SWT.NONE, config.getTimePeriod());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      timeSelector.setLayoutData(gd);

      yAxisRange = new YAxisRangeEditor(dialogArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      yAxisRange.setLayoutData(gd);
      yAxisRange.setSelection(config.isAutoScale(), config.isModifyYBase(), config.getMinYScaleValue(), config.getMaxYScaleValue(), config.getYAxisLabel());

      return dialogArea;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		
		title.setText(""); //$NON-NLS-1$
		checkShowGrid.setSelection(true);
		checkShowLegend.setSelection(true);
		checkShowHostNames.setSelection(false);
		checkAutoRefresh.setSelection(true);
		checkLogScale.setSelection(false);
		checkStacked.setSelection(false);
		checkExtendedLegend.setSelection(false);
		checkAreaChart.setSelection(false);
		checkUseMultipliers.setSelection(true);
		legendLocation.select(3);
		lineWidth.setSelection(2);

      yAxisRange.setSelection(true, false, 0, 100, "");

		refreshIntervalScale.setSelection(30);
		refreshIntervalSpinner.setSelection(30);

		timeSelector.setDefaults();
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		config.setTitle(title.getText());
      config.setGridVisible(checkShowGrid.getSelection());
      config.setLegendVisible(checkShowLegend.getSelection());
		config.setAutoScale(yAxisRange.isAuto());
		config.setShowHostNames(checkShowHostNames.getSelection());
		config.setAutoRefresh(checkAutoRefresh.getSelection());
		config.setLogScale(checkLogScale.getSelection());
		config.setRefreshRate(refreshIntervalSpinner.getSelection());
      config.setStacked(checkStacked.getSelection());
      config.setExtendedLegend(checkExtendedLegend.getSelection());
      config.setArea(checkAreaChart.getSelection());
      config.setLegendPosition((int)Math.pow(2,legendLocation.getSelectionIndex()));
      config.setTranslucent(checkTranslucent.getSelection());
      config.setLineWidth(lineWidth.getSelection());
      config.setTimePeriod(timeSelector.getTimePeriod());
		config.setUseMultipliers(checkUseMultipliers.getSelection());
		config.setMinYScaleValue(yAxisRange.getMinY());
		config.setMaxYScaleValue(yAxisRange.getMaxY());
      config.setModifyYBase(yAxisRange.modifyYBase());
      config.setYAxisLabel(yAxisRange.getLabel());

		if (saveToDatabase && isApply)
		{
			setValid(false);
         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Update predefined graph"), null) {
				@Override
            protected void run(IProgressMonitor monitor) throws Exception
				{
               session.saveGraph((GraphDefinition)config, false);
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							GeneralChart.this.setValid(true);
						}
					});
				}

				@Override
				protected String getErrorMessage()
				{
               return i18n.tr("Cannot update predefined graph");
				}
			}.start();
		}
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{      
      if (isControlCreated() && yAxisRange.validate(true))
         applyChanges(true);
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
	   if (!isControlCreated())
	      return true;
	   
	   if (!yAxisRange.validate(true))
	      return false;
	   
		applyChanges(false);
		return true;
	}
}
