/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.netxms.base.GeoLocation;
import org.netxms.base.GeoLocationFormatException;
import org.netxms.base.PostalAddress;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Location" property page
 */
public class Location extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(Location.class);

   private LabeledText latitude;
   private LabeledText longitude;
   private Button radioTypeUndefined;
   private Button radioTypeManual;
   private Button radioTypeAuto;
   private LabeledText country;
   private LabeledText region;
   private LabeledText city;
   private LabeledText district;
   private LabeledText streetAddress;
   private LabeledText postcode;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public Location(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(Location.class).tr("Location"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "location";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return ((object instanceof DataCollectionTarget) && !(object instanceof Circuit)) || (object instanceof Collector) || (object instanceof Container) || (object instanceof Rack) || (object instanceof Zone);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GeoLocation gl = object.getGeolocation();

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      Group typeGroup = new Group(dialogArea, SWT.NONE);
      typeGroup.setText(i18n.tr("Location type"));
      GridData gd = new GridData();
      gd.verticalSpan = 2;
      typeGroup.setLayoutData(gd);
      layout = new GridLayout();
      typeGroup.setLayout(layout);
      
      radioTypeUndefined = new Button(typeGroup, SWT.RADIO);
      radioTypeUndefined.setText(i18n.tr("&Undefined"));
      radioTypeUndefined.setSelection(gl.getType() == GeoLocation.UNSET);
      
      radioTypeManual = new Button(typeGroup, SWT.RADIO);
      radioTypeManual.setText(i18n.tr("&Manual"));
      radioTypeManual.setSelection(gl.getType() == GeoLocation.MANUAL);
      
      radioTypeAuto = new Button(typeGroup, SWT.RADIO);
      radioTypeAuto.setText(i18n.tr("&Automatic from GPS receiver"));
      radioTypeAuto.setSelection(gl.getType() == GeoLocation.GPS);
      
      latitude = new LabeledText(dialogArea, SWT.NONE);
      latitude.setLabel(i18n.tr("Latitude"));
      if (gl.getType() != GeoLocation.UNSET)
      	latitude.setText(gl.getLatitudeAsString());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      latitude.setLayoutData(gd);
      latitude.setEnabled(gl.getType() == GeoLocation.MANUAL);
      
      longitude = new LabeledText(dialogArea, SWT.NONE);
      longitude.setLabel(i18n.tr("Longitude"));
      if (gl.getType() != GeoLocation.UNSET)
      	longitude.setText(gl.getLongitudeAsString());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      longitude.setLayoutData(gd);
      longitude.setEnabled(gl.getType() == GeoLocation.MANUAL);
      
      final SelectionListener listener = new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				latitude.setEnabled(radioTypeManual.getSelection());
				longitude.setEnabled(radioTypeManual.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		};
		radioTypeUndefined.addSelectionListener(listener);
		radioTypeManual.addSelectionListener(listener);
		radioTypeAuto.addSelectionListener(listener);
		
		country = new LabeledText(dialogArea, SWT.NONE);
		country.setLabel(i18n.tr("Country"));
		country.setText(object.getPostalAddress().country);
		country.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      
      region = new LabeledText(dialogArea, SWT.NONE);
      region.setLabel(i18n.tr("Region"));
      region.setText(object.getPostalAddress().region);
      region.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      city = new LabeledText(dialogArea, SWT.NONE);
      city.setLabel(i18n.tr("City"));
      city.setText(object.getPostalAddress().city);
      city.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      
      district = new LabeledText(dialogArea, SWT.NONE);
      district.setLabel(i18n.tr("District"));
      district.setText(object.getPostalAddress().district);
      district.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      streetAddress = new LabeledText(dialogArea, SWT.NONE);
      streetAddress.setLabel(i18n.tr("Street address"));
      streetAddress.setText(object.getPostalAddress().streetAddress);
      streetAddress.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      postcode = new LabeledText(dialogArea, SWT.NONE);
      postcode.setLabel(i18n.tr("Postcode"));
      postcode.setText(object.getPostalAddress().postcode);
      postcode.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

		return dialogArea;
	}

	/**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
	protected boolean applyChanges(final boolean isApply)
	{
		int type = GeoLocation.UNSET;
		if (radioTypeManual.getSelection())
			type = GeoLocation.MANUAL;
		else if (radioTypeAuto.getSelection())
			type = GeoLocation.GPS;
		
		GeoLocation location;
		if (type == GeoLocation.MANUAL)
		{
			try
			{
				location = GeoLocation.parseGeoLocation(latitude.getText(), longitude.getText());
			}
			catch(GeoLocationFormatException e)
			{
				MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Geolocation format error"));
				return false;
			}
		}
		else
		{
			location = new GeoLocation(type == GeoLocation.GPS);
		}
		
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setGeolocation(location);		
		md.setPostalAddress(new PostalAddress(country.getText().trim(), region.getText().trim(), city.getText().trim(),
		      district.getText().trim(), streetAddress.getText().trim(), postcode.getText().trim()));

		if (isApply)
			setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating geolocation for object {0}", object.getObjectName()), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot modify object's geolocation");
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
					runInUIThread(() -> Location.this.setValid(true));
			}
		}.start();
		
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		radioTypeUndefined.setSelection(true);
		radioTypeManual.setSelection(false);
		radioTypeAuto.setSelection(false);
		latitude.setText(""); //$NON-NLS-1$
		latitude.setEnabled(false);
		longitude.setText(""); //$NON-NLS-1$
		longitude.setEnabled(false);
	}
}
