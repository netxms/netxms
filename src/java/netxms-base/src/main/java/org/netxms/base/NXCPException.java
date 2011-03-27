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
	private static final long serialVersionUID = -1220864361254471L;
	private static final String[] errorText = { "", "Message is too large", "Underlying communictaion session closed" };

	private int errorCode;
	
	/**
	 * @param errorCode
	 */
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

	/* (non-Javadoc)
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
