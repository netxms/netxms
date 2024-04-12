/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.base.helpers;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.viewers.IPostSelectionProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;

/**
 * Selection provider that can transform parent provider's selection. Subclasses should implement transformSelection method.
 */
public abstract class TransformationSelectionProvider implements IPostSelectionProvider
{
   private ISelectionProvider parent;
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
   private Set<ISelectionChangedListener> postSelectionListeners = new HashSet<ISelectionChangedListener>();

   public TransformationSelectionProvider(ISelectionProvider parent)
   {
      this.parent = parent;
      parent.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            if (!selectionListeners.isEmpty())
            {
               SelectionChangedEvent e = new SelectionChangedEvent(TransformationSelectionProvider.this, transformSelection(event.getSelection()));
               for(ISelectionChangedListener l : selectionListeners)
                  l.selectionChanged(e);
            }
         }
      });
      if (parent instanceof IPostSelectionProvider)
      {
         ((IPostSelectionProvider)parent).addPostSelectionChangedListener(new ISelectionChangedListener() {
            @Override
            public void selectionChanged(SelectionChangedEvent event)
            {
               if (!postSelectionListeners.isEmpty())
               {
                  SelectionChangedEvent e = new SelectionChangedEvent(TransformationSelectionProvider.this, transformSelection(event.getSelection()));
                  for(ISelectionChangedListener l : postSelectionListeners)
                     l.selectionChanged(e);
               }
            }
         });
      }
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
   @Override
   public ISelection getSelection()
   {
      return transformSelection(parent.getSelection());
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void setSelection(ISelection selection)
   {
   }

   /**
    * Transform parent's selection
    * 
    * @param selection
    * @return
    */
   protected abstract ISelection transformSelection(ISelection selection);

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void addSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.add(listener);
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void removeSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.remove(listener);
   }

   /**
    * @see org.eclipse.jface.viewers.IPostSelectionProvider#addPostSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void addPostSelectionChangedListener(ISelectionChangedListener listener)
   {
      postSelectionListeners.add(listener);
   }

   /**
    * @see org.eclipse.jface.viewers.IPostSelectionProvider#removePostSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void removePostSelectionChangedListener(ISelectionChangedListener listener)
   {
      postSelectionListeners.remove(listener);
   }

   /**
    * Get selection as structured selection
    * 
    * @return selection as structured selection
    * @throws ClassCastException
    */
   public IStructuredSelection getStructuredSelection() throws ClassCastException
   {
      ISelection selection = getSelection();
      if (selection instanceof IStructuredSelection)
         return (IStructuredSelection)selection;
      throw new ClassCastException("StructuredViewer should return an instance of IStructuredSelection from its getSelection() method.");
   }
}
