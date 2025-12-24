/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.incidents.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.events.IncidentSummary;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for incident list
 */
public class IncidentListFilter extends ViewerFilter implements AbstractViewerFilter
{
   private NXCSession session;
   private String filterString = null;
   private int stateFilter = 0x1F;  // All states by default (bits 0-4)

   /**
    * Create filter
    */
   public IncidentListFilter()
   {
      session = Registry.getSession();
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      IncidentSummary incident = (IncidentSummary)element;

      // Check state filter
      if ((stateFilter & (1 << incident.getState().getValue())) == 0)
         return false;

      // Check text filter
      if ((filterString == null) || filterString.isEmpty())
         return true;

      // Match against title
      if (incident.getTitle().toLowerCase().contains(filterString))
         return true;

      // Match against object name
      AbstractObject object = session.findObjectById(incident.getSourceObjectId());
      if (object != null && object.getObjectName().toLowerCase().contains(filterString))
         return true;

      // Match against assigned user
      if (incident.getAssignedUserId() != 0)
      {
         AbstractUserObject user = session.findUserDBObjectById(incident.getAssignedUserId(), null);
         if (user != null && user.getName().toLowerCase().contains(filterString))
            return true;
      }

      // Match against ID
      if (Long.toString(incident.getId()).contains(filterString))
         return true;

      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
    */
   @Override
   public void setFilterString(String filterString)
   {
      this.filterString = (filterString != null) ? filterString.toLowerCase() : null;
   }

   /**
    * Get filter string
    *
    * @return filter string
    */
   public String getFilterString()
   {
      return filterString;
   }

   /**
    * Set state filter (bit mask)
    *
    * @param stateFilter state filter bit mask
    */
   public void setStateFilter(int stateFilter)
   {
      this.stateFilter = stateFilter;
   }

   /**
    * Get state filter
    *
    * @return state filter bit mask
    */
   public int getStateFilter()
   {
      return stateFilter;
   }
}
