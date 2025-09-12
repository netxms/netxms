/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Reden Solutions
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

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.PollState;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.interfaces.AutoBindObject;
import org.netxms.client.objects.interfaces.PollingTarget;

/**
 * Base interface for dashboard objects
 */
public class DashboardBase extends GenericObject implements AutoBindObject, PollingTarget
{
   public final static int SHOW_AS_OBJECT_VIEW   = 0x00010000;
   public final static int SCROLLABLE            = 0x00020000;
   public final static int SHOW_CONTEXT_SELECTOR = 0x00040000;
   
   private int numColumns;
   private List<DashboardElement> elements;
   private int autoBindFlags;
   private String autoBindFilter;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public DashboardBase(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      numColumns = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COLUMNS);
      autoBindFilter = msg.getFieldAsString(NXCPCodes.VID_AUTOBIND_FILTER);
      autoBindFlags = msg.getFieldAsInt32(NXCPCodes.VID_AUTOBIND_FLAGS);

      int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      elements = new ArrayList<DashboardElement>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         elements.add(new DashboardElement(msg, fieldId, i));
         fieldId += 10;
      }
   }

   /**
    * @return the numColumns
    */
   public int getNumColumns()
   {
      return numColumns;
   }


   /**
    * Check if "scrollable" flag is set for this dashboard.
    *
    * @return true if "scrollable" flag is set for this dashboard
    */
   public boolean isScrollable()
   {
      return (flags & SCROLLABLE) != 0;
   }
   /**
    * Check if "show as object view" flag is set for this dashboard.
    *
    * @return true if "show as object view" flag is set for this dashboard
    */
   public boolean isShowContentSelector()
   {
      return (flags & SHOW_CONTEXT_SELECTOR) != 0;
   }
   

   /**
    * @return the elements
    */
   public List<DashboardElement> getElements()
   {
      return elements;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#isAutoBindEnabled()
    */
   @Override
   public boolean isAutoBindEnabled()
   {
      return (autoBindFlags & OBJECT_BIND_FLAG) != 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#isAutoUnbindEnabled()
    */
   @Override
   public boolean isAutoUnbindEnabled()
   {
      return (autoBindFlags & OBJECT_UNBIND_FLAG) != 0;
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
    * @see org.netxms.client.objects.interfaces.AutoBindObject#getAutoBindFlags()
    */
   @Override
   public int getAutoBindFlags()
   {
      return autoBindFlags;
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
