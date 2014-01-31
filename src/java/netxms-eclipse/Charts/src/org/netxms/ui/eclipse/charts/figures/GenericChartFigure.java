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
package org.netxms.ui.eclipse.charts.figures;

import org.eclipse.draw2d.Figure;
import org.eclipse.jface.preference.IPreferenceStore;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.Messages;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.DataChart;

/**
 * Abstract base class for chart figures
 */
public abstract class GenericChartFigure extends Figure implements DataChart
{
	protected String title = Messages.get().GenericChart_Title0;
	protected boolean titleVisible = false;
	protected boolean legendVisible = true;
	protected boolean displayIn3D = true;
	protected boolean useLogScale = false;
	protected boolean translucent = false;
	protected ChartColor[] palette = null;
	protected IPreferenceStore preferenceStore;
	protected int legendPosition;

	/**
	 * Generic constructor.
	 */
	public GenericChartFigure()
	{
		super();
		preferenceStore = Activator.getDefault().getPreferenceStore();
		createDefaultPalette();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#getTitle()
	 */
	@Override
	public String getChartTitle()
	{
		return title;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isLegendVisible()
	 */
	@Override
	public boolean isLegendVisible()
	{
		return legendVisible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isTitleVisible()
	 */
	@Override
	public boolean isTitleVisible()
	{
		return titleVisible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPalette(org.netxms.ui.eclipse.charts.api.ChartColor[])
	 */
	@Override
	public void setPalette(ChartColor[] colors)
	{
		palette = colors;
	}

	/* (non-Javadoc)
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

	/* (non-Javadoc)
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

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#is3DModeEnabled()
	 */
	@Override
	public boolean is3DModeEnabled()
	{
		return displayIn3D;
	}

	/* (non-Javadoc)
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

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendPosition(int)
	 */
	@Override
	public void setLegendPosition(int position)
	{
		legendPosition = position;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#getLegendPosition()
	 */
	@Override
	public int getLegendPosition()
	{
		return legendPosition;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isTranslucent()
	 */
	@Override
	public boolean isTranslucent()
	{
		return translucent;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTranslucent(boolean)
	 */
	@Override
	public void setTranslucent(boolean translucent)
	{
		this.translucent = translucent;
	}
	
	/**
	 * Dispose figure
	 */
	public void dispose()
	{
	}
}
