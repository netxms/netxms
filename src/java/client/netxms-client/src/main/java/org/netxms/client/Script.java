/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 NetXMS Team
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
package org.netxms.client;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * NXSL script object
 */
public class Script
{
	private long id;
	private String name;
	private String source;
	
	/**
	 * Create script object with given ID and source code.
	 * 
	 * @param id The script ID
	 * @param name The script name
	 * @param source The script source
	 */
	public Script(long id, String name, String source)
	{
		this.id = id;
		this.name = (name != null) ? name : ("[" + Long.toString(id) + "]");
		this.source = source;
   }
	
   /**
   * Create script object from msg
   * 
   * @param msg The NXCPMessage
   */
	public Script(NXCPMessage msg)
	{
      this.id = msg.getFieldAsInt64(NXCPCodes.VID_SCRIPT_ID);
      this.name = msg.getFieldAsString(NXCPCodes.VID_NAME).equals("") ? ("[" + Long.toString(id) + "]") : msg.getFieldAsString(NXCPCodes.VID_NAME);
      this.source = msg.getFieldAsString(NXCPCodes.VID_SCRIPT_CODE);
   }
	
   /**
   * Create script object for list from msg
   * 
   * @param msg The NXCPMessage
   * @param base The base ID
   */
   public Script(NXCPMessage msg, long base)
   {
      this.id = msg.getFieldAsInt64(base++);
      this.name = msg.getFieldAsString(base).equals("") ? ("[" + Long.toString(id) + "]") : msg.getFieldAsString(base);
      base++;
      this.source = null;
   }

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the source
	 */
	public String getSource()
	{
		return source;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
}
