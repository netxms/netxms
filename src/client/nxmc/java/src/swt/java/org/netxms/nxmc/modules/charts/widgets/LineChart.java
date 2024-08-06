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

import java.text.DateFormat;
import java.text.Format;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swtchart.IAxis;
import org.eclipse.swtchart.IAxis.Direction;
import org.eclipse.swtchart.IAxisSet;
import org.eclipse.swtchart.IAxisTick;
import org.eclipse.swtchart.ICustomPaintListener;
import org.eclipse.swtchart.ILineSeries;
import org.eclipse.swtchart.ILineSeries.PlotSymbolType;
import org.eclipse.swtchart.IPlotArea;
import org.eclipse.swtchart.ISeries;
import org.eclipse.swtchart.ISeries.SeriesType;
import org.eclipse.swtchart.ISeriesSet;
import org.eclipse.swtchart.LineStyle;
import org.eclipse.swtchart.Range;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.DataPoint;
import org.netxms.nxmc.modules.charts.widgets.internal.SelectionRectangle;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Line chart widget
 */
public class LineChart extends org.eclipse.swtchart.Chart implements PlotArea
{
   private static final Logger logger = LoggerFactory.getLogger(LineChart.class);

   private static final int MAX_ZOOM_LEVEL = 16;

   private Chart chart;
   private ChartConfiguration configuration;
   private ColorCache colorCache;
   private PreferenceStore preferenceStore;
	private long timeFrom;
	private long timeTo;
	private boolean showToolTips;
	private boolean selectionActive = false;
	private int zoomLevel = 0;
	private MouseMoveListener moveListener;
	private SelectionRectangle selection = new SelectionRectangle();
	private MouseListener zoomMouseListener = null;
	private PaintListener zoomPaintListener = null;
	private boolean tooltipShown = false;
	private boolean zoomedToSelectionX = false;
   private boolean zoomedToSelectionY = false;
   private Date delayedRangeFrom;
   private Date delayedRangeTo;

	/**
	 * @param parent
	 */
   public LineChart(Chart parent)
	{
      super(parent, SWT.NONE);

      chart = parent;
      configuration = chart.getConfiguration();
      colorCache = chart.getColorCache();

      preferenceStore = PreferenceStore.getInstance();
      showToolTips = preferenceStore.getAsBoolean("Chart.ShowToolTips", true);
      setBackground(parent.getBackground());
      selection.setColor(ThemeEngine.getBackgroundColor("Chart.Selection"));

      getTitle().setVisible(false);
      getLegend().setVisible(false);

      setTranslucent(configuration.isTranslucent());
      setUseMultipliers(configuration.isUseMultipliers());

		// Default time range
		timeTo = System.currentTimeMillis();
		timeFrom = timeTo - 3600000;

      // Setup X and Y axis
      Color axisColor = ThemeEngine.getForegroundColor("Chart.PlotArea");
		IAxisSet axisSet = getAxisSet();
		final IAxis xAxis = axisSet.getXAxis(0);
		xAxis.getTitle().setVisible(false);
		xAxis.setRange(new Range(timeFrom, timeTo));
		IAxisTick xTick = xAxis.getTick();
      xTick.setForeground(axisColor);
      DateFormat format = DateFormatFactory.getShortTimeFormat();
		xTick.setFormat(format);
      // FIXME: xTick.setFont(Activator.getDefault().getChartFont());

		final IAxis yAxis = axisSet.getYAxis(0);
		yAxis.getTitle().setVisible(false);
      yAxis.getTick().setForeground(axisColor);
      // FIXME: yAxis.getTick().setFont(Activator.getDefault().getChartFont());
      yAxis.enableLogScale(configuration.isLogScale());
      if (!configuration.isAutoScale())
      {
         yAxis.setRange(new Range(configuration.getMinYScaleValue(), configuration.getMaxYScaleValue()));
      }

      if (!configuration.getYAxisLabel().isBlank())
      {
         yAxis.getTitle().setText(configuration.getYAxisLabel());
         yAxis.getTitle().setVisible(true);
         yAxis.getTitle().setForeground(axisColor);
      }

		// Setup grid
      Color gridColor = ThemeEngine.getForegroundColor("Chart.Grid");
      xAxis.getGrid().setStyle(getLineStyleFromPreferences("Chart.Grid.X.Style"));
      xAxis.getGrid().setForeground(gridColor);
      yAxis.getGrid().setStyle(getLineStyleFromPreferences("Chart.Grid.Y.Style"));
      yAxis.getGrid().setForeground(gridColor);
      setGridVisible(configuration.isGridVisible());

		// Setup plot area
      getPlotArea().setBackground(ThemeEngine.getBackgroundColor("Chart.PlotArea"));
      final IPlotArea plotArea = getPlotArea();
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
				   if (tooltipShown)
				   {
   				   getPlotArea().setToolTipText(null);
   				   tooltipShown = false;
				   }
				}

