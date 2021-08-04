/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer.widgets.helpers;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmHandle;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Filter for alarm list
 */
public class AlarmListFilter extends ViewerFilter
{
   private static final String[] stateText = { Messages.get().AlarmListLabelProvider_AlarmState_Outstanding, Messages.get().AlarmListLabelProvider_AlarmState_Acknowledged, Messages.get().AlarmListLabelProvider_AlarmState_Resolved, Messages.get().AlarmListLabelProvider_AlarmState_Terminated };

   private Set<Long> rootObjects = new HashSet<Long>();
   private int stateFilter = 0xFF;
   private int severityFilter = 0xFF;
   private NXCSession session = ConsoleSharedData.getSession();
   private String filterString = null;

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      Alarm alarm = ((AlarmHandle)element).alarm;
      if (checkSeverity(alarm))
         return true;
      if (checkState(alarm))
         return true;
      if (checkSource(alarm))
         return true;
      if (checkMessage(alarm))
         return true;
      if (checkCount(alarm))
         return true;
      if (checkComments(alarm))
         return true;
      if (checkHelpdeskId(alarm))
         return true;
      if (checkResolvedBy(alarm))
         return true;
      if (checkCreated(alarm))
         return true;
      if (checkChanged(alarm))
         return true;
      return false;
   }
   
   /**
    * @param alarm Alarm selected
    * @return true if matches filter string
    */
   private boolean checkSeverity(Alarm alarm)
   {
      return StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity()).toLowerCase().contains(filterString);
   }
   
   /**
    * @param alarm Alarm selected
    * @return true if matches filter string
    */
   private boolean checkState(Alarm alarm)
   {
      return stateText[alarm.getState()].toLowerCase().contains(filterString);
   }
   
   /**
    * @param alarm Alarm selected
    * @return true if matches filter string
    */
   private boolean checkSource(Alarm alarm)
   {
      AbstractObject object = session.findObjectById(alarm.getSourceObjectId());
      return object != null ? object.getObjectName().toLowerCase().contains(filterString) : false;
   }
   
   /**
    * @param alarm Alarm selected
    * @return true if matches filter string
    */
   private boolean checkMessage(Alarm alarm)
   {
      return alarm.getMessage().toLowerCase().contains(filterString);
   }
   
   /**
    * @param element Alarm selected
    * @return true if matches filter string
    */
   private boolean checkCount(Object element)
   {
      return Integer.toString(((Alarm)element).getRepeatCount()).contains(filterString);
   }
   
   /**
    * @param alarm Alarm selected
    * @return true if matches filter string
    */
   private boolean checkComments(Alarm alarm)
   {
      return Integer.toString(alarm.getCommentsCount()).contains(filterString);
   }
   
   /**
    * @param alarm Alarm selected
    * @return true if matches filter string
    */
   private boolean checkHelpdeskId(Alarm alarm)
   {            
      switch(alarm.getHelpdeskState())
      {
         case Alarm.HELPDESK_STATE_OPEN:
            return alarm.getHelpdeskReference().toLowerCase().contains(filterString);
         case Alarm.HELPDESK_STATE_CLOSED:
            return (alarm.getHelpdeskReference() + Messages.get().AlarmListLabelProvider_Closed).toLowerCase().contains(filterString);
         default:
            return false;
      }
   }

   /**
    * @param alarm Alarm selected
    * @return true if matches filter string
    */
   private boolean checkResolvedBy(Alarm alarm)
   {
      AbstractUserObject user = session.findUserDBObjectById(alarm.getAcknowledgedByUser(), null);
      return user != null ? user.getName().toLowerCase().contains(filterString) : false;
   }

   /**
    * @param alarm Alarm selected
    * @return true if matches filter string
    */
   private boolean checkCreated(Alarm alarm)
   {
      return RegionalSettings.getDateTimeFormat().format(alarm.getCreationTime()).contains(filterString);
   }
   
   /**
    * @param alarm Alarm selected
    * @return true if matches filter string
    */
   private boolean checkChanged(Alarm alarm)
   {
      return RegionalSettings.getDateTimeFormat().format(alarm.getLastChangeTime()).contains(filterString);
   }

   /**
    * @return true if alarm should be displayed
    */
   public boolean filter(Alarm alarm)
   {
      if ((alarm.getStateBit() & stateFilter) == 0)
         return false;

      if (((1 << alarm.getCurrentSeverity().getValue()) & severityFilter) == 0)
         return false;

      synchronized(this.rootObjects)
      {
         if (rootObjects.isEmpty() || rootObjects.contains(alarm.getSourceObjectId()))
            return true; // No filtering by object ID or root object is a source

         AbstractObject object = session.findObjectById(alarm.getSourceObjectId());
         if (object != null)
         {
            // convert List of Longs to array of longs
            long[] rootObjectsArray = new long[rootObjects.size()];
            int i = 0;
            for(long objectId : rootObjects)
            {
               rootObjectsArray[i++] = objectId;
            }
            return object.isChildOf(rootObjectsArray);
         }
      }
      return false;
   }

   /**
    * @param rootObject the rootObject to set
    */
   public final void setRootObject(long rootObject)
   {
      synchronized(this.rootObjects)
      {
         this.rootObjects.clear();
         this.rootObjects.add(rootObject);
      }
   }

   /**
    * @param selectedObjects
    */
   public void setRootObjects(List<Long> selectedObjects)
   {
      synchronized(this.rootObjects)
      {
         this.rootObjects.clear();
         this.rootObjects.addAll(selectedObjects);
      }
   }

   /**
    * @param stateFilter the stateFilter to set
    */
   public void setStateFilter(int stateFilter)
   {
      this.stateFilter = stateFilter;
   }

   /**
    * @param severityFilter the severityFilter to set
    */
   public void setSeverityFilter(int severityFilter)
   {
      this.severityFilter = severityFilter;
   }
   
   /**
    * @return the filterString
    */
   public String getFilterString()
   {
      return filterString;
   }

   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }
}
