/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.series;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Set;
import java.util.Map.Entry;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.ISeries;
import org.swtchart.ISeriesSet;
import org.swtchart.Range;
import org.swtchart.IAxis.Direction;
import org.swtchart.ISeries.SeriesType;
import org.swtchart.internal.axis.Axis;
import org.swtchart.internal.compress.CompressConfig;
import org.swtchart.internal.compress.ICompress;

/**
 * A series container.
 */
public class SeriesSet implements ISeriesSet
{
   /** the chart */
   private Chart chart;

   /** the series */
   private LinkedHashMap<String, Series> seriesMap;

   /**
    * Constructor.
    * 
    * @param chart the chart
    */
   public SeriesSet(Chart chart)
   {
      this.chart = chart;

      seriesMap = new LinkedHashMap<String, Series>();
   }

   /*
    * @see ISeriesSet#createSeries(ISeries.SeriesType, String)
    */
   public ISeries createSeries(SeriesType type, String id)
   {
      if (id == null)
      {
         SWT.error(SWT.ERROR_NULL_ARGUMENT);
         return null; // to suppress warning...
      }

      String identifier = id.trim();

      if ("".equals(identifier))
      {
         SWT.error(SWT.ERROR_INVALID_ARGUMENT);
      }

      Series series = null;
      if (type == SeriesType.BAR)
      {
         series = new BarSeries(chart, identifier);
      }
      else if (type == SeriesType.LINE)
      {
         series = new LineSeries(chart, identifier);
      }
      else
      {
         SWT.error(SWT.ERROR_INVALID_ARGUMENT);
         return null; // to suppress warning...
      }

      Series oldSeries = seriesMap.get(identifier);
      if (oldSeries != null)
      {
         oldSeries.dispose();
      }

      int[] xAxisIds = chart.getAxisSet().getXAxisIds();
      int[] yAxisIds = chart.getAxisSet().getYAxisIds();
      series.setXAxisId(xAxisIds[0]);
      series.setYAxisId(yAxisIds[0]);

      seriesMap.put(identifier, series);

      Axis axis = (Axis)chart.getAxisSet().getXAxis(xAxisIds[0]);
      if (axis != null)
      {
         updateStackAndRiserData();
      }

      // legend will be shown if there is previously no series.
      chart.updateLayout();

      return series;
   }

   /*
    * @see ISeriesSet#getSeries(String)
    */
   public ISeries getSeries(String id)
   {
      if (id == null)
      {
         SWT.error(SWT.ERROR_NULL_ARGUMENT);
      }

      return seriesMap.get(id);
   }

   /*
    * @see ISeriesSet#getSeries()
    */
   public ISeries[] getSeries()
   {
      Set<String> keys = seriesMap.keySet();
      ISeries[] series = new ISeries[keys.size()];
      int i = 0;
      for(String key : keys)
      {
         series[i++] = seriesMap.get(key);
      }
      return series;
   }

   /*
    * @see ISeriesSet#deleteSeries(String)
    */
   public void deleteSeries(String id)
   {
      validateSeriesId(id);

      seriesMap.get(id).dispose();
      seriesMap.remove(id);

      updateStackAndRiserData();

      // legend will be hidden if this is the last series
      chart.updateLayout();
   }

   /*
    * @see ISeriesSet#bringForward(String)
    */
   public void bringForward(String id)
   {
      validateSeriesId(id);

      String seriesId = null;
      LinkedHashMap<String, Series> newSeriesMap = new LinkedHashMap<String, Series>();
      for(Entry<String, Series> entry : seriesMap.entrySet())
      {

         if (entry.getKey().equals(id))
         {
            seriesId = id;
            continue;
         }

         newSeriesMap.put(entry.getKey(), entry.getValue());

         if (seriesId != null)
         {
            newSeriesMap.put(seriesId, seriesMap.get(seriesId));
            seriesId = null;
         }
      }
      if (seriesId != null)
      {
         newSeriesMap.put(seriesId, seriesMap.get(seriesId));
      }
      seriesMap = newSeriesMap;

      updateStackAndRiserData();
      chart.updateLayout();
   }

   /*
    * @see ISeriesSet#bringToFront(String)
    */
   public void bringToFront(String id)
   {
      validateSeriesId(id);

      Series series = seriesMap.get(id);
      seriesMap.remove(id);
      seriesMap.put(series.getId(), series);

      updateStackAndRiserData();
      chart.updateLayout();
   }

   /*
    * @see ISeriesSet#sendBackward(String)
    */
   public void sendBackward(String id)
   {
      validateSeriesId(id);

      String seriesId = null;
      LinkedHashMap<String, Series> newSeriesMap = new LinkedHashMap<String, Series>();
      for(Entry<String, Series> entry : seriesMap.entrySet())
      {

         if (!entry.getKey().equals(id) || seriesId == null)
         {
            newSeriesMap.put(entry.getKey(), entry.getValue());
            seriesId = entry.getKey();
            continue;
         }

         newSeriesMap.remove(seriesId);
         newSeriesMap.put(entry.getKey(), entry.getValue());
         newSeriesMap.put(seriesId, seriesMap.get(seriesId));
      }
      seriesMap = newSeriesMap;

      updateStackAndRiserData();
      chart.updateLayout();
   }

