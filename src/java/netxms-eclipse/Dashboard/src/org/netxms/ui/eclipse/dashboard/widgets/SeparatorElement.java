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
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.SeparatorConfig;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Dashboard separator element
 */
public class SeparatorElement extends ElementWidget
{
	private SeparatorConfig config;
	private Color bkColor;
	private Color fgColor;
	
	/**
	 * @param parent
	 * @param data
	 */
	public SeparatorElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, SWT.NONE, element, viewPart);
		
		try
		{
			config = SeparatorConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new SeparatorConfig();
		}
		
		bkColor = new Color(getDisplay(), ColorConverter.rgbFromInt(config.getBackgroundColorAsInt()));
		fgColor = new Color(getDisplay(), ColorConverter.rgbFromInt(config.getForegroundColorAsInt()));
		
		setBackground(bkColor);
		
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				bkColor.dispose();
				fgColor.dispose();
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardComposite#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		super.paintControl(e);
		
		if (config.getLineWidth() > 0)
		{
			e.gc.setForeground(fgColor);
			Rectangle rect = getClientArea();
			e.gc.setLineWidth(config.getLineWidth());
			int x1 = config.getLeftMargin();
			int x2 = rect.width - config.getRightMargin() - 1;
			int y = config.getTopMargin() + config.getLineWidth() / 2;
			e.gc.drawLine(x1, y, x2, y);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return new Point((wHint == SWT.DEFAULT) ? (config.getLeftMargin() + config.getRightMargin()) : wHint, config.getTopMargin() + config.getBottomMargin() + config.getLineWidth()); 
	}
}
