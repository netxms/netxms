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
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.base.annotations.Internal;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.snmp.SnmpVersion;

/**
 * Abstract data collection object
 */
public abstract class DataCollectionObject
{
	// object types
	public static final int DCO_TYPE_GENERIC = 0;
	public static final int DCO_TYPE_ITEM    = 1;
	public static final int DCO_TYPE_TABLE   = 2;

	// common data collection flags
	public static final int DCF_AGGREGATE_ON_CLUSTER     = 0x00080;
   public static final int DCF_TRANSFORM_AGGREGATED     = 0x00100;
   public static final int DCF_CACHE_MODE_MASK          = 0x03000;
   public static final int DCF_AGGREGATE_WITH_ERRORS    = 0x04000;
   public static final int DCF_HIDE_ON_LAST_VALUES_PAGE = 0x08000;

   // Instance discovery methods
   public static final int IDM_NONE = 0;
   public static final int IDM_AGENT_LIST = 1;
   public static final int IDM_AGENT_TABLE = 2;
   public static final int IDM_SNMP_WALK_VALUES = 3;
   public static final int IDM_SNMP_WALK_OIDS = 4;
   public static final int IDM_SCRIPT = 5;
   public static final int IDM_WINPERF = 6;
   public static final int IDM_WEB_SERVICE = 7;
   public static final int IDM_INTERNAL_TABLE = 8;
   public static final int IDM_SMCLP_TARGETS = 9;
   public static final int IDM_SMCLP_PROPERTIES = 10;
   public static final int IDM_PUSH = 11;

   // Polling schedule types
   public static final int POLLING_SCHEDULE_DEFAULT  = 0;
   public static final int POLLING_SCHEDULE_CUSTOM   = 1;
   public static final int POLLING_SCHEDULE_ADVANCED = 2;

   // Retention types
   public static final int RETENTION_DEFAULT = 0;
   public static final int RETENTION_CUSTOM  = 1;
   public static final int RETENTION_NONE    = 2;

   @Internal
	protected DataCollectionConfiguration owner;

	protected long id;
   protected long nodeId;
	protected long templateId;
	protected long templateItemId;
	protected long resourceId;
	protected long sourceNode;
	protected int pollingScheduleType;
	protected String pollingInterval;
   protected int retentionType;
	protected String retentionTime;
   protected DataOrigin origin;
   protected DataCollectionObjectStatus status;
   protected int flags;
   protected int stateFlags;
	protected String transformationScript;
	protected String name;
	protected String description;
	protected String systemTag;
   protected String userTag;
	protected String perfTabSettings;
	protected int snmpPort;
   protected SnmpVersion snmpVersion;
	protected ArrayList<String> schedules;
	protected Object userData;
   protected String comments;
   protected String instanceName;
   protected int instanceDiscoveryMethod;
   protected String instanceDiscoveryData;
   protected String instanceDiscoveryFilter;
   protected List<Integer> accessList;
   protected int instanceRetentionTime;
   protected long thresholdDisableEndTime;
   protected long relatedObject;

