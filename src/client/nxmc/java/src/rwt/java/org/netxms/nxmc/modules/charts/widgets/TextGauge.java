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
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.nxmc.modules.charts.api.GaugeColorMode;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;

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
    * @see org.netxms.nxmc.modules.charts.widgets.GenericGauge#createRenderData()
    */
   @Override
   protected Object createRenderData()
   {
      return new Font[1];
   }

   /**
    * @see org.netxms.nxmc.modules.charts.widgets.GenericGauge#prepareElementRender(org.eclipse.swt.graphics.GC,
    *      org.netxms.client.datacollection.ChartConfiguration, java.lang.Object, org.netxms.client.datacollection.ChartDciConfig,
    *      org.netxms.nxmc.modules.charts.api.DataSeries, int, int, int, int, int)
    */
   @Override
   protected void prepareElementRender(GC gc, ChartConfiguration configuration, Object renderData, ChartDciConfig dci, DataSeries data, int x, int y, int w, int h, int index)
   {
      if (valueFonts == null)
      {
         ((Font[])renderData)[0] = fixedValueFont;
         return;
      }

      Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, w - INNER_MARGIN_WIDTH * 2, h - INNER_MARGIN_HEIGHT * 2);
      if (configuration.isElementBordersVisible())
      {
         rect.x += INNER_MARGIN_WIDTH;
         rect.y += INNER_MARGIN_HEIGHT;
         rect.width -= INNER_MARGIN_WIDTH * 2;
         rect.height -= INNER_MARGIN_HEIGHT * 2;
      }
      if (configuration.areLabelsVisible())
      {
         rect.height -= gc.textExtent("MMM").y + 8;
      }

      final String value = (configuration.getExpectedTextWidth() > 0) ? createTemplateString(configuration.getExpectedTextWidth()) : getValueAsDisplayString(dci, data);
      final Font font = WidgetHelper.getBestFittingFont(gc, valueFonts, value, rect.width, rect.height);
      if ((((Font[])renderData)[0] == null) || (font.getFontData()[0].getHeight() < ((Font[])renderData)[0].getFontData()[0].getHeight()))
      {
         ((Font[])renderData)[0] = font;
      }
   }

   /**
    * Create template string for font fitting.
    *
    * @param length string length
    * @return template string for font fitting
    */
   private static String createTemplateString(int length)
   {
      char ch[] = new char[length];
      for(int i = 0; i < length; i++)
         ch[i] = 'M';
      return new String(ch);
   }

   /**
    * @see org.netxms.nxmc.modules.charts.widgets.GenericGauge#renderElement(org.eclipse.swt.graphics.GC,
    *      org.netxms.client.datacollection.ChartConfiguration, java.lang.Object, org.netxms.client.datacollection.ChartDciConfig,
    *      org.netxms.nxmc.modules.charts.api.DataSeries, int, int, int, int, int)
    */
	@Override
   protected void renderElement(GC gc, ChartConfiguration config, Object renderData, ChartDciConfig dci, DataSeries data, int x, int y, int w, int h, int index)
	{
		Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, w - INNER_MARGIN_WIDTH * 2, h - INNER_MARGIN_HEIGHT * 2);
		gc.setAntialias(SWT.ON);

      if (config.areLabelsVisible())
		{
         rect.height -= gc.textExtent("MMM").y + 8;
		}

      switch(GaugeColorMode.getByValue(config.getGaugeColorMode()))
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
         default:
            gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_BLACK));
            break;
		}
      final String value = getValueAsDisplayString(dci, data);
      final Font font = ((Font[])renderData)[0];
      gc.setFont(font);
		Point ext = gc.textExtent(value);
		gc.drawText(value, rect.x + rect.width / 2 - ext.x / 2, rect.y + rect.height / 2 - ext.y / 2, SWT.DRAW_TRANSPARENT);

      // Draw label
      if (config.areLabelsVisible())
		{
         gc.setFont(null);
         ext = gc.textExtent(dci.getLabel());
         gc.setForeground(ThemeEngine.getForegroundColor("Chart.Base"));
         gc.drawText(dci.getLabel(), rect.x + ((rect.width - ext.x) / 2), rect.y + rect.height + 4, true);
		}
	}
}
