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
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.IncidentState;
import org.netxms.client.events.IncidentSummary;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.incidents.widgets.IncidentList;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.ViewerElementUpdater;

/**
 * Label provider for incident list
 */
public class IncidentListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private TableViewer viewer;
   private NXCSession session;
   private Image[] stateImages;

   /**
    * Create label provider
    */
   public IncidentListLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;

      session = Registry.getSession();

      stateImages = new Image[5];
      stateImages[IncidentState.OPEN.getValue()] = ResourceManager.getImage("icons/incident-open.png");
      stateImages[IncidentState.IN_PROGRESS.getValue()] = ResourceManager.getImage("icons/incident-in-progress.png");
      stateImages[IncidentState.PENDING.getValue()] = ResourceManager.getImage("icons/incident-pending.png");
      stateImages[IncidentState.RESOLVED.getValue()] = ResourceManager.getImage("icons/incident-resolved.png");
      stateImages[IncidentState.CLOSED.getValue()] = ResourceManager.getImage("icons/incident-closed.png");
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == 0)
      {
         IncidentState state = ((IncidentSummary)element).getState();
         return stateImages[state.getValue()];
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      IncidentSummary incident = (IncidentSummary)element;
      switch(columnIndex)
      {
         case IncidentList.COLUMN_ID:
            return Long.toString(incident.getId());
         case IncidentList.COLUMN_STATE:
            return incident.getState().name(); // FIXME: localization
         case IncidentList.COLUMN_TITLE:
            return incident.getTitle();
         case IncidentList.COLUMN_SOURCE_OBJECT:
            return session.getObjectName(incident.getSourceObjectId());
         case IncidentList.COLUMN_ASSIGNED_USER:
            AbstractUserObject user = session.findUserDBObjectById(incident.getAssignedUserId(), new ViewerElementUpdater(viewer, element));
            return (user != null) ? user.getName() : "[" + Integer.toString(incident.getAssignedUserId()) + "]";
         case IncidentList.COLUMN_CREATED_TIME:
            return DateFormatFactory.getDateTimeFormat().format(incident.getCreationTime());
         case IncidentList.COLUMN_LAST_CHANGE_TIME:
            return DateFormatFactory.getDateTimeFormat().format(incident.getLastChangeTime());
         case IncidentList.COLUMN_ALARMS:
            return Integer.toString(incident.getAlarmCount());
      }
      return "";
   }
}
