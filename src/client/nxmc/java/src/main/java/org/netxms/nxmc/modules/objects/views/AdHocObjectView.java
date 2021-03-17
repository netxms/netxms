/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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

/**
 * Ad-hoc views that intended to be shown only for specific object
 */
public abstract class AdHocObjectView extends ObjectView
{
   private long objectId;

   /**
    * Create ad-hoc object view.
    *
    * @param name view name
    * @param image view image
    * @param baseId base view ID (actual ID will be derived from base and object ID)
    * @param objectId object ID this view is intended for
    */
   public AdHocObjectView(String name, ImageDescriptor image, String baseId, long objectId)
   {
      super(name, image, baseId + "#" + Long.toString(objectId));
      this.objectId = objectId;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && (((AbstractObject)context).getObjectId() == objectId);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }
}
