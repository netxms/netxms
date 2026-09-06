/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import org.netxms.client.constants.CoolingZoneType;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Localized labels for cooling zone types
 */
public final class CoolingZoneTypeLabels
{
   private static final I18n i18n = LocalizationHelper.getI18n(CoolingZoneTypeLabels.class);

   /**
    * Get label for given cooling zone type.
    *
    * @param type cooling zone type
    * @return localized label
    */
   public static String get(CoolingZoneType type)
   {
      switch(type)
      {
         case PLANT:
            return i18n.tr("Cooling plant");
         case ZONE:
            return i18n.tr("Thermal zone");
         default:
            return i18n.tr("Other");
      }
   }

   /**
    * Get labels for all cooling zone types, indexed by type value.
    *
    * @return array of localized labels
    */
   public static String[] getAll()
   {
      CoolingZoneType[] types = CoolingZoneType.values();
      String[] labels = new String[types.length];
      for(CoolingZoneType type : types)
         labels[type.getValue()] = get(type);
      return labels;
   }
}
