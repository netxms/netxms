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
 * Dashboard template
 */
public class DashboardTemplate extends DashboardBase
{
   private String nameTemplate;

	/**
	 * Create from NXCP message.
	 *
	 * @param msg NXCP message
	 * @param session owning client session
	 */
	public DashboardTemplate(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
      nameTemplate = msg.getFieldAsString(NXCPCodes.VID_DASHBOARD_NAME_TEMPLATE);
	}

   /**
    * Get name template
    * 
    * @return name template
    */
   public String getNameTemplate()
   {
      return nameTemplate;
   }

   /**
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
	@Override
	public String getObjectClassName()
	{
		return "Dashboard Template";
	}
}
