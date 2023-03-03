/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import java.util.Set;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;

/**
 * Content provider for file delivery policy element tree
 */
public class FileDeliveryPolicyContentProvider implements ITreeContentProvider
{
   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getElements(java.lang.Object)
    */
   @SuppressWarnings("unchecked")
   @Override
   public Object[] getElements(Object inputElement)
   {
      return ((Set<PathElement>)inputElement).toArray();
   }

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
    */
   @Override
   public Object[] getChildren(Object parentElement)
   {
      return ((PathElement)parentElement).getChildren();
   }

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
    */
   @Override
   public Object getParent(Object element)
   {
      return ((PathElement)element).getParent();
   }

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
    */
   @Override
   public boolean hasChildren(Object element)
   {
      return ((PathElement)element).hasChildren();
   }

   @Override
   public void dispose()
   {
      // TODO Auto-generated method stub
      
   }

   @Override
   public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
   {
      // TODO Auto-generated method stub
      
   }
}