   /*
    * @see ISeriesSet#sendToBack(String)
    */
   public void sendToBack(String id)
   {
      validateSeriesId(id);

      LinkedHashMap<String, Series> newSeriesMap = new LinkedHashMap<String, Series>();
      newSeriesMap.put(id, seriesMap.get(id));
      for(Entry<String, Series> entry : seriesMap.entrySet())
      {
         if (!entry.getKey().equals(id))
         {
            newSeriesMap.put(entry.getKey(), entry.getValue());
         }
      }
      seriesMap = newSeriesMap;

      updateStackAndRiserData();
      chart.updateLayout();
   }

   /**
    * Disposes the series.
    */
   public void dispose()
   {
      for(Entry<String, Series> entry : seriesMap.entrySet())
      {
         entry.getValue().dispose();
      }
   }

   /**
    * Validates the given series id.
    * 
    * @param id the series id.
    */
   private void validateSeriesId(String id)
   {
      if (id == null)
      {
         SWT.error(SWT.ERROR_NULL_ARGUMENT);
      }
      if (seriesMap.get(id) == null)
      {
         throw new IllegalArgumentException("Given series id doesn't exist");
      }
   }

   /**
    * Compresses all series data.
    */
   public void compressAllSeries()
   {
      if (!chart.isCompressEnabled())
      {
         return;
      }

      CompressConfig config = new CompressConfig();

      final int PRECISION = 2;
      Point p = chart.getPlotArea().getSize();
      int width = p.x * PRECISION;
      int height = p.y * PRECISION;
      config.setSizeInPixel(width, height);

      for(ISeries series : getSeries())
      {
         int xAxisId = series.getXAxisId();
         int yAxisId = series.getYAxisId();

         IAxis xAxis = chart.getAxisSet().getXAxis(xAxisId);
         IAxis yAxis = chart.getAxisSet().getYAxis(yAxisId);
         if (xAxis == null || yAxis == null)
         {
            continue;
         }
         Range xRange = xAxis.getRange();
         Range yRange = yAxis.getRange();

         if (xRange == null || yRange == null)
         {
            continue;
         }

         double xMin = xRange.lower;
         double xMax = xRange.upper;
         double yMin = yRange.lower;
         double yMax = yRange.upper;

         config.setXLogScale(xAxis.isLogScaleEnabled());
         config.setYLogScale(yAxis.isLogScaleEnabled());

         double lower = xMin - (xMax - xMin) * 0.015;
         double upper = xMax + (xMax - xMin) * 0.015;
         if (xAxis.isLogScaleEnabled())
         {
            lower = ((Series)series).getXRange().lower;
         }
         config.setXRange(lower, upper);
         lower = yMin - (yMax - yMin) * 0.015;
         upper = yMax + (yMax - yMin) * 0.015;
         if (yAxis.isLogScaleEnabled())
         {
            lower = ((Series)series).getYRange().lower;
         }
         config.setYRange(lower, upper);

         ICompress compressor = ((Series)series).getCompressor();
         compressor.compress(config);
      }
   }

   /**
    * Updates the compressor associated with the given axis.
    * <p>
    * In most cases, compressor is updated when series is changed. However, there is a case that compressor has to be updated with
    * the changes in axis.
    * 
    * @param axis the axis
    */
   public void updateCompressor(Axis axis)
   {
      for(ISeries series : getSeries())
      {
         int axisId = (axis.getDirection() == Direction.X) ? series.getXAxisId() : series.getYAxisId();
         if (axisId != axis.getId())
         {
            continue;
         }

         ICompress compressor = ((Series)series).getCompressor();
         if (axis.isValidCategoryAxis())
         {
            String[] categorySeries = axis.getCategorySeries();
            if (categorySeries == null)
            {
               continue;
            }
            double[] xSeries = new double[categorySeries.length];
            for(int i = 0; i < xSeries.length; i++)
            {
               xSeries[i] = i;
            }
            compressor.setXSeries(xSeries);
         }
         else if (((Series)series).getXSeries() != null)
         {
            compressor.setXSeries(((Series)series).getXSeries());
         }
      }
      compressAllSeries();
   }

   /**
    * Updates the stack and riser data.
    */
   public void updateStackAndRiserData()
   {
      for(IAxis xAxis : chart.getAxisSet().getXAxes())
      {
         ((Axis)xAxis).setNumRisers(0);
         for(IAxis yAxis : chart.getAxisSet().getYAxes())
         {
            updateStackAndRiserData(xAxis, yAxis);
         }
      }
   }

