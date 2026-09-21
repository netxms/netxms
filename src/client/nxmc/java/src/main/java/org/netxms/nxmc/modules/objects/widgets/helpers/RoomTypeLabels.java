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

import org.netxms.client.constants.RoomType;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Localized labels for room types
 */
public final class RoomTypeLabels
{
   /**
    * Get label for given room type.
    *
    * @param type room type
    * @return localized label
    */
   public static String get(RoomType type)
   {
      return get(type, LocalizationHelper.getI18n(RoomTypeLabels.class));
   }

   /**
    * Get label for given room type using provided I18n instance.
    *
    * @param type room type
    * @param i18n I18n instance for current locale
    * @return localized label
    */
   private static String get(RoomType type, I18n i18n)
   {
      switch(type)
      {
         case COMPUTER_ROOM:
            return i18n.tr("Computer room");
         case ELECTRICAL:
            return i18n.tr("Electrical room");
         case MECHANICAL:
            return i18n.tr("Mechanical room");
         case TELECOM:
            return i18n.tr("Telecom room");
         default:
            return i18n.tr("Other");
      }
   }

   /**
    * Get labels for all room types, indexed by type value.
    *
    * @return array of localized labels
    */
   public static String[] getAll()
   {
      RoomType[] types = RoomType.values();
      String[] labels = new String[types.length];
      I18n i18n = LocalizationHelper.getI18n(RoomTypeLabels.class);
      for(RoomType type : types)
         labels[type.getValue()] = get(type, i18n);
      return labels;
   }
}
