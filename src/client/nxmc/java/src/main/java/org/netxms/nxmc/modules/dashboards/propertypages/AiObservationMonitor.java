/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2026 Raden Solutions
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.AiObservationMonitorConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.xnap.commons.i18n.I18n;

/**
 * "AI Observation Monitor" property page
 */
public class AiObservationMonitor extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(AiObservationMonitor.class);

   private AiObservationMonitorConfig config;
   private TitleConfigurator title;
   private ObjectSelector objectSelector;
   private LabeledSpinner spinnerInstanceId;
   private LabeledSpinner spinnerMaxRecords;
   private LabeledSpinner spinnerRefreshInterval;
   private Button checkNewOnly;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public AiObservationMonitor(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(AiObservationMonitor.class).tr("AI Observation Monitor"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "ai-observation-monitor";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof AiObservationMonitorConfig;
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
      config = (AiObservationMonitorConfig)elementConfig;

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      title.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      objectSelector.setLabel(i18n.tr("Root object"));
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId(config.getObjectId());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);

      spinnerInstanceId = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerInstanceId.setLabel(i18n.tr("Operator instance ID (0 = all)"));
      spinnerInstanceId.setRange(0, 1000000);
      spinnerInstanceId.setSelection(config.getInstanceId());
      spinnerInstanceId.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      spinnerMaxRecords = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerMaxRecords.setLabel(i18n.tr("Records to show"));
      spinnerMaxRecords.setRange(1, 500);
      spinnerMaxRecords.setSelection(config.getMaxRecords());
      spinnerMaxRecords.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      spinnerRefreshInterval = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerRefreshInterval.setLabel(i18n.tr("Refresh interval (seconds)"));
      spinnerRefreshInterval.setRange(5, 3600);
      spinnerRefreshInterval.setSelection(config.getRefreshInterval());
      spinnerRefreshInterval.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      checkNewOnly = new Button(dialogArea, SWT.CHECK);
      checkNewOnly.setText(i18n.tr("Show only &new observations"));
      checkNewOnly.setSelection(config.isNewOnly());
      checkNewOnly.setLayoutData(new GridData(SWT.LEFT, SWT.BOTTOM, true, false));

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      title.updateConfiguration(config);
      config.setObjectId(objectSelector.getObjectId());
      config.setInstanceId(spinnerInstanceId.getSelection());
      config.setMaxRecords(spinnerMaxRecords.getSelection());
      config.setRefreshInterval(spinnerRefreshInterval.getSelection());
      config.setNewOnly(checkNewOnly.getSelection());
      return true;
   }
}
