package org.netxms.certificate;

import org.junit.Before;
import org.junit.Test;

import java.io.FileInputStream;
import java.security.KeyStore;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.greaterThan;

public class KeyStoreTest
{
   private final TestListener testListener = new TestListener();
   private KeyStore keyStore;

   @Before
   public void setUp() throws Exception
   {
      keyStore = KeyStore.getInstance("PKCS12");

      FileInputStream fis = new FileInputStream(testListener.keyStoreLocationRequested());

      keyStore.load(fis, null);

      fis.close();
   }

   @Test
   public void testKeyStore_NotEmpty() throws Exception
   {
      int numCerts = keyStore.size();

      assertThat(numCerts, greaterThan(0));
   }
}
