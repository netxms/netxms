/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Data Collection Item representation
 */
public class DataCollectionItem extends DataCollectionObject
{
	// DCI specific flags
	public static final int DCF_ALL_THRESHOLDS = 0x0002;
	public static final int DCF_RAW_VALUE_OCTET_STRING = 0x0004;
	public static final int DCF_SHOW_ON_OBJECT_TOOLTIP = 0x0008;
	public static final int DCF_AGGREGATE_FUNCTION_MASK = 0x0070;
   public static final int DCF_CALCULATE_NODE_STATUSS = 0x0400;
	
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
	
	// Instance discovery methods
	public static final int IDM_NONE = 0;
	public static final int IDM_AGENT_LIST = 1;
	public static final int IDM_AGENT_TABLE = 2;
	public static final int IDM_SNMP_WALK_VALUES = 3;
	public static final int IDM_SNMP_WALK_OIDS = 4;
	
	private int dataType;
	private int deltaCalculation;
	private int sampleCount;
	private String instance;
	private int baseUnits;
	private int multiplier;
	private String customUnitName;
	private int snmpRawValueType;
	private ArrayList<Threshold> thresholds;
	private int instanceDiscoveryMethod;
	private String instanceDiscoveryData;
	private String instanceDiscoveryFilter;
	
