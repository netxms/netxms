/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Layer 2 topology view for given object
 */
public class L2TopologyMapView extends AdHocTopologyMapView
{
   private final I18n i18n = LocalizationHelper.getI18n(L2TopologyMapView.class);

   private Boolean usePhysicalLinks;
   private Action actionShowPhysicalLinks;

   /**
    * Constructor
    *
    * @param rootObjectId root object ID
    */
   public L2TopologyMapView(long rootObjectId)
   {
      super(LocalizationHelper.getI18n(L2TopologyMapView.class).tr("Layer 2 Topology"), ResourceManager.getImageDescriptor("icons/object-views/layer2.png"), "objects.maps.l2-topology", rootObjectId);
   }

   /**
    * Constructor for cloning
    */
   protected L2TopologyMapView()
   {
      super(LocalizationHelper.getI18n(L2TopologyMapView.class).tr("Layer 2 Topology"), ResourceManager.getImageDescriptor("icons/object-views/layer2.png"), "objects.maps.l2-topology", 0);
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AdHocTopologyMapView#cloneView()
    */
   @Override
   public View cloneView()
   {
      L2TopologyMapView view = (L2TopologyMapView)super.cloneView();
      view.usePhysicalLinks = usePhysicalLinks;
      return view;
   }  

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#createActions()
    */
   @Override
   protected void createActions()
   {
      actionShowPhysicalLinks = new Action("Show physical links", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            usePhysicalLinks = actionShowPhysicalLinks.isChecked();
            refresh();
         }
      };
      
      super.createActions();
   } 

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#fillMapContextMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillMapContextMenu(IMenuManager manager)
   {
      super.fillMapContextMenu(manager);
      manager.add(new Separator());
      manager.add(actionShowPhysicalLinks);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      super.fillLocalMenu(manager);
      manager.add(new Separator());
      manager.add(actionShowPhysicalLinks);
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.views.AbstractNetworkMapView#buildMapPage()
    */
   @Override
	protected void buildMapPage(NetworkMapPage oldMapPage)
	{
		if (mapPage == null)
         mapPage = new NetworkMapPage("L2Topology." + getObjectName());

		new Job(String.format(i18n.tr("Get layer 2 topology for %s"), getObjectName()), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
			   if (usePhysicalLinks == null)
			   {
			      usePhysicalLinks = session.getPublicServerVariableAsBoolean("Topology.AdHocRequest.IncludePhysicalLinks");
	            runInUIThread(new Runnable() {               
	               @Override
	               public void run()
	               {
	                  actionShowPhysicalLinks.setChecked(usePhysicalLinks);
	               }
	            });
			   }

				NetworkMapPage page = session.queryLayer2Topology(getObjectId(), -1, usePhysicalLinks);
            session.syncMissingObjects(page.getAllLinkStatusObjects(), 0, NXCSession.OBJECT_SYNC_WAIT);
				replaceMapPage(page, getDisplay());
			}

			@Override
			protected void jobFailureHandler(Exception e)
			{
				// On failure, create map with root object only
            NetworkMapPage page = new NetworkMapPage("L2Topology." + getObjectName());
				page.addElement(new NetworkMapObject(mapPage.createElementId(), getObjectId()));
				replaceMapPage(page, getDisplay());
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format(i18n.tr("Cannot get layer 2 topology for %s"), getObjectName());
			}
		}.start();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("usePhysicalLinks", usePhysicalLinks);
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      usePhysicalLinks = memento.getAsBoolean("usePhysicalLinks", false);     
   }
}
