/**
 * 
 */
package org.netxms.base;

/**
 * @author Victor
 *
 */
public class NXCPException extends Exception
{
	private static final long serialVersionUID = -1220864361254471L;

	private int errorCode;
	
	public NXCPException(int errorCode)
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
