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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.Gauge;
import org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement;
import org.netxms.ui.eclipse.tools.ColorCache;

/**
 * Abstract gauge widget
 */
public abstract class GaugeWidget extends GenericChart implements Gauge, PaintListener, DisposeListener
{
	protected static final int OUTER_MARGIN_WIDTH = 5;
	protected static final int OUTER_MARGIN_HEIGHT = 5;
	protected static final int INNER_MARGIN_WIDTH = 5;
	protected static final int INNER_MARGIN_HEIGHT = 5;
	
	protected static final RGB GREEN_ZONE_COLOR = new RGB(0, 224, 0);
	protected static final RGB YELLOW_ZONE_COLOR = new RGB(255, 242, 0);
	protected static final RGB RED_ZONE_COLOR = new RGB(224, 0, 0);
	
	protected List<DataComparisonElement> parameters = new ArrayList<DataComparisonElement>(MAX_CHART_ITEMS);
	protected Image chartImage = null;
	protected ColorCache colors;
	protected double minValue = 0.0;
	protected double maxValue = 100.0;
	protected double leftRedZone = 0.0;
	protected double leftYellowZone = 0.0;
	protected double rightYellowZone = 70.0;
	protected double rightRedZone = 90.0;
	protected boolean vertical = false;
	protected boolean legendInside = true;
	protected boolean gridVisible = true;
	protected boolean elementBordersVisible = false;
	protected String fontName = "Verdana"; //$NON-NLS-1$
	
	private boolean fontsCreated = false;
	
	/**
	 * @param parent
	 * @param style
	 */
	public GaugeWidget(Composite parent, int style)
	{
		super(parent, style | SWT.NO_BACKGROUND);
		
		colors = new ColorCache(this);
		addPaintListener(this);
		addDisposeListener(this);
		addControlListener(new ControlListener() {
			@Override
			public void controlResized(ControlEvent e)
			{
				if (chartImage != null)
				{
					chartImage.dispose();
					chartImage = null;
				}
				refresh();
			}
			
			@Override
			public void controlMoved(ControlEvent e)
			{
			}
		});
	}
	
	/**
	 * Create fonts
	 */
	protected abstract void createFonts();
	
	/**
	 * Dispose fonts
	 */
	protected abstract void disposeFonts();

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#initializationComplete()
	 */
	@Override
	public void initializationComplete()
	{
	}

