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
package org.netxms.nxmc.base.views;

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.nxmc.Memento;

/**
 * View with context
 */
public abstract class ViewWithContext extends View
{
   protected boolean clearMessagesOnContextChange = true;

   private Object context = null;

   /**
    * Create view with specific ID that is context aware.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    * @param hasFileter true if view should contain filter
    */
   public ViewWithContext(String name, ImageDescriptor image, String id, boolean hasFilter)
   {
      super(name, image, id, hasFilter);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#cloneView()
    */
   @Override
   public View cloneView()
   {
      ViewWithContext view = (ViewWithContext)super.cloneView();
      view.context = context;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      contextChanged(null, context);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getFullName()
    */
   @Override
   public String getFullName()
   {
      return getName() + " - " + getContextName();
   }

   /**
    * Check if this view is hidden. Default implementation always return false. Subclasses may override to implement custom view
    * hiding logic.
    * 
    * @return true if this view is hidden
    */
   public boolean isHidden()
   {
      return false;
   }

   /**
    * Check if this view is valid for given context. Default implementation accepts any non-null context.
    *
    * @param context context to check
    * @return true if view is valid for provided context
    */
   public boolean isValidForContext(Object context)
   {
      return context != null;
   }

   /**
    * Set context
    * 
    * @param context new context
    */
   protected void setContext(Object context)
   {
      setContext(context, true);
   }

   /**
    * Set context
    * 
    * @param context context new context
    * @param update if update should be called
    */
   protected void setContext(Object context, boolean update)
   {
      Object oldContext = this.context;
      this.context = context;
      if (update)
      {
         if (clearMessagesOnContextChange)
            clearMessages();
         contextChanged(oldContext, context);
      }
   }

   /**
    * Get current context
    *
    * @return current context
    */
   protected Object getContext()
   {
      return context;
   }

   /**
    * Get display name for current context.
    *
    * @return display name for current context
    */
   protected String getContextName()
   {
      return (context != null) ? context.toString() : "(null)";
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.View#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      context = restoreContext(memento);
   }

   /**
    * Called when view's context is changed.
    *
    * @param oldContext old context
    * @param newContext new context
    */
   protected abstract void contextChanged(Object oldContext, Object newContext);

   /**
    * Restore context form momento
    *
    * @param memento memento
    */
   protected abstract Object restoreContext(Memento memento);  
}
