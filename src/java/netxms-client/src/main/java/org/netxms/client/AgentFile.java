/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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

import java.io.File;

/**
 * Information about file received from agent
 */
public class AgentFile
{
   private String id;
   private File file;

   /**
    * Create new agent file object
    * 
    * @param id
    * @param file
    */
   public AgentFile(String id, File file)
   {
      this.id = id;
      this.file = file;
   }

   /**
    * @return the id
    */
   public String getId()
   {
      return id;
   }

   /**
    * @return the file
    */
   public File getFile()
   {
      return file;
   }
}
