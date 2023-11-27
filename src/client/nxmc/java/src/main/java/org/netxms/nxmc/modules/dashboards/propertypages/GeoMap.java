/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.eclipse.swt.widgets.Spinner;
import org.netxms.base.GeoLocation;
import org.netxms.base.GeoLocationFormatException;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.GeoMapConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Geo map configuration page
 */
public class GeoMap extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(GeoMap.class);

   private GeoMapConfig config;
   private TitleConfigurator title;
	private LabeledText latitude;
	private LabeledText longitude;
	private Spinner zoom;
   private ObjectSelector objectSelector;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public GeoMap(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(GeoMap.class).tr("Geo Map"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "geomap";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof GeoMapConfig;
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
      config = (GeoMapConfig)elementConfig;

		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		dialogArea.setLayout(layout);
		
      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      title.setLayoutData(gd);

		latitude = new LabeledText(dialogArea, SWT.NONE);
      latitude.setLabel(i18n.tr("Latitude"));
		latitude.setText(GeoLocation.latitudeToString(config.getLatitude()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		latitude.setLayoutData(gd);
		
		longitude = new LabeledText(dialogArea, SWT.NONE);
      longitude.setLabel(i18n.tr("Longitude"));
		longitude.setText(GeoLocation.longitudeToString(config.getLongitude()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		longitude.setLayoutData(gd);

      zoom = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Zoom"), 0, 18, WidgetHelper.DEFAULT_LAYOUT_DATA);
		zoom.setSelection(config.getZoom());

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      objectSelector.setLabel(i18n.tr("Root object"));
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId(config.getRootObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      objectSelector.setLayoutData(gd);
		
		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
      title.updateConfiguration(config);
		try
		{
			GeoLocation center = GeoLocation.parseGeoLocation(latitude.getText(), longitude.getText());
			config.setLatitude(center.getLatitude());
			config.setLongitude(center.getLongitude());
		}
		catch(GeoLocationFormatException e)
		{
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Please enter correct latitude and longitude value"));
		}
		config.setZoom(zoom.getSelection());
		config.setRootObjectId(objectSelector.getObjectId());
		return true;
	}
}
