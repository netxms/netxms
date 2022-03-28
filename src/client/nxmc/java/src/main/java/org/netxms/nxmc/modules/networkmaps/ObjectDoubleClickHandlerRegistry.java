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
package org.netxms.nxmc.modules.networkmaps;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.ServiceLoader;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.services.ObjectDoubleClickHandler;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Registry for object double click handlers
 */
public class ObjectDoubleClickHandlerRegistry
{
   private static final Logger logger = LoggerFactory.getLogger(ObjectDoubleClickHandlerRegistry.class);

   private View view;
   private List<ObjectDoubleClickHandler> doubleClickHandlers = new ArrayList<ObjectDoubleClickHandler>(0);
   private boolean defaultHandlerEnabled = true;

   /**
    * Create new registry
    * 
    * @param view owning view
    */
   public ObjectDoubleClickHandlerRegistry(View view)
   {
      this.view = view;
      registerDoubleClickHandlers();
   }

   /**
    * Handle double click on object by executing appropriate handler
    * 
    * @param object object to process
    * @return true if double click event was handled
    */
   public boolean handleDoubleClick(AbstractObject object)
   {
      for(ObjectDoubleClickHandler h : doubleClickHandlers)
      {
         if ((h.enabledFor() == null) || (h.enabledFor().isInstance(object)))
         {
            if (h.onDoubleClick(object, view))
            {
               return true;
            }
         }
      }

      // Open drill-down object as fallback action
      if (defaultHandlerEnabled && (object.getDrillDownObjectId() != 0))
      {
         AbstractObject drillDownObject = Registry.getSession().findObjectById(object.getDrillDownObjectId());
         if (drillDownObject instanceof NetworkMap)
         {
            /* FIXME: show network map */
         }
         else if (drillDownObject instanceof Dashboard)
         {
            /* FIXME: show dashboard */
         }
      }

      return false;
   }

   /**
    * @return the defaultHandlerEnabled
    */
   public boolean isDefaultHandlerEnabled()
   {
      return defaultHandlerEnabled;
   }

   /**
    * @param defaultHandlerEnabled the defaultHandlerEnabled to set
    */
   public void setDefaultHandlerEnabled(boolean defaultHandlerEnabled)
   {
      this.defaultHandlerEnabled = defaultHandlerEnabled;
   }

   /**
    * Register double click handlers
    */
   private void registerDoubleClickHandlers()
   {
      // Read all registered extensions and create handlers
      ServiceLoader<ObjectDoubleClickHandler> loader = ServiceLoader.load(ObjectDoubleClickHandler.class, getClass().getClassLoader());
      for(ObjectDoubleClickHandler h : loader)
      {
         logger.debug("Adding object double click handler " + h.getDescription());
         doubleClickHandlers.add(h);
      }
      doubleClickHandlers.sort(new Comparator<ObjectDoubleClickHandler>() {
         @Override
         public int compare(ObjectDoubleClickHandler h1, ObjectDoubleClickHandler h2)
         {
            return h1.getPriority() - h2.getPriority();
         }
      });
   }
}
