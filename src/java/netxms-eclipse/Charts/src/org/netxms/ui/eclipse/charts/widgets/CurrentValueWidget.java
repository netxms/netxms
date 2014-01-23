/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Current value pseudo-chart implementation
 */
public class CurrentValueWidget extends GaugeWidget
{
	private Font[] valueFonts = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public CurrentValueWidget(Composite parent, int style)
	{
		super(parent, style);
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.widgets.GaugeWidget#createFonts()
	 */
	@Override
	protected void createFonts()
	{
		valueFonts = new Font[32];
		for(int i = 0; i < valueFonts.length; i++)
			valueFonts[i] = new Font(getDisplay(), fontName, i * 6 + 12, SWT.BOLD); //$NON-NLS-1$
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.widgets.GaugeWidget#disposeFonts()
	 */
	@Override
	protected void disposeFonts()
	{
		if (valueFonts != null)
		{
			for(int i = 0; i < valueFonts.length; i++)
				valueFonts[i].dispose();
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.widgets.GaugeWidget#renderElement(org.eclipse.swt.graphics.GC, org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement, int, int, int, int)
	 */
	@Override
	protected void renderElement(GC gc, DataComparisonElement dci, int x, int y, int w, int h)
	{
		Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, w - INNER_MARGIN_WIDTH * 2, h - INNER_MARGIN_HEIGHT * 2);
		
		if (elementBordersVisible)
		{
		   gc.drawRectangle(rect);
		}
		
		if (legendVisible)
		{
			rect.height -= gc.textExtent("MMM").y + 8; //$NON-NLS-1$
		}
		
		final String value = getValueAsDisplayString(dci);
		final Font font = WidgetHelper.getBestFittingFont(gc, valueFonts, value, rect.width, rect.height); //$NON-NLS-1$
		gc.setFont(font);
		if ((dci.getValue() <= leftRedZone) || (dci.getValue() >= rightRedZone))
		{
			gc.setForeground(colors.create(RED_ZONE_COLOR));
		}
		else if ((dci.getValue() <= leftYellowZone) || (dci.getValue() >= rightYellowZone))
		{
			gc.setForeground(colors.create(YELLOW_ZONE_COLOR));
		}
		else
		{
			gc.setForeground(colors.create(GREEN_ZONE_COLOR));
		}
		Point ext = gc.textExtent(value, SWT.DRAW_TRANSPARENT);
		gc.drawText(value, rect.x + rect.width / 2 - ext.x / 2, rect.y + rect.height / 2 - ext.y / 2, SWT.TRANSPARENT);
		
		// Draw legend, ignore legend position
		if (legendVisible)
		{
         gc.setFont(null);
			ext = gc.textExtent(dci.getName(), SWT.DRAW_TRANSPARENT);
			gc.setForeground(SharedColors.getColor(SharedColors.DIAL_CHART_LEGEND, getDisplay()));
			gc.drawText(dci.getName(), rect.x + ((rect.width - ext.x) / 2), rect.y + rect.height + 4, true);
		}
	}
}
