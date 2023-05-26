/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Event;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.helpers.AbstractTraceViewFilter;

/**
 * Filter for event monitor
 */
public class EventMonitorFilter extends AbstractTraceViewFilter
{
   private NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
      Event event = (Event)element;

      if ((rootObjectId != 0) && (rootObjectId != event.getSourceId()))
      {
         AbstractObject object = session.findObjectById(event.getSourceId());
         if ((object == null) || !object.isChildOf(rootObjectId))
            return false;
      }

      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      return 
         event.getMessage().toLowerCase().contains(filterString) ||
         session.getEventName(event.getCode()).toLowerCase().contains(filterString) || 
         session.getObjectName(event.getSourceId()).toLowerCase().contains(filterString); 
	}
}
