/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import java.util.Date;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.widgets.internal.SelectionRectangle;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.IAxisSet;
import org.swtchart.IAxisTick;
import org.swtchart.ILegend;
import org.swtchart.ILineSeries;
import org.swtchart.ISeriesSet;
import org.swtchart.ITitle;
import org.swtchart.LineStyle;
import org.swtchart.Range;
import org.swtchart.IAxis.Direction;
import org.swtchart.ILineSeries.PlotSymbolType;
import org.swtchart.ISeries.SeriesType;

/**
 * Line chart widget
 *
 */
public class HistoricDataChart extends Chart
{
	private static final int MAX_ZOOM_LEVEL = 16;
	
	private long timeFrom;
	private long timeTo;
	private boolean showToolTips;
	private boolean enableZoom;
	private boolean selectionActive = false;
	private int zoomLevel = 0;
	private MouseMoveListener moveListener;
	private SelectionRectangle selection = new SelectionRectangle();
	private IPreferenceStore preferenceStore;
	
	/**
	 * @param parent
	 * @param style
	 */
	public HistoricDataChart(Composite parent, int style)
	{
		super(parent, style);
		
		preferenceStore = Activator.getDefault().getPreferenceStore();
		showToolTips = preferenceStore.getBoolean("Chart.ShowToolTips");
		enableZoom = preferenceStore.getBoolean("Chart.EnableZoom");
		setBackground(getColorFromPreferences("Chart.Colors.Background"));
		selection.setColor(getColorFromPreferences("Chart.Colors.Selection"));

		// Setup title
		ITitle title = getTitle();
		title.setVisible(preferenceStore.getBoolean("Chart.ShowTitle"));
		title.setForeground(getColorFromPreferences("Chart.Colors.Title"));
		
		// Setup legend
		ILegend legend = getLegend();
		legend.setPosition(SWT.BOTTOM);
		
		// Default time range
		timeTo = System.currentTimeMillis();
		timeFrom = timeTo - 3600000;

		// Setup X and Y axis
		IAxisSet axisSet = getAxisSet();
		final IAxis xAxis = axisSet.getXAxis(0);
		xAxis.getTitle().setVisible(false);
		xAxis.setRange(new Range(timeFrom, timeTo));
		IAxisTick xTick = xAxis.getTick();
		xTick.setForeground(getColorFromPreferences("Chart.Axis.X.Color"));
		DateFormat format = new SimpleDateFormat("HH:mm");
		xTick.setFormat(format);
		
		final IAxis yAxis = axisSet.getYAxis(0);
		yAxis.getTitle().setVisible(false);
		yAxis.getTick().setForeground(getColorFromPreferences("Chart.Axis.Y.Color"));
		
		// Setup grid
		xAxis.getGrid().setStyle(getLineStyleFromPreferences("Chart.Grid.X.Style"));
		xAxis.getGrid().setForeground(getColorFromPreferences("Chart.Grid.X.Color"));
		yAxis.getGrid().setStyle(getLineStyleFromPreferences("Chart.Grid.Y.Style"));
		yAxis.getGrid().setForeground(getColorFromPreferences("Chart.Grid.Y.Color"));
		
		// Setup plot area
		setBackgroundInPlotArea(getColorFromPreferences("Chart.Colors.PlotArea"));
		final Composite plotArea = getPlotArea();
		if (showToolTips)
		{
			plotArea.addMouseTrackListener(new MouseTrackListener() {
				@Override
				public void mouseEnter(MouseEvent e)
				{
				}
	
				@Override
				public void mouseExit(MouseEvent e)
				{
				}
	
				@Override
				public void mouseHover(MouseEvent e)
				{
					Date timestamp = new Date((long)xAxis.getDataCoordinate(e.x));
					double value = yAxis.getDataCoordinate(e.y);
					getPlotArea().setToolTipText(DateFormat.getDateTimeInstance().format(timestamp) + "\n" + value);
				}
			});
		}
		
		if (enableZoom)
		{
			plotArea.addMouseListener(new MouseListener() {
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
			});
			
			plotArea.addPaintListener(new PaintListener() {
				@Override
				public void paintControl(PaintEvent e)
				{
					if (selectionActive)
						selection.draw(e.gc);
				}
			});
		}
	}
	
	/**
	 * Selection start handler
	 * @param e
	 */
	private void startSelection(MouseEvent e)
	{
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
	}
	
	/**
	 * Selection end handler
	 */
	private void endSelection()
	{
		if (!selectionActive)
			return;
		
		selectionActive = false;
		final Composite plotArea = getPlotArea();
		plotArea.removeMouseMoveListener(moveListener);

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
      
		selection.dispose();
		redraw();
	}
	
   /**
    * Sets the axis range.
    * 
    * @param range
    *            the axis range in pixels
    * @param axis
    *            the axis to set range
    */
	private void setRange(Point range, IAxis axis)
	{
		double min = axis.getDataCoordinate(range.x);
		double max = axis.getDataCoordinate(range.y);

		axis.setRange(new Range(min, max));
	}

   /**
	 * Create color object from preference string
	 *  
	 * @param name Preference name
	 * @return Color object
	 */
	private Color getColorFromPreferences(final String name)
	{
		return new Color(getDisplay(), PreferenceConverter.getColor(preferenceStore, name));
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
	 * Add line series to chart
	 * 
	 * @param description Description
	 * @param xSeries X axis data
	 * @param ySeries Y axis data
	 */
	public ILineSeries addLineSeries(int index, String description, Date[] xSeries, double[] ySeries)
	{
		ISeriesSet seriesSet = getSeriesSet();
		ILineSeries series = (ILineSeries)seriesSet.createSeries(SeriesType.LINE, description);
		
		series.setAntialias(SWT.ON);
		series.setSymbolType(PlotSymbolType.NONE);
		series.setLineWidth(2);
		series.setLineColor(getColorFromPreferences("Chart.Colors.Data." + index));
		
		series.setXDateSeries(xSeries);
		series.setYSeries(ySeries);
		
		return series;
	}
	
	/**
	 * Set time range for chart
	 * 
	 * @param from Chart's "from" time
	 * @param to Chart's "to" time
	 */
	public void setTimeRange(final Date from, final Date to)
	{
		timeFrom = from.getTime();
		timeTo = to.getTime();
		getAxisSet().getXAxis(0).setRange(new Range(timeFrom, timeTo));
	}
}
