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
package org.netxms.client.datacollection;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.DataType;

/**
 * Data Collection Item representation
 */
public class DataCollectionItem extends DataCollectionObject
{
	// DCI specific flags
   public static final int DCF_DETECT_ANOMALIES        = 0x00001;
	public static final int DCF_ALL_THRESHOLDS          = 0x00002;
	public static final int DCF_RAW_VALUE_OCTET_STRING  = 0x00004;
	public static final int DCF_SHOW_ON_OBJECT_TOOLTIP  = 0x00008;
	public static final int DCF_AGGREGATE_FUNCTION_MASK = 0x00070;
   public static final int DCF_CALCULATE_NODE_STATUS   = 0x00400;
   public static final int DCF_SHOW_IN_OBJECT_OVERVIEW = 0x00800;
   public static final int DCF_MULTIPLIERS_MASK        = 0x30000;
   public static final int DCF_STORE_CHANGES_ONLY      = 0x40000;

	// Aggregation functions
	public static final int DCF_FUNCTION_SUM = 0;
	public static final int DCF_FUNCTION_AVG = 1;
	public static final int DCF_FUNCTION_MIN = 2;
	public static final int DCF_FUNCTION_MAX = 3;	

	// Delta calculation
	public static final int DELTA_NONE = 0;
	public static final int DELTA_SIMPLE = 1;
	public static final int DELTA_AVERAGE_PER_SECOND = 2;
	public static final int DELTA_AVERAGE_PER_MINUTE = 3;

	// SNMP raw types for input transformation
	public static final int SNMP_RAWTYPE_NONE = 0;
	public static final int SNMP_RAWTYPE_INT32 = 1;
	public static final int SNMP_RAWTYPE_UINT32 = 2;
	public static final int SNMP_RAWTYPE_INT64 = 3;
	public static final int SNMP_RAWTYPE_UINT64 = 4;
	public static final int SNMP_RAWTYPE_DOUBLE = 5;
	public static final int SNMP_RAWTYPE_IP_ADDR = 6;
	public static final int SNMP_RAWTYPE_MAC_ADDR = 7;
   public static final int SNMP_RAWTYPE_IP6_ADDR = 8;

	private DataType dataType;
   private DataType transformedDataType;
	private int deltaCalculation;
	private int sampleCount;
	private int multiplier;
	private String unitName;
	private int snmpRawValueType;
   private List<Threshold> thresholds;
	private String predictionEngine;
   private int allThresholdsRearmEvent;

