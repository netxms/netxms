/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Text;

/**
 * File in log parser rule
 */
public class LogParserFile
{
   @Text(required=false)
   private String file = null;
   
   @Attribute(required=false)
   private String encoding = null;

   /**
    * @return
    */
   public String getFile()
   {
      return file;
   }
   
   /**
    * @return
    */
   public String getEncoding()
   {
      return encoding;
   }
   
   /**
    * @param file
    */
   public void setFile(String file)
   {
      this.file = file;
   }
   
   /**
    * @param encoding
    */
   public void setEncoding(String encoding)
   {
      this.encoding = encoding;
   }
}
