/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Rack;

/**
 * Ad-hoc rack view
 */
public class AdHocRackView extends RackView
{
   private long contextObjectId;
   private Rack rack;

   /**
    * Create ad-hoc chassis view.
    * 
    * @param contextObjectId ID of object that is context for this view
    * @param chassis chassis object to be shown
    */
   public AdHocRackView(long contextObjectId, Rack rack)
   {
      super(rack.getGuid().toString());
      this.contextObjectId = contextObjectId;
      this.rack = rack;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.RackView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && (((AbstractObject)context).getObjectId() == contextObjectId);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.RackView#getPriority()
    */
   @Override
   public int getPriority()
   {
      return DEFAULT_PRIORITY;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getName()
    */
   @Override
   public String getName()
   {
      return super.getName() + " - " + rack.getObjectName();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.RackView#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      super.createContent(parent);
      buildRackView(rack);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.RackView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      // Ignore object change - this view always show rack set at construction
   }
}
