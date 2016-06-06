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
package org.netxms.ui.eclipse.objects;

import org.eclipse.core.runtime.IAdapterFactory;
import org.netxms.client.objects.AbstractObject;

/**
 * Adapter for object wrappers
 */
public class ObjectWrapperAdapterFactory implements IAdapterFactory
{
   private static final Class<?>[] supportedClasses = 
   {
      AbstractObject.class
   };

   /* (non-Javadoc)
    * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
    */
   @SuppressWarnings("rawtypes")
   @Override
   public Object getAdapter(Object adaptableObject, Class adapterType)
   {
      if (adapterType == AbstractObject.class)
      {
         return ((ObjectWrapper)adaptableObject).getObject();
      }
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.core.runtime.IAdapterFactory#getAdapterList()
    */
   @Override
   public Class<?>[] getAdapterList()
   {
      return supportedClasses;
   }
}
