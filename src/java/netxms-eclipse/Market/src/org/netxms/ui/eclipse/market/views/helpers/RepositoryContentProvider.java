/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.market.views.helpers;

import java.util.List;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.ui.IViewPart;
import org.netxms.ui.eclipse.market.objects.MarketObject;
import org.netxms.ui.eclipse.market.objects.RepositoryRuntimeInfo;

/**
 * Content provider
 */
public class RepositoryContentProvider implements ITreeContentProvider
{
   private IViewPart viewPart;
   private Viewer viewer;
   
   /**
    * @param viewPart
    */
   public RepositoryContentProvider(IViewPart viewPart)
   {
      this.viewPart = viewPart;
   }
   
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
      this.viewer = viewer;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getElements(java.lang.Object)
    */
   @Override
   public Object[] getElements(Object inputElement)
   {
      return ((List<?>)inputElement).toArray();
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
    */
   @Override
   public Object[] getChildren(Object parentElement)
   {
      if ((parentElement instanceof RepositoryRuntimeInfo) && !((RepositoryRuntimeInfo)parentElement).isLoaded())
      {
         ((RepositoryRuntimeInfo)parentElement).load(viewPart, new Runnable() {
            @Override
            public void run()
            {
               viewer.refresh();
            }
         });
      }
      return ((MarketObject)parentElement).getChildren();
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
    */
   @Override
   public Object getParent(Object element)
   {
      return ((MarketObject)element).getParent();
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
    */
   @Override
   public boolean hasChildren(Object element)
   {
      return ((MarketObject)element).hasChildren();
   }
}
