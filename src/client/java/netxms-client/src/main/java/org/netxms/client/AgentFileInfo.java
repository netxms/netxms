/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
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

/**
 * Information about Agent file
 */
public class AgentFileInfo
{
   private String remoteName;
   private long itemCount;
   private long size;
   
   /**
    * Create new agent file info object
    * 
    * @param remoteName remote file name
    * @param itemCount number of sub-elements
    * @param size file size
    */
   public AgentFileInfo(String remoteName, long itemCount, long size)
   {
      this.remoteName = remoteName;
      this.itemCount = itemCount;
      this.size = size;
   }
   
   /**
    * @return file size
    */
   public long getSize()
   {
      return size;
   }
   
   /**
    * @return item count
    */
   public long getItemCount()
   {
      return itemCount;
   }
   
   /**
    * @return name of file
    */
   public String getName()
   {
      return remoteName;
   }
}
