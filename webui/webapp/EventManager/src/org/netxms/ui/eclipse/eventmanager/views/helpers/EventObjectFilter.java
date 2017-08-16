/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.eventmanager.views.helpers;

import java.util.List;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.events.EventGroup;
import org.netxms.client.events.EventObject;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Event template filter
 */
public class EventObjectFilter extends ViewerFilter
{
   private String filterText = null;
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if (filterText == null)
         return true;
      else if (checkName(element))
         return true;
      else if (checkDescription(element))
         return true;
      else if (checkMessage(element))
         return true;
      
      return false;
   }
   
   /**
    * Check template message
    * 
    * @param element to check
    * @return true if succesfull
    */
   public boolean checkMessage(Object element)
   {
      if (element instanceof EventTemplate)
         return ((EventTemplate)element).getMessage().toLowerCase().contains(filterText.toLowerCase());
      return false;
   }
   
   /**
    * Check object description
    * 
    * @param element to check
    * @return true if successful
    */
   public boolean checkDescription(Object element)
   {
      boolean result = ((EventObject)element).getDescription().toLowerCase().contains(filterText.toLowerCase());
      if (element instanceof EventGroup)
      {
         List<EventObject> children = ConsoleSharedData.getSession().findMultipleEventObjects(((EventGroup)element).getEventCodes());
         result = containsDescription(children);
      }
      return result;
   }
   
   /**
    * Check children for match
    * 
    * @param children to check
    * @return true if succesfull
    */
   private boolean containsDescription(List<EventObject> children)
   {
      if (children != null)
      {
         for(EventObject f : children)
         {
            if (f.getDescription().toLowerCase().contains(filterText.toLowerCase()))
               return true;
         }
         
         for(EventObject f : children)
         {
            if (f instanceof EventGroup)
            {
               List<EventObject> childs = ConsoleSharedData.getSession().findMultipleEventObjects(((EventGroup)f).getEventCodes());
               if (containsName(childs))
                  return true;
            }
         }
      }
      return false;
   }
   
   /**
    * Check Object name
    * 
    * @param element to check
    * @return true if success
    */
   public boolean checkName(Object element)
   {
      boolean result = ((EventObject)element).getName().toLowerCase().contains(filterText.toLowerCase());
      if (element instanceof EventGroup)
      {
         List<EventObject> children = ConsoleSharedData.getSession().findMultipleEventObjects(((EventGroup)element).getEventCodes());
         result = containsName(children);
      }
      return result;
   }

   /**
    * Check children for match
    * 
    * @param children to check
    * @return true if succesfull
    */
   private boolean containsName(List<EventObject> children)
   {
      if (children != null)
      {
         for(EventObject f : children)
         {
            if (f.getName().toLowerCase().contains(filterText.toLowerCase()))
               return true;
         }
         
         for(EventObject f : children)
         {
            if (f instanceof EventGroup)
            {
               List<EventObject> childs = ConsoleSharedData.getSession().findMultipleEventObjects(((EventGroup)f).getEventCodes());
               if (containsName(childs))
                  return true;
            }
         }
      }
      return false;
   }
   
   /**
    * Set filter text
    * 
    * @param filterText string to filter
    */
   public void setFilterText(String filterText)
   {
      if ((filterText == null) || filterText.trim().isEmpty())
         this.filterText = null;
      else
         this.filterText = filterText.trim().toLowerCase();
   }
}
