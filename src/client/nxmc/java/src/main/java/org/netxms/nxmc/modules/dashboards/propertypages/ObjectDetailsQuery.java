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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.ObjectDetailsConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Properties for "Object query" dashboard element ("Query" page)
 */
public class ObjectDetailsQuery extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectDetailsQuery.class);

   private ObjectDetailsConfig config;
   private TitleConfigurator title;
   private ScriptEditor query;
   private ObjectSelector rootObject;
   private LabeledText orderingProperties;
   private LabeledSpinner refreshInterval;
   private LabeledSpinner recordLimit;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public ObjectDetailsQuery(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(ObjectDetailsQuery.class).tr("Query"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "query";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof ObjectDetailsConfig;
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
      config = (ObjectDetailsConfig)elementConfig;

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      title.setLayoutData(gd);

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Query"));

      query = new ScriptEditor(dialogArea, SWT.BORDER, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      query.setText(config.getQuery());

      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.widthHint = 300;
      gd.heightHint = 300;
      gd.horizontalSpan = 2;
      query.setLayoutData(gd);

      rootObject = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      rootObject.setLabel(i18n.tr("Root object"));
      rootObject.setObjectClass(AbstractObject.class);
      rootObject.setObjectId(config.getRootObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      rootObject.setLayoutData(gd);

      orderingProperties = new LabeledText(dialogArea, SWT.NONE);
      orderingProperties.setLabel(i18n.tr("Order by"));
      orderingProperties.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));
      orderingProperties.setText(config.getOrderingPropertiesAsText());

      refreshInterval = new LabeledSpinner(dialogArea, SWT.NONE);
      refreshInterval.setLabel(i18n.tr("Refresh interval (seconds)"));
      refreshInterval.setRange(1, 100000);
      refreshInterval.setSelection(config.getRefreshRate());
      refreshInterval.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, false, false));
      
      recordLimit = new LabeledSpinner(dialogArea, SWT.NONE);
      recordLimit.setLabel(i18n.tr("Record limit (0 to disable)"));
      recordLimit.setRange(0, 10000);
      recordLimit.setSelection(config.getRecordLimit());
      recordLimit.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, false, false));
      
      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      title.updateConfiguration(config);
      config.setQuery(query.getText());
      config.setRootObjectId(rootObject.getObjectId());
      config.setRefreshRate(refreshInterval.getSelection());
      config.setOrderingProperties(orderingProperties.getText().trim());
      config.setRecordLimit(recordLimit.getSelection());
      return true;
   }
}
