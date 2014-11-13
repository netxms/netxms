/**
 * 
 */
package org.netxms.client.snmp;

/**
 * SNMP Object Id format exception
 *
 */
public class SnmpObjectIdFormatException extends Exception
{
	private static final long serialVersionUID = 5474043433024422833L;
	
	private String message;

	public SnmpObjectIdFormatException(String message)
	{
		this.message = message;
	}

	/* (non-Javadoc)
	 * @see java.lang.Throwable#getMessage()
	 */
	@Override
	public String getMessage()
	{
		return message;
	}
}
