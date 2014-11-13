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
package org.netxms.api.client.servermanager;

import java.io.IOException;
import java.util.Map;

import org.netxms.api.client.NetXMSClientException;

/**
 * Interface for server management.
 *
 */
public interface ServerManager
{
	/**
	 * Get server configuration variables.
	 * 
	 * @return Map containing server configuration variables
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NetXMSClientException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract Map<String, ServerVariable> getServerVariables() throws IOException, NetXMSClientException;

	/**
	 * Set server configuration variable.
	 * 
	 * @param name
	 *           variable's name
	 * @param value
	 *           new variable's value
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NetXMSClientException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract void setServerVariable(final String name, final String value) throws IOException, NetXMSClientException;

	/**
	 * Delete server configuration variable.
	 * 
	 * @param name
	 *           variable's name
	 * @throws IOException
	 *            if socket I/O error occurs
	 * @throws NetXMSClientException
	 *            if NetXMS server returns an error or operation was timed out
	 */
	public abstract void deleteServerVariable(final String name) throws IOException, NetXMSClientException;
}
