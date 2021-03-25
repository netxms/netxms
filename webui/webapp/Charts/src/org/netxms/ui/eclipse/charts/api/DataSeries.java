/**
 * 
 */
package org.netxms.ui.eclipse.charts.api;

import java.util.Arrays;
import java.util.Date;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;

/**
 * @author victor
 *
 */
public class DataSeries
{
   private DataType dataType;
   private DciDataRow[] values;
   private Severity currentThresholdSeverity;

   /**
    * Create empty data series
    */
   public DataSeries()
   {
      dataType = DataType.FLOAT;
      values = new DciDataRow[0];
      currentThresholdSeverity = Severity.NORMAL;
   }

   /**
    * 
    */
   public DataSeries(DciData data)
   {
      dataType = data.getDataType();
      values = data.getValues();
      currentThresholdSeverity = Severity.NORMAL;
   }

   public DataSeries(DciDataRow value)
   {
      this(value, DataType.FLOAT, Severity.NORMAL);
   }

   public DataSeries(DciDataRow value, DataType dataType)
   {
      this(value, dataType, Severity.NORMAL);
   }

   public DataSeries(DciDataRow value, DataType dataType, Severity currentThresholdSeverity)
   {
      this.dataType = dataType;
      values = new DciDataRow[1];
      values[0] = value;
      this.currentThresholdSeverity = currentThresholdSeverity;
   }

   public DataSeries(double value)
   {
      dataType = DataType.FLOAT;
      values = new DciDataRow[1];
      values[0] = new DciDataRow(new Date(), Double.valueOf(value));
      currentThresholdSeverity = Severity.NORMAL;
   }

   /**
    * @return the dataType
    */
   public DataType getDataType()
   {
      return dataType;
   }

   /**
    * Get current value as double
    *
    * @return current value as double
    */
   public double getCurrentValue()
   {
      return (values.length > 0) ? values[0].getValueAsDouble() : 0;
   }

   /**
    * Get current value as string
    *
    * @return current value as string
    */
   public String getCurrentValueAsString()
   {
      return (values.length > 0) ? values[0].getValueAsString() : "";
   }

   /**
    * Get all values
    *
    * @return all values
    */
   public DciDataRow[] getValues()
   {
      return values;
   }

   /**
    * Get minimum value for series.
    *
    * @return minimum value for series
    */
   public double getMinValue()
   {
      if (values.length == 0)
         return 0;
      double minValue = values[0].getValueAsDouble();
      for(int i = 1; i < values.length; i++)
      {
         double curr = values[i].getValueAsDouble();
         if (curr < minValue)
            minValue = curr;
      }
      return minValue;
   }

   /**
    * Get maximum value for series.
    *
    * @return maximum value for series
    */
   public double getMaxValue()
   {
      if (values.length == 0)
         return 0;
      double maxValue = values[0].getValueAsDouble();
      for(int i = 1; i < values.length; i++)
      {
         double curr = values[i].getValueAsDouble();
         if (curr > maxValue)
            maxValue = curr;
      }
      return maxValue;
   }

   /**
    * Get average value for series.
    *
    * @return average value for series
    */
   public double getAverageValue()
   {
      if (values.length == 0)
         return 0;
      double sum = values[0].getValueAsDouble();
      for(int i = 1; i < values.length; i++)
      {
         sum += values[i].getValueAsDouble();
      }
      return sum / values.length;
   }

   /**
    * Get severity of active threshold.
    *
    * @return severity of active threshold
    */
   public Severity getActiveThresholdSeverity()
   {
      return currentThresholdSeverity;
   }

   /**
    * Set severity of active threshold.
    *
    * @param severity severity of active threshold
    */
   public void setActiveThresholdSeverity(Severity severity)
   {
      currentThresholdSeverity = severity;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "DataSeries [dataType=" + dataType + ", values=" + Arrays.toString(values) + ", currentThresholdSeverity=" + currentThresholdSeverity + "]";
   }
}
