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
package org.netxms.nxmc.base;

import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.services.PreferenceInitializer;

/**
 * Preference initializer for charts
 */
public class BasePreferenceInitializer implements PreferenceInitializer
{
   /**
    * @see org.netxms.nxmc.services.PreferenceInitializer#initializeDefaultPreferences(org.netxms.nxmc.PreferenceStore)
    */
   @Override
   public void initializeDefaultPreferences(PreferenceStore ps)
   {
      ps.setDefault("HTTP_PROXY_ENABLED", false); 
      ps.setDefault("HTTP_PROXY_SERVER", "");  
      ps.setDefault("HTTP_PROXY_PORT", "8000");  
      ps.setDefault("HTTP_PROXY_EXCLUSIONS", "localhost|127.0.0.1|10.*|192.168.*");  
      ps.setDefault("HTTP_PROXY_AUTH", false); 
      ps.setDefault("HTTP_PROXY_LOGIN", "");  
      ps.setDefault("HTTP_PROXY_PASSWORD", "");  

      ps.setDefault("DateFormatFactory.Format.DateTime", DateFormatFactory.DATETIME_FORMAT_SERVER); 
      ps.setDefault("DateFormatFactory.Format.Date", "dd.MM.yyyy"); 
      ps.setDefault("DateFormatFactory.Format.Time", "HH:mm:ss"); 
      ps.setDefault("DateFormatFactory.Format.ShortTime", "HH:mm"); 
      ps.setDefault("DateFormatFactory.UseServerTimeZone", false);       
   }
}
