/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventGroup;
import org.netxms.client.events.EventObject;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Content provider for Event Configuration Manager
 */
public class EventObjectContentProvider implements ITreeContentProvider
{
   private Viewer viewer;
   private NXCSession session = ConsoleSharedData.getSession();

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IContentProvider#dispose()
    */
   @Override
   public void dispose()
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
   {
      this.setViewer(viewer);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getElements(java.lang.Object)
    */
   @Override
   public Object[] getElements(Object inputElement)
   {
      return (Object[])inputElement;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
    */
   @Override
   public Object[] getChildren(Object parentElement)
   {
      if ((parentElement instanceof EventGroup) && (((EventGroup)parentElement).hasChildren()))
      {
         List<EventObject> children = session.findMultipleEventObjects(((EventGroup)parentElement).getEventCodes());
         Object[] childArray = new Object[children.size()];
         for(int i = 0; i < children.size(); i++)
         {
            children.get(i).addParent((EventGroup)parentElement);
            childArray[i] = children.get(i);
         }
         return childArray;
      }
      return new Object[0];
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
    */
   @Override
   public Object getParent(Object element)
   {
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
    */
   @Override
   public boolean hasChildren(Object element)
   {
      if (element instanceof EventGroup)
         return ((EventGroup)element).hasChildren();
      
      return false;
   }

   /**
    * Get current viewer
    * 
    * @return viewer
    */
   public Viewer getViewer()
   {
      return viewer;
   }

   /**
    * Set viewer
    * @param viewer to set
    */
   public void setViewer(Viewer viewer)
   {
      this.viewer = viewer;
   }
   
}