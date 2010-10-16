/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.api.client;

/**
 * Interface for client library notifications
 *
 */
public class SessionNotification implements ISessionNotification
{
	protected int code;
	protected long subCode;
	protected Object object;

	/**
	 * @param code
	 * @param object
	 */
	public SessionNotification(int code, Object object)
	{
		this.code = code;
		this.subCode = 0;
		this.object = object;
	}

	/**
	 * @param code
	 * @param subCode
	 */
	public SessionNotification(int code, long subCode)
	{
		this.code = code;
		this.subCode = subCode;
		this.object = null;
	}

	/**
	 * @param code
	 * @param subCode
	 * @param object
	 */
	public SessionNotification(int code, long subCode, Object object)
	{
		this.code = code;
		this.subCode = subCode;
		this.object = object;
	}

	/**
	 * @param code
	 */
	public SessionNotification(int code)
	{
		this.code = code;
		this.subCode = 0;
		this.object = null;
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.ISessionNotification#getCode()
	 */
	public final int getCode()
	{
		return code;
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.ISessionNotification#getSubCode()
	 */
	public final long getSubCode()
	{
		return subCode;
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.ISessionNotification#getObject()
	 */
	public final Object getObject()
	{
		return object;
	}
}
