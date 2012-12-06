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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.base.GeoLocation;
import org.netxms.base.GeoLocationFormatException;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.GeoMapConfig;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Geo map configuration page
 */
public class GeoMap extends PropertyPage
{
	private static final long serialVersionUID = 1L;

	private GeoMapConfig config;
	private LabeledText title;
	private LabeledText latitude;
	private LabeledText longitude;
	private Spinner zoom;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (GeoMapConfig)getElement().getAdapter(GeoMapConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		dialogArea.setLayout(layout);
		
		title = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI);
		title.setLabel(Messages.GeoMap_Title);
		title.setText(config.getTitle());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 3;
		title.setLayoutData(gd);
		
		latitude = new LabeledText(dialogArea, SWT.NONE);
		latitude.setLabel(Messages.GeoMap_Latitude);
		latitude.setText(GeoLocation.latitudeToString(config.getLatitude()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		latitude.setLayoutData(gd);
		
		longitude = new LabeledText(dialogArea, SWT.NONE);
		longitude.setLabel(Messages.GeoMap_Longitude);
		longitude.setText(GeoLocation.longitudeToString(config.getLongitude()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		longitude.setLayoutData(gd);
		
		zoom = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, Messages.GeoMap_Zoom, 0, 18, WidgetHelper.DEFAULT_LAYOUT_DATA);
		zoom.setSelection(config.getZoom());
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		try
		{
			GeoLocation center = GeoLocation.parseGeoLocation(latitude.getText(), longitude.getText());
			config.setLatitude(center.getLatitude());
			config.setLongitude(center.getLongitude());
		}
		catch(GeoLocationFormatException e)
		{
			MessageDialog.openError(getShell(), Messages.GeoMap_Error, Messages.GeoMap_ErrorText);
		}
		config.setTitle(title.getText());
		config.setZoom(zoom.getSelection());
		return true;
	}
}
