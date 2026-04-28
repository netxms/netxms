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
import org.eclipse.swt.custom.StackLayout;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.netxms.base.GeoLocation;
import org.netxms.base.GeoLocationFormatException;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.MapCanvasType;
import org.netxms.client.maps.MapInitialViewMode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LatLonZoomEditor;
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
   private Combo canvasTypeCombo;
   private Composite pageStack;
   private StackLayout pageStackLayout;
   private Composite graphContainer;
   private Group sizeGroup;
   private LabeledSpinner width;
   private LabeledSpinner height;
   private Button fitToScreen;
   private Group typeGroup;
	private Button radioTypeNone;
	private Button radioTypeImage;
	private Button radioTypeGeoMap;
	private ImageSelector image;
   private LabeledCombo comboImageFit;
   private Group geomapGroup;
   private LatLonZoomEditor graphGeoEditor;
   private Group geoCanvasGroup;
   private LabeledCombo initialViewMode;
   private LatLonZoomEditor geoCanvasEditor;
	private Composite colorArea;
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

		GridData canvasTypeGd = new GridData();
		canvasTypeGd.grabExcessHorizontalSpace = true;
		canvasTypeGd.horizontalAlignment = SWT.FILL;
		canvasTypeCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Canvas type"), canvasTypeGd);
		canvasTypeCombo.add(i18n.tr("Graph"));
		canvasTypeCombo.add(i18n.tr("Geographical"));
		canvasTypeCombo.select((map.getCanvasType() == MapCanvasType.GEOGRAPHICAL) ? 1 : 0);

		// One canvas-type's content visible at a time; both children fill pageStack.
		pageStack = new Composite(dialogArea, SWT.NONE);
		pageStackLayout = new StackLayout();
		pageStack.setLayout(pageStackLayout);
		GridData pageStackGd = new GridData();
		pageStackGd.grabExcessHorizontalSpace = true;
		pageStackGd.grabExcessVerticalSpace = true;
		pageStackGd.horizontalAlignment = SWT.FILL;
		pageStackGd.verticalAlignment = SWT.FILL;
		pageStack.setLayoutData(pageStackGd);

		graphContainer = new Composite(pageStack, SWT.NONE);
		GridLayout graphLayout = new GridLayout();
		graphLayout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		graphLayout.marginWidth = 0;
		graphLayout.marginHeight = 0;
		graphLayout.numColumns = 1;
		graphContainer.setLayout(graphLayout);

      sizeGroup = new Group(graphContainer, SWT.NONE);
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
      
		typeGroup = new Group(graphContainer, SWT.NONE);
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

      image = new ImageSelector(graphContainer, SWT.NONE);
      image.setLabel(i18n.tr("Background image"));
      if (radioTypeImage.getSelection())
         image.setImageGuid(map.getBackground(), true);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      image.setLayoutData(gd);
      
      comboImageFit = new LabeledCombo(graphContainer, SWT.NONE);
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
         geomapGroup = new Group(graphContainer, SWT.NONE);
         geomapGroup.setText(i18n.tr("Geographic map"));
         gd = new GridData();
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
         geomapGroup.setLayoutData(gd);
         geomapGroup.setLayout(new GridLayout());

         graphGeoEditor = new LatLonZoomEditor(geomapGroup, SWT.NONE,
               map.getBackgroundLocation(), map.getBackgroundZoom());
         gd = new GridData();
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.FILL;
         graphGeoEditor.setLayoutData(gd);
      }
      
      colorArea = new Composite(graphContainer, SWT.NONE);
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

      // Geographical-canvas group. Replaces every other element on the page
      // when canvas type is GEOGRAPHICAL. The original elements (size group,
      // background-type group, image / geomap groups, color picker) are left
      // intact for the GRAPH case and just hidden when GEOGRAPHICAL is active.
      geoCanvasGroup = new Group(pageStack, SWT.NONE);
      geoCanvasGroup.setText(i18n.tr("Geographical canvas"));
      GridLayout geoCanvasLayout = new GridLayout();
      geoCanvasGroup.setLayout(geoCanvasLayout);

      GridData viewModeGd = new GridData();
      viewModeGd.horizontalAlignment = SWT.FILL;
      viewModeGd.grabExcessHorizontalSpace = true;
      initialViewMode = new LabeledCombo(geoCanvasGroup, SWT.READ_ONLY);
      initialViewMode.setLabel(i18n.tr("Initial view mode"));
      initialViewMode.add(i18n.tr("Fit to screen"));
      initialViewMode.add(i18n.tr("Zoom and center"));
      initialViewMode.select((map.getInitialViewMode() == MapInitialViewMode.ZOOM_AND_CENTER) ? 1 : 0);
      initialViewMode.setLayoutData(viewModeGd);

      geoCanvasEditor = new LatLonZoomEditor(geoCanvasGroup, SWT.NONE,
            map.getBackgroundLocation(), map.getBackgroundZoom());
      GridData editorGd = new GridData();
      editorGd.grabExcessHorizontalSpace = true;
      editorGd.horizontalAlignment = SWT.FILL;
      geoCanvasEditor.setLayoutData(editorGd);

      initialViewMode.getComboControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            updateGeoCanvasCoordinateState();
         }
      });

      canvasTypeCombo.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            updateCanvasTypeVisibility();
         }
      });

		enableControls(radioTypeImage.getSelection(), disableGeolocationBackground ? false : radioTypeGeoMap.getSelection());
		updateCanvasTypeVisibility();
      return dialogArea;
	}

	/**
	 * Switch the page between the GRAPH (original) and GEOGRAPHICAL layouts.
	 * Uses a StackLayout so both children fill the same area — switching is
	 * just changing topControl and laying out the stack, no exclude/visible
	 * gymnastics and no size reflow up the dialog hierarchy.
	 */
	private void updateCanvasTypeVisibility()
	{
		boolean geo = (canvasTypeCombo.getSelectionIndex() == 1);
		pageStackLayout.topControl = geo ? geoCanvasGroup : graphContainer;
		pageStack.layout(true, true);
		updateGeoCanvasCoordinateState();
	}

	/**
	 * Centre coordinates and zoom level only have an effect when the initial
	 * view mode is "Zoom and center" — for "Fit to screen" the viewport is
	 * derived from the placed objects' bounding box, so grey out those inputs
	 * to make the relationship clear.
	 */
	private void updateGeoCanvasCoordinateState()
	{
		if ((geoCanvasGroup == null) || (canvasTypeCombo.getSelectionIndex() != 1))
			return;
		boolean zoomAndCenter = (initialViewMode.getSelectionIndex() == 1);
		geoCanvasEditor.setEnabled(zoomAndCenter);
	}

	/**
	 * @param imageGroup
	 * @param geoGroup
	 */
	private void enableControls(boolean imageGroup, boolean geoGroup)
	{
		image.setEnabled(imageGroup);
		if (!disableGeolocationBackground)
			graphGeoEditor.setEnabled(geoGroup);
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());

		boolean geographicalCanvas = (canvasTypeCombo.getSelectionIndex() == 1);
		md.setMapCanvasType(geographicalCanvas ? MapCanvasType.GEOGRAPHICAL : MapCanvasType.GRAPH);

		if (geographicalCanvas)
		{
			// Geographical canvas: only the canvas-specific settings apply —
			// the tile background is implicit, map size / image / color are
			// inapplicable here. Pass the map's currently-persisted background
			// GUID and color through unchanged so toggling Canvas type to
			// Geographical and back doesn't quietly wipe the user's saved
			// image / "None" / GEOMAP choice on the graph side.
			GeoLocation location;
			try
			{
				location = GeoLocation.parseGeoLocation(geoCanvasEditor.getLatitudeText(), geoCanvasEditor.getLongitudeText());
			}
			catch(GeoLocationFormatException e)
			{
				MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Geolocation format error"));
				return false;
			}
			md.setMapBackground(map.getBackground(), location, geoCanvasEditor.getZoom(), map.getBackgroundColor());
			md.setMapInitialViewMode((initialViewMode.getSelectionIndex() == 1) ? MapInitialViewMode.ZOOM_AND_CENTER : MapInitialViewMode.FIT_TO_SCREEN);
			submitModification(md, isApply);
			return true;
		}

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
				location = GeoLocation.parseGeoLocation(graphGeoEditor.getLatitudeText(), graphGeoEditor.getLongitudeText());
			}
			catch(GeoLocationFormatException e)
			{
				MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Geolocation format error"));
				return false;
			}
			md.setMapBackground(NetworkMap.GEOMAP_BACKGROUND, location, graphGeoEditor.getZoom(), ColorConverter.rgbToInt(backgroundColor.getColorValue()));
		}
		
		submitModification(md, isApply);
		return true;
	}

	/**
	 * Submit the modification request to the server. Centralised so both the
	 * graph-canvas and geographical-canvas paths in {@link #applyChanges} flow
	 * through the same job + setValid handling.
	 */
	private void submitModification(final NXCObjectModificationData md, final boolean isApply)
	{
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
	}
}
