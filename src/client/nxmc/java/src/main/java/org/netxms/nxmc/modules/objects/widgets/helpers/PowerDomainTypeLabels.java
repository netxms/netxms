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

import org.netxms.client.constants.PowerDomainType;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Localized labels for power domain types
 */
public final class PowerDomainTypeLabels
{
   private static final I18n i18n = LocalizationHelper.getI18n(PowerDomainTypeLabels.class);

   /**
    * Get label for given power domain type.
    *
    * @param type power domain type
    * @return localized label
    */
   public static String get(PowerDomainType type)
   {
      switch(type)
      {
         case GRID_ENTRY:
            return i18n.tr("Grid entry");
         case GENERATOR:
            return i18n.tr("Generator");
         case UPS:
            return i18n.tr("UPS");
         case PDU:
            return i18n.tr("PDU");
         case BUSWAY:
            return i18n.tr("Busway");
         default:
            return i18n.tr("Other");
      }
   }

   /**
    * Get labels for all power domain types, indexed by type value.
    *
    * @return array of localized labels
    */
   public static String[] getAll()
   {
      PowerDomainType[] types = PowerDomainType.values();
      String[] labels = new String[types.length];
      for(PowerDomainType type : types)
         labels[type.getValue()] = get(type);
      return labels;
   }
}
