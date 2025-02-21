/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.tools;

import java.net.MalformedURLException;
import java.net.URL;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.UrlLauncher;
import org.eclipse.swt.widgets.Display;

/**
 * Helper class to interact with external web browser
 */
public class ExternalWebBrowser
{
   /**
    * Open given URL in external web browser
    *
    * @param url URL to open
    */
   public static void open(URL url)
   {
      open(url.toString());
   }

   /**
    * Open given URL in external web browser
    *
    * @param url URL to open
    */
   public static void open(String url)
   {
      UrlLauncher launcher = RWT.getClient().getService(UrlLauncher.class);
      launcher.openURL(url);
   }

   /**
    * Get local address that can be used for connecting to this client. It is always 127.0.0.1 for desktop client but can be
    * different for web client.
    * 
    * @param display current display
    * @return local address that can be used for connecting to this client
    */
   public static String getLocalAddress(Display display)
   {
      final String[] address = new String[1];
      display.syncExec(() -> {
         try
         {
            address[0] = new URL(RWT.getRequest().getRequestURL().toString()).getHost();
         }
         catch(MalformedURLException e)
         {
            address[0] = "127.0.0.1";
         }
      });
      return address[0];
   }
}
