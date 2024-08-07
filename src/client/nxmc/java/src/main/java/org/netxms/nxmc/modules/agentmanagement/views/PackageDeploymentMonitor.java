/**
 * NetXMS - open source network management system
 * Copyright (C) 2022-2023 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.views;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.PackageDeployment;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentStatus;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentStatusComparator;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.DeploymentStatusLabelProvider;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.RefreshTimer;
import org.xnap.commons.i18n.I18n;

/**
 * Package deployment monitor
 */
public class PackageDeploymentMonitor extends ObjectView
{
   private I18n i18n = LocalizationHelper.getI18n(PackageDeploymentMonitor.class);
	public static final String ID = "PackageDeploymentMonitor"; //$NON-NLS-1$

	public static final int COLUMN_NODE = 0;
	public static final int COLUMN_STATUS = 1;
	public static final int COLUMN_ERROR = 2;

	private SortableTableViewer viewer;
	private long packageId;
	private Set<Long> applicableObjects;
	private Action actionRedeploy;
   private Action actionExportSelectionToCsv;
   private Action actionExportAllToCsv;
	private PackageDeployment listener;
   private RefreshTimer refreshTimer;

	/**
	 * Default constructor
	 */
   public PackageDeploymentMonitor()
   {
      super(LocalizationHelper.getI18n(PackageDeploymentMonitor.class).tr("Package Deployment Monitor"), ResourceManager.getImageDescriptor("icons/object-views/package_deploy.gif"), ID + UUID.randomUUID().toString(), false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      PackageDeploymentMonitor view = (PackageDeploymentMonitor)super.cloneView();
      view.packageId = packageId;
      view.applicableObjects = applicableObjects;
      view.listener = listener;
      return view;
   }   
	
   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
		final String[] names = { i18n.tr("Node"), i18n.tr("Status"), i18n.tr("Message") };
		final int[] widths = { 200, 110, 400 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NODE, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new DeploymentStatusLabelProvider());
		viewer.setComparator(new DeploymentStatusComparator());
      
      refreshTimer = new RefreshTimer(100, viewer.getControl(), new Runnable() {
          @Override
          public void run()
          {
             getWindow().getShell().getDisplay().asyncExec(new Runnable() {
                @Override
                public void run()
                {
                   viewer.setInput(listener.getDeployments().toArray());
                }
             });
          }
       });
      
      listener.addMonitor(this);
		
	   createActions();
      createPopupMenu();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/**
	 * Update deployment status
	 * 
	 * @param statusArray 
	 */
	public void viewStatusUpdate()
	{
	   refreshTimer.execute();
	}
   
	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
      actionExportSelectionToCsv = new ExportToCsvAction(this, viewer, true);
	   actionRedeploy = new Action(i18n.tr("Restart failed installation"), SharedIcons.RESTART) { 
         @Override
         public void run()
         {
            redeployPackages();
         }
      };
	}
	
	/**
	 * Start package redeployment
	 */
	private void redeployPackages()
	{
	   final Set<Long> objects = new HashSet<Long>();
	   Collection<DeploymentStatus> array =  listener.getDeployments();
	   for(DeploymentStatus obj : array)
	   {
	      if(obj.getStatus() == 4) //status failed
	      {
	         objects.add(obj.getNodeObject().getObjectId());
	      }
	   }

      Job job = new Job(i18n.tr("Deploy agent package"), this) {
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.deployPackage(packageId, objects.toArray(new Long[objects.size()]), listener);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot start package deployment");
         }
      };
      job.setUser(false);
      job.start();
	   
	}
   
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllToCsv);
      manager.add(actionRedeploy);
      manager.add(new Separator());
      super.fillLocalToolBar(manager);
   }

   /**
    * @param packageId the packageId to set
    */
   public void setPackageId(long packageId)
   {
      this.packageId = packageId;
   }

   /**
    * @param packageId the packageId to set
    */
   public void setApplicableObjects(final Set<Long> objects)
   {
      this.applicableObjects = objects;
   }
   
   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionExportSelectionToCsv);
      manager.add(actionExportAllToCsv);
   }

   /**
    * Set update listener
    * 
    * @param listener
    */
   public void setPackageDeploymentListener(PackageDeployment listener)
   {
      this.listener = listener;
   }

   @Override
   public boolean isValidForContext(Object context)
   {
      if (context != null && context instanceof AbstractObject)
      {
         for (long objectId : applicableObjects)
         {
            if (((AbstractObject)context).isChildOf(objectId) || ((AbstractObject)context).getObjectId() == objectId)
               return true;
         }
      }
      return false;
   }

   @Override
   public boolean isCloseable()
   {
      return true;
   }

   @Override
   public void dispose()
   {
      listener.removeMonitor(this);
      super.dispose();
   }

   public void deploymentComplete()
   {
      getWindow().getShell().getDisplay().asyncExec(new Runnable() {
         @Override
         public void run()
         {   
            addMessage(MessageArea.INFORMATION, i18n.tr("Package deployment completed"));
         }
      });
   } 

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      throw(new ViewNotRestoredException(i18n.tr("Not restorable view")));
   }  
}
