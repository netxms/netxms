/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
package org.netxms.ui.eclipse.networkmaps.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.Activator;

/**
 * Internal connection topology view for given object
 */
public class InternalConnectionTopology extends AbstractNetworkMapView
{
   public static final String ID = "org.netxms.ui.eclipse.networkmaps.views.InternalConnectionMap";
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.networkmaps.views.AbstractNetworkMapView#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      setPartName("Internal Connection Map - " + rootObject.getObjectName());
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.networkmaps.views.AbstractNetworkMapView#buildMapPage()
    */
   @Override
   protected void buildMapPage()
   {
      if (mapPage == null)
         mapPage = new NetworkMapPage(ID + rootObject.getObjectName());
      
      new ConsoleJob(String.format("Get internal connection map for %s", rootObject.getObjectName()), this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            NetworkMapPage page = session.queryInternalConnectionTopology(rootObject.getObjectId());
            replaceMapPage(page, getDisplay());
         }

         @Override
         protected void jobFailureHandler()
         {
            // On failure, create map with root object only
            NetworkMapPage page = new NetworkMapPage(ID+rootObject.getObjectName());
            page.addElement(new NetworkMapObject(mapPage.createElementId(), rootObject.getObjectId()));
            replaceMapPage(page, getDisplay());
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot get internal connection map for %s", rootObject.getObjectName());
         }
      }.start();
   }
}
