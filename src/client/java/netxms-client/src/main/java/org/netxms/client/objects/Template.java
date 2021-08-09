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
import org.netxms.client.objects.interfaces.AutoBindObject;

/**
 * This class represents NetXMS TEMPLATE objects.
 */
public class Template extends GenericObject
{	
	private int version;
	private int autoApplyFlags;
	private String autoApplyFilter;

	/**
	 * Create from NXCP message.
	 *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public Template(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		version = msg.getFieldAsInt32(NXCPCodes.VID_VERSION);
		autoApplyFilter = msg.getFieldAsString(NXCPCodes.VID_AUTOBIND_FILTER);
		autoApplyFlags = msg.getFieldAsInt32(NXCPCodes.VID_AUTOBIND_FLAGS);
	}

	/**
	 * @return template version
	 */
	public int getVersion()
	{
		return version;
	}

	/**
	 * @return template version as string in form
	 */
	public String getVersionAsString()
	{
		return Integer.toString(version);
	}

   /**
    * @return true if automatic bind is enabled
    */
   public boolean isAutoApplyEnabled()
   {
      return (autoApplyFlags & AutoBindObject.OBJECT_BIND_FLAG) > 0;
   }

   /**
    * @return true if automatic unbind is enabled
    */
   public boolean isAutoRemoveEnabled()
   {
      return (autoApplyFlags & AutoBindObject.OBJECT_UNBIND_FLAG) > 0;
   }
   
   public int getAutoApplyFlags()
   {
      return autoApplyFlags;
   }

	/**
	 * @return Filter script for automatic apply
	 */
	public String getAutoApplyFilter()
	{
		return autoApplyFilter;
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Template";
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, autoApplyFilter);
      return strings;
   }
}
