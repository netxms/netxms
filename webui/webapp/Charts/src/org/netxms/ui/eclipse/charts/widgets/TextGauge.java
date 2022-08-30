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
package org.netxms.ui.eclipse.charts.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.DataSeries;
import org.netxms.ui.eclipse.charts.api.GaugeColorMode;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Current value pseudo-chart implementation
 */
public class TextGauge extends GenericGauge
{
   private Font fixedValueFont = null;
	private Font[] valueFonts = null;
	
	/**
	 * @param parent
	 * @param style
	 */
   public TextGauge(Chart parent)
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
      int fontSize = chart.getConfiguration().getFontSize();
      if (fontSize > 0)
      {
         fixedValueFont = new Font(getDisplay(), fontName, fontSize, SWT.BOLD);
      }
      else
      {
         valueFonts = new Font[32];
         for(int i = 0; i < valueFonts.length; i++)
            valueFonts[i] = new Font(getDisplay(), fontName, i * 6 + 12, SWT.BOLD);
      }
	}

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericGauge#disposeFonts()
    */
	@Override
	protected void disposeFonts()
	{
      if (fixedValueFont != null)
         fixedValueFont.dispose();

		if (valueFonts != null)
		{
			for(int i = 0; i < valueFonts.length; i++)
				valueFonts[i].dispose();
		}
	}

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericGauge#renderElement(org.eclipse.swt.graphics.GC,
    *      org.netxms.client.datacollection.ChartConfiguration, org.netxms.client.datacollection.GraphItem,
    *      org.netxms.ui.eclipse.charts.api.DataSeries, int, int, int, int)
    */
	@Override
   protected void renderElement(GC gc, ChartConfiguration config, GraphItem dci, DataSeries data, int x, int y, int w, int h)
	{
		Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, w - INNER_MARGIN_WIDTH * 2, h - INNER_MARGIN_HEIGHT * 2);
		gc.setAntialias(SWT.ON);

      if (config.isElementBordersVisible())
		{
         gc.setForeground(getColorFromPreferences("Chart.Axis.Y.Color")); //$NON-NLS-1$
		   gc.drawRectangle(rect);
		   rect.x += INNER_MARGIN_WIDTH;
		   rect.y += INNER_MARGIN_HEIGHT;
		   rect.width -= INNER_MARGIN_WIDTH * 2;
         rect.height -= INNER_MARGIN_HEIGHT * 2;
		}

      if (config.areLabelsVisible())
		{
			rect.height -= gc.textExtent("MMM").y + 8; //$NON-NLS-1$
		}

      final String value = getValueAsDisplayString(dci, data);
      final Font font = (valueFonts != null) ? WidgetHelper.getBestFittingFont(gc, valueFonts, value, rect.width, rect.height) : fixedValueFont;
		gc.setFont(font);
      switch(GaugeColorMode.getByValue(config.getGaugeColorMode()))
		{
		   case ZONE:
            if ((data.getCurrentValue() <= config.getLeftRedZone()) || (data.getCurrentValue() >= config.getRightRedZone()))
      		{
               gc.setForeground(chart.getColorCache().create(RED_ZONE_COLOR));
      		}
            else if ((data.getCurrentValue() <= config.getLeftYellowZone()) || (data.getCurrentValue() >= config.getRightYellowZone()))
      		{
               gc.setForeground(chart.getColorCache().create(YELLOW_ZONE_COLOR));
      		}
      		else
      		{
               gc.setForeground(chart.getColorCache().create(GREEN_ZONE_COLOR));
      		}
      		break;
		   case CUSTOM:
            gc.setForeground(chart.getColorCache().create(chart.getPaletteEntry(0).getRGBObject()));
   		   break;
		   case THRESHOLD:
            gc.setForeground(StatusDisplayInfo.getStatusColor(data.getActiveThresholdSeverity()));
		      break;
         default:
            gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_BLACK));
            break;
		}
		Point ext = gc.textExtent(value);
		gc.drawText(value, rect.x + rect.width / 2 - ext.x / 2, rect.y + rect.height / 2 - ext.y / 2, SWT.DRAW_TRANSPARENT);

      // Draw label
      if (config.areLabelsVisible())
		{
         gc.setFont(null);
         ext = gc.textExtent(dci.getDescription());
			gc.setForeground(getColorFromPreferences("Chart.Colors.Legend")); //$NON-NLS-1$
         gc.drawText(dci.getDescription(), rect.x + ((rect.width - ext.x) / 2), rect.y + rect.height + 4, true);
		}
	}
}
