/**
 * NetXMS - open source network management system
 * Copyright (C) 2019-2023 Raden Solutions
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
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.UserSession;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.UserSessionsView;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * User session comparator
 */
public class UserSessionComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      int result;

      UserSession s1 = (UserSession)e1;
      UserSession s2 = (UserSession)e2;
      
      Integer id = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");
      switch(id)
      {
         case UserSessionsView.COLUMN_AGENT_PID:
            result = Long.compare(s1.getAgentPID(), s2.getAgentPID());
            break;
         case UserSessionsView.COLUMN_AGENT_TYPE:
            result = Integer.compare(s1.getAgentType(), s2.getAgentType());
            break;
         case UserSessionsView.COLUMN_CLIENT_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(s1.getClientAddress(), s2.getClientAddress());
            break;
         case UserSessionsView.COLUMN_CLIENT_NAME:
            result = s1.getClientName().compareToIgnoreCase(s2.getClientName());
            break;
         case UserSessionsView.COLUMN_DISPLAY:
            result = s1.getDisplayDescription().compareToIgnoreCase(s2.getDisplayDescription());
            break;
         case UserSessionsView.COLUMN_ID:
            result = Integer.compare(s1.getId(), s2.getId());
            break;
         case UserSessionsView.COLUMN_IDLE_TIME:
            result = Integer.compare(s1.getIdleTime(), s2.getIdleTime());
            break;
         case UserSessionsView.COLUMN_LOGIN_TIME:
            result = s1.getLoginTime().compareTo(s2.getLoginTime());
            break;
         case UserSessionsView.COLUMN_TERMINAL:
            result = s1.getTerminal().compareToIgnoreCase(s2.getTerminal());
            break;
         case UserSessionsView.COLUMN_USER:
            result = s1.getLoginName().compareToIgnoreCase(s2.getLoginName());
            break;
         case UserSessionsView.COLUMN_STATE:
            result = Boolean.compare(s1.isConnected(), s2.isConnected());
            break;
         default:
            result = 0;
            break;
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
