/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.NavigationView;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.SubtreeType;
import org.netxms.nxmc.modules.objects.widgets.ObjectTree;

/**
 * Object browser - displays object tree.
 */
public class ObjectBrowser extends NavigationView
{
   private SubtreeType subtreeType;
   private ObjectTree objectTree;

   /**
    * @param name
    * @param image
    */
   public ObjectBrowser(String name, ImageDescriptor image, SubtreeType subtreeType)
   {
      super(name, image, "ObjectBrowser." + subtreeType.toString());
      this.subtreeType = subtreeType;
   }

   /**
    * @see org.netxms.nxmc.base.views.NavigationView#getSelectionProvider()
    */
   @Override
   public ISelectionProvider getSelectionProvider()
   {
      return (objectTree != null) ? objectTree.getSelectionProvider() : null;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      objectTree = new ObjectTree(parent, SWT.NONE, ObjectTree.MULTI, calculateClassFilter(), true, true);

      Menu menu = new ObjectContextMenuManager(this, objectTree.getSelectionProvider()).createContextMenu(objectTree.getTreeControl());
      objectTree.getTreeControl().setMenu(menu);
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
