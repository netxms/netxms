/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.util.Arrays;
import java.util.Random;
import junit.framework.TestCase;

/**
 * Tests for ICE encryption algorithm
 */
public class IceTest extends TestCase
{
   private static final byte[] TEST_DATA = "lorem ipsum".getBytes();
   
   public void testIce()
   {
      byte[] key = new byte[16];
      Random r = new Random(0);
      r.nextBytes(key);
      
      byte[] encrypted = IceCrypto.encrypt(TEST_DATA, key);
      byte[] decrypted = Arrays.copyOf(IceCrypto.decrypt(encrypted, key), TEST_DATA.length);
      assertTrue(Arrays.equals(decrypted, TEST_DATA));
   }
   
   public void testEncryptedPasswords() throws Exception
   {
      String encrypted = EncryptedPassword.encrypt("admin", "SomePassword");
      String decrypted = EncryptedPassword.decrypt("admin", encrypted);
      assertEquals(decrypted, "SomePassword");
   }
}
