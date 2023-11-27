/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.RackDiagramConfig;
import org.netxms.nxmc.modules.dashboards.config.RackDisplayMode;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Rack diagram element properties
 */
public class RackDiagram extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RackDiagram.class);
   private static final String[] RACK_DISPLAY_MODES = 
         { 
            LocalizationHelper.getI18n(RackDiagram.class).tr("Full"), 
            LocalizationHelper.getI18n(RackDiagram.class).tr("Front"), 
            LocalizationHelper.getI18n(RackDiagram.class).tr("Back") 
         };

   private RackDiagramConfig config;
   private ObjectSelector objectSelector;
   private TitleConfigurator title;
   private Combo displayMode;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public RackDiagram(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(RackDiagram.class).tr("Alarm Viewer"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "alarm-viewer";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof RackDiagramConfig;
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
      config = (RackDiagramConfig)elementConfig;

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false, true);
      objectSelector.setLabel(i18n.tr("Rack"));
      objectSelector.setClassFilter(ObjectSelectionDialog.createRackSelectionFilter());
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId(config.getObjectId());
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

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      displayMode = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Display mode"), gd);
      displayMode.setItems(RACK_DISPLAY_MODES); 
      displayMode.setText(RACK_DISPLAY_MODES[config.getDisplayMode().getValue()]);

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
      config.setDisplayMode(RackDisplayMode.getByValue(displayMode.getSelectionIndex()));
      return true;
   }
}
