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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.TableValueConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.datacollection.widgets.DciSelector;
import org.netxms.nxmc.modules.datacollection.widgets.TemplateDciSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Table last value element properties
 */
public class TableValue extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(TableValue.class);

	private TableValueConfig config;
	private DciSelector dciSelector;
   private TemplateDciSelector dciName;
   private TemplateDciSelector dciDescription;
   private TemplateDciSelector dciTag;
   private TitleConfigurator title;
	private Spinner refreshRate;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public TableValue(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(TableValue.class).tr("Table Value"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "table-value";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof TableValueConfig;
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
      config = (TableValueConfig)elementConfig;

		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);

      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      title.setLayoutData(gd);

      dciSelector = new DciSelector(dialogArea, SWT.NONE, true);
      dciSelector.setLabel(i18n.tr("Table DCI"));
		dciSelector.setDcObjectType(DataCollectionObject.DCO_TYPE_TABLE);
		dciSelector.setDciId(config.getObjectId(), config.getDciId());
      gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		dciSelector.setLayoutData(gd);
      dciSelector.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            boolean isTemplate = (dciSelector.getNodeId() == AbstractObject.CONTEXT);
            dciName.setEnabled(isTemplate);
            dciDescription.setEnabled(isTemplate);
            dciTag.setEnabled(isTemplate);
         }
      });

      dciName = new TemplateDciSelector(dialogArea, SWT.NONE);
      dciName.setLabel(i18n.tr("DCI Name"));
      dciName.setText(config.getDciName());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      dciName.setLayoutData(gd);
      dciName.setEnabled(config.getObjectId() == AbstractObject.CONTEXT);

      dciDescription = new TemplateDciSelector(dialogArea, SWT.NONE);
      dciDescription.setLabel(i18n.tr("DCI Description"));
      dciDescription.setText(config.getDciDescription());
      dciDescription.setField(TemplateDciSelector.Field.DESCRIPTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      dciDescription.setLayoutData(gd);
      dciDescription.setEnabled(config.getObjectId() == AbstractObject.CONTEXT);

      dciTag = new TemplateDciSelector(dialogArea, SWT.NONE);
      dciTag.setLabel(i18n.tr("DCI Tag"));
      dciTag.setText(config.getDciDescription());
      dciTag.setField(TemplateDciSelector.Field.DESCRIPTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      dciTag.setLayoutData(gd);
      dciTag.setEnabled(config.getObjectId() == AbstractObject.CONTEXT);

		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      refreshRate = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Refresh interval (seconds)"), 1, 10000, gd);
		refreshRate.setSelection(config.getRefreshRate());

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
      title.updateConfiguration(config);
		config.setObjectId(dciSelector.getNodeId());
		config.setDciId(dciSelector.getDciId());
      config.setDciName(dciName.getText());
      config.setDciDescription(dciDescription.getText());
      config.setDciTag(dciTag.getText());
		config.setRefreshRate(refreshRate.getSelection());
		return true;
	}
}
