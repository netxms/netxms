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
package org.netxms.nxmc.modules.events.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.events.SyslogRecord;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.helpers.AbstractTraceViewFilter;

/**
 * Filter for syslog monitor
 */
public class SyslogMonitorFilter extends AbstractTraceViewFilter
{
   private NXCSession session = Registry.getSession();
	
	/**
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
      SyslogRecord record = (SyslogRecord)element;

      if ((rootObjectId != 0) && (rootObjectId != record.getSourceObjectId()))
      {
         AbstractObject object = session.findObjectById(record.getSourceObjectId());
         if ((object == null) || !object.isChildOf(rootObjectId))
            return false;
      }

		if ((filterString == null) || filterString.isEmpty())
			return true;
		
      if (record.getMessage().toLowerCase().contains(filterString))
			return true;
		
      if (record.getHostname().toLowerCase().contains(filterString))
			return true;
		
      if (record.getTag().toLowerCase().contains(filterString))
			return true;
		
      AbstractObject object = session.findObjectById(record.getSourceObjectId());
		if (object != null)
		{
			return object.getObjectName().toLowerCase().contains(filterString);
		}

		return false;
	}
}
