/**
 * 
 */
package org.netxms.client;

import java.util.Date;

/**
 * Single row in DCI data
 * @author Victor
 *
 */
public class NXCDCIDataRow
{
	private Date timestamp;
	private Object value;

	public NXCDCIDataRow(Date timestamp, Object value)
	{
		super();
		this.timestamp = timestamp;
		this.value = value;
	}

	/**
	 * @return the timestamp
	 */
	public Date getTimestamp()
	{
		return timestamp;
	}

	/**
	 * @return the value
	 */
	public Object getValue()
	{
		return value;
	}
	
	/**
	 * @return the value
	 */
	public String getValueAsString()
	{
		return value.toString();
	}

	/**
	 * @return the value
	 */
	public long getValueAsLong()
	{
		if (value instanceof Long)
			return ((Long)value).longValue();

		if (value instanceof Double)
			return ((Double)value).longValue();
		
		return 0;
	}

	/**
	 * @return the value
	 */
	public double getValueAsDouble()
	{
		if (value instanceof Long)
			return ((Long)value).doubleValue();

		if (value instanceof Double)
			return ((Double)value).doubleValue();
		
		return 0;
	}
}
