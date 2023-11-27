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
package org.netxms.nxmc.modules.alarms.widgets.helpers;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmHandle;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.interfaces.ZoneMember;
import org.netxms.client.search.SearchAttributeProvider;
import org.netxms.client.search.SearchQuery;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.localization.LocalizationHelper;

/**
 * Filter for alarm list
 */
public class AlarmListFilter extends ViewerFilter implements AbstractViewerFilter
{
   private static final String[] stateText = 
      { 
         LocalizationHelper.getI18n(AlarmListFilter.class).tr("Outstanding"),
         LocalizationHelper.getI18n(AlarmListFilter.class).tr("Acknowledged"),
         LocalizationHelper.getI18n(AlarmListFilter.class).tr("Resolved"),
         LocalizationHelper.getI18n(AlarmListFilter.class).tr("Terminated") 
      };

   private Set<Long> rootObjects = new HashSet<Long>();
   private int stateFilter = 0xFF;
   private int severityFilter = 0xFF;
   private NXCSession session = Registry.getSession();
   private String filterString = null;
   private SearchQuery query = null;

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if (query == null)
         return true;

      final Alarm alarm = ((AlarmHandle)element).alarm;
      return query.match(new SearchAttributeProvider() {
         @Override
         public String getText()
         {
            return alarm.getMessage();
         }

         @Override
         public String getAttribute(String name)
         {
            if (name.equals("acknowledgedby"))
            {
               AbstractUserObject user = session.findUserDBObjectById(alarm.getAcknowledgedByUser(), null);
               return (user != null) ? user.getName() : null;
            }
            if (name.equals("event"))
               return session.getEventName(alarm.getSourceEventCode());
            if (name.equals("hascomments"))
               return alarm.getCommentsCount() > 0 ? "yes" : "no";
            if (name.equals("repeatcount"))
               return Integer.toString(alarm.getRepeatCount());
            if (name.equals("resolvedby"))
            {
               AbstractUserObject user = session.findUserDBObjectById(alarm.getResolvedByUser(), null);
               return (user != null) ? user.getName() : null;
            }
            if (name.equals("severity"))
               return alarm.getCurrentSeverity().toString();
            if (name.equals("source"))
               return session.getObjectName(alarm.getSourceObjectId());
            if (name.equals("state"))
               return stateText[alarm.getState()];
            if (name.equals("zone") && session.isZoningEnabled())
            {
               ZoneMember zm = session.findObjectById(alarm.getSourceObjectId(), ZoneMember.class);
               return (zm != null) ? zm.getZoneName() : null;
            }
            return null;
         }
      });
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

      synchronized(rootObjects)
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
    * Set root object for filter (0 to disable filtering by root object).
    *
    * @param rootObject new root object ID or 0
    */
   public final void setRootObject(long rootObject)
   {
      synchronized(rootObjects)
      {
         rootObjects.clear();
         if (rootObject != 0)
            rootObjects.add(rootObject);
      }
   }

   /**
    * Set root objects for filter.
    *
    * @param rootObjects new root object set
    */
   public void setRootObjects(List<Long> rootObjects)
   {
      synchronized(this.rootObjects)
      {
         this.rootObjects.clear();
         this.rootObjects.addAll(rootObjects);
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
      if (filterString == null)
      {
         this.filterString = null;
         query = null;
         return;
      }

      String s = filterString.toLowerCase().trim();
      if (!s.equals(this.filterString))
      {
         this.filterString = s;
         if (!s.isEmpty())
         {
            query = new SearchQuery(s);
         }
         else
         {
            query = null;
         }
      }
   }
}
