/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2023 RadenSolutions
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
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.objects.BusinessService;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.TitleConfigurator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AvailabilityChartConfig;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;

/**
 * "Availability Chart" property page for availability charts
 */
public class AvailabilityChart extends PropertyPage
{
   private AvailabilityChartConfig config;
   private TitleConfigurator title;
   private ObjectSelector objectSelector;
   private Combo periodSelector;
   private LabeledSpinner daysSelector;
   private Combo legendPosition;
   private Button checkShowLegend;
   private Button checkTranslucent;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      config = getElement().getAdapter(AvailabilityChartConfig.class);

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
      objectSelector.setLabel("Business service");
      objectSelector.setClassFilter(ObjectSelectionDialog.createBusinessServiceSelectionFilter());
      objectSelector.setObjectClass(BusinessService.class);
      objectSelector.setObjectId(config.getObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);

      periodSelector = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Period", WidgetHelper.DEFAULT_LAYOUT_DATA);
      periodSelector.add("Today");
      periodSelector.add("Yesterday");
      periodSelector.add("This week");
      periodSelector.add("Last week");
      periodSelector.add("This month");
      periodSelector.add("Last month");
      periodSelector.add("This year");
      periodSelector.add("Last year");
      periodSelector.add("Custom");
      periodSelector.select(config.getPeriod());
      periodSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            daysSelector.setVisible(periodSelector.getSelectionIndex() == AvailabilityChartConfig.CUSTOM);
         }
      });

      legendPosition = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, Messages.get().AbstractChart_LegendPosition, WidgetHelper.DEFAULT_LAYOUT_DATA);
      legendPosition.add(Messages.get().AbstractChart_Left);
      legendPosition.add(Messages.get().AbstractChart_Right);
      legendPosition.add(Messages.get().AbstractChart_Top);
      legendPosition.add(Messages.get().AbstractChart_Bottom);
      legendPosition.select(positionIndexFromValue(config.getLegendPosition()));

      daysSelector = new LabeledSpinner(dialogArea, SWT.NONE);
      daysSelector.setLabel("Period length (days)");
      daysSelector.setRange(1, 1000);
      daysSelector.setSelection(config.getNumberOfDays());
      daysSelector.setVisible(periodSelector.getSelectionIndex() == AvailabilityChartConfig.CUSTOM);

      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(Messages.get().AbstractChart_Options);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      optionsGroup.setLayoutData(gd);
      GridLayout optionsLayout = new GridLayout();
      optionsGroup.setLayout(optionsLayout);

      checkShowLegend = new Button(optionsGroup, SWT.CHECK);
      checkShowLegend.setText(Messages.get().AbstractChart_ShowLegend);
      checkShowLegend.setSelection(config.isShowLegend());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      checkShowLegend.setLayoutData(gd);

      checkTranslucent = new Button(optionsGroup, SWT.CHECK);
      checkTranslucent.setText(Messages.get().AbstractChart_Translucent);
      checkTranslucent.setSelection(config.isTranslucent());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      checkTranslucent.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
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
}
