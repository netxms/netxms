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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.ObjectStatusChartConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Availability chart element properties
 */
public class ObjectStatusChart extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectStatusChart.class);

	private ObjectStatusChartConfig config;
	private ObjectSelector objectSelector;
   private TitleConfigurator title;
	private Spinner refreshRate;
	private Button checkShowLegend;
	private Button checkTransposed;
	private Button checkTranslucent;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public ObjectStatusChart(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(ObjectStatusChart.class).tr("Object Status Chart"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "object-status-chart";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof ObjectStatusChartConfig;
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
      config = (ObjectStatusChartConfig)elementConfig;

		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false, true);
      objectSelector.setLabel(i18n.tr("Root object"));
		objectSelector.setObjectClass(AbstractObject.class);
		objectSelector.setObjectId(config.getRootObject());
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

		checkTransposed = new Button(optionsGroup, SWT.CHECK);
      checkTransposed.setText(i18n.tr("Trans&posed"));
		checkTransposed.setSelection(config.isTransposed());

		checkTranslucent = new Button(optionsGroup, SWT.CHECK);
      checkTranslucent.setText(i18n.tr("&Translucent"));
		checkTranslucent.setSelection(config.isTranslucent());

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
		config.setRootObject(objectSelector.getObjectId());
		config.setShowLegend(checkShowLegend.getSelection());
		config.setTransposed(checkTransposed.getSelection());
		config.setTranslucent(checkTranslucent.getSelection());
		config.setRefreshRate(refreshRate.getSelection());
		return true;
	}
}
