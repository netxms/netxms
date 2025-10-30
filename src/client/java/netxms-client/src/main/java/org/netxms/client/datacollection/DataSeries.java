/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.client.datacollection;

import java.util.ArrayList;
import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.Severity;

/**
 * Class to hold series of collected DCI data
 */
public class DataSeries
{
	private long nodeId;
	private long dciId;
	private String dciDescription;
	private String dciName;
	private DataType dataType;
	private ArrayList<DciDataRow> values = new ArrayList<DciDataRow>();
   private Severity activeThresholdSeverity;
   private MeasurementUnit measurementUnits; 
   private int multiplierPower;
   private int useMultiplier = 0;  // 0 - auto, 1 - use, 2 - do not use
   private Threshold[] thresholds = null;

   /**
    * Create empty data series
    */
   public DataSeries()
   {
      this.nodeId = 0;
      this.dciId = 0;
      dataType = DataType.FLOAT;
      activeThresholdSeverity = Severity.NORMAL;
   }

	/**
	 * @param nodeId The node ID
	 * @param dciId The dci ID
	 */
	public DataSeries(long nodeId, long dciId)
	{
		this.nodeId = nodeId;
		this.dciId = dciId;
		this.dataType = DataType.INT32;
      activeThresholdSeverity = Severity.NORMAL;
	}

   /**
    * Create data series with single value
    * 
    * @param value initial value
    */
   public DataSeries(double value)
   {
      dataType = DataType.FLOAT;
      values.add(new DciDataRow(new Date(), Double.valueOf(value)));
      activeThresholdSeverity = Severity.NORMAL;
   }

   /**
    * Copy constructor
    * 
    * @param src source object to copy
    */
   public DataSeries(DataSeries src)
   {
      this.nodeId = src.nodeId;
      this.dciId = src.dciId;
      this.dciDescription = src.dciDescription;
      this.dciName = src.dciName;
      this.dataType = src.dataType;
      this.values.addAll(src.values);
      this.activeThresholdSeverity = src.activeThresholdSeverity;
      this.measurementUnits = src.measurementUnits;
      this.multiplierPower = src.multiplierPower;
      this.useMultiplier = src.useMultiplier;
      this.thresholds = src.thresholds;
   }

   /**
    * Update data series from NXCP message
    * 
    * @param msg NXCP message
    */
   public void updateFromMessage(NXCPMessage msg)
   {
      dciName = msg.getFieldAsString(NXCPCodes.VID_DCI_NAME);
      dciDescription = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
      activeThresholdSeverity = Severity.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_CURRENT_SEVERITY));
      multiplierPower = msg.getFieldAsInt32(NXCPCodes.VID_MULTIPLIER);
      useMultiplier = msg.getFieldAsInt32(NXCPCodes.VID_USE_MULTIPLIER);

      String units = msg.getFieldAsString(NXCPCodes.VID_UNITS_NAME);
      measurementUnits = ((units != null) && !units.isBlank()) ? new MeasurementUnit(units) : null;

      int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_THRESHOLDS);
      thresholds = new Threshold[count];
      long fieldId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
      for(int i = 0; i < count; i++, fieldId += 20)
         thresholds[i] = new Threshold(msg, fieldId);
   }

   /**
    * Set data type.
    *
    * @param dataType new data type
    */
   public void setDataType(DataType dataType)
   {
      this.dataType = dataType;
   }

   /**
    * @return the nodeId
    */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the dciId
	 */
	public long getDciId()
	{
		return dciId;
	}

	/**
	 * @return the values
	 */
	public DciDataRow[] getValues()
	{
		return values.toArray(new DciDataRow[values.size()]);
	}
		
	/**
	 * Get last added value
	 * 
	 * @return last added value
	 */
	public DciDataRow getLastValue()
	{
		return (values.size() > 0) ? values.get(values.size() - 1) : null;
	}
	
   /**
    * Get current value as double
    *
    * @return current value as double
    */
   public double getCurrentValue()
   {
      return (values.size() > 0) ? values.get(values.size() - 1).getValueAsDouble() : 0;
   }

   /**
    * Get minimum value for series.
    *
    * @return minimum value for series
    */
   public double getMinValue()
   {
      if (values.size() == 0)
         return 0;
      double minValue = values.get(0).getValueAsDouble();
      for(int i = 1; i < values.size(); i++)
      {
         double curr = values.get(i).getValueAsDouble();
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
      if (values.size() == 0)
         return 0;
      double maxValue = values.get(0).getValueAsDouble();
      for(int i = 1; i < values.size(); i++)
      {
         double curr = values.get(i).getValueAsDouble();
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
      if (values.size() == 0)
         return 0;
      double sum = values.get(0).getValueAsDouble();
      for(int i = 1; i < values.size(); i++)
      {
         sum += values.get(i).getValueAsDouble();
      }
      return sum / values.size();
   }

   /**
    * Get current value as string
    *
    * @return current value as string
    */
   public String getCurrentValueAsString()
   {
      return (values.size() > 0) ? values.get(values.size() - 1).getValueAsString() : "";
   }
	
	/**
	 * Add new value
	 * 
	 * @param row DciDataRow
	 */
	public void addDataRow(DciDataRow row)
	{
		values.add(row);
	}

	/**
	 * @return the dataType
	 */
	public DataType getDataType()
	{
		return dataType;
	}

	/**
	 * Invert values
	 */
	public void invert()
	{
	   for(DciDataRow r : values)
	      r.invert();
	}

   /**
    * Get current threshold severity
    * 
    * @return current threshold severity
    */
   public Severity getActiveThresholdSeverity()
   {
      return activeThresholdSeverity;
   }

   /**
    * Get measurement units
    * 
    * @return the measurementUnit
    */
   public MeasurementUnit getMeasurementUnit()
   {
      return measurementUnits;
   }

   /**
    * Get multiplier power
    *    
    * @return the multiplierPower
    */
   public int getMultiplierPower()
   {
      return multiplierPower;
   }

   /**
    * Get multiplier usage flag
    * 
    * @return the useMultiplier
    */
   public int getUseMultiplier()
   {
      return useMultiplier;
   }
   
   /**
    * Create data formatter for this data series
    * 
    * @return data formatter
    */
   public DataFormatter getDataFormatter()
   {
      return new DataFormatter(this);
   }

   /**
    * @return
    */
   public Threshold[] getThresholds()
   {
      return thresholds;
   }

   /**
    * Get DCI description.
    *
    * @return DCI description
    */
   public String getDciDescription()
   {
      return dciDescription;
   }

   /**
    * Get DCI name.
    *
    * @return DCI name
    */
   public String getDciName()
   {
      return dciName;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "DciData [nodeId=" + nodeId + ", dciId=" + dciId + ", dciName=" + dciName + ", dciDescription=" + dciDescription + 
            ", dataType=" + dataType + ", valuesSize=" + values.size() +
            ", activeThresholdSeverity=" + activeThresholdSeverity + ", multiplierPower=" + multiplierPower + 
            ", useMultiplier=" + useMultiplier + ", measurementUnits=" + measurementUnits + ", thresholds= " + thresholds + "]";
   }
}
