/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.client.datacollection;

import java.io.File;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Downloaded file of completed DCI history export together with export statistics
 */
public class DataExportResult
{
   private final File file;
   private final int metricCount;
   private final long sampleCount;
   private final long fileSize;

   /**
    * Create result from server response and received file
    *
    * @param msg server response
    * @param file received file
    */
   public DataExportResult(NXCPMessage msg, File file)
   {
      this.file = file;
      metricCount = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ITEMS);
      sampleCount = msg.getFieldAsInt64(NXCPCodes.VID_NUM_RECORDS);
      fileSize = msg.getFieldAsInt64(NXCPCodes.VID_FILE_SIZE);
   }

   /**
    * @return received file (local temporary file)
    */
   public File getFile()
   {
      return file;
   }

   /**
    * @return number of exported metrics
    */
   public int getMetricCount()
   {
      return metricCount;
   }

   /**
    * @return number of exported sample rows
    */
   public long getSampleCount()
   {
      return sampleCount;
   }

   /**
    * @return size of downloaded file in bytes
    */
   public long getFileSize()
   {
      return fileSize;
   }
}
