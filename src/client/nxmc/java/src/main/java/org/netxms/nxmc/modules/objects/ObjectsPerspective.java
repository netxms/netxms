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
package org.netxms.nxmc.modules.objects;

import java.util.HashSet;
import java.util.Set;
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.views.AlarmsView;
import org.netxms.nxmc.modules.datacollection.views.DataCollectionView;
import org.netxms.nxmc.modules.datacollection.views.PerformanceView;
import org.netxms.nxmc.modules.objects.views.Dot1xStatusView;
import org.netxms.nxmc.modules.objects.views.EntityMIBView;
import org.netxms.nxmc.modules.objects.views.InterfacesView;
import org.netxms.nxmc.modules.objects.views.ObjectBrowser;
import org.netxms.nxmc.modules.objects.views.ObjectOverviewView;
import org.netxms.nxmc.modules.objects.views.SwitchForwardingDatabaseView;
import org.netxms.nxmc.modules.objects.widgets.ObjectTree;
import org.netxms.nxmc.modules.snmp.views.MibExplorer;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.FontTools;
import org.xnap.commons.i18n.I18n;

/**
 * Object browser perspective
 */
public abstract class ObjectsPerspective extends Perspective
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectsPerspective.class);

   private SubtreeType subtreeType;
   private ObjectTree objectTree;
   private Composite headerArea;
   private Label objectName;
   private ToolBar objectToolBar;
   private ToolBar objectMenuBar;

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
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = true;
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
      addNavigationView(new ObjectBrowser("Objects", null, subtreeType));
      addMainView(new ObjectOverviewView());
      addMainView(new AlarmsView());
      addMainView(new DataCollectionView());
      addMainView(new InterfacesView());
      addMainView(new EntityMIBView());
      addMainView(new PerformanceView());
      addMainView(new Dot1xStatusView());
      addMainView(new SwitchForwardingDatabaseView());
      addMainView(new MibExplorer());
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
         objectName.setText(object.getObjectName());
         updateObjectToolBar(object);
         updateObjectMenuBar(object);
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

      addObjectToolBarItem(i18n.tr("Properties"), SharedIcons.IMG_EDIT, new Runnable() {
         @Override
         public void run()
         {
            ObjectPropertiesManager.openObjectPropertiesDialog(object, getWindow().getShell());
         }
      });

      addObjectToolBarItem(i18n.tr("Delete"), SharedIcons.IMG_DELETE_OBJECT, new Runnable() {
         @Override
         public void run()
         {
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
    * Update object toolbar
    *
    * @param object selected object
    */
   private void updateObjectMenuBar(final AbstractObject object)
   {
      for(ToolItem item : objectMenuBar.getItems())
         item.dispose();

      addObjectMenu(i18n.tr("Tools"), ObjectToolsMenuFactory.createMenu(new StructuredSelection(object), null, objectToolBar, getWindow()));
   }

   /**
    * Add drop-down menu for current object
    *
    * @param name menu name
    * @param menu menu
    */
   private void addObjectMenu(String name, final Menu menu)
   {
      if ((menu == null) || (menu.getItemCount() == 0))
         return;

      ToolItem item = new ToolItem(objectMenuBar, SWT.PUSH);
      item.setText(name + " \u25BE");
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            Rectangle rect = item.getBounds();
            Point pt = new Point(rect.x, rect.y + rect.height);
            pt = objectMenuBar.toDisplay(pt);
            menu.setLocation(pt.x, pt.y);
            menu.setVisible(true);
         }
      });
   }

   /**
    * Calculate class filter based on subtree type.
    *
    * @return root objects
    */
   private Set<Integer> calculateClassFilter()
   {
      Set<Integer> classFilter = new HashSet<Integer>();
      switch(subtreeType)
      {
         case INFRASTRUCTURE:
            classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
            classFilter.add(AbstractObject.OBJECT_CONTAINER);
            classFilter.add(AbstractObject.OBJECT_CLUSTER);
            classFilter.add(AbstractObject.OBJECT_CHASSIS);
            classFilter.add(AbstractObject.OBJECT_RACK);
            classFilter.add(AbstractObject.OBJECT_NODE);
            classFilter.add(AbstractObject.OBJECT_INTERFACE);
            classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
            classFilter.add(AbstractObject.OBJECT_VPNCONNECTOR);
            classFilter.add(AbstractObject.OBJECT_NETWORKSERVICE);
            classFilter.add(AbstractObject.OBJECT_CONDITION);
            classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
            classFilter.add(AbstractObject.OBJECT_SENSOR);
            break;
         case MAPS:
            classFilter.add(AbstractObject.OBJECT_NETWORKMAP);
            classFilter.add(AbstractObject.OBJECT_NETWORKMAPGROUP);
            classFilter.add(AbstractObject.OBJECT_NETWORKMAPROOT);
            break;
         case NETWORK:
            classFilter.add(AbstractObject.OBJECT_NETWORK);
            classFilter.add(AbstractObject.OBJECT_ZONE);
            classFilter.add(AbstractObject.OBJECT_SUBNET);
            classFilter.add(AbstractObject.OBJECT_NODE);
            classFilter.add(AbstractObject.OBJECT_INTERFACE);
            classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
            classFilter.add(AbstractObject.OBJECT_VPNCONNECTOR);
            classFilter.add(AbstractObject.OBJECT_NETWORKSERVICE);
            break;
         case TEMPLATES:
            classFilter.add(AbstractObject.OBJECT_TEMPLATE);
            classFilter.add(AbstractObject.OBJECT_TEMPLATEGROUP);
            classFilter.add(AbstractObject.OBJECT_TEMPLATEROOT);
            break;
         case DASHBOARDS:
            classFilter.add(AbstractObject.OBJECT_DASHBOARD);
            classFilter.add(AbstractObject.OBJECT_DASHBOARDGROUP);
            classFilter.add(AbstractObject.OBJECT_DASHBOARDROOT);
            break;
         case BUSINESS_SERVICES:
            classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICE);
            classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICEROOT);
            classFilter.add(AbstractObject.OBJECT_NODELINK);
            classFilter.add(AbstractObject.OBJECT_SLMCHECK);
            break;
      }
      return classFilter;
   }
}
