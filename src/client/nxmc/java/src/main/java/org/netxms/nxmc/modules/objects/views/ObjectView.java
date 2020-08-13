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
package org.netxms.nxmc.modules.objects.views;

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.ViewWithContext;

/**
 * Base class for all context views where context should be NetXMS object.
 */
public abstract class ObjectView extends ViewWithContext
{
   /**
    * Create object view with random ID.
    *
    * @param name view name
    * @param image view image
    */
   public ObjectView(String name, ImageDescriptor image)
   {
      super(name, image);
   }

   /**
    * Create object view with specific ID.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    */
   public ObjectView(String name, ImageDescriptor image, String id)
   {
      super(name, image, id);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getContextName()
    */
   @Override
   protected String getContextName()
   {
      return (getContext() != null) ? ((AbstractObject)getContext()).getObjectName() : "[none]";
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      onObjectChange((AbstractObject)newContext);
   }

   /**
    * Called when context is changed to new object. Default implementation does nothing.
    *
    * @param object new context
    */
   protected void onObjectChange(AbstractObject object)
   {
   }

   /**
    * Get current context as object.
    *
    * @return current context as object
    */
   public AbstractObject getObject()
   {
      return (AbstractObject)getContext();
   }
}
