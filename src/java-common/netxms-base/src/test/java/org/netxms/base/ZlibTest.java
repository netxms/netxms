/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.base;

import static org.junit.jupiter.api.Assertions.assertTrue;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.util.Arrays;
import org.junit.jupiter.api.Test;
import com.jcraft.jzlib.Deflater;
import com.jcraft.jzlib.DeflaterOutputStream;
import com.jcraft.jzlib.InflaterInputStream;
import com.jcraft.jzlib.JZlib;

/**
 * Tests for bundled ZLib implementation
 */
public class ZlibTest
{
   private static final String TEXT = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
   
   @Test
   public void testCompression() throws Exception
   {
      ByteArrayOutputStream bytesOut = new ByteArrayOutputStream();
      bytesOut.write(new byte[] { 0x01, 0x02, 0x03, 0x04 });
      DeflaterOutputStream zout = new DeflaterOutputStream(bytesOut, new Deflater(JZlib.Z_BEST_COMPRESSION));
      byte[] bytes = TEXT.getBytes();
      zout.write(bytes);
      zout.close();
      byte[] zbytes = bytesOut.toByteArray();
      ByteArrayInputStream bytesIn = new ByteArrayInputStream(zbytes);
      bytesIn.skip(4);
      InflaterInputStream zin = new InflaterInputStream(bytesIn);
      byte[] dbytes = new byte[bytes.length];
      DataInputStream din = new DataInputStream(zin);
      din.readFully(dbytes);
      assertTrue(Arrays.equals(bytes, dbytes));
      zin.close();
      System.out.println(String.format("Size: clear text %d, compressed %d", bytes.length, zbytes.length));
   }
}
