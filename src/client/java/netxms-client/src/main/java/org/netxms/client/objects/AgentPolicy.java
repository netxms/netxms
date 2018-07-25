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
package org.netxms.client.objects;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Generic agent policy object
 *
 */
public class AgentPolicy extends GenericObject
{   
	private int version;
	private int policyType;
	private int flags;
   private boolean autoBind;
   private boolean autoUnbind;
	private String autoDeployFilter;
	
	/**
	 * @param msg
	 * @param session
	 */
	public AgentPolicy(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		policyType = msg.getFieldAsInt32(NXCPCodes.VID_POLICY_TYPE);
		version = msg.getFieldAsInt32(NXCPCodes.VID_VERSION);
		flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
		autoDeployFilter = msg.getFieldAsString(NXCPCodes.VID_AUTOBIND_FILTER);
      autoBind = msg.getFieldAsBoolean(NXCPCodes.VID_AUTOBIND_FLAG);
      autoUnbind = msg.getFieldAsBoolean(NXCPCodes.VID_AUTOUNBIND_FLAG);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "AgentPolicy";
	}

	/**
	 * @return the version
	 */
	public int getVersion()
	{
		return version;
	}

	/**
	 * @return the policyType
	 */
	public int getPolicyType()
	{
		return policyType;
	}

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * @return true if automatic deployment is enabled
    */
   public boolean isAutoDeployEnabled()
   {
      return autoBind;
   }

   /**
    * @return true if automatic uninstall is enabled
    */
   public boolean isAutoUninstallEnabled()
   {
      return autoUnbind;
   }

   /**
    * @return the deployFilter
    */
   public String getAutoDeployFilter()
   {
      return autoDeployFilter;
   }
}
