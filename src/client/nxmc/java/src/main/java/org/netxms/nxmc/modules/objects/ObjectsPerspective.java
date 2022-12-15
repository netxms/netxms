/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects;

import java.util.ArrayList;
import java.util.List;
import java.util.ServiceLoader;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.AgentConfigEditorView;
import org.netxms.nxmc.modules.alarms.views.AlarmsView;
import org.netxms.nxmc.modules.businessservice.views.BusinessServiceAvailabilityView;
import org.netxms.nxmc.modules.businessservice.views.BusinessServiceChecksView;
import org.netxms.nxmc.modules.dashboards.views.ContextDashboardView;
import org.netxms.nxmc.modules.dashboards.views.DashboardView;
import org.netxms.nxmc.modules.datacollection.views.DataCollectionView;
import org.netxms.nxmc.modules.datacollection.views.PerformanceView;
import org.netxms.nxmc.modules.datacollection.views.SummaryDataCollectionView;
import org.netxms.nxmc.modules.filemanager.views.AgentFileManager;
import org.netxms.nxmc.modules.networkmaps.views.PredefinedMap;
import org.netxms.nxmc.modules.nxsl.views.ScriptExecutorView;
import org.netxms.nxmc.modules.objects.views.ChassisView;
import org.netxms.nxmc.modules.objects.views.Dot1xStatusView;
import org.netxms.nxmc.modules.objects.views.EntityMIBView;
import org.netxms.nxmc.modules.objects.views.HardwareInventoryView;
import org.netxms.nxmc.modules.objects.views.InterfacesView;
import org.netxms.nxmc.modules.objects.views.MaintenanceJournalView;
import org.netxms.nxmc.modules.objects.views.OSPFView;
import org.netxms.nxmc.modules.objects.views.ObjectBrowser;
import org.netxms.nxmc.modules.objects.views.ObjectOverviewView;
import org.netxms.nxmc.modules.objects.views.PhysicalLinkView;
import org.netxms.nxmc.modules.objects.views.PortView;
import org.netxms.nxmc.modules.objects.views.ProcessesView;
import org.netxms.nxmc.modules.objects.views.RackView;
import org.netxms.nxmc.modules.objects.views.ServicesView;
import org.netxms.nxmc.modules.objects.views.SoftwareInventoryView;
import org.netxms.nxmc.modules.objects.views.StatusMapView;
import org.netxms.nxmc.modules.objects.views.SwitchForwardingDatabaseView;
import org.netxms.nxmc.modules.snmp.views.MibExplorer;
import org.netxms.nxmc.modules.worldmap.views.ObjectGeoLocationView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.services.ObjectPerspectiveElement;
import org.netxms.nxmc.tools.FontTools;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Object browser perspective
 */
