/**
 * 
 */
package org.netxms.client.reports;

import org.netxms.base.NXCPMessage;

/**
 * Parameter for report
 */
public class ReportParameter
{
	public static final int DT_STRING = 0; 
	public static final int DT_TIMESTAMP = 1; 
	public static final int DT_INTEGER = 2; 
	
	private String name;
	private String description;
	private String defaultValue;
	private int dataType;
	
	/**
	 * Create new parameter object.
	 * 
	 * @param name
	 * @param description
	 * @param defaultValue
	 * @param dataType
	 */
	public ReportParameter(String name, String description, String defaultValue, int dataType)
	{
		this.name = name;
		this.description = description;
		this.defaultValue = defaultValue;
		this.dataType = dataType;
	}

	/**
	 * Create report parameter from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public ReportParameter(NXCPMessage msg, long baseId)
	{
		name = msg.getVariableAsString(baseId);
		description = msg.getVariableAsString(baseId + 1);
		defaultValue = msg.getVariableAsString(baseId + 2);
		dataType = msg.getVariableAsInteger(baseId + 3);
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public ReportParameter(ReportParameter src)
	{
		name = src.name;
		description = src.description;
		dataType = src.dataType;
		defaultValue = src.defaultValue;
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
	 * @return the defaultValue
	 */
	public String getDefaultValue()
	{
		return defaultValue;
	}

	/**
	 * @param defaultValue the defaultValue to set
	 */
	public void setDefaultValue(String defaultValue)
	{
		this.defaultValue = defaultValue;
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
}