	/**
	 * Create data collection object from NXCP message
	 * 
	 * @param owner Owning configuration object
	 * @param msg NXCP message
	 */
	protected DataCollectionObject(final DataCollectionConfiguration owner, final NXCPMessage msg)
	{
		this.owner = owner;
		id = msg.getFieldAsInt64(NXCPCodes.VID_DCI_ID);
		nodeId = (owner != null) ? owner.getOwnerId() : msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID);
		templateId = msg.getFieldAsInt64(NXCPCodes.VID_TEMPLATE_ID);
		templateItemId = msg.getFieldAsInt64(NXCPCodes.VID_TEMPLATE_ITEM_ID);
		resourceId = msg.getFieldAsInt64(NXCPCodes.VID_RESOURCE_ID);
		sourceNode = msg.getFieldAsInt64(NXCPCodes.VID_AGENT_PROXY);
		pollingScheduleType = msg.getFieldAsInt32(NXCPCodes.VID_POLLING_SCHEDULE_TYPE);
		pollingInterval = msg.getFieldAsString(NXCPCodes.VID_POLLING_INTERVAL);
		retentionType = msg.getFieldAsInt32(NXCPCodes.VID_RETENTION_TYPE);
		retentionTime = msg.getFieldAsString(NXCPCodes.VID_RETENTION_TIME);
      origin = DataOrigin.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_DCI_SOURCE_TYPE));
      status = DataCollectionObjectStatus.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_DCI_STATUS));
		flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
      stateFlags = msg.getFieldAsInt32(NXCPCodes.VID_STATE_FLAGS);
		transformationScript = msg.getFieldAsString(NXCPCodes.VID_TRANSFORMATION_SCRIPT);
		name = msg.getFieldAsString(NXCPCodes.VID_NAME);
		description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
		systemTag = msg.getFieldAsString(NXCPCodes.VID_SYSTEM_TAG);
      userTag = msg.getFieldAsString(NXCPCodes.VID_USER_TAG);
		perfTabSettings = msg.getFieldAsString(NXCPCodes.VID_PERFTAB_SETTINGS);
		snmpPort = msg.getFieldAsInt32(NXCPCodes.VID_SNMP_PORT);
      snmpVersion = msg.isFieldPresent(NXCPCodes.VID_SNMP_VERSION)
            ? SnmpVersion.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_SNMP_VERSION))
            : SnmpVersion.DEFAULT;
		comments = msg.getFieldAsString(NXCPCodes.VID_COMMENTS);
		instanceRetentionTime = msg.getFieldAsInt32(NXCPCodes.VID_INSTANCE_RETENTION);
		
		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_SCHEDULES);
		schedules = new ArrayList<String>(count);
		long varId = NXCPCodes.VID_DCI_SCHEDULE_BASE;
		for(int i = 0; i < count; i++, varId++)
		{
			schedules.add(msg.getFieldAsString(varId));
		}
		
      instanceName = msg.getFieldAsString(NXCPCodes.VID_INSTANCE);
      instanceDiscoveryMethod = msg.getFieldAsInt32(NXCPCodes.VID_INSTD_METHOD);
      instanceDiscoveryData = msg.getFieldAsString(NXCPCodes.VID_INSTD_DATA);
      instanceDiscoveryFilter = msg.getFieldAsString(NXCPCodes.VID_INSTD_FILTER);      
      relatedObject = msg.getFieldAsInt64(NXCPCodes.VID_RELATED_OBJECT);
      thresholdDisableEndTime = msg.getFieldAsInt64(NXCPCodes.VID_THRESHOLD_ENABLE_TIME);

      Integer acl[] = msg.getFieldAsInt32ArrayEx(NXCPCodes.VID_ACL);
      if (acl == null)
         accessList = new ArrayList<Integer>(0);
      else
         accessList = new ArrayList<Integer>(Arrays.asList(acl));
	}

	/**
    * Constructor for new data collection objects.
    *
    * @param owner Owning configuration object
    * @param nodeId Owning node ID
    * @param id Identifier assigned to new item
    */
   protected DataCollectionObject(final DataCollectionConfiguration owner, long nodeId, long id)
	{
		this.owner = owner;
		this.id = id;
      this.nodeId = nodeId;
		templateId = 0;
		resourceId = 0;
		sourceNode = 0;
		pollingScheduleType = POLLING_SCHEDULE_DEFAULT;
		pollingInterval = null;
		retentionType = RETENTION_DEFAULT;
		retentionTime = null;
      origin = DataOrigin.AGENT;
      status = DataCollectionObjectStatus.ACTIVE;
		flags = 0;
		transformationScript = null;
		perfTabSettings = null;
		name = "";
		description = "";
		systemTag = "";
      userTag = "";
		snmpPort = 0;
      snmpVersion = SnmpVersion.DEFAULT;
		schedules = new ArrayList<String>(0);
		comments = "";
      instanceName = "";
      accessList = new ArrayList<Integer>(0);
      instanceRetentionTime = -1;
      relatedObject = 0;
      thresholdDisableEndTime = 0;
	}

   /**
    * Constructor for new data collection objects.
    *
    * @param owner Owning configuration object
    * @param id Identifier assigned to new item
    */
   protected DataCollectionObject(final DataCollectionConfiguration owner, long id)
   {
      this(owner, owner.getOwnerId(), id);
   }

   /**
    * Constructor for new data collection objects.
    *
    * @param nodeId Owning node ID
    * @param id Identifier assigned to new item
    */
   protected DataCollectionObject(long nodeId, long id)
   {
      this(null, nodeId, id);
   }

   /**
    * Default constructor (intended for deserialization)
    */
   protected DataCollectionObject()
   {
      this(null, 0, 0);
   }

	/**
    * Object copy constructor
    *
    * @param owner object owner
    * @param src object to copy
    */
	public DataCollectionObject(DataCollectionConfiguration owner, DataCollectionObject src)
   {
	   this.owner = owner;
	   id = src.id;
      nodeId = (owner != null) ? owner.getOwnerId() : src.nodeId;
	   templateId = src.templateId;
	   resourceId = src.resourceId;
	   sourceNode = src.sourceNode;
	   pollingScheduleType = src.pollingScheduleType;
	   pollingInterval = src.pollingInterval;
	   retentionType = src.retentionType;
	   retentionTime = src.retentionTime;
	   origin = src.origin;
	   status = src.status;
	   flags = src.flags;
	   transformationScript = src.transformationScript;
	   name = src.name;
	   description = src.description;
	   systemTag = src.systemTag;
      userTag = src.userTag;
	   perfTabSettings = src.perfTabSettings;
	   snmpPort = src.snmpPort;
      snmpVersion = src.snmpVersion;
	   schedules = new ArrayList<String>(src.schedules);
	   userData = src.userData;
	   comments = src.comments;
	   instanceName = src.instanceName;
	   instanceDiscoveryMethod = src.instanceDiscoveryMethod;
	   instanceDiscoveryData = src.instanceDiscoveryData;
	   instanceDiscoveryFilter = src.instanceDiscoveryFilter;
      accessList = new ArrayList<Integer>(src.accessList);
	   instanceRetentionTime = src.instanceRetentionTime;
	   relatedObject = src.relatedObject;
      thresholdDisableEndTime = src.thresholdDisableEndTime;
   }

   /**
	 * Fill NXCP message with item's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		msg.setFieldInt32(NXCPCodes.VID_DCI_ID, (int)id);
		msg.setFieldInt16(NXCPCodes.VID_POLLING_SCHEDULE_TYPE, pollingScheduleType);
		msg.setField(NXCPCodes.VID_POLLING_INTERVAL, pollingInterval);
      msg.setFieldInt16(NXCPCodes.VID_RETENTION_TYPE, retentionType);
		msg.setField(NXCPCodes.VID_RETENTION_TIME, retentionTime);
      msg.setFieldInt16(NXCPCodes.VID_DCI_SOURCE_TYPE, origin.getValue());
      msg.setFieldInt16(NXCPCodes.VID_DCI_STATUS, status.getValue());
		msg.setField(NXCPCodes.VID_NAME, name);
		msg.setField(NXCPCodes.VID_DESCRIPTION, description);
		msg.setField(NXCPCodes.VID_SYSTEM_TAG, systemTag);
      msg.setField(NXCPCodes.VID_USER_TAG, userTag);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
		msg.setField(NXCPCodes.VID_TRANSFORMATION_SCRIPT, transformationScript);
		msg.setFieldInt32(NXCPCodes.VID_RESOURCE_ID, (int)resourceId);
		msg.setFieldInt32(NXCPCodes.VID_AGENT_PROXY, (int)sourceNode);
		if (perfTabSettings != null)
			msg.setField(NXCPCodes.VID_PERFTAB_SETTINGS, perfTabSettings);
		msg.setFieldInt16(NXCPCodes.VID_SNMP_PORT, snmpPort);
      msg.setFieldInt16(NXCPCodes.VID_SNMP_VERSION, snmpVersion.getValue());
		msg.setField(NXCPCodes.VID_COMMENTS, comments);
		msg.setFieldInt32(NXCPCodes.VID_INSTANCE_RETENTION, instanceRetentionTime);

		msg.setFieldInt32(NXCPCodes.VID_NUM_SCHEDULES, schedules.size());
		long varId = NXCPCodes.VID_DCI_SCHEDULE_BASE;
		for(int i = 0; i < schedules.size(); i++)
		{
			msg.setField(varId++, schedules.get(i));
		}

      msg.setField(NXCPCodes.VID_INSTANCE, instanceName);
      msg.setFieldInt16(NXCPCodes.VID_INSTD_METHOD, instanceDiscoveryMethod);
      if (instanceDiscoveryData != null)
         msg.setField(NXCPCodes.VID_INSTD_DATA, instanceDiscoveryData);
      if (instanceDiscoveryFilter != null)
         msg.setField(NXCPCodes.VID_INSTD_FILTER, instanceDiscoveryFilter);
      msg.setFieldInt32(NXCPCodes.VID_RELATED_OBJECT, (int)relatedObject);
      msg.setFieldInt64(NXCPCodes.VID_THRESHOLD_ENABLE_TIME, thresholdDisableEndTime);

      msg.setField(NXCPCodes.VID_ACL, accessList.toArray(new Long[accessList.size()]));
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
	 * Get source node (node where actual data collection took place) ID
	 * 
	 * @return source node ID (0 if not set)
	 */
	public long getSourceNode()
	{
		return sourceNode;
	}

	/**
	 * Set source node (node where actual data collection took place) ID. Set to 0 to use DCI's owning node.
	 * 
	 * @param sourceNode source node ID
	 */
	public void setSourceNode(long sourceNode)
	{
		this.sourceNode = sourceNode;
	}

	/**
	 * @return the pollingInterval
	 */
	public String getPollingInterval()
	{
		return pollingInterval;
	}

   /**
    * @return polling interval suitable for sorting
    */
   public int getComparablePollingInterval()
   {
      switch(pollingScheduleType)
      {
         case POLLING_SCHEDULE_DEFAULT:
            return 0;
         case POLLING_SCHEDULE_ADVANCED:
            return -1;
         default:
            try
            {
               return Integer.parseInt(pollingInterval);
            }
            catch(NumberFormatException e)
            {
               return 0;
            }
      }
   }

   /**
	 * @param pollingInterval the pollingInterval to set
	 */
	public void setPollingInterval(String pollingInterval)
	{
		this.pollingInterval = pollingInterval;
	}

	/**
    * @return the pollingScheduleType
    */
   public int getPollingScheduleType()
   {
      return pollingScheduleType;
   }

   /**
    * @param pollingScheduleType the pollingScheduleType to set
    */
   public void setPollingScheduleType(int pollingScheduleType)
   {
      this.pollingScheduleType = pollingScheduleType;
   }

	/**
	 * @return the retentionTime
	 */
	public String getRetentionTime()
	{
		return retentionTime;
	}
	
	/**
	 * @return The retention time
	 */
	public int getComparableRetentionTime()
	{
	   switch(retentionType)
	   {
	      case RETENTION_DEFAULT:
	         return 0;
	      case RETENTION_NONE:
	         return -1;
	      default:
	         try
	         {
	            return Integer.parseInt(retentionTime);
	         }
	         catch(NumberFormatException e)
	         {
	            return 0;
	         }
	   }
	}

   /**
    * @param retentionTime the retentionTime to set
    */
   public void setRetentionTime(String retentionTime)
   {
      this.retentionTime = retentionTime;
   }

	/**
    * @return the retentionType
    */
   public int getRetentionType()
   {
      return retentionType;
   }

   /**
    * @param retentionType the retentionType to set
    */
   public void setRetentionType(int retentionType)
   {
      this.retentionType = retentionType;
   }

	/**
	 * @return the origin
	 */
   public DataOrigin getOrigin()
	{
		return origin;
	}

	/**
	 * @param origin the origin to set
	 */
   public void setOrigin(DataOrigin origin)
	{
		this.origin = origin;
	}

	/**
	 * @return the status
	 */
   public DataCollectionObjectStatus getStatus()
	{
		return status;
	}

	/**
	 * @param status the status to set
	 */
   public void setStatus(DataCollectionObjectStatus status)
	{
		this.status = status;
	}

	/**
	 * @return the useAdvancedSchedule
	 */
	public boolean isUseAdvancedSchedule()
	{
		return pollingScheduleType == POLLING_SCHEDULE_ADVANCED;
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
		schedules.addAll(newSchedules);
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
	 * @return id of owning node
	 */
	public long getNodeId()
	{
      return nodeId;
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
    * Get user-assigned tag.
    * 
    * @return User-assigned tag of this DCI
    */
   public String getUserTag()
   {
      return userTag;
   }

   /**
    * Set user-assigned tag.
    * 
    * @param userTag New user-assigned tag for DCI
    */
   public void setUserTag(String userTag)
   {
      this.userTag = userTag;
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
    * @return the snmpVersion
    */
   public SnmpVersion getSnmpVersion()
   {
      return snmpVersion;
   }

   /**
    * @param snmpVersion the snmpVersion to set
    */
   public void setSnmpVersion(SnmpVersion snmpVersion)
   {
      this.snmpVersion = snmpVersion;
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
    * @return the stateFlags
    */
   public int getStateFlags()
	{
		return stateFlags;
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

	public void setAggregateOnCluster(boolean enable)
	{
		if (enable)
			flags |= DCF_AGGREGATE_ON_CLUSTER;
		else
			flags &= ~DCF_AGGREGATE_ON_CLUSTER;
	}

   /**
    * Include node DCI value into aggregated value even in case of 
    * data collection error (system will use last known value in that case)
    * 
    * @return true if enabled
    */
   public boolean isAggregateWithErrors()
   {
      return (flags & DCF_AGGREGATE_WITH_ERRORS) != 0;
   }

   /**
    * Enable or disable inclusion of node DCI value into aggregated value even
    * in case of data collection error (system will use last known value in that case)
    * 
    * @param enable true to enable
    */
   public void setAggregateWithErrors(boolean enable)
   {
      if (enable)
         flags |= DCF_AGGREGATE_WITH_ERRORS;
      else
         flags &= ~DCF_AGGREGATE_WITH_ERRORS;
   }

   public boolean isTransformAggregated()
   {
      return (flags & DCF_TRANSFORM_AGGREGATED) != 0;
   }

   public void setTransformAggregated(boolean enable)
   {
      if (enable)
         flags |= DCF_TRANSFORM_AGGREGATED;
      else
         flags &= ~DCF_TRANSFORM_AGGREGATED;
   }

   /**
    * @return the comments
    */
   public String getComments()
   {
      return comments;
   }

   /**
    * @param comments the comments to set
    */
   public void setComments(String comments)
   {
      this.comments = comments;
   }

   /**
    * @return aggregation function
    */
   public AgentCacheMode getCacheMode()
   {
      return AgentCacheMode.getByValue((int)((flags & DCF_CACHE_MODE_MASK) >> 12));
   }

   public void setCacheMode(AgentCacheMode mode)
   {
      flags = (flags & ~DCF_CACHE_MODE_MASK) | ((mode.getValue() & 0x03) << 12);
   }
   
   public boolean isNewItem()
   {
      return id == 0;
   }
   
   public void setId(long id)
   {
      this.id = id;
   }

   public void setNodeId(long nodeId)
   {
      this.nodeId = nodeId;
   }

   /**
    * @return instance name
    */
   public String getInstanceName()
   {
      return instanceName;
   }

   /**
    * @param instanceName the instance to set
    */
   public void setInstanceName(String instanceName)
   {
      this.instanceName = instanceName;
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
    * Get data collection object access list
    * 
    * @return access list
    */
   public List<Integer> getAccessList()
   {
      return accessList;
   }

   /**
    * Set data collection object access list
    * 
    * @param list new access list
    */
   public void setAccessList(List<Integer> list)
   {
      accessList = list;
   }
   
   /**
    * Get instance retention time
    * 
    * @return instance retention time
    */
   public int getInstanceRetentionTime()
   {
      return instanceRetentionTime;
   }
   
   /**
    * Set instance retention time
    * 
    * @param instanceRetentionTime the retention time to set
    */
   public void setInstanceRetentionTime(int instanceRetentionTime)
   {
      this.instanceRetentionTime = instanceRetentionTime;
   }

   /**
    * Returns if dco is hidden on Last Values view
    * 
    * @return if dco should be hidden on Last Values view
    */
   public boolean isHideOnLastValuesView()
   {
      return (flags & DCF_HIDE_ON_LAST_VALUES_PAGE) != 0;
   }
   
   /**
    * Enable or disable usage of this DCI for node status calculation
    * 
    * @param enable true to enable
    */
   public void setHideOnLastValuesView(boolean enable)
   {
      if (enable)
         flags |= DCF_HIDE_ON_LAST_VALUES_PAGE;
      else
         flags &= ~DCF_HIDE_ON_LAST_VALUES_PAGE;
   }

   /**
    * @return the relatedObject
    */
   public long getRelatedObject()
   {
      return relatedObject;
   }

   /**
    * @param relatedObject the relatedObject to set
    */
   public void setRelatedObject(long relatedObject)
   {
      this.relatedObject = relatedObject;
   }

   /**
    * @return the templateItemId
    */
   public long getTemplateItemId()
   {
      return templateItemId;
   }

   /**
    * Get time until which threshold processing is disabled for this DCI. Value of 0 means that threshold processing is enabled.
    * Value of -1 means that threshold processing is disabled permanently.
    *
    * @return the thresholdDisableEndTime time in seconds since epoch until which threshold processing is disabled
    */
   public long getThresholdDisableEndTime()
   {
      return thresholdDisableEndTime;
   }

   /**
    * Set time until which threshold processing is disabled for this DCI. Value of 0 means that threshold processing is enabled.
    * Value of -1 means that threshold processing is disabled permanently.
    * 
    * @param thresholdDisableEndTime time in seconds since epoch until which threshold processing is disabled
    */
   public void setThresholdDisableEndTime(long thresholdDisableEndTime)
   {
      this.thresholdDisableEndTime = thresholdDisableEndTime;
   }
}