   /**
    * Updates the stack and riser data for given axes.
    * 
    * @param xAxis the X axis
    * @param yAxis the Y axis
    */
   private void updateStackAndRiserData(IAxis xAxis, IAxis yAxis)
   {
      int riserCnt = 0;
      int stackRiserPosition = -1;
      double[] stackBarSeries = null;
      double[] stackLineSeries = null;

      if (((Axis)xAxis).isValidCategoryAxis())
      {
         String[] categorySeries = xAxis.getCategorySeries();
         if (categorySeries != null)
         {
            int size = categorySeries.length;
            stackBarSeries = new double[size];
            stackLineSeries = new double[size];
         }
      }
      else
      {
         // Build combined time series
         // Time stamps in series are reversed - latest value first
         List<Date> combinedTimeSeries = new ArrayList<Date>();
         for(ISeries series : getSeries())
         {
            if (!series.isStackEnabled())
               continue;
                     
            for(Date d : series.getXDateSeries())
            {
               int i;
               for(i = 0; i < combinedTimeSeries.size(); i++)
               {
                  if (combinedTimeSeries.get(i).getTime() == d.getTime())
                     break;                 
                  if (combinedTimeSeries.get(i).getTime() < d.getTime())
                  {
                     combinedTimeSeries.add(i, d);
                     break;
                  }
               }
               if (i == combinedTimeSeries.size())
                  combinedTimeSeries.add(d);
            }
         }         
         
         // insert missing values
         for(ISeries series : getSeries())
         {
            if (!series.isStackEnabled())
               continue;
            
            double[] ySeries = new double[combinedTimeSeries.size()];
            int combinedIndex = 0;
            double lastValue = 0;
            long lastTimestamp = 0;
            for(int i = 0; i < series.getYSeries().length; i++)
            {
               Date currentTimestamp = series.getXDateSeries()[i];
               double currentValue = series.getYSeries()[i];
               long currentCombinedTimestamp = combinedTimeSeries.get(combinedIndex).getTime();
               while(currentCombinedTimestamp > currentTimestamp.getTime())
               {
                  if (lastTimestamp != 0)
                  {
                     // do linear interpolation for missed value
                     ySeries[combinedIndex] = lastValue + (currentValue - lastValue) * ((double)(lastTimestamp - currentCombinedTimestamp) / (double)(lastTimestamp - currentTimestamp.getTime()));
                  }
                  else
                  {
                     ySeries[combinedIndex] = 0;
                  }
                  combinedIndex++;
                  currentCombinedTimestamp = combinedTimeSeries.get(combinedIndex).getTime();
               }
               ySeries[combinedIndex++] = currentValue;
               lastTimestamp = currentTimestamp.getTime();
               lastValue = currentValue;
            }
            
            series.setXDateSeries(combinedTimeSeries.toArray(new Date[combinedTimeSeries.size()]));
            series.setYSeries(ySeries);
         }            
         
         stackLineSeries = new double[combinedTimeSeries.size()];
      }
         
      for(ISeries series : getSeries())
      {
         if (series.getXAxisId() != xAxis.getId() || series.getYAxisId() != yAxis.getId() || !series.isVisible())
         {
            continue;
         }

         if (series.isStackEnabled() && !chart.getAxisSet().getYAxis(series.getYAxisId()).isLogScaleEnabled())
         {
            if (series.getType() == SeriesType.BAR)
            {
               if (stackRiserPosition == -1)
               {
                  stackRiserPosition = riserCnt;
                  riserCnt++;
               }
               ((BarSeries)series).setRiserIndex(((Axis)xAxis).getNumRisers() + stackRiserPosition);
               setStackSeries(stackBarSeries, series);
            }
            else if (series.getType() == SeriesType.LINE)
            {
               setStackSeries(stackLineSeries, series);
            }
         }
         else
         {
            if (series.getType() == SeriesType.BAR)
            {
               ((BarSeries)series).setRiserIndex(((Axis)xAxis).getNumRisers() + riserCnt++);
            }
         }
      }

      ((Axis)xAxis).setNumRisers(((Axis)xAxis).getNumRisers() + riserCnt);
   }

   /**
    * Sets the stack series.
    * 
    * @param stackSeries the stack series
    * @param series the series
    */
   private void setStackSeries(double[] stackSeries, ISeries series)
   {
      double[] ySeries = series.getYSeries();
      if (ySeries == null || stackSeries == null)
      {
         return;
      }

      for(int i = 0; i < stackSeries.length; i++)
      {
         if (i >= ySeries.length)
         {
            break;
         }
         stackSeries[i] = new BigDecimal(new Double(stackSeries[i]).toString()).add(new BigDecimal(new Double(ySeries[i]).toString())).doubleValue();
      }
      double[] copiedStackSeries = new double[stackSeries.length];
      System.arraycopy(stackSeries, 0, copiedStackSeries, 0, stackSeries.length);
      ((Series)series).setStackSeries(copiedStackSeries);
   }
}
