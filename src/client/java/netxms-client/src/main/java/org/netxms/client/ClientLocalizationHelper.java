/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2023 Raden Solutions
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
package org.netxms.client;

import java.util.Locale;
import java.util.PropertyResourceBundle;
import java.util.ResourceBundle;

/**
 * Helper to get localized message 
 */
public class ClientLocalizationHelper
{
   /**
    * Get message text for given text
    * 
    * @param key text to translate
    * @return message text
    */
   public static String getText(String key)
   {
      return getText(key, Locale.getDefault(), key);
   }

   /**
    * Get message text for given RCC
    * 
    * @param key request completion code
    * @param lang language code
    * @return message text
    */
   public static String getText(String key, String lang)
   {
      return getText(key, new Locale(lang), key);
   }

   /**
    * Get message text for given text
    * 
    * @param key request completion code
    * @param locale locale
    * @return message text
    */
   public static String getText(String key, Locale locale)
   {
      return getText(key, locale, key);
   }

   /**
    * Get message text for given text
    * 
    * @param key request completion code
    * @param locale locale
    * @param defaultValue default value if mesage is not found
    * @return message text
    */
   public static String getText(String key, Locale locale, String defaultValue)
   {
      try
      {
         ResourceBundle bundle = PropertyResourceBundle.getBundle("netxms-client-messages", locale);
         return bundle.getString(key);
      }
      catch(Exception e)
      {
         return defaultValue;
      }
   }
}
