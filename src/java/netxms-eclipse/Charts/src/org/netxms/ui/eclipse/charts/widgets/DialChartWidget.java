/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.text.DecimalFormat;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.charts.Messages;
import org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dial chart implementation
 */
public class DialChartWidget extends GaugeWidget
{
	private static final int NEEDLE_PIN_RADIUS = 8;
	private static final int SCALE_OFFSET = 30;	// In percents
	private static final int SCALE_WIDTH = 10;	// In percents
	
	private Font[] scaleFonts = null;
	private Font[] valueFonts = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DialChartWidget(Composite parent, int style)
	{
		super(parent, style);
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.widgets.GaugeWidget#createFonts()
	 */
	@Override
	protected void createFonts()
	{
		scaleFonts = new Font[16];
		for(int i = 0; i < scaleFonts.length; i++)
			scaleFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.NORMAL); //$NON-NLS-1$

		valueFonts = new Font[16];
		for(int i = 0; i < valueFonts.length; i++)
			valueFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.BOLD); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.widgets.GaugeWidget#disposeFonts()
	 */
	@Override
	protected void disposeFonts()
	{
		if (scaleFonts != null)
		{
			for(int i = 0; i < scaleFonts.length; i++)
			{
				scaleFonts[i].dispose();
			}
		}
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
		if (legendVisible && !legendInside)
		{
			rect.height -= gc.textExtent("MMM").y - 4; //$NON-NLS-1$
		}
		if (rect.height > rect.width)
		{
			rect.y += (rect.height - rect.width) / 2;
			rect.height = rect.width;
		}
		else
		{
			rect.x += (rect.width - rect.height) / 2;
			rect.width = rect.height;
		}
		
		double angleValue = (maxValue - minValue) / 270;
		int outerRadius = (rect.width + 1) / 2;
		int scaleOuterOffset = ((rect.width / 2) * SCALE_OFFSET / 100);
		int scaleInnerOffset = ((rect.width / 2) * (SCALE_OFFSET + SCALE_WIDTH) / 100);
		
		int cx = rect.x + rect.width / 2 + 1;
		int cy = rect.y + rect.height / 2 + 1;
		gc.setBackground(getColorFromPreferences("Chart.Colors.PlotArea")); //$NON-NLS-1$
		gc.fillArc(rect.x, rect.y, rect.width, rect.height, 0, 360);
		
		// Draw zones
		int startAngle = 225;
		startAngle = drawZone(gc, rect, startAngle, minValue, leftRedZone, angleValue, RED_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, leftRedZone, leftYellowZone, angleValue, YELLOW_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, leftYellowZone, rightYellowZone, angleValue, GREEN_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, rightYellowZone, rightRedZone, angleValue, YELLOW_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, rightRedZone, maxValue, angleValue, RED_ZONE_COLOR);
		
		// Draw center part and border
		gc.setBackground(getColorFromPreferences("Chart.Colors.PlotArea")); //$NON-NLS-1$
		gc.setForeground(SharedColors.getColor(SharedColors.DIAL_CHART_SCALE, getDisplay()));
		gc.fillArc(rect.x + scaleInnerOffset, rect.y + scaleInnerOffset, rect.width - scaleInnerOffset * 2, rect.height - scaleInnerOffset * 2, 0, 360);
		gc.setLineWidth(2);
		gc.drawArc(rect.x, rect.y, rect.width, rect.height, 0, 360);
		gc.setLineWidth(1);
		
		// Draw scale
		gc.setForeground(getColorFromPreferences("Chart.Axis.X.Color")); //$NON-NLS-1$
		int textOffset = ((rect.width / 2) * SCALE_OFFSET / 200);
		double arcLength = (outerRadius - scaleOuterOffset) * 4.7123889803846898576939650749193;	// r * (270 degrees angle in radians)
		int step = (arcLength >= 200) ? 27 : 54;
		double valueStep = Math.abs((maxValue - minValue) / ((arcLength >= 200) ? 10 : 20)); 
		int textWidth = (int)(Math.sqrt((outerRadius - scaleOuterOffset) * (outerRadius - scaleOuterOffset) / 2) * 0.7);
		final Font markFont = WidgetHelper.getBestFittingFont(gc, scaleFonts, "900MM", textWidth, outerRadius - scaleOuterOffset); //$NON-NLS-1$
		gc.setFont(markFont);
		for(int i = 225; i >= -45; i -= step)
		{
			if (gridVisible)
			{
				Point l1 = positionOnArc(cx, cy, outerRadius - scaleOuterOffset, i);
				Point l2 = positionOnArc(cx, cy, outerRadius - scaleInnerOffset, i);
				gc.drawLine(l1.x, l1.y, l2.x, l2.y);
			}

			String value = roundedMarkValue(i, angleValue, valueStep);
			Point t = positionOnArc(cx, cy, outerRadius - textOffset, i);
			Point ext = gc.textExtent(value, SWT.DRAW_TRANSPARENT);
			gc.drawText(value, t.x - ext.x / 2, t.y - ext.y / 2, SWT.DRAW_TRANSPARENT);
		}
		gc.drawArc(rect.x + scaleOuterOffset, rect.y + scaleOuterOffset, rect.width - scaleOuterOffset * 2, rect.height - scaleOuterOffset * 2, -45, 270);
		gc.drawArc(rect.x + scaleInnerOffset, rect.y + scaleInnerOffset, rect.width - scaleInnerOffset * 2, rect.height - scaleInnerOffset * 2, -45, 270);
		
		// Draw needle
		gc.setBackground(SharedColors.getColor(SharedColors.DIAL_CHART_NEEDLE, getDisplay()));
		double dciValue = dci.getValue();
		if (dciValue < minValue)
			dciValue = minValue;
		if (dciValue > maxValue)
			dciValue = maxValue;
		int angle = (int)(225 - (dciValue - minValue) / angleValue);
		Point needleEnd = positionOnArc(cx, cy, outerRadius - ((rect.width / 2) * (SCALE_WIDTH / 2) / 100), angle);
		Point np1 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle - 90);
		Point np2 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle + 90);
		gc.fillPolygon(new int[] { np1.x, np1.y, needleEnd.x, needleEnd.y, np2.x, np2.y });
		gc.fillArc(cx - NEEDLE_PIN_RADIUS, cy - NEEDLE_PIN_RADIUS, NEEDLE_PIN_RADIUS * 2 - 1, NEEDLE_PIN_RADIUS * 2 - 1, 0, 360);
		gc.setBackground(SharedColors.getColor(SharedColors.DIAL_CHART_NEEDLE_PIN, getDisplay()));
		gc.fillArc(cx - NEEDLE_PIN_RADIUS / 2, cy - NEEDLE_PIN_RADIUS / 2, NEEDLE_PIN_RADIUS - 1, NEEDLE_PIN_RADIUS - 1, 0, 360);
		
		// Draw current value
		String value = getValueAsDisplayString(dci);
		gc.setFont(WidgetHelper.getMatchingSizeFont(valueFonts, markFont));
		Point ext = gc.textExtent(value, SWT.DRAW_TRANSPARENT);
		gc.setLineWidth(3);
		gc.setBackground(SharedColors.getColor(SharedColors.DIAL_CHART_VALUE_BACKGROUND, getDisplay()));
		int boxW = Math.max(outerRadius - scaleInnerOffset - 6, ext.x + 8);
		gc.fillRoundRectangle(cx - boxW / 2, cy + rect.height / 4, boxW, ext.y + 6, 3, 3);
		gc.setForeground(SharedColors.getColor(SharedColors.DIAL_CHART_VALUE, getDisplay()));
		gc.drawText(value, cx - ext.x / 2, cy + rect.height / 4 + 3, true);
		
		// Draw legend, ignore legend position
		if (legendVisible)
		{
			ext = gc.textExtent(dci.getName(), SWT.DRAW_TRANSPARENT);
			gc.setForeground(SharedColors.getColor(SharedColors.DIAL_CHART_LEGEND, getDisplay()));
			if (legendInside)
			{
				gc.setFont(markFont);
				gc.drawText(dci.getName(), rect.x + ((rect.width - ext.x) / 2), rect.y + scaleInnerOffset / 2 + rect.height / 4, true);
			}
			else
			{
				gc.setFont(null);
				gc.drawText(dci.getName(), rect.x + ((rect.width - ext.x) / 2), rect.y + rect.height + 4, true);
			}
		}
	}
	
	/**
	 * Draw colored zone.
	 * 
	 * @param gc
	 * @param rect
	 * @param startAngle
	 * @param maxValue2
	 * @param leftRedZone2
	 * @param angleValue
	 * @param color color
	 * @return
	 */
	private int drawZone(GC gc, Rectangle rect, int startAngle, double minValue, double maxValue, double angleValue, RGB color)
	{
		if (minValue >= maxValue)
			return startAngle;	// Ignore incorrect zone settings
		
		int angle = (int)((maxValue - minValue) / angleValue);
		if (angle <= 0)
			return startAngle;
		
		int offset = ((rect.width / 2) * SCALE_OFFSET / 100);
		
		gc.setBackground(colors.create(color));
		gc.fillArc(rect.x + offset, rect.y + offset, rect.width - offset * 2, rect.height - offset * 2, startAngle, -angle);
		return startAngle - angle;
	}

	/**
	 * Find point coordinates on arc by given angle and radius. Angles are 
	 * interpreted such that 0 degrees is at the 3 o'clock position.
	 * 
	 * @param cx center point X coordinate
	 * @param cy center point Y coordinate
	 * @param radius radius
	 * @param angle angle
	 * @return point coordinates
	 */
	private Point positionOnArc(int cx, int cy, int radius, int angle)
	{
		return new Point((int)(radius * Math.cos(Math.toRadians(angle)) + cx), (int)(radius * -Math.sin(Math.toRadians(angle)) + cy));
	}
	
	/**
	 * Get rounded value for scale mark
	 * 
	 * @param angle
	 * @param angleValue
	 * @return
	 */
	private String roundedMarkValue(int angle, double angleValue, double step)
	{
		double value = (225 - angle) * angleValue + minValue;
		double absValue = Math.abs(value);
		if (absValue >= 10000000000L)
		{
			return Long.toString(Math.round(value / 1000000000)) + Messages.DialChartWidget_G;
		}
		else if (absValue >= 1000000000)
		{
			return new DecimalFormat("#.#").format(value / 1000000000) + Messages.DialChartWidget_G; //$NON-NLS-1$
		}
		else if (absValue >= 10000000)
		{
			return Long.toString(Math.round(value / 1000000)) + Messages.DialChartWidget_M;
		}
		else if (absValue >= 1000000)
		{
			return new DecimalFormat("#.#").format(value / 1000000) + Messages.DialChartWidget_M; //$NON-NLS-1$
		}
		else if (absValue >= 10000)
		{
			return Long.toString(Math.round(value / 1000)) + Messages.DialChartWidget_K;
		}
		else if (absValue >= 1000)
		{
			return new DecimalFormat("#.#").format(value / 1000) + Messages.DialChartWidget_K; //$NON-NLS-1$
		}
		else if ((absValue >= 1) && (step >= 1))
		{
			return Long.toString(Math.round(value));
		}
		else if (absValue == 0)
		{
			return "0"; //$NON-NLS-1$
		}
		else
		{
			if (step < 0.00001)
				return Double.toString(value);
			if (step < 0.0001)
				return new DecimalFormat("#.#####").format(value); //$NON-NLS-1$
			if (step < 0.001)
				return new DecimalFormat("#.####").format(value); //$NON-NLS-1$
			if (step < 0.01)
				return new DecimalFormat("#.###").format(value); //$NON-NLS-1$
			return new DecimalFormat("#.##").format(value); //$NON-NLS-1$
		}
	}
}
