/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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

/**
 * View with context
 */
public abstract class ViewWithContext extends View
{
   private Object context = null;

   /**
    * Create view with random ID that is context aware.
    *
    * @param name view name
    * @param image view image
    */
   public ViewWithContext(String name, ImageDescriptor image)
   {
      super(name, image);
   }

   /**
    * Create view with specific ID that is context aware.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    */
   public ViewWithContext(String name, ImageDescriptor image, String id)
   {
      super(name, image, id);
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
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      if (context != null) // Cloned view will have context set at creation
         contextChanged(null, context);
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
      Object oldContext = this.context;
      this.context = context;
      contextChanged(oldContext, context);
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
    * Called when view's context is changed.
    *
    * @param oldContext old context
    * @param newContext new context
    */
   protected abstract void contextChanged(Object oldContext, Object newContext);
}
