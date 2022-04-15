/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.client.server;

import java.util.Date;

/**
 * Generic interface for any type of remote (in relation to client) file
 */
public interface RemoteFile
{
   /**
    * Get file name.
    *
    * @return file name
    */
   public String getName();

   /**
    * Get file extension.
    *
    * @return file extension
    */
   public String getExtension();

   /**
    * Get file modification time.
    *
    * @return file modification time
    */
   public Date getModificationTime();

   /**
    * Get file size in bytes.
    *
    * @return file size in bytes
    */
   public long getSize();

   /**
    * Check if this file object represents directory.
    *
    * @return true if this file object represents directory
    */
   public boolean isDirectory();

   /**
    * Check if this file object is a placeholder (does not correspond to actual remote file).
    *
    * @return true if this file object is a placeholder
    */
   public boolean isPlaceholder();
}
