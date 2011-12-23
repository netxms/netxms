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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LabelConfig;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Dashboard label element
 */
public class LabelElement extends ElementWidget
{
	private LabelConfig config; 
	private Label label;
	private Font font;
	
	/**
	 * @param parent
	 * @param data
	 */
	public LabelElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
		
		try
		{
			config = LabelConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new LabelConfig();
		}
		
		FillLayout layout = new FillLayout();
		layout.marginHeight = 4;
		setLayout(layout);
		label = new Label(this, SWT.CENTER);
		label.setText(config.getTitle());
		label.setBackground(ColorConverter.colorFromInt(config.getBackgroundColorAsInt(), colors));
		label.setForeground(ColorConverter.colorFromInt(config.getForegroundColorAsInt(), colors));
		setBackground(ColorConverter.colorFromInt(config.getBackgroundColorAsInt(), colors));
		
		font = new Font(getDisplay(), "Verdana", 10, SWT.BOLD);
		label.setFont(font);
		
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				font.dispose();
			}
		});
	}
}
