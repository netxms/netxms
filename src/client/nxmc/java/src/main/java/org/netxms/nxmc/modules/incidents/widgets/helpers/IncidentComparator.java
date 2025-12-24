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
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Incident;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.incidents.widgets.IncidentList;

/**
 * Comparator for incident list
 */
public class IncidentComparator extends ViewerComparator
{
   private NXCSession session;

   /**
    * Create comparator
    */
   public IncidentComparator()
   {
      session = Registry.getSession();
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      Incident i1 = (Incident)e1;
      Incident i2 = (Incident)e2;

      int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");
      int result;

      switch(column)
      {
         case IncidentList.COLUMN_ID:
            result = Long.compare(i1.getId(), i2.getId());
            break;
         case IncidentList.COLUMN_STATE:
            result = Integer.compare(i1.getState(), i2.getState());
            break;
         case IncidentList.COLUMN_TITLE:
            result = i1.getTitle().compareToIgnoreCase(i2.getTitle());
            break;
         case IncidentList.COLUMN_SOURCE_OBJECT:
            result = getObjectName(i1.getSourceObjectId()).compareToIgnoreCase(getObjectName(i2.getSourceObjectId()));
            break;
         case IncidentList.COLUMN_ASSIGNED_USER:
            result = getUserName(i1.getAssignedUserId()).compareToIgnoreCase(getUserName(i2.getAssignedUserId()));
            break;
         case IncidentList.COLUMN_CREATED_TIME:
            result = i1.getCreationTime().compareTo(i2.getCreationTime());
            break;
         case IncidentList.COLUMN_LAST_CHANGE_TIME:
            result = i1.getLastChangeTime().compareTo(i2.getLastChangeTime());
            break;
         case IncidentList.COLUMN_ALARMS:
            int c1 = (i1.getLinkedAlarmIds() != null) ? i1.getLinkedAlarmIds().length : 0;
            int c2 = (i2.getLinkedAlarmIds() != null) ? i2.getLinkedAlarmIds().length : 0;
            result = Integer.compare(c1, c2);
            break;
         case IncidentList.COLUMN_COMMENTS:
            result = Integer.compare(i1.getCommentsCount(), i2.getCommentsCount());
            break;
         default:
            result = 0;
            break;
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }

   /**
    * Get object name by ID
    */
   private String getObjectName(long objectId)
   {
      if (objectId == 0)
         return "";
      AbstractObject object = session.findObjectById(objectId);
      return (object != null) ? object.getObjectName() : "";
   }

   /**
    * Get user name by ID
    */
   private String getUserName(int userId)
   {
      AbstractUserObject user = session.findUserDBObjectById(userId, null);
      return (user != null) ? user.getName() : "";
   }
}
