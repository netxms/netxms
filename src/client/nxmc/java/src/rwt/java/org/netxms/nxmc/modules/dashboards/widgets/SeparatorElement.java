/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.nxmc.modules.dashboards.config.SeparatorConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.tools.ColorConverter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.gson.Gson;

/**
 * Dashboard separator element
 */
public class SeparatorElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(SeparatorElement.class);
   
	private SeparatorConfig config;
	private Color bkColor;
	private Color fgColor;

	/**
	 * @param parent
	 * @param data
	 */
   public SeparatorElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, SWT.NONE, element, view);

		try
		{
         config = new Gson().fromJson(element.getData(), SeparatorConfig.class);
		}
		catch(Exception e)
		{
         logger.error("Cannot parse dashboard element configuration", e);
			config = new SeparatorConfig();
		}

      getMainArea().dispose();

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

      addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
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
      });
	}

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return new Point((wHint == SWT.DEFAULT) ? (config.getLeftMargin() + config.getRightMargin()) : wHint, config.getTopMargin() + config.getBottomMargin() + config.getLineWidth()); 
	}
}
