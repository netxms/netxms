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
package org.netxms.nxmc.tools;

import org.eclipse.core.runtime.ListenerList;
import org.eclipse.jface.viewers.IPostSelectionProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;

/**
 * IPostSelectionProvider implementation that delegates to another ISelectionProvider or IPostSelectionProvider. The selection
 * provider used for delegation can be exchanged dynamically. Registered listeners are adjusted accordingly. This utility class may
 * be used in workbench parts with multiple viewers.
 * 
 * @author Marc R. Hoffmann
 */
public class IntermediateSelectionProvider implements IPostSelectionProvider
{
	private final ListenerList<ISelectionChangedListener> selectionListeners = new ListenerList<ISelectionChangedListener>();
	private final ListenerList<ISelectionChangedListener> postSelectionListeners = new ListenerList<ISelectionChangedListener>();
   private ISelectionProvider delegate = null;
   private SelectionTransformation transformation;

   private ISelectionChangedListener selectionListener = new ISelectionChangedListener() {
      public void selectionChanged(SelectionChangedEvent event)
      {
         if (event.getSelectionProvider() == delegate)
         {
            fireSelectionChanged(transformation.transform(event.getSelection()));
         }
      }
   };

   private ISelectionChangedListener postSelectionListener = new ISelectionChangedListener() {
      public void selectionChanged(SelectionChangedEvent event)
      {
         if (event.getSelectionProvider() == delegate)
         {
            firePostSelectionChanged(transformation.transform(event.getSelection()));
         }
      }
   };

   /**
    * Default constructor
    */
   public IntermediateSelectionProvider()
   {
      this.transformation = new DummyTransformation();
   }

   /**
    * Create provider with given delegate
    */
   public IntermediateSelectionProvider(ISelectionProvider delegate)
   {
      this.transformation = new DummyTransformation();
      setSelectionProviderDelegate(delegate);
   }

   /**
    * Create provider with given transformation
    */
   public IntermediateSelectionProvider(SelectionTransformation transformation)
   {
      this.transformation = transformation;
   }

   /**
    * Create provider with given transformation
    */
   public IntermediateSelectionProvider(ISelectionProvider delegate, SelectionTransformation transformation)
   {
      this.transformation = transformation;
      setSelectionProviderDelegate(delegate);
   }

	/**
	 * Sets a new selection provider to delegate to. Selection listeners registered with the previous delegate are removed before.
	 * 
	 * @param newDelegate new selection provider
	 */
	public void setSelectionProviderDelegate(ISelectionProvider newDelegate)
	{
		if (delegate == newDelegate)
		{
			return;
		}
		if (delegate != null)
		{
			delegate.removeSelectionChangedListener(selectionListener);
			if (delegate instanceof IPostSelectionProvider)
			{
				((IPostSelectionProvider)delegate).removePostSelectionChangedListener(postSelectionListener);
			}
		}
		delegate = newDelegate;
		if (newDelegate != null)
		{
			newDelegate.addSelectionChangedListener(selectionListener);
			if (newDelegate instanceof IPostSelectionProvider)
			{
				((IPostSelectionProvider)newDelegate).addPostSelectionChangedListener(postSelectionListener);
			}
         ISelection selection = transformation.transform(newDelegate.getSelection());
         fireSelectionChanged(selection);
         firePostSelectionChanged(selection);
		}
	}

	protected void fireSelectionChanged(ISelection selection)
	{
		fireSelectionChanged(selectionListeners, selection);
	}

	protected void firePostSelectionChanged(ISelection selection)
	{
		fireSelectionChanged(postSelectionListeners, selection);
	}

	private void fireSelectionChanged(ListenerList<ISelectionChangedListener> list, ISelection selection)
	{
		SelectionChangedEvent event = new SelectionChangedEvent(delegate, selection);
		Object[] listeners = list.getListeners();
		for(int i = 0; i < listeners.length; i++)
		{
			ISelectionChangedListener listener = (ISelectionChangedListener)listeners[i];
			listener.selectionChanged(event);
		}
	}

	// IPostSelectionProvider Implementation

	public void addSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.add(listener);
	}

	public void removeSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.remove(listener);
	}

	public void addPostSelectionChangedListener(ISelectionChangedListener listener)
	{
		postSelectionListeners.add(listener);
	}

	public void removePostSelectionChangedListener(ISelectionChangedListener listener)
	{
		postSelectionListeners.remove(listener);
	}

	public ISelection getSelection()
	{
      return delegate == null ? null : transformation.transform(delegate.getSelection());
	}

	public void setSelection(ISelection selection)
	{
		if (delegate != null)
		{
			delegate.setSelection(selection);
		}
	}

   private class DummyTransformation implements SelectionTransformation
   {
      @Override
      public ISelection transform(ISelection input)
      {
         return input;
      }
   }
}
