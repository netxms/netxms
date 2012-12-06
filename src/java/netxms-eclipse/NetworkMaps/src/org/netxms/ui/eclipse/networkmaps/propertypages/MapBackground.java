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
package org.netxms.ui.eclipse.networkmaps.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Scale;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.base.GeoLocation;
import org.netxms.base.GeoLocationFormatException;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.imagelibrary.widgets.ImageSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Map Background" property page
 */
public class MapBackground extends PropertyPage
{
	private NetworkMap object;
	private Button radioTypeNone;
	private Button radioTypeImage;
	private Button radioTypeGeoMap;
	private ImageSelector image;
	private LabeledText latitude;
	private LabeledText longitude;
	private Scale zoomScale;
	private Spinner zoomSpinner;
	private Label zoomLabel;
	private ColorSelector backgroundColor;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (NetworkMap)getElement().getAdapter(NetworkMap.class);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 1;
		dialogArea.setLayout(layout);
      
		Group typeGroup = new Group(dialogArea, SWT.NONE);
      typeGroup.setText("Background type");
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      typeGroup.setLayoutData(gd);
      layout = new GridLayout();
      typeGroup.setLayout(layout);
      
      final SelectionListener listener = new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				enableControls(radioTypeImage.getSelection(), radioTypeGeoMap.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		};
      
      radioTypeNone = new Button(typeGroup, SWT.RADIO);
      radioTypeNone.setText("&None");
      radioTypeNone.setSelection(object.getBackground().equals(NXCommon.EMPTY_GUID));
      radioTypeNone.addSelectionListener(listener);
      
      radioTypeImage = new Button(typeGroup, SWT.RADIO);
      radioTypeImage.setText("&Image");
      radioTypeImage.setSelection(!object.getBackground().equals(NXCommon.EMPTY_GUID) && !object.getBackground().equals(NetworkMap.GEOMAP_BACKGROUND));
      radioTypeImage.addSelectionListener(listener);

      radioTypeGeoMap = new Button(typeGroup, SWT.RADIO);
      radioTypeGeoMap.setText("&Geographic Map");
      radioTypeGeoMap.setSelection(object.getBackground().equals(NetworkMap.GEOMAP_BACKGROUND));
      radioTypeGeoMap.addSelectionListener(listener);
      
      image = new ImageSelector(dialogArea, SWT.NONE);
      image.setLabel("Background image");
      if (radioTypeImage.getSelection())
      	image.setImageGuid(object.getBackground(), true);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      image.setLayoutData(gd);
      
      Group geomapGroup = new Group(dialogArea, SWT.NONE);
      geomapGroup.setText("Geographic map");
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      geomapGroup.setLayoutData(gd);
      layout = new GridLayout();
      geomapGroup.setLayout(layout);
      
      GeoLocation gl = object.getBackgroundLocation();

      latitude = new LabeledText(geomapGroup, SWT.NONE);
      latitude.setLabel("Latitude");
   	latitude.setText(gl.getLatitudeAsString());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      latitude.setLayoutData(gd);
      
      longitude = new LabeledText(geomapGroup, SWT.NONE);
      longitude.setLabel("Longitude");
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
      zoomLabel.setText("Zoom level");
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.horizontalSpan = 2;
      zoomLabel.setLayoutData(gd);
      
      zoomScale = new Scale(zoomGroup, SWT.HORIZONTAL);
      zoomScale.setMinimum(1);
      zoomScale.setMaximum(18);
      zoomScale.setSelection(object.getBackgroundZoom());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      zoomScale.setLayoutData(gd);
      zoomScale.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				zoomSpinner.setSelection(zoomScale.getSelection());
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
      });
      
      zoomSpinner = new Spinner(zoomGroup, SWT.BORDER);
      zoomSpinner.setMinimum(1);
      zoomSpinner.setMaximum(18);
      zoomSpinner.setSelection(object.getBackgroundZoom());
      zoomSpinner.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				zoomScale.setSelection(zoomSpinner.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
      
      Composite colorArea = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.marginTop = WidgetHelper.OUTER_SPACING;
      colorArea.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      colorArea.setLayoutData(gd);
      Label label = new Label(colorArea, SWT.NONE);
      label.setText("Background color:");
      backgroundColor = new ColorSelector(colorArea);
      backgroundColor.setColorValue(ColorConverter.rgbFromInt(object.getBackgroundColor()));

		enableControls(radioTypeImage.getSelection(), radioTypeGeoMap.getSelection());
      return dialogArea;
	}
	
	/**
	 * @param imageGroup
	 * @param geoGroup
	 */
	private void enableControls(boolean imageGroup, boolean geoGroup)
	{
		image.setEnabled(imageGroup);
		latitude.setEnabled(geoGroup);
		longitude.setEnabled(geoGroup);
		zoomLabel.setEnabled(geoGroup);
		zoomScale.setEnabled(geoGroup);
		zoomSpinner.setEnabled(geoGroup);
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		
		if (radioTypeNone.getSelection())
		{
			md.setMapBackground(NXCommon.EMPTY_GUID, new GeoLocation(false), 0, ColorConverter.rgbToInt(backgroundColor.getColorValue()));
		}
		else if (radioTypeImage.getSelection())
		{
			md.setMapBackground(image.getImageGuid(), new GeoLocation(false), 0, ColorConverter.rgbToInt(backgroundColor.getColorValue()));
		}
		else if (radioTypeGeoMap.getSelection())
		{
			GeoLocation location;
			try
			{
				location = GeoLocation.parseGeoLocation(latitude.getText(), longitude.getText());
			}
			catch(GeoLocationFormatException e)
			{
				MessageDialog.openError(getShell(), "Error", "Geolocation format error");
				return false;
			}
			md.setMapBackground(NetworkMap.GEOMAP_BACKGROUND, location, zoomSpinner.getSelection(), ColorConverter.rgbToInt(backgroundColor.getColorValue()));
		}
		
		if (isApply)
			setValid(false);

		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Update map background for map object " + object.getObjectName(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot modify background for map object " + object.getObjectName();
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					Display.getDefault().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							MapBackground.this.setValid(true);
						}
					});
				}
			}
		}.start();
		
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