	/**
	 * Create data collection item object from NXCP message
	 * 
	 * @param owner Owning configuration object
	 * @param msg NXCP message
	 */
	public DataCollectionItem(final DataCollectionConfiguration owner, final NXCPMessage msg)
	{
		super(owner, msg);

		dataType = DataType.getByValue(msg.getFieldAsInt16(NXCPCodes.VID_DCI_DATA_TYPE));
      transformedDataType = DataType.getByValue(msg.getFieldAsInt16(NXCPCodes.VID_TRANSFORMED_DATA_TYPE));
		deltaCalculation = msg.getFieldAsInt32(NXCPCodes.VID_DCI_DELTA_CALCULATION);
		sampleCount = msg.getFieldAsInt32(NXCPCodes.VID_SAMPLE_COUNT);
		multiplier = msg.getFieldAsInt32(NXCPCodes.VID_MULTIPLIER);
		unitName = msg.getFieldAsString(NXCPCodes.VID_UNITS_NAME);
		snmpRawValueType = msg.getFieldAsInt32(NXCPCodes.VID_SNMP_RAW_VALUE_TYPE);
      allThresholdsRearmEvent = msg.getFieldAsInt32(NXCPCodes.VID_DEACTIVATION_EVENT);
		predictionEngine = msg.getFieldAsString(NXCPCodes.VID_NPE_NAME);

		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_THRESHOLDS);
		thresholds = new ArrayList<Threshold>(count);
		long fieldId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < count; i++, fieldId += 20)
		{
			thresholds.add(new Threshold(msg, fieldId));
		}
	}

	/**
    * Constructor for new data collection items.
    *
    * @param owner Owning configuration object
    * @param nodeId Owning node ID
    * @param id Identifier assigned to new item
    */
   protected DataCollectionItem(DataCollectionConfiguration owner, long nodeId, long id)
	{
      super(owner, nodeId, id);
		dataType = DataType.INT32;
      transformedDataType = DataType.NULL;
		deltaCalculation = DELTA_NONE;
		sampleCount = 0;
		multiplier = 0;
		unitName = null;
		snmpRawValueType = SNMP_RAWTYPE_NONE;
      allThresholdsRearmEvent = 0;
		predictionEngine = "";
		thresholds = new ArrayList<Threshold>(0);
	}

   /**
    * Constructor for new data collection items.
    *
    * @param owner Owning configuration object
    * @param id Identifier assigned to new item
    */
   public DataCollectionItem(DataCollectionConfiguration owner, long id)
   {
      this(owner, owner.getOwnerId(), id);
   }

   /**
    * Constructor for new data collection items.
    *
    * @param nodeId Owning node ID
    * @param id Identifier assigned to new item
    */
   public DataCollectionItem(long nodeId, long id)
   {
      this(null, nodeId, id);
   }

   /**
    * Default constructor (intended for deserialization)
    */
   protected DataCollectionItem()
   {
      this(null, 0, 0);
   }

   /**
    * Object copy constructor
    *
    * @param owner object owner
    * @param src object to copy
    */
   protected DataCollectionItem(DataCollectionConfiguration owner, DataCollectionItem src)
   {
      super(owner, src);
      dataType = src.dataType;
      transformedDataType = src.transformedDataType;
      deltaCalculation = src.deltaCalculation;
      sampleCount = src.sampleCount;
      multiplier = src.multiplier;
      unitName = src.unitName;
      snmpRawValueType = src.snmpRawValueType;
      allThresholdsRearmEvent = src.allThresholdsRearmEvent;
      thresholds = new ArrayList<Threshold>(src.thresholds);
      predictionEngine = src.predictionEngine;
   }

   /**
	 * Fill NXCP message with item's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		super.fillMessage(msg);

		msg.setFieldInt16(NXCPCodes.VID_DCOBJECT_TYPE, DCO_TYPE_ITEM);
		msg.setFieldInt16(NXCPCodes.VID_DCI_DATA_TYPE, dataType.getValue());
      msg.setFieldInt16(NXCPCodes.VID_TRANSFORMED_DATA_TYPE, transformedDataType.getValue());
		msg.setFieldInt16(NXCPCodes.VID_DCI_DELTA_CALCULATION, deltaCalculation);
		msg.setFieldInt16(NXCPCodes.VID_SAMPLE_COUNT, sampleCount);
		msg.setFieldInt16(NXCPCodes.VID_SNMP_RAW_VALUE_TYPE, snmpRawValueType);
      msg.setFieldInt32(NXCPCodes.VID_DEACTIVATION_EVENT, allThresholdsRearmEvent);
		msg.setField(NXCPCodes.VID_NPE_NAME, predictionEngine);
		msg.setFieldInt32(NXCPCodes.VID_MULTIPLIER, multiplier);
		if (unitName != null)
			msg.setField(NXCPCodes.VID_UNITS_NAME, unitName);

		msg.setFieldInt32(NXCPCodes.VID_NUM_THRESHOLDS, thresholds.size());
		long varId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < thresholds.size(); i++, varId +=10)
		{
			thresholds.get(i).fillMessage(msg, varId);
		}
	}

	/**
	 * @return the dataType
	 */
	public DataType getDataType()
	{
		return dataType;
	}

	/**
	 * @param dataType the dataType to set
	 */
	public void setDataType(DataType dataType)
	{
		this.dataType = dataType;
	}

	/**
    * @return the transformedDataType
    */
   public DataType getTransformedDataType()
   {
      return transformedDataType;
   }

   /**
    * @param transformedDataType the transformedDataType to set
    */
   public void setTransformedDataType(DataType transformedDataType)
   {
      this.transformedDataType = transformedDataType;
   }

   /**
    * @return the deltaCalculation
    */
	public int getDeltaCalculation()
	{
		return deltaCalculation;
	}

	/**
	 * @param deltaCalculation the deltaCalculation to set
	 */
	public void setDeltaCalculation(int deltaCalculation)
	{
		this.deltaCalculation = deltaCalculation;
	}

	/**
	 * @return the processAllThresholds
	 */
	public boolean isProcessAllThresholds()
	{
		return (flags & DCF_ALL_THRESHOLDS) != 0;
	}

	/**
	 * @param processAllThresholds the processAllThresholds to set
	 */
	public void setProcessAllThresholds(boolean processAllThresholds)
	{
		if (processAllThresholds)
			flags |= DCF_ALL_THRESHOLDS;
		else
			flags &= ~DCF_ALL_THRESHOLDS;
	}

	/**
	 * @return State of DCF_RAW_VALUE_OCTET_STRING flag
	 */
	public boolean isSnmpRawValueInOctetString()
	{
		return (flags & DCF_RAW_VALUE_OCTET_STRING) != 0;
	}

	/**
	 * Set state of DCF_RAW_VALUE_OCTET_STRING flag
	 * 
	 * @param enable true to enable
	 */
	public void setSnmpRawValueInOctetString(boolean enable)
	{
		if (enable)
			flags |= DCF_RAW_VALUE_OCTET_STRING;
		else
			flags &= ~DCF_RAW_VALUE_OCTET_STRING;
	}

	/**
	 * @return true of DCI should be shown on object tooltip
	 */
	public boolean isShowOnObjectTooltip()
	{
		return (flags & DCF_SHOW_ON_OBJECT_TOOLTIP) != 0;
	}

	/**
	 * @param show indicator if DCI should be shown on object tooltip
	 */
	public void setShowOnObjectTooltip(boolean show)
	{
		if (show)
			flags |= DCF_SHOW_ON_OBJECT_TOOLTIP;
		else
			flags &= ~DCF_SHOW_ON_OBJECT_TOOLTIP;
	}

   /**
    * @return true of DCI should be shown in object overview
    */
   public boolean isShowInObjectOverview()
   {
      return (flags & DCF_SHOW_IN_OBJECT_OVERVIEW) != 0;
   }

   /**
    * @param show indicator if DCI should be shown in object overview
    */
   public void setShowInObjectOverview(boolean show)
   {
      if (show)
         flags |= DCF_SHOW_IN_OBJECT_OVERVIEW;
      else
         flags &= ~DCF_SHOW_IN_OBJECT_OVERVIEW;
   }

	/**
	 * @return aggregation function
	 */
	public int getAggregationFunction()
	{
		return (int)((flags & DCF_AGGREGATE_FUNCTION_MASK) >> 4);
	}

	/**
	 * @param func The function to set
	 */
	public void setAggregationFunction(int func)
	{
		flags = (flags & ~DCF_AGGREGATE_FUNCTION_MASK) | ((func & 0x07) << 4);
	}

	/**
	 * @return the multiplier
	 */
	public int getMultiplier()
	{
		return multiplier;
	}

	/**
	 * @param multiplier the multiplier to set
	 */
	public void setMultiplier(int multiplier)
	{
		this.multiplier = multiplier;
	}

	/**
	 * @return the customUnitName
	 */
	public String getUnitName()
	{
		return unitName;
	}

	/**
	 * @param unitName the customUnitName to set
	 */
	public void setUnitName(String unitName)
	{
		this.unitName = unitName;
	}

	/**
	 * @return the thresholds
	 */
   public List<Threshold> getThresholds()
	{
		return thresholds;
	}

   /**
    * @return the allThresholdsRearmEvent
    */
   public int getAllThresholdsRearmEvent()
   {
      return allThresholdsRearmEvent;
   }

   /**
    * @param allThresholdsRearmEvent the allThresholdsRearmEvent to set
    */
   public void setAllThresholdsRearmEvent(int allThresholdsRearmEvent)
   {
      this.allThresholdsRearmEvent = allThresholdsRearmEvent;
   }

	/**
	 * @return the snmpRawValueType
	 */
	public int getSnmpRawValueType()
	{
		return snmpRawValueType;
	}

	/**
	 * @param snmpRawValueType the snmpRawValueType to set
	 */
	public void setSnmpRawValueType(int snmpRawValueType)
	{
		this.snmpRawValueType = snmpRawValueType;
	}

	/**
	 * @return the sampleCount
	 */
	public int getSampleCount()
	{
		return sampleCount;
	}

	/**
	 * @param sampleCount the sampleCount to set
	 */
	public void setSampleCount(int sampleCount)
	{
		this.sampleCount = sampleCount;
	}
	
	/**
    * @return State of DCF_CALCULATE_NODE_STATUS flag
    */
   public boolean isUsedForNodeStatusCalculation()
   {
      return (flags & DCF_CALCULATE_NODE_STATUS) != 0;
   }
   
   /**
    * Enable or disable usage of this DCI for node status calculation
    * 
    * @param enable true to enable
    */
   public void setUsedForNodeStatusCalculation(boolean enable)
   {
      if (enable)
         flags |= DCF_CALCULATE_NODE_STATUS;
      else
         flags &= ~DCF_CALCULATE_NODE_STATUS;
   }

   /**
    * @return State of DCF_DETECT_ANOMALIES flag
    */
   public boolean isAnomalyDetectionEnabled()
   {
      return (flags & DCF_DETECT_ANOMALIES) != 0;
   }

   /**
    * Enable or disable anomaly detection for this DCI
    * 
    * @param enable true to enable
    */
   public void setAnomalyDetectionEnabled(boolean enable)
   {
      if (enable)
         flags |= DCF_DETECT_ANOMALIES;
      else
         flags &= ~DCF_DETECT_ANOMALIES;
   }

   /**
    * @return the predictionEngine
    */
   public String getPredictionEngine()
   {
      return predictionEngine;
   }

   /**
    * @param predictionEngine the predictionEngine to set
    */
   public void setPredictionEngine(String predictionEngine)
   {
      this.predictionEngine = predictionEngine;
   }

   /**
    * Get multiplier usage mode (DEFAULT, YES, or NO).
    * 
    * @return multiplier usage mode (DEFAULT, YES, or NO).
    */
   public int getMultipliersSelection()
   {
      return (int)((flags & DCF_MULTIPLIERS_MASK) >> 16);
   }

   /**
    * Set multiplier usage mode.
    *
    * @param mode multiplier usage mode (DEFAULT, YES, or NO).
    */
   public void setMultiplierSelection(int mode)
   {
      flags = (flags & ~DCF_MULTIPLIERS_MASK) | ((mode & 0x03) << 16);
   }
   
   /**
    * @return the storeChangesOnly.
    */
   public boolean isStoreChangesOnly()
   {
      return (flags & DCF_STORE_CHANGES_ONLY) != 0;
   }
   
   /**
    * @param storeChangesOnly the storeChangesOnly to set
    */
   public void setStoreChangesOnly(boolean storeChangesOnly)
   {
      if (storeChangesOnly)
         flags |= DCF_STORE_CHANGES_ONLY;
      else
         flags &= ~DCF_STORE_CHANGES_ONLY;
   }
}
