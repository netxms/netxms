/**
 * 
 */
package org.netxms.client;

import java.util.Date;

import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class NXCDCIValue
{
	private long id;					// DCI id
	private long nodeId;				// related node object id
	private String name;				// name
	private String description;	// description
	private String value;			// value
	private int source;				// data source (agent, SNMP, etc.)
	private int dataType;
	private int status;				// status (active, disabled, etc.)
	private Date timestamp;
	
	/**
	 * Constructor for creating NXCDCIValue from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param base Base variable ID for value object
	 */
	NXCDCIValue(long nodeId, NXCPMessage msg, long base)
	{
		long var = base;
	
		this.nodeId = nodeId;
		id = msg.getVariableAsInteger(var++);
		name = msg.getVariableAsString(var++);
		description = msg.getVariableAsString(var++);
		source = msg.getVariableAsInteger(var++);
		dataType = msg.getVariableAsInteger(var++);
		value = msg.getVariableAsString(var++);
		timestamp = new Date(msg.getVariableAsInt64(var++) * 1000);
		status = msg.getVariableAsInteger(var);
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the value
	 */
	public String getValue()
	{
		return value;
	}

	/**
	 * @return the source
	 */
	public int getSource()
	{
		return source;
	}

	/**
	 * @return the dataType
	 */
	public int getDataType()
	{
		return dataType;
	}

	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @return the timestamp
	 */
	public Date getTimestamp()
	{
		return timestamp;
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}
}
