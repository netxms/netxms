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
package org.netxms.client;

import java.io.File;

/**
 * Class that returns received file and
 */
public class ReceivedFile
{
   public static final int SUCCESS = 0;
   public static final int CANCELED = 1;
   public static final int FAILED = 2;
   public static final int TIMEOUT = 3;

   private File file;
   private int status;

   /**
    * Create new received file handle
    *
    * @param f      TODO
    * @param status TODO
    */
   public ReceivedFile(File f, int status)
   {
      file = f;
      this.status = status;
   }

   /**
    * Get status of receive operation
    *
    * @return the status
    */
   public int getStatus()
   {
      return status;
   }

   /**
    * Get received file
    *
    * @return the file
    */
   public File getFile()
   {
      return file;
   }

   /**
    * Method that checks if error should be generated
    *
    * @return true if file transfer operation has failed
    */
   public boolean isFailed()
   {
      return status == FAILED || status == TIMEOUT;
   }
}
