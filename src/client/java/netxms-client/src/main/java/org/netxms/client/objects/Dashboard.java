/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
 * Dashboard object
 */
public class Dashboard extends DashboardBase
{

   private int displayPriority;
   private long forcedContextObjectId;

	/**
	 * Create from NXCP message.
	 *
	 * @param msg NXCP message
	 * @param session owning client session
	 */
	public Dashboard(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
      displayPriority = msg.getFieldAsInt32(NXCPCodes.VID_DISPLAY_PRIORITY);
      forcedContextObjectId = msg.getFieldAsInt32(NXCPCodes.VID_FORCED_CONTEXT_OBJECT);
	}

	/**
    * Get dashboard's display priority if it should be shown as object view.
    *
    * @return dashboard's display priority
    */
   public int getDisplayPriority()
   {
      return displayPriority;
   }

   /**
    * Get forced context object ID.
    *
    * @return forced context object ID or 0 if none
    */
   public long getForcedContextObjectId()
   {
      return forcedContextObjectId;
   }

   /**
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
	@Override
	public String getObjectClassName()
	{
		return "Dashboard";
	}

   /**
    * Check if this dashboard is instance of template (has forced context object).
    * 
    * @return true if this dashboard is instance of template
    */
   public boolean isTemplateInstance()
   {
      return forcedContextObjectId != 0;
   }
}
