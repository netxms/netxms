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

import java.io.IOException;
import java.net.URL;
import org.eclipse.jface.util.Util;
import org.eclipse.swt.program.Program;
import org.eclipse.swt.widgets.Display;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Helper class to interact with external web browser
 */
public class ExternalWebBrowser
{
   private static Logger logger = LoggerFactory.getLogger(ExternalWebBrowser.class);

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
      if (url.startsWith("file:"))
      {
         url = url.substring(5);
         while (url.startsWith("/"))
         {
            url = url.substring(1);
         }
         url = "file:///" + url;
      }

      if (Util.isWindows())
      {
         Program.launch(url);
      } 
      else if (Util.isMac())
      {
         try 
         {
            Runtime.getRuntime().exec("/usr/bin/open '" + url + "'");
         } 
         catch (IOException e)
         {
            logger.error("Exception while starting external browser", e);
         }
      }
      else
      {
         try
         {
            Runtime.getRuntime().exec("xdg-open " + urlEncode(url));
         }
         catch(IOException e)
         {
            logger.error("Exception while starting external browser", e);
         }
      }
   }

   /**
    * This method encodes the URL removing spaces and replacing them with <code>"%20"</code>.
    */
   private static String urlEncode(String url)
   {
      StringBuilder sb = new StringBuilder(url.length());
      for(char c : url.toCharArray())
      {
         if (c == ' ')
         {
            sb.append("%20");
         }
         else
         {
            sb.append(c);
         }
      }
      return sb.toString();
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
      return "127.0.0.1";
   }
}
