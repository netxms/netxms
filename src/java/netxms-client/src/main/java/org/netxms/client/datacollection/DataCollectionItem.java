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
	private String transformationScript;
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
		
		dataType = msg.getVariableAsInteger(NXCPCodes.VID_DCI_DATA_TYPE);
		deltaCalculation = msg.getVariableAsInteger(NXCPCodes.VID_DCI_DELTA_CALCULATION);
		sampleCount = msg.getVariableAsInteger(NXCPCodes.VID_SAMPLE_COUNT);
		transformationScript = msg.getVariableAsString(NXCPCodes.VID_TRANSFORMATION_SCRIPT);
		instance = msg.getVariableAsString(NXCPCodes.VID_INSTANCE);
		baseUnits = msg.getVariableAsInteger(NXCPCodes.VID_BASE_UNITS);
		multiplier = msg.getVariableAsInteger(NXCPCodes.VID_MULTIPLIER);
		customUnitName = msg.getVariableAsString(NXCPCodes.VID_CUSTOM_UNITS_NAME);
		snmpRawValueType = msg.getVariableAsInteger(NXCPCodes.VID_SNMP_RAW_VALUE_TYPE);
		instanceDiscoveryMethod = msg.getVariableAsInteger(NXCPCodes.VID_INSTD_METHOD);
		instanceDiscoveryData = msg.getVariableAsString(NXCPCodes.VID_INSTD_DATA);
		instanceDiscoveryFilter = msg.getVariableAsString(NXCPCodes.VID_INSTD_FILTER);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_THRESHOLDS);
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
		transformationScript = null;
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
		
		msg.setVariableInt16(NXCPCodes.VID_DCOBJECT_TYPE, DCO_TYPE_ITEM);
		msg.setVariableInt16(NXCPCodes.VID_DCI_DATA_TYPE, dataType);
		msg.setVariableInt16(NXCPCodes.VID_DCI_DELTA_CALCULATION, deltaCalculation);
		msg.setVariableInt16(NXCPCodes.VID_SAMPLE_COUNT, sampleCount);
		msg.setVariable(NXCPCodes.VID_INSTANCE, instance);
		msg.setVariable(NXCPCodes.VID_TRANSFORMATION_SCRIPT, transformationScript);
		msg.setVariableInt16(NXCPCodes.VID_SNMP_RAW_VALUE_TYPE, snmpRawValueType);
		msg.setVariableInt16(NXCPCodes.VID_BASE_UNITS, baseUnits);
		msg.setVariableInt32(NXCPCodes.VID_MULTIPLIER, multiplier);
		if (customUnitName != null)
			msg.setVariable(NXCPCodes.VID_CUSTOM_UNITS_NAME, customUnitName);
		
		msg.setVariableInt16(NXCPCodes.VID_INSTD_METHOD, instanceDiscoveryMethod);
		if (instanceDiscoveryData != null)
			msg.setVariable(NXCPCodes.VID_INSTD_DATA, instanceDiscoveryData);
		if (instanceDiscoveryFilter != null)
			msg.setVariable(NXCPCodes.VID_INSTD_FILTER, instanceDiscoveryFilter);
		
		msg.setVariableInt32(NXCPCodes.VID_NUM_THRESHOLDS, thresholds.size());
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
	 * @return the transformationScript
	 */
	public String getTransformationScript()
	{
		return transformationScript;
	}

	/**
	 * @param transformationScript the transformationScript to set
	 */
	public void setTransformationScript(String transformationScript)
	{
		this.transformationScript = transformationScript;
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
}
