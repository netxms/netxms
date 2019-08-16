/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.Messages;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.DataPoint;
import org.netxms.ui.eclipse.charts.api.HistoricalDataChart;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.IAxisSet;
import org.swtchart.IAxisTick;
import org.swtchart.ICustomPaintListener;
import org.swtchart.ILegend;
import org.swtchart.ILineSeries;
import org.swtchart.ILineSeries.PlotSymbolType;
import org.swtchart.IPlotArea;
import org.swtchart.ISeries;
import org.swtchart.ISeries.SeriesType;
import org.swtchart.ISeriesSet;
import org.swtchart.ITitle;
import org.swtchart.LineStyle;
import org.swtchart.Range;

/**
 * Line chart widget
 */
public class LineChart extends Chart implements HistoricalDataChart
{
	//private static final int MAX_ZOOM_LEVEL = 16;
	
	private List<GraphItem> items = new ArrayList<GraphItem>();
	private List<GraphItemStyle> itemStyles = new ArrayList<GraphItemStyle>(GraphSettings.MAX_GRAPH_ITEM_COUNT);
	private long timeFrom;
	private long timeTo;
	private boolean showToolTips;
	private boolean zoomEnabled;
	private boolean gridVisible;
	private boolean stacked;
	//private boolean selectionActive = false;
	private boolean adjustYAxis = true;
	private boolean modifyYBase = false;
	private int lineWidth = 2;
	//private int zoomLevel = 0;
	private int legendPosition = GraphSettings.POSITION_BOTTOM;
	//private MouseMoveListener moveListener;
	private IPreferenceStore preferenceStore;
	private MouseListener zoomMouseListener = null;
	private PaintListener zoomPaintListener = null;
	private ColorCache colors;
	private Set<String> errors = new HashSet<String>(0);
	private Image errorImage = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public LineChart(Composite parent, int style)
	{
		super(parent, style);
		
		colors = new ColorCache(this);
		
		preferenceStore = Activator.getDefault().getPreferenceStore();
		showToolTips = preferenceStore.getBoolean("Chart.ShowToolTips"); //$NON-NLS-1$
		zoomEnabled = preferenceStore.getBoolean("Chart.EnableZoom"); //$NON-NLS-1$
		setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$
		//selection.setColor(getColorFromPreferences("Chart.Colors.Selection")); //$NON-NLS-1$

		// Create default item styles
		IPreferenceStore preferenceStore = Activator.getDefault().getPreferenceStore();
		for(int i = 0; i < GraphSettings.MAX_GRAPH_ITEM_COUNT; i++)
			itemStyles.add(new GraphItemStyle(GraphItemStyle.LINE, ColorConverter.getColorFromPreferencesAsInt(preferenceStore, "Chart.Colors.Data." + i), 0, 0)); //$NON-NLS-1$
		
		// Setup title
		ITitle title = getTitle();
		title.setVisible(preferenceStore.getBoolean("Chart.ShowTitle")); //$NON-NLS-1$
		title.setForeground(getColorFromPreferences("Chart.Colors.Title")); //$NON-NLS-1$
		title.setFont(Activator.getDefault().getChartTitleFont());
		
		// Setup legend
		ILegend legend = getLegend();
		legend.setPosition(swtPositionFromInternal(legendPosition));
		legend.setFont(Activator.getDefault().getChartFont());
		legend.setForeground(getColorFromPreferences("Chart.Colors.Legend")); //$NON-NLS-1$
		
		// Default time range
		timeTo = System.currentTimeMillis();
		timeFrom = timeTo - 3600000;

		// Setup X and Y axis
		IAxisSet axisSet = getAxisSet();
		final IAxis xAxis = axisSet.getXAxis(0);
		xAxis.getTitle().setVisible(false);
		xAxis.setRange(new Range(timeFrom, timeTo));
		IAxisTick xTick = xAxis.getTick();
		xTick.setForeground(getColorFromPreferences("Chart.Axis.X.Color")); //$NON-NLS-1$
		DateFormat format = new SimpleDateFormat(Messages.get().LineChart_ShortTimeFormat);
		xTick.setFormat(format);
		xTick.setFont(Activator.getDefault().getChartFont());
		
		final IAxis yAxis = axisSet.getYAxis(0);
		yAxis.getTitle().setVisible(false);
		yAxis.getTick().setForeground(getColorFromPreferences("Chart.Axis.Y.Color")); //$NON-NLS-1$
		yAxis.getTick().setFont(Activator.getDefault().getChartFont());
		
		// Setup grid
		xAxis.getGrid().setStyle(getLineStyleFromPreferences("Chart.Grid.X.Style")); //$NON-NLS-1$
		xAxis.getGrid().setForeground(getColorFromPreferences("Chart.Grid.X.Color")); //$NON-NLS-1$
		yAxis.getGrid().setStyle(getLineStyleFromPreferences("Chart.Grid.Y.Style")); //$NON-NLS-1$
		yAxis.getGrid().setForeground(getColorFromPreferences("Chart.Grid.Y.Color")); //$NON-NLS-1$
		
		// Setup plot area
		setBackgroundInPlotArea(getColorFromPreferences("Chart.Colors.PlotArea")); //$NON-NLS-1$
		final Composite plotArea = getPlotArea();
		if (showToolTips)
		{
			/*
			plotArea.addMouseTrackListener(new MouseTrackListener() {
				@Override
				public void mouseEnter(MouseEvent e)
				{
				}
	
				@Override
				public void mouseExit(MouseEvent e)
				{
				   if (tooltipShown)
				   {
   				   getPlotArea().setToolTipText(null);
   				   tooltipShown = false;
				   }
				}
	
				@Override
				public void mouseHover(MouseEvent e)
				{
				   ISeries series = getSeriesAtPoint(e.x, e.y);
				   if (series != null)
				   {
   					Date timestamp = new Date((long)xAxis.getDataCoordinate(e.x));
   					double value = yAxis.getDataCoordinate(e.y);
   					getPlotArea().setToolTipText(
   					      series.getName() + "\n" + //$NON-NLS-1$
   					      RegionalSettings.getDateTimeFormat().format(timestamp) + "\n" + //$NON-NLS-1$ 
   					      DataFormatter.roundDecimalValue(value, cachedTickStep, 5));
   					tooltipShown = true;
				   }
				}
			});
			*/
		}
		
		zoomMouseListener = new MouseListener() {
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
				if (e.button == 1)
					startSelection(e);
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
				if (e.button == 1)
					endSelection();
			}
		};
		
