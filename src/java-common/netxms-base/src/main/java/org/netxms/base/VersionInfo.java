/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.base;

import java.util.MissingResourceException;
import java.util.PropertyResourceBundle;
import java.util.ResourceBundle;

/**
 * NetXMS version information
 */
public final class VersionInfo
{
   /**
    * Private constructor
    */
   private VersionInfo()
   {
   }

   /**
    * Read property form specific bundle
    *
    * @param bundleName bundle name
    * @param propertyName property name
    * @return property value or null
    */
   private static String readPropertyFromBundle(String bundleName, String propertyName)
   {
      try
      {
         ResourceBundle bundle = PropertyResourceBundle.getBundle(bundleName);
         try
         {
            return bundle.getString(propertyName);
         }
         catch(MissingResourceException e)
         {
            return null;
         }
      }
      catch(Exception e)
      {
         return null;
      }
   }

   /**
    * Read property from resource bundle
    *
    * @param propertyName property name
    * @param defaultValue default value if cannot be read
    * @return property value or default value
    */
   private static String readProperty(String propertyName, String defaultValue)
   {
      String value = readPropertyFromBundle("netxms-build-tag", propertyName);
      if (value == null)
         value = readPropertyFromBundle("netxms-version", propertyName);
      return (value != null) ? value : defaultValue;
   }

   /**
    * Get product version
    *
    * @return product version
    */
   public static String version()
   {
      return readProperty("NETXMS_VERSION", "0.1-SNAPSHOT");
   }

   /**
    * Get product base version
    *
    * @return product base version
    */
   public static String baseVersion()
   {
      return readProperty("NETXMS_BASE_VERSION", "0.1");
   }

   /**
    * Get product version qualifier (rc, sp, etc.)
    *
    * @return product version qualifier
    */
   public static String qualifier()
   {
      return readProperty("NETXMS_VERSION_QUALIFIER", "");
   }

   /**
    * Get product build tag
    *
    * @return product build tag
    */
   public static String buildTag()
   {
      return readProperty("NETXMS_BUILD_TAG", "untagged");
   }
}