				@Override
				public void mouseHover(MouseEvent e)
				{
               ISeries<?> series = getSeriesAtPoint(e.x, e.y);
				   if (series != null)
				   {
   					Date timestamp = new Date((long)xAxis.getDataCoordinate(e.x));
   					double value = yAxis.getDataCoordinate(e.y);
   					getPlotArea().setToolTipText(
                        series.getDescription() + "\n" + DateFormatFactory.getDateTimeFormat().format(timestamp) + "\n" +
   					      (useMultipliers ? DataFormatter.roundDecimalValue(value, cachedTickStep, 5) : Double.toString(value)));
   					tooltipShown = true;
				   }
				}
			});

			plotArea.addMouseMoveListener(new MouseMoveListener() {
            @Override
            public void mouseMove(MouseEvent e)
            {
               if (tooltipShown)
               {
                  plotArea.setToolTipText(null);
                  tooltipShown = false;
               }
            }
         });
		}

      plotArea.addMouseListener(new MouseListener() {
         @Override
         public void mouseUp(MouseEvent e)
         {
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            chart.fireDoubleClickListeners();
         }
      });

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
				if (selectionActive)
					selection.draw(e.gc);
			}
		};

      setZoomEnabled(parent.getConfiguration().isZoomEnabled());

		((IPlotArea)plotArea).addCustomPaintListener(new ICustomPaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				paintThresholds(e, yAxis);
			}

			@Override
			public boolean drawBehindSeries()
			{
				return true;
			}
		});
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

      final IPlotArea plotArea = getPlotArea();
		moveListener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e)
			{
				selection.setEndPoint(e.x, e.y);
            plotArea.getControl().redraw();
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
      final IPlotArea plotArea = getPlotArea();
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
			zoomedToSelectionX = true;
			zoomedToSelectionY = true;
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
	 * Return line style object matching given label. If no match found, LineStyle.NONE is returned.
	 * 
	 * @param name Line style label
	 * @return Line style object
	 */
	private LineStyle getLineStyleFromPreferences(final String name)
	{
      String value = preferenceStore.getAsString(name);
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
   @SuppressWarnings("deprecation")
   private ILineSeries<?> addLineSeries(int index, String description, Date[] xSeries, double[] ySeries)
	{
		ISeriesSet seriesSet = getSeriesSet();
      ILineSeries<?> series = (ILineSeries<?>)seriesSet.createSeries(SeriesType.LINE, Integer.toString(index));

      series.setDescription(description);
		series.setSymbolType(PlotSymbolType.NONE);
      series.setLineWidth(configuration.getLineWidth());
      series.setLineColor(ThemeEngine.getForegroundColor("Chart.Data." + Integer.toString(index + 1)));

		series.setXDateSeries(xSeries);
		series.setYSeries(ySeries);

		try
		{
         series.enableStack(configuration.isStacked());
		}
		catch(IllegalStateException e)
		{
         logger.error("Exception while adding chart series", e);
		}

		return series;
	}

   /**
    * Set time range
    *
    * @param from start time
    * @param to end time
    */
	public void setTimeRange(final Date from, final Date to)
	{
	   if (zoomedToSelectionX)
	   {
	      delayedRangeFrom = from;
	      delayedRangeTo = to;
	      return;
	   }

      delayedRangeFrom = null;
      delayedRangeTo = null;

		timeFrom = from.getTime();
		timeTo = to.getTime();
		getAxisSet().getXAxis(0).setRange(new Range(timeFrom, timeTo));

		int seconds = (int)((timeTo - timeFrom) / 1000);
      Format format;
      int angle;
		if (seconds <= 600)
		{
         format = DateFormatFactory.getTimeFormat();
			angle = 0;
		}
		else if (seconds <= 86400)
		{
         format = DateFormatFactory.getShortTimeFormat();
			angle = 0;
		}
		else if (seconds <= 86400 * 7)
		{
         format = new SimpleDateFormat("E " + ((SimpleDateFormat)DateFormat.getTimeInstance(DateFormat.SHORT)).toPattern());
			angle = 0;
		}
		else
		{
         format = DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT);
			angle = 45;
		}

		IAxisTick xTick = getAxisSet().getXAxis(0).getTick();
		xTick.setFormat(format);
		xTick.setTickLabelAngle(angle);
	}

   /**
     * Paint DCI thresholds
    */
   private void paintThresholds(PaintEvent e, IAxis axis)
   {
      GC gc = e.gc;
      Rectangle clientArea = ((Composite)getPlotArea().getControl()).getClientArea();
      NXCSession session = Registry.getSession();

      List<ChartDciConfig> items = chart.getItems();
      for(int i = 0; i < items.size(); i++)
      {
         Threshold[] tr = chart.getThreshold(i);
         if (items.get(i).showThresholds && !configuration.isStacked() && tr != null)
         {
            for (int j = 0; j < tr.length; j++)
            {
               try
               {
                  int y = axis.getPixelCoordinate(Integer.parseInt(tr[j].getValue()));
                  final EventTemplate event = (EventTemplate)session.findEventTemplateByCode(((Threshold)tr[j]).getFireEvent());
                  gc.setForeground(StatusDisplayInfo.getStatusColor(event.getSeverity()));
                  gc.setLineStyle(SWT.LINE_DOT);
                  gc.setLineWidth(3);
                  gc.drawLine(0, y, clientArea.width, y);
               }
               catch (Exception ex)
               {
                  //Do nothing for String thresholds
               }
            }
         }
      }
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.PlotArea#refresh()
    */
	public void refresh()
	{
      List<ChartDciConfig> items = chart.getItems();
      for(int i = 0; i < items.size(); i++)
         updateSeries(i, items.get(i));

	   updateLayout();
	   updateStackAndRiserData();
      if (configuration.isAutoScale() && !zoomedToSelectionY)
	      adjustYAxis(true);
	   else
	      redraw();
	}

   /**
    * Update data series on chart
    *
    * @param index item index
    * @param item graph item
    */
   private void updateSeries(int index, ChartDciConfig item)
	{
      final DciDataRow[] values = chart.getDataSeries().get(index).getValues();

		// Create series
		Date[] xSeries = new Date[values.length];
		double[] ySeries = new double[values.length];
		for(int i = 0; i < values.length; i++)
		{
			xSeries[i] = values[i].getTimestamp();
			ySeries[i] = values[i].getValueAsDouble();
		}

      ILineSeries<?> series = addLineSeries(index, item.getLabel(), xSeries, ySeries);
      if (item.getColorAsInt() != -1)
         series.setLineColor(ColorConverter.colorFromInt(item.getColorAsInt(), colorCache));
      series.enableArea(item.isArea(configuration.isArea()));
      series.setInverted(item.invertValues);
	}

   /**
    * @param enableZoom
    */
   private void setZoomEnabled(boolean enableZoom)
	{
      final IPlotArea plotArea = getPlotArea();
		if (enableZoom)
		{
			plotArea.addMouseListener(zoomMouseListener);
         plotArea.getControl().addPaintListener(zoomPaintListener);
		}
		else
		{
			plotArea.removeMouseListener(zoomMouseListener);
         plotArea.getControl().removePaintListener(zoomPaintListener);
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

   /**
    * Set grid visible
    *
    * @param visible true to set grid visible
    */
   private void setGridVisible(boolean visible)
	{
		final LineStyle ls = visible ? LineStyle.DOT : LineStyle.NONE;
		getAxisSet().getXAxis(0).getGrid().setStyle(ls);
		getAxisSet().getYAxis(0).getGrid().setStyle(ls);
	}

   /**
    * Adjust X axis
    *
    * @param repaint true to repaint chart
    */
	public void adjustXAxis(boolean repaint)
	{
      zoomedToSelectionX = false;
      if (delayedRangeFrom != null)
      {
         setTimeRange(delayedRangeFrom, delayedRangeTo);
      }
      else
      {
   		for(final IAxis axis : getAxisSet().getXAxes())
   		{
   			axis.adjustRange();
   		}
      }
		if (repaint)
			redraw();
	}

   /**
    * Adjust Y axis
    *
    * @param repaint true to repaint chart
    */
	public void adjustYAxis(boolean repaint)
	{
      zoomedToSelectionY = false;
		final IAxis yAxis = getAxisSet().getYAxis(0);
		yAxis.adjustRange();
		final Range range = yAxis.getRange();
      if (!configuration.isModifyYBase() && (range.lower > 0))
         range.lower = 0;
      else if (range.lower < 0)
         range.lower = - adjustRange(Math.abs(range.lower));
      range.upper = adjustRange(range.upper);
		yAxis.setRange(range);
		if (repaint)
			redraw();
	}

   /**
    * Zoom in
    */
	public void zoomIn()
	{
		getAxisSet().zoomIn();
		redraw();
	}

   /**
    * Zoom out
    */
	public void zoomOut()
	{
		getAxisSet().zoomOut();
		redraw();
	}

   /**
    * Set axis color.
    *
    * @param color new axis color
    */
	public void setAxisColor(ChartColor color)
	{
      final Color c = colorCache.create(color.getRGBObject());
		getAxisSet().getXAxis(0).getTick().setForeground(c);
		getAxisSet().getYAxis(0).getTick().setForeground(c);
      getAxisSet().getYAxis(0).getTitle().setForeground(c);
	}

   /**
    * Set grid color.
    *
    * @param color new grid color
    */
	public void setGridColor(ChartColor color)
	{
      final Color c = colorCache.create(color.getRGBObject());
		getAxisSet().getXAxis(0).getGrid().setForeground(c);
		getAxisSet().getYAxis(0).getGrid().setForeground(c);
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
      ISeries<?> closestSeries = null;

      /* over all series */
      ISeries<?>[] series = getSeriesSet().getSeries();
      for(ISeries<?> s : series)
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
   public ISeries<?> getSeriesAtPoint(int px, int py)
   {
      ISeries<?>[] series = getSeriesSet().getSeries();
      for(ISeries<?> s : series)
      {
         int size = s.getSize();
         for(int i = 1; i < size; i++)
         {
            Point p1 = s.getPixelCoordinates(i - 1);
            Point p2 = s.getPixelCoordinates(i);
            if ((px > p1.x + 2) || (px < p2.x - 2) || (py < Math.min(p1.y, p2.y) - 2) || (py > Math.max(p1.y, p2.y) + 2))
               continue;
            if (pointToLineDistance(px, py, p2, p1) <= ((ILineSeries<?>)s).getLineWidth() * 3.0)
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
}
