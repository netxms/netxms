/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.api;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objectbrowser.Activator;

/**
 * Registry for object open handlers
 */
public class ObjectOpenHandlerRegistry
{
   private List<OpenHandlerData> openHandlers = new ArrayList<OpenHandlerData>(0);

   /**
    * Create new registry 
    */
   public ObjectOpenHandlerRegistry()
   {
      registerOpenHandlers();
   }

   /**
    * Call object open handler
    * 
    * @return
    */
   public boolean callOpenObjectHandler(AbstractObject object)
   {
      for(OpenHandlerData h : openHandlers)
      {
         if ((h.enabledFor == null) || (h.enabledFor.isInstance(object)))
         {
            if (h.handler.openObject(object))
               return true;
         }
      }
      return false;
   }
   
   /**
    * Register object open handlers
    */
   private void registerOpenHandlers()
   {
      // Read all registered extensions and create handlers
      final IExtensionRegistry reg = Platform.getExtensionRegistry();
      IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.objectbrowser.objectOpenHandlers"); //$NON-NLS-1$
      for(int i = 0; i < elements.length; i++)
      {
         try
         {
            final OpenHandlerData h = new OpenHandlerData();
            h.handler = (ObjectOpenHandler)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
            h.priority = safeParseInt(elements[i].getAttribute("priority")); //$NON-NLS-1$
            final String className = elements[i].getAttribute("enabledFor"); //$NON-NLS-1$
            try
            {
               h.enabledFor = (className != null) ? Class.forName(className) : null;
            }
            catch(Exception e)
            {
               h.enabledFor = null;
            }
            openHandlers.add(h);
         }
         catch(CoreException e)
         {
            Activator.logError("Exception in registerOpenHandlers()", e);
         }
      }
      
      // Sort handlers by priority
      Collections.sort(openHandlers, new Comparator<OpenHandlerData>() {
         @Override
         public int compare(OpenHandlerData arg0, OpenHandlerData arg1)
         {
            return arg0.priority - arg1.priority;
         }
      });
   }
   
   /**
    * @param s
    * @return
    */
   private static int safeParseInt(String s)
   {
      if (s == null)
         return 65535;
      try
      {
         return Integer.parseInt(s);
      }
      catch(NumberFormatException e)
      {
         return 65535;
      }
   }

   /**
    * Internal data for object open handlers
    */
   private class OpenHandlerData
   {
      ObjectOpenHandler handler;
      int priority;
      Class<?> enabledFor;
   }
}
