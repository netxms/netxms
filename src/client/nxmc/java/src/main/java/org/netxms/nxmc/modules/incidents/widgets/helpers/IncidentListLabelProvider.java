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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Incident;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.incidents.widgets.IncidentList;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Label provider for incident list
 */
public class IncidentListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private NXCSession session;
   private Image[] stateImages;

   /**
    * Create label provider
    */
   public IncidentListLabelProvider()
   {
      session = Registry.getSession();

      stateImages = new Image[5];
      stateImages[Incident.STATE_OPEN] = ResourceManager.getImage("icons/incident-open.png");
      stateImages[Incident.STATE_IN_PROGRESS] = ResourceManager.getImage("icons/incident-in-progress.png");
      stateImages[Incident.STATE_PENDING] = ResourceManager.getImage("icons/incident-pending.png");
      stateImages[Incident.STATE_RESOLVED] = ResourceManager.getImage("icons/incident-resolved.png");
      stateImages[Incident.STATE_CLOSED] = ResourceManager.getImage("icons/incident-closed.png");
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == IncidentList.COLUMN_STATE)
      {
         int state = ((Incident)element).getState();
         if (state >= 0 && state < stateImages.length)
            return stateImages[state];
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      Incident incident = (Incident)element;
      switch(columnIndex)
      {
         case IncidentList.COLUMN_ID:
            return Long.toString(incident.getId());
         case IncidentList.COLUMN_STATE:
            return incident.getStateName();
         case IncidentList.COLUMN_TITLE:
            return incident.getTitle();
         case IncidentList.COLUMN_SOURCE_OBJECT:
            return getObjectName(incident.getSourceObjectId());
         case IncidentList.COLUMN_ASSIGNED_USER:
            return getUserName(incident.getAssignedUserId());
         case IncidentList.COLUMN_CREATED_TIME:
            return DateFormatFactory.getDateTimeFormat().format(incident.getCreationTime());
         case IncidentList.COLUMN_LAST_CHANGE_TIME:
            return DateFormatFactory.getDateTimeFormat().format(incident.getLastChangeTime());
         case IncidentList.COLUMN_ALARMS:
            long[] alarmIds = incident.getLinkedAlarmIds();
            return (alarmIds != null) ? Integer.toString(alarmIds.length) : "0";
         case IncidentList.COLUMN_COMMENTS:
            return Integer.toString(incident.getCommentsCount());
      }
      return "";
   }

   /**
    * Get object name by ID
    */
   private String getObjectName(long objectId)
   {
      if (objectId == 0)
         return "";
      AbstractObject object = session.findObjectById(objectId);
      return (object != null) ? object.getObjectName() : "[" + Long.toString(objectId) + "]";
   }

   /**
    * Get user name by ID
    */
   private String getUserName(int userId)
   {
      AbstractUserObject user = session.findUserDBObjectById(userId, null);
      return (user != null) ? user.getName() : "[" + Long.toString(userId) + "]";
   }
}
