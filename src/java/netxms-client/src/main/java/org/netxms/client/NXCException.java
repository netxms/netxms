/**
 * 
 */
package org.netxms.client;

/**
 * @author Victor
 *
 */
public class NXCException extends Exception
{
	private static final long serialVersionUID = -3247688667511123892L;

	private int errorCode;
	
	public NXCException(int errorCode)
	{
		super();
		this.errorCode = errorCode;
	}

	/**
	 * @return the errorCode
	 */
	public int getErrorCode()
	{
		return errorCode;
	}
}
