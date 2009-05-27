/**
 * 
 */
package org.netxms.client;

import java.util.Map;

/**
 * @author Victor
 * 
 * This class is used to hold data for NXCSession.modifyObject()
 *
 */
public class NXCObjectModificationData
{
	// Modification flags
	public static final long MODIFY_NAME              = 0x00000001L;
	public static final long MODIFY_ACL               = 0x00000002L;
	public static final long MODIFY_CUSTOM_ATTRIBUTES = 0x00000004L;
	public static final long MODIFY_AUTO_APPLY        = 0x00000008L;
	public static final long MODIFY_AUTO_BIND         = 0x00000010L;
	public static final long MODIFY_POLICY_CONFIG     = 0x00000020L;
	public static final long MODIFY_VERSION           = 0x00000040L;
	public static final long MODIFY_DESCRIPTION       = 0x00000080L;
	public static final long MODIFY_AGENT_PORT        = 0x00000100L;
	
	private long flags;		// Flags which indicates what object's data should be modified
	private long objectId;
	private String name;
	private NXCAccessListElement[] acl;
	private boolean inheritAccessRights;
	private Map<String, String> customAttributes;
	private boolean autoApplyEnabled;
	private String autoApplyFilter;
	private boolean autoBindEnabled;
	private String autoBindFilter;
	private String configFileName;
	private String configFileContent;
	private int version;
	private String description;
	private int agentPort;
	
	/**
	 * Constructor for creating modification data for given object
	 */
	public NXCObjectModificationData(long objectId)
	{
		this.objectId = objectId;
		flags = 0;
	}

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setObjectId(long objectId)
	{
		this.objectId = objectId;
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
	public void setName(final String name)
	{
		this.name = name;
		flags |= MODIFY_NAME;
	}

	/**
	 * @return the flags
	 */
	public long getFlags()
	{
		return flags;
	}

	/**
	 * @return the acl
	 */
	public NXCAccessListElement[] getACL()
	{
		return (acl != null) ? acl : new NXCAccessListElement[0];
	}

	/**
	 * @param acl the acl to set
	 */
	public void setACL(NXCAccessListElement[] acl)
	{
		this.acl = acl;
		flags |= MODIFY_ACL;
	}

	/**
	 * @return the inheritAccessRights
	 */
	public boolean isInheritAccessRights()
	{
		return inheritAccessRights;
	}

	/**
	 * @param inheritAccessRights the inheritAccessRights to set
	 */
	public void setInheritAccessRights(boolean inheritAccessRights)
	{
		this.inheritAccessRights = inheritAccessRights;
		flags |= MODIFY_ACL;
	}

	/**
	 * @return the customAttributes
	 */
	public Map<String, String> getCustomAttributes()
	{
		return customAttributes;
	}

	/**
	 * @param customAttributes the customAttributes to set
	 */
	public void setCustomAttributes(Map<String, String> customAttributes)
	{
		this.customAttributes = customAttributes;
		flags |= MODIFY_CUSTOM_ATTRIBUTES;
	}

	/**
	 * @return the autoApplyEnabled
	 */
	public boolean isAutoApplyEnabled()
	{
		return autoApplyEnabled;
	}

	/**
	 * @param autoApplyEnabled the autoApplyEnabled to set
	 */
	public void setAutoApplyEnabled(boolean autoApplyEnabled)
	{
		this.autoApplyEnabled = autoApplyEnabled;
		flags |= MODIFY_AUTO_APPLY;
	}

	/**
	 * @return the autoApplyFilter
	 */
	public String getAutoApplyFilter()
	{
		return autoApplyFilter;
	}

	/**
	 * @param autoApplyFilter the autoApplyFilter to set
	 */
	public void setAutoApplyFilter(String autoApplyFilter)
	{
		this.autoApplyFilter = autoApplyFilter;
		flags |= MODIFY_AUTO_APPLY;
	}

	/**
	 * @return the autoApplyEnabled
	 */
	public boolean isAutoBindEnabled()
	{
		return autoBindEnabled;
	}

	/**
	 * @param autoApplyEnabled the autoApplyEnabled to set
	 */
	public void setAutoBindEnabled(boolean autoBindEnabled)
	{
		this.autoBindEnabled = autoBindEnabled;
		flags |= MODIFY_AUTO_BIND;
	}

	/**
	 * @return the autoApplyFilter
	 */
	public String getAutoBindFilter()
	{
		return autoBindFilter;
	}

	/**
	 * @param autoApplyFilter the autoApplyFilter to set
	 */
	public void setAutoBindFilter(String autoBindFilter)
	{
		this.autoBindFilter = autoBindFilter;
		flags |= MODIFY_AUTO_BIND;
	}

	/**
	 * @return the configFileName
	 */
	public String getConfigFileName()
	{
		return configFileName;
	}

	/**
	 * @param configFileName the configFileName to set
	 */
	public void setConfigFileName(String configFileName)
	{
		this.configFileName = configFileName;
		flags |= MODIFY_POLICY_CONFIG;
	}

	/**
	 * @return the configFileContent
	 */
	public String getConfigFileContent()
	{
		return configFileContent;
	}

	/**
	 * @param configFileContent the configFileContent to set
	 */
	public void setConfigFileContent(String configFileContent)
	{
		this.configFileContent = configFileContent;
		flags |= MODIFY_POLICY_CONFIG;
	}

	/**
	 * @return the version
	 */
	public int getVersion()
	{
		return version;
	}

	/**
	 * @param version the version to set
	 */
	public void setVersion(int version)
	{
		this.version = version;
		flags |= MODIFY_VERSION;
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
		flags |= MODIFY_DESCRIPTION;
	}

	public int getAgentPort()
	{
		return agentPort;
	}

	public void setAgentPort(int agentPort)
	{
		this.agentPort = agentPort;
		flags |= MODIFY_AGENT_PORT;
	}
	
}
