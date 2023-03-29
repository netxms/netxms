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
package org.netxms.nxmc.modules.objects.views.helpers;

import java.util.Map.Entry;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.AssetView;
import org.netxms.nxmc.modules.serverconfig.views.helpers.AssetManagementAttributeLabelProvider;
import org.xnap.commons.i18n.I18n;

/**
 * Asset instance label provider
 */
public class AssetAttributeInstanceLabelProvider extends LabelProvider implements ITableLabelProvider
{
   final I18n i18n = LocalizationHelper.getI18n(AssetView.class);

   NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @SuppressWarnings("unchecked")
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      String name = ((Entry<String, String>)element).getKey();

      switch(columnIndex)
      {
         case AssetView.NAME:
            return getName(name);
         case AssetView.VALUE:
            return ((Entry<String, String>)element).getValue();
         case AssetView.IS_MANDATORY:
            return isMandatory(name);
         case AssetView.IS_UNIQUE:
            return isUnique(name);
         case AssetView.SYSTEM_TYPE:
            return getSystemType(name);            
      }
      return null;
   }

   /**
    * Get name of the system type
    * 
    * @param name attribute name
    * @return system type name
    */
   public String getSystemType(String name)
   {
      return AssetManagementAttributeLabelProvider.SYSTEM_TYPE[session.getAssetManagementSchema().get(name).getSystemType().getValue()];
   }

   /**
    * Text representation if value is unique
    * 
    * @param name attribute name
    * @return Text representation if value is unique
    */
   public String isUnique(String name)
   {
      return session.getAssetManagementSchema().get(name).isUnique() ? i18n.tr("Yes") : i18n.tr("No");
   }

   /**
    * Text representation if value is mandatory
    * 
    * @param name attribute name
    * @return Text representation if value is mandatory
    */
   public String isMandatory(String name)
   {
      return session.getAssetManagementSchema().get(name).isMandatory() ? i18n.tr("Yes") : i18n.tr("No");
   }

   /**
    * @param name get name that should be displayed for user 
    * 
    * @return user displayed name 
    */
   public String getName(String name)
   {
      String displayName = session.getAssetManagementSchema().get(name).getDisplayName();
      return (displayName == null || displayName.isEmpty()) ? name : displayName;
   }
}
