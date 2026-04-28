/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.base.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Scale;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.base.GeoLocation;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Composite editor for a (latitude, longitude, zoom level) triple used by
 * everything geo-related in NetXMS — the geographic-map background of a graph
 * canvas, the geographical canvas's initial view, and dashboard geo widgets.
 *
 * <p>The widget renders three rows: Latitude (text), Longitude (text), and a
 * Zoom level scale+spinner pair that mirror each other. Tile zoom levels are
 * constrained to {@code [1..18]}.</p>
 *
 * <p>The constructor accepts the initial values; subsequent edits are read
 * back via {@link #getLatitudeText}, {@link #getLongitudeText}, and
 * {@link #getZoom}. Parsing/validation of the lat/lon text is left to the
 * caller (use {@link GeoLocation#parseGeoLocation}).</p>
 */
public class LatLonZoomEditor extends Composite
{
   public static final int MIN_ZOOM = 1;
   public static final int MAX_ZOOM = 18;

   private final LabeledText latitude;
   private final LabeledText longitude;
   private final Label zoomLabel;
   private final Scale zoomScale;
   private final Spinner zoomSpinner;

   /**
    * Construct a new editor.
    *
    * @param parent  parent composite (any layout)
    * @param style   SWT style bits applied to this composite
    * @param initialLocation initial latitude/longitude (may be {@code null}
    *                — empty fields are shown)
    * @param initialZoom initial zoom level; clamped to the legal range; values
    *                {@code <= 0} are shown as the minimum
    */
   public LatLonZoomEditor(Composite parent, int style, GeoLocation initialLocation, int initialZoom)
   {
      super(parent, style);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      setLayout(layout);

      latitude = new LabeledText(this, SWT.NONE);
      latitude.setLabel("Latitude"); //$NON-NLS-1$
      latitude.setText((initialLocation != null) ? initialLocation.getLatitudeAsString() : "");
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      latitude.setLayoutData(gd);

      longitude = new LabeledText(this, SWT.NONE);
      longitude.setLabel("Longitude"); //$NON-NLS-1$
      longitude.setText((initialLocation != null) ? initialLocation.getLongitudeAsString() : "");
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      longitude.setLayoutData(gd);

      Composite zoomRow = new Composite(this, SWT.NONE);
      GridLayout zoomLayout = new GridLayout();
      zoomLayout.numColumns = 2;
      zoomLayout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      zoomLayout.marginHeight = 0;
      zoomLayout.marginWidth = 0;
      zoomLayout.marginTop = WidgetHelper.OUTER_SPACING;
      zoomRow.setLayout(zoomLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      zoomRow.setLayoutData(gd);

      zoomLabel = new Label(zoomRow, SWT.NONE);
      zoomLabel.setText("Zoom level"); //$NON-NLS-1$
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.horizontalSpan = 2;
      zoomLabel.setLayoutData(gd);

      int clampedZoom = clampZoom(initialZoom);

      zoomScale = new Scale(zoomRow, SWT.HORIZONTAL);
      zoomScale.setMinimum(MIN_ZOOM);
      zoomScale.setMaximum(MAX_ZOOM);
      zoomScale.setSelection(clampedZoom);
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

      zoomSpinner = new Spinner(zoomRow, SWT.BORDER);
      zoomSpinner.setMinimum(MIN_ZOOM);
      zoomSpinner.setMaximum(MAX_ZOOM);
      zoomSpinner.setSelection(clampedZoom);
      zoomSpinner.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            zoomScale.setSelection(zoomSpinner.getSelection());
         }
      });
   }

   private static int clampZoom(int z)
   {
      if (z < MIN_ZOOM) return MIN_ZOOM;
      if (z > MAX_ZOOM) return MAX_ZOOM;
      return z;
   }

   /**
    * @return latitude as currently entered (raw text — use
    *         {@link GeoLocation#parseGeoLocation} to parse)
    */
   public String getLatitudeText()
   {
      return latitude.getText();
   }

   /**
    * @return longitude as currently entered (raw text)
    */
   public String getLongitudeText()
   {
      return longitude.getText();
   }

   /**
    * @return zoom level (always in {@code [MIN_ZOOM..MAX_ZOOM]})
    */
   public int getZoom()
   {
      return zoomSpinner.getSelection();
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
    *
    * Cascade enable state down to the contained controls. {@code Composite}'s
    * own setEnabled doesn't propagate, so this is the right hook for callers
    * that want the whole editor greyed out (e.g. when "Fit to screen" is the
    * active view mode and the entered coordinates wouldn't be used).
    */
   @Override
   public void setEnabled(boolean enabled)
   {
      super.setEnabled(enabled);
      latitude.setEnabled(enabled);
      longitude.setEnabled(enabled);
      zoomLabel.setEnabled(enabled);
      zoomScale.setEnabled(enabled);
      zoomSpinner.setEnabled(enabled);
   }
}
