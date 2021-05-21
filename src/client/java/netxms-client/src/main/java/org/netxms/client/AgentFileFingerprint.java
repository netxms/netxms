/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * File fingerprint received from agent
 */
public class AgentFileFingerprint
{
   private long size;
   private long crc32;
   private byte[] md5;
   private byte[] sha256;
   private byte[] data;

   /**
    * Create new agent file fingerprint object from NXCP message.
    * 
    * @param msg NXCP message
    */
   public AgentFileFingerprint(NXCPMessage msg)
   {
      size = msg.getFieldAsInt64(NXCPCodes.VID_FILE_SIZE);
      crc32 = msg.getFieldAsInt64(NXCPCodes.VID_HASH_CRC32);
      md5 = msg.getFieldAsBinary(NXCPCodes.VID_HASH_MD5);
      sha256 = msg.getFieldAsBinary(NXCPCodes.VID_HASH_SHA256);
      data = msg.getFieldAsBinary(NXCPCodes.VID_FILE_DATA);
   }

   /**
    * @return file size
    */
   public long getSize()
   {
      return size;
   }

   /**
    * Get CRC32 checksum of the file.
    *
    * @return file CRC32 checksum
    */
   public long getCRC32()
   {
      return crc32;
   }

   /**
    * Get MD5 hash of the file.
    * 
    * @return file MD5 hash
    */
   public byte[] getMD5()
   {
      return md5;
   }

   /**
    * Get SHA256 hash of the file.
    * 
    * @return file SHA256 hash
    */
   public byte[] getSHA256()
   {
      return sha256;
   }

   /**
    * Get first 64 bytes of the file.
    *
    * @return first 64 bytes of the file
    */
   public byte[] getFileData()
   {
      return data;
   }
}
