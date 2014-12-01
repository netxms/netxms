/**
 * 
 */
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * Table column definition
 */
public class TableColumnDefinition
{
	private String name;
	private String displayName;
	private int dataType;
	private boolean instanceColumn;
	
	/**
	 * @param name
	 * @param displayName
	 * @param dataType
	 * @param instanceColumn
	 */
	public TableColumnDefinition(String name, String displayName, int dataType, boolean instanceColumn)
	{
		this.name = name;
		this.displayName = (displayName != null) ? displayName : name;
		this.dataType = dataType;
		this.instanceColumn = instanceColumn;
	}

	/**
	 * @param msg
	 * @param baseId
	 */
	protected TableColumnDefinition(NXCPMessage msg, long baseId)
	{
		name = msg.getFieldAsString(baseId);
		dataType = msg.getFieldAsInt32(baseId + 1);
		displayName = msg.getFieldAsString(baseId + 2);
		if (displayName == null)
			displayName = name;
		instanceColumn = msg.getFieldAsBoolean(baseId + 3);
	}
	
	/**
	 * @param msg
	 * @param baseId
	 */
	protected void fillMessage(NXCPMessage msg, long baseId)
	{
		msg.setField(baseId, name);
		msg.setFieldInt32(baseId + 1, dataType);
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
	public int getDataType()
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
