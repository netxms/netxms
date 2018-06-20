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
package org.netxms.ui.eclipse.networkmaps.api;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.networkmaps.Messages;
import org.netxms.ui.eclipse.networkmaps.views.PredefinedMap;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Registry for object double click handlers
 */
public class ObjectDoubleClickHandlerRegistry
{
   private IViewPart viewPart;
   private List<DoubleClickHandlerData> doubleClickHandlers = new ArrayList<DoubleClickHandlerData>(0);
   private boolean defaultHandlerEnabled = true;

   /**
    * Create new registry 
    */
   public ObjectDoubleClickHandlerRegistry(IViewPart viewPart)
   {
      this.viewPart = viewPart;
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
      for(DoubleClickHandlerData h : doubleClickHandlers)
      {
         if ((h.enabledFor == null) || (h.enabledFor.isInstance(object)))
         {
            if (h.handler.onDoubleClick(object, viewPart))
            {
               return true;
            }
         }
      }

      // Open drill-down object as fallback action
      if (defaultHandlerEnabled && (object.getDrillDownObjectId() != 0))
      {
         AbstractObject drillDownObject = ConsoleSharedData.getSession().findObjectById(object.getDrillDownObjectId());
         if (drillDownObject instanceof NetworkMap)
         {
            try
            {
               viewPart.getSite().getPage().showView(PredefinedMap.ID, Long.toString(drillDownObject.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
            }
            catch(PartInitException e)
            {
               MessageDialogHelper.openError(viewPart.getSite().getShell(), Messages.get().AbstractNetworkMapView_Error, String.format("Cannot open drill-down object view: %s", e.getMessage()));
            }
         }
         else if (drillDownObject instanceof Dashboard)
         {
            try
            {
               viewPart.getSite().getPage().showView("org.netxms.ui.eclipse.dashboard.views.DashboardView", Long.toString(drillDownObject.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
            }
            catch(PartInitException e)
            {
               MessageDialogHelper.openError(viewPart.getSite().getShell(), Messages.get().AbstractNetworkMapView_Error, String.format("Cannot open drill-down object view: %s", e.getMessage()));
            }
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
      final IExtensionRegistry reg = Platform.getExtensionRegistry();
      IConfigurationElement[] elements = reg
            .getConfigurationElementsFor("org.netxms.ui.eclipse.networkmaps.objectDoubleClickHandlers"); //$NON-NLS-1$
      for(int i = 0; i < elements.length; i++)
      {
         try
         {
            final DoubleClickHandlerData h = new DoubleClickHandlerData();
            h.handler = (ObjectDoubleClickHandler)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
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
            doubleClickHandlers.add(h);
         }
         catch(CoreException e)
         {
            e.printStackTrace();
         }
      }

      // Sort handlers by priority
      Collections.sort(doubleClickHandlers, new Comparator<DoubleClickHandlerData>() {
         @Override
         public int compare(DoubleClickHandlerData arg0, DoubleClickHandlerData arg1)
         {
            return arg0.priority - arg1.priority;
         }
      });
   }

   /**
    * @param string
    * @return
    */
   private static int safeParseInt(String string)
   {
      if (string == null)
         return 65535;
      try
      {
         return Integer.parseInt(string);
      }
      catch(NumberFormatException e)
      {
         return 65535;
      }
   }

   /**
    * Internal data for object double click handlers
    */
   private class DoubleClickHandlerData
   {
      ObjectDoubleClickHandler handler;
      int priority;
      Class<?> enabledFor;
   }
}
