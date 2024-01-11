/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.localization;

import java.util.Locale;
import org.eclipse.rap.rwt.RWT;
import org.xnap.commons.i18n.I18n;
import org.xnap.commons.i18n.I18nFactory;

/**
 * Helper class for localization tasks
 */
public final class LocalizationHelper
{
   /**
    * Prevent construction
    */
   private LocalizationHelper()
   {
   }

   /**
    * Get I18n object for given class
    *
    * @param c Java class
    * @return I18n object for translation
    */
   public static I18n getI18n(Class<?> c)
   {
      return I18nFactory.getI18n(c, RWT.getLocale(), I18nFactory.FALLBACK);
   }

   /**
    * Get user's locale.
    *
    * @return user's locale
    */
   public static Locale getLocale()
   {
      return RWT.getLocale();
   }
}