   /**
	 * Create color object from preference string
	 *  
	 * @param name Preference name
	 * @return Color object
	 */
	protected Color getColorFromPreferences(final String name)
	{
		return colors.create(PreferenceConverter.getColor(preferenceStore, name));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setChartTitle(java.lang.String)
	 */
	@Override
	public void setChartTitle(String title)
	{
		this.title = title;
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTitleVisible(boolean)
	 */
	@Override
	public void setTitleVisible(boolean visible)
	{
		titleVisible = visible;
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendVisible(boolean)
	 */
	@Override
	public void setLegendVisible(boolean visible)
	{
		legendVisible = visible;
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#set3DModeEnabled(boolean)
	 */
	@Override
	public void set3DModeEnabled(boolean enabled)
	{
		displayIn3D = enabled;
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLogScaleEnabled(boolean)
	 */
	@Override
	public void setLogScaleEnabled(boolean enabled)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#addParameter(org.netxms.client.datacollection.GraphItem, double)
	 */
	@Override
	public int addParameter(GraphItem dci, double value)
	{
      parameters.add(new DataComparisonElement(dci, value, null));
		return parameters.size() - 1;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#updateParameter(int, double, boolean)
	 */
	@Override
	public void updateParameter(int index, double value, boolean updateChart)
	{
		try
		{
			parameters.get(index).setValue(value);
		}
		catch(IndexOutOfBoundsException e)
		{
		}

 		if (updateChart)
			refresh();
	}

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#updateParameter(int, org.netxms.client.datacollection.DciDataRow,
    * int, boolean)
    */
   @Override
   public void updateParameter(int index, DciDataRow value, int dataType, boolean updateChart)
   {
      try
      {
         DataComparisonElement p = parameters.get(index);
         p.setValue(value.getValueAsDouble());
         p.setRawValue(value.getValueAsString());
         p.getObject().setDataType(dataType);
      }
      catch(IndexOutOfBoundsException e)
      {
      }

      if (updateChart)
         refresh();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#updateParameterThresholds(int,
    * org.netxms.client.datacollection.Threshold[])
	 */
	@Override
	public void updateParameterThresholds(int index, Threshold[] thresholds)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#setChartType(int)
	 */
	@Override
	public void setChartType(int chartType)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#getChartType()
	 */
	@Override
	public int getChartType()
	{
		return GAUGE_CHART;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#setTransposed(boolean)
	 */
	@Override
	public void setTransposed(boolean transposed)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#isTransposed()
	 */
	@Override
	public boolean isTransposed()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#setLabelsVisible(boolean)
	 */
	@Override
	public void setLabelsVisible(boolean visible)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#isLabelsVisible()
	 */
	@Override
	public boolean isLabelsVisible()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#setRotation(double)
	 */
	@Override
	public void setRotation(double angle)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#getRotation()
	 */
	@Override
	public double getRotation()
	{
		return 0;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#refresh()
	 */
	@Override
	public void refresh()
	{
		redraw();
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#rebuild()
	 */
	@Override
	public void rebuild()
	{
		redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#hasAxes()
	 */
	@Override
	public boolean hasAxes()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		render(e.gc);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
	 */
	@Override
	public void widgetDisposed(DisposeEvent e)
	{
		if (chartImage != null)
			chartImage.dispose();
		disposeFonts();
	}
	
	/**
	 * Render chart
	 */
	private void render(GC gc)
	{
		if (!fontsCreated)
		{
			createFonts();
			fontsCreated = true;
		}
		
		Point size = getSize();
		if (chartImage == null)
		{
			if ((size.x <= 0) || (size.y <= 0))
				return;
			chartImage = new Image(getDisplay(), size.x, size.y);
		}
		
		gc.setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$
		gc.fillRectangle(0, 0, size.x, size.y);
		gc.setAntialias(SWT.ON);
		gc.setTextAntialias(SWT.ON);
		
		int top = OUTER_MARGIN_HEIGHT;
		
		// Draw title
		if (titleVisible && (title != null))
		{
			gc.setFont(Activator.getDefault().getChartTitleFont(getDisplay()));
			gc.setForeground(getColorFromPreferences("Chart.Colors.Title")); //$NON-NLS-1$
			Point ext = gc.textExtent(title);
			int x = (ext.x < size.x) ? (size.x - ext.x) / 2 : 0;
			gc.drawText(title, x, top, true);
			top += ext.y + INNER_MARGIN_HEIGHT;
		}
		
		if ((parameters.size() == 0) || (size.x < OUTER_MARGIN_WIDTH * 2) || (size.y < OUTER_MARGIN_HEIGHT * 2))
		{
			gc.dispose();
			return;
		}
		
		if (vertical)
		{
			int w = (size.x - OUTER_MARGIN_WIDTH * 2);
			int h = (size.y - OUTER_MARGIN_HEIGHT - top) / parameters.size();
			if ((w > 40) && (h > 40 * parameters.size()))
			{
				for(int i = 0; i < parameters.size(); i++)
				{
					renderElement(gc, parameters.get(i), 0, top + i * h, w, h);
				}
			}
		}
		else
		{
			int w = (size.x - OUTER_MARGIN_WIDTH * 2) / parameters.size();
			int h = size.y - OUTER_MARGIN_HEIGHT - top;
			if ((w > 40 * parameters.size()) && (h > 40))
			{
				for(int i = 0; i < parameters.size(); i++)
				{
					renderElement(gc, parameters.get(i), i * w, top, w, h);
				}
			}
		}
		
		gc.dispose();
	}

	/**
	 * @param dataComparisonElement
	 * @param i
	 * @param w
	 */
	protected abstract void renderElement(GC gc, DataComparisonElement dci, int x, int y, int w, int h);

	/**
	 * @param dci
	 * @return
	 */
	protected String getValueAsDisplayString(DataComparisonElement dci)
	{
		switch(dci.getObject().getDataType())
		{
			case DataCollectionItem.DT_INT:
				return Integer.toString((int)dci.getValue());
			case DataCollectionItem.DT_UINT:
			case DataCollectionItem.DT_INT64:
			case DataCollectionItem.DT_UINT64:
				return Long.toString((long)dci.getValue());
         	case DataCollectionItem.DT_STRING:
            	return dci.getRawValue();
			default:
				return Double.toString(dci.getValue());
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getMinValue()
	 */
	@Override
	public double getMinValue()
	{
		return minValue;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setMinValue(double)
	 */
	@Override
	public void setMinValue(double minValue)
	{
		this.minValue = minValue;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getMaxValue()
	 */
	@Override
	public double getMaxValue()
	{
		return maxValue;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setMaxValue(double)
	 */
	@Override
	public void setMaxValue(double maxValue)
	{
		this.maxValue = maxValue;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getLeftRedZone()
	 */
	@Override
	public double getLeftRedZone()
	{
		return leftRedZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setLeftRedZone(double)
	 */
	@Override
	public void setLeftRedZone(double leftRedZone)
	{
		this.leftRedZone = leftRedZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getLeftYellowZone()
	 */
	@Override
	public double getLeftYellowZone()
	{
		return leftYellowZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setLeftYellowZone(double)
	 */
	@Override
	public void setLeftYellowZone(double leftYellowZone)
	{
		this.leftYellowZone = leftYellowZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getRightYellowZone()
	 */
	@Override
	public double getRightYellowZone()
	{
		return rightYellowZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setRightYellowZone(double)
	 */
	@Override
	public void setRightYellowZone(double rightYellowZone)
	{
		this.rightYellowZone = rightYellowZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getRightRedZone()
	 */
	@Override
	public double getRightRedZone()
	{
		return rightRedZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setRightRedZone(double)
	 */
	@Override
	public void setRightRedZone(double rightRedZone)
	{
		this.rightRedZone = rightRedZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#isLegendInside()
	 */
	@Override
	public boolean isLegendInside()
	{
		return legendInside;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setLegendInside(boolean)
	 */
	@Override
	public void setLegendInside(boolean legendInside)
	{
		this.legendInside = legendInside;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isGridVisible()
	 */
	@Override
	public boolean isGridVisible()
	{
		return gridVisible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setGridVisible(boolean)
	 */
	@Override
	public void setGridVisible(boolean visible)
	{
		gridVisible = visible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setBackgroundColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setBackgroundColor(ChartColor color)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPlotAreaColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setPlotAreaColor(ChartColor color)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendColor(org.netxms.ui.eclipse.charts.api.ChartColor, org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setLegendColor(ChartColor foreground, ChartColor background)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setAxisColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setAxisColor(ChartColor color)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setGridColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setGridColor(ChartColor color)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#addError(java.lang.String)
	 */
	@Override
	public void addError(String message)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#clearErrors()
	 */
	@Override
	public void clearErrors()
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#isVertical()
	 */
	@Override
	public boolean isVertical()
	{
		return vertical;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setVertical(boolean)
	 */
	@Override
	public void setVertical(boolean vertical)
	{
		this.vertical = vertical;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.Gauge#isElementBordersVisible()
	 */
	@Override
   public boolean isElementBordersVisible()
   {
      return elementBordersVisible;
   }

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.Gauge#setElementBordersVisible(boolean)
	 */
	@Override
   public void setElementBordersVisible(boolean elementBordersVisible)
   {
      this.elementBordersVisible = elementBordersVisible;
   }

   /* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.Gauge#getFontName()
	 */
	@Override
	public String getFontName()
	{
		return fontName;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.Gauge#setFontName(java.lang.String)
	 */
	@Override
	public void setFontName(String fontName)
	{
		if ((fontName == null) || fontName.isEmpty())
			fontName = "Verdana"; //$NON-NLS-1$
		if (!fontName.equals(this.fontName))
		{
			this.fontName = fontName;
			if (fontsCreated)
				disposeFonts();
			createFonts();
			fontsCreated = true;
		}
	}
	
	  @Override
	   public void setYAxisRange(int from, int to)
	   {
	   }
}
