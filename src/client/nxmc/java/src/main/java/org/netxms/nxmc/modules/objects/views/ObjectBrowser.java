/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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

import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
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
      objectTree = new ObjectTree(parent, SWT.NONE, ObjectTree.MULTI, calculateRootObjects(), null, true, true);

      Menu menu = new ObjectContextMenuManager(this, objectTree.getSelectionProvider()).createContextMenu(objectTree.getTreeControl());
      objectTree.getTreeControl().setMenu(menu);
   }

   /**
    * Calculate root objects based on subtree type.
    *
    * @return root objects
    */
   private long[] calculateRootObjects()
   {
      // TODO: handle situation when root object inaccessible to current user
      switch(subtreeType)
      {
         case INFRASTRUCTURE:
            return new long[] { AbstractObject.SERVICEROOT };
         case MAPS:
            return new long[] { AbstractObject.NETWORKMAPROOT };
         case NETWORK:
            return new long[] { AbstractObject.NETWORK };
         case TEMPLATES:
            return new long[] { AbstractObject.TEMPLATEROOT };
         default:
            return new long[0];
      }
   }
}
