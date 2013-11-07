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
package org.netxms.api.client.scripts;

import java.io.IOException;
import java.util.List;
import org.netxms.api.client.NetXMSClientException;

public interface ScriptLibraryManager
{
	/**
	 * Get list of all scripts in script library.
	 * 
	 * @return ID/name pairs for scripts in script library
	 * @throws IOException if socket or file I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public abstract List<Script> getScriptLibrary() throws IOException, NetXMSClientException;

	/**
	 * Get script from library
	 * 
	 * @param scriptId script ID
	 * @return script source code
	 * @throws IOException if socket or file I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public abstract Script getScript(long scriptId) throws IOException, NetXMSClientException;

	/**
	 * Modify script. If scriptId is 0, new script will be created in library.
	 * 
	 * @param scriptId script ID
	 * @param name script name
	 * @param source script source code
	 * @return script ID (newly assigned if new script was created)
	 * @throws IOException if socket or file I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public abstract long modifyScript(long scriptId, String name, String source) throws IOException, NetXMSClientException;

	/**
	 * Rename script in script library.
	 * 
	 * @param scriptId script ID
	 * @param name new script name
	 * @throws IOException if socket or file I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public abstract void renameScript(long scriptId, String name) throws IOException, NetXMSClientException;

	/**
	 * Delete script from library
	 * 
	 * @param scriptId script ID
	 * @throws IOException if socket or file I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public abstract void deleteScript(long scriptId) throws IOException, NetXMSClientException;
}
