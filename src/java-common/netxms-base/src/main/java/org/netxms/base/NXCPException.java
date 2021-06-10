/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.base;

/**
 * NXCP exception. Used to indicate protocol level errors.
 */
public class NXCPException extends Exception
{
	public static final int MESSAGE_TOO_LARGE = 1; 
	public static final int SESSION_CLOSED = 2; 
	public static final int NO_CIPHER = 3; 
	public static final int DECRYPTION_ERROR = 4; 
   public static final int FATAL_PROTOCOL_ERROR = 5;
	
	private static final long serialVersionUID = -1220864361254471L;
   private static final String[] errorText = { "", "Message is too large", "Underlying communication session closed", "No cipher", "Decryption error", "Fatal protocol error" };

	private int errorCode;
	
	/**
	 * Create new exception with given error code.
	 *
	 * @param errorCode NetXMS API error code (RCC)
	 */
	public NXCPException(int errorCode)
	{
		super();
		this.errorCode = errorCode;
	}

	/**
	 * Create new exception with given error code and root cause.
	 *
	 * @param errorCode NetXMS API error code (RCC)
	 * @param cause root cause
	 */
	public NXCPException(int errorCode, Throwable cause)
	{
		super(cause);
		this.errorCode = errorCode;
	}

	/**
	 * Get NetXMS API error code (RCC).
	 *
	 * @return NetXMS API error code
	 */
	public int getErrorCode()
	{
		return errorCode;
	}

	/**
	 * @see java.lang.Throwable#getMessage()
	 */
	@Override
	public String getMessage()
	{
		try
		{
			return "NXCP error: " + errorText[errorCode];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return "NXCP error " + Integer.toString(errorCode);
		}
	}
}
