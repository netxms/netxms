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
package org.netxms.nxmc.modules.networkmaps.views;

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;

/**
 * Ad-hoc map view
 */
public abstract class AdHocTopologyMapView extends AbstractNetworkMapView
{
   protected long rootObjectId;

   /**
    * Create ad-hoc map view.
    * 
    * @param contextObjectId ID of object that is context for this view
    * @param chassis chassis object to be shown
    */
   public AdHocTopologyMapView(String name, ImageDescriptor image, String id, long rootObjectId)
   {
      super(name, image, id);
      this.rootObjectId = rootObjectId;
      objectMoveLocked = false;
      readOnly = true;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      AdHocTopologyMapView view = (AdHocTopologyMapView)super.cloneView();
      view.rootObjectId = rootObjectId;
      return view;
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.PredefinedMapView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && (((AbstractObject)context).getObjectId() == rootObjectId);
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.PredefinedMapView#getPriority()
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
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (isActive())
         super.refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      refresh();
      super.activate();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("rootObjectId", rootObjectId);
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      rootObjectId = memento.getAsLong("rootObjectId", 0);     
   }
}
