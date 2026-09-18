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
import org.netxms.client.constants.IncidentState;
import org.netxms.client.events.IncidentSummary;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.helpers.TokenizedFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for incident list
 */
public class IncidentListFilter extends ViewerFilter implements AbstractViewerFilter
{
   /** All states (bits 0-4) */
   public static final int STATE_FILTER_ALL = 0x1F;
   /** All states except closed */
   public static final int STATE_FILTER_ACTIVE = STATE_FILTER_ALL & ~(1 << IncidentState.CLOSED.getValue());
   /** Closed state only */
   public static final int STATE_FILTER_CLOSED = 1 << IncidentState.CLOSED.getValue();

   private NXCSession session;
   private TokenizedFilter filter = new TokenizedFilter(null);
   private int stateFilter = STATE_FILTER_ALL;

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
      if (filter.isEmpty())
         return true;

      // Match against title, source object name, assigned user name, and ID
      AbstractObject object = session.findObjectById(incident.getSourceObjectId());
      AbstractUserObject user = (incident.getAssignedUserId() != 0) ? session.findUserDBObjectById(incident.getAssignedUserId(), null) : null;
      return filter.matches(incident.getTitle(), (object != null) ? object.getObjectName() : null, (user != null) ? user.getName() : null,
            Long.toString(incident.getId()));
   }

   /**
    * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
    */
   @Override
   public void setFilterString(String filterString)
   {
      this.filter = new TokenizedFilter(filterString);
   }

   /**
    * Get filter string
    *
    * @return filter string
    */
   public String getFilterString()
   {
      return filter.getFilterString();
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
