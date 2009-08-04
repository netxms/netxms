/**
 * 
 */
package org.netxms.client.datacollection;

import java.util.ArrayList;

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
	
	// DCI data
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
	private boolean processAllThresholds;
	private boolean useAdvancedSchedule;
	private String name;
	private String description;
	private String transformationScript;
	private String instance;
	private int baseUnits;
	private int multiplier;
	private String customUnitName;
	private ArrayList<String> schedules;
	private ArrayList<Threshold> thresholds;
	
	/**
	 * Create data collection item object from NXCP message
	 * 
	 * @param msg NXCP message
	 */
	protected DataCollectionItem(final DataCollectionConfiguration owner, final NXCPMessage msg)
	{
		this.owner = owner;
		id = msg.getVariableAsInt64(NXCPCodes.VID_DCI_ID);
		templateId = msg.getVariableAsInt64(NXCPCodes.VID_TEMPLATE_ID);
		resourceId = msg.getVariableAsInt64(NXCPCodes.VID_RESOURCE_ID);
		proxyNode = msg.getVariableAsInt64(NXCPCodes.VID_PROXY_NODE);
		dataType = msg.getVariableAsInteger(NXCPCodes.VID_DCI_DATA_TYPE);
		pollingInterval = msg.getVariableAsInteger(NXCPCodes.VID_POLLING_INTERVAL);
		retentionTime = msg.getVariableAsInteger(NXCPCodes.VID_RETENTION_TIME);
		origin = msg.getVariableAsInteger(NXCPCodes.VID_DCI_SOURCE_TYPE);
		status = msg.getVariableAsInteger(NXCPCodes.VID_DCI_STATUS);
		deltaCalculation = msg.getVariableAsInteger(NXCPCodes.VID_DCI_DELTA_CALCULATION);
		processAllThresholds = msg.getVariableAsBoolean(NXCPCodes.VID_ALL_THRESHOLDS);
		useAdvancedSchedule = msg.getVariableAsBoolean(NXCPCodes.VID_ADV_SCHEDULE);
		transformationScript = msg.getVariableAsString(NXCPCodes.VID_DCI_FORMULA);
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
		instance = msg.getVariableAsString(NXCPCodes.VID_INSTANCE);
		baseUnits = msg.getVariableAsInteger(NXCPCodes.VID_BASE_UNITS);
		multiplier = msg.getVariableAsInteger(NXCPCodes.VID_MULTIPLIER);
		customUnitName = msg.getVariableAsString(NXCPCodes.VID_CUSTOM_UNITS_NAME);
		
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
		return processAllThresholds;
	}

	/**
	 * @param processAllThresholds the processAllThresholds to set
	 */
	public void setProcessAllThresholds(boolean processAllThresholds)
	{
		this.processAllThresholds = processAllThresholds;
	}

	/**
	 * @return the useAdvancedSchedule
	 */
	public boolean isUseAdvancedSchedule()
	{
		return useAdvancedSchedule;
	}

	/**
	 * @param useAdvancedSchedule the useAdvancedSchedule to set
	 */
	public void setUseAdvancedSchedule(boolean useAdvancedSchedule)
	{
		this.useAdvancedSchedule = useAdvancedSchedule;
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
}
