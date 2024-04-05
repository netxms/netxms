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
import org.netxms.client.asset.AssetAttribute;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.assetmanagement.views.AssetManagementSchemaManager;
import org.xnap.commons.i18n.I18n;

/**
 * Asset attribute list label provider
 */
public class AssetAttributeListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   final I18n i18n = LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class);

   public final static String[] DATA_TYPES = { 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("String"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("Integer"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("Number"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("Boolean"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("Enum"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("MAC address"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("IP address"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("UUID"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("Object reference"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("Date") };
   public final static String[] SYSTEM_TYPE = { 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("None"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("Serial"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("IP address"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("MAC address"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("Vendor"), 
         LocalizationHelper.getI18n(AssetAttributeListLabelProvider.class).tr("Model") };

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AssetAttribute attr = (AssetAttribute)element;
      switch(columnIndex)
      {
         case AssetManagementSchemaManager.COLUMN_DATA_TYPE:
            return DATA_TYPES[attr.getDataType().getValue()];
         case AssetManagementSchemaManager.COLUMN_DISPLAY_NAME:
            return attr.getDisplayName();
         case AssetManagementSchemaManager.COLUMN_HAS_SCRIPT:
            return attr.getAutofillScript().isEmpty() ? i18n.tr("No") : i18n.tr("Yes");
         case AssetManagementSchemaManager.COLUMN_IS_HIDDEN:
            return attr.isHidden() ? i18n.tr("Yes") : i18n.tr("No");
         case AssetManagementSchemaManager.COLUMN_IS_MANDATORY:
            return attr.isMandatory() ? i18n.tr("Yes") : i18n.tr("No");
         case AssetManagementSchemaManager.COLUMN_IS_UNIQUE:
            return attr.isUnique() ? i18n.tr("Yes") : i18n.tr("No");
         case AssetManagementSchemaManager.COLUMN_NAME:
            return attr.getName();
         case AssetManagementSchemaManager.COLUMN_RANGE_MAX:
            return Integer.toString(attr.getRangeMax());
         case AssetManagementSchemaManager.COLUMN_RANGE_MIN:
            return Integer.toString(attr.getRangeMin());
         case AssetManagementSchemaManager.COLUMN_SYSTEM_TYPE:
            return SYSTEM_TYPE[attr.getSystemType().getValue()];
         
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }
}
