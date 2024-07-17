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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Scale;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.NetworkMapConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.xnap.commons.i18n.I18n;

/**
 * network map element properties
 */
public class NetworkMap extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(NetworkMap.class);

	private NetworkMapConfig config;
	private ObjectSelector objectSelector;
   private TitleConfigurator title;
	private Scale zoomLevelScale;
	private Spinner zoomLevelSpinner;
	private Button enableObjectDoubleClick;
	private Button enableHideLinkLabels;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public NetworkMap(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(NetworkMap.class).tr("Network Map"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "network-map";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof NetworkMapConfig;
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
      config = (NetworkMapConfig)elementConfig;

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

		objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
      objectSelector.setLabel(i18n.tr("Network map"));
		objectSelector.setObjectClass(org.netxms.client.objects.NetworkMap.class);
		objectSelector.setObjectId(config.getObjectId());
      gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		objectSelector.setLayoutData(gd);
		
		Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Zoom level (%)"));
		gd = new GridData();
		gd.horizontalSpan = 2;
		label.setLayoutData(gd);

		zoomLevelScale = new Scale(dialogArea, SWT.HORIZONTAL);
		zoomLevelScale.setMinimum(10);
		zoomLevelScale.setMaximum(400);
		zoomLevelScale.setSelection(config.getZoomLevel());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		zoomLevelScale.setLayoutData(gd);
      zoomLevelScale.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				zoomLevelSpinner.setSelection(zoomLevelScale.getSelection());
			}
		});
		
		zoomLevelSpinner = new Spinner(dialogArea, SWT.BORDER);
		zoomLevelSpinner.setMinimum(10);
		zoomLevelSpinner.setMaximum(400);
		zoomLevelSpinner.setSelection(config.getZoomLevel());
      zoomLevelSpinner.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				zoomLevelScale.setSelection(zoomLevelSpinner.getSelection());
			}
		});
		
		enableObjectDoubleClick = new Button(dialogArea, SWT.CHECK);
      enableObjectDoubleClick.setText(i18n.tr("Enable double click &action on objects"));
		enableObjectDoubleClick.setSelection(config.isObjectDoubleClickEnabled());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      enableObjectDoubleClick.setLayoutData(gd);
      
      enableHideLinkLabels = new Button(dialogArea, SWT.CHECK);
      enableHideLinkLabels.setText(i18n.tr("Hide link labels"));
      enableHideLinkLabels.setSelection(config.isHideLinkLabelsEnabled());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      enableHideLinkLabels.setLayoutData(gd);

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
		config.setZoomLevel(zoomLevelSpinner.getSelection());
		config.setObjectDoubleClickEnabled(enableObjectDoubleClick.getSelection());
		config.setHideLinkLabelsEnabled(enableHideLinkLabels.getSelection());
		return true;
	}
}
