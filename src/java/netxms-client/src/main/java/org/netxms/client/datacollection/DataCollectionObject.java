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
import java.util.Collection;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Abstract data collection object
 */
public abstract class DataCollectionObject
{
	// object types
	public static final int DCO_TYPE_GENERIC = 0;
	public static final int DCO_TYPE_ITEM    = 1;
	public static final int DCO_TYPE_TABLE   = 2;
	
	// data sources
	public static final int INTERNAL = 0;
	public static final int AGENT = 1;
	public static final int SNMP = 2;
	public static final int CHECKPOINT_SNMP = 3;
	public static final int PUSH = 4;
	public static final int WINPERF = 5;
	public static final int ILO = 6;
	
	// data collection object status
	public static final int ACTIVE = 0;
	public static final int DISABLED = 1;
	public static final int NOT_SUPPORTED = 2;
	
	// Data types
	public static final int DT_INT = 0;
	public static final int DT_UINT = 1;
	public static final int DT_INT64 = 2;
	public static final int DT_UINT64 = 3;
	public static final int DT_STRING = 4;
	public static final int DT_FLOAT = 5;
	public static final int DT_NULL = 6;
	
	// common data collection flags
	public static final int DCF_ADVANCED_SCHEDULE = 0x0001;
	public static final int DCF_AGGREGATE_ON_CLUSTER = 0x0080;
	
	protected DataCollectionConfiguration owner;
	protected long id;
	protected long templateId;
	protected long resourceId;
	protected long proxyNode;
	protected int pollingInterval;
	protected int retentionTime;
	protected int origin;
	protected int status;
	protected int flags;
	protected String transformationScript;
	protected String name;
	protected String description;
	protected String systemTag;
	protected String perfTabSettings;
	protected int snmpPort;
	protected ArrayList<String> schedules;
	protected Object userData;

	/**
	 * Create data collection object from NXCP message
	 * 
	 * @param owner Owning configuration object
	 * @param msg NXCP message
	 */
	protected DataCollectionObject(final DataCollectionConfiguration owner, final NXCPMessage msg)
	{
		this.owner = owner;
		id = msg.getVariableAsInt64(NXCPCodes.VID_DCI_ID);
		templateId = msg.getVariableAsInt64(NXCPCodes.VID_TEMPLATE_ID);
		resourceId = msg.getVariableAsInt64(NXCPCodes.VID_RESOURCE_ID);
		proxyNode = msg.getVariableAsInt64(NXCPCodes.VID_AGENT_PROXY);
		pollingInterval = msg.getVariableAsInteger(NXCPCodes.VID_POLLING_INTERVAL);
		retentionTime = msg.getVariableAsInteger(NXCPCodes.VID_RETENTION_TIME);
		origin = msg.getVariableAsInteger(NXCPCodes.VID_DCI_SOURCE_TYPE);
		status = msg.getVariableAsInteger(NXCPCodes.VID_DCI_STATUS);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		transformationScript = msg.getVariableAsString(NXCPCodes.VID_TRANSFORMATION_SCRIPT);
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
		systemTag = msg.getVariableAsString(NXCPCodes.VID_SYSTEM_TAG);
		perfTabSettings = msg.getVariableAsString(NXCPCodes.VID_PERFTAB_SETTINGS);
		snmpPort = msg.getVariableAsInteger(NXCPCodes.VID_SNMP_PORT);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_SCHEDULES);
		schedules = new ArrayList<String>(count);
		long varId = NXCPCodes.VID_DCI_SCHEDULE_BASE;
		for(int i = 0; i < count; i++, varId++)
		{
			schedules.add(msg.getVariableAsString(varId));
		}
	}

	/**
	 * Constructor for new data collection objects.
	 * 
	 * @param owner Owning configuration object
	 * @param id Identifier assigned to new item
	 */
	protected DataCollectionObject(final DataCollectionConfiguration owner, long id)
	{
		this.owner = owner;
		this.id = id;
		templateId = 0;
		resourceId = 0;
		proxyNode = 0;
		pollingInterval = owner.getSession().getDefaultDciPollingInterval();
		retentionTime = owner.getSession().getDefaultDciRetentionTime();
		origin = AGENT;
		status = ACTIVE;
		flags = 0;
		transformationScript = null;
		perfTabSettings = null;
		name = "";
		description = "";
		systemTag = "";
		snmpPort = 0;
		schedules = new ArrayList<String>(0);
	}
	
	/**
	 * Fill NXCP message with item's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		msg.setVariableInt32(NXCPCodes.VID_DCI_ID, (int)id);
		msg.setVariableInt32(NXCPCodes.VID_POLLING_INTERVAL, pollingInterval);
		msg.setVariableInt32(NXCPCodes.VID_RETENTION_TIME, retentionTime);
		msg.setVariableInt16(NXCPCodes.VID_DCI_SOURCE_TYPE, origin);
		msg.setVariableInt16(NXCPCodes.VID_DCI_STATUS, status);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		msg.setVariable(NXCPCodes.VID_DESCRIPTION, description);
		msg.setVariable(NXCPCodes.VID_SYSTEM_TAG, systemTag);
		msg.setVariableInt16(NXCPCodes.VID_FLAGS, flags);
		msg.setVariable(NXCPCodes.VID_TRANSFORMATION_SCRIPT, transformationScript);
		msg.setVariableInt32(NXCPCodes.VID_RESOURCE_ID, (int)resourceId);
		msg.setVariableInt32(NXCPCodes.VID_AGENT_PROXY, (int)proxyNode);
		if (perfTabSettings != null)
			msg.setVariable(NXCPCodes.VID_PERFTAB_SETTINGS, perfTabSettings);
		msg.setVariableInt16(NXCPCodes.VID_SNMP_PORT, snmpPort);
		
		msg.setVariableInt32(NXCPCodes.VID_NUM_SCHEDULES, schedules.size());
		long varId = NXCPCodes.VID_DCI_SCHEDULE_BASE;
		for(int i = 0; i < schedules.size(); i++)
		{
			msg.setVariable(varId++, schedules.get(i));
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
	 * @return the userData
	 */
	public Object getUserData()
	{
		return userData;
	}

	/**
	 * @param userData the userData to set
	 */
	public void setUserData(Object userData)
	{
		this.userData = userData;
	}
	
	/**
	 * @return the processAllThresholds
	 */
	public boolean isAggregateOnCluster()
	{
		return (flags & DCF_AGGREGATE_ON_CLUSTER) != 0;
	}

	/**
	 * @param enable
	 */
	public void setAggregateOnCluster(boolean enable)
	{
		if (enable)
			flags |= DCF_AGGREGATE_ON_CLUSTER;
		else
			flags &= ~DCF_AGGREGATE_ON_CLUSTER;
	}
}
