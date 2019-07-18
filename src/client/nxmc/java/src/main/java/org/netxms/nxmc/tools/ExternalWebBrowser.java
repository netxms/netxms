/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
      String href = url.toString();
      if (href.startsWith("file:"))
      {
         href = href.substring(5);
         while (href.startsWith("/"))
         {
            href = href.substring(1);
         }
         href = "file:///" + href;
      }

      if (Util.isWindows())
      {
         Program.launch(href);
      } 
      else if (Util.isMac())
      {
         try 
         {
            Runtime.getRuntime().exec("/usr/bin/open '" + href + "'");
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
            Runtime.getRuntime().exec("firefox " + urlEncode(href)); // FIXME: detect or configure other browsers
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
}