	/**
	 * Create data collection item object from NXCP message
	 * 
	 * @param owner Owning configuration object
	 * @param msg NXCP message
	 */
	protected DataCollectionItem(final DataCollectionConfiguration owner, final NXCPMessage msg)
	{
		super(owner, msg);
		
		dataType = msg.getFieldAsInt32(NXCPCodes.VID_DCI_DATA_TYPE);
		deltaCalculation = msg.getFieldAsInt32(NXCPCodes.VID_DCI_DELTA_CALCULATION);
		sampleCount = msg.getFieldAsInt32(NXCPCodes.VID_SAMPLE_COUNT);
		instance = msg.getFieldAsString(NXCPCodes.VID_INSTANCE);
		baseUnits = msg.getFieldAsInt32(NXCPCodes.VID_BASE_UNITS);
		multiplier = msg.getFieldAsInt32(NXCPCodes.VID_MULTIPLIER);
		customUnitName = msg.getFieldAsString(NXCPCodes.VID_CUSTOM_UNITS_NAME);
		snmpRawValueType = msg.getFieldAsInt32(NXCPCodes.VID_SNMP_RAW_VALUE_TYPE);
		instanceDiscoveryMethod = msg.getFieldAsInt32(NXCPCodes.VID_INSTD_METHOD);
		instanceDiscoveryData = msg.getFieldAsString(NXCPCodes.VID_INSTD_DATA);
		instanceDiscoveryFilter = msg.getFieldAsString(NXCPCodes.VID_INSTD_FILTER);
		
		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_THRESHOLDS);
		thresholds = new ArrayList<Threshold>(count);
		long varId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < count; i++, varId += 20)
		{
			thresholds.add(new Threshold(msg, varId));
		}
	}

	/**
	 * Constructor for new data collection items.
	 * 
	 * @param owner Owning configuration object
	 * @param id Identifier assigned to new item
	 */
	protected DataCollectionItem(final DataCollectionConfiguration owner, long id)
	{
		super(owner, id);
		
		dataType = DT_INT;
		deltaCalculation = DELTA_NONE;
		sampleCount = 0;
		instance = "";
		baseUnits = 0;
		multiplier = 0;
		customUnitName = null;
		snmpRawValueType = SNMP_RAWTYPE_NONE;
		thresholds = new ArrayList<Threshold>(0);
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
		msg.setFieldInt16(NXCPCodes.VID_DCI_DATA_TYPE, dataType);
		msg.setFieldInt16(NXCPCodes.VID_DCI_DELTA_CALCULATION, deltaCalculation);
		msg.setFieldInt16(NXCPCodes.VID_SAMPLE_COUNT, sampleCount);
		msg.setField(NXCPCodes.VID_INSTANCE, instance);
		msg.setFieldInt16(NXCPCodes.VID_SNMP_RAW_VALUE_TYPE, snmpRawValueType);
		msg.setFieldInt16(NXCPCodes.VID_BASE_UNITS, baseUnits);
		msg.setFieldInt32(NXCPCodes.VID_MULTIPLIER, multiplier);
		if (customUnitName != null)
			msg.setField(NXCPCodes.VID_CUSTOM_UNITS_NAME, customUnitName);
		
		msg.setFieldInt16(NXCPCodes.VID_INSTD_METHOD, instanceDiscoveryMethod);
		if (instanceDiscoveryData != null)
			msg.setField(NXCPCodes.VID_INSTD_DATA, instanceDiscoveryData);
		if (instanceDiscoveryFilter != null)
			msg.setField(NXCPCodes.VID_INSTD_FILTER, instanceDiscoveryFilter);
		
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
	public int getDataType()
	{
		return dataType;
	}

	/**
	 * @param dataType the dataType to set
	 */
	public void setDataType(int dataType)
	{
		this.dataType = dataType;
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
	 * @param enable
	 */
	public void setSnmpRawValueInOctetString(boolean enable)
	{
		if (enable)
			flags |= DCF_RAW_VALUE_OCTET_STRING;
		else
			flags &= ~DCF_RAW_VALUE_OCTET_STRING;
	}

	/**
	 * @return the processAllThresholds
	 */
	public boolean isShowOnObjectTooltip()
	{
		return (flags & DCF_SHOW_ON_OBJECT_TOOLTIP) != 0;
	}

	/**
	 * @param processAllThresholds the processAllThresholds to set
	 */
	public void setShowOnObjectTooltip(boolean show)
	{
		if (show)
			flags |= DCF_SHOW_ON_OBJECT_TOOLTIP;
		else
			flags &= ~DCF_SHOW_ON_OBJECT_TOOLTIP;
	}

	/**
	 * @return aggregation function
	 */
	public int getAggregationFunction()
	{
		return (flags & DCF_AGGREGATE_FUNCTION_MASK) >> 4;
	}

	/**
	 * @param func
	 */
	public void setAggregationFunction(int func)
	{
		flags = (flags & ~DCF_AGGREGATE_FUNCTION_MASK) | ((func & 0x07) << 4);
	}

	/**
	 * @return the instance
	 */
	public String getInstance()
	{
		return instance;
	}

	/**
	 * @param instance the instance to set
	 */
	public void setInstance(String instance)
	{
		this.instance = instance;
	}

	/**
	 * @return the baseUnits
	 */
	public int getBaseUnits()
	{
		return baseUnits;
	}

	/**
	 * @param baseUnits the baseUnits to set
	 */
	public void setBaseUnits(int baseUnits)
	{
		this.baseUnits = baseUnits;
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
	public String getCustomUnitName()
	{
		return customUnitName;
	}

	/**
	 * @param customUnitName the customUnitName to set
	 */
	public void setCustomUnitName(String customUnitName)
	{
		this.customUnitName = customUnitName;
	}

	/**
	 * @return the thresholds
	 */
	public ArrayList<Threshold> getThresholds()
	{
		return thresholds;
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
	 * @return the instanceDiscoveryMethod
	 */
	public final int getInstanceDiscoveryMethod()
	{
		return instanceDiscoveryMethod;
	}

	/**
	 * @param instanceDiscoveryMethod the instanceDiscoveryMethod to set
	 */
	public final void setInstanceDiscoveryMethod(int instanceDiscoveryMethod)
	{
		this.instanceDiscoveryMethod = instanceDiscoveryMethod;
	}

	/**
	 * @return the instanceDiscoveryData
	 */
	public final String getInstanceDiscoveryData()
	{
		return instanceDiscoveryData;
	}

	/**
	 * @param instanceDiscoveryData the instanceDiscoveryData to set
	 */
	public final void setInstanceDiscoveryData(String instanceDiscoveryData)
	{
		this.instanceDiscoveryData = instanceDiscoveryData;
	}

	/**
	 * @return the instanceDiscoveryFilter
	 */
	public final String getInstanceDiscoveryFilter()
	{
		return instanceDiscoveryFilter;
	}

	/**
	 * @param instanceDiscoveryFilter the instanceDiscoveryFilter to set
	 */
	public final void setInstanceDiscoveryFilter(String instanceDiscoveryFilter)
	{
		this.instanceDiscoveryFilter = instanceDiscoveryFilter;
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
    * @return State of DCF_CALCULATE_NODE_STATUSS flag
    */
   public boolean isUsedForNodeStatusCalculation()
   {
      return (flags & DCF_CALCULATE_NODE_STATUSS) != 0;
   }
   
   public void setUsedForNodeStatusCalculation(boolean enable)
   {
      if(enable)
         flags |= DCF_CALCULATE_NODE_STATUSS;
      else
         flags &= ~DCF_CALCULATE_NODE_STATUSS;
   }

}
