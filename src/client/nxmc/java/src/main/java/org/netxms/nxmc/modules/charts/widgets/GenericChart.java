/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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

import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.DataChart;
import org.xnap.commons.i18n.I18n;

/**
 * Abstract base class for charts
 */
public abstract class GenericChart extends Canvas implements DataChart
{
   private final I18n i18n = LocalizationHelper.getI18n(GenericChart.class);

   protected String title = i18n.tr("Chart");
	protected boolean titleVisible = false;
	protected boolean legendVisible = true;
	protected boolean displayIn3D = true;
	protected boolean useLogScale = false;
	protected boolean translucent = false;
	protected ChartColor[] palette = null;
   protected PreferenceStore preferenceStore;
	protected int legendPosition;

	/**
	 * Generic constructor.
	 * 
	 * @param parent parent composite
	 * @param style SWT control styles
	 */
	public GenericChart(Composite parent, int style)
	{
		super(parent, style);

      preferenceStore = PreferenceStore.getInstance();
		createDefaultPalette();
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#getTitle()
    */
	@Override
	public String getChartTitle()
	{
		return title;
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#isLegendVisible()
    */
	@Override
	public boolean isLegendVisible()
	{
		return legendVisible;
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#isTitleVisible()
    */
	@Override
	public boolean isTitleVisible()
	{
		return titleVisible;
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#setPalette(org.netxms.ui.eclipse.charts.api.ChartColor[])
    */
	@Override
	public void setPalette(ChartColor[] colors)
	{
		palette = colors;
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#setPaletteEntry(int, org.netxms.ui.eclipse.charts.api.ChartColor)
    */
	@Override
	public void setPaletteEntry(int index, ChartColor color)
	{
		try
		{
			palette[index] = color;
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
		}
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#getPaletteEntry(int)
    */
	@Override
	public ChartColor getPaletteEntry(int index)
	{
		try
		{
			return palette[index];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return null;
		}
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#is3DModeEnabled()
    */
	@Override
	public boolean is3DModeEnabled()
	{
		return displayIn3D;
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#isLogScaleEnabled()
    */
	@Override
	public boolean isLogScaleEnabled()
	{
		return useLogScale;
	}

	/**
	 * Create default palette from preferences
	 */
	protected void createDefaultPalette()
	{
		palette = new ChartColor[MAX_CHART_ITEMS];
		for(int i = 0; i < MAX_CHART_ITEMS; i++)
		{
			palette[i] = ChartColor.getDefaultColor(i);
		}
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendPosition(int)
    */
	@Override
	public void setLegendPosition(int position)
	{
		legendPosition = position;
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#getLegendPosition()
    */
	@Override
	public int getLegendPosition()
	{
		return legendPosition;
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#isTranslucent()
    */
	@Override
	public boolean isTranslucent()
	{
		return translucent;
	}

   /**
    * @see org.netxms.ui.eclipse.charts.api.DataChart#setTranslucent(boolean)
    */
	@Override
	public void setTranslucent(boolean translucent)
	{
		this.translucent = translucent;
	}
}
