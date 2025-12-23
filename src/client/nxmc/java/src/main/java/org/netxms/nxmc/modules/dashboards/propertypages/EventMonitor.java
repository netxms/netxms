/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.EventMonitorConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.xnap.commons.i18n.I18n;

/**
 * Event monitor element properties
 */
public class EventMonitor extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(EventMonitor.class);

   private EventMonitorConfig config;
   private ObjectSelector objectSelector;
   private LabeledText filterText;
   private LabeledSpinner maxEvents;
   private LabeledSpinner timeRangeMinutes;
   private TitleConfigurator title;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public EventMonitor(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(EventMonitor.class).tr("Event Monitor"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "event-monitor";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof EventMonitorConfig;
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
      config = (EventMonitorConfig)elementConfig;

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

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      objectSelector.setLabel(i18n.tr("Root object"));
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId(config.getObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);

      filterText = new LabeledText(dialogArea, SWT.NONE);
      filterText.setLabel(i18n.tr("Event filter"));
      filterText.setText(config.getFilter());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      filterText.setLayoutData(gd);
      filterText.setText(config.getFilter());

      maxEvents = new LabeledSpinner(dialogArea, SWT.NONE);
      maxEvents.setLabel(i18n.tr("Maximum events to load (0 to disable)"));
      maxEvents.setRange(0, 1000);
      maxEvents.setSelection(config.getMaxEvents());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      maxEvents.setLayoutData(gd);

      timeRangeMinutes = new LabeledSpinner(dialogArea, SWT.NONE);
      timeRangeMinutes.setLabel(i18n.tr("Time range (minutes, 0 for no limit)"));
      timeRangeMinutes.setRange(0, 10080);
      timeRangeMinutes.setSelection(config.getTimeRangeMinutes());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      timeRangeMinutes.setLayoutData(gd);

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
      config.setFilter(filterText.getText().trim());
      config.setMaxEvents(maxEvents.getSelection());
      config.setTimeRangeMinutes(timeRangeMinutes.getSelection());
      return true;
   }
}
