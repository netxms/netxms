/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.DciTier;
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
   private int pollingInterval;
   private boolean storeChangesOnly;
   private boolean aggregated;
   private DciTier tierServed = DciTier.RAW;
   private DataCollectionObjectStatus dciStatus = DataCollectionObjectStatus.ACTIVE;
   private int errorCount = 0;
   private int mappingTableId = 0;

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
      this.pollingInterval = src.pollingInterval;
      this.storeChangesOnly = src.storeChangesOnly;
      this.aggregated = src.aggregated;
      this.tierServed = src.tierServed;
      this.dciStatus = src.dciStatus;
      this.errorCount = src.errorCount;
      this.mappingTableId = src.mappingTableId;
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
      pollingInterval = msg.getFieldAsInt32(NXCPCodes.VID_POLLING_INTERVAL);
      storeChangesOnly = msg.getFieldAsBoolean(NXCPCodes.VID_STORE_CHANGES_ONLY);
      dciStatus = DataCollectionObjectStatus.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_DCI_STATUS));
      errorCount = msg.getFieldAsInt32(NXCPCodes.VID_ERROR_COUNT);
      // Legacy servers that don't know about tier dispatch omit the field — default to RAW
      // rather than the AUTO value (0), which is a request-side sentinel and never served.
      tierServed = msg.isFieldPresent(NXCPCodes.VID_DCI_TIER_USED)
            ? DciTier.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_DCI_TIER_USED))
            : DciTier.RAW;

      String units = msg.getFieldAsString(NXCPCodes.VID_UNITS_NAME);
      measurementUnits = ((units != null) && !units.isBlank()) ? new MeasurementUnit(units) : null;

      mappingTableId = msg.getFieldAsInt32(NXCPCodes.VID_MAPPING_TABLE_ID);

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
   public DciDataRow getLastAddedValue()
   {
      return !values.isEmpty() ? values.get(values.size() - 1) : null;
   }

   /**
    * Get current value as double
    *
    * @return current value as double
    */
   public double getCurrentValue()
   {
      return !values.isEmpty() ? values.get(0).getValueAsDouble() : 0;
   }

   /**
    * Get current value as string
    *
    * @return current value as string
    */
   public String getCurrentValueAsString()
   {
      return !values.isEmpty() ? values.get(0).getValueAsString() : "";
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
      DciDataRow first = values.get(0);
      double minValue = first.isAggregated() ? first.getMinValue() : first.getValueAsDouble();
      for(int i = 1; i < values.size(); i++)
      {
         DciDataRow row = values.get(i);
         double curr = row.isAggregated() ? row.getMinValue() : row.getValueAsDouble();
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
      DciDataRow first = values.get(0);
      double maxValue = first.isAggregated() ? first.getMaxValue() : first.getValueAsDouble();
      for(int i = 1; i < values.size(); i++)
      {
         DciDataRow row = values.get(i);
         double curr = row.isAggregated() ? row.getMaxValue() : row.getValueAsDouble();
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
    * Get given percentile of series values (linear interpolation between closest ranks). Used for
    * 95th percentile / burstable billing overlays. Computed over processed values, matching the
    * average calculation.
    *
    * @param percentile percentile to calculate (0-100)
    * @return percentile value, or 0 if series is empty
    */
   public double getPercentile(double percentile)
   {
      int count = values.size();
      if (count == 0)
         return 0;
      if (count == 1)
         return values.get(0).getValueAsDouble();

      double[] sorted = new double[count];
      for(int i = 0; i < count; i++)
         sorted[i] = values.get(i).getValueAsDouble();
      java.util.Arrays.sort(sorted);

      double rank = (percentile / 100.0) * (count - 1);
      int lower = (int)Math.floor(rank);
      int upper = (int)Math.ceil(rank);
      if (lower == upper)
         return sorted[lower];
      double fraction = rank - lower;
      return sorted[lower] + fraction * (sorted[upper] - sorted[lower]);
   }

   /**
    * Get 95th percentile of series values (used for burstable billing overlay line and extended legend).
    *
    * @return 95th percentile value, or 0 if series is empty
    */
   public double getPercentile95()
   {
      return getPercentile(95.0);
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
    * Get polling interval in seconds.
    *
    * @return polling interval in seconds
    */
   public int getPollingInterval()
   {
      return pollingInterval;
   }

   /**
    * @return the storeChangesOnly
    */
   public boolean isStoreChangesOnly()
   {
      return storeChangesOnly;
   }

   /**
    * Check if this series contains aggregated data.
    *
    * @return true if data is aggregated
    */
   public boolean isAggregated()
   {
      return aggregated;
   }

   /**
    * Set aggregated flag.
    *
    * @param aggregated true if data is aggregated
    */
   public void setAggregated(boolean aggregated)
   {
      this.aggregated = aggregated;
   }

   /**
    * Get the tier the server actually read from to fulfill the request. May differ from the
    * caller's requested tier when AUTO selects a tier or when an explicit tier is downgraded
    * (e.g. requested HOURLY on a DCI without an hourly aggregate table).
    *
    * @return tier served (RAW for legacy servers that don't set the field)
    */
   public DciTier getTierServed()
   {
      return tierServed;
   }

   /**
    * Get DCI status.
    *
    * @return DCI status
    */
   public DataCollectionObjectStatus getDciStatus()
   {
      return dciStatus;
   }

   /**
    * Get DCI error count.
    *
    * @return DCI error count
    */
   public int getErrorCount()
   {
      return errorCount;
   }

   /**
    * Get ID of the mapping table used to translate raw values into display strings.
    *
    * @return mapping table ID, or 0 when no mapping is configured
    */
   public int getMappingTableId()
   {
      return mappingTableId;
   }

   /**
    * Check if DCI is in error state (disabled, unsupported, or has polling errors).
    *
    * @return true if DCI is in error state
    */
   public boolean isDataCollectionError()
   {
      return (dciStatus != DataCollectionObjectStatus.ACTIVE) || (errorCount > 0);
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