		zoomPaintListener = new PaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				//if (selectionActive)
				//	selection.draw(e.gc);
			}
		};
		
		setZoomEnabled(zoomEnabled);
		
		((IPlotArea)plotArea).addCustomPaintListener(new ICustomPaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				paintThresholds(e, yAxis);
				paintErrorIndicator(e.gc);
			}

			@Override
			public boolean drawBehindSeries()
			{
				return true;
			}
		});
		
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				if (errorImage != null)
					errorImage.dispose();
			}
		});
	}
	
	/**
	 * Selection start handler
	 * @param e
	 */
	private void startSelection(MouseEvent e)
	{
      /*
		if (zoomLevel >= MAX_ZOOM_LEVEL)
			return;
		
		selectionActive = true;
		selection.setStartPoint(e.x, e.y);
		selection.setEndPoint(e.x, e.y);
		
		final Composite plotArea = getPlotArea();
		moveListener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e)
			{
				selection.setEndPoint(e.x, e.y);
				plotArea.redraw();
			}
		};
		plotArea.addMouseMoveListener(moveListener);
      */
	}
	
	/**
	 * Selection end handler
	 */
	private void endSelection()
	{
      /*
		if (!selectionActive)
			return;
		
		selectionActive = false;
		final Composite plotArea = getPlotArea();
		plotArea.removeMouseMoveListener(moveListener);

		if (selection.isUsableSize())
		{
			for(IAxis axis : getAxisSet().getAxes())
			{
				Point range = null;
				if ((getOrientation() == SWT.HORIZONTAL && axis.getDirection() == Direction.X) ||
				    (getOrientation() == SWT.VERTICAL && axis.getDirection() == Direction.Y))
				{
					range = selection.getHorizontalRange();
				} 
				else 
				{
					range = selection.getVerticalRange();
				}
	
				if (range != null && range.x != range.y)
				{
					setRange(range, axis);
				}
			}
		}
      
		selection.dispose();
		redraw();
      */
	}
	
   /**
    * Sets the axis range.
    * 
    * @param range
    *            the axis range in pixels
    * @param axis
    *            the axis to set range
    */
	/*
	private void setRange(Point range, IAxis axis)
	{
		double min = axis.getDataCoordinate(range.x);
		double max = axis.getDataCoordinate(range.y);

		axis.setRange(new Range(min, max));
	}
	*/

   /**
	 * Create color object from preference string
	 *  
	 * @param name Preference name
	 * @return Color object
	 */
	private Color getColorFromPreferences(final String name)
	{
		return colors.create(PreferenceConverter.getColor(preferenceStore, name));
	}
	
	/**
	 * Return line style object matching given label. If no match found, LineStyle.NONE is returned.
	 * 
	 * @param name Line style label
	 * @return Line style object
	 */
	private LineStyle getLineStyleFromPreferences(final String name)
	{
		String value = preferenceStore.getString(name);
		for(LineStyle s : LineStyle.values())
			if (s.label.equalsIgnoreCase(value))
				return s;
		return LineStyle.NONE;
	}

	/**
	 * Convert GraphSettings position representation to SWT
	 * @param value position representation in GraphSettings
	 * @return position representation in SWT
	 */
	private int swtPositionFromInternal(int value)
	{
		switch(value)
		{
			case GraphSettings.POSITION_LEFT:
				return SWT.LEFT;
			case GraphSettings.POSITION_RIGHT:
				return SWT.RIGHT;
			case GraphSettings.POSITION_TOP:
				return SWT.TOP;
			case GraphSettings.POSITION_BOTTOM:
				return SWT.BOTTOM;
		}
		return SWT.BOTTOM;
	}

	/**
	 * Add line series to chart
	 * 
	 * @param description Description
	 * @param xSeries X axis data
	 * @param ySeries Y axis data
	 */
	private ILineSeries addLineSeries(int index, String description, Date[] xSeries, double[] ySeries, boolean updateChart)
	{
		ISeriesSet seriesSet = getSeriesSet();
		ILineSeries series = (ILineSeries)seriesSet.createSeries(SeriesType.LINE, Integer.toString(index), updateChart);

		series.setName(description);
		series.setSymbolType(PlotSymbolType.NONE);
		series.setLineWidth(lineWidth);
		series.setLineColor(getColorFromPreferences("Chart.Colors.Data." + index)); //$NON-NLS-1$
		
		series.setXDateSeries(xSeries);
		series.setYSeries(ySeries);
		
		try
		{
		   series.enableStack(stacked, updateChart);
		}
		catch(IllegalStateException e)
		{
		   Activator.logError("Exception while addig chart series", e); //$NON-NLS-1$
		}
		
		if (updateChart)
		   updateLayout();
		
		return series;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#setTimeRange(java.util.Date, java.util.Date)
	 */
	@Override
	public void setTimeRange(final Date from, final Date to)
	{
		timeFrom = from.getTime();
		timeTo = to.getTime();
		getAxisSet().getXAxis(0).setRange(new Range(timeFrom, timeTo));
		
		int seconds = (int)((timeTo - timeFrom) / 1000);
		String formatString;
		int angle;
		if (seconds <= 600)
		{
			formatString = Messages.get().LineChart_MediumTimeFormat;
			angle = 0;
		}
		else if (seconds <= 86400)
		{
			formatString = Messages.get().LineChart_ShortTimeFormat;
			angle = 0;
		}
		else if (seconds <= 86400 * 7)
		{
			formatString = Messages.get().LineChart_Medium2TimeFormat;
			angle = 0;
		}
		else
		{
			formatString = Messages.get().LineChart_LongTimeFormat;
			angle = 45;
		}
		
		IAxisTick xTick = getAxisSet().getXAxis(0).getTick();
		DateFormat format = new SimpleDateFormat(formatString);
		xTick.setFormat(format);
		xTick.setTickLabelAngle(angle);
	}
	
	/**
	 * Paint DCI thresholds
	 */
	private void paintThresholds(PaintEvent e, IAxis axis)
	{
		GC gc = e.gc;
		Rectangle clientArea = getPlotArea().getClientArea();
		
		for(GraphItemStyle style : itemStyles)
		{
			if (style.isShowThresholds())
			{
				int y = axis.getPixelCoordinate(10);
				gc.setForeground(ColorConverter.colorFromInt(style.getColor(), colors));
				//gc.setLineStyle(SWT.LINE_DOT);
				gc.setLineWidth(3);
				gc.drawLine(0, y, clientArea.width, y);
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#initializationComplete()
	 */
	@Override
	public void initializationComplete()
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTitle(java.lang.String)
	 */
	@Override
	public void setChartTitle(String title)
	{
		getTitle().setText(title);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTitleVisible(boolean)
	 */
	@Override
	public void setTitleVisible(boolean visible)
	{
		getTitle().setVisible(visible);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isTitleVisible()
	 */
	@Override
	public boolean isTitleVisible()
	{
		return getTitle().isVisible();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#getChartTitle()
	 */
	@Override
	public String getChartTitle()
	{
		return getTitle().getText();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendVisible(boolean)
	 */
	@Override
	public void setLegendVisible(boolean visible)
	{
		getLegend().setVisible(visible);
		redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isLegendVisible()
	 */
	@Override
	public boolean isLegendVisible()
	{
		return getLegend().isVisible();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendPosition(int)
	 */
	@Override
	public void setLegendPosition(int position)
	{
		legendPosition = position;
		getLegend().setPosition(swtPositionFromInternal(position));
		redraw();
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
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPalette(org.netxms.ui.eclipse.charts.api.ChartColor[])
	 */
	@Override
	public void setPalette(ChartColor[] colors)
	{
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPaletteEntry(int, org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setPaletteEntry(int index, ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#getPaletteEntry(int)
	 */
	@Override
	public ChartColor getPaletteEntry(int index)
	{
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#set3DModeEnabled(boolean)
	 */
	@Override
	public void set3DModeEnabled(boolean enabled)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#is3DModeEnabled()
	 */
	@Override
	public boolean is3DModeEnabled()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLogScaleEnabled(boolean)
	 */
	@Override
	public void setLogScaleEnabled(boolean enabled)
	{
		getAxisSet().getYAxis(0).enableLogScale(enabled);
		redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isLogScaleEnabled()
	 */
	@Override
	public boolean isLogScaleEnabled()
	{
		return getAxisSet().getYAxis(0).isLogScaleEnabled();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTranslucent(boolean)
	 */
	@Override
	public void setTranslucent(boolean translucent)
	{
	   super.setTranslucent(translucent);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isTranslucent()
	 */
	@Override
	public boolean isTranslucent()
	{
		return super.isTranslucent();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#refresh()
	 */
	@Override
	public void refresh()
	{
	   updateLayout();
	   updateStackAndRiserData();
	   if (adjustYAxis)
	      adjustYAxis(true);
	   else
	      redraw();
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#rebuild()
	 */
	@Override
	public void rebuild()
	{
      updateStackAndRiserData();
      if (adjustYAxis)
         adjustYAxis(true);
      else
         redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#hasAxes()
	 */
	@Override
	public boolean hasAxes()
	{
		return true;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#addParameter(org.netxms.client.datacollection.GraphItem)
	 */
	@Override
	public int addParameter(GraphItem item)
	{
		items.add(item);
		return items.size() - 1;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#updateParameter(int, org.netxms.client.datacollection.DciData, boolean)
	 */
	@Override
	public void updateParameter(int index, DciData data, boolean updateChart)
	{
		if ((index < 0) || (index >= items.size()))
			return;
		
		final GraphItem item = items.get(index);
		final DciDataRow[] values = data.getValues();
		
		// Create series
		Date[] xSeries = new Date[values.length];
		double[] ySeries = new double[values.length];
		for(int i = 0; i < values.length; i++)
		{
			xSeries[i] = values[i].getTimestamp();
			ySeries[i] = values[i].getValueAsDouble();
		}
		
		ILineSeries series = addLineSeries(index, item.getDescription(), xSeries, ySeries, updateChart);
		applyItemStyle(index, series);
		
		if (updateChart)
		{
		   if (adjustYAxis)
   		{
   			adjustYAxis(true);
   		}
		   else
		   {
		      redraw();
		   }
		}
	}

	/**
	 * Apply graph item style
	 * 
	 * @param index item index
	 * @param series added series
	 */
	private void applyItemStyle(int index, ILineSeries series)
	{
		if ((index < 0) || (index >= itemStyles.size()))
			return;

		GraphItemStyle style = itemStyles.get(index);
		series.setLineColor(ColorConverter.colorFromInt(style.getColor(), colors));
		series.enableArea(style.getType() == GraphItemStyle.AREA);
		series.setInverted(style.isInverted());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#getItemStyles()
	 */
	@Override
	public List<GraphItemStyle> getItemStyles()
	{
		return itemStyles;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#setItemStyles(java.util.List)
	 */
	@Override
	public void setItemStyles(List<GraphItemStyle> itemStyles)
	{
		this.itemStyles = itemStyles;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#isZoomEnabled()
	 */
	@Override
	public boolean isZoomEnabled()
	{
		return zoomEnabled;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#setZoomEnabled(boolean)
	 */
	@Override
	public void setZoomEnabled(boolean enableZoom)
	{
		this.zoomEnabled = enableZoom;
		final Canvas plotArea = getPlotArea();
		if (enableZoom)
		{
			plotArea.addMouseListener(zoomMouseListener);
			plotArea.addPaintListener(zoomPaintListener);
		}
		else
		{
			plotArea.removeMouseListener(zoomMouseListener);
			plotArea.removePaintListener(zoomPaintListener);
		}
	}
	
	/**
	 * Adjust upper border of current range
	 * 
	 * @param upper
	 * @return
	 */
	private static double adjustRange(double upper)
	{
		double adjustedUpper = upper;
      for(double d = 0.00001; d < 10000000000000000000.0; d *= 10)
		{
         if ((upper > d) && (upper <= d * 10))
         {
         	adjustedUpper -= adjustedUpper % d;
         	adjustedUpper += d;
            break;
         }
		}
      return adjustedUpper;
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
		final LineStyle ls = visible ? LineStyle.DOT : LineStyle.NONE;
		getAxisSet().getXAxis(0).getGrid().setStyle(ls);
		getAxisSet().getYAxis(0).getGrid().setStyle(ls);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#adjustXAxis()
	 */
	@Override
	public void adjustXAxis(boolean repaint)
	{
		for(final IAxis axis : getAxisSet().getXAxes())
		{
			axis.adjustRange();
		}
		if (repaint)
			redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#adjustYAxis(boolean)
	 */
	@Override
	public void adjustYAxis(boolean repaint)
	{
	   adjustYAxis = true;
		final IAxis yAxis = getAxisSet().getYAxis(0);
		yAxis.adjustRange();
		final Range range = yAxis.getRange();
      if (!modifyYBase && range.lower > 0)
         range.lower = 0;
      else if (range.lower < 0)
         range.lower = - adjustRange(Math.abs(range.lower));
      range.upper = adjustRange(range.upper);
		yAxis.setRange(range);

		if (repaint)
			redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#zoomIn()
	 */
	@Override
	public void zoomIn()
	{
		getAxisSet().zoomIn();
		redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#zoomOut()
	 */
	@Override
	public void zoomOut()
	{
		getAxisSet().zoomOut();
		redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendColor(org.netxms.ui.eclipse.charts.api.ChartColor, org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setLegendColor(ChartColor foreground, ChartColor background)
	{
		getLegend().setForeground(colors.create(foreground.getRGBObject()));
		getLegend().setBackground(colors.create(background.getRGBObject()));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setAxisColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setAxisColor(ChartColor color)
	{
		final Color c = colors.create(color.getRGBObject());
		getAxisSet().getXAxis(0).getTick().setForeground(c);
		getAxisSet().getYAxis(0).getTick().setForeground(c);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setGridColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setGridColor(ChartColor color)
	{
		final Color c = colors.create(color.getRGBObject());
		getAxisSet().getXAxis(0).getGrid().setForeground(c);
		getAxisSet().getYAxis(0).getGrid().setForeground(c);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setBackgroundColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setBackgroundColor(ChartColor color)
	{
		setBackground(colors.create(color.getRGBObject()));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPlotAreaColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setPlotAreaColor(ChartColor color)
	{
		setBackgroundInPlotArea(colors.create(color.getRGBObject()));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#addError(java.lang.String)
	 */
	@Override
	public void addError(String message)
	{
		if (errors.add(message))
			redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#clearErrors()
	 */
	@Override
	public void clearErrors()
	{
		if (errors.size() > 0)
		{
			errors.clear();
			redraw();
		}
	}
	
	/**
	 * Draw error indicator if needed
	 * 
	 * @param gc
	 */
	private void paintErrorIndicator(GC gc)
	{
		if (errors.size() == 0)
			return;

		if (errorImage == null)
			errorImage = Activator.getImageDescriptor("icons/chart_error.png").createImage(); //$NON-NLS-1$

		gc.setAlpha(127);
		gc.setBackground(colors.create(127, 127, 127));
		gc.fillRectangle(getPlotArea().getClientArea());
		gc.setAlpha(255);
		gc.drawImage(errorImage, 10, 10);
		
		gc.setForeground(colors.create(192, 0, 0));
		Iterator<String> it = errors.iterator();
		int y = 12;
		int h = gc.textExtent("X").y; //$NON-NLS-1$
		while(it.hasNext())
		{
			gc.drawText(it.next(), 40, y, true);
			y += h + 5;
		}
	}

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#setStacked(boolean)
    */
   @Override
   public void setStacked(boolean stacked)
   {
      this.stacked = stacked;
      redraw();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#isStacked()
    */
   @Override
   public boolean isStacked()
   {
      return stacked;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#setExtendedLegend(boolean)
    */
   @Override
   public void setExtendedLegend(boolean extended)
   {
      getLegend().setExtended(extended);
      if (isLegendVisible())
      {
         updateLayout();
         redraw();
      }
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#isExtendedLegend()
    */
   @Override
   public boolean isExtendedLegend()
   {
      return getLegend().isExtended();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.api.DataChart#setYAxisRange(double, double)
    */
   @Override
   public void setYAxisRange(double from, double to)
   {
      getAxisSet().getYAxis(0).setRange(new Range(from, to));
      adjustYAxis = false;
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#setLineWidth(int)
    */
   @Override
   public void setLineWidth(int width)
   {
      lineWidth = width;

      // update existing series
      for(ISeries s : getSeriesSet().getSeries())
      {
         if (s instanceof ILineSeries)
            ((ILineSeries)s).setLineWidth(lineWidth);
      }
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#getLineWidth()
    */
   @Override
   public int getLineWidth()
   {
      return lineWidth;
   }

   /**
    * Get data point closest to given point in plot area
    * 
    * @param px
    * @param py
    * @return
    */
   public DataPoint getClosestDataPoint(int px, int py) 
   {
      IAxis xAxis = getAxisSet().getXAxis(0);
      IAxis yAxis = getAxisSet().getYAxis(0);

      double x = xAxis.getDataCoordinate(px);
      double y = yAxis.getDataCoordinate(py);

      double closestX = 0;
      double closestY = 0;
      double minDist = Double.MAX_VALUE;
      ISeries closestSeries = null;

      /* over all series */
      ISeries[] series = getSeriesSet().getSeries();
      for(ISeries s : series) 
      {
          double[] xS = s.getXSeries();
          double[] yS = s.getYSeries();

          /* check all data points */
          for (int i = 0; i < xS.length; i++) 
          {
              /* compute distance to mouse position */
              double newDist = Math.sqrt(Math.pow((x - xS[i]), 2) + Math.pow((y - yS[i]), 2));

              /* if closer to mouse, remember */
              if (newDist < minDist) 
              {
                  minDist = newDist;
                  closestX = xS[i];
                  closestY = yS[i];
                  closestSeries = s;
              }
          }
      }
      
      return (closestSeries != null) ? new DataPoint(new Date((long)closestX), closestY, closestSeries) : null;
   }
   
   /**
    * Get series at given point
    * 
    * @param px
    * @param py
    * @return
    */
   public ISeries getSeriesAtPoint(int px, int py)
   {
      ISeries[] series = getSeriesSet().getSeries();
      for(ISeries s : series) 
      {
         int size = s.getSize();
         for(int i = 1; i < size; i++)
         {
            Point p1 = s.getPixelCoordinates(i - 1);
            Point p2 = s.getPixelCoordinates(i);
            if ((px > p1.x + 2) || (px < p2.x - 2) || (py < Math.min(p1.y, p2.y) - 2) || (py > Math.max(p1.y, p2.y) + 2))
               continue;
            if (pointToLineDistance(px, py, p2, p1) <= ((ILineSeries)s).getLineWidth() * 3.0)
               return s;
         }
      }
      return null;
   }
   
   /**
    * Calculate distance from point to line.
    * https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
    * 
    * @param x
    * @param y
    * @param p1
    * @param p2
    * @return
    */
   private static double pointToLineDistance(int x, int y, Point p1, Point p2)
   {
      int dx = p2.x - p1.x;
      int dy = p2.y - p1.y;
      
      double area2 = (double)Math.abs(dy * x - dx * y + p2.x * p1.y - p2.y * p1.x);
      double dist = Math.sqrt(dx * dx + dy * dy);
      return area2 / dist;
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#modifyYBase(boolean)
    */
   public void modifyYBase(boolean modifyYBase)
   {
      this.modifyYBase = modifyYBase;
   }
}
