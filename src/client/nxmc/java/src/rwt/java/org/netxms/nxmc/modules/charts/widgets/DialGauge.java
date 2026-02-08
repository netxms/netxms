/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.charts.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.nxmc.modules.charts.api.GaugeColorMode;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Dial chart implementation
 */
public class DialGauge extends GenericGauge
{
   private static final int NEEDLE_PIN_RADIUS = 8;
   private static final int SCALE_OFFSET = 18; // In percents
   private static final int SCALE_WIDTH = 3; // In percents
   private static final int COLOR_RING_OFFSET = 2; // In percents
   private static final int COLOR_RING_WIDTH = 5; // In percents
   private static final int LABEL_TOP_MARGIN = 6;

	private Font[] scaleFonts = null;
	private Font[] valueFonts = null;

	/**
	 * @param parent
	 * @param style
	 */
   public DialGauge(Chart parent)
	{
      super(parent);
	}

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericGauge#createFonts()
    */
	@Override
	protected void createFonts()
	{
      String fontName = chart.getConfiguration().getFontName();

		scaleFonts = new Font[16];
		for(int i = 0; i < scaleFonts.length; i++)
         scaleFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.NORMAL);

		valueFonts = new Font[16];
		for(int i = 0; i < valueFonts.length; i++)
         valueFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.BOLD);
	}

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericGauge#disposeFonts()
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

   /**
    * @see org.netxms.nxmc.modules.charts.widgets.GenericGauge#renderElement(org.eclipse.swt.graphics.GC,
    *      org.netxms.client.datacollection.ChartConfiguration, java.lang.Object, org.netxms.client.datacollection.ChartDciConfig,
    *      org.netxms.nxmc.modules.charts.api.DataSeries, int, int, int, int, int)
    */
	@Override
   protected void renderElement(GC gc, ChartConfiguration configuration, Object renderData, ChartDciConfig dci, DataSeries data, int x, int y, int w, int h, int index)
	{
      Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, w - INNER_MARGIN_WIDTH * 2, h - INNER_MARGIN_HEIGHT * 2);
      int heightAdjustment = rect.height / 6;
      rect.height += heightAdjustment;

      if (configuration.areLabelsVisible() && !configuration.areLabelsInside())
		{
         rect.height -= gc.textExtent("MMM").y + LABEL_TOP_MARGIN; //$NON-NLS-1$
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

      double maxValue = configuration.getMaxYScaleValue();
      double minValue = configuration.getMinYScaleValue();

      double angleValue = (maxValue - minValue) / 270.0;
		int outerRadius = (rect.width + 1) / 2;
		int scaleOuterOffset = ((rect.width / 2) * SCALE_OFFSET / 100);
		int scaleCenterOffset = ((rect.width / 2) * (SCALE_OFFSET + SCALE_WIDTH) / 100);
      int scaleInnerOffset = ((rect.width / 2) * (SCALE_OFFSET + SCALE_WIDTH + COLOR_RING_OFFSET + COLOR_RING_WIDTH) / 100);

		int cx = rect.x + rect.width / 2 + 1;
		int cy = rect.y + rect.height / 2 + 1;
      gc.setBackground(ThemeEngine.getBackgroundColor("Chart.PlotArea"));
		gc.fillArc(rect.x, rect.y, rect.width, rect.height, 0, 360);

		// Draw zones
      GaugeColorMode colorMode = GaugeColorMode.getByValue(configuration.getGaugeColorMode());
      switch(colorMode)
		{
         case CUSTOM:
            drawZone(gc, rect, 225, minValue, maxValue, angleValue, chart.getPaletteEntry(0).getRGBObject());
            break;
         case DATA_SOURCE:
            drawZone(gc, rect, 225, minValue, maxValue, angleValue, getDataSourceColor(dci, index));
            break;
         case THRESHOLD:
            drawZone(gc, rect, 225, minValue, maxValue, angleValue, StatusDisplayInfo.getStatusColor(data.getActiveThresholdSeverity()).getRGB());
            break;
         case ZONE:
            double startAngle = 225;
            startAngle = drawZone(gc, rect, startAngle, minValue, configuration.getLeftRedZone(), angleValue, RED_ZONE_COLOR);
            startAngle = drawZone(gc, rect, startAngle, configuration.getLeftRedZone(), configuration.getLeftYellowZone(), angleValue, YELLOW_ZONE_COLOR);
            startAngle = drawZone(gc, rect, startAngle, configuration.getLeftYellowZone(), configuration.getRightYellowZone(), angleValue, GREEN_ZONE_COLOR);
            startAngle = drawZone(gc, rect, startAngle, configuration.getRightYellowZone(), configuration.getRightRedZone(), angleValue, YELLOW_ZONE_COLOR);
            startAngle = drawZone(gc, rect, startAngle, configuration.getRightRedZone(), maxValue, angleValue, RED_ZONE_COLOR);
            break;
		   default:
		      break;
		}

      // Draw center part
      gc.setBackground(ThemeEngine.getBackgroundColor("Chart.PlotArea"));
      gc.setForeground(ThemeEngine.getForegroundColor("Chart.PlotArea"));
      gc.fillArc(rect.x + scaleInnerOffset, rect.y + scaleInnerOffset, rect.width - scaleInnerOffset * 2, rect.height - scaleInnerOffset * 2, 0, 360);

		// Draw scale
      Color scaleColor = ThemeEngine.getForegroundColor("Chart.DialScale");
      gc.setForeground(scaleColor);
		int textOffset = ((rect.width / 2) * SCALE_OFFSET / 200);
		double arcLength = (outerRadius - scaleOuterOffset) * 4.7123889803846898576939650749193;	// r * (270 degrees angle in radians)
		int step = (arcLength >= 200) ? 27 : 54;
		double valueStep = Math.abs((maxValue - minValue) / ((arcLength >= 200) ? 10 : 20)); 
		int textWidth = (int)(Math.sqrt((outerRadius - scaleOuterOffset) * (outerRadius - scaleOuterOffset) / 2) * 0.7);
		final Font markFont = WidgetHelper.getBestFittingFont(gc, scaleFonts, "900MM", textWidth, outerRadius - scaleOuterOffset); //$NON-NLS-1$
		gc.setFont(markFont);
      gc.setLineWidth(1);
		for(int i = 225; i >= -45; i -= step)
		{
         if (configuration.isGridVisible())
			{
	         gc.setForeground(scaleColor);
				Point l1 = positionOnArc(cx, cy, outerRadius - scaleOuterOffset, i);
				Point l2 = positionOnArc(cx, cy, outerRadius - scaleCenterOffset, i);
				gc.drawLine(l1.x, l1.y, l2.x, l2.y);
			}
			
	      double angle = (225 - i) * angleValue + minValue;
			String value = DataFormatter.roundDecimalValue(angle, valueStep, 5);
			Point t = positionOnArc(cx, cy, outerRadius - textOffset, i);
			Point ext = gc.textExtent(value);
			gc.drawText(value, t.x - ext.x / 2, t.y - ext.y / 2, SWT.DRAW_TRANSPARENT);
		}
		gc.drawArc(rect.x + scaleCenterOffset, rect.y + scaleCenterOffset, rect.width - scaleCenterOffset * 2, rect.height - scaleCenterOffset * 2, -45, 270);

      // Draw current value
      double dciValue = data.getCurrentValue();
      if (dciValue < minValue)
         dciValue = minValue;
      if (dciValue > maxValue)
         dciValue = maxValue;
      gc.setFont(WidgetHelper.getBestFittingFont(gc, valueFonts, Integer.toString((int)maxValue) + ".00", outerRadius - scaleInnerOffset - 6, rect.height / 8));
      switch(colorMode)
      {
         case CUSTOM:
            gc.setForeground(chart.getColorCache().create(chart.getPaletteEntry(0).getRGBObject()));
            break;
         case DATA_SOURCE:
            gc.setForeground(chart.getColorCache().create(getDataSourceColor(dci, index)));
            break;
         case THRESHOLD:
            gc.setForeground(StatusDisplayInfo.getStatusColor(data.getActiveThresholdSeverity()));
            break;
         case ZONE:
            if ((dciValue <= configuration.getLeftRedZone()) || (dciValue >= configuration.getRightRedZone()))
               gc.setForeground(chart.getColorCache().create(RED_ZONE_COLOR));
            else if ((dciValue <= configuration.getLeftYellowZone()) || (dciValue >= configuration.getRightYellowZone()))
               gc.setForeground(chart.getColorCache().create(YELLOW_ZONE_COLOR));
            else
               gc.setForeground(chart.getColorCache().create(GREEN_ZONE_COLOR));
            break;
         default:
            gc.setForeground(scaleColor);
            break;
      }
      String value = getValueAsDisplayString(dci, data);
      Point ext = gc.textExtent(value);
      gc.drawText(value, cx - ext.x / 2, cy + rect.height / 4 + 3, true);

		// Draw needle
      gc.setBackground(gc.getForeground()); // Use same color as for current value
		int angle = (int)(225 - (dciValue - minValue) / angleValue);
      Point needleEnd = positionOnArc(cx, cy, outerRadius - scaleInnerOffset, angle);
      Point np1 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle - 90);
      Point np2 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle + 90);
		gc.fillPolygon(new int[] { np1.x, np1.y, needleEnd.x, needleEnd.y, np2.x, np2.y });
      gc.setBackground(ThemeEngine.getBackgroundColor("Chart.DialNeedlePin"));
		gc.fillArc(cx - NEEDLE_PIN_RADIUS, cy - NEEDLE_PIN_RADIUS, NEEDLE_PIN_RADIUS * 2 - 1, NEEDLE_PIN_RADIUS * 2 - 1, 0, 360);
      gc.setBackground(ThemeEngine.getBackgroundColor("Chart.PlotArea"));
      gc.fillArc(cx - NEEDLE_PIN_RADIUS / 2, cy - NEEDLE_PIN_RADIUS / 2, NEEDLE_PIN_RADIUS - 1, NEEDLE_PIN_RADIUS - 1, 0, 360);

      // Draw labels
      if (configuration.areLabelsVisible())
		{
         gc.setFont(configuration.areLabelsInside() ? WidgetHelper.getBestFittingFont(gc, scaleFonts, "XXXXXXXXXXXXXXXXXXXXXXXX", rect.width - scaleInnerOffset * 2 - 6, rect.height / 8) : null);
         ext = gc.textExtent(dci.getLabel());
         gc.setForeground(ThemeEngine.getForegroundColor("Chart.Base"));
         if (configuration.areLabelsInside())
			{
            gc.drawText(dci.getLabel(), rect.x + ((rect.width - ext.x) / 2), rect.y + scaleCenterOffset / 2 + rect.height / 4, true);
			}
			else
			{
            gc.drawText(dci.getLabel(), rect.x + ((rect.width - ext.x) / 2), rect.y + rect.height + LABEL_TOP_MARGIN - heightAdjustment, true);
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
   private double drawZone(GC gc, Rectangle rect, double startAngle, double minValue, double maxValue, double angleValue, RGB color)
	{
		if (minValue >= maxValue)
			return startAngle;	// Ignore incorrect zone settings

      double angle = (maxValue - minValue) / angleValue;
		if (angle <= 0)
			return startAngle;

      int offset = ((rect.width / 2) * (SCALE_OFFSET + SCALE_WIDTH + COLOR_RING_OFFSET) / 100);

      gc.setBackground(chart.getColorCache().create(color));
      gc.fillArc(rect.x + offset, rect.y + offset, rect.width - offset * 2, rect.height - offset * 2, (int)Math.ceil(startAngle), (int)-Math.ceil(angle));
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
    * Get minimal element size
    * 
    * @return
    */
	@Override
   protected Point getMinElementSize()
   {
      return new Point(40, 40);
   }
}
