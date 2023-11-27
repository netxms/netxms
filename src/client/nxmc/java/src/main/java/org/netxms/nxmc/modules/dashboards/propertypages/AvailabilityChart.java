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
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.objects.BusinessService;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.AvailabilityChartConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Availability chart element properties
 */
public class AvailabilityChart extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(AvailabilityChart.class);

   private AvailabilityChartConfig config;
   private TitleConfigurator title;
   private ObjectSelector objectSelector;
   private Combo periodSelector;
   private LabeledSpinner daysSelector;
   private Combo legendPosition;
   private Button checkShowLegend;
   private Button checkTranslucent;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public AvailabilityChart(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(AvailabilityChart.class).tr("Availability Chart"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "availability-chart";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof AvailabilityChartConfig;
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
      config = (AvailabilityChartConfig)elementConfig;

		Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      title.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      objectSelector.setLabel(i18n.tr("Business service"));
      objectSelector.setClassFilter(ObjectSelectionDialog.createBusinessServiceSelectionFilter());
      objectSelector.setObjectClass(BusinessService.class);
      objectSelector.setObjectId(config.getObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);

      periodSelector = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Period"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      periodSelector.add(i18n.tr("Today"));
      periodSelector.add(i18n.tr("Yesterday"));
      periodSelector.add(i18n.tr("This week"));
      periodSelector.add(i18n.tr("Last week"));
      periodSelector.add(i18n.tr("This month"));
      periodSelector.add(i18n.tr("Last month"));
      periodSelector.add(i18n.tr("This year"));
      periodSelector.add(i18n.tr("Last year"));
      periodSelector.add(i18n.tr("Custom"));
      periodSelector.select(config.getPeriod());
      periodSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            daysSelector.setVisible(periodSelector.getSelectionIndex() == AvailabilityChartConfig.CUSTOM);
         }
      });

      legendPosition = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Legend position"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      legendPosition.add(i18n.tr("Left"));
      legendPosition.add(i18n.tr("Right"));
      legendPosition.add(i18n.tr("Top"));
      legendPosition.add(i18n.tr("Bottom"));
      legendPosition.select(positionIndexFromValue(config.getLegendPosition()));

      daysSelector = new LabeledSpinner(dialogArea, SWT.NONE);
      daysSelector.setLabel("Period length (days)");
      daysSelector.setRange(1, 1000);
      daysSelector.setSelection(config.getNumberOfDays());
      daysSelector.setVisible(periodSelector.getSelectionIndex() == AvailabilityChartConfig.CUSTOM);

      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(i18n.tr("Options"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
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

      checkTranslucent = new Button(optionsGroup, SWT.CHECK);
      checkTranslucent.setText(i18n.tr("T&ranslucent"));
      checkTranslucent.setSelection(config.isTranslucent());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      checkTranslucent.setLayoutData(gd);

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
      config.setObjectId(objectSelector.getObjectId());
      config.setPeriod(periodSelector.getSelectionIndex());
      config.setNumberOfDays(daysSelector.getSelection());
      config.setLegendPosition(1 << legendPosition.getSelectionIndex());
      config.setShowLegend(checkShowLegend.getSelection());
      config.setTranslucent(checkTranslucent.getSelection());
		return true;
	}
}
