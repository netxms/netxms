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

import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LabelConfig;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Label configuration page
 */
public class LabelProperties extends PropertyPage
{
	private LabelConfig config;
	private LabeledText title; 
	private ColorSelector foreground;
	private ColorSelector background;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (LabelConfig)getElement().getAdapter(LabelConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		title = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI);
		title.setLabel(Messages.LabelProperties_Title);
		title.setText(config.getTitle());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		title.setLayoutData(gd);
		
		Composite fgArea = new Composite(dialogArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		fgArea.setLayoutData(gd);
		RowLayout areaLayout = new RowLayout();
		areaLayout.type = SWT.HORIZONTAL;
		areaLayout.marginBottom = 0;
		areaLayout.marginTop = 0;
		areaLayout.marginLeft = 0;
		areaLayout.marginRight = 0;
		fgArea.setLayout(areaLayout);
		new Label(fgArea, SWT.NONE).setText(Messages.LabelProperties_TextColor);
		foreground = new ColorSelector(fgArea);
		foreground.setColorValue(ColorConverter.rgbFromInt(config.getForegroundColorAsInt()));
		
		Composite bgArea = new Composite(dialogArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		bgArea.setLayoutData(gd);
		areaLayout = new RowLayout();
		areaLayout.type = SWT.HORIZONTAL;
		areaLayout.marginBottom = 0;
		areaLayout.marginTop = 0;
		areaLayout.marginLeft = 0;
		areaLayout.marginRight = 0;
		bgArea.setLayout(areaLayout);
		new Label(bgArea, SWT.NONE).setText(Messages.LabelProperties_BgColor);
		background = new ColorSelector(bgArea);
		background.setColorValue(ColorConverter.rgbFromInt(config.getBackgroundColorAsInt()));
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		config.setTitle(title.getText());
		config.setForeground("0x" + Integer.toHexString(ColorConverter.rgbToInt(foreground.getColorValue()))); //$NON-NLS-1$
		config.setBackground("0x" + Integer.toHexString(ColorConverter.rgbToInt(background.getColorValue()))); //$NON-NLS-1$
		return true;
	}
}
