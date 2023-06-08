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

import org.eclipse.core.runtime.IProgressMonitor;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Internal connection topology view for given object
 */
public class InternalConnectionTopology extends AbstractNetworkMapView
{
   private static final I18n i18n = LocalizationHelper.getI18n(InternalConnectionTopology.class);
   public static final String ID = "InternalConnectionTopology";

   /**
    * Constructor
    */
   public InternalConnectionTopology()
   {
      super(i18n.tr("Internal connection map"), null, "InternalConnectionTopology");
   }
   
   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#buildMapPage()
    */
   @Override
   protected void buildMapPage()
   {
      if (mapPage == null)
         mapPage = new NetworkMapPage(ID + getObjectName());
      
      new Job(String.format("Get internal connection map for %s", getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            NetworkMapPage page = session.queryInternalConnectionTopology(getObjectId());
            replaceMapPage(page, getDisplay());
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            // On failure, create map with root object only
            NetworkMapPage page = new NetworkMapPage(ID + getObjectName());
            page.addElement(new NetworkMapObject(mapPage.createElementId(), getObjectId()));
            replaceMapPage(page, getDisplay());
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot get internal connection map for %s", getObjectName());
         }
      }.start();
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
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      refresh();
   }  
}
