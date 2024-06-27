/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 RadenSolutions
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
import org.netxms.client.PollState;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.objects.interfaces.AutoBindObject;
import org.netxms.client.objects.interfaces.PollingTarget;

/**
 * Circuit object
 */
public class Circuit extends DataCollectionTarget implements AutoBindObject, PollingTarget
{	
   private int autoBindFlags;
	private String autoBindFilter;

	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public Circuit(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		autoBindFilter = msg.getFieldAsString(NXCPCodes.VID_AUTOBIND_FILTER);
      autoBindFlags = msg.getFieldAsInt32(NXCPCodes.VID_AUTOBIND_FLAGS);
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

   /**
    * @see org.netxms.client.objects.AbstractObject#isAlarmsVisible()
    */
   @Override
   public boolean isAlarmsVisible()
   {
      return true;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#isAutoBindEnabled()
    */
   @Override
   public boolean isAutoBindEnabled()
   {
      return (autoBindFlags & OBJECT_BIND_FLAG) > 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#isAutoUnbindEnabled()
    */
   @Override
   public boolean isAutoUnbindEnabled()
   {
      return (autoBindFlags & OBJECT_UNBIND_FLAG) > 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#getAutoBindFilter()
    */
   @Override
	public String getAutoBindFilter()
	{
		return autoBindFilter;
	}

	/**
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Circuit";
	}

	/**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getFlags()
    */
   @Override
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
      addString(strings, autoBindFilter);
      return strings;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#getAutoBindFlags()
    */
   @Override
   public int getAutoBindFlags()
   {
      return autoBindFlags;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getIfXTablePolicy()
    */
   @Override
   public int getIfXTablePolicy()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getAgentCacheMode()
    */
   @Override
   public AgentCacheMode getAgentCacheMode()
   {
      return null;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getPollerNodeId()
    */
   @Override
   public long getPollerNodeId()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveAgent()
    */
   @Override
   public boolean canHaveAgent()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveInterfaces()
    */
   @Override
   public boolean canHaveInterfaces()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHavePollerNode()
    */
   @Override
   public boolean canHavePollerNode()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canUseEtherNetIP()
    */
   @Override
   public boolean canUseEtherNetIP()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canUseModbus()
    */
   @Override
   public boolean canUseModbus()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getPollStates()
    */
   @Override
   public PollState[] getPollStates()
   {
      return pollStates;
   }
}
