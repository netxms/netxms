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
package org.netxms.client.datacollection;

import java.util.ArrayList;
import java.util.Collection;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class DataCollectionItem
{
	// Data sources
	public static final int INTERNAL = 0;
	public static final int AGENT = 1;
	public static final int SNMP = 2;
	public static final int CHECKPOINT_SNMP = 3;
	public static final int PUSH = 4;
	
	// Status
	public static final int ACTIVE = 0;
	public static final int DISABLED = 1;
	public static final int NOT_SUPPORTED = 2;
	
	// Data type
	public static final int DT_INT = 0;
	public static final int DT_UINT = 1;
	public static final int DT_INT64 = 2;
	public static final int DT_UINT64 = 3;
	public static final int DT_STRING = 4;
	public static final int DT_FLOAT = 5;
	public static final int DT_NULL = 6;
	
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
	
	// Flags
	public static final int DCF_ADVANCED_SCHEDULE = 0x0001;
	public static final int DCF_ALL_THRESHOLDS = 0x0002;
	public static final int DCF_RAW_VALUE_OCTET_STRING = 0x0004;
		
	// DCI attributes
	private DataCollectionConfiguration owner;
	private long id;
	private long templateId;
	private long resourceId;
	private long proxyNode;
	private int dataType;
	private int pollingInterval;
	private int retentionTime;
	private int origin;
	private int status;
	private int deltaCalculation;
	private int flags;
	private String name;
	private String description;
	private String transformationScript;
	private String instance;
	private String systemTag;
	private String perfTabSettings;
	private int baseUnits;
	private int multiplier;
	private String customUnitName;
	private int snmpRawValueType;
	private int snmpPort;
	private ArrayList<String> schedules;
	private ArrayList<Threshold> thresholds;
	
	/**
	 * Create data collection item object from NXCP message
	 * 
	 * @param owner Owning configuration object
	 * @param msg NXCP message
	 */
	protected DataCollectionItem(final DataCollectionConfiguration owner, final NXCPMessage msg)
	{
		this.owner = owner;
		id = msg.getVariableAsInt64(NXCPCodes.VID_DCI_ID);
		templateId = msg.getVariableAsInt64(NXCPCodes.VID_TEMPLATE_ID);
		resourceId = msg.getVariableAsInt64(NXCPCodes.VID_RESOURCE_ID);
		proxyNode = msg.getVariableAsInt64(NXCPCodes.VID_AGENT_PROXY);
		dataType = msg.getVariableAsInteger(NXCPCodes.VID_DCI_DATA_TYPE);
		pollingInterval = msg.getVariableAsInteger(NXCPCodes.VID_POLLING_INTERVAL);
		retentionTime = msg.getVariableAsInteger(NXCPCodes.VID_RETENTION_TIME);
		origin = msg.getVariableAsInteger(NXCPCodes.VID_DCI_SOURCE_TYPE);
		status = msg.getVariableAsInteger(NXCPCodes.VID_DCI_STATUS);
		deltaCalculation = msg.getVariableAsInteger(NXCPCodes.VID_DCI_DELTA_CALCULATION);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		transformationScript = msg.getVariableAsString(NXCPCodes.VID_DCI_FORMULA);
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
		instance = msg.getVariableAsString(NXCPCodes.VID_INSTANCE);
		systemTag = msg.getVariableAsString(NXCPCodes.VID_SYSTEM_TAG);
		baseUnits = msg.getVariableAsInteger(NXCPCodes.VID_BASE_UNITS);
		multiplier = msg.getVariableAsInteger(NXCPCodes.VID_MULTIPLIER);
		customUnitName = msg.getVariableAsString(NXCPCodes.VID_CUSTOM_UNITS_NAME);
		perfTabSettings = msg.getVariableAsString(NXCPCodes.VID_PERFTAB_SETTINGS);
		snmpRawValueType = msg.getVariableAsInteger(NXCPCodes.VID_SNMP_RAW_VALUE_TYPE);
		snmpPort = msg.getVariableAsInteger(NXCPCodes.VID_SNMP_PORT);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_SCHEDULES);
		schedules = new ArrayList<String>(count);
		long varId = NXCPCodes.VID_DCI_SCHEDULE_BASE;
		for(int i = 0; i < count; i++, varId++)
		{
			schedules.add(msg.getVariableAsString(varId));
		}
		
		count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_THRESHOLDS);
		thresholds = new ArrayList<Threshold>(count);
		varId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < count; i++, varId += 10)
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
		this.owner = owner;
		this.id = id;
		templateId = 0;
		resourceId = 0;
		proxyNode = 0;
		dataType = DT_INT;
		pollingInterval = 60;
		retentionTime = 30;
		origin = AGENT;
		status = ACTIVE;
		deltaCalculation = DELTA_NONE;
		flags = 0;
		transformationScript = null;
		perfTabSettings = null;
		name = "";
		description = "";
		instance = "";
		systemTag = "";
		baseUnits = 0;
		multiplier = 0;
		customUnitName = null;
		snmpRawValueType = SNMP_RAWTYPE_NONE;
		snmpPort = 0;
		schedules = new ArrayList<String>(0);
		thresholds = new ArrayList<Threshold>(0);
	}
	
	/**
	 * Fill NXCP message with item's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		msg.setVariableInt32(NXCPCodes.VID_DCI_ID, (int)id);
		msg.setVariableInt16(NXCPCodes.VID_DCI_DATA_TYPE, dataType);
		msg.setVariableInt32(NXCPCodes.VID_POLLING_INTERVAL, pollingInterval);
		msg.setVariableInt32(NXCPCodes.VID_RETENTION_TIME, retentionTime);
		msg.setVariableInt16(NXCPCodes.VID_DCI_SOURCE_TYPE, origin);
		msg.setVariableInt16(NXCPCodes.VID_DCI_DELTA_CALCULATION, deltaCalculation);
		msg.setVariableInt16(NXCPCodes.VID_DCI_STATUS, status);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		msg.setVariable(NXCPCodes.VID_DESCRIPTION, description);
		msg.setVariable(NXCPCodes.VID_INSTANCE, instance);
		msg.setVariable(NXCPCodes.VID_SYSTEM_TAG, systemTag);
		msg.setVariable(NXCPCodes.VID_DCI_FORMULA, transformationScript);
		msg.setVariableInt16(NXCPCodes.VID_FLAGS, flags);
		msg.setVariableInt16(NXCPCodes.VID_SNMP_RAW_VALUE_TYPE, snmpRawValueType);
		msg.setVariableInt32(NXCPCodes.VID_RESOURCE_ID, (int)resourceId);
		msg.setVariableInt32(NXCPCodes.VID_AGENT_PROXY, (int)proxyNode);
		msg.setVariableInt16(NXCPCodes.VID_BASE_UNITS, baseUnits);
		msg.setVariableInt32(NXCPCodes.VID_MULTIPLIER, multiplier);
		if (customUnitName != null)
			msg.setVariable(NXCPCodes.VID_CUSTOM_UNITS_NAME, customUnitName);
		if (perfTabSettings != null)
			msg.setVariable(NXCPCodes.VID_PERFTAB_SETTINGS, perfTabSettings);
		msg.setVariableInt16(NXCPCodes.VID_SNMP_PORT, snmpPort);
		
		msg.setVariableInt32(NXCPCodes.VID_NUM_SCHEDULES, schedules.size());
		long varId = NXCPCodes.VID_DCI_SCHEDULE_BASE;
		for(int i = 0; i < schedules.size(); i++)
		{
			msg.setVariable(varId++, schedules.get(i));
		}

		msg.setVariableInt32(NXCPCodes.VID_NUM_THRESHOLDS, thresholds.size());
		varId = NXCPCodes.VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < thresholds.size(); i++, varId +=10)
		{
			thresholds.get(i).fillMessage(msg, varId);
		}
	}

	/**
	 * @return the templateId
	 */
	public long getTemplateId()
	{
		return templateId;
	}

	/**
	 * @param templateId the templateId to set
	 */
	public void setTemplateId(long templateId)
	{
		this.templateId = templateId;
	}

	/**
	 * @return the resourceId
	 */
	public long getResourceId()
	{
		return resourceId;
	}

	/**
	 * @param resourceId the resourceId to set
	 */
	public void setResourceId(long resourceId)
	{
		this.resourceId = resourceId;
	}

	/**
	 * @return the proxyNode
	 */
	public long getProxyNode()
	{
		return proxyNode;
	}

	/**
	 * @param proxyNode the proxyNode to set
	 */
	public void setProxyNode(long proxyNode)
	{
		this.proxyNode = proxyNode;
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
	 * @return the pollingInterval
	 */
	public int getPollingInterval()
	{
		return pollingInterval;
	}

	/**
	 * @param pollingInterval the pollingInterval to set
	 */
	public void setPollingInterval(int pollingInterval)
	{
		this.pollingInterval = pollingInterval;
	}

	/**
	 * @return the retentionTime
	 */
	public int getRetentionTime()
	{
		return retentionTime;
	}

	/**
	 * @param retentionTime the retentionTime to set
	 */
	public void setRetentionTime(int retentionTime)
	{
		this.retentionTime = retentionTime;
	}

	/**
	 * @return the origin
	 */
	public int getOrigin()
	{
		return origin;
	}

	/**
	 * @param origin the origin to set
	 */
	public void setOrigin(int origin)
	{
		this.origin = origin;
	}

	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @param status the status to set
	 */
	public void setStatus(int status)
	{
		this.status = status;
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
	 * @return the useAdvancedSchedule
	 */
	public boolean isUseAdvancedSchedule()
	{
		return (flags & DCF_ADVANCED_SCHEDULE) != 0;
	}

	/**
	 * @param useAdvancedSchedule the useAdvancedSchedule to set
	 */
	public void setUseAdvancedSchedule(boolean useAdvancedSchedule)
	{
		if (useAdvancedSchedule)
			flags |= DCF_ADVANCED_SCHEDULE;
		else
			flags &= ~DCF_ADVANCED_SCHEDULE;
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
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
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
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the schedules
	 */
	public ArrayList<String> getSchedules()
	{
		return schedules;
	}
	
	/**
	 * Set schedules
	 * 
	 * @param newSchedules Collection containing new schedules
	 */
	public void setSchedules(Collection<String> newSchedules)
	{
		schedules.clear();
		for(String s : newSchedules)
		{
			schedules.add(new String(s));
		}
	}

	/**
	 * @return the thresholds
	 */
	public ArrayList<Threshold> getThresholds()
	{
		return thresholds;
	}

	/**
	 * Get owning data collection configuration.
	 * 
	 * @return the owner
	 */
	public DataCollectionConfiguration getOwner()
	{
		return owner;
	}

	/**
	 * Get ID of owning node.
	 * 
	 * @return
	 */
	public long getNodeId()
	{
		return owner.getNodeId();
	}

	/**
	 * Get system tag. In most situations, system tag should not be shown to user.
	 * 
	 * @return System tag associated with this DCI
	 */
	public String getSystemTag()
	{
		return systemTag;
	}

	/**
	 * Set system tag. In most situations, user should not have possibility to set system tag manually.
	 * 
	 * @param systemTag New system tag for DCI
	 */
	public void setSystemTag(String systemTag)
	{
		this.systemTag = systemTag;
	}

	/**
	 * @return the perfTabSettings
	 */
	public String getPerfTabSettings()
	{
		return perfTabSettings;
	}

	/**
	 * @param perfTabSettings the perfTabSettings to set
	 */
	public void setPerfTabSettings(String perfTabSettings)
	{
		this.perfTabSettings = perfTabSettings;
	}

	/**
	 * @return the snmpPort
	 */
	public int getSnmpPort()
	{
		return snmpPort;
	}

	/**
	 * @param snmpPort the snmpPort to set
	 */
	public void setSnmpPort(int snmpPort)
	{
		this.snmpPort = snmpPort;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
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
}
