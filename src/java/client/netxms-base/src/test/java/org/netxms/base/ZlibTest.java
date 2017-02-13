/**
 * 
 */
package org.netxms.base;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.util.Arrays;
import com.jcraft.jzlib.Deflater;
import com.jcraft.jzlib.DeflaterOutputStream;
import com.jcraft.jzlib.InflaterInputStream;
import com.jcraft.jzlib.JZlib;
import junit.framework.TestCase;

/**
 * Tests for bundled ZLib implementation
 *
 */
public class ZlibTest extends TestCase
{
   private static final String TEXT = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
   
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