public abstract class ObjectsPerspective extends Perspective
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectsPerspective.class);

   private SubtreeType subtreeType;
   private Composite headerArea;
   private Label objectName;
   private ToolBar objectToolBar;
   private ToolBar objectMenuBar;
   private Image imageEditConfig;
   private Image imageExecuteScript;
   private List<ObjectPerspectiveElement> additionalElements = new ArrayList<>();

   /**
    * Create new object perspective
    *
    * @param id
    * @param name
    * @param image
    * @param subtreeType
    */
   protected ObjectsPerspective(String id, String name, Image image, SubtreeType subtreeType)
   {
      super(id, name, image);
      this.subtreeType = subtreeType;
      imageEditConfig = ResourceManager.getImage("icons/object-views/agent-config.png");
      imageExecuteScript = ResourceManager.getImage("icons/object-views/script-executor.png");

      ServiceLoader<ObjectPerspectiveElement> loader = ServiceLoader.load(ObjectPerspectiveElement.class, getClass().getClassLoader());
      for(ObjectPerspectiveElement e : loader)
         additionalElements.add(e);
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = true;
      configuration.enableNavigationHistory = true;
      configuration.hasSupplementalArea = false;
      configuration.multiViewNavigationArea = false;
      configuration.multiViewMainArea = true;
      configuration.hasHeaderArea = true;
      configuration.priority = 20;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      addNavigationView(new ObjectBrowser(getName(), null, subtreeType));
      addMainView(new AgentFileManager());
      addMainView(new AlarmsView());
      addMainView(new BusinessServiceAvailabilityView());
      addMainView(new BusinessServiceChecksView());
      addMainView(new ChassisView());
      addMainView(new DashboardView());
      addMainView(new DataCollectionView());
      addMainView(new Dot1xStatusView());
      addMainView(new EntityMIBView());
      addMainView(new HardwareInventoryView());
      addMainView(new InterfacesView());
      addMainView(new MaintenanceJournalView());
      addMainView(new MibExplorer());
      addMainView(new ObjectGeoLocationView());
      addMainView(new ObjectOverviewView());
      addMainView(new OSPFView());
      addMainView(new PerformanceView());
      addMainView(new PhysicalLinkView());
      addMainView(new PortView());
      addMainView(new PredefinedMap());
      addMainView(new ProcessesView());
      addMainView(new RackView());
      addMainView(new ServicesView());
      addMainView(new SoftwareInventoryView());
      addMainView(new StatusMapView());
      addMainView(new SummaryDataCollectionView());
      addMainView(new SwitchForwardingDatabaseView());

      NXCSession session = Registry.getSession();
      for(ObjectPerspectiveElement e : additionalElements)
      {
         String componentId = e.getRequiredComponentId();
         if ((componentId == null) || session.isServerComponentRegistered(componentId))
            addMainView(e.createView());
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#createHeaderArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createHeaderArea(Composite parent)
   {
      headerArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 5;
      headerArea.setLayout(layout);

      objectName = new Label(headerArea, SWT.LEFT);
      objectName.setFont(FontTools.createTitleFont());

      Label separator = new Label(headerArea, SWT.SEPARATOR | SWT.VERTICAL);
      GridData gd = new GridData(SWT.CENTER, SWT.FILL, false, true);
      gd.heightHint = 24;
      separator.setLayoutData(gd);

      objectToolBar = new ToolBar(headerArea, SWT.FLAT | SWT.HORIZONTAL | SWT.RIGHT);

      separator = new Label(headerArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData(SWT.CENTER, SWT.FILL, false, true);
      gd.heightHint = 24;
      separator.setLayoutData(gd);

      objectMenuBar = new ToolBar(headerArea, SWT.FLAT | SWT.HORIZONTAL | SWT.RIGHT);
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#navigationSelectionChanged(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   protected void navigationSelectionChanged(IStructuredSelection selection)
   {
      super.navigationSelectionChanged(selection);
      if (selection.getFirstElement() instanceof AbstractObject)
      {
         AbstractObject object = (AbstractObject)selection.getFirstElement();
         objectName.setText(object.getNameWithAlias());
         updateObjectToolBar(object);
         updateObjectMenuBar(object);
         updateContextDashboards(object);
      }
      else
      {
         objectName.setText("");
      }
      headerArea.layout();
   }

   /**
    * Update object toolbar
    *
    * @param object selected object
    */
   private void updateObjectToolBar(final AbstractObject object)
   {
      for(ToolItem item : objectToolBar.getItems())
         item.dispose();

      addObjectToolBarItem(i18n.tr("Properties"), SharedIcons.IMG_PROPERTIES, new Runnable() {
         @Override
         public void run()
         {
            ObjectPropertiesManager.openObjectPropertiesDialog(object, getWindow().getShell());
         }
      });

      addObjectToolBarItem(i18n.tr("Execute script"), imageExecuteScript, new Runnable() {
         @Override
         public void run()
         {
            addMainView(new ScriptExecutorView(object.getObjectId()), true, false);
         }
      });

      if ((object instanceof Node) && ((Node)object).hasAgent())
      {
         addObjectToolBarItem(i18n.tr("Edit agent configuration"), imageEditConfig, new Runnable() {
            @Override
            public void run()
            {
               addMainView(new AgentConfigEditorView((Node)object), true, false);
            }
         });
      }

      addObjectToolBarItem(i18n.tr("Delete"), SharedIcons.IMG_DELETE_OBJECT, new Runnable() {
         @Override
         public void run()
         {
            String question = String.format(i18n.tr("Are you sure you want to delete \"%s\"?"), object.getObjectName());
            boolean confirmed = MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Delete"), question);
            
            if (confirmed)
            {
               final NXCSession session =  Registry.getSession();
               new Job(i18n.tr("Delete objects"), null) {
                  @Override
                  protected void run(IProgressMonitor monitor) throws Exception
                  {
                     session.deleteObject(object.getObjectId());
                  }
                  
                  @Override
                  protected String getErrorMessage()
                  {
                     return i18n.tr("Cannot delete object");
                  }
               }.start();
            }
         }
      });
   }

   /**
    * Add object toolbar item.
    *
    * @param name tooltip text
    * @param icon icon
    * @param handler selection handler
    */
   private void addObjectToolBarItem(String name, Image icon, final Runnable handler)
   {
      ToolItem item = new ToolItem(objectToolBar, SWT.PUSH);
      item.setImage(icon);
      item.setToolTipText(name);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            handler.run();
         }
      });
   }

   /**
    * Update object menu bar
    *
    * @param object selected object
    */
   private void updateObjectMenuBar(final AbstractObject object)
   {
      for(ToolItem item : objectMenuBar.getItems())
         item.dispose();

      addObjectMenu(i18n.tr("Tools"), ObjectMenuFactory.createToolsMenu(new StructuredSelection(object), null, objectToolBar, new ViewPlacement(this)));
      addObjectMenu(i18n.tr("Poll"), ObjectMenuFactory.createPollMenu(new StructuredSelection(object), null, objectToolBar, new ViewPlacement(this)));
      addObjectMenu(i18n.tr("Create"), new ObjectCreateMenuManager(getWindow().getShell(), null, object));
   }

   /**
    * Add drop-down menu for current object
    *
    * @param name menu name
    * @param menu menu
    */
   private void addObjectMenu(String name, final Menu menu)
   {
      if ((menu != null) && (menu.getItemCount() != 0))
         createMenuToolItem(name, null, menu);
   }

   /**
    * Add drop-down menu for current object
    *
    * @param name menu name
    * @param menu menu
    */
   private void addObjectMenu(String name, final MenuManager menuManager)
   {
      if ((menuManager != null) && !menuManager.isEmpty())
         createMenuToolItem(name, menuManager, null);
   }

   /**
    * Create toolbar item that will show drop down menu.
    *
    * @param name item name
    * @param menuManager menu manager that will provide menu or null if menu provided directly
    * @param menu menu to show or null if menu provided by manager
    */
   private void createMenuToolItem(String name, final MenuManager menuManager, final Menu menu)
   {
      ToolItem item = new ToolItem(objectMenuBar, SWT.PUSH);
      item.setText("  " + name + " \u25BE  ");
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            Rectangle rect = item.getBounds();
            Point pt = new Point(rect.x, rect.y + rect.height);
            pt = objectMenuBar.toDisplay(pt);
            Menu m = (menuManager != null) ? menuManager.createContextMenu(objectToolBar) : menu;
            m.setLocation(pt.x, pt.y);
            m.setVisible(true);
         }
      });
   }

   /**
    * Update context dashboard views after object selection change.
    *
    * @param object selected object
    */
   private void updateContextDashboards(AbstractObject object)
   {
      if (!((object instanceof DataCollectionTarget) || (object instanceof Container) || (object instanceof Rack) || (object instanceof EntireNetwork) || (object instanceof ServiceRoot) ||
            (object instanceof Subnet) || (object instanceof Zone) || (object instanceof Condition)))
         return;

      for(AbstractObject d : object.getDashboards(true))
      {
         if ((((Dashboard)d).getFlags() & Dashboard.SHOW_AS_OBJECT_VIEW) == 0)
            continue;

         String viewId = "ContextDashboard." + d.getObjectId();
         if (findMainView(viewId) == null)
         {
            addMainView(new ContextDashboardView((Dashboard)d));
         }
      }
   }
}
