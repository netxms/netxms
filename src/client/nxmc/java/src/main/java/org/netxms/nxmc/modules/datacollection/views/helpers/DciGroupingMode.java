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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.netxms.nxmc.localization.LocalizationHelper;

/**
 * Grouping modes for data collection views
 */
public enum DciGroupingMode
{
   NONE,
   TAG,
   ORIGIN,
   STATUS,
   TEMPLATE,
   INSTANCE_DISCOVERY,
   RELATED_OBJECT;

   /**
    * Get display name of grouping mode.
    *
    * @return display name
    */
   public String getDisplayName()
   {
      switch(this)
      {
         case INSTANCE_DISCOVERY:
            return LocalizationHelper.getI18n(DciGroupingMode.class).tr("&Instance discovery");
         case ORIGIN:
            return LocalizationHelper.getI18n(DciGroupingMode.class).tr("&Origin");
         case RELATED_OBJECT:
            return LocalizationHelper.getI18n(DciGroupingMode.class).tr("&Related object");
         case STATUS:
            return LocalizationHelper.getI18n(DciGroupingMode.class).tr("&Status");
         case TAG:
            return LocalizationHelper.getI18n(DciGroupingMode.class).tr("T&ag");
         case TEMPLATE:
            return LocalizationHelper.getI18n(DciGroupingMode.class).tr("&Template");
         default:
            return LocalizationHelper.getI18n(DciGroupingMode.class).tr("&None");
      }
   }

   /**
    * Get grouping mode by name, falling back to NONE for unknown names.
    *
    * @param name mode name
    * @return grouping mode
    */
   public static DciGroupingMode getByName(String name)
   {
      if (name != null)
      {
         for(DciGroupingMode m : values())
            if (m.name().equals(name))
               return m;
      }
      return NONE;
   }
}
