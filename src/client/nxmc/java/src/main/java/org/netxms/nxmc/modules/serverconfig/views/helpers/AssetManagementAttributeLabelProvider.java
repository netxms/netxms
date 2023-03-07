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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.asset.AssetManagementAttribute;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.views.AssetManagementAttributesView;
import org.xnap.commons.i18n.I18n;

/**
 * Asset management attribute label provider
 */
public class AssetManagementAttributeLabelProvider extends LabelProvider implements ITableLabelProvider
{
   final static I18n i18n = LocalizationHelper.getI18n(AssetManagementAttributeLabelProvider.class);
   
   public final static String[] DATA_TYPES = { i18n.tr("String"), i18n.tr("Integer"), i18n.tr("Number"), 
         i18n.tr("Boolean"), i18n.tr("Enum"), i18n.tr("MAC address"), i18n.tr("IP address"), 
         i18n.tr("UUID"), i18n.tr("Object reference") };
   public final static String[] SYSTEM_TYPE = { i18n.tr("None"), i18n.tr("Serial"), i18n.tr("IP address"), 
         i18n.tr("MAC address"), i18n.tr("Vendor"), i18n.tr("Model") };

   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AssetManagementAttribute attr = (AssetManagementAttribute)element;
      switch(columnIndex)
      {
         case AssetManagementAttributesView.NAME:
            return attr.getName();
         case AssetManagementAttributesView.DISPLAY_NAME:
            return attr.getDisplayName();
         case AssetManagementAttributesView.DATA_TYPE:
            return DATA_TYPES[attr.getDataType().getValue()];
         case AssetManagementAttributesView.IS_MANDATORY:
            return attr.isMandatory() ? i18n.tr("Yes") : i18n.tr("No");
         case AssetManagementAttributesView.IS_UNIQUE:
            return attr.isUnique() ? i18n.tr("Yes") : i18n.tr("No");
         case AssetManagementAttributesView.HAS_SCRIPT:
            return attr.getAutofillScript().isEmpty() ? i18n.tr("No") : i18n.tr("Yes");
         case AssetManagementAttributesView.RANGE_MIN:
            return Integer.toString(attr.getRangeMin());
         case AssetManagementAttributesView.RANGE_MAX:
            return Integer.toString(attr.getRangeMax());
         case AssetManagementAttributesView.SYSTEM_TYPE:
            return SYSTEM_TYPE[attr.getSystemType().getValue()];
         
      }
      return null;
   }

   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

}
