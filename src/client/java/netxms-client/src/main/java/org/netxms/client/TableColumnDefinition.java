/**
 * 
 */
package org.netxms.client;

import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.DataType;

/**
 * Table column definition
 */
public class TableColumnDefinition
{
	private String name;
	private String displayName;
	private DataType dataType;
	private boolean instanceColumn;
	
	/**
	 * @param name The name to set
	 * @param displayName The display name to set
	 * @param dataType The data type to set
	 * @param instanceColumn Set true if instance column
	 */
	public TableColumnDefinition(String name, String displayName, DataType dataType, boolean instanceColumn)
	{
		this.name = name;
		this.displayName = (displayName != null) ? displayName : name;
		this.dataType = dataType;
		this.instanceColumn = instanceColumn;
	}

	/**
	 * @param msg The NXCPMessage
	 * @param baseId The base ID
	 */
	protected TableColumnDefinition(NXCPMessage msg, long baseId)
	{
		name = msg.getFieldAsString(baseId);
		dataType = DataType.getByValue(msg.getFieldAsInt32(baseId + 1));
		displayName = msg.getFieldAsString(baseId + 2);
		if (displayName == null)
			displayName = name;
		instanceColumn = msg.getFieldAsBoolean(baseId + 3);
	}
	
	/**
	 * @param msg The NXCPMessage
	 * @param baseId The base ID
	 */
	protected void fillMessage(NXCPMessage msg, long baseId)
	{
		msg.setField(baseId, name);
		msg.setFieldInt32(baseId + 1, dataType.getValue());
		msg.setField(baseId + 2, displayName);
		msg.setFieldInt16(baseId + 3, instanceColumn ? 1 : 0);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the displayName
	 */
	public String getDisplayName()
	{
		return displayName;
	}

	/**
	 * @return the dataType
	 */
	public DataType getDataType()
	{
		return dataType;
	}

	/**
	 * @return the instanceColumn
	 */
	public boolean isInstanceColumn()
	{
		return instanceColumn;
	}
}
