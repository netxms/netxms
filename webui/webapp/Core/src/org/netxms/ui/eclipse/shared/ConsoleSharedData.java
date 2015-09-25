/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.shared;

import java.util.TimeZone;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.ClientInfo;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;

/**
 * Compatibility class for RCP plugins
 */
public class ConsoleSharedData
{
   public static final String ATTRIBUTE_SESSION = "netxms.session";
   public static final String ATTRIBUTE_TIMEZONE = "netxms.timezone";
   
	/**
	 * Get NetXMS session
	 * 
	 * @return
	 */
	public static NXCSession getSession()
	{
		return (NXCSession)RWT.getUISession().getAttribute(ATTRIBUTE_SESSION);
	}
	
	/**
	 * Get client timezone
	 * 
	 * @return
	 */
	public static TimeZone getTimeZone()
	{
      return (TimeZone)RWT.getUISession().getAttribute(ATTRIBUTE_TIMEZONE);
	}

   /**
    * Set server time zone
    */
   public static void setServerTimeZone()
   {
      NXCSession session = getSession();
      if (session != null)
      {
         String tz = session.getServerTimeZone();
         RWT.getUISession().setAttribute(ATTRIBUTE_TIMEZONE, TimeZone.getTimeZone(tz.replaceAll("[A-Za-z]+([\\+\\-][0-9]+).*", "GMT$1"))); //$NON-NLS-1$ //$NON-NLS-2$
      }
   }
   
   /**
    * Reset time zone to default
    */
   public static void resetTimeZone()
   {
      ClientInfo clientInfo = RWT.getClient().getService(ClientInfo.class);
      if (clientInfo != null)
      {
         String[] tzList = TimeZone.getAvailableIDs(-clientInfo.getTimezoneOffset() * 60000);  // convert offset in minutes to milliseconds (in RAP - means ahead of UTC)
         if (tzList.length > 0)
         {
            RWT.getUISession().setAttribute(ATTRIBUTE_TIMEZONE, TimeZone.getTimeZone(tzList[0]));
         }
      }
   }
	
	/**
	 * Get value of console property
	 * 
	 * @param name name of the property
	 * @return property value or null
	 */
	public static Object getProperty(final String name)
	{
		return RWT.getUISession().getAttribute("netxms." + name);
	}
	
   /**
    * @param display
    * @param name
    * @return
    */
   public static Object getProperty(Display display, final String name)
   {
      return (display != null) ? RWT.getUISession(display).getAttribute("netxms." + name) : null;
   }
   
	/**
	 * Set value of console property
	 * 
	 * @param name name of the property
	 * @param value new property value
	 */
	public static void setProperty(final String name, final Object value)
	{
		RWT.getUISession().setAttribute("netxms." + name, value);
	}

   /**
    * Set value of console property
    * 
    * @param display
    * @param name
    * @param value
    */
   public static void setProperty(Display display, final String name, final Object value)
   {
      RWT.getUISession(display).setAttribute("netxms." + name, value);
   }
}
