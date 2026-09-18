/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.ConnectionHistoryRecord;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.helpers.TokenizedFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for connection history
 */
public class ConnectionHistoryFilter extends ViewerFilter implements AbstractViewerFilter
{
   private NXCSession session = Registry.getSession();
   private TokenizedFilter filter = new TokenizedFilter(null);

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if (filter.isEmpty())
         return true;

      final ConnectionHistoryRecord r = (ConnectionHistoryRecord)element;
      return filter.matches(r.getMacAddress().toString(), r.getIpAddress(), r.getEventTypeText(),
            (r.getNodeId() != 0) ? session.getObjectName(r.getNodeId()) : null,
            (r.getSwitchId() != 0) ? session.getObjectName(r.getSwitchId()) : null,
            (r.getInterfaceId() != 0) ? session.getObjectName(r.getInterfaceId()) : null);
   }

   /**
    * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
    */
   @Override
   public void setFilterString(String filterString)
   {
      this.filter = new TokenizedFilter(filterString);
   }
}
