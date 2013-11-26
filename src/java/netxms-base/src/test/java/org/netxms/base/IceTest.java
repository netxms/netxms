/**
 * 
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
