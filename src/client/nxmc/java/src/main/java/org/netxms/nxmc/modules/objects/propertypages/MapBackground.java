/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Scale;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.base.GeoLocation;
import org.netxms.base.GeoLocationFormatException;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.widgets.ImageSelector;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Map Background" property page
 */
public class MapBackground extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(MapBackground.class);
   
   private NetworkMap map;
   private LabeledSpinner width;
   private LabeledSpinner height;
   private Button fitToScreen;
	private Button radioTypeNone;
	private Button radioTypeImage;
	private Button radioTypeGeoMap;
	private ImageSelector image;
   private LabeledCombo comboImageFit;
	private LabeledText latitude;
	private LabeledText longitude;
	private Scale zoomScale;
	private Spinner zoomSpinner;
	private Label zoomLabel;
	private ColorSelector backgroundColor;
	private boolean disableGeolocationBackground;

	/**
	 * Constructor 
	 * 
	 * @param object object to show properties for
	 */
   public MapBackground(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(MapBackground.class).tr("Map Background"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "map-background";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof NetworkMap);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);

      map = (NetworkMap)object;

      PreferenceStore settings = PreferenceStore.getInstance();
      disableGeolocationBackground = settings.getAsBoolean("DISABLE_GEOLOCATION_BACKGROUND", false); //$NON-NLS-1$

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 1;
		dialogArea.setLayout(layout);
      
      Group sizeGroup = new Group(dialogArea, SWT.NONE);
      sizeGroup.setText(i18n.tr("Map size"));
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      sizeGroup.setLayoutData(gd);
      layout = new GridLayout();
      layout.numColumns = 2;
      sizeGroup.setLayout(layout);

      fitToScreen = new Button(sizeGroup, SWT.CHECK);
      fitToScreen.setText("Fit to scrren");
      fitToScreen.setSelection(map.isFitToScreen());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      fitToScreen.setLayoutData(gd);
      fitToScreen.addSelectionListener(new SelectionAdapter() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            width.setEnabled(!fitToScreen.getSelection());
            height.setEnabled(!fitToScreen.getSelection());
         }
      });
      
      width = new LabeledSpinner(sizeGroup, SWT.NONE);
      width.setRange(0, Integer.MAX_VALUE);
      width.setLabel(i18n.tr("Width"));
      width.setSelection(map.getWidth());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      width.setLayoutData(gd);
      width.setEnabled(!fitToScreen.getSelection());
      
      height = new LabeledSpinner(sizeGroup, SWT.NONE);
      height.setRange(0, Integer.MAX_VALUE);
      height.setLabel(i18n.tr("Height"));
      height.setSelection(map.getHeight());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      height.setLayoutData(gd);
      height.setEnabled(!fitToScreen.getSelection());
      
		Group typeGroup = new Group(dialogArea, SWT.NONE);
      typeGroup.setText(i18n.tr("Background type"));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      typeGroup.setLayoutData(gd);
      layout = new GridLayout();
      typeGroup.setLayout(layout);

      final SelectionListener listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
		      enableControls(radioTypeImage.getSelection(), disableGeolocationBackground ? false : radioTypeGeoMap.getSelection());
			}
		};
      
      radioTypeNone = new Button(typeGroup, SWT.RADIO);
      radioTypeNone.setText(i18n.tr("&None"));
      radioTypeNone.setSelection(map.getBackground().equals(NXCommon.EMPTY_GUID));
      radioTypeNone.addSelectionListener(listener);
      
      radioTypeImage = new Button(typeGroup, SWT.RADIO);
      radioTypeImage.setText(i18n.tr("&Image"));
      radioTypeImage.setSelection(!map.getBackground().equals(NXCommon.EMPTY_GUID) && !map.getBackground().equals(NetworkMap.GEOMAP_BACKGROUND));
      radioTypeImage.addSelectionListener(listener);

      if (!disableGeolocationBackground)
      {
         radioTypeGeoMap = new Button(typeGroup, SWT.RADIO);
         radioTypeGeoMap.setText(i18n.tr("&Geographic Map"));
         radioTypeGeoMap.setSelection(map.getBackground().equals(NetworkMap.GEOMAP_BACKGROUND));
         radioTypeGeoMap.addSelectionListener(listener);
      }
      else
      {
         if (map.getBackground().equals(NetworkMap.GEOMAP_BACKGROUND))
            radioTypeNone.setSelection(true);
      }

      image = new ImageSelector(dialogArea, SWT.NONE);
      image.setLabel(i18n.tr("Background image"));
      if (radioTypeImage.getSelection())
         image.setImageGuid(map.getBackground(), true);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      image.setLayoutData(gd);
      
      comboImageFit = new LabeledCombo(dialogArea, SWT.NONE);
      comboImageFit.setLabel("Image fit");
      comboImageFit.add("Default");
      comboImageFit.add("Center");
      comboImageFit.add("Fit");
      if (map.isCenterBackgroundImage())
      {
         comboImageFit.select(1);         
      }
      else if (map.isFitBackgroundImage())
      {
         comboImageFit.select(2);            
      }
      else
      {
         comboImageFit.select(0);          
      }

      if (!disableGeolocationBackground)
      {
         Group geomapGroup = new Group(dialogArea, SWT.NONE);
         geomapGroup.setText(i18n.tr("Geographic map"));
         gd = new GridData();
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
         geomapGroup.setLayoutData(gd);
         layout = new GridLayout();
         geomapGroup.setLayout(layout);
         
         GeoLocation gl = map.getBackgroundLocation();
   
         latitude = new LabeledText(geomapGroup, SWT.NONE);
         latitude.setLabel(i18n.tr("Latitude"));
      	latitude.setText(gl.getLatitudeAsString());
         gd = new GridData();
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
         latitude.setLayoutData(gd);
         
         longitude = new LabeledText(geomapGroup, SWT.NONE);
         longitude.setLabel(i18n.tr("Longitude"));
         longitude.setText(gl.getLongitudeAsString());
         gd = new GridData();
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
         longitude.setLayoutData(gd);
         
         Composite zoomGroup = new Composite(geomapGroup, SWT.NONE);
         layout = new GridLayout();
         layout.numColumns = 2;
         layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
         layout.marginHeight = 0;
         layout.marginWidth = 0;
         layout.marginTop = WidgetHelper.OUTER_SPACING;
         zoomGroup.setLayout(layout);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         zoomGroup.setLayoutData(gd);
         
         zoomLabel = new Label(zoomGroup, SWT.NONE);
         zoomLabel.setText(i18n.tr("Zoom level"));
         gd = new GridData();
         gd.horizontalAlignment = SWT.LEFT;
         gd.horizontalSpan = 2;
         zoomLabel.setLayoutData(gd);
         
         zoomScale = new Scale(zoomGroup, SWT.HORIZONTAL);
         zoomScale.setMinimum(1);
         zoomScale.setMaximum(18);
         zoomScale.setSelection(map.getBackgroundZoom());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         zoomScale.setLayoutData(gd);
         zoomScale.addSelectionListener(new SelectionAdapter() {
   			@Override
   			public void widgetSelected(SelectionEvent e)
   			{
   				zoomSpinner.setSelection(zoomScale.getSelection());
   			}
         });
         
         zoomSpinner = new Spinner(zoomGroup, SWT.BORDER);
         zoomSpinner.setMinimum(1);
         zoomSpinner.setMaximum(18);
         zoomSpinner.setSelection(map.getBackgroundZoom());
         zoomSpinner.addSelectionListener(new SelectionAdapter() {
   			@Override
   			public void widgetSelected(SelectionEvent e)
   			{
   				zoomScale.setSelection(zoomSpinner.getSelection());
   			}
   		});
      }
      
      Composite colorArea = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      colorArea.setLayout(layout);
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      colorArea.setLayoutData(gd);
      Label label = new Label(colorArea, SWT.NONE);
      label.setText(i18n.tr("Background color:"));
      
      backgroundColor = new ColorSelector(colorArea);
      backgroundColor.setColorValue(ColorConverter.rgbFromInt(map.getBackgroundColor()));

		enableControls(radioTypeImage.getSelection(), disableGeolocationBackground ? false : radioTypeGeoMap.getSelection());
      return dialogArea;
	}
	
	/**
	 * @param imageGroup
	 * @param geoGroup
	 */
	private void enableControls(boolean imageGroup, boolean geoGroup)
	{
		image.setEnabled(imageGroup);
		if (!disableGeolocationBackground)
		{
   		latitude.setEnabled(geoGroup);
   		longitude.setEnabled(geoGroup);
   		zoomLabel.setEnabled(geoGroup);
   		zoomScale.setEnabled(geoGroup);
   		zoomSpinner.setEnabled(geoGroup);
		}
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());

      if (fitToScreen.getSelection())
      {
         md.setObjectFlags(NetworkMap.MF_FIT_TO_SCREEN, NetworkMap.MF_FIT_TO_SCREEN);  
      }
      else
      {
         md.setObjectFlags(0, NetworkMap.MF_FIT_TO_SCREEN);           
         md.setMapSize(width.getSelection(), height.getSelection());
      }
      
		if (radioTypeNone.getSelection())
		{
			md.setMapBackground(NXCommon.EMPTY_GUID, new GeoLocation(false), 0, ColorConverter.rgbToInt(backgroundColor.getColorValue()));
		}
		else if (radioTypeImage.getSelection())
		{
			md.setMapBackground(image.getImageGuid(), new GeoLocation(false), 0, ColorConverter.rgbToInt(backgroundColor.getColorValue()));
			if (comboImageFit.getSelectionIndex() == 1)
			{
	         md.setObjectFlags(NetworkMap.MF_CENTER_BKGND_IMAGE, NetworkMap.MF_BKGND_IMAGE_FLAGS);			   
			}
			else if (comboImageFit.getSelectionIndex() == 2)
         {
            md.setObjectFlags(NetworkMap.MF_FIT_BKGND_IMAGE, NetworkMap.MF_BKGND_IMAGE_FLAGS);           
         }
			else
			{
            md.setObjectFlags(0, NetworkMap.MF_BKGND_IMAGE_FLAGS);   
			}
		}
		else if (!disableGeolocationBackground && radioTypeGeoMap.getSelection())
		{
			GeoLocation location;
			try
			{
				location = GeoLocation.parseGeoLocation(latitude.getText(), longitude.getText());
			}
			catch(GeoLocationFormatException e)
			{
				MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Geolocation format error"));
				return false;
			}
			md.setMapBackground(NetworkMap.GEOMAP_BACKGROUND, location, zoomSpinner.getSelection(), ColorConverter.rgbToInt(backgroundColor.getColorValue()));
		}
		
		if (isApply)
			setValid(false);

		final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating map background for map object {0}", object.getObjectName()), null, messageArea) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot modify background for map object {0}", object.getObjectName());
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> MapBackground.this.setValid(true));
			}
		}.start();
		
		return true;
	}
}
