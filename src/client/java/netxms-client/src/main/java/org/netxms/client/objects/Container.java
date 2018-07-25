/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.util.Set;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Container object
 */
public class Container extends GenericObject
{	
	private int flags;
   private boolean autoBind;
   private boolean autoUnbind;
	private String autoBindFilter;
	
	/**
	 * @param msg
	 */
	public Container(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
		autoBindFilter = msg.getFieldAsString(NXCPCodes.VID_AUTOBIND_FILTER);
      autoBind = msg.getFieldAsBoolean(NXCPCodes.VID_AUTOBIND_FLAG);
      autoUnbind = msg.getFieldAsBoolean(NXCPCodes.VID_AUTOUNBIND_FLAG);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#isAlarmsVisible()
    */
   @Override
   public boolean isAlarmsVisible()
   {
      return true;
   }

	/**
	 * @return true if automatic bind is enabled
	 */
	public boolean isAutoBindEnabled()
	{
		return autoBind;
	}

	/**
	 * @return true if automatic unbind is enabled
	 */
	public boolean isAutoUnbindEnabled()
	{
		return autoUnbind;
	}

	/**
	 * @return Filter script for automatic bind
	 */
	public String getAutoBindFilter()
	{
		return autoBindFilter;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Container";
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}


   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, autoBindFilter);
      return strings;
   }
}
