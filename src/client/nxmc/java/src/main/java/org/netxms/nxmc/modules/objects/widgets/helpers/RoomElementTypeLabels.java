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

import org.netxms.client.constants.RoomElementType;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Localized labels for room passive element types
 */
public final class RoomElementTypeLabels
{
   /**
    * Get label for given room passive element type.
    *
    * @param type room passive element type
    * @return localized label
    */
   public static String get(RoomElementType type)
   {
      return get(type, LocalizationHelper.getI18n(RoomElementTypeLabels.class));
   }

   /**
    * Get label for given room passive element type using provided I18n instance.
    *
    * @param type room passive element type
    * @param i18n I18n instance for current locale
    * @return localized label
    */
   private static String get(RoomElementType type, I18n i18n)
   {
      switch(type)
      {
         case COLUMN:
            return i18n.tr("Column");
         case WALL:
            return i18n.tr("Wall");
         case RAMP:
            return i18n.tr("Ramp");
         case STAIRS:
            return i18n.tr("Stairs");
         case DOOR:
            return i18n.tr("Door");
         default:
            return i18n.tr("Other");
      }
   }

   /**
    * Get labels for all room passive element types, indexed by type value.
    *
    * @return array of localized labels
    */
   public static String[] getAll()
   {
      RoomElementType[] types = RoomElementType.values();
      String[] labels = new String[types.length];
      I18n i18n = LocalizationHelper.getI18n(RoomElementTypeLabels.class);
      for(RoomElementType type : types)
         labels[type.getValue()] = get(type, i18n);
      return labels;
   }
}
