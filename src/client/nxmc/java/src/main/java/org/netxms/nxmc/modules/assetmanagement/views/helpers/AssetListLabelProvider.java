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
package org.netxms.nxmc.modules.assetmanagement.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;

/**
 * Asset list label provider
 */
public class AssetListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private SortableTableViewer viewer;
   private AssetPropertyReader propertyReader;
   private NXCSession session = Registry.getSession();
   private BaseObjectLabelProvider objectLabelProvider = new BaseObjectLabelProvider();

   /**
    * Create label provider.
    *
    * @param viewer owning viewer
    */
   public AssetListLabelProvider(SortableTableViewer viewer, AssetPropertyReader propertyReader)
   {
      super();
      this.viewer = viewer;
      this.propertyReader = propertyReader;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == 0)
         return objectLabelProvider.getImage(element);
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      if (columnIndex == 0)
         return objectLabelProvider.getText(element);

      String attribute = (String)viewer.getColumnById(columnIndex).getData("Attribute");
      if (attribute == null)
         return null;

      Asset asset = getAsset(element);
      if (asset == null)
         return null;

      return propertyReader.valueToText(attribute, asset.getProperties().get(attribute));
   }

   /**
    * Get asset from element
    *
    * @param element list element
    * @return asset object
    */
   private Asset getAsset(Object element)
   {
      if (element instanceof Asset)
         return (Asset)element;
      return session.findObjectById(((AbstractObject)element).getAssetId(), Asset.class);
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      objectLabelProvider.dispose();
      super.dispose();
   }   
}
