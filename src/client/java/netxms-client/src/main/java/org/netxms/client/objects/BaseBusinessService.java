/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.client.objects.interfaces.AutoBindDCIObject;
import org.netxms.client.objects.interfaces.AutoBindObject;
import org.netxms.client.objects.interfaces.PollingTarget;

/**
 * Base class for business service objects
 */
public class BaseBusinessService extends GenericObject implements AutoBindObject, AutoBindDCIObject, PollingTarget
{
   private int autoBindFlags;
   private String objectAutoBindFilter;
   private String dciAutoBindFilter;
   private int objectStatusThreshold;
   private int dciStatusThreshold;

   /**
    * Constructor
    * 
    * @param msg NXCPMessage with data
    * @param session NXCPSession
    */
   public BaseBusinessService(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      objectAutoBindFilter = msg.getFieldAsString(NXCPCodes.VID_AUTOBIND_FILTER);
      autoBindFlags = msg.getFieldAsInt32(NXCPCodes.VID_AUTOBIND_FLAGS);
      objectStatusThreshold = msg.getFieldAsInt32(NXCPCodes.VID_OBJECT_STATUS_THRESHOLD);
      dciAutoBindFilter = msg.getFieldAsString(NXCPCodes.VID_AUTOBIND_FILTER_2);
      dciStatusThreshold = msg.getFieldAsInt32(NXCPCodes.VID_DCI_STATUS_THRESHOLD);
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
      return objectAutoBindFilter;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindDCIObject#isDciAutoBindEnabled()
    */
   @Override
   public boolean isDciAutoBindEnabled()
   {
      return (autoBindFlags & DCI_BIND_FLAG) > 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindDCIObject#isDciAutoUnbindEnabled()
    */
   @Override
   public boolean isDciAutoUnbindEnabled()
   {
      return (autoBindFlags & DCI_UNBIND_FLAG) > 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindDCIObject#getDciAutoBindFilter()
    */
   @Override
   public String getDciAutoBindFilter()
   {
      return dciAutoBindFilter;
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
    * @see org.netxms.client.objects.interfaces.PollingTarget#getFlags()
    */
   @Override
   public int getFlags()
   {
      return flags;
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
    * @return the dciStatusThreshold
    */
   public int getDciStatusThreshold()
   {
      return dciStatusThreshold;
   }

   /**
    * @return the nodeStatusThreshold
    */
   public int getObjectStatusThreshold()
   {
      return objectStatusThreshold;
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, objectAutoBindFilter);
      addString(strings, dciAutoBindFilter);
      return strings;
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
